#pragma once

#include "SignalCoreLib/IWindow.h"

#include <cmath>

namespace SignalCoreLib {

// 汉宁窗：w[n] = 0.5 * (1 - cos(2πn/(N-1)))。
// 幅值校正因子为 2，保证整周期采样正弦信号的谱峰值恢复真实幅值。
class HannWindow : public IWindow {
public:
    const char* name() const override { return "hann"; }

    void apply(std::vector<double>& samples) const override {
        const std::size_t n = samples.size();
        if (n <= 1) {
            return;
        }
        const double denom = static_cast<double>(n - 1);
        for (std::size_t i = 0; i < n; ++i) {
            const double w = 0.5 * (1.0 - std::cos(2.0 * kPi * static_cast<double>(i) / denom));
            samples[i] *= w;
        }
    }

    double correctionFactor() const override { return 2.0; }

private:
    static constexpr double kPi = 3.14159265358979323846;
};

} // namespace SignalCoreLib
