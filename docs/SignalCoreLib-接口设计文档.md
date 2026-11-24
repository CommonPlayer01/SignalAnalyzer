# SignalCoreLib 接口设计文档

| 项目 | 内容 |
| --- | --- |
| 库名称 | `SignalCoreLib`（静态库，CMake target `SignalCoreLib`） |
| 版本 | 1.0 |
| 语言标准 | C++17（`CMAKE_CXX_EXTENSIONS OFF`） |
| 命名空间 | `SignalCoreLib` |
| 公共头文件目录 | `SignalCoreLib/include/SignalCoreLib/` |
| 头文件引用方式 | `#include "SignalCoreLib/Xxx.h"` |
| 外部依赖 | 无（仅 C++ 标准库；不依赖 Qt、不使用第三方 FFT） |
| 适用场景 | 桌面/控制台/服务端程序中的信号读取、FFT、窗函数、幅值谱/自功率谱、RMS 计算 |

---

## 1. 设计目标与约束

1. **算法与界面解耦**：库内不包含任何 Qt 头文件与界面代码，可被非 Qt 工程直接链接。
2. **可重入、可在线程中使用**：除 `WindowFactory` 的注册表外，所有接口为纯函数或无状态类，可在工作线程中并发调用。
3. **接口最小化**：对外只暴露 4 个可实例化入口（`SignalReader`、`SpectrumAnalyzer`、`WindowFactory`、`RmsCalculator`）与 4 个数据/抽象类型（`SignalData`、`Spectrum`、`ReadResult`、`IWindow`）。
4. **可扩展**：新增窗函数只需新增 `IWindow` 实现 + 在 `WindowFactory.cpp` 登记一行，既有处理框架代码零修改（开闭原则）。
5. **错误以返回值表达**：库内不抛异常（`std::bad_alloc` 等标准库异常除外），失败通过 `ReadResult::ok` / 空指针 / 空容器传达。
6. **确定性**：相同输入必然得到逐位一致的输出，便于回归测试。

---

## 2. 模块架构

```
                 ┌──────────────────────────────────────────────┐
                 │            调用方（App / 测试）               │
                 └──────────────────────────────────────────────┘
                        │            │            │
        ┌───────────────┘            │            └────────────────┐
        ▼                            ▼                             ▼
┌───────────────┐          ┌──────────────────┐           ┌────────────────┐
│ SignalReader  │          │ SpectrumAnalyzer │           │ RmsCalculator  │
│ readFile/parse│          │ amplitudeSpectrum│           │ compute        │
└───────┬───────┘          │ powerSpectrum    │           └────────────────┘
        │                  │ frequencyAxis    │
        │                  │ nextPowerOfTwo   │
        │                  └────────┬─────────┘
        │                           │ 依赖
        ▼                           ▼
┌───────────────┐          ┌──────────────────┐        ┌──────────────────┐
│  SignalData   │◀─────────│       Fft        │        │   IWindow (抽象)  │
│  Spectrum     │          │ forward/inverse  │        │       ▲    ▲      │
│  ReadResult   │          └──────────────────┘        │       │    │      │
└───────────────┘                                      │  RectWindow  HannWindow
                                                       └──────────────────┘
                                                                ▲
                                                     ┌──────────────────┐
                                                     │  WindowFactory   │
                                                     │ registerFactory  │
                                                     │ create / names   │
                                                     └──────────────────┘
```

依赖方向单向：`SignalReader → SignalData`，`SpectrumAnalyzer → Fft + IWindow + SignalData`，`WindowFactory → IWindow 实现`。不存在循环依赖。

---

## 3. 接口索引

| 头文件 | 类型 | 类别 | 接口 |
| --- | --- | --- | --- |
| `SignalData.h` | `SignalData` | 数据容器 | `sampleRate()` `size()` `samples()` `at()` |
| `SignalReader.h` | `SignalReader` / `ReadResult` / `ProbeResult` | 输入 | `readFile()` `parse()` `probeFile()` `probe()` |
| `Fft.h` | `Fft` | 算法 | `forward()` `inverse()` |
| `IWindow.h` | `IWindow` | 抽象接口 | `name()` `apply()` `correctionFactor()` |
| `RectWindow.h` | `RectWindow` | 窗实现 | 见 `IWindow` |
| `HannWindow.h` | `HannWindow` | 窗实现 | 见 `IWindow` |
| `WindowFactory.h` | `WindowFactory` / `IWindowFactory` | 工厂 | `registerFactory()` `create()` `names()` `createWindow()` |
| `SpectrumAnalyzer.h` | `SpectrumAnalyzer` / `Spectrum` | 算法 | `amplitudeSpectrum()` `powerSpectrum()` `frequencyAxis()` `nextPowerOfTwo()` |
| `RmsCalculator.h` | `RmsCalculator` | 算法 | `compute()` |

所有类型均位于 `namespace SignalCoreLib`，全部为静态成员函数或纯数据/抽象类，**不提供导出宏（非 DLL 导出符号，静态库链接）**。

---

## 4. 数据结构

### 4.1 `SignalData`（`SignalData.h`）

信号数据与元数据容器，持有采样率与幅值序列。

| 成员 | 签名 | 说明 |
| --- | --- | --- |
| 构造 | `SignalData()` | 默认构造：`sampleRate()==0.0`，`size()==0` |
| 构造 | `SignalData(double sampleRate, std::vector<double> samples)` | 移动语义接管 `samples`，O(1) 移交 |
| 查询 | `double sampleRate() const` | 采样率，单位 Hz |
| 查询 | `std::size_t size() const` | 有效点数（不含补零点） |
| 查询 | `const std::vector<double>& samples() const` | 幅值序列只读引用，空序列安全 |
| 查询 | `double at(std::size_t i) const` | 单点访问 |

**约定与约束**

- 只读视图：类内不提供写接口，`samples()` 返回 `const&`，调用方无需拷贝即可遍历。
- `at(i)` **不做越界检查**（与 `std::vector::at` 语义不同），索引合法性由调用方保证。
- 容器可拷贝、可移动；拷贝为深拷贝。
- 构造时不校验 `sampleRate` 正负与序列是否为空（校验责任在 `SignalReader`）。

### 4.2 `Spectrum`（`SpectrumAnalyzer.h`）

频谱结果，两轴等长（长度均为 `N_fft/2 + 1`）。

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `frequencies` | `std::vector<double>` | 频率轴，单位 Hz，`f_k = k·fs/N_fft`，`k = 0..N_fft/2` |
| `magnitudes` | `std::vector<double>` | 幅值轴：幅值谱为线性幅值；自功率谱为其平方 |

### 4.3 `ReadResult`（`SignalReader.h`）

文件/文本解析结果。

| 字段 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `ok` | `bool` | `false` | 是否解析成功 |
| `error` | `std::string` | 空 | 失败原因，仅 `ok==false` 时有意义 |
| `skippedLines` | `std::size_t` | `0` | 坏行数（无法解析为 `double` 或为 `nan`/`inf`，已线性插值填补） |
| `data` | `SignalData` | 空 | 解析出的数据，仅 `ok==true` 时有效 |

---

## 5. 接口详细定义

### 5.1 `SignalReader` — 信号文件读取

```cpp
class SignalReader {
public:
    static ReadResult readFile(const std::string& path);
    static ReadResult parse(const std::string& text);
    static ProbeResult probeFile(const std::string& path);
    static ProbeResult probe(const std::string& text);
};
```

| 接口 | 参数 | 返回值 | 失败情形 |
| --- | --- | --- | --- |
| `readFile(path)` | `path`：文件路径（**窄字符串**） | `ReadResult` | 打不开 → `ok=false`，`error="cannot open file: <path>"`；其余同 `parse` |
| `parse(text)` | `text`：完整文件文本内容 | `ReadResult` | 见下表 |
| `probeFile(path)` | 同上 | `ProbeResult` | 轻量校验：只判「可读 + 首行 fs + 至少一行非空数据行」，不逐行解析、不检测坏行 |
| `probe(text)` | 同上 | `ProbeResult` | 同上（文本版，便于测试） |

> `probeFile/probe` 用于「导入文件夹」时快速筛选文件，不检测坏行；坏行与数值有效性在用户实际选中文件、执行完整解析（`readFile/parse`）时才判定。

`parse` 的三个失败分支（优先级自上而下）：

| 条件 | `error` 文本 |
| --- | --- |
| 文本为空（连一行都没有） | `empty file` |
| 首行不包含合法 `fs` | `missing or invalid fs metadata` |
| 数据区没有任何可解析为 `double` 的行 | `no valid data rows` |

**输入格式约定**

```
首行：键值对元数据，逗号或空格分隔，例如  fs=2560,N=4096
第 2 行起：每行一个 double 幅值
```

解析细则：

1. 元数据按 `,` 切段，段内再按空白切分 `token`，仅识别 `key=value` 形式。
2. 只读取键 `fs`；`N` 等其它键被忽略（有效点数由实际数据行决定，避免元数据与内容不一致）。
3. `fs` 的值必须被 `strtod` **完整消费**（`*end == '\0'`）；`fs=2560Hz` 视为非法 `fs`。
4. 数据区：先 `trim` 首尾空白；**空行直接忽略，不计入 `skippedLines`**。
5. 数据行必须被 `strtod` 完整消费且为有限值；`bad-line`、`1.0abc`、`nan`、`inf` 计入 `skippedLines`，
   并用前后有效样本**线性插值**填补（保持采样序列等间隔，时间轴不错位）。
6. 支持 `strtod` 可接受的十进制/科学计数法（如 `1.5`、`-2e-3`）。

**路径与编码注意**

- `path` 为 `std::string`，Windows 下由 `std::ifstream` 按 ANSI 代码页解释；含中文/非 ASCII 路径时，调用方应传本地编码字节串（Qt 侧使用 `QString::toLocal8Bit()`）。
- 小数点的字面形式依赖当前 C locale，默认 `C` locale 下为 `.`。

### 5.2 `Fft` — 基 2 快速傅里叶变换

```cpp
class Fft {
public:
    using Complex = std::complex<double>;
    static void forward(std::vector<Complex>& a);  // 原地正向变换
    static void inverse(std::vector<Complex>& a);  // 原地逆向变换（含 1/N 归一化）
};
```

| 项目 | 说明 |
| --- | --- |
| 算法 | 迭代实现（避免递归栈开销），位反转置换 + 蝶形运算，时域抽取 |
| 数据流 | **原地**修改入参，无返回值；结果通过 `a` 取出 |
| 长度要求 | `a.size()` 必须是 2 的整数次幂 |
| 边界行为 | `a.size() <= 1` 时立即返回（不修改、不报错） |
| 归一化 | `forward` 不缩放；`inverse` 乘以 `1/N`，故 `inverse(forward(x)) == x`（误差 ~1e-15 量级） |
| 非法长度 | 未定义行为（**不抛异常、不返回错误码**），调用方须先用 `SpectrumAnalyzer::nextPowerOfTwo()` 规整 |

### 5.3 `IWindow` / `RectWindow` / `HannWindow` — 窗函数

```cpp
class IWindow {
public:
    virtual ~IWindow() = default;
    virtual const char* name() const = 0;
    virtual void apply(std::vector<double>& samples) const = 0;
    virtual double correctionFactor() const = 0;
};
```

| 成员 | 语义 |
| --- | --- |
| `name()` | 唯一注册名，与 `WindowFactory::create()` 的键一致；返回静态字符串，生命周期为整个进程 |
| `apply(samples)` | 对序列**原地**加权，输出长度与输入一致 |
| `correctionFactor()` | 幅值校正因子；谱峰值 `×` 该因子恢复正弦真实幅值 |

内置实现：

| 类 | `name()` | 系数 | `apply` 行为 | `correctionFactor()` |
| --- | --- | --- | --- | --- |
| `RectWindow` | `"rect"` | `1.0`（恒等变换） | 不修改数据 | `1.0` |
| `HannWindow` | `"hann"` | `w[n] = 0.5·(1 − cos(2πn/(N−1)))`，`n = 0..N−1` | `samples[i] *= w[i]` | `2.0` |

`HannWindow` 边界：`N <= 1` 时直接返回，不修改数据，不产生除零。

**扩展方式**：继承 `IWindow` 实现 `name/apply/correctionFactor`，在 `WindowFactory.cpp` 的 `BuiltinWindowRegistrar` 中追加一行 `registerFactory`，或运行期通过 `registerFactory` 注入自定义工厂。无需修改 `SpectrumAnalyzer`。

### 5.4 `WindowFactory` / `IWindowFactory` — 窗函数工厂

```cpp
class IWindowFactory {
public:
    virtual ~IWindowFactory() = default;
    virtual std::unique_ptr<IWindow> createWindow() const = 0;   // 工厂方法
};

class WindowFactory {
public:
    static bool registerFactory(const std::string& name, std::unique_ptr<IWindowFactory> factory);
    static std::unique_ptr<IWindow> create(const std::string& name);
    static std::vector<std::string> names();
};
```

| 接口 | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `registerFactory(name, factory)` | `name`：注册名；`factory`：具体工厂，所有权转移给注册表 | `true` 成功；`false` = `factory` 为空 **或** 同名已存在 | **同名不覆盖**，保持注册表确定性 |
| `create(name)` | `name`：注册名，精确匹配（大小写敏感） | `std::unique_ptr<IWindow>`；未注册返回 `nullptr` | 每次调用返回**新实例**，调用方持有所有权 |
| `names()` | — | 已注册名称列表 | 顺序**未定义**（底层 `unordered_map`），界面填充下拉框时应自行排序 |

**注册表实现要点**

- 函数内静态对象（`registry()`），规避静态初始化顺序问题；C++11 起初始化线程安全。
- `create()` 返回 `nullptr` 而非抛异常，调用方必须判空（如 `SpectrumAnalyzer` 亦接受 `nullptr` 表示不加窗）。
- **线程安全边界**：启动阶段（`main` 之前/单线程阶段）注册，之后只读调用 `create/names` 是安全的；运行期并发 `registerFactory` 与其他访问不保证安全（`unordered_map` 无内部同步）。
- 内置 `"rect"`、`"hann"` 由 `WindowFactory.cpp` 内的静态 `BuiltinWindowRegistrar` 在 `main` 之前自动注册，调用方无需初始化。

### 5.5 `SpectrumAnalyzer` — 频谱分析

```cpp
struct Spectrum {
    std::vector<double> frequencies;
    std::vector<double> magnitudes;
};

class SpectrumAnalyzer {
public:
    static Spectrum amplitudeSpectrum(const SignalData& data, const IWindow* window);
    static Spectrum powerSpectrum(const SignalData& data, const IWindow* window);
    static std::vector<double> frequencyAxis(double fs, std::size_t fftSize);
    static std::size_t nextPowerOfTwo(std::size_t n);
};
```

| 接口 | 参数 | 返回值 | 说明 |
| --- | --- | --- | --- |
| `amplitudeSpectrum(data, window)` | `window` 可为 `nullptr`（等效矩形窗） | 线性单边幅值谱 | 主计算入口 |
| `powerSpectrum(data, window)` | 同上 | 单边自功率谱 | 由幅值谱逐点平方得到，单位为幅值² |
| `frequencyAxis(fs, fftSize)` | `fftSize > 0` | 长度 `fftSize/2 + 1` 的频率轴 | `f_k = k·fs/fftSize`；`fftSize==0` 会除零，属违约调用 |
| `nextPowerOfTwo(n)` | `n` 为点数 | 不小于 `n` 的最小 2 的整数次幂 | `n==0 → 1`；`n` 为 2 的幂时原样返回 |

**计算流程（`amplitudeSpectrum` / `powerSpectrum` 共用）**

1. `N_raw = data.size()`，`N_fft = nextPowerOfTwo(N_raw)`。
2. 拷贝数据到长度 `N_fft` 的复数缓冲，尾部补零（**零填充**，不改变采样率）。
3. 若 `window != nullptr`：仅对前 `N_raw` 个点加窗（**窗长为有效点数，而非补零后的 `N_fft`**），并取 `correction = window->correctionFactor()`。
4. `Fft::forward` 得到 `X[k]`。
5. 单边谱输出 `k = 0..N_fft/2`：

   \[
   A[k]=\begin{cases}\dfrac{|X[k]|}{N_{fft}}\cdot c, & k=0 \text{ 或 } k=N_{fft}/2\\[6pt] \dfrac{2|X[k]|}{N_{fft}}\cdot c, & \text{其它}\end{cases}
   \]

   其中 `c = correctionFactor()`（无窗时 `c = 1`）。
6. `powerSpectrum` 输出 `A[k]^2`；`amplitudeSpectrum` 直接输出 `A[k]`。
7. 频率轴由 `frequencyAxis(fs, N_fft)` 生成，保证与 `magnitudes` 等长、下标一一对应。

**输出保证**

- `magnitudes.size() == frequencies.size() == N_fft/2 + 1`，恒成立。
- `k = 0` 为直流分量（不做去直流、不做去趋势），`data.size()` 为偶数且等于 `N_fft` 时，最后一个点为 Nyquist 分量。
- 不修改 `data`（入参为 `const&`），可在多个线程对同一 `SignalData` 并发调用。

### 5.6 `RmsCalculator` — 有效值

```cpp
class RmsCalculator {
public:
    static double compute(const std::vector<double>& samples);  // 空序列返回 0
};
```

\[
\mathrm{RMS} = \sqrt{\frac{1}{N}\sum_{i=0}^{N-1} x_i^2}
\]

- 空序列返回 `0.0`，不报错。
- 纯函数、无状态；不修改入参。
- 对整周期正弦 `A·sin(...)`，结果约为 `A/√2`。

---

## 6. 错误处理汇总

| 场景 | 表现 | 调用方应对 |
| --- | --- | --- |
| 文件不存在/不可读 | `ReadResult::ok == false`，`error` 含路径 | 提示用户，跳过该文件 |
| 首行缺 `fs` 或 `fs` 非法 | `ok == false`，`error == "missing or invalid fs metadata"` | 判为无效文件 |
| 文件为空 | `ok == false`，`error == "empty file"` | 判为无效文件 |
| 无有效数据行 | `ok == false`，`error == "no valid data rows"` | 判为无效文件 |
| 存在坏行 | `ok == true`，`skippedLines > 0` | 坏行已线性插值填补，展示坏行数，不阻断流程 |
| 窗名称未注册 | `WindowFactory::create()` 返回 `nullptr` | 判空回退到 `nullptr`（不加窗）或提示 |
| 重复注册窗名称 | `registerFactory` 返回 `false`，原注册保留 | 视为可忽略的幂等失败 |
| `Fft` 传入非 2 幂长度 | 未定义行为（静默产生错误结果） | 必须先用 `nextPowerOfTwo()` 规整 |
| `frequencyAxis` 传入 `fftSize == 0` | 除零（`inf`/`nan`） | 保证 `fftSize >= 1` |
| `SignalData::at()` 越界 | 未定义行为（无检查） | 用 `size()` 约束索引 |

**库内不抛异常**，除 `std::vector` 扩容等内存失败的标准库异常外，接口不会主动 `throw`。

---

## 7. 线程安全模型

| 组件 | 线程安全 | 说明 |
| --- | --- | --- |
| `SignalData` | 只读并发安全 | 多线程并发读 `samples()`/`at()` 安全；写只能在构造时发生 |
| `SignalReader` | 是 | 无全局状态，局部变量 |
| `Fft` | 是 | 仅操作入参 |
| `SpectrumAnalyzer` | 是 | 无状态，结果由返回值持有 |
| `RmsCalculator` | 是 | 纯函数 |
| `IWindow` 实现 | 是 | `apply` 为 `const`；`HannWindow` 无成员状态 |
| `WindowFactory` | **部分** | 注册表初始化线程安全；`create/names` 只读并发安全；运行期并发写不安全 |

推荐用法：在后台工作线程中执行 `readFile → create(window) → amplitudeSpectrum → compute`，把结果整体回传主线程，不共享可变状态。

---

## 8. 使用示例

### 8.1 端到端：读取 → 加窗 → 幅值谱 → RMS

```cpp
#include "SignalCoreLib/SignalReader.h"
#include "SignalCoreLib/WindowFactory.h"
#include "SignalCoreLib/SpectrumAnalyzer.h"
#include "SignalCoreLib/RmsCalculator.h"

#include <cstdio>
#include <memory>

void analyze(const std::string& path) {
    using namespace SignalCoreLib;

    const ReadResult read = SignalReader::readFile(path);
    if (!read.ok) {
        std::fprintf(stderr, "读取失败：%s\n", read.error.c_str());
        return;
    }

    const SignalData& data = read.data;
    std::unique_ptr<IWindow> window = WindowFactory::create("hann"); // 未注册则为 nullptr

    const Spectrum spec = SpectrumAnalyzer::amplitudeSpectrum(data, window.get());
    const double rms = RmsCalculator::compute(data.samples());

    std::printf("fs=%.3f Hz, N=%zu, fftSize=%zu, bins=%zu, skipped=%zu, RMS=%.6f\n",
                data.sampleRate(), data.size(),
                SpectrumAnalyzer::nextPowerOfTwo(data.size()),
                spec.magnitudes.size(), read.skippedLines, rms);

    // 找谱峰（跳过 DC）
    std::size_t peak = 1;
    for (std::size_t k = 1; k < spec.magnitudes.size(); ++k) {
        if (spec.magnitudes[k] > spec.magnitudes[peak]) peak = k;
    }
    std::printf("谱峰：%.3f Hz, 幅值=%.6f\n", spec.frequencies[peak], spec.magnitudes[peak]);
}
```

### 8.2 自行构造数据（无需文件）

```cpp
#include "SignalCoreLib/SignalData.h"
#include "SignalCoreLib/SpectrumAnalyzer.h"

#include <cmath>
#include <vector>

constexpr double kPi = 3.14159265358979323846;

std::vector<double> sine(double fs, std::size_t n, double f0, double amp) {
    std::vector<double> s(n);
    for (std::size_t i = 0; i < n; ++i)
        s[i] = amp * std::sin(2.0 * kPi * f0 * static_cast<double>(i) / fs);
    return s;
}

// fs=256 Hz, N=64, f0=16 Hz, A=2 ⇒ 谱峰应落在 16 Hz、幅值 2.0
const SignalCoreLib::SignalData data(256.0, sine(256.0, 64, 16.0, 2.0));
const SignalCoreLib::Spectrum spec =
    SignalCoreLib::SpectrumAnalyzer::amplitudeSpectrum(data, nullptr);
```

### 8.3 文本解析（测试注入，避免真实文件）

```cpp
const std::string text = "fs=2560,N=4096\n1.0\n2.0\nbad-line\n4.0\n";
const SignalCoreLib::ReadResult r = SignalCoreLib::SignalReader::parse(text);
// r.ok == true, r.data.size() == 4, r.skippedLines == 1, r.data.sampleRate() == 2560
```

### 8.4 直接使用 FFT

```cpp
std::vector<SignalCoreLib::Fft::Complex> buf(64);
// ... 填充 buf ...
SignalCoreLib::Fft::forward(buf);   // 正向
SignalCoreLib::Fft::inverse(buf);   // 还原（含 1/N 归一化）
```

### 8.5 注册自定义窗函数

```cpp
class BlackmanWindow : public SignalCoreLib::IWindow {
public:
    const char* name() const override { return "blackman"; }
    void apply(std::vector<double>& x) const override {
        const std::size_t n = x.size();
        if (n <= 1) return;
        const double d = static_cast<double>(n - 1);
        for (std::size_t i = 0; i < n; ++i) {
            const double t = 2.0 * 3.14159265358979323846 * static_cast<double>(i) / d;
            x[i] *= 0.42 - 0.5 * std::cos(t) + 0.08 * std::cos(2.0 * t);
        }
    }
    double correctionFactor() const override { return 1.0 / 0.42; }
};

class BlackmanWindowFactory : public SignalCoreLib::IWindowFactory {
public:
    std::unique_ptr<SignalCoreLib::IWindow> createWindow() const override {
        return std::make_unique<BlackmanWindow>();
    }
};

// 单线程启动阶段注册
SignalCoreLib::WindowFactory::registerFactory(
    "blackman", std::make_unique<BlackmanWindowFactory>());
```

---

## 9. 集成与构建

### 9.1 CMake 集成

```cmake
add_subdirectory(SignalCoreLib)              # 或 find_package / install 后导入
target_link_libraries(MyApp PRIVATE SignalCoreLib)
```

- `target_include_directories` 已设为 `PUBLIC`，链接后可直接 `#include "SignalCoreLib/...h"`。
- 静态库默认 `POSITION_INDEPENDENT_CODE ON`，可被共享库/插件链接。
- 头文件与库均配置了 `install()` 规则（`include/SignalCoreLib` + `${CMAKE_INSTALL_LIBDIR}`）。

### 9.2 编译告警基线

| 编译器 | 选项 |
| --- | --- |
| MSVC | `/W4 /permissive- /utf-8`（源码含 UTF-8 中文注释，必须 `/utf-8`） |
| GCC/Clang | `-Wall -Wextra` |

### 9.3 验证

| 目标 | 说明 |
| --- | --- |
| `SignalCoreLibSmokeTest` | 控制台冒烟测试，逐项调用全部公开接口并校验结果（默认开启） |
| `SignalCoreLibTests` | GoogleTest 单元测试（默认关闭，`-DSIGNALCORELIB_BUILD_TESTS=ON` 开启） |

---

## 10. 已知限制与注意事项

1. **非 2 幂长度会低估幅值**：`N_raw` 非 2 的幂时按 `N_fft` 补零，而幅值归一化使用 `N_fft`，谱峰幅值被低估约 `N_raw/N_fft`。例如 1000 点补零到 1024，幅值偏低约 2.3%。需要精确幅值时，应保证有效点数为 2 的整数次幂（整周期采样）。
2. **不处理直流与趋势**：`amplitudeSpectrum` 不去均值、不去趋势、不做积分/微分类变换。
3. **幅值校正为相干增益校正**：`correctionFactor` 针对整周期采样正弦有效；非整周期信号存在频谱泄漏，峰值幅值只能近似恢复。
4. **窗长与补零**：加窗只作用于前 `N_raw` 个点，补零部分不参与窗系数计算（与 `N_fft` 长度窗不同）。
5. **`Fft` 长度不做运行期检查**：`isPowerOfTwo` 为私有辅助函数，`forward/inverse` 不会校验入参；传入非 2 幂长度将静默得出错误结果。
6. **`names()` 顺序不稳定**：来自 `unordered_map` 遍历，界面应显式排序后再展示。
7. **`fs` 未做正数校验**：`fs <= 0` 时 `SignalReader` 仍会成功返回，频率轴为 `0` 或负值，需调用方自行过滤。
8. **元数据 `N` 被忽略**：有效点数以实际数据行为准，不与首行 `N` 交叉校验。
9. **路径编码依赖平台**：Windows 下非 ASCII 路径需传本地编码字节串。
10. **`nextPowerOfTwo` 溢出**：极大 `n`（接近 `SIZE_MAX/2`）会移位溢出并可能死循环，实际文件规模下不构成风险。
