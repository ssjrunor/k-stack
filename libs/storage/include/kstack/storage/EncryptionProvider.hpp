#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace kstack::storage {

// Persistence must supply a real key-backed provider. There is no plaintext fallback.
class EncryptionProvider {
public:
    virtual ~EncryptionProvider() = default;
    [[nodiscard]] virtual std::vector<std::uint8_t> encrypt(std::span<const std::uint8_t> data) = 0;
    [[nodiscard]] virtual std::vector<std::uint8_t> decrypt(std::span<const std::uint8_t> data) = 0;
};

} // namespace kstack::storage
