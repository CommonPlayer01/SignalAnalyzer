#pragma once

#include <vector>
#include <cstddef>
#include <cmath>

namespace SignalCoreLib {

// 窗函数抽象接口。
// 新增窗函数只需新增一个实现类，并在 WindowFactory 中登记一处，
// 无需修改任何处理框架代码（可扩展性验收定义）。
class IWindow {
public:
    virtual ~IWindow() = default;

    // 返回窗函数的唯一名称（与 WindowFactory 注册名一致）。
    virtual const char* name() const = 0;

    // 对输入序列原地加窗（输出长度与输入一致）。
    virtual void apply(std::vector<double>& samples) const = 0;

    // 幅值校正因子：加窗后谱峰值乘以该因子可恢复真实幅值。
    // 矩形窗为 1（等效不加窗），汉宁窗为 2。
    virtual double correctionFactor() const = 0;
};

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