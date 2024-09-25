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
#include <QMutex>
#include <QMutexLocker>
#include <QCoreApplication>

using namespace QtCharts;

class AppWindow : public QWidget {
    Q_OBJECT

public:
    AppWindow(CameraHandler& cameraHandler, QWidget *parent = nullptr)
        : QWidget(parent), cameraHandler(cameraHandler) {
        QThreadPool::globalInstance()->setMaxThreadCount(4);
        setupUI();
        if (!wrap_cudaSetDevice(0)) {
            QMessageBox::critical(this, "Error", "Failed to set CUDA device.");
        }
        startCamera();

        // シグナルとスロットの接続
        connect(this, &AppWindow::graphDataReady, this, &AppWindow::onGraphDataReady);
    }

    ~AppWindow() {
        QThreadPool::globalInstance()->waitForDone();
        QMutexLocker locker(&dataMutex);
        saveGraphData(frameCountData, meanData, stddevData, kData, timestampData, tempData);
    }

signals:
    void graphDataReady(int frameNumber, double timestamp, double temp, float mean, float stddev, float cv);

private slots:
    void onBrowseButtonClicked() {
        try {
            timer->stop();
            cameraHandler.stopAcquisition();
        } catch(const std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
        }

        QString directory = QFileDialog::getExistingDirectory(this, "Select Directory");
        if (!directory.isEmpty()) {
            pathLineEdit->setText(directory);
        }

        try {
            startCamera();
            cameraHandler.startAcquisition();
        } catch(const std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
        }
    }

    void onBrowseGraphButtonClicked() {
        try {
            timer->stop();
            cameraHandler.stopAcquisition();
        } catch(const std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
        }

        QString directory = QFileDialog::getExistingDirectory(this, "Select Directory");
        if (!directory.isEmpty()) {
            pathLineEditforGraph->setText(directory);
        }

        try {
            startCamera();
            cameraHandler.startAcquisition();
        } catch(const std::exception& e) {
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
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdateTime).count();

        QImage image;
        double timestamp;
        double temp;
        std::tie(image, timestamp, temp) = cameraHandler.captureImage();

        lastUpdateTime = now;
        sumProcTime += elapsed;

        if (frameCount % FrameRate == 0) {
            currentProcTime = sumProcTime / FrameRate;
            sumProcTime = 0.0;
        }

        double fps = 1000000.0 / currentProcTime;
        fpsLabel->setText(QString("Processed FPS: %1\n").arg(fps, 0, 'f', 2) + QString("Camera FPS: %3").arg(1.0/(timestamp-latestCameraTimeStamp), 0, 'f', 2));
        latestCameraTimeStamp = timestamp;
        timestampLabel->setText(QString("Timestamp: %1").arg(timestamp, 0, 'f', 2));
        tempLabel->setText(QString("Temperature: %1").arg(temp, 0, 'f', 2));

        if (!image.isNull()) {
            imageView->setPixmap(QPixmap::fromImage(image));
            QImage imageCopy = image.copy();
            QtConcurrent::run(this, &AppWindow::resultProcessing, imageCopy, std::make_tuple(frameCount, timestamp, temp), pathLineEdit->text(), recording);
            QCoreApplication::processEvents();
            frameCount++;
        }
    }

    void onGraphDataReady(int frameNumber, double timestamp, double temp, float mean, float stddev, float cv) {
        UpdateGraph(frameNumber, timestamp, temp, mean, stddev, cv);
    }

private:
    QPushButton *recordButton;
    QLineEdit *pathLineEdit;
    QLineEdit *pathLineEditforGraph;
    QLabel *imageView;
    CameraHandler& cameraHandler;
    QTimer *timer;
    QLabel *fpsLabel;
    QLabel *timestampLabel;
    QLabel *tempLabel;
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
    std::vector<double> timestampData;
    std::vector<double> tempData;

    QChartView *trendChartView;
    QLineSeries *trendMeanSeries;
    QLineSeries *trendStddevSeries;
    QLineSeries *trendKSeries;
    QValueAxis *trendAxisX;
    QValueAxis *trendAxisY;
    float trendMean = 0;
    float trendStddev = 0;
    float trendK = 0;
    double latestCameraTimeStamp = 0.0;
    double sumProcTime = 0.0;
    double currentProcTime = 0.0;

    const int trendInterval = 30;
    const int saveImageInterval = 600;
    const int saveGraphInterval = 6000;

    int Width = cameraHandler.getWidth();
    int Height = cameraHandler.getHeight();
    int FrameRate = cameraHandler.getFrameRate();
    int frameCount = 0;

    bool recording = false;
    int imageCount = 0;

    QMutex dataMutex; // データ保護用のMutex

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
        fpsLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(fpsLabel);

        timestampLabel = new QLabel(this);
        timestampLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(timestampLabel);

        tempLabel = new QLabel(this);
        tempLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(tempLabel);

        imageData = new uint8_t[Width * Height];

        // グラフの初期化
        chartView = new QChartView();
        chartView->setMinimumSize(400, 300);
        QChart *chart = new QChart();
        chartView->setChart(chart);

        meanSeries = new QLineSeries();
        stddevSeries = new QLineSeries();
        kSeries = new QLineSeries();

        meanSeries->setName("Mean");
        stddevSeries->setName("StdDev");
        kSeries->setName("K = StdDev/Mean");

        chart->addSeries(meanSeries);
        chart->addSeries(stddevSeries);
        chart->addSeries(kSeries);

        axisX = new QValueAxis();
        axisY = new QValueAxis();
        chart->addAxis(axisX, Qt::AlignBottom);
        chart->addAxis(axisY, Qt::AlignLeft);

        meanSeries->attachAxis(axisX);
        meanSeries->attachAxis(axisY);
        stddevSeries->attachAxis(axisX);
        stddevSeries->attachAxis(axisY);
        kSeries->attachAxis(axisX);
        kSeries->attachAxis(axisY);

        QVBoxLayout *graphLayout = new QVBoxLayout();
        pathLineEditforGraph = new QLineEdit();
        pathLineEditforGraph->setPlaceholderText("Save at: ");
        QPushButton *browseButtonforGraph = new QPushButton("Browse");

        QHBoxLayout *graphTopLayout = new QHBoxLayout();
        graphTopLayout->addWidget(pathLineEditforGraph);
        graphTopLayout->addWidget(browseButtonforGraph);

        graphLayout->addLayout(graphTopLayout);
        graphLayout->addWidget(chartView);

        QHBoxLayout *windowLayout = new QHBoxLayout(this);
        windowLayout->addLayout(mainLayout);
        windowLayout->addLayout(graphLayout);

        connect(browseButtonforGraph, &QPushButton::clicked, this, &AppWindow::onBrowseGraphButtonClicked);

        // トレンドグラフの初期化
        trendChartView = new QChartView();
        trendChartView->setMinimumSize(400, 300);
        QChart *trendChart = new QChart();
        trendChartView->setChart(trendChart);

        trendMeanSeries = new QLineSeries();
        trendStddevSeries = new QLineSeries();
        trendKSeries = new QLineSeries();

        trendMeanSeries->setName("Mean");
        trendStddevSeries->setName("StdDev");
        trendKSeries->setName("K = StdDev/Mean");

        trendChart->addSeries(trendMeanSeries);
        trendChart->addSeries(trendStddevSeries);
        trendChart->addSeries(trendKSeries);

        trendAxisX = new QValueAxis();
        trendAxisY = new QValueAxis();
        trendChart->addAxis(trendAxisX, Qt::AlignBottom);
        trendChart->addAxis(trendAxisY, Qt::AlignLeft);

        trendMeanSeries->attachAxis(trendAxisX);
        trendMeanSeries->attachAxis(trendAxisY);
        trendStddevSeries->attachAxis(trendAxisX);
        trendStddevSeries->attachAxis(trendAxisY);
        trendKSeries->attachAxis(trendAxisX);
        trendKSeries->attachAxis(trendAxisY);

        windowLayout->addWidget(trendChartView);
    }

    void startCamera() {
        double frameRate = cameraHandler.getFrameRate()*1.5; // QTimerは処理が遅延したらその分後ろにズレてトリガーされるためなるだけはやくしておいてよい。
        int interval = static_cast<int>(1000.0 / frameRate); // フレームレートをミリ秒に変換
        timer->start(interval);
    }

    void UpdateGraph(int frameNumber, double timestamp, double temp, float mean, float stddev, float cv) {
        QMutexLocker locker(&dataMutex);

        // データを更新
        frameCountData.push_back(frameNumber);
        meanData.push_back(mean / 255.0f);
        stddevData.push_back(stddev / 255.0f);
        kData.push_back(cv);
        timestampData.push_back(timestamp);
        tempData.push_back(temp);

        // グラフを更新
        meanSeries->append(frameNumber, mean / 255.0f);
        stddevSeries->append(frameNumber, stddev / 255.0f);
        kSeries->append(frameNumber, cv);
        axisX->setRange(frameNumber - 500, frameNumber);
        axisY->setRange(0, 1.0);

        if (frameNumber > 500) {
            meanSeries->remove(0);
            stddevSeries->remove(0);
            kSeries->remove(0);
        }

        // トレンドデータの更新
        if (frameNumber % trendInterval != 0) {
            trendMean += mean / 255.0f;
            trendStddev += stddev / 255.0f;
            trendK += cv;
        } else {
            trendMeanSeries->append(frameNumber, trendMean / trendInterval);
            trendStddevSeries->append(frameNumber, trendStddev / trendInterval);
            trendKSeries->append(frameNumber, trendK / trendInterval);
            trendAxisX->setRange(0, frameNumber);
            trendAxisY->setRange(0, 1.0);
            trendMean = 0;
            trendStddev = 0;
            trendK = 0;
        }
    }

    void saveGraphData(const std::vector<int>& saveframeCountData, const std::vector<float>& savemeanData,
                       const std::vector<float>& savestddevData, const std::vector<float>& savekData,
                       const std::vector<double>& savetimestampData, const std::vector<double>& savetempData) {
        QString filePath = pathLineEditforGraph->text() + "/graph_data.csv";
        QFile file(filePath);

        bool fileExists = file.exists();

        if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Failed to open graph data file for writing.");
            return;
        }

        QTextStream stream(&file);

        // 新規ファイルの場合はヘッダーを書き込む
        if (!fileExists) {
            stream << "Frame,TimeStamp,Temperature,Mean,StdDev,CV\n";
        }

        // データを追記
        for (size_t i = 0; i < saveframeCountData.size(); ++i) {
            stream << saveframeCountData[i] << ","
                   << savetimestampData[i] << ","
                   << savetempData[i] << ","
                   << savemeanData[i] << ","
                   << savestddevData[i] << ","
                   << savekData[i] << "\n";
        }

        file.close();

        // データベクターをクリア
        frameCountData.clear();
        meanData.clear();
        stddevData.clear();
        kData.clear();
        timestampData.clear();
        tempData.clear();
    }

    void resultProcessing(QImage image, std::tuple<int, double, double>(tuple), const QString& directory, bool recording) {
        try {
            int frameNumber;
            double timestamp;
            double temp;
            std::tie(frameNumber, timestamp, temp) = tuple;
            const uint8_t* data = image.bits();
            auto [mean, stddev, cv] = calculateMeanStdDevK(data, Width * Height);

            if (!std::isfinite(mean) || !std::isfinite(stddev) || !std::isfinite(cv)) {
                mean = 0.0f;
                stddev = 0.0f;
                cv = 0.0f;
                std::cerr << "Invalid value detected at frame " << frameNumber << std::endl;
            }

            // シグナルを発行してメインスレッドにデータを送信
            emit graphDataReady(frameNumber, timestamp, temp, mean, stddev, cv);

            // 画像の保存
            if (recording && frameNumber % saveImageInterval == 0) {
                cameraHandler.saveImage(std::move(image), directory, frameNumber);
            }

            // グラフデータの保存
            if (recording && frameNumber % saveGraphInterval == 0) {
                QMutexLocker locker(&dataMutex);
                saveGraphData(frameCountData, meanData, stddevData, kData, timestampData, tempData);
            }
        } catch(const std::exception& e) {
            // ワーカースレッドから直接GUI要素を操作しない
            std::cerr << e.what() << std::endl;
        }
    }
};

#endif // APPWINDOW_H
