#include "ComputeTask.h"

#include <QByteArray>
#include <QString>

#include <QVector>

#include <cmath>
#include <cstddef>
#include <memory>
#include <vector>

#include "SignalCoreLib/RmsCalculator.h"
#include "SignalCoreLib/SignalReader.h"
#include "SignalCoreLib/SpectrumAnalyzer.h"
#include "SignalCoreLib/WindowFactory.h"

namespace {

constexpr double kPi = 3.14159265358979323846;    // 圆周率（内置正弦生成用）
constexpr std::size_t kStaleCheckStride = 65536; // 长循环中每 N 次检查一次代际号
constexpr std::size_t kMaxDisplayPoints = 50000; // 目标最大显示点数

// 极值抽取降采样：每个分箱保留 y 的最小/最大两个点（按 x 升序），
// 保持波形包络；输出规模约 2*kMaxDisplayPoints。仅供本文件内部使用。
void downsampleForDisplay(const std::vector<double> &x,
                          const std::vector<double> &y,
                          QVector<double> &outX,
                          QVector<double> &outY)
{
    const std::size_t n = x.size();

    outX.clear();
    outY.clear();
    if (n == 0)
        return;

    if (n <= kMaxDisplayPoints) {
        outX.resize(static_cast<int>(n));
        outY.resize(static_cast<int>(n));
        for (std::size_t i = 0; i < n; ++i) {
            outX[static_cast<int>(i)] = x[i];
            outY[static_cast<int>(i)] = y[i];
        }
        return;
    }

    const std::size_t bucket = (n + kMaxDisplayPoints - 1) / kMaxDisplayPoints;
    outX.reserve(static_cast<int>(n / bucket * 2 + 4));
    outY.reserve(static_cast<int>(n / bucket * 2 + 4));

    for (std::size_t start = 0; start < n; start += bucket) {
        const std::size_t end = start + bucket < n ? start + bucket : n;

        std::size_t imin = start;
        std::size_t imax = start;
        for (std::size_t i = start + 1; i < end; ++i) {
            if (y[i] < y[imin]) imin = i;
            if (y[i] > y[imax]) imax = i;
        }

        const std::size_t first = imin < imax ? imin : imax;
        const std::size_t second = imin < imax ? imax : imin;
        outX.append(x[first]);
        outY.append(y[first]);
        outX.append(x[second]);
        outY.append(y[second]);
    }
}

} // namespace

ComputeTask::ComputeTask(std::shared_ptr<std::atomic<std::uint64_t>> generation)
    : m_generation(std::move(generation))
{}

// 计算过期判定：只看计算代际号，扫描任务不会让计算失效。
bool ComputeTask::isStale(quint64 generation) const
{
    return m_generation->load(std::memory_order_relaxed) != generation;
}

// 处理单个计算请求：工作线程内完成信号获取、频谱/时域分析、降采样，最后发布结果。
void ComputeTask::onComputeRequest(const ComputeRequest &request, quint64 generation)
{
    // 代际号已过期（期间已有更新的请求发出），本次旧请求直接丢弃。
    if (isStale(generation))
        return;

    // 初始化结果对象：带回代际号与请求元信息，后续再逐步填充计算产物。
    ComputeResult result;
    result.generation = generation;
    result.signalType = request.signalType;
    result.builtin = request.useBuiltin;
    result.label = request.label;

    // 取得 SignalData：内置正弦直接生成，否则从文件读取解析。
    SignalCoreLib::SignalData data;
    if (request.useBuiltin) {
        // 内置正弦：不读文件，直接生成 N 点 sin(2π f i / fs)。
        if (request.builtinFs <= 0.0 || request.builtinN == 0) {
            result.error = QStringLiteral("内置信号参数非法（fs 或 N 无效）");
            emit computeFinished(generation, result);
            return;
        }
        std::vector<double> samples(request.builtinN);
        const double w = 2.0 * kPi * request.builtinFreq / request.builtinFs;
        for (std::size_t i = 0; i < request.builtinN; ++i)
            samples[i] = request.builtinAmp * std::sin(w * static_cast<double>(i));
        data = SignalCoreLib::SignalData(request.builtinFs, std::move(samples));
    } else {
        // 文件读取与解析：SignalCore（SignalReader）。
        const QByteArray localPath = request.filePath.toLocal8Bit();
        SignalCoreLib::ReadResult read =
            SignalCoreLib::SignalReader::readFile(localPath.constData());
        if (!read.ok) {
            result.error = QString::fromStdString(read.error);
            emit computeFinished(generation, result);
            return;
        }
        result.skippedLines = read.skippedLines;
        data = std::move(read.data);
    }
    if (isStale(generation))
        return;

    // 数据就绪：标记计算成功，并记录采样率与数据点数等基础信息。
    result.ok = true;
    result.fs = data.sampleRate();
    result.pointCount = data.size();

    // 窗函数：矩形窗（等效不加窗）/ 汉宁窗（校正因子 2）。
    const std::unique_ptr<SignalCoreLib::IWindow> window =
        SignalCoreLib::WindowFactory::create(request.windowName.toStdString());

    // 先在本线程构造完整曲线数据（xs/ys），再统一发布为只读共享指针。
    std::vector<double> xs;
    std::vector<double> ys;

    if (request.signalType == SignalType::TimeDomain) {
        // 时域：横轴为时间（秒）。
        const std::size_t n = data.size();
        xs.resize(n);
        ys.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            xs[i] = static_cast<double>(i) / data.sampleRate();
            ys[i] = data.at(i);
            if ((i & (kStaleCheckStride - 1)) == 0 && isStale(generation))
                return; // 协作式取消：旧任务尽早退出
        }
        result.fftSize = 0;
    } else {
        // 频谱：加窗仅作用于频谱计算。
        SignalCoreLib::Spectrum spectrum =
            (request.signalType == SignalType::AmplitudeSpectrum)
                ? SignalCoreLib::SpectrumAnalyzer::amplitudeSpectrum(data, window.get())
                : SignalCoreLib::SpectrumAnalyzer::powerSpectrum(data, window.get());

        result.fftSize = SignalCoreLib::SpectrumAnalyzer::nextPowerOfTwo(data.size());

        // 排除 DC 后查找谱峰。
        std::size_t peak = 1;
        for (std::size_t k = 1; k < spectrum.magnitudes.size(); ++k) {
            if (spectrum.magnitudes[k] > spectrum.magnitudes[peak])
                peak = k;
        }
        if (!spectrum.magnitudes.empty()) {
            result.peakFreq = spectrum.frequencies[peak];
            result.peakMag = spectrum.magnitudes[peak];
        }

        // 频谱结果直接接管（移动），避免再拷贝一份。
        xs = std::move(spectrum.frequencies);
        ys = std::move(spectrum.magnitudes);
    }

    // RMS 均方根：基于原始样本序列计算，不受加窗处理影响。
    result.rms = SignalCoreLib::RmsCalculator::compute(data.samples());

    if (isStale(generation))
        return; // 兜底：算完的旧结果也不上屏。

    // 全量数据发布为只读共享指针：跨线程只拷贝指针，不拷贝数据。
    result.x = std::make_shared<std::vector<double>>(std::move(xs));
    result.y = std::make_shared<std::vector<double>>(std::move(ys));

    // 显示数据在工作线程内完成降采样，UI 线程只做 setData/replot。
    downsampleForDisplay(*result.x, *result.y, result.displayX, result.displayY);

    emit computeFinished(generation, result);
}