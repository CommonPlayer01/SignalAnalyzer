#pragma once

#include <vector>
#include <complex>
#include <cstddef>

namespace SignalCoreLib {

// 自写基 2 FFT（时域抽取，迭代实现）。
// 要求 N 为 2 的整数次幂；非 2 的幂次输入由调用方（SpectrumAnalyzer）
// 预先做零填充规整。禁止使用任何第三方 FFT 库。
class Fft {
public:
    using Complex = std::complex<double>;

    // 原地计算 FFT（正向）。n 必须为 2 的整数次幂，否则行为未定义。
    static void forward(std::vector<Complex>& a);

    // 原地计算逆 FFT。与 forward 成对，满足 roundtrip 恒等。
    static void inverse(std::vector<Complex>& a);

private:
    static void transform(std::vector<Complex>& a, bool inverse);
    static bool isPowerOfTwo(std::size_t n);
};

} // namespace SignalCoreLib
