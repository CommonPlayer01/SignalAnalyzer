/**
 * @file    MainWindow.cpp
 * @brief   主窗口（MainWindow）实现：界面交互层的「薄客户端」。
 *
 * 设计要点：
 *  - 本类不做任何信号处理/FFT 运算，只负责收集用户意图（选文件夹、选文件、
 *    选信号类型、选窗函数），把请求交给 SignalWorker；
 *  - SignalWorker 在常驻后台线程中调用 SignalCore 完成计算，再以信号形式把
 *    结果回传本类；本类再交给 PlotController 绘制到 QCustomPlot；
 *  - 由此保证 UI 线程不阻塞，同时计算与界面逻辑解耦。
 *
 * 数据流（单向）：
 *  用户操作 -> MainWindow（本文件） -> SignalWorker（后台线程）
 *           -> SignalCore（算法） -> SignalWorker -> MainWindow -> PlotController
 */

#include "MainWindow.h"

// QDir / QFileDialog / QFileInfo(List)：文件夹枚举、目录选择框、文件元信息查询。
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileInfoList>

// QColor / QPen：初始化绘图曲线颜色与画笔；Qt 命名空间常量（如 Qt::AlignTop）。
#include <QColor>
#include <QPen>
#include <Qt>

// QSpinBox：「显示点数」参数（只显示前 N 个点）。
#include <QSpinBox>

// QCustomPlot：第三方绘图控件，UI 中 ui.plot 的类型。
#include "qcustomplot.h"

// PlotController：把 ComputeResult 绘制成曲线的封装（负责降采样、坐标轴、图例）。
#include "PlotController.h"
// SelfTestDialog：自验对话框（输入已知正弦参数，显示理论/检测对照）。
#include "SelfTestDialog.h"
// SignalWorker：后台计算协调器，同时也是 SignalType / ComputeRequest 等类型的定义处。
#include "SignalWorker.h"

namespace {

/**
 * @brief 生成文件的内容签名（判重辅助，判重主键是绝对路径，见 onImportFolder）。
 *
 * 签名 = 文件名 | 文件大小(字节) | 最后修改时间(自 epoch 起的毫秒数)。
 * 签名只用于判断「同一路径下的文件内容是否发生变化」：
 *  - 签名相同 → 内容未变，跳过（重复）；
 *  - 签名不同 → 内容已变，复用原条目并刷新签名（不新增条目）。
 */
QString fileKey(const QFileInfo &fi)
{
    return fi.fileName()
           + QLatin1Char('|')
           + QString::number(fi.size())
           + QLatin1Char('|')
           + QString::number(fi.lastModified().toMSecsSinceEpoch());
}

} // namespace

/**
 * @brief 构造函数：初始化界面、创建后台 worker 与绘图控制器、连接信号槽。
 *
 * 初始化顺序很关键：
 *  1) setupUi 生成全部子控件；
 *  2) 创建 SignalWorker（启动后台常驻线程）与 PlotController（依赖 ui.plot 已存在）；
 *  3) 填充下拉框，并连接「用户交互 -> 槽函数」与「worker 结果 -> 槽函数」；
 *  4) 配置绘图区基础样式与交互能力。
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // unique_ptr 管理生命周期：窗口析构时自动销毁 worker（并安全退出其线程）
    // 与绘图控制器。
    m_worker = std::make_unique<SignalWorker>();
    m_plotController = std::make_unique<PlotController>(ui.plot);

    // 填充信号类型、窗函数下拉框的选项。
    setupCombos();

    // 时域不支持叠加对比，默认禁用「叠加对比」勾选框（仅频谱可用）。
    ui.chkOverlay->setEnabled(currentSignalType() != SignalType::TimeDomain);
    // 「显示点数」只对时域信号生效，频谱下禁用该参数。
    ui.spinPointLimit->setEnabled(currentSignalType() == SignalType::TimeDomain);
    // 自验即谱峰检测，只对「幅值谱」开放：其余类型下禁用自验按钮。
    ui.btnSelfTest->setEnabled(currentSignalType() == SignalType::AmplitudeSpectrum);

    // ---- 用户交互 -> 本类槽函数 ----
    // 点击「导入文件夹」按钮。
    connect(ui.btnImportFolder, &QPushButton::clicked,
            this, &MainWindow::onImportFolder);
    // 切换文件：QOverload 用于在重载信号中明确选择 int 版本。
    connect(ui.comboFile, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFileChanged);
    // 切换信号类型（时域 / 幅值谱 / 自功率谱）。
    connect(ui.comboSignal, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSignalChanged);
    // 切换窗函数（矩形窗 / 汉宁窗）。
    connect(ui.comboWindow, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onWindowChanged);
    // 叠加对比控制：清空曲线、打开自验对话框。
    connect(ui.btnClearCurves, &QPushButton::clicked,
            this, &MainWindow::onClearCurves);
    connect(ui.btnSelfTest, &QPushButton::clicked,
            this, &MainWindow::onOpenSelfTest);
    // 导出图片：把当前绘图区保存为图片文件（按扩展名选格式）。
    connect(ui.btnExportImage, &QPushButton::clicked,
            this, &MainWindow::onExportImage);
    // 叠加勾选状态切换：取消勾选时清除叠加曲线，仅保留最近一条结果。
    connect(ui.chkOverlay, &QCheckBox::toggled,
            this, &MainWindow::onOverlayToggled);
    // 显示点数参数变化：立即按新窗口重绘（不重新计算）。
    connect(ui.spinPointLimit, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onPointLimitChanged);

    // ---- 后台计算结果 -> 本类槽函数 ----
    // 注意：worker 已按「代际号」过滤过期结果，本类收到的总是最新一次请求的结果。
    connect(m_worker.get(), &SignalWorker::computeFinished,
            this, &MainWindow::onComputeFinished);
    connect(m_worker.get(), &SignalWorker::scanFinished,
            this, &MainWindow::onScanFinished);

    // 初始化绘图区（基础交互：缩放 / 拖拽）。
    // 允许鼠标拖拽平移、滚轮缩放坐标轴范围。
    ui.plot->setInteraction(QCP::iRangeDrag, true);
    ui.plot->setInteraction(QCP::iRangeZoom, true);
    // 只创建一个 graph（曲线对象），后续 setData 复用同一条曲线。
    ui.plot->addGraph();
    // 设置曲线为蓝色，避免默认黑色在深色背景下不清晰。
    ui.plot->graph(0)->setPen(QPen(QColor(30, 120, 200)));
    // 显示图例，并把图例放在坐标区右上角（inset 布局的第 0 个元素）。
    ui.plot->legend->setVisible(true);
    ui.plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);
    // 坐标轴标签：横轴单位随结果变化（时域为秒、频谱为 Hz），此处先给默认值。
    ui.plot->xAxis->setLabel(tr("时间 (s)"));
    ui.plot->yAxis->setLabel(tr("幅值"));

    // 初始清空绘图区（隐藏图例、清空数据），避免空白窗口显示残留状态。
    m_plotController->clear();
}

/**
 * @brief 析构函数：成员 unique_ptr 自动释放 worker 与绘图控制器。
 *
 * 函数体为空是刻意为之——所有资源均由 RAII（unique_ptr）管理，
 * SignalWorker 的析构会负责停止后台线程。
 */
MainWindow::~MainWindow()
{}

/**
 * @brief 初始化两个下拉框的选项。
 *
 * 约定：
 *  - comboSignal 的选项顺序必须与枚举 SignalType 的取值严格一致
 *    （TimeDomain=0 / AmplitudeSpectrum=1 / PowerSpectrum=2），
 *    因为 currentSignalType() 直接用 currentIndex() 强转枚举；
 *  - comboWindow 的 userData 保存传给算法层的窗函数名 "rect" / "hann"，
 *    显示文本可自由本地化，不影响计算。
 */
void MainWindow::setupCombos()
{
    // 窗函数下拉框：矩形窗（等效不加窗）与汉宁窗（任务书 3.(5)）。
    ui.comboWindow->addItem(tr("矩形窗（等效不加窗）"), QStringLiteral("rect"));
    ui.comboWindow->addItem(tr("汉宁窗"), QStringLiteral("hann"));

    // 信号类型下拉框（任务书 3.(4)）。
    ui.comboSignal->addItem(tr("时域信号"));
    ui.comboSignal->addItem(tr("幅值谱（线性谱）"));
    ui.comboSignal->addItem(tr("自功率谱"));
}

/**
 * @brief 取当前选中的信号类型。
 * @return 由 comboSignal 当前下标强转得到的 SignalType。
 *
 * 依赖 setupCombos() 中「下拉框顺序 == 枚举顺序」的约定，二者不可单独修改。
 */
SignalType MainWindow::currentSignalType() const
{
    return static_cast<SignalType>(ui.comboSignal->currentIndex());
}

/**
 * @brief 取当前选中窗函数的算法标识（userData，而非显示文本）。
 * @return "rect" 或 "hann"；若下拉框为空则返回空字符串。
 */
QString MainWindow::currentWindowName() const
{
    return ui.comboWindow->currentData().toString();
}

/**
 * @brief 「导入文件夹」按钮的响应：选择目录、过滤重复文件、发起后台批量扫描。
 *
 * 两种导入语义：
 *  - 选择了**不同**的文件夹：视为「切换数据源」，先清空上一文件夹的导入状态
 *    （comboFile、m_importedPaths、m_keyByPath、m_currentPath 与绘图区），
 *    该文件夹下的全部 txt 都作为新文件导入，即「替换」；
 *  - 选择了**同一个**文件夹：视为「刷新/补充」，以**绝对路径**为主键做判重：
 *      · 路径已存在且内容签名未变 → 跳过（重复）；
 *      · 路径已存在但内容签名已变 → 复用原条目、只刷新签名（替换条目，不新增条目）；
 *      · 路径不存在             → 作为新文件追加（追加）。
 *
 * 处理流程：
 *  1) 弹出目录选择框，默认定位到上次导入的目录（首次为用户主目录）；
 *  2) 若文件夹发生变化，先重置导入状态（替换逻辑）；
 *  3) 枚举该目录下所有 *.txt（仅文件、按文件名排序，不递归子目录）；
 *  4) 按「路径 → 内容签名」分类出候选新文件，并统计重复/已更新文件数；
 *  5) 若没有候选文件，更新状态栏提示（必要时重算当前文件）后返回；
 *  6) 否则调用 m_worker->requestScan()，真正的文件校验（采样率、坏行统计）
 *     在后台线程完成，结果通过 scanFinished 信号回到 onScanFinished()。
 *
 * 注意：本函数不解析文件内容，因此不会阻塞 UI。
 */
void MainWindow::onImportFolder()
{
    // getExistingDirectory：模态目录选择框；第三个参数为初始目录。
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("选择数据源文件夹"), m_dataFolder.isEmpty() ? QDir::homePath() : m_dataFolder);

    // 用户点了「取消」：保持原有状态不变。
    if (dir.isEmpty())
        return;

    // 判断是否为「切换数据源文件夹」（首次导入时 m_dataFolder 为空，也按切换处理）。
    const bool switchedFolder = (dir != m_dataFolder);

    if (switchedFolder) {
        // ---- 替换逻辑：丢弃上一文件夹的全部导入状态 ----
        // 清空下拉框时先屏蔽信号：clear() 会把当前下标置为 -1 并发出
        // currentIndexChanged(-1)，屏蔽后可避免触发多余的重算/清图流程。
        ui.comboFile->blockSignals(true);
        ui.comboFile->clear();
        ui.comboFile->blockSignals(false);

        // 路径表、路径->签名映射必须同步清空，否则会与已清空的下拉框下标错位，
        // 且旧文件夹的文件会在新文件夹中继续被误判为「重复」。
        m_importedPaths.clear();
        m_keyByPath.clear();
        m_currentPath.clear();
        m_plotController->clear();   // 旧文件夹的曲线不能残留
        m_selfTestOverlayLabels.clear();
    }
    // 注：同一文件夹重复导入时不清空任何状态，走后面的判重逻辑实现「追加」。

    m_dataFolder = dir;

    // 枚举 txt 文件。判重主键是**绝对路径**，fileKey() 只作为该路径的内容签名：
    //  - 切换文件夹后 m_keyByPath 已清空，因此该文件夹的文件全部作为候选（替换）；
    //  - 同一文件夹重导入时，路径已存在且签名未变 → 跳过；签名已变 → 只刷新签名（不新增条目）。
    // QDir::Files 表示只取文件、不取子目录；QDir::Name 表示按名称排序，保证导入顺序稳定。
    const QFileInfoList entries = QDir(dir).entryInfoList(QStringList() << QStringLiteral("*.txt"),
                                                          QDir::Files, QDir::Name);
    QStringList candidates;   // 本次真正需要后台扫描的「新路径」
    int duplicates = 0;       // 路径已存在且内容未变化的文件数
    int updated = 0;          // 路径已存在但内容已变化（复用条目）的文件数
    bool currentFileChanged = false; // 变化的是否正好是当前查看的文件

    for (const QFileInfo &fi : entries) {
        const QString path = fi.absoluteFilePath();
        const QString key = fileKey(fi);

        // 以路径查已有记录：找到即说明这条文件已经在列表里，不新增条目。
        const auto it = m_keyByPath.find(path);
        if (it != m_keyByPath.end()) {
            if (it.value() == key) {
                // 内容没变：真正的重复文件，跳过。
                ++duplicates;
                continue;
            }
            // 同一路径、内容已变化：替换（复用条目 + 刷新签名），绝不产生同名条目。
            // 列表条目无需更新：每次计算都会重新读盘，选中时自然显示最新内容。
            it.value() = key;
            ++updated;
            if (path == m_currentPath)
                currentFileChanged = true;
            continue;
        }
        candidates << path;   // 路径不存在：新文件，交给后台校验后追加
    }

    // 暂存本次统计，供 onScanFinished() 汇总到同一条状态栏消息中。
    m_pendingDuplicates = duplicates;
    m_pendingUpdated = updated;
    m_pendingCurrentUpdate = currentFileChanged;

    if (candidates.isEmpty()) {
        // 没有任何新文件：区分「有文件被更新」「全是重复」「目录里没有 txt」三种提示。
        if (updated > 0)
            statusBar()->showMessage(tr("未发现新文件，%1 个文件内容已更新").arg(updated));
        else if (duplicates > 0)
            statusBar()->showMessage(tr("未发现新文件，已跳过 %1 个重复文件").arg(duplicates));
        else
            statusBar()->showMessage(tr("文件夹中未找到 txt 数据文件"));

        // 当前查看的文件内容变了：立即重算重绘（其余被更新的条目在选中时会重新读盘）。
        if (currentFileChanged)
            requestCompute();
        return;
    }

    // 先给用户即时反馈，再异步扫描（扫描过程中界面依然可交互）。
    statusBar()->showMessage(tr("正在导入 %1 个文件…").arg(candidates.size()));
    m_worker->requestScan(candidates);
}

/**
 * @brief 后台批量扫描完成的回调：登记有效文件、更新下拉框与状态栏。
 * @param result 扫描结果，含有效路径列表及三类跳过统计。
 *
 * 关键点：
 *  - m_importedPaths 与 comboFile 的下标必须一一对应（都以「有效文件」为单位），
 *    因此这里逐个路径同时追加到两者；
 *  - 只有有效文件才写入 m_keyByPath（路径 -> 内容签名），被跳过的文件下次导入时会被再次尝试；
 *  - 内容已变化（但路径已存在）的文件走「复用条目 + 刷新签名」，不会出现在 validPaths 中；
 *  - 若本次有新增文件，会自动选中第一个新文件（触发 onFileChanged -> requestCompute），
 *    让用户导入后立即看到曲线。
 */
void MainWindow::onScanFinished(const ScanResult &result)
{
    // 记录本次新增文件在 comboFile 中的起始下标。
    // 必须在 addItem 之前取 count()，此时 count() 恰好等于下一个可用下标。
    const int firstNewIndex = ui.comboFile->count();
    int added = 0;
    for (const QString &path : result.validPaths) {
        const QFileInfo fi(path);
        // 路径表、路径->签名映射、下拉框三者同步追加，保持下标一致。
        m_importedPaths << path;
        m_keyByPath.insert(path, fileKey(fi));
        ui.comboFile->addItem(fi.fileName());   // 下拉框只显示文件名，完整路径存在 m_importedPaths
        ++added;
    }

    // 组装状态栏汇总：先给出新增数，再按需追加各类跳过统计。
    QString summary = tr("导入完成：新增 %1 个有效文件").arg(added);
    if (m_pendingUpdated > 0)
        summary += tr("，%1 个文件内容已更新").arg(m_pendingUpdated);
    if (m_pendingDuplicates > 0)
        summary += tr("，跳过 %1 个重复文件").arg(m_pendingDuplicates);
    if (result.skippedMissingFs > 0)
        summary += tr("，跳过 %1 个缺失采样率文件").arg(result.skippedMissingFs);
    if (result.skippedEmpty > 0)
        summary += tr("，跳过 %1 个无效文件").arg(result.skippedEmpty);
    statusBar()->showMessage(summary);

    // 导入/切换文件夹后，自动选中本次新增的第一个文件并重新加载（触发计算）。
    // 即使之前已在查看旧文件夹的文件，也会切到新文件夹的首个文件。
    if (added > 0) {
        // 提示：QComboBox::setCurrentIndex 仅在下标真正变化时才会发出
        // currentIndexChanged；若 selected 下标与当前一致，则不会重复计算。
        ui.comboFile->setCurrentIndex(firstNewIndex);
    } else if (m_currentPath.isEmpty() && !m_importedPaths.isEmpty()) {
        // 没有新增文件，但此前也没有任何选中项（例如首次导入全部被判重）：
        // 兜底选中第 0 项，保证界面至少有一份可查看的数据。
        ui.comboFile->setCurrentIndex(0);
    }

    // 内容被更新且正在查看的若是当前文件，需要重算一次才能看到最新曲线。
    // （added > 0 时上面的 setCurrentIndex 会切到新文件并触发计算，无需重复请求）
    if (m_pendingCurrentUpdate && added == 0 && !m_currentPath.isEmpty())
        requestCompute();
    m_pendingCurrentUpdate = false;
}

/**
 * @brief 文件下拉框切换回调：更新当前路径并触发一次重新计算。
 * @param index comboFile 的新下标；理论上与 m_importedPaths 下标一一对应。
 *
 * 越界或负数（例如清空下拉框）时，视为「无选中文件」：清空路径并清空绘图区，
 * 防止旧曲线残留在界面上。
 */
void MainWindow::onFileChanged(int index)
{
    if (index < 0 || index >= m_importedPaths.size()) {
        m_currentPath.clear();
        m_plotController->clear();
        m_selfTestOverlayLabels.clear();
        return;
    }

    m_currentPath = m_importedPaths[index];
    requestCompute();
}

/**
 * @brief 信号类型切换回调：类型仅影响计算，需对当前文件重新计算。
 * @param index 新的下标（未直接使用），实际类型由 currentSignalType() 读取。
 *
 * 仅在已选中文件时重算；没有文件时不做任何事，避免无意义的空请求。
 */
void MainWindow::onSignalChanged(int index)
{
    Q_UNUSED(index);

    // 切换类型即切换结果语义：立即清空旧画布，避免等待后台重算期间残留旧类型曲线。
    // 同时丢弃可恢复的最后结果，防止重算完成前的空档被 setResult(m_lastResult) 恢复旧图。
    m_plotController->clear();
    m_hasLastResult = false;
    m_selfTestOverlayLabels.clear();

    // 时域不支持叠加对比：禁用勾选框；若正勾选则强制取消（会触发清空叠加曲线）。
    const bool spectral = (currentSignalType() != SignalType::TimeDomain);
    ui.chkOverlay->setEnabled(spectral);
    if (!spectral && ui.chkOverlay->isChecked())
        ui.chkOverlay->setChecked(false);

    // 「显示点数」只对时域生效：频谱下禁用（其值保留，切回时域后重新生效）。
    ui.spinPointLimit->setEnabled(!spectral);

    // 自验功能仅「幅值谱」可用：切换类型时同步按钮可用态。
    ui.btnSelfTest->setEnabled(currentSignalType() == SignalType::AmplitudeSpectrum);

    if (!m_currentPath.isEmpty())
        requestCompute();
}

/**
 * @brief 窗函数切换回调：窗函数仅影响频谱类计算，同样重新计算当前文件。
 * @param index 新的下标（未直接使用），实际窗名由 currentWindowName() 读取。
 */
void MainWindow::onWindowChanged(int index)
{
    Q_UNUSED(index);
    if (!m_currentPath.isEmpty())
        requestCompute();
}

/**
 * @brief 组装并投递一次计算请求到后台线程。
 *
 * 请求内容 = 当前文件路径 + 当前信号类型 + 当前窗函数。
 * 该函数是「文件切换 / 类型切换 / 窗函数切换」三条交互路径的统一出口，
 * 因此任何影响结果的 UI 变化都只需调它即可。
 *
 * 若当前没有选中文件则直接返回，不产生后台任务。
 */
void MainWindow::requestCompute()
{
    if (m_currentPath.isEmpty())
        return;

    ComputeRequest request;
    request.filePath = m_currentPath;
    request.signalType = currentSignalType();
    request.windowName = currentWindowName();

    // 图例标签：文件名 · 窗函数 · 信号类型。随请求透传，避免异步回调时读到已变化的 UI 状态。
    const QString winText = (request.windowName == QStringLiteral("hann"))
                                ? tr("汉宁窗") : tr("矩形窗");
    request.label = tr("%1 · %2 · %3")
                        .arg(QFileInfo(m_currentPath).fileName(),
                             winText,
                             ui.comboSignal->currentText());

    // 异步投递：立即返回，结果稍后由 onComputeFinished 接收。
    m_worker->requestCompute(request);
}

/**
 * @brief 后台计算完成的回调：绘制曲线并在状态栏展示统计信息。
 * @param result 计算结果；result.ok 为 false 时表示解析/计算失败。
 *
 * 失败处理：
 *  - 清空绘图区，避免用户误把上一次的曲线当成当前结果；
 *  - 状态栏显示失败原因（result.error）。
 *
 * 成功处理：
 *  - 交给 PlotController 绘制（内部做降采样，避免大数据量卡顿）；
 *  - 状态栏依次展示采样率、点数、FFT 点数、RMS；
 *    频谱类结果再追加谱峰频率/幅值与窗函数名；
 *    若存在坏行则最后追加坏行数。
 */
void MainWindow::onComputeFinished(const ComputeResult &result)
{
    // ---- 自验结果：回填弹窗对照表，按需叠加到主图，不走文件结果的常规叠加逻辑。----
    if (result.builtin) {
        if (m_selfTestDialog) {
            if (result.ok)
                m_selfTestDialog->applyDetection(result.peakFreq, result.peakMag);
            else
                m_selfTestDialog->reportFailure(result.error);
        }
        if (result.ok && m_pendingSelfTestOverlay) {
            m_plotController->appendResult(result);
            // 记录本次叠加的图例名，供用户取消勾选时精确移除。
            m_selfTestOverlayLabels << result.label;
        }
        statusBar()->showMessage(
            result.ok ? tr("自验：检测谱峰 %1 Hz / %2")
                            .arg(result.peakFreq, 0, 'f', 3)
                            .arg(result.peakMag, 0, 'f', 4)
                      : tr("自验失败：%1").arg(result.error));
        return;
    }

    if (!result.ok) {
        // 叠加模式下失败不清除已有曲线，避免破坏正在对比的多条曲线。
        if (!ui.chkOverlay->isChecked())
            m_plotController->clear();
        statusBar()->showMessage(tr("解析失败：%1").arg(result.error));
        return;
    }

    // 保存最近一次成功结果（取消叠加 / 切回时域时用其恢复单曲线）。
    m_lastResult = result;
    m_hasLastResult = true;

    // 仅频谱类（幅值谱 / 自功率谱）允许叠加；时域强制单曲线。
    const bool canOverlay = (result.signalType != SignalType::TimeDomain)
                            && ui.chkOverlay->isChecked();
    if (canOverlay)
        m_plotController->appendResult(result);
    else
        m_plotController->setResult(result);

    // 基础统计：采样率（取整显示）与有效点数 N。
    QString message = tr("fs=%1 Hz · N=%2")
                          .arg(result.fs, 0, 'f', 0)
                          .arg(static_cast<qulonglong>(result.pointCount));
    // 仅频谱类结果才有 FFT 点数（零填充后的长度），时域时为 0，故按需拼接。
    if (result.fftSize > 0)
        message += tr(" / FFT=%1").arg(static_cast<qulonglong>(result.fftSize));
    // 均方根值保留 4 位小数。
    message += tr(" · RMS=%1").arg(result.rms, 0, 'f', 4);

    // 频谱类结果额外展示谱峰位置/幅值与所用窗函数，便于对照任务书要求。
    if (result.signalType != SignalType::TimeDomain) {
        message += tr(" · 谱峰 %1 Hz / %2")
                       .arg(result.peakFreq, 0, 'f', 1)
                       .arg(result.peakMag, 0, 'f', 4);
        // 注：此处直接读取下拉框当前值（与请求同源），属显示用途。
        message += tr(" · 窗函数 %1").arg(currentWindowName());
    }

    // 无法解析为数值的数据行数量（已用线性插值填补）。
    if (result.skippedLines > 0)
        message += tr(" · 插值坏行 %1").arg(result.skippedLines);

    statusBar()->showMessage(message);
}

// 清空所有叠加曲线。
void MainWindow::onClearCurves()
{
    m_plotController->clear();
    m_hasLastResult = false;   // 清空后无可恢复曲线
    m_selfTestOverlayLabels.clear(); // 自验曲线随之一并清空
    statusBar()->showMessage(tr("已清空曲线"));
}

// 叠加勾选状态变化：取消勾选时清除叠加曲线，只保留最近一条结果。
void MainWindow::onOverlayToggled(bool checked)
{
    if (checked)
        return; // 勾选时无需立即动作；后续切换配置才追加曲线。

    if (m_hasLastResult)
        m_plotController->setResult(m_lastResult); // 内部先清空再画单条
    else
        m_plotController->clear();
    // setResult/clear 都会重建全部曲线，已叠加的自验曲线随之消失。
    m_selfTestOverlayLabels.clear();
}

// 打开（或前置）自验对话框：懒创建并连接其请求信号。
void MainWindow::onOpenSelfTest()
{
    if (!m_selfTestDialog) {
        m_selfTestDialog = new SelfTestDialog(this);
        connect(m_selfTestDialog, &SelfTestDialog::selfTestRequested,
                this, &MainWindow::onSelfTestRequested);
        // 取消「叠加到主图」：把已叠加的自验曲线从主图移除（文件结果保持不动）。
        connect(m_selfTestDialog, &SelfTestDialog::overlayCancelled,
                this, &MainWindow::onSelfTestOverlayCancelled);
    }
    m_selfTestDialog->show();
    m_selfTestDialog->raise();
    m_selfTestDialog->activateWindow();
}

// 自验请求：内置生成正弦（不读文件），走与文件相同的后台计算与谱峰检测。
void MainWindow::onSelfTestRequested(double freq, double amp, double fs, int n, bool overlayToPlot)
{
    m_pendingSelfTestOverlay = overlayToPlot;

    ComputeRequest request;
    request.useBuiltin = true;
    request.builtinFreq = freq;
    request.builtinAmp = amp;
    request.builtinFs = fs;
    request.builtinN = static_cast<std::size_t>(n);
    request.signalType = SignalType::AmplitudeSpectrum;
    request.windowName = currentWindowName();
    request.label = tr("自验 f=%1Hz A=%2")
                        .arg(freq, 0, 'f', 3)
                        .arg(amp, 0, 'f', 3);
    m_worker->requestCompute(request);
}

// 显示点数参数变化：按新值收缩/恢复 x 轴窗口，不重新计算数据。
void MainWindow::onPointLimitChanged(int n)
{
    m_plotController->setPointLimit(n);

    if (m_hasLastResult) {
        statusBar()->showMessage(
            n > 0 ? tr("只显示前 %1 个点").arg(n) : tr("显示全部数据点"));
    }
}

// 导出图片：弹出保存对话框，把当前绘图区保存为图片文件。
void MainWindow::onExportImage()
{
    // 默认文件名：基名_信号类型_窗函数，存放目录跟随当前数据源文件夹。
    // 信号类型复用下拉框当前文本；窗函数沿用与图例一致的 hann→汉宁窗 映射。
    QString defaultPath;
    if (!m_currentPath.isEmpty()) {
        const QFileInfo fi(m_currentPath);
        const QString winText = (currentWindowName() == QStringLiteral("hann"))
                                    ? tr("汉宁窗") : tr("矩形窗");
        const QString baseName = QStringLiteral("%1_%2_%3")
                                     .arg(fi.completeBaseName(),
                                          ui.comboSignal->currentText(),
                                          winText);
        defaultPath = fi.absoluteDir().absoluteFilePath(baseName + QStringLiteral(".png"));
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("导出图片"),
        defaultPath,
        tr("PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg);;BMP 图片 (*.bmp)"));
    if (filePath.isEmpty())
        return; // 用户取消

    if (m_plotController->exportImage(filePath)) {
        statusBar()->showMessage(tr("图片已导出：%1").arg(filePath));
    } else {
        statusBar()->showMessage(tr("导出图片失败：%1").arg(filePath));
    }
}

// 取消「叠加到主图」：按图例名移除此前叠加的自验曲线，文件结果曲线保持不动。
void MainWindow::onSelfTestOverlayCancelled()
{
    if (m_selfTestOverlayLabels.isEmpty())
        return;

    bool removedAny = false;
    for (const QString &label : m_selfTestOverlayLabels)
        removedAny = m_plotController->removeResult(label) || removedAny;
    m_selfTestOverlayLabels.clear();

    if (removedAny)
        statusBar()->showMessage(tr("已从主图移除自验曲线"));
}