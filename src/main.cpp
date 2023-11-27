#include "appwindow.h"
#include "camerahandler.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    try {
        CameraHandler cameraHandler;
        AppWindow window(cameraHandler);
        window.show();
        return app.exec();
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(nullptr, "Error", e.what());
        return 1;
    }
}