#include "ScanTask.h"

#include <QByteArray>
#include <QString>

#include <string>

#include "SignalCoreLib/SignalReader.h"

namespace {

// 判断错误类别（用于导入统计，避免为分类改动 SignalCore）。
enum class ReadErrorKind { MissingFs, Empty, Other };

ReadErrorKind classify(const std::string &error)
{
    if (error.find("fs") != std::string::npos)
        return ReadErrorKind::MissingFs;
    if (error.find("no valid data rows") != std::string::npos
        || error.find("empty file") != std::string::npos)
        return ReadErrorKind::Empty;
    return ReadErrorKind::Other;
}

} // namespace

ScanTask::ScanTask(std::shared_ptr<std::atomic<std::uint64_t>> generation)
    : m_generation(std::move(generation))
{}

// 扫描过期判定：只看扫描代际号，计算任务不会让扫描失效
// （旧版本共用代际号时，切换信号类型/窗函数会把正在进行的导入扫描静默丢弃）。
bool ScanTask::isStale(quint64 generation) const
{
    return m_generation->load(std::memory_order_relaxed) != generation;
}

void ScanTask::onScanRequest(const QStringList &paths, quint64 generation)
{
    ScanResult result;
    result.generation = generation;

    for (const QString &path : paths) {
        // 仅当出现了更新的扫描请求（即用户又导入了一次）时才放弃本次扫描；
        // 切换信号类型/窗函数等计算请求不再打断导入。
        if (isStale(generation))
            return;

        const QByteArray localPath = path.toLocal8Bit();
        // 轻量校验：只判断文件是否可用（有 fs + 有数据行），
        // 不逐行解析数值、不检测坏行——坏行/有效性留到实际计算时判定。
        const SignalCoreLib::ProbeResult probe =
            SignalCoreLib::SignalReader::probeFile(localPath.constData());

        if (probe.ok) {
            result.validPaths << path;
            continue;
        }

        switch (classify(probe.error)) {
        case ReadErrorKind::MissingFs: ++result.skippedMissingFs; break;
        case ReadErrorKind::Empty:     ++result.skippedEmpty;     break;
        case ReadErrorKind::Other:     ++result.skippedEmpty;     break;
        }
    }

    emit scanFinished(generation, result);
}
