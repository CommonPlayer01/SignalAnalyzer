#pragma once

#include <vector>
#include <cmath>

namespace SignalCoreLib {

// 有效值（RMS）计算。
// 纯函数，无状态，可在工作线程中安全调用。
class RmsCalculator {
public:
    // 计算序列的有效值（均方根）。空序列返回 0。
    static double compute(const std::vector<double>& samples) {
        if (samples.empty()) {
            return 0.0;
        }
        double sumSq = 0.0;
        for (double x : samples) {
            sumSq += x * x;
        }
        return std::sqrt(sumSq / static_cast<double>(samples.size()));
    }
};

} // namespace SignalCoreLib
