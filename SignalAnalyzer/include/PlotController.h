#pragma once

#include <QObject>

#include "SignalTypes.h"

class QCustomPlot;

// 绘图封装：桥接计算结果与 QCustomPlot。
// 只接收已算好并已降采样的结果（ComputeResult::displayX / displayY），
// 负责写入曲线、坐标轴标签与重绘；不在 UI 线程做数据搬运或降采样。
class PlotController : public QObject
{
    Q_OBJECT

public:
    explicit PlotController(QCustomPlot *plot, QObject *parent = nullptr);

    // 依据结果绘制（display 数据已在工作线程降采样完成）。
    void setResult(const ComputeResult &result);
    void clear();

private:
    QCustomPlot *m_plot;
};
