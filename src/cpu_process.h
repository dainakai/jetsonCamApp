#ifndef CPU_PROCESS_H
#define CPU_PROCESS_H

#include <tuple>
#include <cmath>
#include <cstdint>

inline std::tuple<float, float, float> calculateMeanStdDevK(const uint8_t* data, int size) {
    // Calculate the mean
    float sum = 0.0f;
    for (int i = 0; i < size; ++i) {
        sum += static_cast<float>(data[i]);
    }
    float mean = sum / size;

    // Calculate the variance
    float sumSqDiff = 0.0f;
    for (int i = 0; i < size; ++i) {
        float diff = static_cast<float>(data[i]) - mean;
        sumSqDiff += diff * diff;
    }
    float variance = sumSqDiff / size;

    // Calculate the standard deviation
    float stdDev = std::sqrt(variance);

    // Calculate the coefficient of variation (k)
    float k = stdDev / mean;

    return std::make_tuple(mean, stdDev, k);
}

#endif // CPU_PROCESS_H