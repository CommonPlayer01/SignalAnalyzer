#pragma once

#include <QObject>
#include <QStringList>

#include <atomic>
#include <cstdint>
#include <memory>

#include "SignalTypes.h"

// 扫描任务执行对象：在后台线程中批量轻量校验 txt 文件，
// 统计有效文件与无效原因（缺失 fs / 空文件）。
// 注意：扫描只做轻量校验（可读 + fs + 有数据行），不逐行解析数值、不检测坏行；
// 坏行/数值有效性留到实际计算时由 SignalReader::parse 判定。
// 只持有「扫描代际号」——仅被更新的扫描请求（即再次导入）取消，计算请求不会打断它。
// 通过 moveToThread 常驻在 SignalWorker 提供的线程中，不对外暴露。
class ScanTask : public QObject
{
    Q_OBJECT

public:
    explicit ScanTask(std::shared_ptr<std::atomic<std::uint64_t>> generation);

signals:
    // 携带本次请求的代际号，由 SignalWorker 门面做最终过滤。
    void scanFinished(quint64 generation, const ScanResult &result);

public slots:
    void onScanRequest(const QStringList &paths, quint64 generation);

private:
    bool isStale(quint64 generation) const;

    std::shared_ptr<std::atomic<std::uint64_t>> m_generation;
};
