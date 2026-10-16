/*
 * Author: Runor Ewhro
 * Description: holds how the history panel is laid out, such as grouping,
 *              row density, and the preview pane, remembers those choices
 *              between launches, and shows them as a native menu.
 */
#pragma once

#include <QObject>
#include <QString>

namespace kstack::desktop {

class ViewOptionsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString groupBy READ groupBy WRITE setGroupBy NOTIFY groupByChanged)
    Q_PROPERTY(QString density READ density WRITE setDensity NOTIFY densityChanged)
    Q_PROPERTY(bool previewVisible READ previewVisible WRITE setPreviewVisible NOTIFY previewVisibleChanged)
    Q_PROPERTY(bool menuOpen READ menuOpen NOTIFY menuOpenChanged)

public:
    explicit ViewOptionsController(QObject* parent = nullptr);
    [[nodiscard]] QString groupBy() const;
    void setGroupBy(const QString& groupBy);
    [[nodiscard]] QString density() const;
    void setDensity(const QString& density);
    [[nodiscard]] bool previewVisible() const noexcept;
    void setPreviewVisible(bool visible);
    [[nodiscard]] bool menuOpen() const noexcept;
    // shows the options as a native menu at a global screen position and
    // returns once it closes
    Q_INVOKABLE void showMenu(qreal x, qreal y);

signals:
    void groupByChanged();
    void densityChanged();
    void previewVisibleChanged();
    void menuOpenChanged();

private:
    QString groupBy_;
    QString density_;
    bool previewVisible_ = false;
    bool menuOpen_ = false;
};

} // namespace kstack::desktop
