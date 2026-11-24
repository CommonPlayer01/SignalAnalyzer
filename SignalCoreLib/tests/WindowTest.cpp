// ============================================================================
// 窗函数单元测试（GoogleTest）
// ----------------------------------------------------------------------------
// 对应《测试用例表》中窗函数相关的用例：
//   用例5  汉宁窗：加窗后谱峰乘幅值校正因子 2，恢复真实幅值（LMS 表 1.2）
//
// 运行方式：
//   全部：   ctest --test-dir build -C Release
//   本套件： build\Release\SignalCoreLibTests.exe --gtest_filter=WindowTest.*
// ============================================================================

#include "SignalCoreLib/HannWindow.h"
#include "SignalCoreLib/RectWindow.h"
#include "SignalCoreLib/SignalData.h"
#include "SignalCoreLib/SpectrumAnalyzer.h"
#include "SignalCoreLib/WindowFactory.h"
#include "TestCommon.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <string>

using namespace SignalCoreLib;
using namespace SignalCoreLibTest;

// ---------------------------------------------------------------------------
// 用例 5：汉宁窗加窗后，谱峰乘幅值校正因子 2 恢复真实幅值（LMS 表 1.2）。
// ---------------------------------------------------------------------------
TEST(WindowTest, HannCorrectionFactorRestoresAmplitude) {
    EXPECT_DOUBLE_EQ(HannWindow().correctionFactor(), 2.0);
    EXPECT_DOUBLE_EQ(RectWindow().correctionFactor(), 1.0);

    const double fs = 256.0;
    const std::size_t n = 64;
    const double f0 = 16.0;
    const double a = 5.0;

    const SignalData data(fs, makeIntegerPeriodSine(fs, n, f0, a));
    const HannWindow hann;
    const Spectrum spec = SpectrumAnalyzer::amplitudeSpectrum(data, &hann);

    // 加窗造成相干增益损失（约 0.49），分析器内部乘校正因子 2 后，
    // 谱峰应恢复到真实幅值附近（有限长 N 误差约 1.6%，取 5% 容差）。
    const std::size_t peak = findPeakBin(spec);
    EXPECT_NEAR(spec.frequencies[peak], f0, 1e-9);
    EXPECT_NEAR(spec.magnitudes[peak], a, 0.05 * a);
}

// ---------------------------------------------------------------------------
// 工厂门面冒烟测试：验证重构后内置窗仍可按名称创建、名称列表正确。
// ---------------------------------------------------------------------------
TEST(WindowTest, FactoryCreatesBuiltinWindowsByName) {
    const auto rect = WindowFactory::create("rect");
    const auto hann = WindowFactory::create("hann");
    ASSERT_NE(rect, nullptr);
    ASSERT_NE(hann, nullptr);
    EXPECT_STREQ(rect->name(), "rect");
    EXPECT_STREQ(hann->name(), "hann");

    // 未知名称返回 nullptr。
    EXPECT_EQ(WindowFactory::create("unknown"), nullptr);

    // 名称列表包含两个内置窗（不关心顺序）。
    const auto names = WindowFactory::names();
    EXPECT_EQ(names.size(), 2u);
    EXPECT_NE(std::find(names.begin(), names.end(), "rect"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "hann"), names.end());
}
