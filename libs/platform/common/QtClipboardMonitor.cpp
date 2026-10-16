/*
  Author: Runor Ewhro
  Description: observes text and URL clipboard changes through Qt where the
               native macOS pasteboard adapter is not selected.
*/
#include "QtClipboardMonitor.hpp"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QStringList>
#include <QUrl>

#include <utility>

namespace kstack::platform {

QtClipboardMonitor::~QtClipboardMonitor() { stop(); }

// prefer a URL representation when one is present, then fall back to plain
// text so copied browser links are recorded as their destination
bool QtClipboardMonitor::start(TextCallback onText, AccessCallback onAccessChanged) {
    stop();
    // Qt does not expose a portable clipboard permission state. A failure to
    // obtain the clipboard is reported by the return value at startup.
    static_cast<void>(onAccessChanged);
    auto* clipboard = QGuiApplication::clipboard();
    if (!clipboard) {
        return false;
    }
    connection_ = QObject::connect(clipboard, &QClipboard::dataChanged, clipboard,
                                   [clipboard, callback = std::move(onText)] {
                                       const auto* mime = clipboard->mimeData();
                                       if (!mime) {
                                           return;
                                       }
                                       if (mime->hasUrls()) {
                                           QStringList urls;
                                           for (const auto& url : mime->urls()) {
                                               urls.push_back(url.toString(QUrl::FullyEncoded));
                                           }
                                           if (!urls.isEmpty()) {
                                               callback(urls.join(QLatin1Char('\n')).toUtf8().toStdString());
                                               return;
                                           }
                                       }
                                       if (mime->hasText()) {
                                           callback(mime->text().toUtf8().toStdString());
                                       }
                                   });
    return true;
}

void QtClipboardMonitor::stop() { QObject::disconnect(connection_); }

} // namespace kstack::platform
