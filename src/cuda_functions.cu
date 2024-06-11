#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/universal_vector.h>
#include <thrust/reduce.h>
#include <thrust/functional.h>
#include <thrust/transform_reduce.h>
#include <tuple>

#ifdef __x86_64__
extern "C" std::tuple<float, float, float> calculateMeanStdDevK(const uint8_t* data, int size) {
    thrust::device_vector<uint8_t> d_data(size);
    thrust::copy(data, data + size, d_data.begin());

    float mean = static_cast<float>(thrust::reduce(d_data.begin(), d_data.end(), 0, thrust::plus<int>())) / size;

    thrust::device_vector<float> d_diff(size);
    thrust::transform(d_data.begin(), d_data.end(), d_diff.begin(),
                      [mean] __device__ (uint8_t val) { return (static_cast<float>(val) - mean) * (static_cast<float>(val) - mean); });

    float variance = thrust::reduce(d_diff.begin(), d_diff.end(), 0.0f, thrust::plus<float>()) / size;
    float stdDev = sqrtf(variance);

    float k = stdDev / mean;

    return std::make_tuple(mean, stdDev, k);
}
#elif __aarch64__
extern "C" std::tuple<float, float, float> calculateMeanStdDevK(uint8_t* data, int size) {
    thrust::universal_vector<uint8_t> d_data(size);
    thrust::copy(data, data + size, d_data.begin());

    float mean = static_cast<float>(thrust::reduce(d_data.begin(), d_data.end(), 0, thrust::plus<int>())) / size;

    thrust::device_vector<float> d_diff(size);
    thrust::transform(d_data.begin(), d_data.end(), d_diff.begin(),
                      [mean] __device__ (uint8_t val) { return (static_cast<float>(val) - mean) * (static_cast<float>(val) - mean); });

    float variance = thrust::reduce(d_diff.begin(), d_diff.end(), 0.0f, thrust::plus<float>()) / size;
    float stdDev = sqrtf(variance);

    float k = stdDev / mean;

    return std::make_tuple(mean, stdDev, k);
}
#else
#error "Unsupported architecture"
#endif