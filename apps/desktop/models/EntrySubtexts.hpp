/*
 * Author: Runor Ewhro
 * Description: holds the optional second line a row shows under its title,
 *              keyed by entry. Any kind can fill one in when it has something
 *              to add; text in another language gets its translation.
 */
#pragma once

#include <kstack/core/ClipboardEntry.hpp>

#include <QHash>
#include <QObject>
#include <QString>

#include <cstdint>
#include <vector>

namespace kstack::desktop {

class EntrySubtexts final : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    // empty when the entry has no subtext
    [[nodiscard]] QString value(std::uint64_t id) const;
    void set(std::uint64_t id, const QString& text);
    void remove(std::uint64_t id);
    void clear();
    // forgets entries the history no longer holds, such as those dropped by
    // the retention limit, so no derived text outlives its entry
    void retain(const std::vector<core::ClipboardEntry>& entries);

signals:
    void changed(quint64 id);

private:
    QHash<quint64, QString> texts_;
};

} // namespace kstack::desktop
