#pragma once

#include "SignalCoreLib/IWindow.h"

namespace SignalCoreLib {

// 矩形窗：等效于不加窗，幅值校正因子为 1。
class RectWindow : public IWindow {
public:
    const char* name() const override { return "rect"; }

    // 矩形窗不改变数据（系数恒为 1）。
    void apply(std::vector<double>& samples) const override {
        (void)samples; // 矩形窗为恒等变换
    }

    double correctionFactor() const override { return 1.0; }
};

} // namespace SignalCoreLib
