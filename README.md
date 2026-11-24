# SignalAnalyzer (CMake 版)

基于原 Visual Studio 项目的核心代码文件，重新创建的使用 CMake 构建的信号分析器项目。
`SignalAnalyzer` 主程序依赖 `SignalCoreLib` 静态库完成 FFT、窗函数、幅值谱与有效值计算。

## 项目结构

```
SignalAnalyzer-cmake/
├── CMakeLists.txt                        # 顶层 CMake 配置（启用 CTest）
├── CMakePresets.json                     # vs142 预设（VS + MSVC v142）
├── SignalCoreLib/                        # 核心静态库（不依赖 Qt）
│   ├── CMakeLists.txt
│   ├── include/SignalCoreLib/*.h         # 公共 API
│   ├── src/*.cpp                         # Fft / SpectrumAnalyzer / SignalReader / WindowFactory
│   └── tests/                            # GoogleTest 单元测试 + 控制台冒烟测试
├── SignalAnalyzer/                       # 主程序（界面薄客户端）
│   ├── CMakeLists.txt
│   ├── main.cpp                          # 程序入口（与 CMakeLists.txt 同级）
│   ├── SignalAnalyzer.ui / .qrc
│   ├── include/                          # .h：SignalAnalyzer / SignalWorker / ComputeTask / ScanTask / PlotController / SignalTypes
│   └── src/                              # .cpp：SignalAnalyzer / SignalWorker / ComputeTask / ScanTask / PlotController
└── third_party/qcustomplot/              # QCustomPlot 2.1.1
```

## 依赖

- Visual Studio（本机为 VS 2026） + **MSVC v142 工具集**（`14.29.30133`）
- CMake >= 3.21（`CMakePresets.json` 使用 schema v3；本机为 4.5.0）
- Qt 5.15.2 `msvc2019_64`（Widgets + PrintSupport），路径 `D:/Software/Qt/5.15.2/msvc2019_64`
- 可选：GoogleTest（**默认不启用**，不需要联网；启用方法见“可选项”）

## 构建（Visual Studio + MSVC v142）

方式一：CMake 预设（推荐）

```powershell
cmake --preset vs142
cmake --build --preset vs142-release
```

方式二：手动指定生成器与工具集

```powershell
cmake -S . -B build-vs142 -G "Visual Studio 18 2026" -A x64 -T v142 `
      -DCMAKE_PREFIX_PATH="D:/Software/Qt/5.15.2/msvc2019_64"
cmake --build build-vs142 --config Release
```

生成后可直接用 Visual Studio 打开 `build-vs142/SignalAnalyzer.slnx`，
所有工程（`SignalAnalyzer`、`SignalCoreLib`、`qcustomplot`、测试目标）均为 `v142` 工具集。

> 在 VS 中从“配置/Configure”下拉框选择 **`vs142`** 预设即可。
> 不要使用 `Qt-Debug`/`Qt-Release` 预设：它们由 Qt 扩展生成，使用 Ninja 与 VS 默认工具集
> （本机为 v145），不满足 v142 要求。

> 如果使用其他 VS 版本，请把 `-G "Visual Studio 18 2026"` 换成对应生成器
> （如 `-G "Visual Studio 17 2022"`），`-T v142` 保持不变即可。

## 测试（验证 SignalCoreLib 调用）

```powershell
ctest --test-dir build-vs142 -C Release --output-on-failure
```

包含两类测试：

| 测试项 | 说明 |
| --- | --- |
| `SignalCoreLibSmokeTest` | 控制台程序，无 Qt/无第三方依赖，逐项调用 `Fft`、`WindowFactory`、`SpectrumAnalyzer`、`RmsCalculator`、`SignalReader` 并校验结果（默认开启） |
| `SignalCoreLibTests` | GoogleTest 单元测试（`SIGNALCORELIB_BUILD_TESTS=ON` 时生成） |

单独运行冒烟测试（返回 0 表示全部通过）：

```powershell
.\build-vs142\bin\Release\SignalCoreLibSmokeTest.exe
```

## 运行界面

需要把 Qt 运行库加入 `PATH`：

```powershell
$env:PATH = "D:\Software\Qt\5.15.2\msvc2019_64\bin;" + $env:PATH
.\build-vs142\bin\Release\SignalAnalyzer.exe
```

选择文件夹后导入全部 txt 文件；通过“文件选择 / 信号选择（时域、幅值谱、自功率谱）/
窗函数选择（矩形窗、汉宁窗）”联动绘制曲线。时域横轴为秒，频谱横轴为 Hz，坐标轴名称、
单位、图例齐全；状态栏显示采样率、有效点数/FFT 点数、RMS、谱峰与窗函数。

## 与 SignalCoreLib 的依赖关系

- 主程序 `target_link_libraries(SignalAnalyzer PRIVATE SignalCoreLib)`。
- 数据流（界面与算法解耦）：
  - `MainWindow`（`SignalAnalyzer`）仅负责交互，向 `SignalWorker` 发起请求；
  - `SignalWorker` 是 App 内调用 SignalCore 的唯一入口，在常驻后台线程中执行计算，
    并通过代际号（generation）丢弃过期结果；
  - `PlotController` 接收已算好的结果，降采样后绘制到 QCustomPlot。
- `ComputeTask` 实际调用：
  - `SignalCoreLib::SignalReader::readFile()` 读取解析文件；
  - `SignalCoreLib::WindowFactory::create("rect"/"hann")` 创建窗函数；
  - `SignalCoreLib::SpectrumAnalyzer::amplitudeSpectrum()/powerSpectrum()` 计算谱；
  - `SignalCoreLib::RmsCalculator::compute()` 计算有效值。

## 可选项

| CMake 变量 | 默认值 | 说明 |
| --- | --- | --- |
| `SIGNALCORELIB_BUILD_TESTS` | `OFF` | GoogleTest 单元测试，默认关闭，避免配置阶段联网 |
| `SIGNALCORELIB_GTEST_SOURCE_DIR` | 空 | 本地 googletest 源码目录，供离线启用单元测试 |
| `SIGNALCORELIB_BUILD_SMOKE_TEST` | `ON` | 控制台冒烟测试（离线可用，覆盖库函数调用验证） |

启用单元测试（三选一）：

```powershell
# 1) 系统/包管理器已安装 GoogleTest（find_package 优先）
cmake --preset vs142 -DSIGNALCORELIB_BUILD_TESTS=ON

# 2) 离线：指定本地 googletest 源码目录
cmake --preset vs142 -DSIGNALCORELIB_BUILD_TESTS=ON `
      -DSIGNALCORELIB_GTEST_SOURCE_DIR="C:/deps/googletest"

# 3) 允许联网：由 FetchContent 下载 v1.15.2
cmake --preset vs142 -DSIGNALCORELIB_BUILD_TESTS=ON
```

## 常见问题

- **`CMake Error ... FetchContent.cmake ... Build step for googletest failed: 1`**
  旧配置把单元测试默认设为 ON，配置阶段要联网下载 googletest，网络受限时会导致
  VS 配置/生成直接失败。现默认关闭；删除旧构建目录（如 `out/build`、`build-vs142`）
  后重新配置即可。需要单元测试时按上面的离线方式指定本地源码。
- **VS 中工具集不是 v142**（如 `14.52` / v145）：说明使用了 Qt 扩展生成的
  `Qt-Debug`/`Qt-Release` 预设，或打开了旧的构建目录。请改选 `vs142` 预设，
  删除旧构建目录后重新配置。

## 其他说明

- `SignalCoreLib` 为静态库，公共头文件位于 `include/SignalCoreLib/`。
- 主程序通过 `qt5_wrap_ui`/`qt5_add_resources` 处理 `.ui` 和 `.qrc`，`qcustomplot` 以静态库方式编译并链接 Qt PrintSupport。
- MSVC 编译时对源码启用 `/utf-8`，保证中文注释与中文字面量正确解析。
- MinGW 仍可构建：`cmake -S . -B build -G "MinGW Makefiles"`。
