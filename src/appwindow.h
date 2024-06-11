#ifndef APPWINDOW_H
#define APPWINDOW_H

#include "camerahandler.h"
#include "cuda_functions.h"

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
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QThreadPool>

using namespace QtCharts;

class AppWindow : public QWidget {
    Q_OBJECT

public:
    AppWindow(CameraHandler& cameraHandler, QWidget *parent = nullptr)
    : QWidget(parent), cameraHandler(cameraHandler) {
        setupUI();
        startCamera();
    }

    ~AppWindow() {
        QThreadPool::globalInstance()->waitForDone();
        delete[] imageData;
        saveGraphData();
    }

private slots:
    void onBrowseButtonClicked() {
        try
        {
            timer->stop();
            cameraHandler.stopAcquisition();
        }
        catch(const std::exception& e)
        {
            QMessageBox::critical(this, "Error", e.what());
        }
        
        QString directory = QFileDialog::getExistingDirectory(this, "Select Directory");
        if (!directory.isEmpty()) {
            pathLineEdit->setText(directory);
        }

        try
        {
            timer->start(1000 / cameraHandler.getFrameRate());
            cameraHandler.startAcquisition();
        }
        catch(const std::exception& e)
        {
            QMessageBox::critical(this, "Error", e.what());
        }
    }

    void onBrowseGraphButtonClicked() {
        try
        {
            timer->stop();
            cameraHandler.stopAcquisition();
        }
        catch(const std::exception& e)
        {
            QMessageBox::critical(this, "Error", e.what());
        }
        
        QString directory = QFileDialog::getExistingDirectory(this, "Select Directory");
        if (!directory.isEmpty()) {
            pathLineEditforGraph->setText(directory);
        }

        try
        {
            timer->start(1000 / cameraHandler.getFrameRate());
            cameraHandler.startAcquisition();
        }
        catch(const std::exception& e)
        {
            QMessageBox::critical(this, "Error", e.what());
        }
    }

    void onRecordButtonToggled(bool checked) {
        if (checked) {
            if (pathLineEdit->text().isEmpty()) {
                QMessageBox::critical(this, "Error", "Please select directory to save images.");
                recordButton->setChecked(false);
                return;
            }

            recordButton->setText("□Stop");
            recording = true;
        } else {
            recordButton->setText("🔴REC");
            recording = false;
        }
    }

    void updateImage() {
        QImage image = cameraHandler.captureImage();
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count();
        lastUpdateTime = now;
        double fps = 1000.0 / elapsed;
        fpsLabel -> setText(QString("FPS: %1").arg(fps, 0, 'f', 2));
        if (!image.isNull()) {
            imageView->setPixmap(QPixmap::fromImage(image));
            if (recording) {
                QtConcurrent::run(&cameraHandler, &CameraHandler::saveImage, image, pathLineEdit->text(), frameCount);
            }

            QtConcurrent::run(this, &AppWindow::UpdateGraph, image, frameCount);

            frameCount++;
        }
    }

private:
    QPushButton *recordButton;
    QLineEdit *pathLineEdit;
    QLineEdit *pathLineEditforGraph;
    QLabel *imageView;
    CameraHandler& cameraHandler;
    QTimer *timer;
    QLabel *fpsLabel;
    std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();
    uint8_t* imageData;
    QChartView *chartView;
    QLineSeries *meanSeries;
    QLineSeries *stddevSeries;
    QLineSeries *kSeries;
    QValueAxis *axisX;
    QValueAxis *axisY;
    std::vector<int> frameCountData;
    std::vector<float> meanData;
    std::vector<float> stddevData;
    std::vector<float> kData;
    int Width = cameraHandler.getWidth();
    int Height = cameraHandler.getHeight();
    int frameCount = 0;


    bool recording = false;
    int imageCount = 0;

    void setupUI() {
        recordButton = new QPushButton("🔴REC");
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

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addLayout(topLayout);
        mainLayout->addWidget(imageView);

        connect(browseButton, &QPushButton::clicked, this, &AppWindow::onBrowseButtonClicked);
        connect(recordButton, &QPushButton::toggled, this, &AppWindow::onRecordButtonToggled);

        // 画像更新用のタイマーを設定
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &AppWindow::updateImage);

        fpsLabel = new QLabel(this);
        fpsLabel -> setAlignment(Qt::AlignCenter);
        mainLayout -> addWidget(fpsLabel);

        imageData = new uint8_t[Width * Height];

        // グラフの初期化
        chartView = new QChartView();
        chartView->setMinimumSize(400,300);
        QChart *chart = new QChart();
        chartView -> setChart(chart);

        meanSeries = new QLineSeries();
        stddevSeries = new QLineSeries();
        kSeries = new QLineSeries();

        meanSeries -> setName("Mean");
        stddevSeries -> setName("StdDev");
        kSeries -> setName("K = StdDev/Mean");

        chart -> addSeries(meanSeries);
        chart -> addSeries(stddevSeries);
        chart -> addSeries(kSeries);

        axisX = new QValueAxis();
        axisY = new QValueAxis();
        chart -> addAxis(axisX, Qt::AlignBottom);
        chart -> addAxis(axisY, Qt::AlignLeft);

        meanSeries -> attachAxis(axisX);
        meanSeries -> attachAxis(axisY);
        stddevSeries -> attachAxis(axisX);
        stddevSeries -> attachAxis(axisY);
        kSeries -> attachAxis(axisX);
        kSeries -> attachAxis(axisY);

        QVBoxLayout *graphLayout = new QVBoxLayout();
        pathLineEditforGraph = new QLineEdit();
        pathLineEditforGraph -> setPlaceholderText("Save at: ");
        QPushButton *browseButtonforGraph = new QPushButton("Browse");

        QHBoxLayout *graphTopLayout = new QHBoxLayout();
        graphTopLayout -> addWidget(pathLineEditforGraph);
        graphTopLayout -> addWidget(browseButtonforGraph);

        graphLayout -> addLayout(graphTopLayout);
        graphLayout -> addWidget(chartView);

        QHBoxLayout *windowLayout = new QHBoxLayout(this);
        windowLayout -> addLayout(mainLayout);
        windowLayout -> addLayout(graphLayout);

        connect(browseButtonforGraph, &QPushButton::clicked, this, &AppWindow::onBrowseGraphButtonClicked);

    }

    void startCamera() {
        double frameRate = cameraHandler.getFrameRate();
        int interval = static_cast<int>(1000.0 / frameRate); // フレームレートをミリ秒に変換
        timer->start(interval);
    }

    void UpdateGraph(QImage image, int frameNumber) {
        const uint8_t* data = image.bits();
        auto [mean, stddev, kurtosis] = calculateMeanStdDevK(data, Width * Height);
        meanSeries->append(frameNumber, mean/255.0);
        stddevSeries->append(frameNumber, stddev/255.0);
        kSeries->append(frameNumber, kurtosis);
        axisX->setRange(frameNumber-500, frameNumber);
        axisY->setRange(0, 1.0);
        if (frameNumber > 500) {
            meanSeries->remove(0);
            stddevSeries->remove(0);
            kSeries->remove(0);
        }
        frameCountData.push_back(frameNumber);
        meanData.push_back(mean/255.0);
        stddevData.push_back(stddev/255.0);
        kData.push_back(kurtosis);
    }

    void saveGraphData() {
        QFile file(pathLineEditforGraph->text() + "/graph_data.csv");
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Error", "Failed to save graph data.");
            return;
        }

        QTextStream stream(&file);
        stream << "Frame,Mean,StdDev,Kurtosis\n";
        for (int i = 0; i < frameCountData.size(); i++) {
            stream << frameCountData[i] << "," << meanData[i] << "," << stddevData[i] << "," << kData[i] << "\n";
        }
        file.close();
    }
};

#endif // APPWINDOW_H