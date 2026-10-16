/*
 * Author: Runor Ewhro
 * Description: requests a link's page, stops reading once the title has
 *              arrived or the size cap is reached, and turns the decoded title
 *              into the entry's subtext.
 */
#include "LinkTitleController.hpp"

#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QTextDocumentFragment>

#include <algorithm>
#include <array>

namespace kstack::desktop {
namespace {

// the title sits in <head>, so a page that has not shown one by here
// is not going to
constexpr qsizetype maxBytes = 256 * 1024;
constexpr int timeoutMs = 8000;
constexpr int maxRedirects = 5;
// a row shows one line; anything past this is elided anyway
constexpr qsizetype maxTitleLength = 300;

// Reject local names before DNS. Resolved addresses are checked separately.
bool isLocalHost(const QString& host) {
    if (host.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0 ||
        host.endsWith(QLatin1String(".localhost"), Qt::CaseInsensitive) ||
        host.endsWith(QLatin1String(".local"), Qt::CaseInsensitive)) {
        return true;
    }
    return false;
}

QUrl fetchableUrl(const std::string& text) {
    const QString flat = QString::fromStdString(text).trimmed();
    const QString href = flat.startsWith(QLatin1String("www."), Qt::CaseInsensitive)
                             ? QStringLiteral("https://") + flat
                             : flat;
    const QUrl url(href, QUrl::StrictMode);
    const QString scheme = url.scheme().toLower();
    const bool secure = scheme == QLatin1String("https");
    const int port = url.port(secure ? 443 : 80);
    if (!url.isValid() || (scheme != QLatin1String("http") && scheme != QLatin1String("https")) ||
        url.host().isEmpty() || isLocalHost(url.host()) || !url.userName().isEmpty() ||
        !url.password().isEmpty() || port != (secure ? 443 : 80)) {
        return {};
    }
    return url;
}

bool hasClosingTitle(const QByteArray& html) {
    return html.contains("</title>") || html.contains("</TITLE>") || html.contains("</Title>");
}

QByteArray charsetIn(const QByteArray& text) {
    static const QRegularExpression pattern(QStringLiteral(R"(charset\s*=\s*["']?([A-Za-z0-9._:-]+))"),
                                            QRegularExpression::CaseInsensitiveOption);
    const auto match = pattern.match(QString::fromLatin1(text));
    return match.hasMatch() ? match.captured(1).toLatin1() : QByteArray();
}

} // namespace

LinkTitleController::LinkTitleController(const core::HistoryService& history,
                                         EntrySubtexts& subtexts, QObject* parent)
    : QObject(parent), history_(history), subtexts_(subtexts) {
    // A proxy could reconnect by hostname after we validated a DNS answer.
    network_.setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
}

bool LinkTitleController::isPublicAddress(const QHostAddress& address) {
    bool ipv4 = false;
    const quint32 bits = address.toIPv4Address(&ipv4);
    if (ipv4) {
        const QHostAddress normalized(bits);
        // Public routes only. Qt's isGlobal() includes RFC 1918 addresses,
        // so use explicit exclusions that also cover shared and test ranges.
        static const std::array<std::pair<QHostAddress, int>, 14> excluded{{
            {QHostAddress(QStringLiteral("0.0.0.0")), 8},
            {QHostAddress(QStringLiteral("10.0.0.0")), 8},
            {QHostAddress(QStringLiteral("100.64.0.0")), 10},
            {QHostAddress(QStringLiteral("127.0.0.0")), 8},
            {QHostAddress(QStringLiteral("169.254.0.0")), 16},
            {QHostAddress(QStringLiteral("172.16.0.0")), 12},
            {QHostAddress(QStringLiteral("192.0.0.0")), 24},
            {QHostAddress(QStringLiteral("192.0.2.0")), 24},
            {QHostAddress(QStringLiteral("192.88.99.0")), 24},
            {QHostAddress(QStringLiteral("192.168.0.0")), 16},
            {QHostAddress(QStringLiteral("198.18.0.0")), 15},
            {QHostAddress(QStringLiteral("198.51.100.0")), 24},
            {QHostAddress(QStringLiteral("203.0.113.0")), 24},
            {QHostAddress(QStringLiteral("224.0.0.0")), 4},
        }};
        if (std::any_of(excluded.begin(), excluded.end(), [&normalized](const auto& subnet) {
                return normalized.isInSubnet(subnet.first, subnet.second);
            })) {
            return false;
        }
        return !normalized.isInSubnet(QHostAddress(QStringLiteral("240.0.0.0")), 4);
    }
    if (address.protocol() != QAbstractSocket::IPv6Protocol) {
        return false;
    }
    // Accept global unicast, excluding documentation and tunnel ranges.
    return address.isInSubnet(QHostAddress(QStringLiteral("2000::")), 3) &&
           !address.isInSubnet(QHostAddress(QStringLiteral("2001::")), 32) &&
           !address.isInSubnet(QHostAddress(QStringLiteral("2001:db8::")), 32) &&
           !address.isInSubnet(QHostAddress(QStringLiteral("2002::")), 16);
}

void LinkTitleController::entryCaptured(const core::ClipboardEntry& entry) {
    if (entry.kind == core::ContentKind::Link) {
        fetch(fetchableUrl(entry.text), entry.id, maxRedirects);
    }
}

void LinkTitleController::fetch(const QUrl& url, std::uint64_t id, int redirectsRemaining) {
    if (url.isEmpty() || !entryExists(id)) {
        return;
    }
    const QHostAddress literal(url.host());
    if (!literal.isNull()) {
        if (isPublicAddress(literal)) {
            fetchAt(url, literal, id, redirectsRemaining);
        }
        return;
    }
    // Resolve asynchronously, then connect to the validated IP itself. A
    // second name lookup by the HTTP client would reopen a DNS rebinding gap.
    QHostInfo::lookupHost(url.host(), this, [this, url, id, redirectsRemaining](const QHostInfo& info) {
        if (info.error() != QHostInfo::NoError || info.addresses().isEmpty() || !entryExists(id)) {
            return;
        }
        const auto& addresses = info.addresses();
        if (std::any_of(addresses.begin(), addresses.end(), [](const QHostAddress& address) {
                return !isPublicAddress(address);
            })) {
            return;
        }
        fetchAt(url, addresses.front(), id, redirectsRemaining);
    });
}

QNetworkRequest LinkTitleController::requestForAddress(const QUrl& url,
                                                       const QHostAddress& address) {
    QUrl endpoint = url;
    endpoint.setHost(address.toString());
    endpoint.setFragment({});
    QNetworkRequest request(endpoint);
    // The connection is made to this IP, while Host and TLS verification keep
    // the authority from the copied URL. HTTP/1 avoids an IP :authority.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    request.setPeerVerifyName(url.host());
    request.setRawHeader("Host", url.authority(QUrl::FullyEncoded).toLatin1());
    request.setTransferTimeout(timeoutMs);
    request.setRawHeader("Accept", "text/html,application/xhtml+xml;q=0.9");
    // Do not attach or save cookies. The request still exposes its URL and IP.
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    return request;
}

void LinkTitleController::fetchAt(const QUrl& url, const QHostAddress& address, std::uint64_t id,
                                  int redirectsRemaining) {
    if (!isPublicAddress(address)) {
        return;
    }
    QNetworkReply* reply = network_.get(requestForAddress(url, address));
    auto* body = new QByteArray;
    // images, PDFs, and downloads have no title worth waiting for
    connect(reply, &QNetworkReply::metaDataChanged, reply, [reply] {
        const QString type = reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
        if (!type.isEmpty() && !type.contains(QLatin1String("html"))) {
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::readyRead, reply, [reply, body] {
        body->append(reply->read(maxBytes - body->size()));
        if (body->size() >= maxBytes || hasClosingTitle(*body)) {
            reply->close();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, body, id, url, redirectsRemaining] {
        const QByteArray contentType = reply->rawHeader("Content-Type");
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray location = reply->rawHeader("Location");
        reply->deleteLater();
        const QByteArray html = std::move(*body);
        delete body;
        // the entry may have been removed or cleared while it loaded
        if (!entryExists(id)) {
            return;
        }
        if (status >= 300 && status < 400 && !location.isEmpty()) {
            if (redirectsRemaining > 0) {
                // Resolve relative Location headers against the original
                // hostname, not the vetted IP in the transport request.
                const QUrl target = QUrl::fromEncoded(location, QUrl::StrictMode);
                if (!target.isValid() || target.isEmpty()) {
                    return;
                }
                const QUrl next = url.resolved(target);
                if (!(url.scheme() == QLatin1String("https") && next.scheme() != QLatin1String("https"))) {
                    fetch(fetchableUrl(next.toString(QUrl::FullyEncoded).toUtf8().toStdString()), id,
                          redirectsRemaining - 1);
                }
            }
            return;
        }
        const QString title = titleFromHtml(html, contentType);
        if (!title.isEmpty()) {
            subtexts_.set(id, title);
        }
    });
}

QString LinkTitleController::titleFromHtml(const QByteArray& html, const QByteArray& contentType) {
    static const QRegularExpression pattern(QStringLiteral(R"(<title\b[^>]*>(.*?)</title\s*>)"),
                                            QRegularExpression::CaseInsensitiveOption |
                                                QRegularExpression::DotMatchesEverythingOption);
    // the header's charset wins, then the page's own <meta>, then UTF-8
    QByteArray charset = charsetIn(contentType);
    if (charset.isEmpty()) {
        charset = charsetIn(html.left(4096));
    }
    QStringDecoder decoder(charset.isEmpty() ? "UTF-8" : charset.constData());
    if (!decoder.isValid()) {
        decoder = QStringDecoder(QStringDecoder::Utf8);
    }
    const QString text = decoder.decode(html);
    const auto match = pattern.match(text);
    if (!match.hasMatch()) {
        return {};
    }
    // titles carry entities such as &amp; and &#8211;
    return QTextDocumentFragment::fromHtml(match.captured(1)).toPlainText().simplified().left(maxTitleLength);
}

bool LinkTitleController::entryExists(std::uint64_t id) const {
    const auto& entries = history_.entries();
    return std::any_of(entries.begin(), entries.end(), [id](const auto& item) { return item.id == id; });
}

} // namespace kstack::desktop
