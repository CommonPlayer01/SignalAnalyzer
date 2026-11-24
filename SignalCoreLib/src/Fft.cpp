#include "SignalCoreLib/Fft.h"

#include <cmath>
#include <utility>

namespace SignalCoreLib {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

bool Fft::isPowerOfTwo(std::size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

void Fft::transform(std::vector<Complex>& a, bool inverse) {
    const std::size_t n = a.size();
    if (n <= 1) {
        return;
    }

    // 位反转置换（迭代实现，避免递归栈开销）。
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(a[i], a[j]);
        }
    }

    // 蝶形运算。
    for (std::size_t len = 2; len <= n; len <<= 1) {
        const double angle = (inverse ? 1.0 : -1.0) * 2.0 * kPi / static_cast<double>(len);
        const Complex wlen(std::cos(angle), std::sin(angle));
        for (std::size_t i = 0; i < n; i += len) {
            Complex w(1.0, 0.0);
            for (std::size_t j = 0; j < len / 2; ++j) {
                const Complex u = a[i + j];
                const Complex v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }

    if (inverse) {
        for (auto& x : a) {
            x /= static_cast<double>(n);
        }
    }
}

void Fft::forward(std::vector<Complex>& a) {
    transform(a, false);
}

void Fft::inverse(std::vector<Complex>& a) {
    transform(a, true);
}

} // namespace SignalCoreLib
