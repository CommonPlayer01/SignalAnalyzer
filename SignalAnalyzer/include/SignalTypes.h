#pragma once

// ============================================================================
// 跨层共享的纯数据类型（无 Qt 对象依赖，仅值语义结构体 + 枚举）。
// 由 UI 层、ComputeTask、ScanTask、SignalWorker 共同使用。
// 单独成头文件，避免各任务对象互相包含 SignalWorker.h。
// ============================================================================

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

#include <cstddef>
#include <memory>
#include <vector>

// 信号类型（与 comboSignal 顺序一致）。
enum class SignalType {
    TimeDomain = 0,        // 时域信号
    AmplitudeSpectrum = 1, // 幅值谱（线性谱）
    PowerSpectrum = 2      // 自功率谱
};

// 一次计算请求：由 UI 层（MainWindow）发起，投递给计算线程。
struct ComputeRequest {
    QString filePath;                 // 待解析的 txt 文件完整路径（内置信号时可为空）
    SignalType signalType = SignalType::TimeDomain;
    QString windowName;               // "rect" / "hann"

    // 内置正弦参数：useBuiltin=true 时按 f/A/fs/N 直接生成，不读文件。
    bool useBuiltin = false;
    double builtinFreq = 0.0;         // 频率 (Hz)
    double builtinAmp = 0.0;          // 幅值
    double builtinFs = 0.0;           // 采样率 (Hz)
    std::size_t builtinN = 0;         // 点数

    QString label;                    // 图例名（由 UI 生成并随请求透传）
};

// 一次计算结果：由 ComputeTask 计算线程产出，回传 UI 线程绘图。
struct ComputeResult {
    quint64 generation = 0;           // 代际号（用于丢弃过期结果）
    bool ok = false;
    QString error;                    // ok=false 时的失败原因
    int skippedLines = 0;             // 跳过的坏行数

    // 全量数据：以共享指针跨线程传递，Queued 连接只拷贝指针，避免深拷贝大数据。
    // 时域横轴为秒，频谱为 Hz；x/y 长度一致。
    std::shared_ptr<const std::vector<double>> x;
    std::shared_ptr<const std::vector<double>> y;

    // 显示数据：由工作线程（ComputeTask）预先降采样，
    // UI 线程直接 setData，不再搬运全量数据、也不在 UI 线程做降采样。
    QVector<double> displayX;
    QVector<double> displayY;

    double fs = 0.0;                  // 采样率
    std::size_t pointCount = 0;       // 有效点数 N
    std::size_t fftSize = 0;          // FFT 点数 M（零填充后，时域为 0）
    double rms = 0.0;
    double peakFreq = 0.0;
    double peakMag = 0.0;
    SignalType signalType = SignalType::TimeDomain;

    bool builtin = false;             // 是否来自内置正弦（自验）
    QString label;                    // 图例名（随请求透传）
};

// 批量导入扫描结果（轻量校验：不含坏行统计，坏行留到实际解析时判定）。
struct ScanResult {
    quint64 generation = 0;
    QStringList validPaths;           // 有效文件（按导入顺序）
    int skippedMissingFs = 0;         // 缺失/非法 fs 的文件数
    int skippedEmpty = 0;             // 空文件 / 无法打开等无效文件数
};

Q_DECLARE_METATYPE(ComputeRequest)
Q_DECLARE_METATYPE(ComputeResult)
Q_DECLARE_METATYPE(ScanResult)