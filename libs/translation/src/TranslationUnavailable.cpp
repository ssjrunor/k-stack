/*
 * Author: Runor Ewhro
 * Description: stands in for detection and translation when the app is
 *              built without them, so callers need no build checks.
 */
#include <kstack/translation/LanguageDetector.hpp>
#include <kstack/translation/Translator.hpp>

namespace kstack::translation {
namespace {

class UnavailableTranslator final : public Translator {
public:
    bool canTranslate(std::string_view, std::string_view) const override { return false; }
    bool translate(std::string, std::string, std::string, Done) override { return false; }
};

} // namespace

std::optional<DetectedLanguage> detectLanguage(std::string_view) { return std::nullopt; }

std::unique_ptr<Translator> createTranslator(std::vector<ModelFiles>) {
    return std::make_unique<UnavailableTranslator>();
}

} // namespace kstack::translation
