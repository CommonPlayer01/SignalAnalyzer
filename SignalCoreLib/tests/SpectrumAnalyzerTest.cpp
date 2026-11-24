// ============================================================================
// 频谱分析单元测试（GoogleTest）
// ----------------------------------------------------------------------------
// 对应《测试用例表》中谱分析相关的用例：
//   用例6  自功率谱与幅值谱模平方的关系、单边谱转换（LMS 3.2 节）
//
// 运行方式：
//   全部：   ctest --test-dir build -C Release
//   本套件： build\Release\SignalCoreLibTests.exe --gtest_filter=SpectrumAnalyzerTest.*
// ============================================================================

#include "SignalCoreLib/SignalData.h"
#include "SignalCoreLib/SpectrumAnalyzer.h"
#include "TestCommon.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

using namespace SignalCoreLib;
using namespace SignalCoreLibTest;

// ---------------------------------------------------------------------------
// 用例 6：自功率谱 = 幅值谱模平方（单边谱，LMS 3.2 节）。
// ---------------------------------------------------------------------------
TEST(SpectrumAnalyzerTest, PowerSpectrumEqualsSquaredAmplitudeSpectrum) {
    const double fs = 256.0;
    const std::size_t n = 64;
    unsigned long seed = 2024;

    std::vector<double> x(n);
    for (std::size_t i = 0; i < n; ++i) {
        x[i] = pseudoRandom(seed);
    }
    const SignalData data(fs, std::move(x));

    const Spectrum amp = SpectrumAnalyzer::amplitudeSpectrum(data, nullptr);
    const Spectrum pw = SpectrumAnalyzer::powerSpectrum(data, nullptr);

    // 二者同为单边谱，长度一致。
    EXPECT_EQ(pw.magnitudes.size(), amp.magnitudes.size());
    EXPECT_EQ(pw.magnitudes.size(), n / 2 + 1);

    // 自功率谱逐点等于幅值谱模平方。
    for (std::size_t k = 0; k < pw.magnitudes.size(); ++k) {
        const double expected = amp.magnitudes[k] * amp.magnitudes[k];
        EXPECT_NEAR(pw.magnitudes[k], expected, 1e-9 * (1.0 + expected));
    }
}
