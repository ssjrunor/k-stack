/*
 * Author: Runor Ewhro
 * Description: portable look-and-feel defaults built on Qt alone: no system
 *              material, the palette's highlight as the accent, the system
 *              fixed-width font, and Qt's native shortcut notation.
 */
#include "PlatformUi.hpp"

#include <QFontDatabase>
#include <QGuiApplication>
#include <QKeySequence>
#include <QPalette>

namespace kstack::desktop {

PlatformUi::PlatformUi(QObject* parent) : QObject(parent) {}

bool PlatformUi::nativeMaterial() const noexcept { return nativeMaterial_; }

QString PlatformUi::monoFamily() const {
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

bool PlatformUi::attachMaterial(QWindow* window, qreal cornerRadius) {
    Q_UNUSED(window);
    Q_UNUSED(cornerRadius);
    return false;
}

void PlatformUi::setReveal(qreal opacity, qreal scale) {
    Q_UNUSED(opacity);
    Q_UNUSED(scale);
}

bool PlatformUi::reduceMotion() const { return false; }

QColor PlatformUi::accentColor() const {
    return QGuiApplication::palette().color(QPalette::Active, QPalette::Highlight);
}

QString PlatformUi::keyText(const QString& portableSequence) const {
    return QKeySequence(portableSequence, QKeySequence::PortableText).toString(QKeySequence::NativeText);
}

void PlatformUi::setNativeMaterial(bool attached) {
    if (nativeMaterial_ != attached) {
        nativeMaterial_ = attached;
        emit nativeMaterialChanged();
    }
}

} // namespace kstack::desktop
