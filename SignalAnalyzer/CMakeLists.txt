# SignalAnalyzer —— 主程序（可执行文件）
#
# 目录约定：
#   include/ —— 头文件
#   src/     —— 源文件
#   本目录   —— CMakeLists.txt 与 .ui / .qrc 资源

# ---- 生成 ui_MainWindow.h（由 .ui 文件）----
# .ui 引用了自定义控件 QCustomPlot，需要在 uic 阶段能找到 qcustomplot.h
set(UI_FILE ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow.ui)

# Qt5 用 qt5_wrap_ui，Qt6 用 qt6_wrap_ui（需额外指定 include 路径以解析自定义控件）
if(${QT_PKG} STREQUAL "Qt6")
    set(UI_HEADERS "")
    qt6_wrap_ui(UI_HEADERS ${UI_FILE})
else()
    set(UI_HEADERS "")
    qt5_wrap_ui(UI_HEADERS ${UI_FILE})
endif()

# ---- 生成 qrc_SignalAnalyzer.cpp（由 .qrc 文件）----
set(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/SignalAnalyzer.qrc)
if(${QT_PKG} STREQUAL "Qt6")
    set(QRC_SOURCES "")
    qt6_add_resources(QRC_SOURCES ${QRC_FILE})
else()
    set(QRC_SOURCES "")
    qt5_add_resources(QRC_SOURCES ${QRC_FILE})
endif()

# ---- 源文件 ----
set(ANALYZER_SOURCES
    main.cpp
    src/MainWindow.cpp
    include/MainWindow.h
    include/SignalTypes.h
    src/ComputeTask.cpp
    include/ComputeTask.h
    src/ScanTask.cpp
    include/ScanTask.h
    src/SignalWorker.cpp
    include/SignalWorker.h
    src/PlotController.cpp
    include/PlotController.h
    src/SelfTestDialog.cpp
    include/SelfTestDialog.h
    ${UI_HEADERS}
    ${QRC_SOURCES}
)

add_executable(SignalAnalyzer ${ANALYZER_SOURCES})

# 链接 Qt Widgets
target_link_libraries(SignalAnalyzer PRIVATE ${QT_PKG}::Widgets)

# 链接核心静态库
target_link_libraries(SignalAnalyzer PRIVATE SignalCoreLib)

# 链接 qcustomplot（在子目录中定义）
add_subdirectory(../third_party/qcustomplot qcustomplot_build)
target_link_libraries(SignalAnalyzer PRIVATE qcustomplot)

# 头文件搜索路径（本项目 include/ 与 qcustomplot.h）
target_include_directories(SignalAnalyzer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/qcustomplot
)

# AUTOMOC 需要能找到 ui_ 头文件
target_include_directories(SignalAnalyzer PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Windows GUI 子系统（无控制台窗口）
set_target_properties(SignalAnalyzer PROPERTIES WIN32_EXECUTABLE TRUE)

# 源码含中文注释与中文字面量：MSVC 需显式按 UTF-8 解析，避免按系统代码页(GBK)误读。
if(MSVC)
    target_compile_options(SignalAnalyzer PRIVATE /utf-8)
endif()