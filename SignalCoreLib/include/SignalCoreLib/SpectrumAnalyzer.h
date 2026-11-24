#pragma once

#include "SignalCoreLib/Fft.h"
#include "SignalCoreLib/IWindow.h"
#include "SignalCoreLib/SignalData.h"

#include <vector>
#include <complex>
#include <memory>
#include <cstddef>

namespace SignalCoreLib {

// 频谱分析结果。
struct Spectrum {
    std::vector<double> frequencies; // 频率轴（Hz），禁止用点序号
    std::vector<double> magnitudes;  // 幅值（单边谱）
};

// 频谱分析器：自写 FFT 与谱分析。
// 提供幅值谱（线性谱）与自功率谱两种输出，横轴为真实频率（Hz）。
// 无状态（加窗参数由调用方传入），可在工作线程中安全调用。
class SpectrumAnalyzer {
public:
    // 计算幅值谱（线性单边谱）。
    // window 可为 nullptr（等效矩形窗）；否则先加窗再 FFT。
    // 返回单边谱：仅保留 0..N/2 的幅度，DC 与 Nyquist 分量不再额外乘 2 之外的校正
    // （幅值谱采用 2/N 归一化恢复正弦真实幅值，窗函数校正因子额外叠加）。
    static Spectrum amplitudeSpectrum(const SignalData& data, const IWindow* window);

    // 计算自功率谱（单边），由幅值谱模平方换算得到（LMS 3.2 节）。
    static Spectrum powerSpectrum(const SignalData& data, const IWindow* window);

    // 计算频率轴（Hz）：f_k = k * fs / N，k = 0..N/2。
    static std::vector<double> frequencyAxis(double fs, std::size_t fftSize);

    // 将点数规整为不小于 N 的最近 2 的整数次幂（零填充策略）。
    static std::size_t nextPowerOfTwo(std::size_t n);

private:
    static Spectrum computeSpectrum(const SignalData& data, const IWindow* window, bool power);
};

} // namespace SignalCoreLib
