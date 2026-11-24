#pragma once

#include <QObject>

#include <atomic>
#include <cstdint>
#include <memory>

#include "SignalTypes.h"

// 计算任务执行对象：在后台线程中完成文件解析与信号计算。
// 只持有「计算代际号」——仅被更新的计算请求取消，扫描任务不会让它失效。
// 通过 moveToThread 常驻在 SignalWorker 提供的线程中，不对外暴露。
class ComputeTask : public QObject
{
    Q_OBJECT

public:
    explicit ComputeTask(std::shared_ptr<std::atomic<std::uint64_t>> generation);

signals:
    // 携带本次请求的代际号，由 SignalWorker 门面做最终过滤。
    void computeFinished(quint64 generation, const ComputeResult &result);

public slots:
    void onComputeRequest(const ComputeRequest &request, quint64 generation);

private:
    bool isStale(quint64 generation) const;

    std::shared_ptr<std::atomic<std::uint64_t>> m_generation;
};
