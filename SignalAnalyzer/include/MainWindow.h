#pragma once

#include <QtWidgets/QMainWindow>

#include <QHash>
#include <QStringList>

#include <memory>

#include "ui_MainWindow.h"

#include "SignalTypes.h"

class PlotController;
class SelfTestDialog;
class SignalWorker;

// MainWindow（界面薄客户端）：仅负责交互与显示，
// 全部计算通过 SignalWorker（App 内调用 SignalCore 的唯一入口）发起。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onImportFolder();
    void onFileChanged(int index);
    void onSignalChanged(int index);
    void onWindowChanged(int index);
    void onComputeFinished(const ComputeResult &result);
    void onScanFinished(const ScanResult &result);
    void onClearCurves();
    void onOpenSelfTest();
    void onExportImage();
    void onSelfTestRequested(double freq, double amp, double fs, int n, bool overlayToPlot);
    void onSelfTestOverlayCancelled();
    void onOverlayToggled(bool checked);
    void onPointLimitChanged(int n);

private:
    void setupCombos();
    void requestCompute();
    SignalType currentSignalType() const;
    QString currentWindowName() const;

    Ui::MainWindow ui;
    std::unique_ptr<SignalWorker> m_worker;
    std::unique_ptr<PlotController> m_plotController;

    SelfTestDialog *m_selfTestDialog = nullptr; // 自验对话框（懒创建，父对象管理）
    bool m_pendingSelfTestOverlay = false;      // 最近一次自验请求是否叠加到主图
    QStringList m_selfTestOverlayLabels;        // 已叠加到主图的自验曲线图例名（取消勾选时据此清理）
    ComputeResult m_lastResult;                 // 最近一次成功的文件计算结果（取消叠加时恢复单曲线）
    bool m_hasLastResult = false;               // m_lastResult 是否有效

    QString m_dataFolder;
    QStringList m_importedPaths;   // 有效文件路径（与 comboFile 顺序一致）
    QHash<QString, QString> m_keyByPath;  // 绝对路径 -> 「文件名|大小|修改时间」内容签名（路径为身份）
    QString m_currentPath;         // 当前选中文件
    int m_pendingDuplicates = 0;   // 本次导入路径已存在且内容未变化的文件数
    int m_pendingUpdated = 0;      // 本次导入内容已变化、复用原条目并刷新签名的文件数
    bool m_pendingCurrentUpdate = false; // 当前查看文件的内容是否已变化（扫描结束后需重算）
};