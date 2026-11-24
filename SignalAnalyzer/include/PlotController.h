#pragma once

#include <QObject>

#include "SignalTypes.h"

class QCustomPlot;
class QCPGraph;

// 绘图封装：桥接计算结果与 QCustomPlot（支持多条曲线叠加对比）。
// 只接收已算好并已降采样的结果（ComputeResult::displayX / displayY），
// 负责写入曲线、坐标轴标签与重绘；不在 UI 线程做数据搬运或降采样。
class PlotController : public QObject
{
    Q_OBJECT

public:
    explicit PlotController(QCustomPlot *plot, QObject *parent = nullptr);

    // 用结果替换全部曲线（清空后只画这一条）。
    void setResult(const ComputeResult &result);
    // 在现有曲线上追加一条（叠加对比）。
    void appendResult(const ComputeResult &result);
    // 移除图例名等于 label 的曲线（撤销「自验叠加到主图」的入口）。
    // 返回是否真的移除了曲线；移除后重排剩余曲线的配色并重绘。
    bool removeResult(const QString &label);
    // 设置「只显示前 N 个显示点」：n<=0 表示显示全部。
    // 已有曲线会立即按新窗口重绘（不重新计算数据）。
    void setPointLimit(int n);
    // 清空所有曲线数据。
    void clear();

private:
    // 取得下一条可用曲线（按需 addGraph），并按 4 色调色板循环配色。
    QCPGraph *acquireGraph();
    // 按 m_pointLimit 把 x 轴窗口收缩到前 N 个点，并按可见区间重算 y 轴。
    // 调用前应已执行 rescaleAxes()。
    void applyViewLimit();

    QCustomPlot *m_plot;
    int m_curveCount = 0;
    int m_pointLimit = 100;      // 只显示前 N 个显示点；0（或负数）表示显示全部
    bool m_timeDomain = false; // 当前图为时域曲线时，「显示点数」限制才生效
};
