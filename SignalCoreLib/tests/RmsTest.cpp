// ============================================================================
// 有效值（RMS）单元测试（GoogleTest）
// ----------------------------------------------------------------------------
// 对应《测试用例表》中有效值相关的 2 个用例：
//   用例7  正弦信号 A=10 → 7.07（A/√2）
//   用例8  采样序列 {0,3,4,3} → √8.5 ≈ 2.92
//
// 运行方式：
//   全部：   ctest --test-dir build -C Release
//   本套件： build\Release\SignalCoreLibTests.exe --gtest_filter=RmsTest.*
// ============================================================================

#include "SignalCoreLib/RmsCalculator.h"
#include "TestCommon.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <vector>

using namespace SignalCoreLib;
using namespace SignalCoreLibTest;

// ---------------------------------------------------------------------------
// 用例 7：正弦信号 A=10 的有效值 = A/√2 ≈ 7.07。
// ---------------------------------------------------------------------------
TEST(RmsTest, SineAmplitude10) {
    const double a = 10.0;
    const double fs = 2560.0;
    const double f0 = 50.0; // N*f0/fs = 80，整周期
    const std::size_t n = 4096;

    const double rms = RmsCalculator::compute(makeIntegerPeriodSine(fs, n, f0, a));
    EXPECT_NEAR(rms, a / std::sqrt(2.0), 1e-6);
}

// ---------------------------------------------------------------------------
// 用例 8：采样序列 {0,3,4,3} 的有效值 = √(34/4) = √8.5 ≈ 2.92。
// ---------------------------------------------------------------------------
TEST(RmsTest, SampleSequence0343) {
    const std::vector<double> samples = {0.0, 3.0, 4.0, 3.0};
    EXPECT_NEAR(RmsCalculator::compute(samples), std::sqrt(8.5), 1e-12);
}
