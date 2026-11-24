#include "SignalCoreLib/SpectrumAnalyzer.h"

#include <cmath>

namespace SignalCoreLib {

std::size_t SpectrumAnalyzer::nextPowerOfTwo(std::size_t n) {
    std::size_t p = 1;
    while (p < n) {
        p <<= 1;
    }
    return p;
}

std::vector<double> SpectrumAnalyzer::frequencyAxis(double fs, std::size_t fftSize) {
    std::vector<double> freqs;
    freqs.reserve(fftSize / 2 + 1);
    for (std::size_t k = 0; k <= fftSize / 2; ++k) {
        freqs.push_back(fs * static_cast<double>(k) / static_cast<double>(fftSize));
    }
    return freqs;
}

Spectrum SpectrumAnalyzer::computeSpectrum(const SignalData& data, const IWindow* window, bool power) {
    const std::size_t nRaw = data.size();
    const std::size_t nFft = nextPowerOfTwo(nRaw);

    // 准备 FFT 输入：拷贝并零填充至 2 的整数次幂。
    std::vector<Fft::Complex> buf(nFft, Fft::Complex(0.0, 0.0));
    for (std::size_t i = 0; i < nRaw; ++i) {
        buf[i] = Fft::Complex(data.at(i), 0.0);
    }

    // 加窗（仅作用于频谱计算）。
    double correction = 1.0;
    if (window != nullptr) {
        // 将实数序列转出、加窗、转回。
        std::vector<double> realPart(nRaw);
        for (std::size_t i = 0; i < nRaw; ++i) {
            realPart[i] = buf[i].real();
        }
        window->apply(realPart);
        for (std::size_t i = 0; i < nRaw; ++i) {
            buf[i] = Fft::Complex(realPart[i], 0.0);
        }
        correction = window->correctionFactor();
    }

    Fft::forward(buf);

    // 单边谱：k = 0..nFft/2。
    Spectrum spec;
    spec.frequencies = frequencyAxis(data.sampleRate(), nFft);

    const std::size_t half = nFft / 2;
    spec.magnitudes.reserve(half + 1);

    // 幅值谱归一化：2/N 恢复正弦幅值（DC 与 Nyquist 乘 1，其余乘 2），
    // 再叠加窗函数校正因子。
    for (std::size_t k = 0; k <= half; ++k) {
        double mag = std::abs(buf[k]);
        double norm;
        if (k == 0 || k == half) {
            norm = mag / static_cast<double>(nFft);
        } else {
            norm = 2.0 * mag / static_cast<double>(nFft);
        }
        norm *= correction;

        if (power) {
            // 自功率谱 = 幅值谱模平方（LMS 3.2 节）。
            spec.magnitudes.push_back(norm * norm);
        } else {
            spec.magnitudes.push_back(norm);
        }
    }

    return spec;
}

Spectrum SpectrumAnalyzer::amplitudeSpectrum(const SignalData& data, const IWindow* window) {
    return computeSpectrum(data, window, false);
}

Spectrum SpectrumAnalyzer::powerSpectrum(const SignalData& data, const IWindow* window) {
    return computeSpectrum(data, window, true);
}

} // namespace SignalCoreLib
