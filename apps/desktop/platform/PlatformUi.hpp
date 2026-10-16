/*
 * Author: Runor Ewhro
 * Description: describes how the desktop shell looks and moves on the
 *              running platform: an optional system material behind the
 *              panel, the accent color, reduced motion, the monospace font,
 *              and shortcut labels. The base class is portable; a platform
 *              subclass overrides only what its system adds.
 */
#pragma once

#include <QColor>
#include <QObject>
#include <QString>

class QWindow;

namespace kstack::desktop {

class PlatformUi : public QObject {
    Q_OBJECT
    // true once a system material sits behind the panel; QML then leaves the
    // panel's surface, edge, and shadow to the system
    Q_PROPERTY(bool nativeMaterial READ nativeMaterial NOTIFY nativeMaterialChanged)
    Q_PROPERTY(QString monoFamily READ monoFamily CONSTANT)

public:
    explicit PlatformUi(QObject* parent = nullptr);
    ~PlatformUi() override = default;

    [[nodiscard]] bool nativeMaterial() const noexcept;
    [[nodiscard]] QString monoFamily() const;

    // places the system material behind a transparent, borderless window;
    // call once before it is first shown. The base has no material.
    virtual bool attachMaterial(QWindow* window, qreal cornerRadius);

    // moves the material with the panel's open and close animation; scale is
    // about the window centre and 1, 1 is the resting state
    Q_INVOKABLE virtual void setReveal(qreal opacity, qreal scale);
    // read each time the panel opens, so settings changes take effect
    Q_INVOKABLE [[nodiscard]] virtual bool reduceMotion() const;
    Q_INVOKABLE [[nodiscard]] virtual QColor accentColor() const;
    // a portable sequence such as "Ctrl+O" in the platform's own notation
    Q_INVOKABLE [[nodiscard]] QString keyText(const QString& portableSequence) const;

signals:
    void nativeMaterialChanged();

protected:
    void setNativeMaterial(bool attached);

private:
    bool nativeMaterial_ = false;
};

} // namespace kstack::desktop
