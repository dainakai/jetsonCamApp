#pragma once
#include <tuple>
#include <stdint.h>

extern "C" std::tuple<float, float, float> calculateMeanStdDevK(const uint8_t* data, int size);
extern "C" int wrap_cudaSetDevice(int num);