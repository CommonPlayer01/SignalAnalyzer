#include "PlotController.h"

#include <QColor>
#include <QPen>
#include <QPixmap>
#include <QVector>
#include <Qt>

#include <iterator>

#include "qcustomplot.h"

namespace {

// 叠加曲线配色（8 色循环：蓝/橙/绿/红/紫/青/褐/粉）。
// 色相大致按 360°/8 等分选取，彼此区分度较高；可继续追加更多条目。
const QColor kPalette[] = {
    QColor(0x1E, 0x78, 0xC8),
    QColor(0xF0, 0x8A, 0x24),
    QColor(0x2F, 0xA8, 0x4F),
    QColor(0xD6, 0x45, 0x45),
    QColor(0x94, 0x66, 0xD6),
    QColor(0x16, 0xA0, 0xA8),
    QColor(0xB0, 0x6A, 0x4B),
    QColor(0xD4, 0x57, 0xA8),
};
// 由数组实际长度自动推出取色上限：新增颜色时无需再手动同步 kPaletteSize。
constexpr int kPaletteSize = static_cast<int>(std::size(kPalette));

QString graphNameFor(SignalType type)
{
    switch (type) {
    case SignalType::AmplitudeSpectrum: return QStringLiteral("幅值谱（线性谱）");
    case SignalType::PowerSpectrum:     return QStringLiteral("自功率谱");
    case SignalType::TimeDomain:
    default:                            return QStringLiteral("时域信号");
    }
}

QString xLabelFor(SignalType type)
{
    return type == SignalType::TimeDomain ? QStringLiteral("时间 (s)")
                                          : QStringLiteral("频率 (Hz)");
}

QString yLabelFor(SignalType type)
{
    switch (type) {
    case SignalType::PowerSpectrum: return QStringLiteral("功率");
    case SignalType::AmplitudeSpectrum:
    case SignalType::TimeDomain:
    default:                        return QStringLiteral("幅值");
    }
}

} // namespace

PlotController::PlotController(QCustomPlot *plot, QObject *parent)
    : QObject(parent)
    , m_plot(plot)
{}

QCPGraph *PlotController::acquireGraph()
{
    if (m_plot == nullptr)
        return nullptr;

    QCPGraph *graph = (m_curveCount < m_plot->graphCount())
                          ? m_plot->graph(m_curveCount)
                          : m_plot->addGraph();
    // 重新显示并加入图例（clear() 时曾整体移出），保证叠加/替换后的曲线都能在图例中区分。
    graph->setVisible(true);
    graph->addToLegend();
    graph->setPen(QPen(kPalette[m_curveCount % kPaletteSize], 1.5));
    ++m_curveCount;
    return graph;
}

void PlotController::setResult(const ComputeResult &result)
{
    clear();
    appendResult(result);
}

void PlotController::appendResult(const ComputeResult &result)
{
    if (m_plot == nullptr)
        return;

    QCPGraph *graph = acquireGraph();
    if (graph == nullptr)
        return;

    // 图例名优先使用结果标签（含文件名/窗函数等），否则回退到信号类型默认名。
    graph->setName(result.label.isEmpty() ? graphNameFor(result.signalType) : result.label);
    graph->setData(result.displayX, result.displayY);

    // 「显示点数」限制只对时域信号生效；频谱始终显示全量。
    m_timeDomain = (result.signalType == SignalType::TimeDomain);

    // 坐标轴标签随最近一条结果更新。
    m_plot->xAxis->setLabel(xLabelFor(result.signalType));
    m_plot->yAxis->setLabel(yLabelFor(result.signalType));
    m_plot->rescaleAxes();
    applyViewLimit();
    m_plot->replot();
}

bool PlotController::removeResult(const QString &label)
{
    if (m_plot == nullptr || label.isEmpty())
        return false;

    bool removed = false;
    // 倒序移除：removeGraph 会让其后的 graph 下标前移，倒序遍历可避免跳过元素。
    for (int i = m_plot->graphCount() - 1; i >= 0; --i) {
        QCPGraph *graph = m_plot->graph(i);
        if (graph->name() != label)
            continue;
        m_plot->removeGraph(graph); // 内部同步移出图例并销毁该曲线
        --m_curveCount;
        removed = true;
    }
    if (!removed)
        return false;

    if (m_curveCount < 0)
        m_curveCount = 0;

    // 剩余曲线按槽位重排配色，避免与下一次 append 的调色板撞色。
    for (int i = 0; i < m_curveCount && i < m_plot->graphCount(); ++i)
        m_plot->graph(i)->setPen(QPen(kPalette[i % kPaletteSize], 1.5));

    m_plot->rescaleAxes();
    applyViewLimit();
    m_plot->replot();
    return true;
}

void PlotController::setPointLimit(int n)
{
    m_pointLimit = qMax(0, n);

    if (m_plot == nullptr || m_curveCount <= 0)
        return;

    // 参数变化只调整显示窗口，不重建曲线数据。
    // 频谱不使用该限制：始终恢复全量显示。
    if (m_pointLimit == 0 || !m_timeDomain)
        m_plot->rescaleAxes();
    else
        applyViewLimit();
    m_plot->replot();
}

void PlotController::applyViewLimit()
{
    // 「只显示前 N 个点」仅适用于时域信号。
    if (m_plot == nullptr || m_pointLimit <= 0 || m_curveCount <= 0 || !m_timeDomain)
        return;

    // 以第一条有数据的曲线的第 N 个点作为 x 轴右端（各曲线共用同一时间/频率轴）。
    QCPGraph *first = nullptr;
    for (int i = 0; i < m_curveCount && i < m_plot->graphCount(); ++i) {
        QCPGraph *graph = m_plot->graph(i);
        if (graph != nullptr && !graph->data()->isEmpty()) {
            first = graph;
            break;
        }
    }
    if (first == nullptr)
        return;

    const int total = first->data()->size();
    if (total <= m_pointLimit)
        return; // 数据不足 N 个点：保持全量显示

    const double x0 = first->data()->at(0)->key;
    const double x1 = first->data()->at(m_pointLimit - 1)->key;
    if (!(x1 > x0))
        return; // 退化范围（例如所有点 x 相同）：不做收缩

    m_plot->xAxis->setRange(x0, x1);

    // y 轴按「当前可见 x 区间」重算：第一条重设范围，后续曲线只扩大，
    // 避免远处数据的最值把前 N 点压扁。
    bool firstGraph = true;
    for (int i = 0; i < m_curveCount && i < m_plot->graphCount(); ++i) {
        QCPGraph *graph = m_plot->graph(i);
        if (graph == nullptr || graph->data()->isEmpty())
            continue;
        graph->rescaleValueAxis(!firstGraph, true);
        firstGraph = false;
    }
}

void PlotController::clear()
{
    if (m_plot == nullptr)
        return;

    for (int i = 0; i < m_plot->graphCount(); ++i) {
        m_plot->graph(i)->data()->clear();
        m_plot->graph(i)->setName(QString());
        // 同时移出图例并隐藏，避免取消勾选/清空后图例残留空的彩色条目。
        m_plot->graph(i)->removeFromLegend();
        m_plot->graph(i)->setVisible(false);
    }
    m_curveCount = 0;
    m_plot->replot();
}

bool PlotController::exportImage(const QString &filePath)
{
    if (m_plot == nullptr || filePath.isEmpty())
        return false;

    // toPixmap 先在内存渲染当前绘图区，再由 QPixmap::save 按文件扩展名选择格式。
    // 使用较高 scale 保证导出清晰度（打印/报告场景）；与屏幕分辨率无关。
    const QPixmap pixmap = m_plot->toPixmap(0, 0, 2.0);
    return pixmap.save(filePath);
}