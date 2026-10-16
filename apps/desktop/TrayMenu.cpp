/*
 * Author: Runor Ewhro
 * Description: builds the tray menu, keeps its recent entries in step
 *              with the session history, copies an entry when it is chosen,
 *              hides possible passwords, and draws the tray and entry
 *              glyphs as template images, with real swatches for colors.
 */
#include "TrayMenu.hpp"

#include "models/EntryPresentation.hpp"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QIcon>
#include <QKeySequence>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

#include <chrono>

namespace kstack::desktop {

namespace {
constexpr int kRecentCount = 9;
constexpr int kTitleWidth = 300;
constexpr int kTooltipLength = 400;
constexpr int kConfirmMs = 900;

// glyphs are drawn in black and marked as masks, so platforms that tint tray
// and menu icons can recolor them for light, dark, and highlighted states
template <typename Draw>
QIcon templateIcon(int size, Draw draw) {
    constexpr qreal scale = 2.0;
    QPixmap image(QSize(size, size) * scale);
    image.setDevicePixelRatio(scale);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::black, 1.4);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    draw(painter, pen);
    painter.end();
    QIcon icon(image);
    icon.setIsMask(true);
    return icon;
}

// a stack of three slabs; the top slab carries the capture state: filled
// while capturing, dashed while paused, struck through when blocked
QIcon stackIcon(bool capturing, bool blocked) {
    return templateIcon(18, [&](QPainter& painter, QPen& pen) {
        const QRectF top(3, 2.5, 12, 3.6);
        painter.drawRoundedRect(QRectF(3, 7.5, 12, 3.6), 1.1, 1.1);
        painter.drawRoundedRect(QRectF(3, 12.5, 12, 3.6), 1.1, 1.1);
        if (blocked) {
            painter.drawLine(QPointF(5.5, 1.5), QPointF(12.5, 6.0));
        } else if (capturing) {
            painter.setBrush(Qt::black);
            painter.drawRoundedRect(top, 1.1, 1.1);
        } else {
            pen.setDashPattern({1.4, 1.2});
            painter.setPen(pen);
            painter.drawRoundedRect(top, 1.1, 1.1);
        }
    });
}

// shown in the tray for a moment after a copy, since the menu itself
// closes as soon as an entry is chosen
QIcon copiedIcon() {
    return templateIcon(18, [](QPainter& painter, QPen& pen) {
        pen.setWidthF(1.8);
        painter.setPen(pen);
        QPainterPath tick;
        tick.moveTo(4, 9.5);
        tick.lineTo(7.5, 13);
        tick.lineTo(14, 5);
        painter.drawPath(tick);
    });
}

// a filled swatch of the color itself, so it is drawn as an ordinary image
QIcon swatchIcon(const QColor& color) {
    constexpr qreal scale = 2.0;
    QPixmap image(QSize(16, 16) * scale);
    image.setDevicePixelRatio(scale);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(0, 0, 0, 60), 1));
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(2.5, 2.5, 11, 11), 3, 3);
    painter.end();
    return QIcon(image);
}

QIcon kindIcon(core::ContentKind kind, const QColor& swatch) {
    using core::ContentKind;
    if (kind == ContentKind::Color && swatch.isValid()) {
        return swatchIcon(swatch);
    }
    return templateIcon(16, [kind](QPainter& painter, QPen&) {
        switch (kind) {
        case ContentKind::Link:
            painter.translate(8, 8);
            painter.rotate(-45);
            painter.drawRoundedRect(QRectF(-6.5, -2.4, 7.5, 4.8), 2.4, 2.4);
            painter.drawRoundedRect(QRectF(-1, -2.4, 7.5, 4.8), 2.4, 2.4);
            return;
        case ContentKind::Code:
            painter.drawPolyline(QPolygonF({QPointF(5.5, 4), QPointF(2, 8), QPointF(5.5, 12)}));
            painter.drawPolyline(QPolygonF({QPointF(10.5, 4), QPointF(14, 8), QPointF(10.5, 12)}));
            return;
        case ContentKind::Path: {
            QPainterPath folder;
            folder.moveTo(2, 4);
            folder.lineTo(6.5, 4);
            folder.lineTo(8, 5.5);
            folder.lineTo(14, 5.5);
            folder.lineTo(14, 12.5);
            folder.lineTo(2, 12.5);
            folder.closeSubpath();
            painter.drawPath(folder);
            return;
        }
        case ContentKind::Email:
        case ContentKind::Phone:
        case ContentKind::Address:
            painter.drawRoundedRect(QRectF(2, 4, 12, 8.5), 1.5, 1.5);
            painter.drawPolyline(QPolygonF({QPointF(2.5, 4.5), QPointF(8, 9), QPointF(13.5, 4.5)}));
            return;
        case ContentKind::Number:
        case ContentKind::VerificationCode:
            painter.drawLine(QPointF(6, 3), QPointF(5, 13));
            painter.drawLine(QPointF(11, 3), QPointF(10, 13));
            painter.drawLine(QPointF(3, 6.5), QPointF(13.5, 6.5));
            painter.drawLine(QPointF(2.5, 9.5), QPointF(13, 9.5));
            return;
        case ContentKind::Date:
        case ContentKind::Time:
            painter.drawRoundedRect(QRectF(2.5, 3.5, 11, 10), 2, 2);
            painter.drawLine(QPointF(2.5, 6.5), QPointF(13.5, 6.5));
            painter.drawLine(QPointF(5.5, 2), QPointF(5.5, 4.5));
            painter.drawLine(QPointF(10.5, 2), QPointF(10.5, 4.5));
            return;
        case ContentKind::Secret:
            painter.drawRoundedRect(QRectF(3, 7, 10, 7), 1.5, 1.5);
            painter.drawArc(QRectF(5, 2.5, 6, 8), 0, 180 * 16);
            return;
        case ContentKind::Color:
        case ContentKind::Text:
            break;
        }
        painter.drawLine(QPointF(3, 4.5), QPointF(13, 4.5));
        painter.drawLine(QPointF(3, 8), QPointF(13, 8));
        painter.drawLine(QPointF(3, 11.5), QPointF(9, 11.5));
    });
}

// links keep the end of their path visible; other titles keep their
// beginning; ampersands are doubled so they are not read as mnemonics
QString menuTitle(const EntryPresentation& p, const QFontMetrics& metrics) {
    const bool link = p.kind == QLatin1String("link");
    const QString title = link ? p.title + p.titleTail : p.title;
    QString elided = metrics.elidedText(title, link ? Qt::ElideMiddle : Qt::ElideRight, kTitleWidth);
    return elided.replace(QLatin1Char('&'), QStringLiteral("&&"));
}

QString capturedTime(const core::ClipboardEntry& entry) {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        entry.capturedAt.time_since_epoch())
                        .count();
    return QLocale().toString(QDateTime::fromMSecsSinceEpoch(ms).time(), QLocale::ShortFormat);
}
} // namespace

TrayMenu::TrayMenu(const core::HistoryService& history, HistoryListModel& historyModel,
                   HistoryController& historyController, SettingsController& settings,
                   QObject* parent)
    : QObject(parent), history_(history), historyController_(historyController),
      settings_(settings) {
    status_ = menu_.addAction(QString());
    status_->setEnabled(false);
    menu_.addSeparator();
    recentEnd_ = menu_.addSeparator();

    auto* open = menu_.addAction(QStringLiteral("Open history"));
    connect(open, &QAction::triggered, this, &TrayMenu::openHistoryRequested);
    capture_ = menu_.addAction(QString());
    connect(capture_, &QAction::triggered, this,
            [this] { settings_.setCaptureEnabled(!settings_.captureEnabled()); });
    auto* clear = menu_.addAction(settings_.persistentHistory()
                                      ? QStringLiteral("Clear history")
                                      : QStringLiteral("Clear session"));
    connect(clear, &QAction::triggered, this, [this] { historyController_.clear(); });
    menu_.addSeparator();
    auto* quit = menu_.addAction(QStringLiteral("Quit"));
    connect(quit, &QAction::triggered, this, [] { QApplication::quit(); });

    confirmTimer_.setSingleShot(true);
    confirmTimer_.setInterval(kConfirmMs);
    connect(&confirmTimer_, &QTimer::timeout, this, &TrayMenu::showCaptureState);
    connect(&settings_, &SettingsController::captureStatusChanged, this,
            &TrayMenu::showCaptureState);
    connect(&historyModel, &QAbstractItemModel::modelReset, this, &TrayMenu::rebuildRecent);

    icon_.setContextMenu(&menu_);
    showCaptureState();
    rebuildRecent();
}

bool TrayMenu::show() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return false;
    }
    icon_.show();
    return true;
}

// replace the recent section with the newest entries, each copying itself
// when chosen; an empty session shows where copies will appear
void TrayMenu::rebuildRecent() {
    for (QAction* action : std::as_const(recent_)) {
        menu_.removeAction(action);
        delete action;
    }
    recent_.clear();

    const auto& entries = history_.entries();
    if (entries.empty()) {
        auto* empty = new QAction(QStringLiteral("Copied text and links appear here"), &menu_);
        empty->setEnabled(false);
        menu_.insertAction(recentEnd_, empty);
        recent_.append(empty);
        return;
    }

    const QFontMetrics metrics(QApplication::font(&menu_));
    const auto count = std::min<std::size_t>(entries.size(), kRecentCount);
    for (std::size_t i = 0; i < count; ++i) {
        const auto& entry = entries[i];
        const EntryPresentation p = presentEntry(entry);

        auto* action = new QAction(kindIcon(entry.kind, p.swatch), menuTitle(p, metrics), &menu_);
        action->setShortcut(QKeySequence(Qt::CTRL | static_cast<Qt::Key>(Qt::Key_1 + i)));
        action->setShortcutContext(Qt::WidgetShortcut);
        // a possible password never appears in the menu, not even on hover
        const QString preview = p.sensitive ? QStringLiteral("Possible password, hidden")
                                            : QString::fromStdString(entry.text).left(kTooltipLength).trimmed();
        action->setToolTip(QStringLiteral("%1\nCopied at %2").arg(preview, capturedTime(entry)));
        const auto id = static_cast<qulonglong>(entry.id);
        connect(action, &QAction::triggered, this, [this, id] {
            historyController_.copy(id);
            confirmCopy();
        });
        menu_.insertAction(recentEnd_, action);
        recent_.append(action);
    }
}

// keep the status line, the capture action's verb, and the glyph in step
// with the controller so the menu never describes a stale state
void TrayMenu::showCaptureState() {
    if (confirmTimer_.isActive()) {
        return;
    }
    const bool capturing = settings_.captureEnabled();
    const bool blocked = settings_.captureBlocked();
    status_->setText(settings_.captureStatus());
    capture_->setText(capturing ? QStringLiteral("Pause capture")
                      : blocked ? QStringLiteral("Try again")
                                : QStringLiteral("Resume capture"));
    icon_.setIcon(stackIcon(capturing, blocked));
    icon_.setToolTip(settings_.captureStatus());
    if (blocked && settings_.captureStatus() == QLatin1String("History save failed") && icon_.isVisible()) {
        icon_.showMessage(QStringLiteral("History save failed"),
                          QStringLiteral("Clipboard capture has stopped. Open history to check storage."),
                          QSystemTrayIcon::Critical);
    }
}

void TrayMenu::confirmCopy() {
    icon_.setIcon(copiedIcon());
    icon_.setToolTip(QStringLiteral("Copied"));
    confirmTimer_.start();
}

} // namespace kstack::desktop
