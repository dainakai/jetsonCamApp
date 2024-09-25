// 20240923 Mutex導入前。

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
        QThreadPool::globalInstance()->setMaxThreadCount(4);
        setupUI();
        startCamera();
    }

    ~AppWindow() {
        QThreadPool::globalInstance()->waitForDone();
        saveGraphData(frameCountData, meanData, stddevData, kData, timestampData);
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
            startCamera();
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
            startCamera();
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
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdateTime).count();

        QImage image;
        double timestamp;
        double temp;
        std::tie(image, timestamp, temp) = cameraHandler.captureImage();

        lastUpdateTime = now;
        double fps = 1000000.0 / elapsed;
        fpsLabel -> setText(QString("FPS: %3").arg(fps, 0, 'f', 2));
        timestampLabel -> setText(QString("Timestamp: %1").arg(timestamp, 0, 'f', 2));
        tempLabel -> setText(QString("Temperature: %1").arg(temp, 0, 'f', 2));
        // std::cout << "Timestamp: " << timestamp << ",\tTemperature: " << temp << std::endl;
        if (!image.isNull()) {
            imageView->setPixmap(QPixmap::fromImage(image));
            QtConcurrent::run(this, &AppWindow::resultProcessing, std::move(image), frameCount, timestamp, pathLineEdit->text(), recording);
            QCoreApplication::processEvents();
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

    QChartView *trendChartView;
    QLineSeries *trendMeanSeries;
    QLineSeries *trendStddevSeries;
    QLineSeries *trendKSeries;
    QValueAxis *trendAxisX;
    QValueAxis *trendAxisY;
    float trendMean = 0;
    float trendStddev = 0;
    float trendK = 0;

    const int trendInterval = 30;
    const int saveImageInterval = 600;
    const int saveGraphInterval = 6000;

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

        timestampLabel = new QLabel(this);
        timestampLabel -> setAlignment(Qt::AlignCenter);
        mainLayout -> addWidget(timestampLabel);

        tempLabel = new QLabel(this);
        tempLabel -> setAlignment(Qt::AlignCenter);
        mainLayout -> addWidget(tempLabel);

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

        // トレンドグラフの初期化
        trendChartView = new QChartView();
        trendChartView->setMinimumSize(400,300);
        QChart *trendChart = new QChart();
        trendChartView -> setChart(trendChart);

        trendMeanSeries = new QLineSeries();
        trendStddevSeries = new QLineSeries();
        trendKSeries = new QLineSeries();

        trendMeanSeries -> setName("Mean");
        trendStddevSeries -> setName("StdDev");
        trendKSeries -> setName("K = StdDev/Mean");

        trendChart -> addSeries(trendMeanSeries);
        trendChart -> addSeries(trendStddevSeries);
        trendChart -> addSeries(trendKSeries);
        
        trendAxisX = new QValueAxis();
        trendAxisY = new QValueAxis();
        trendChart -> addAxis(trendAxisX, Qt::AlignBottom);
        trendChart -> addAxis(trendAxisY, Qt::AlignLeft);

        trendMeanSeries -> attachAxis(trendAxisX);
        trendMeanSeries -> attachAxis(trendAxisY);
        trendStddevSeries -> attachAxis(trendAxisX);
        trendStddevSeries -> attachAxis(trendAxisY);
        trendKSeries -> attachAxis(trendAxisX);
        trendKSeries -> attachAxis(trendAxisY);

        windowLayout -> addWidget(trendChartView);

    }

    void startCamera() {
        double frameRate = cameraHandler.getFrameRate();
        int interval = static_cast<int>(1000.0 / frameRate); // フレームレートをミリ秒に変換
        timer->start(1);
    }

    void UpdateGraph(QImage image, int frameNumber, double timestamp) {
        const uint8_t* data = image.bits();
        auto [mean, stddev, kurtosis] = calculateMeanStdDevK(data, Width * Height);
        
        if (!std::isfinite(mean) || !std::isfinite(stddev) || !std::isfinite(kurtosis)) {
            mean = 0.0;
            stddev = 0.0;
            kurtosis = 0.0;
            std::cerr << "Invalid value detected at frame " << frameNumber << std::endl;
        }

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
        timestampData.push_back(timestamp);

        if (frameNumber % trendInterval != 0) {
            trendMean += mean/255.0;
            trendStddev += stddev/255.0;
            trendK += kurtosis;
        }else{
            trendMeanSeries->append(frameNumber, trendMean/trendInterval);
            trendStddevSeries->append(frameNumber, trendStddev/trendInterval);
            trendKSeries->append(frameNumber, trendK/trendInterval);
            trendAxisX->setRange(0, frameNumber);
            trendAxisY->setRange(0, 1.0);
            trendMean = 0;
            trendStddev = 0;
            trendK = 0;
        }
    }

    void saveGraphData(const std::vector<int> saveframeCountData, const  std::vector<float> savemeanData, const std::vector<float> savestddevData, const std::vector<float> savekData, const std::vector<double> savetimestampData) {
        frameCountData.clear();
        meanData.clear();
        stddevData.clear();
        kData.clear();
        timestampData.clear();

        QString filePath = pathLineEditforGraph->text() + "/graph_data.csv";
        QFile file(filePath);
        
        bool fileExists = file.exists();
        
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Failed to open graph data file for writing.");
            return;
        }

        QTextStream stream(&file);

        // ファイルが新規作成された場合のみヘッダーを書き込む
        if (!fileExists) {
            stream << "Frame,TimeStamp,Mean,StdDev,Kurtosis\n";
        }

        // 新しいデータを追記
        for (size_t i = 0; i < saveframeCountData.size(); ++i) {
            stream << saveframeCountData[i] << "," 
                << savetimestampData[i] << ","
                << savemeanData[i] << "," 
                << savestddevData[i] << "," 
                << savekData[i] << "\n";
        }

        file.close();
    }

    void resultProcessing(QImage image, int frameNumber, double timestamp, const QString& directory, bool recording) {
        try
        {
            UpdateGraph(image, frameNumber, timestamp);
            if (recording && frameNumber % saveImageInterval == 0) {
                cameraHandler.saveImage(std::move(image), directory, frameNumber);
            }
            if (recording && frameNumber % saveGraphInterval == 0) {
                saveGraphData(frameCountData, meanData, stddevData, kData, timestampData);
            }
        }
        catch(const std::exception& e)
        {
            QMessageBox::critical(this, "Error", e.what());
            std::cerr << e.what() << std::endl;
        }
    }
};

#endif // APPWINDOW_H