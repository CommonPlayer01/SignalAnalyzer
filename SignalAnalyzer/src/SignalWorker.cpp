#include "SignalWorker.h"

#include <QMetaObject>
#include <QMetaType>
#include <QThread>

#include "ComputeTask.h"
#include "ScanTask.h"

SignalWorker::SignalWorker(QObject *parent)
    : QObject(parent)
    , m_computeGeneration(std::make_shared<std::atomic<std::uint64_t>>(0))
    , m_scanGeneration(std::make_shared<std::atomic<std::uint64_t>>(0))
{
    // 跨线程 Queued 连接/调用需要这些值类型在 Qt 元对象系统中注册。
    qRegisterMetaType<ComputeRequest>("ComputeRequest");
    qRegisterMetaType<ComputeResult>("ComputeResult");
    qRegisterMetaType<ScanResult>("ScanResult");

    // 创建任务对象：无 parent（moveToThread 要求），分别持有各自的代际号。
    m_computeTask = new ComputeTask(m_computeGeneration);
    m_scanTask    = new ScanTask(m_scanGeneration);

    // 两个任务都放进同一个常驻工作线程（方案 A：串行执行，行为与原实现一致）。
    m_computeTask->moveToThread(&m_workerThread);
    m_scanTask->moveToThread(&m_workerThread);

    // 线程退出时在工作线程内销毁任务对象（标准 worker-object 生命周期写法）。
    connect(&m_workerThread, &QThread::finished, m_computeTask, &QObject::deleteLater);
    connect(&m_workerThread, &QThread::finished, m_scanTask,    &QObject::deleteLater);

    // 任务结果先回到门面做代际号过滤，再对外转发。
    connect(m_computeTask, &ComputeTask::computeFinished,
            this, &SignalWorker::onImplComputeFinished);
    connect(m_scanTask, &ScanTask::scanFinished,
            this, &SignalWorker::onImplScanFinished);

    m_workerThread.start();
}

SignalWorker::~SignalWorker()
{
    // 停止事件循环并等待线程结束；任务对象由 finished->deleteLater 销毁。
    m_workerThread.quit();
    m_workerThread.wait();
}

void SignalWorker::requestCompute(const ComputeRequest &request)
{
    // 只递增计算代际号：不影响正在进行的文件扫描。
    const quint64 generation = ++(*m_computeGeneration);
    QMetaObject::invokeMethod(m_computeTask, "onComputeRequest", Qt::QueuedConnection,
                              Q_ARG(ComputeRequest, request),
                              Q_ARG(quint64, generation));
}

void SignalWorker::requestScan(const QStringList &paths)
{
    // 只递增扫描代际号：不影响正在进行的计算。
    const quint64 generation = ++(*m_scanGeneration);
    QMetaObject::invokeMethod(m_scanTask, "onScanRequest", Qt::QueuedConnection,
                              Q_ARG(QStringList, paths),
                              Q_ARG(quint64, generation));
}

void SignalWorker::onImplComputeFinished(quint64 generation, const ComputeResult &result)
{
    // 兜底正确性：仅采纳最新「计算」代际的结果。
    if (generation == m_computeGeneration->load(std::memory_order_relaxed))
        emit computeFinished(result);
}

void SignalWorker::onImplScanFinished(quint64 generation, const ScanResult &result)
{
    // 兜底正确性：仅采纳最新「扫描」代际的结果（旧导入的文件不会混入当前列表）。
    if (generation == m_scanGeneration->load(std::memory_order_relaxed))
        emit scanFinished(result);
}