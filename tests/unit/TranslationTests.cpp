/*
 * Author: Runor Ewhro
 * Description: checks target-language choice, model routing, language
 *              detection, and, when KSTACK_TEST_DEEN_CONFIG names an
 *              installed German-English model, a real translation.
 */
#include <kstack/translation/LanguageDetector.hpp>
#include <kstack/translation/LanguageTag.hpp>
#include <kstack/translation/Translator.hpp>

#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void checkLanguageChoice() {
    using kstack::translation::chooseTargetLanguage;
    using kstack::translation::primaryLanguage;
    require(primaryLanguage("en-CA") == "en", "region not removed");
    require(primaryLanguage("pt_BR") == "pt", "underscore tag not reduced");
    require(primaryLanguage("zh-Hant-TW") == "zh", "script tag not reduced");
    require(primaryLanguage("FR") == "fr", "tag not lowercased");
    require(primaryLanguage("").empty() && primaryLanguage("x").empty() &&
                primaryLanguage("e1-US").empty(),
            "malformed tag accepted");

    const auto onlyGerman = [](const std::string& language) { return language == "de"; };
    require(chooseTargetLanguage({"fr-CA", "de-DE", "en-US"}, onlyGerman) == "de",
            "later preferred language not chosen");
    require(!chooseTargetLanguage({"fr-CA", "en-US"}, onlyGerman).has_value(),
            "unavailable language chosen");
    int asked = 0;
    const auto countAsks = [&asked](const std::string&) {
        ++asked;
        return false;
    };
    (void)chooseTargetLanguage({"en-CA", "en-US", "en"}, countAsks);
    require(asked == 1, "one language tried more than once");
}

void checkRoutes() {
    using kstack::translation::ModelFiles;
    using kstack::translation::findRoute;
    const std::vector<ModelFiles> models{
        {{"de"}, "en", "deen-base"},
        {{"de"}, "en", "deen-tiny"},
        {{"en"}, "fr", "enfr"},
        {{"sr", "hr", "bs"}, "en", "hbsen"},
    };
    require(findRoute(models, "de", "en") == std::vector<std::size_t>{0},
            "direct route or model preference failed");
    require(findRoute(models, "de", "fr") == std::vector<std::size_t>{0, 2},
            "route through English failed");
    require(findRoute(models, "hr", "en") == std::vector<std::size_t>{3},
            "multi-source model not used");
    require(findRoute(models, "fr", "de").empty(), "route without models found");
    require(findRoute(models, "en", "en").empty(), "same-language route found");
    require(findRoute(models, "en", "de").empty(), "reverse direction found");
}

void checkDetection() {
#ifdef KSTACK_TRANSLATION_BUILT
    using kstack::translation::detectLanguage;
    struct Case {
        const char* text;
        const char* language;
    };
    const Case cases[] = {
        {"Können wir das Meeting auf Donnerstag verschieben?", "de"},
        {"¿Dónde está la estación de tren más cercana?", "es"},
        {"Je vous enverrai le rapport demain matin.", "fr"},
        {"Can we move the review to Thursday afternoon?", "en"},
    };
    for (const auto& item : cases) {
        const auto detected = detectLanguage(item.text);
        if (!detected || detected->language != item.language) {
            std::cerr << "detection failed for: " << item.text << " (got "
                      << (detected ? detected->language : "nothing") << ")\n";
            std::exit(1);
        }
    }
    require(!detectLanguage("").has_value(), "empty text detected");
#endif
}

void checkTranslation() {
    const char* config = std::getenv("KSTACK_TEST_DEEN_CONFIG");
    if (config == nullptr) {
        std::cout << "translation check skipped: KSTACK_TEST_DEEN_CONFIG is not set\n";
        return;
    }
    auto translator = kstack::translation::createTranslator({{{"de"}, "en", config}});
    require(translator->canTranslate("de", "en"), "installed model not routable");
    std::promise<std::optional<std::string>> result;
    require(translator->translate("Guten Morgen", "de", "en",
                                  [&result](std::optional<std::string> text) {
                                      result.set_value(std::move(text));
                                  }),
            "translation not queued");
    auto future = result.get_future();
    require(future.wait_for(std::chrono::seconds(30)) == std::future_status::ready,
            "translation timed out");
    const auto text = future.get();
    require(text.has_value() && text->find("Good morning") != std::string::npos,
            "translation result wrong");
}
} // namespace

int main() {
    checkLanguageChoice();
    checkRoutes();
    checkDetection();
    checkTranslation();
}
