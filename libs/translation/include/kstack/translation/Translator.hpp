/*
 * Author: Runor Ewhro
 * Description: translates text between languages with models installed on
 *              the device. when no model connects two languages, the text is
 *              simply not translated.
 */
#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace kstack::translation {

// one installed model; a model may read several source languages, such as
// Serbo-Croatian reading Serbian, Croatian, and Bosnian
struct ModelFiles {
    std::vector<std::string> sources;
    std::string target;
    std::filesystem::path config; // the model's Marian YAML configuration
};

// indices of the models that carry text from source to target: one model,
// or two through English when no direct model exists; empty when no route
// exists. Earlier models win, so callers list preferred models first.
[[nodiscard]] std::vector<std::size_t> findRoute(const std::vector<ModelFiles>& models,
                                                 std::string_view source, std::string_view target);

class Translator {
public:
    // runs on the translator's own thread, with nothing on failure
    using Done = std::function<void(std::optional<std::string>)>;
    virtual ~Translator() = default;
    [[nodiscard]] virtual bool canTranslate(std::string_view source,
                                            std::string_view target) const = 0;
    // queues the text and returns at once; false when no route exists, in
    // which case done is never called
    virtual bool translate(std::string text, std::string source, std::string target,
                           Done done) = 0;
};

// a translator over the given models; one that translates nothing when
// translation is not built into this copy of the app
[[nodiscard]] std::unique_ptr<Translator> createTranslator(std::vector<ModelFiles> models);

} // namespace kstack::translation
