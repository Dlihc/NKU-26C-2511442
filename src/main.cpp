#include "MainWindow.h"
#include <QApplication>
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("NKU-26C-MGS-tribute");
    MainWindow window;
    window.show();
    return app.exec();
}
