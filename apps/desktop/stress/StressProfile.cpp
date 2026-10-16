/*
 * Author: Runor Ewhro
 * Description: reads isolated stress history and checks generated entries in
 *              normal encrypted history without exposing other clipboard data.
 */
#include "StressProfile.hpp"

#include "../platform/macos/HistoryKeyStore.hpp"

#include <kstack/algorithms/ContentHasher.hpp>
#include <kstack/storage/Database.hpp>
#include <kstack/storage/EntryRepository.hpp>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace kstack::desktop {

StressProfile validatedStressProfile(const QString& directory) {
    const QFileInfo profile(directory);
    const QString tempRoot = QFileInfo(QDir::tempPath()).canonicalFilePath();
    const QString path = profile.canonicalFilePath();
    if (!profile.isAbsolute() || !profile.isDir() || profile.isSymLink() ||
        tempRoot.isEmpty() || !path.startsWith(tempRoot + QLatin1Char('/')) ||
        !QFileInfo(path).fileName().startsWith(QLatin1String("kstack-stress-"))) {
        throw std::runtime_error("Stress profile must be a real kstack-stress-* directory under the system temp folder");
    }
    const QByteArray digest = QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256).toHex();
    return {path, "org.runorewhro.clipboard-history.stress." + digest.toStdString()};
}

void reportStressProfile(const StressProfile& profile) {
    const QString path = QDir(profile.directory).filePath(QStringLiteral("history.db"));
    if (!QFileInfo::exists(path)) {
        throw std::runtime_error("Stress history database does not exist");
    }
    auto key = historyDatabaseKey(true, profile.keyService);
    std::unique_ptr<storage::Database> database;
    try {
        database = std::make_unique<storage::Database>(path.toStdString(), key);
    } catch (...) {
        std::fill(key.begin(), key.end(), 0);
        throw;
    }
    std::fill(key.begin(), key.end(), 0);
    storage::EntryRepository entries(*database);
    QJsonArray content;
    for (const auto& entry : entries.loadAll()) {
        content.append(QString::fromStdString(entry.text));
    }
    const QByteArray json = QJsonDocument(QJsonObject{{QStringLiteral("entries"), content}})
                                .toJson(QJsonDocument::Compact);
    std::cout.write(json.constData(), json.size());
    std::cout << '\n';
}

void removeStressProfileKey(const StressProfile& profile) {
    deleteStressHistoryKey(profile.keyService);
}

// Query the normal database through a second read-only SQLCipher connection.
// The app can continue writing to its WAL, and only generated-item positions
// leave this process; the user's other clipboard contents stay private.
void verifyLiveStressEntries(const QString& manifestPath) {
    QFile manifest(manifestPath);
    if (!manifest.open(QIODevice::ReadOnly) || manifest.size() > 50'000'000) {
        throw std::runtime_error("Cannot read the stress manifest");
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(manifest.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray() ||
        document.array().isEmpty() || document.array().size() > 100'000) {
        throw std::runtime_error("Invalid stress manifest");
    }
    const QJsonArray expected = document.array();
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString path = QDir(directory).filePath(QStringLiteral("history.db"));
    if (!QFileInfo::exists(path)) {
        throw std::runtime_error("Normal encrypted history does not exist; check the running app build");
    }
    auto key = historyDatabaseKey(true);
    sqlite3* raw = nullptr;
    const int flags = SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_NOFOLLOW;
    const int opened = sqlite3_open_v2(path.toUtf8().constData(), &raw, flags, nullptr);
    std::unique_ptr<sqlite3, decltype(&sqlite3_close)> database(raw, sqlite3_close);
    if (opened != SQLITE_OK) {
        std::fill(key.begin(), key.end(), 0);
        throw std::runtime_error("Cannot open normal encrypted history");
    }
    const int keyed = sqlite3_key(database.get(), key.data(), static_cast<int>(key.size()));
    std::fill(key.begin(), key.end(), 0);
    if (keyed != SQLITE_OK) {
        throw std::runtime_error("Cannot unlock normal encrypted history");
    }
    sqlite3_busy_timeout(database.get(), 5000);
    sqlite3_stmt* rawQuery = nullptr;
    if (sqlite3_prepare_v2(database.get(),
            "SELECT 1 FROM entries WHERE content_hash = ? AND content = ? LIMIT 1",
            -1, &rawQuery, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Cannot query normal encrypted history");
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> query(rawQuery, sqlite3_finalize);
    QJsonArray missing;
    for (qsizetype index = 0; index < expected.size(); ++index) {
        if (!expected.at(index).isString()) {
            throw std::runtime_error("Invalid stress manifest entry");
        }
        const QByteArray content = expected.at(index).toString().toUtf8();
        const std::uint64_t hash = algorithms::contentHash(
            std::string_view(content.constData(), static_cast<std::size_t>(content.size())));
        std::array<std::uint8_t, 8> hashBytes{};
        for (std::size_t byte = 0; byte < hashBytes.size(); ++byte) {
            hashBytes[byte] = static_cast<std::uint8_t>(hash >> (byte * 8));
        }
        sqlite3_reset(query.get());
        sqlite3_clear_bindings(query.get());
        if (sqlite3_bind_blob(query.get(), 1, hashBytes.data(), static_cast<int>(hashBytes.size()), SQLITE_TRANSIENT) != SQLITE_OK ||
            sqlite3_bind_text(query.get(), 2, content.constData(), content.size(), SQLITE_TRANSIENT) != SQLITE_OK) {
            throw std::runtime_error("Cannot bind stress entry");
        }
        const int result = sqlite3_step(query.get());
        if (result == SQLITE_DONE) {
            missing.append(static_cast<int>(index));
        } else if (result != SQLITE_ROW) {
            throw std::runtime_error("Cannot read normal encrypted history");
        }
    }
    const QByteArray json = QJsonDocument(QJsonObject{
        {QStringLiteral("saved"), static_cast<int>(expected.size() - missing.size())},
        {QStringLiteral("missing"), missing},
    }).toJson(QJsonDocument::Compact);
    std::cout.write(json.constData(), json.size());
    std::cout << '\n';
}

} // namespace kstack::desktop
