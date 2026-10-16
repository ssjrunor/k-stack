/*
 * Author: Runor Ewhro
 * Description: macOS system services: the share menu, showing a day in
 *              Calendar, reminders through EventKit, events opened directly
 *              in Calendar, and places shown in Maps.
 */
#pragma once

#include "../SystemServices.hpp"

#include <memory>

namespace kstack::desktop {

class MacSystemServices final : public SystemServices {
    Q_OBJECT

public:
    explicit MacSystemServices(QObject* parent = nullptr);
    ~MacSystemServices() override;

    [[nodiscard]] bool canShare() const override;
    [[nodiscard]] bool canShowCalendarDay() const override;
    [[nodiscard]] bool canAddReminder() const override;

    void share(const QString& text, const QString& url, qreal x, qreal y) override;
    void showCalendarDay(const QString& isoDateTime) override;
    void addReminder(const QString& title, const QString& isoDateTime, bool allDay) override;
    void showOnMap(const QString& place) override;

protected:
    void openEventFile(const QString& path) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace kstack::desktop
