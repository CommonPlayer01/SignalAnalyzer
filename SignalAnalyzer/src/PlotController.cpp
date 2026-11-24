#include "PlotController.h"

#include <QColor>
#include <QPen>
#include <Qt>

#include "qcustomplot.h"

namespace {

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

void PlotController::setResult(const ComputeResult &result)
{
    if (m_plot == nullptr)
        return;

    // 显示数据由 ComputeTask 在工作线程内完成降采样，
    // UI 线程只需把结果写入曲线并重绘（不再搬运全量数据）。
    m_plot->graph(0)->setName(graphNameFor(result.signalType));
    m_plot->graph(0)->setData(result.displayX, result.displayY);
    m_plot->xAxis->setLabel(xLabelFor(result.signalType));
    m_plot->yAxis->setLabel(yLabelFor(result.signalType));
    m_plot->rescaleAxes();
    m_plot->replot();
}

void PlotController::clear()
{
    if (m_plot == nullptr)
        return;

    m_plot->graph(0)->setName(QString());
    m_plot->graph(0)->setData(QVector<double>(), QVector<double>());
    m_plot->replot();
}


