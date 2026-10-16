/*
 * Author: Runor Ewhro
 * Description: hands clipboard entries to the rest of the system: sharing,
 *              calendars, reminders, and maps. Each capability is a flag QML
 *              reads to decide what to offer. The base class is portable,
 *              adding events through a standard calendar file and showing
 *              places on a web map; a platform subclass adds the rest.
 */
#pragma once

#include <QObject>
#include <QString>

class QWindow;

namespace kstack::desktop {

class SystemServices : public QObject {
    Q_OBJECT
    // true while a share sheet, permission prompt, or calendar request is in
    // progress, so the panel does not close underneath it
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool canShare READ canShare CONSTANT)
    Q_PROPERTY(bool canAddEvent READ canAddEvent CONSTANT)
    Q_PROPERTY(bool canShowCalendarDay READ canShowCalendarDay CONSTANT)
    Q_PROPERTY(bool canAddReminder READ canAddReminder CONSTANT)

public:
    explicit SystemServices(QObject* parent = nullptr);
    ~SystemServices() override = default;

    // the window that share sheets are anchored to
    void setWindow(QWindow* window);
    [[nodiscard]] bool busy() const noexcept;

    [[nodiscard]] virtual bool canShare() const;
    [[nodiscard]] virtual bool canAddEvent() const;
    [[nodiscard]] virtual bool canShowCalendarDay() const;
    [[nodiscard]] virtual bool canAddReminder() const;

    // x and y are in the window's coordinates; a url shares as a link or a
    // file, otherwise the text is shared
    Q_INVOKABLE virtual void share(const QString& text, const QString& url, qreal x, qreal y);
    Q_INVOKABLE virtual void addCalendarEvent(const QString& title, const QString& isoDateTime, bool allDay);
    Q_INVOKABLE virtual void showCalendarDay(const QString& isoDateTime);
    Q_INVOKABLE virtual void addReminder(const QString& title, const QString& isoDateTime, bool allDay);
    Q_INVOKABLE virtual void showOnMap(const QString& place);
    // clears a stuck busy state, for example after a share target that never
    // reports back; called whenever the panel opens
    Q_INVOKABLE void resetBusy();

signals:
    void busyChanged();
    void finished(const QString& message, bool succeeded);

protected:
    void setBusy(bool busy);
    [[nodiscard]] QWindow* window() const noexcept;
    // writes a one-event iCalendar file and returns its path, or an empty
    // string when it cannot be written
    [[nodiscard]] static QString writeEventFile(const QString& title, const QString& isoDateTime,
                                                bool allDay);
    // opens the event file in the calendar app; the base uses the default
    // handler for calendar files
    virtual void openEventFile(const QString& path);

private:
    QWindow* window_ = nullptr;
    bool busy_ = false;
};

} // namespace kstack::desktop
