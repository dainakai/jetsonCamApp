#ifndef APPWINDOW_H
#define APPWINDOW_H

#include "camerahandler.h"

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QLabel>
#include <QTimer>
#include <QtConcurrent>
#include <QMessageBox>

class AppWindow : public QWidget {
    Q_OBJECT

public:
    AppWindow(CameraHandler& cameraHandler, QWidget *parent = nullptr)
    : QWidget(parent), cameraHandler(cameraHandler) {
        setupUI();
        startCamera();
    }

private slots:
    void onBrowseButtonClicked() {
        QString directory = QFileDialog::getExistingDirectory(this, "Select Directory");
        if (!directory.isEmpty()) {
            pathLineEdit->setText(directory);
        }
    }

    void onRecordButtonToggled(bool checked) {
        if (checked) {
            if (pathLineEdit->text().isEmpty()) {
                QMessageBox::critical(this, "Error", "Please select directory to save images.");
                recordButton->setChecked(false);
                return;
            }

            recordButton->setText("Stop");
            recording = true;
            imageCount = 0;
        } else {
            recordButton->setText("REC");
            recording = false;
        }
    }

    void updateImage() {
        QImage image = cameraHandler.captureImage();
        if (!image.isNull()) {
            imageView->setPixmap(QPixmap::fromImage(image));
            if (recording) {
                QtConcurrent::run(&cameraHandler, &CameraHandler::saveImage, image, pathLineEdit->text(), std::ref(imageCount));
            }
        }
    }

private:
    QPushButton *recordButton;
    QLineEdit *pathLineEdit;
    QLabel *imageView;
    CameraHandler& cameraHandler;
    QTimer *timer;

    bool recording = false;
    int imageCount = 0;

    void setupUI() {
        recordButton = new QPushButton("REC");
        recordButton->setCheckable(true);

        pathLineEdit = new QLineEdit();
        pathLineEdit->setPlaceholderText("Save at: ");

        QPushButton *browseButton = new QPushButton("Browse");

        imageView = new QLabel();
        imageView->setMinimumSize(640, 480);
        imageView->setAlignment(Qt::AlignCenter);
        imageView->setStyleSheet("QLabel { background-color: white; }");

        QHBoxLayout *topLayout = new QHBoxLayout;
        topLayout->addWidget(recordButton);
        topLayout->addWidget(pathLineEdit);
        topLayout->addWidget(browseButton);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(topLayout);
        mainLayout->addWidget(imageView);

        connect(browseButton, &QPushButton::clicked, this, &AppWindow::onBrowseButtonClicked);
        connect(recordButton, &QPushButton::toggled, this, &AppWindow::onRecordButtonToggled);

        // 画像更新用のタイマーを設定
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &AppWindow::updateImage);
    }

    void startCamera() {
        double frameRate = cameraHandler.getFrameRate();
        int interval = static_cast<int>(1000.0 / frameRate); // フレームレートをミリ秒に変換
        timer->start(interval);
    }
};

#endif // APPWINDOW_H