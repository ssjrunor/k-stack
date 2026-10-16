/*
 * Author: Runor Ewhro
 * Description: places the system glass material behind a transparent Qt
 *              Quick window on macOS, so the panel looks and blurs like
 *              Spotlight while QML draws only the content on top, and lets
 *              QML scale and fade that material in step with its own motion.
 */
#pragma once

#include <QObject>
#include <QtGlobal>

class QWindow;

namespace kstack::desktop {

enum class NativeMaterial { None, Glass, Blur };

// call once before the window is first shown; the material then follows the
// window as it shows, hides, moves, and resizes
NativeMaterial attachNativeMaterial(QWindow* window, qreal cornerRadius);

// exposed to QML so open and close animations move the glass with the content
class NativeMaterialBridge final : public QObject {
    Q_OBJECT

public:
    explicit NativeMaterialBridge(QWindow* window, QObject* parent = nullptr);
    // scale is about the window centre; 1 and 1 is the resting state
    Q_INVOKABLE void setReveal(qreal opacity, qreal scale);
    // read on each open so a change in Accessibility settings takes effect
    Q_INVOKABLE [[nodiscard]] bool reduceMotion() const;

private:
    QWindow* window_;
};

} // namespace kstack::desktop
