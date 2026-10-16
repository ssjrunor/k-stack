/*
 * Author: Runor Ewhro
 * Description: stores and announces per-entry subtexts.
 */
#include "EntrySubtexts.hpp"

#include <QSet>

namespace kstack::desktop {

QString EntrySubtexts::value(std::uint64_t id) const { return texts_.value(id); }

void EntrySubtexts::set(std::uint64_t id, const QString& text) {
    if (texts_.value(id) == text) {
        return;
    }
    if (text.isEmpty()) {
        texts_.remove(id);
    } else {
        texts_.insert(id, text);
    }
    emit changed(id);
}

void EntrySubtexts::remove(std::uint64_t id) {
    if (texts_.remove(id) > 0) {
        emit changed(id);
    }
}

void EntrySubtexts::clear() { texts_.clear(); }

void EntrySubtexts::retain(const std::vector<core::ClipboardEntry>& entries) {
    QSet<quint64> kept;
    for (const auto& entry : entries) {
        kept.insert(entry.id);
    }
    texts_.removeIf([&kept](const auto& item) { return !kept.contains(item.key()); });
}

} // namespace kstack::desktop
