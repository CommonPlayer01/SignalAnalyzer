#pragma once

#include <QObject>
#include <QThread>

#include <atomic>
#include <cstdint>
#include <memory>

#include "SignalTypes.h"

class ComputeTask;
class ScanTask;

// App 内调用 SignalCore 的唯一入口，后台计算协调门面。
// 职责：
//  - 持有常驻工作线程，并把 ComputeTask / ScanTask 安置其中；
//  - 持有「计算」「扫描」两个独立代际号，并做结果回传过滤（见设计文档 4.3 / 4.4）。
//
// 代际号拆分说明：
//  - 计算与扫描是语义不同的两类任务：计算「最新优先、旧结果可丢弃」，
//    扫描「最新一次导入胜出、旧扫描结果不得混入当前文件夹」；
//  - 若二者共用一个计数器，切换信号类型/窗函数就会把正在进行的导入扫描
//    判定为过期并静默丢弃，反之导入也会掐掉正在进行的计算；
//  - 因此这里为两类任务各维护一个独立代际号，互不取消，只取消各自同类任务。
class SignalWorker : public QObject
{
    Q_OBJECT

public:
    explicit SignalWorker(QObject *parent = nullptr);
    ~SignalWorker() override;

    // UI 线程调用：发起一次计算请求（内部自增「计算」代际号）。
    void requestCompute(const ComputeRequest &request);

    // UI 线程调用：批量校验文件（内部自增「扫描」代际号）。
    void requestScan(const QStringList &paths);

signals:
    // 仅当结果代际号等于对应类别的最新代际号时才发出（旧结果被丢弃）。
    void computeFinished(const ComputeResult &result);
    void scanFinished(const ScanResult &result);

private slots:
    void onImplComputeFinished(quint64 generation, const ComputeResult &result);
    void onImplScanFinished(quint64 generation, const ScanResult &result);

private:
    std::shared_ptr<std::atomic<std::uint64_t>> m_computeGeneration; // 计算代际号
    std::shared_ptr<std::atomic<std::uint64_t>> m_scanGeneration;    // 扫描代际号

    QThread m_workerThread;          // 常驻工作线程（对象本身留在 UI 线程）
    ComputeTask *m_computeTask = nullptr; // 无 parent，moveToThread 到工作线程
    ScanTask    *m_scanTask    = nullptr;
};
