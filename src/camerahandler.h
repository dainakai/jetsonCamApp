#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include "Spinnaker.h"
#include <QImage>

class CameraHandler {
public:
    CameraHandler() {
        system = Spinnaker::System::GetInstance();
        camList = system->GetCameras();
        if (camList.GetSize() == 0) {
            camList.Clear();
            system->ReleaseInstance();
            throw std::runtime_error("No cameras found.");
        }
        pCam = camList.GetByIndex(0);
        pCam->Init();
        pCam->BeginAcquisition();
        pCam->EndAcquisition();
        pCam->BeginAcquisition();
    }

    ~CameraHandler() {
        pCam->EndAcquisition();
        pCam = nullptr;
        camList.Clear();
        system->ReleaseInstance();
    }

    QImage captureImage() {
        Spinnaker::ImagePtr pResultImage = pCam->GetNextImage();
        const size_t width = pResultImage->GetWidth();
        const size_t height = pResultImage->GetHeight();
        const size_t stride = pResultImage->GetStride();
        Spinnaker::PixelFormatEnums pixelFormat = pResultImage->GetPixelFormat();
        const unsigned char *imageData = static_cast<const unsigned char*>(pResultImage->GetData());

        QImage image;
        if (pixelFormat == Spinnaker::PixelFormat_Mono8)
        {
            image = QImage(imageData, width, height, stride, QImage::Format_Grayscale8);
            return image;
        }
        else if (pixelFormat == Spinnaker::PixelFormat_RGB8)
        {
            image = QImage(imageData, width, height, stride, QImage::Format_RGB888);
            return image;
        }else
        {
            image = QImage(width, height, QImage::Format_RGB888);
            return image;
        }
    }

    double getFrameRate() {
        return pCam->AcquisitionFrameRate.GetValue();
    }

    int getWidth() {
        return pCam->Width.GetValue();
    }

    int getHeight() {
        return pCam->Height.GetValue();
    }

    void saveImage(const QImage& image, const QString& directory, int imageCount) {
        QString filename = QString("%1/%2.bmp").arg(directory).arg(imageCount, 6, 10, QLatin1Char('0'));
        image.save(filename);
    }

    void stopAcquisition() {
        pCam->EndAcquisition();
    }

    void startAcquisition() {
        pCam->BeginAcquisition();
    }

private:
    Spinnaker::SystemPtr system;
    Spinnaker::CameraList camList;
    Spinnaker::CameraPtr pCam;
};

#endif // CAMERAHANDLER_H