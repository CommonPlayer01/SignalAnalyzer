// ============================================================================
// 测试公共辅助（header-only，供各测试文件 include）
// 提供确定性伪随机数、朴素 DFT 对拍实现、整周期正弦构造与峰值查找。
// ============================================================================
#pragma once

#include "SignalCoreLib/Fft.h"
#include "SignalCoreLib/SpectrumAnalyzer.h"

#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

namespace SignalCoreLibTest {

inline constexpr double kPi = 3.14159265358979323846;

// 确定性伪随机数（线性同余），保证测试可重复。
inline double pseudoRandom(unsigned long& seed) {
    seed = seed * 1103515245UL + 12345UL;
    return static_cast<double>((seed >> 8) % 2001) / 1000.0 - 1.0;
}

// 朴素 O(N^2) DFT，作为 FFT 的对拍参考实现。
inline void naiveDft(const std::vector<SignalCoreLib::Fft::Complex>& in,
                     std::vector<SignalCoreLib::Fft::Complex>& out) {
    const std::size_t n = in.size();
    out.assign(n, SignalCoreLib::Fft::Complex(0.0, 0.0));
    for (std::size_t k = 0; k < n; ++k) {
        for (std::size_t t = 0; t < n; ++t) {
            const double angle = -2.0 * kPi * static_cast<double>((k * t) % n)
                                 / static_cast<double>(n);
            out[k] += in[t] * SignalCoreLib::Fft::Complex(std::cos(angle), std::sin(angle));
        }
    }
}

// 整周期采样的单频正弦：N*f0/fs 为整数，频谱恰好落在 FFT 频点上。
inline std::vector<double> makeIntegerPeriodSine(double fs, std::size_t n,
                                                 double f0, double a) {
    std::vector<double> x(n);
    for (std::size_t i = 0; i < n; ++i) {
        x[i] = a * std::sin(2.0 * kPi * f0 * static_cast<double>(i) / fs);
    }
    return x;
}

// 返回单边谱中幅值最大的频点（排除 DC）。
inline std::size_t findPeakBin(const SignalCoreLib::Spectrum& spec) {
    std::size_t peak = 0;
    double peakMag = -1.0;
    for (std::size_t k = 1; k < spec.magnitudes.size(); ++k) {
        if (spec.magnitudes[k] > peakMag) {
            peakMag = spec.magnitudes[k];
            peak = k;
        }
    }
    return peak;
}

} // namespace SignalCoreLibTest
