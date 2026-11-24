#pragma once

#include <vector>
#include <string>
#include <cstddef>

namespace SignalCoreLib {

// 信号数据与元数据容器。
// 持有采样率 fs、有效点数 N 与幅值序列，对外提供只读访问。
// 本类不依赖任何 Qt 模块，可在工作线程中安全使用。
class SignalData {
public:
    SignalData() = default;
    SignalData(double sampleRate, std::vector<double> samples)
        : fs_(sampleRate), samples_(std::move(samples)) {}

    // 采样率（Hz）。缺失 fs 时由 SignalReader 判为无效，不构造本对象。
    double sampleRate() const { return fs_; }

    // 有效点数。
    std::size_t size() const { return samples_.size(); }

    // 幅值序列的只读访问。
    const std::vector<double>& samples() const { return samples_; }

    // 单点访问（带边界检查版本由调用方自行保证索引合法）。
    double at(std::size_t i) const { return samples_[i]; }

private:
    double fs_ = 0.0;
    std::vector<double> samples_;
};

} // namespace SignalCoreLib
