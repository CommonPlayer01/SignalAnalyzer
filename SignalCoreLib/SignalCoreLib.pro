# SignalCoreLib 静态库工程
# 核心算法库：FFT、窗函数、谱分析、有效值、文件读取。
# 硬性要求：不依赖任何 Qt 界面模块，可在工作线程中安全调用。

QT -= gui
CONFIG += staticlib
CONFIG += c++17
TEMPLATE = lib
TARGET = SignalCoreLib

# 仅依赖 Qt Core 之外的标准库；此处保持纯净，不链接 Qt。
# 说明：SignalCoreLib 设计上不 include 任何 Qt 头文件。
# 如编译器需要，可保持 QT = core（无 gui/widgets），但代码中不使用。

INCLUDEPATH += $$PWD/include

HEADERS += \
    $$PWD/include/SignalCoreLib/SignalData.h \
    $$PWD/include/SignalCoreLib/RmsCalculator.h \
    $$PWD/include/SignalCoreLib/Fft.h \
    $$PWD/include/SignalCoreLib/IWindow.h \
    $$PWD/include/SignalCoreLib/RectWindow.h \
    $$PWD/include/SignalCoreLib/HannWindow.h \
    $$PWD/include/SignalCoreLib/WindowFactory.h \
    $$PWD/include/SignalCoreLib/SpectrumAnalyzer.h \
    $$PWD/include/SignalCoreLib/SignalReader.h

SOURCES += \
    $$PWD/src/Fft.cpp \
    $$PWD/src/SpectrumAnalyzer.cpp \
    $$PWD/src/SignalReader.cpp \
    $$PWD/src/WindowFactory.cpp
