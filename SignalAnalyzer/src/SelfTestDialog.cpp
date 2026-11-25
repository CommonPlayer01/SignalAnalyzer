#include "SelfTestDialog.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
constexpr int kFreqRow = 0;
constexpr int kAmpRow = 1;
}

/**
 * @brief 构造函数：搭建自验对话框界面（4 个参数输入、生成按钮、叠加勾选、
 *        理论/检测对照表），并连接生成按钮与「取消叠加」信号的槽。
 */
SelfTestDialog::SelfTestDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("自验：理论 vs 检测"));
    setMinimumWidth(440);

    // ---- 输入区：f / A / fs / N ----
    m_freqSpin = new QDoubleSpinBox(this);
    m_freqSpin->setRange(0.0, 1.0e6);
    m_freqSpin->setDecimals(3);
    m_freqSpin->setValue(128.0);
    m_freqSpin->setSuffix(QStringLiteral(" Hz"));

    m_ampSpin = new QDoubleSpinBox(this);
    m_ampSpin->setRange(0.0, 1.0e6);
    m_ampSpin->setDecimals(3);
    m_ampSpin->setValue(1.0);

    m_fsSpin = new QDoubleSpinBox(this);
    m_fsSpin->setRange(1.0, 1.0e7);
    m_fsSpin->setDecimals(3);
    m_fsSpin->setValue(1024.0);
    m_fsSpin->setSuffix(QStringLiteral(" Hz"));

    m_nSpin = new QSpinBox(this);
    m_nSpin->setRange(4, 1 << 24);
    m_nSpin->setValue(1024);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("频率 f"), m_freqSpin);
    form->addRow(QStringLiteral("幅值 A"), m_ampSpin);
    form->addRow(QStringLiteral("采样率 fs"), m_fsSpin);
    form->addRow(QStringLiteral("点数 N"), m_nSpin);

    // ---- 操作区 ----
    m_generateBtn = new QPushButton(QStringLiteral("生成并检测"), this);
    m_generateBtn->setDefault(true);
    m_overlayCheck = new QCheckBox(QStringLiteral("叠加到主图"), this);

    auto *btnRow = new QHBoxLayout;
    btnRow->addWidget(m_generateBtn);
    btnRow->addWidget(m_overlayCheck);
    btnRow->addStretch();

    // ---- 对照表：理论 / 检测 / 偏差 ----
    m_table = new QTableWidget(2, 3, this);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("理论值"), QStringLiteral("检测值"), QStringLiteral("偏差")});
    m_table->setVerticalHeaderLabels(
        {QStringLiteral("频率 (Hz)"), QStringLiteral("幅值")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto *hint = new QLabel(
        QStringLiteral("提示：整周期参数（f = k·fs/N）检测最精确；"
                       "非整周期会出现偏差，这正体现了频谱泄漏的影响。"),
        this);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color:#5F6368;font-size:12px;"));

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(btnRow);
    layout->addWidget(m_table);
    layout->addWidget(hint);

    connect(m_generateBtn, &QPushButton::clicked, this, &SelfTestDialog::onGenerateClicked);
    // 取消「叠加到主图」时立即通知主窗口，撤掉此前叠加的自验曲线；
    // 勾选动作无需通知（叠加只在下一次「生成并检测」时生效）。
    connect(m_overlayCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked)
            emit overlayCancelled();
    });
}

/**
 * @brief 「生成并检测」按钮槽：读取 f/A/fs/N 四个参数，先把理论值填入表格、
 *        检测值置为「检测中…」，然后发 selfTestRequested 请求主窗口生成正弦并检测。
 */
void SelfTestDialog::onGenerateClicked()
{
    m_theoryFreq = m_freqSpin->value();
    m_theoryAmp = m_ampSpin->value();
    const double fs = m_fsSpin->value();
    const int n = m_nSpin->value();

    // 先填理论值，检测值/偏差置为「检测中…」，等待后台结果回填。
    m_table->setItem(kFreqRow, 0, new QTableWidgetItem(QString::number(m_theoryFreq, 'f', 3)));
    m_table->setItem(kAmpRow, 0, new QTableWidgetItem(QString::number(m_theoryAmp, 'f', 4)));
    m_table->setItem(kFreqRow, 1, new QTableWidgetItem(QStringLiteral("检测中…")));
    m_table->setItem(kAmpRow, 1, new QTableWidgetItem(QStringLiteral("检测中…")));
    m_table->setItem(kFreqRow, 2, new QTableWidgetItem(QStringLiteral("—")));
    m_table->setItem(kAmpRow, 2, new QTableWidgetItem(QStringLiteral("—")));

    m_generateBtn->setEnabled(false);
    emit selfTestRequested(m_theoryFreq, m_theoryAmp, fs, n, m_overlayCheck->isChecked());
}

/**
 * @brief 主窗口回填检测结果：把检测到的频率/幅值与理论值之差写入表格，并恢复按钮可用。
 */
void SelfTestDialog::applyDetection(double detFreq, double detMag)
{
    m_generateBtn->setEnabled(true);

    const double freqErr = detFreq - m_theoryFreq;
    const double ampErr = detMag - m_theoryAmp;

    m_table->setItem(kFreqRow, 1, new QTableWidgetItem(QString::number(detFreq, 'f', 3)));
    m_table->setItem(kFreqRow, 2, new QTableWidgetItem(QString::number(freqErr, 'f', 3)));
    m_table->setItem(kAmpRow, 1, new QTableWidgetItem(QString::number(detMag, 'f', 4)));
    m_table->setItem(kAmpRow, 2, new QTableWidgetItem(QString::number(ampErr, 'f', 4)));
}

/**
 * @brief 自验计算失败处理：恢复按钮可用，表格检测值标为「失败」并显示原因。
 */
void SelfTestDialog::reportFailure(const QString &error)
{
    m_generateBtn->setEnabled(true);
    m_table->setItem(kFreqRow, 1, new QTableWidgetItem(QStringLiteral("失败")));
    m_table->setItem(kAmpRow, 1, new QTableWidgetItem(QStringLiteral("失败")));
    m_table->setItem(kFreqRow, 2, new QTableWidgetItem(error));
    m_table->setItem(kAmpRow, 2, new QTableWidgetItem(QString()));
}