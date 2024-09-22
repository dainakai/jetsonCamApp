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
        std::cerr << "Runtime error: " << e.what() << std::endl;
        QMessageBox::critical(nullptr, "Error", e.what());
        return 1;
    } catch (const Spinnaker::Exception& e) {
        std::cerr << "Spinnaker error: " << e.what() << std::endl;
        QMessageBox::critical(nullptr, "Error", e.what());
        return 1;
    } catch (...) {
        std::cerr << "Unknown error." << std::endl;
        QMessageBox::critical(nullptr, "Error", "Unknown error.");
        return 1;
    }
}