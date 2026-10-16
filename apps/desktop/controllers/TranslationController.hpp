/*
 * Author: Runor Ewhro
 * Description: translates newly captured plain text that is written in a
 *              language other than the user's into their first preferred
 *              language that has an installed model, and shows the result as
 *              the entry's subtext; the preview can also translate an entry's
 *              full text on request. Translation runs on the device.
 */
#pragma once

#include "../models/EntrySubtexts.hpp"

#include <kstack/core/HistoryService.hpp>
#include <kstack/translation/Translator.hpp>

#include <QObject>
#include <QString>

#include <memory>
#include <optional>
#include <string>

namespace kstack::desktop {

class TranslationController final : public QObject {
    Q_OBJECT

public:
    TranslationController(const core::HistoryService& history, EntrySubtexts& subtexts,
                          QObject* parent = nullptr);
    ~TranslationController() override;
    // called once for each entry the history accepts
    void entryCaptured(const core::ClipboardEntry& entry);
    // the name of the language the text seems to be in, such as "German",
    // when it can be translated into the user's language; empty otherwise
    Q_INVOKABLE QString translatableFrom(const QString& text) const;
    // translates an entry's full text; the answer arrives as translated or
    // translationFailed with the same id
    Q_INVOKABLE void translate(qulonglong id);
    // where models are installed: KSTACK_TRANSLATION_MODELS when set,
    // otherwise "translation-models" in the app's data folder
    [[nodiscard]] static QString modelsDirectory();

signals:
    void translated(qulonglong id, const QString& text);
    void translationFailed(qulonglong id);

private:
    // the detected language when there is a route from it to the target
    [[nodiscard]] std::optional<std::string> routableLanguage(const std::string& text,
                                                              bool requireReliable) const;
    [[nodiscard]] const core::ClipboardEntry* findEntry(std::uint64_t id) const;
    void apply(std::uint64_t id, const std::optional<std::string>& translation);
    void deliver(std::uint64_t id, const std::optional<std::string>& translation);

    const core::HistoryService& history_;
    EntrySubtexts& subtexts_;
    std::optional<std::string> targetLanguage_;
    // declared last so its thread stops before the rest of this object goes
    std::unique_ptr<translation::Translator> translator_;
};

} // namespace kstack::desktop
