/*
 * Author: Runor Ewhro
 * Description: starts the clipboard app or a macOS encrypted-history stress
 *              verification command.
 */
#include "Application.hpp"
#ifdef KSTACK_ENABLE_STRESS_PROFILE
#include "stress/StressProfile.hpp"
#endif

#include <QApplication>
#include <QCoreApplication>

#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    const std::string_view mode = argc > 1 ? std::string_view(argv[1]) : std::string_view{};
#ifdef KSTACK_ENABLE_STRESS_PROFILE
    if (mode == "--stress-enabled" && argc == 2) {
        std::cout << "kstack-stress-v1\n";
        return 0;
    }
    if ((mode == "--stress-report" || mode == "--stress-cleanup" ||
         mode == "--stress-verify-live") && argc == 3) {
        QCoreApplication qtApp(argc, argv);
        QCoreApplication::setApplicationName("Clipboard History");
        try {
            if (mode == "--stress-verify-live") {
                kstack::desktop::verifyLiveStressEntries(QString::fromUtf8(argv[2]));
            } else {
                const auto profile = kstack::desktop::validatedStressProfile(QString::fromUtf8(argv[2]));
                if (mode == "--stress-report") {
                    kstack::desktop::reportStressProfile(profile);
                } else {
                    kstack::desktop::removeStressProfileKey(profile);
                }
            }
            return 0;
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 2;
        }
    }
#endif
    if (mode.starts_with("--stress-") && mode != "--stress-profile") {
        std::cerr << "Stress profile command unavailable\n";
        return 2;
    }
    if (mode == "--stress-profile" && argc != 3) {
        std::cerr << "Stress profile needs one temporary directory\n";
        return 2;
    }
#ifndef KSTACK_ENABLE_STRESS_PROFILE
    if (mode == "--stress-profile") {
        std::cerr << "Stress profile support is not built\n";
        return 2;
    }
#endif
    QApplication qtApp(argc, argv);
    QApplication::setApplicationName("Clipboard History");
    kstack::desktop::Application application;
#ifdef KSTACK_ENABLE_STRESS_PROFILE
    if (mode == "--stress-profile") {
        try {
            const auto profile = kstack::desktop::validatedStressProfile(QString::fromUtf8(argv[2]));
            if (!application.start(profile.directory, profile.keyService)) {
                return 1;
            }
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 2;
        }
    } else
#endif
    if (!application.start()) {
        return 1;
    }
    return QApplication::exec();
}
