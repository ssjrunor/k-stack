/*
 * Author: Runor Ewhro
 * Description: checks that a page title is found, decoded, and flattened to
 *              one line, and that local links are never fetched. With
 *              KSTACK_LINK_TITLE_URL set, it also fetches that link as the
 *              app would and prints the subtext it gets.
 */
#include "controllers/LinkTitleController.hpp"

#include <QGuiApplication>
#include <QTimer>

#include <iostream>
#include <string>

namespace {
using kstack::desktop::EntrySubtexts;
using kstack::desktop::LinkTitleController;

int failures = 0;

void requireTitle(const QByteArray& html, const QByteArray& contentType, const QString& expected) {
    const QString actual = LinkTitleController::titleFromHtml(html, contentType);
    if (actual != expected) {
        std::cerr << "title of " << html.toStdString() << ": expected \"" << expected.toStdString()
                  << "\", got \"" << actual.toStdString() << "\"\n";
        ++failures;
    }
}

void checkParsing() {
    requireTitle("<html><head><title>Example Domain</title></head>", {}, QStringLiteral("Example Domain"));
    requireTitle("<TITLE lang=\"en\">\n  Upper\n  case\n</TITLE>", {}, QStringLiteral("Upper case"));
    requireTitle("<title>Fish &amp; Chips &#8211; Menu</title>", {}, QStringLiteral("Fish & Chips – Menu"));
    requireTitle("<title>Caf\xc3\xa9</title>", "text/html; charset=utf-8", QStringLiteral("Café"));
    requireTitle("<meta charset=\"iso-8859-1\"><title>Caf\xe9</title>", {}, QStringLiteral("Café"));
    // an <svg> <title> or a missing one gives nothing past the head's
    requireTitle("<head><title></title></head>", {}, QString());
    requireTitle("<p>no title here</p>", {}, QString());
}

void checkAddressGate() {
    for (const char* text : {"0.0.0.0", "10.1.2.3", "100.64.1.1", "127.0.0.1",
                             "169.254.1.2", "172.16.0.1", "192.168.1.1", "198.18.0.1",
                             "224.0.0.1", "255.255.255.255", "::1", "fe80::1",
                             "fc00::1", "::ffff:127.0.0.1", "2001:db8::1"}) {
        if (LinkTitleController::isPublicAddress(QHostAddress(QString::fromLatin1(text)))) {
            std::cerr << "non-public address accepted: " << text << '\n';
            ++failures;
        }
    }
    for (const char* text : {"1.1.1.1", "8.8.8.8", "2606:4700:4700::1111"}) {
        if (!LinkTitleController::isPublicAddress(QHostAddress(QString::fromLatin1(text)))) {
            std::cerr << "public address rejected: " << text << '\n';
            ++failures;
        }
    }
}

void checkPinnedRequest() {
    const QUrl original(QStringLiteral("https://example.com/private?token=abc#section"));
    const auto request = LinkTitleController::requestForAddress(
        original, QHostAddress(QStringLiteral("1.1.1.1")));
    if (request.url().host() != QLatin1String("1.1.1.1") ||
        request.url().path() != original.path() || request.url().query() != original.query() ||
        !request.url().fragment().isEmpty() || request.rawHeader("Host") != "example.com" ||
        request.peerVerifyName() != QLatin1String("example.com") ||
        request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt() !=
            QNetworkRequest::ManualRedirectPolicy) {
        std::cerr << "link request did not keep its vetted IP and original authority\n";
        ++failures;
    }
}

void checkLocalLinksSkipped(QGuiApplication& app) {
    kstack::core::HistoryService history;
    EntrySubtexts subtexts;
    LinkTitleController controller(history, subtexts);
    for (const char* link : {"http://localhost:8080/admin", "http://192.168.1.1/", "http://10.0.0.5/x",
                             "http://printer.local/"}) {
        const auto entry = history.add(link);
        if (!entry) {
            continue;
        }
        controller.entryCaptured(*entry);
    }
    // anything fetched would have to come back through the event loop
    QTimer::singleShot(200, &app, &QCoreApplication::quit);
    app.exec();
    for (const auto& entry : history.entries()) {
        if (!subtexts.value(entry.id).isEmpty()) {
            std::cerr << "fetched a local link: " << entry.text << '\n';
            ++failures;
        }
    }
}

void fetchLive(QGuiApplication& app, const QByteArray& url) {
    kstack::core::HistoryService history;
    EntrySubtexts subtexts;
    LinkTitleController controller(history, subtexts);
    const auto entry = history.add(url.toStdString());
    if (!entry || entry->kind != kstack::core::ContentKind::Link) {
        std::cerr << "not captured as a link: " << url.toStdString() << '\n';
        ++failures;
        return;
    }
    QObject::connect(&subtexts, &EntrySubtexts::changed, &app, &QCoreApplication::quit);
    QTimer::singleShot(10000, &app, &QCoreApplication::quit);
    controller.entryCaptured(*entry);
    app.exec();
    std::cout << url.toStdString() << " -> \"" << subtexts.value(entry->id).toStdString() << "\"\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    QGuiApplication app(argc, argv);
    checkParsing();
    checkAddressGate();
    checkPinnedRequest();
    checkLocalLinksSkipped(app);
    const QByteArray live = qgetenv("KSTACK_LINK_TITLE_URL");
    if (!live.isEmpty()) {
        fetchLive(app, live);
    }
    return failures == 0 ? 0 : 1;
}
