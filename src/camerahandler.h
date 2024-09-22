#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include "Spinnaker.h"
#include <QImage>
#include <tuple>

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
        inittimestamp = pCam->GetNextImage()->GetTimeStamp();
        pCam->EndAcquisition();
        pCam->BeginAcquisition();
    }

    ~CameraHandler() {
        pCam->EndAcquisition();
        pCam = nullptr;
        camList.Clear();
        system->ReleaseInstance();
    }

    std::tuple<QImage, double, double> captureImage() {
        Spinnaker::ImagePtr pResultImage = nullptr;
        try
        {
            pResultImage = pCam->GetNextImage(1000);
            if (pResultImage->IsIncomplete())
            {
                std::cerr << "Image incomplete with image status " << pResultImage->GetImageStatus() << std::endl;
                std::cerr << "Image time stamp: " << pResultImage->GetTimeStamp() << std::endl;
                std::cerr << "Device temperature: " << pCam->DeviceTemperature.GetValue() << std::endl;
                std::cout << std::endl << std::endl;
                pResultImage->Release();
                return std::make_tuple(QImage(), 0, 0);
            }
            double temp = pCam->DeviceTemperature.GetValue();
            const size_t width = pResultImage->GetWidth();
            const size_t height = pResultImage->GetHeight();
            const size_t stride = pResultImage->GetStride();
            const double timestamp = (pResultImage->GetTimeStamp() - inittimestamp)/1000000000.0;
            Spinnaker::PixelFormatEnums pixelFormat = pResultImage->GetPixelFormat();
            const unsigned char *imageData = static_cast<const unsigned char*>(pResultImage->GetData());

            QImage image;
            if (pixelFormat == Spinnaker::PixelFormat_Mono8)
            {
                image = QImage(imageData, width, height, stride, QImage::Format_Grayscale8);
                pResultImage->Release();
                return std::make_tuple(image, timestamp, temp);
            }
            else
            {
                pResultImage->Release();
                throw std::runtime_error("Unsupported pixel format. Only Mono8 is supported. Please change the pixel format in SpinView.");
            }
        }
        catch (Spinnaker::Exception& e)
        {
            if (pResultImage != nullptr && !pResultImage->IsIncomplete())
            {
                pResultImage->Release();
            }
            std::cerr << "Error: " << e.what() << std::endl;
            return std::make_tuple(QImage(), 0, 0);
        }
        catch (std::exception& e)
        {
            if (pResultImage != nullptr && !pResultImage->IsIncomplete())
            {
                pResultImage->Release();
            }
            std::cerr << "Error: " << e.what() << std::endl;
            return std::make_tuple(QImage(), 0, 0);
        }
        catch (...)
        {
            if (pResultImage != nullptr && !pResultImage->IsIncomplete())
            {
                pResultImage->Release();
            }
            std::cerr << "Unknown error." << std::endl;
            return std::make_tuple(QImage(), 0, 0);
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
    uint64_t inittimestamp;
};

#endif // CAMERAHANDLER_H