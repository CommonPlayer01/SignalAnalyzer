#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QTableWidget;

// 自验对话框：输入已知正弦参数（f/A/fs/N），生成后由后台计算并检测谱峰，
// 以表格展示「理论值 / 检测值 / 偏差」对照，用于验证算法的频率/幅值准确性。
class SelfTestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelfTestDialog(QWidget *parent = nullptr);

public slots:
    // 由主窗口在收到自验计算结果（ComputeResult::builtin==true）后回填检测值。
    void applyDetection(double detFreq, double detMag);
    // 由主窗口在自验计算失败时调用，恢复按钮并提示。
    void reportFailure(const QString &error);

signals:
    // 请求主窗口生成该正弦并检测；overlayToPlot 表示是否叠加到主图。
    void selfTestRequested(double freq, double amp, double fs, int n, bool overlayToPlot);
    // 用户取消勾选「叠加到主图」：主窗口据此清理已叠加到主图的自验曲线。
    void overlayCancelled();

private slots:
    void onGenerateClicked();

private:
    QDoubleSpinBox *m_freqSpin = nullptr;
    QDoubleSpinBox *m_ampSpin = nullptr;
    QDoubleSpinBox *m_fsSpin = nullptr;
    QSpinBox *m_nSpin = nullptr;
    QCheckBox *m_overlayCheck = nullptr;
    QPushButton *m_generateBtn = nullptr;
    QTableWidget *m_table = nullptr;

    double m_theoryFreq = 0.0; // 本次请求的理论频率
    double m_theoryAmp = 0.0;  // 本次请求的理论幅值
};
