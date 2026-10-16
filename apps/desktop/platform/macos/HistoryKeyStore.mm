/*
 * Author: Runor Ewhro
 * Description: creates a local Keychain key only for a new history database;
 *              stress profiles use a separate service and can remove it.
 */
#include "HistoryKeyStore.hpp"

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include <cstring>
#include <optional>
#include <stdexcept>

namespace kstack::desktop {
namespace {
NSDictionary* lookupQuery(const std::string& service) {
    NSString* serviceName = [NSString stringWithUTF8String:service.c_str()];
    if (serviceName == nil) {
        throw std::runtime_error("Invalid history key service");
    }
    return @{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrService: serviceName,
        (__bridge id)kSecAttrAccount: @"database-key",
        (__bridge id)kSecAttrSynchronizable: @NO,
        (__bridge id)kSecMatchLimit: (__bridge id)kSecMatchLimitOne,
        (__bridge id)kSecReturnData: @YES,
    };
}

std::optional<std::array<std::uint8_t, 32>> readKey(const std::string& service) {
    CFTypeRef result = nullptr;
    const OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)lookupQuery(service), &result);
    if (status == errSecItemNotFound) {
        return std::nullopt;
    }
    if (status != errSecSuccess) {
        throw std::runtime_error("Cannot read the history key from Keychain");
    }
    NSData* data = CFBridgingRelease(result);
    if (data.length != 32) {
        throw std::runtime_error("The Keychain history key has an invalid length");
    }
    std::array<std::uint8_t, 32> key{};
    std::memcpy(key.data(), data.bytes, key.size());
    return key;
}
} // namespace

std::array<std::uint8_t, 32> historyDatabaseKey(bool databaseExists, const std::string& service) {
    if (const auto existing = readKey(service)) {
        return *existing;
    }
    if (databaseExists) {
        throw std::runtime_error("History exists, but its Keychain key is missing");
    }
    std::array<std::uint8_t, 32> key{};
    if (SecRandomCopyBytes(kSecRandomDefault, key.size(), key.data()) != errSecSuccess) {
        throw std::runtime_error("Cannot generate a history key");
    }
    NSMutableDictionary* attributes = [lookupQuery(service) mutableCopy];
    [attributes removeObjectForKey:(__bridge id)kSecMatchLimit];
    [attributes removeObjectForKey:(__bridge id)kSecReturnData];
    attributes[(__bridge id)kSecValueData] = [NSData dataWithBytes:key.data() length:key.size()];
    const OSStatus status = SecItemAdd((__bridge CFDictionaryRef)attributes, nullptr);
    if (status == errSecDuplicateItem) {
        if (const auto existing = readKey(service)) {
            return *existing;
        }
        throw std::runtime_error("History key changed during creation");
    }
    if (status != errSecSuccess) {
        throw std::runtime_error("Cannot save the history key to Keychain");
    }
    return key;
}

#ifdef KSTACK_ENABLE_STRESS_PROFILE
void deleteStressHistoryKey(const std::string& service) {
    if (!service.starts_with("org.runorewhro.clipboard-history.stress.")) {
        throw std::runtime_error("Refusing to remove a non-stress history key");
    }
    NSDictionary* query = @{
        (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
        (__bridge id)kSecAttrService: [NSString stringWithUTF8String:service.c_str()],
        (__bridge id)kSecAttrAccount: @"database-key",
    };
    const OSStatus status = SecItemDelete((__bridge CFDictionaryRef)query);
    if (status != errSecSuccess && status != errSecItemNotFound) {
        throw std::runtime_error("Cannot remove the stress history key from Keychain (status " +
                                 std::to_string(status) + ")");
    }
}
#endif
} // namespace kstack::desktop
