/*
 * Author: Runor Ewhro
 * Description: starts the desktop clipboard history process.
 */
#include "Application.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication qtApp(argc, argv);
    QApplication::setApplicationName("Clipboard History");
    kstack::desktop::Application application;
    if (!application.start()) {
        return 1;
    }
    return QApplication::exec();
}
