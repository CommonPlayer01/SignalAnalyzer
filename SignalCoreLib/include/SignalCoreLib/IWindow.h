#pragma once

#include <vector>
#include <cstddef>

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

} // namespace SignalCoreLib
