// ============================================================================
// SignalCoreLib 调用冒烟测试（控制台程序，无 Qt / 无第三方测试框架依赖）
// ----------------------------------------------------------------------------
// 目的：验证静态库可被正常链接，且对外接口能被成功调用并返回正确结果。
// 覆盖：Fft、WindowFactory、SpectrumAnalyzer、RmsCalculator、SignalReader。
// 运行：ctest --test-dir <build> -C Release  或直接执行 SignalCoreLibSmokeTest.exe
// ============================================================================

#include "SignalCoreLib/Fft.h"
#include "SignalCoreLib/HannWindow.h"
#include "SignalCoreLib/RmsCalculator.h"
#include "SignalCoreLib/SignalData.h"
#include "SignalCoreLib/SignalReader.h"
#include "SignalCoreLib/SpectrumAnalyzer.h"
#include "SignalCoreLib/WindowFactory.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;

int g_failures = 0;

void check(const char *name, bool pass, const char *detail)
{
    std::printf("[%s] %-42s %s\n", pass ? "PASS" : "FAIL", name, detail);
    if (!pass)
        ++g_failures;
}

std::vector<double> makeSine(double fs, std::size_t n, double f0, double amplitude)
{
    std::vector<double> samples(n);
    for (std::size_t i = 0; i < n; ++i) {
        samples[i] = amplitude
                     * std::sin(2.0 * kPi * f0 * static_cast<double>(i) / fs);
    }
    return samples;
}

std::size_t findPeak(const SignalCoreLib::Spectrum &spectrum)
{
    std::size_t peak = 0;
    for (std::size_t k = 1; k < spectrum.magnitudes.size(); ++k) {
        if (spectrum.magnitudes[k] > spectrum.magnitudes[peak])
            peak = k;
    }
    return peak;
}

} // namespace

int main()
{
    using namespace SignalCoreLib;

    // 1) FFT 正反变换往返。
    {
        std::vector<Fft::Complex> buffer(64);
        for (std::size_t i = 0; i < buffer.size(); ++i) {
            const double t = static_cast<double>(i);
            buffer[i] = Fft::Complex(std::sin(0.1 * t), 0.3 * std::cos(0.2 * t));
        }
        const std::vector<Fft::Complex> original = buffer;

        Fft::forward(buffer);
        Fft::inverse(buffer);

        double maxError = 0.0;
        for (std::size_t i = 0; i < buffer.size(); ++i)
            maxError = std::max(maxError, std::abs(buffer[i] - original[i]));

        char detail[128] = {};
        std::snprintf(detail, sizeof(detail), "maxErr=%.2e", maxError);
        check("Fft::forward + Fft::inverse roundtrip", maxError < 1e-9, detail);
    }

    // 2) 窗函数工厂。
    {
        const auto rect = WindowFactory::create("rect");
        const auto hann = WindowFactory::create("hann");
        const auto names = WindowFactory::names();

        char detail[128] = {};
        std::snprintf(detail, sizeof(detail), "registered=%d", static_cast<int>(names.size()));
        check("WindowFactory::create(\"rect\"/\"hann\")",
              rect != nullptr && hann != nullptr && !names.empty(), detail);
    }

    // 3) 幅值谱：谱峰频率与幅值恢复。
    {
        const double fs = 256.0;
        const std::size_t n = 64;
        const double f0 = 16.0;
        const double amplitude = 2.0;

        const SignalData data(fs, makeSine(fs, n, f0, amplitude));
        const Spectrum spectrum = SpectrumAnalyzer::amplitudeSpectrum(data, nullptr);
        const std::size_t peak = findPeak(spectrum);

        char detail[160] = {};
        std::snprintf(detail, sizeof(detail), "peak=%.2f Hz, amp=%.4f",
                      spectrum.frequencies[peak], spectrum.magnitudes[peak]);
        check("SpectrumAnalyzer::amplitudeSpectrum",
              spectrum.magnitudes.size() == n / 2 + 1
                  && std::fabs(spectrum.frequencies[peak] - f0) < 1e-9
                  && std::fabs(spectrum.magnitudes[peak] - amplitude) < 1e-6,
              detail);
    }

    // 4) 汉宁窗幅值校正。
    {
        const double fs = 256.0;
        const std::size_t n = 64;
        const double amplitude = 5.0;

        const SignalData data(fs, makeSine(fs, n, 16.0, amplitude));
        const HannWindow hann;
        const Spectrum spectrum = SpectrumAnalyzer::amplitudeSpectrum(data, &hann);
        const std::size_t peak = findPeak(spectrum);

        char detail[160] = {};
        std::snprintf(detail, sizeof(detail), "peakAmp=%.4f, expected=%.1f",
                      spectrum.magnitudes[peak], amplitude);
        check("HannWindow correction factor",
              std::fabs(spectrum.magnitudes[peak] - amplitude) < 0.05 * amplitude,
              detail);
    }

    // 5) 有效值。
    {
        const double rms = RmsCalculator::compute(makeSine(2560.0, 4096, 50.0, 10.0));
        char detail[128] = {};
        std::snprintf(detail, sizeof(detail), "rms=%.6f, expected=%.6f",
                      rms, 10.0 / std::sqrt(2.0));
        check("RmsCalculator::compute", std::fabs(rms - 10.0 / std::sqrt(2.0)) < 1e-6, detail);
    }

    // 6) 文本解析（坏行跳过计数）。
    {
        const std::string text = "fs=2560,N=4096\n1.0\n2.0\nbad-line\n4.0\n";
        const ReadResult result = SignalReader::parse(text);

        char detail[160] = {};
        std::snprintf(detail, sizeof(detail), "ok=%d, samples=%d, skipped=%d, fs=%.0f",
                      result.ok ? 1 : 0,
                      static_cast<int>(result.data.size()),
                      static_cast<int>(result.skippedLines),
                      result.data.sampleRate());
        check("SignalReader::parse",
              result.ok && result.data.size() == 3 && result.skippedLines == 1
                  && std::fabs(result.data.sampleRate() - 2560.0) < 1e-9,
              detail);
    }

    std::printf("\nSignalCoreLib smoke test: %s (%d failure(s))\n",
                g_failures == 0 ? "OK" : "FAILED", g_failures);
    return g_failures == 0 ? 0 : 1;
}
