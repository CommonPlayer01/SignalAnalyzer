// ============================================================================
// FFT 单元测试（GoogleTest）
// 对应《测试用例表》中 FFT 相关的 4 个用例：
//   1. 整周期正弦：谱峰频率与幅值正确；
//   2. 与朴素 DFT 对拍：任意复数序列一致；
//   3. 单位冲激：频谱平坦；
//   4. Parseval：能量守恒。
// 运行：
//   本套件： build\Release\SignalCoreLibTests.exe --gtest_filter=FftTest.*
// ============================================================================

#include <gtest/gtest.h>

#include "SignalCoreLib/Fft.h"
#include "TestCommon.h"

#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

using Complex = SignalCoreLib::Fft::Complex;


TEST(FftTest, IntegerPeriodSinePeakFrequencyAndAmplitude) {
    const double fs = 256.0;
    const std::size_t n = 64;
    const double f0 = 16.0;
    const double amplitude = 2.0;

    const auto samples = SignalCoreLibTest::makeIntegerPeriodSine(fs, n, f0, amplitude);

    std::vector<Complex> x(n);
    for (std::size_t i = 0; i < n; ++i)
        x[i] = Complex(samples[i], 0.0);

    Fft::forward(x);

    // k0 = 4 * <delta>f 频点 k0 = f0 * n / fs，幅值 = amplitude * n / 2（单边谱放大）。
    const std::size_t k0 = static_cast<std::size_t>(f0 * static_cast<double>(n) / fs);
    EXPECT_NEAR(std::abs(x[k0]), amplitude * static_cast<double>(n) / 2.0, 1e-6);
}

TEST(FftTest, AgainstNaiveDft) {
    const std::size_t n = 64;
    unsigned long seed = 12345UL;

    std::vector<Complex> x(n);
    for (auto &v : x)
        v = Complex(SignalCoreLibTest::pseudoRandom(seed), SignalCoreLibTest::pseudoRandom(seed));

    std::vector<Complex> y = x;
    Fft::forward(y);

    std::vector<Complex> ref;
    SignalCoreLibTest::naiveDft(x, ref);

    ASSERT_EQ(y.size(), ref.size());
    for (std::size_t k = 0; k < n; ++k)
        EXPECT_NEAR(std::abs(y[k] - ref[k]), 0.0, 1e-9);
}

TEST(FftTest, ImpulseHasFlatSpectrum) {
    const std::size_t n = 128;
    std::vector<Complex> x(n, Complex(0.0, 0.0));
    x[0] = Complex(1.0, 0.0); // 单位冲激

    Fft::forward(x);

    const double target = std::abs(x[0]);
    for (std::size_t k = 0; k < n; ++k)
        EXPECT_NEAR(std::abs(x[k]), target, 1e-9);
}

TEST(FftTest, ParsevalEnergyConservation) {
    const std::size_t n = 256;
    unsigned long seed = 98765UL;

    std::vector<Complex> x(n);
    for (auto &v : x)
        v = Complex(SignalCoreLibTest::pseudoRandom(seed), SignalCoreLibTest::pseudoRandom(seed));

    double timeEnergy = 0.0;
    for (const auto &v : x)
        timeEnergy += std::norm(v);

    Fft::forward(x);

    double freqEnergy = 0.0;
    for (const auto &v : x)
        freqEnergy += std::norm(v);

    // Parseval：Σ|x|² = (1/N) Σ|X|²（未归一化的正向 FFT）。
    EXPECT_NEAR(timeEnergy, freqEnergy / static_cast<double>(n), 1e-9);
}
