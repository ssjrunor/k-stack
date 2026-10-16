/*
 * Author: Runor Ewhro
 * Description: fetches the page behind each newly captured link and shows
 *              its <title> as the entry's subtext. Only the start of the page
 *              is read, and nothing is fetched for hosts on the local network.
 */
#pragma once

#include "../models/EntrySubtexts.hpp"

#include <kstack/core/HistoryService.hpp>

#include <QNetworkAccessManager>
#include <QHostAddress>
#include <QNetworkRequest>
#include <QObject>
#include <QUrl>

#include <cstdint>

namespace kstack::desktop {

class LinkTitleController final : public QObject {
    Q_OBJECT

public:
    LinkTitleController(const core::HistoryService& history, EntrySubtexts& subtexts,
                        QObject* parent = nullptr);
    // called once for each entry the history accepts
    void entryCaptured(const core::ClipboardEntry& entry);
    // the page title in an HTML prefix, decoded and on one line; empty when
    // the prefix has none. contentType is the response's Content-Type header.
    [[nodiscard]] static QString titleFromHtml(const QByteArray& html, const QByteArray& contentType);
    // Addresses used for a title request must be safe to connect to directly.
    [[nodiscard]] static bool isPublicAddress(const QHostAddress& address);
    [[nodiscard]] static QNetworkRequest requestForAddress(const QUrl& url,
                                                            const QHostAddress& address);

private:
    [[nodiscard]] bool entryExists(std::uint64_t id) const;
    void fetch(const QUrl& url, std::uint64_t id, int redirectsRemaining);
    void fetchAt(const QUrl& url, const QHostAddress& address, std::uint64_t id,
                 int redirectsRemaining);

    const core::HistoryService& history_;
    EntrySubtexts& subtexts_;
    QNetworkAccessManager network_;
};

} // namespace kstack::desktop
