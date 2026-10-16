/*
  Author: Runor Ewhro
  Description: wires the desktop services, QML history window, and background
               menu bar control into one running clipboard application.
*/
#include "Application.hpp"

#ifdef Q_OS_MACOS
#include "macos/NativeMaterial.hpp"
#endif

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QQmlContext>
#include <QUrl>

namespace kstack::desktop {

namespace {
// draw the menu bar glyph as a stack of three slabs; the top slab carries the
// capture state: filled while capturing, dashed while paused, struck through
// when the clipboard cannot be read
QIcon makeTrayIcon(bool capturing, bool blocked) {
    constexpr qreal scale = 2.0;
    QPixmap image(QSize(18, 18) * scale);
    image.setDevicePixelRatio(scale);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(Qt::black, 1.4);
    painter.setPen(pen);
    const QRectF top(3, 2.5, 12, 3.6);
    const QRectF middle(3, 7.5, 12, 3.6);
    const QRectF bottom(3, 12.5, 12, 3.6);
    painter.drawRoundedRect(middle, 1.1, 1.1);
    painter.drawRoundedRect(bottom, 1.1, 1.1);
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
    painter.end();
    QIcon icon(image);
    icon.setIsMask(true);
    return icon;
}
} // namespace

Application::Application()
    : clipboardService_(history_), capturePipeline_(clipboardService_), searchService_(history_),
      historyModel_(history_), searchModel_(searchService_),
      clipboardController_(capturePipeline_, historyModel_, searchModel_),
      historyController_(history_, historyModel_, searchModel_), searchController_(searchModel_),
      settingsController_(clipboardController_) {}

// load the history window hidden, expose it through the tray menu, then start
// capture; a failed monitor start opens the window so the state is visible
bool Application::start() {
    QApplication::setQuitOnLastWindowClosed(false);
    engine_.rootContext()->setContextProperty("historyModel", &historyModel_);
    engine_.rootContext()->setContextProperty("searchModel", &searchModel_);
    engine_.rootContext()->setContextProperty("historyController", &historyController_);
    engine_.rootContext()->setContextProperty("searchController", &searchController_);
    engine_.rootContext()->setContextProperty("settingsController", &settingsController_);
    engine_.rootContext()->setContextProperty("summonMode",
                                              QSystemTrayIcon::isSystemTrayAvailable());
    engine_.load(QUrl(QStringLiteral("qrc:/KStack/qml/Main.qml")));
    if (engine_.rootObjects().isEmpty()) {
        return false;
    }
    historyWindow_ = qobject_cast<QQuickWindow*>(engine_.rootObjects().front());
    if (!historyWindow_) {
        return false;
    }
#ifdef Q_OS_MACOS
    // the panel's corner radius comes from QML so the glass and the rows
    // inside it stay concentric
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        const qreal radius = historyWindow_->property("cornerRadius").toReal();
        const NativeMaterial material = attachNativeMaterial(historyWindow_, radius);
        if (material != NativeMaterial::None) {
            materialBridge_ = std::make_unique<NativeMaterialBridge>(historyWindow_);
            historyWindow_->setProperty("materialBridge", QVariant::fromValue(materialBridge_.get()));
            historyWindow_->setProperty("nativeMaterial", true);
        }
    }
#endif

    auto* statusAction = trayMenu_.addAction(settingsController_.captureStatus());
    statusAction->setEnabled(false);
    trayMenu_.addSeparator();
    auto* openAction = trayMenu_.addAction(QStringLiteral("Open history"));
    QObject::connect(openAction, &QAction::triggered, &trayMenu_, [this] {
        historyWindow_->show();
        historyWindow_->raise();
        historyWindow_->requestActivate();
    });
    auto* captureAction = trayMenu_.addAction(QString());
    QObject::connect(captureAction, &QAction::triggered, &trayMenu_, [this] {
        settingsController_.setCaptureEnabled(!settingsController_.captureEnabled());
    });
    auto* clearAction = trayMenu_.addAction(QStringLiteral("Clear session"));
    QObject::connect(clearAction, &QAction::triggered, &trayMenu_,
                     [this] { historyController_.clear(); });
    trayMenu_.addSeparator();
    auto* quitAction = trayMenu_.addAction(QStringLiteral("Quit"));
    QObject::connect(quitAction, &QAction::triggered, &trayMenu_, [] { QApplication::quit(); });

    // keep the status line, the capture action's verb, and the glyph in step
    // with the controller so the menu never describes a stale state
    const auto showCaptureState = [this, statusAction, captureAction] {
        const bool capturing = settingsController_.captureEnabled();
        const bool blocked = settingsController_.captureBlocked();
        statusAction->setText(settingsController_.captureStatus());
        captureAction->setText(capturing ? QStringLiteral("Pause capture")
                               : blocked ? QStringLiteral("Try again")
                                         : QStringLiteral("Resume capture"));
        trayIcon_.setIcon(makeTrayIcon(capturing, blocked));
        trayIcon_.setToolTip(settingsController_.captureStatus());
    };
    QObject::connect(&settingsController_, &SettingsController::captureStatusChanged, &trayMenu_,
                     showCaptureState);
    showCaptureState();
    trayIcon_.setContextMenu(&trayMenu_);
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        trayIcon_.show();
    } else {
        QApplication::setQuitOnLastWindowClosed(true);
        historyWindow_->show();
    }
    settingsController_.setCaptureEnabled(true);
    if (!settingsController_.captureEnabled()) {
        historyWindow_->show();
        historyWindow_->raise();
        historyWindow_->requestActivate();
    }
    return true;
}

} // namespace kstack::desktop
