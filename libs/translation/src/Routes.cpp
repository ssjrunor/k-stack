/*
 * Author: Runor Ewhro
 * Description: finds the installed models that connect two languages,
 *              directly or by way of English, which most models pair with.
 */
#include <kstack/translation/Translator.hpp>

#include <algorithm>
#include <optional>

namespace kstack::translation {
namespace {

constexpr std::string_view pivotLanguage = "en";

std::optional<std::size_t> findModel(const std::vector<ModelFiles>& models,
                                     std::string_view source, std::string_view target) {
    for (std::size_t i = 0; i < models.size(); ++i) {
        const auto& sources = models[i].sources;
        if (models[i].target == target &&
            std::find(sources.begin(), sources.end(), source) != sources.end()) {
            return i;
        }
    }
    return std::nullopt;
}

} // namespace

std::vector<std::size_t> findRoute(const std::vector<ModelFiles>& models,
                                   std::string_view source, std::string_view target) {
    if (source.empty() || target.empty() || source == target) {
        return {};
    }
    if (const auto direct = findModel(models, source, target)) {
        return {*direct};
    }
    if (source == pivotLanguage || target == pivotLanguage) {
        return {};
    }
    const auto first = findModel(models, source, pivotLanguage);
    const auto second = findModel(models, pivotLanguage, target);
    if (first && second) {
        return {*first, *second};
    }
    return {};
}

} // namespace kstack::translation
