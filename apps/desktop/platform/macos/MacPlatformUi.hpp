/*
 * Author: Runor Ewhro
 * Description: macOS look and feel: Liquid Glass or a behind-window blur
 *              under the panel, the user's accent color, and the Reduce
 *              Motion accessibility setting.
 */
#pragma once

#include "../PlatformUi.hpp"

#include <memory>

namespace kstack::desktop {

class MacPlatformUi final : public PlatformUi {
    Q_OBJECT

public:
    explicit MacPlatformUi(QObject* parent = nullptr);
    ~MacPlatformUi() override;

    bool attachMaterial(QWindow* window, qreal cornerRadius) override;
    void setReveal(qreal opacity, qreal scale) override;
    [[nodiscard]] bool reduceMotion() const override;
    [[nodiscard]] QColor accentColor() const override;

    struct Backdrop;

private:
    QWindow* window_ = nullptr;
    std::shared_ptr<Backdrop> backdrop_;
};

} // namespace kstack::desktop
