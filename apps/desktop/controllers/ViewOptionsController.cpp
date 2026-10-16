/*
 * Author: Runor Ewhro
 * Description: stores the panel's view choices in the user's settings and
 *              builds the native view menu with one checked item per choice.
 */
#include "ViewOptionsController.hpp"

#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QPoint>
#include <QSettings>

namespace kstack::desktop {

namespace {
const QString kGroupKey = QStringLiteral("panel/groupBy");
const QString kDensityKey = QStringLiteral("panel/density");
const QString kPreviewKey = QStringLiteral("panel/preview");
} // namespace

ViewOptionsController::ViewOptionsController(QObject* parent) : QObject(parent) {
    const QSettings settings;
    groupBy_ = settings.value(kGroupKey, QStringLiteral("time")).toString();
    density_ = settings.value(kDensityKey, QStringLiteral("compact")).toString();
    previewVisible_ = settings.value(kPreviewKey, false).toBool();
}

QString ViewOptionsController::groupBy() const { return groupBy_; }

void ViewOptionsController::setGroupBy(const QString& groupBy) {
    if (groupBy_ == groupBy) {
        return;
    }
    groupBy_ = groupBy;
    QSettings().setValue(kGroupKey, groupBy_);
    emit groupByChanged();
}

QString ViewOptionsController::density() const { return density_; }

void ViewOptionsController::setDensity(const QString& density) {
    if (density_ == density) {
        return;
    }
    density_ = density;
    QSettings().setValue(kDensityKey, density_);
    emit densityChanged();
}

bool ViewOptionsController::previewVisible() const noexcept { return previewVisible_; }

void ViewOptionsController::setPreviewVisible(bool visible) {
    if (previewVisible_ == visible) {
        return;
    }
    previewVisible_ = visible;
    QSettings().setValue(kPreviewKey, previewVisible_);
    emit previewVisibleChanged();
}

bool ViewOptionsController::menuOpen() const noexcept { return menuOpen_; }

void ViewOptionsController::showMenu(qreal x, qreal y) {
    QMenu menu;
    const auto addChoices = [&menu](const QList<QPair<QString, QString>>& choices,
                                    const QString& current, auto apply) {
        auto* group = new QActionGroup(&menu);
        for (const auto& [value, label] : choices) {
            QAction* action = menu.addAction(label);
            action->setCheckable(true);
            action->setChecked(value == current);
            group->addAction(action);
            QObject::connect(action, &QAction::triggered, &menu, [apply, value] { apply(value); });
        }
    };
    addChoices({{QStringLiteral("time"), QStringLiteral("Group by time")},
                {QStringLiteral("kind"), QStringLiteral("Group by kind")},
                {QStringLiteral("none"), QStringLiteral("No grouping")}},
               groupBy_, [this](const QString& value) { setGroupBy(value); });
    menu.addSeparator();
    addChoices({{QStringLiteral("compact"), QStringLiteral("Compact rows")},
                {QStringLiteral("detailed"), QStringLiteral("Detailed rows")}},
               density_, [this](const QString& value) { setDensity(value); });
    menu.addSeparator();
    QAction* preview = menu.addAction(QStringLiteral("Show preview"));
    preview->setCheckable(true);
    preview->setChecked(previewVisible_);
    preview->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
    preview->setShortcutContext(Qt::WidgetShortcut);
    connect(preview, &QAction::toggled, this, &ViewOptionsController::setPreviewVisible);

    // the panel ignores losing focus while the menu tracks the pointer, so
    // opening the menu does not dismiss the panel underneath it
    menuOpen_ = true;
    emit menuOpenChanged();
    menu.exec(QPoint(static_cast<int>(x), static_cast<int>(y)));
    menuOpen_ = false;
    emit menuOpenChanged();
}

} // namespace kstack::desktop
