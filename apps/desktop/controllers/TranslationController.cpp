/*
 * Author: Runor Ewhro
 * Description: finds the installed models, picks the target language from
 *              the system's preferred languages, and turns translations that
 *              finish on the translator's thread into subtexts on this one.
 */
#include "TranslationController.hpp"

#include <kstack/translation/LanguageDetector.hpp>
#include <kstack/translation/LanguageTag.hpp>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QStandardPaths>

#include <algorithm>
#include <vector>

namespace kstack::desktop {
namespace {

// a row shows one line, so only the start of a long item is translated,
// which also keeps each translation quick
constexpr qsizetype translatedLength = 400;
// the preview shows more, but a very long paste still stops somewhere
constexpr qsizetype previewTranslatedLength = 4000;

struct InstalledModel {
    translation::ModelFiles files;
    bool base = false; // base models translate better than tiny ones
};

// each model is a folder holding the model_info.json and config*.yml that
// the published Bergamot archives contain
std::vector<translation::ModelFiles> installedModels(const QString& directory) {
    std::vector<InstalledModel> found;
    const QDir root(directory);
    for (const QString& name : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        const QDir folder(root.filePath(name));
        QFile infoFile(folder.filePath(QStringLiteral("model_info.json")));
        const QStringList configs =
            folder.entryList({QStringLiteral("config*.yml")}, QDir::Files, QDir::Name);
        if (configs.isEmpty() || !infoFile.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QJsonObject info = QJsonDocument::fromJson(infoFile.readAll()).object();
        InstalledModel model;
        for (const QString& source : info.value(QStringLiteral("srcTags")).toObject().keys()) {
            model.files.sources.push_back(translation::primaryLanguage(source.toStdString()));
        }
        model.files.target =
            translation::primaryLanguage(info.value(QStringLiteral("trgTag")).toString().toStdString());
        model.files.config = folder.filePath(configs.front()).toStdString();
        model.base = info.value(QStringLiteral("type")).toString() == QLatin1String("base");
        if (!model.files.sources.empty() && !model.files.target.empty()) {
            found.push_back(std::move(model));
        }
    }
    std::stable_sort(found.begin(), found.end(),
                     [](const auto& left, const auto& right) { return left.base && !right.base; });
    std::vector<translation::ModelFiles> models;
    for (auto& model : found) {
        models.push_back(std::move(model.files));
    }
    return models;
}

} // namespace

TranslationController::TranslationController(const core::HistoryService& history,
                                             EntrySubtexts& subtexts, QObject* parent)
    : QObject(parent), history_(history), subtexts_(subtexts) {
    auto models = installedModels(modelsDirectory());
    std::vector<std::string> preferred;
    for (const QString& tag : QLocale::system().uiLanguages()) {
        preferred.push_back(tag.toStdString());
    }
    targetLanguage_ = translation::chooseTargetLanguage(preferred, [&models](const std::string& language) {
        return std::any_of(models.begin(), models.end(),
                           [&language](const auto& model) { return model.target == language; });
    });
    translator_ = translation::createTranslator(std::move(models));
}

TranslationController::~TranslationController() = default;

QString TranslationController::modelsDirectory() {
    const QString configured = qEnvironmentVariable("KSTACK_TRANSLATION_MODELS");
    if (!configured.isEmpty()) {
        return configured;
    }
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("translation-models"));
}

void TranslationController::entryCaptured(const core::ClipboardEntry& entry) {
    // only prose: links, codes, and possible passwords are never translated
    if (entry.kind != core::ContentKind::Text || !targetLanguage_) {
        return;
    }
    const std::string text = QString::fromStdString(entry.text).left(translatedLength).toStdString();
    // short or mixed text gives an unreliable guess, and a wrong guess would
    // show a nonsense translation under every such row, so those wait until
    // someone asks in the preview
    const auto source = routableLanguage(text, true);
    if (!source) {
        return;
    }
    const std::uint64_t id = entry.id;
    translator_->translate(text, *source, *targetLanguage_,
                           [this, id](std::optional<std::string> translation) {
                               QMetaObject::invokeMethod(
                                   this, [this, id, translation] { apply(id, translation); },
                                   Qt::QueuedConnection);
                           });
}

std::optional<std::string> TranslationController::routableLanguage(const std::string& text,
                                                                  bool requireReliable) const {
    if (!targetLanguage_) {
        return std::nullopt;
    }
    const auto detected = translation::detectLanguage(text);
    if (!detected || (requireReliable && !detected->reliable) ||
        !translator_->canTranslate(detected->language, *targetLanguage_)) {
        return std::nullopt;
    }
    return detected->language;
}

const core::ClipboardEntry* TranslationController::findEntry(std::uint64_t id) const {
    const auto& entries = history_.entries();
    const auto entry = std::find_if(entries.begin(), entries.end(),
                                    [id](const auto& item) { return item.id == id; });
    return entry == entries.end() ? nullptr : &*entry;
}

QString TranslationController::translatableFrom(const QString& text) const {
    const auto source = routableLanguage(text.left(previewTranslatedLength).toStdString(), false);
    if (!source) {
        return {};
    }
    return QLocale::languageToString(QLocale(QString::fromStdString(*source)).language());
}

void TranslationController::translate(qulonglong id) {
    const core::ClipboardEntry* entry = findEntry(id);
    if (entry == nullptr || entry->kind != core::ContentKind::Text) {
        emit translationFailed(id);
        return;
    }
    const std::string text = QString::fromStdString(entry->text).left(previewTranslatedLength).toStdString();
    const auto source = routableLanguage(text, false);
    const bool queued =
        source && translator_->translate(text, *source, *targetLanguage_,
                                         [this, id](std::optional<std::string> translation) {
                                             QMetaObject::invokeMethod(
                                                 this, [this, id, translation] { deliver(id, translation); },
                                                 Qt::QueuedConnection);
                                         });
    if (!queued) {
        emit translationFailed(id);
    }
}

void TranslationController::deliver(std::uint64_t id, const std::optional<std::string>& translation) {
    // a removed entry's translation is dropped like its subtext would be
    if (findEntry(id) == nullptr || !translation || translation->empty()) {
        emit translationFailed(id);
        return;
    }
    emit translated(id, QString::fromStdString(*translation).trimmed());
}

void TranslationController::apply(std::uint64_t id, const std::optional<std::string>& translation) {
    // the entry may have been removed or cleared while it was translating
    const core::ClipboardEntry* entry = findEntry(id);
    if (entry == nullptr || !translation) {
        return;
    }
    const QString text = QString::fromStdString(*translation).simplified();
    if (!text.isEmpty() && text != QString::fromStdString(entry->text).simplified()) {
        subtexts_.set(id, text);
    }
}

} // namespace kstack::desktop
