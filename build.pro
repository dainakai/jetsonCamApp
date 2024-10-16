TEMPLATE = app
TARGET = jetsonCamApp
QT += widgets concurrent charts
OBJECTS_DIR = obj
MOC_DIR = moc
SOURCES += src/main.cpp
HEADERS += src/appwindow.h src/camerahandler.h src/cpu_process.h
# CUDA_DIR = /usr/local/cuda
# INCLUDEPATH += $$CUDA_DIR/include
CONFIG += c++17

# Linux用の設定
unix:!macx {
    # 64ビットx86アーキテクチャの場合
    contains(QT_ARCH, "x86_64") {
        message("Configuring for x86_64 Linux")
        INCLUDEPATH += /opt/spinnaker/include/
        LIBS += -L/opt/spinnaker/lib -lSpinnaker -L./obj #-lcuda_functions -L$$CUDA_DIR/lib64 -lcudart
    }

    # 64ビットARMアーキテクチャの場合
    contains(QT_ARCH, "arm64") {
        message("Configuring for arm64 Linux")
        INCLUDEPATH += /opt/spinnaker/include/
        LIBS += -L/opt/spinnaker/lib -lSpinnaker -L./obj #-lcuda_functions -L$$CUDA_DIR/lib64 -lcudart
    }
}

# Windows用の設定
win32 {
    # Windows特有の設定
}
