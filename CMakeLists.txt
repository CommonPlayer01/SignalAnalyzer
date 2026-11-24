cmake_minimum_required(VERSION 3.16)

project(SignalAnalyzer
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 禁用 CMake 内置的 AUTOUIC，由我们手动调用 uic/moc/rcc 精确控制
# （原因：.ui 中使用了自定义控件 QCustomPlot，AUTOUIC 需要额外 include 路径，手动方式更可控）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# 输出目录统一
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Qt 组件（Widgets + PrintSupport，PrintSupport 供 qcustomplot 使用）
find_package(Qt6 COMPONENTS Widgets PrintSupport QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets PrintSupport)
    set(QT_PKG Qt5)
else()
    set(QT_PKG Qt6)
endif()

# Windows 下禁用控制台窗口（GUI 应用）
if(WIN32)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

# MSVC 工具集提示：本项目按 MSVC v142（19.29）验证。
# 若使用 VS 生成器，请通过 -T v142 或 "vs142" 配置预设配置，
# 避免 VS 默认工具集（如 v143/v145）改变编译结果。
if(MSVC)
    message(STATUS "MSVC 编译器版本: ${CMAKE_CXX_COMPILER_VERSION}（要求 v142 / 19.29）")
    if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.30)
        message(WARNING
            "当前 MSVC 版本为 ${CMAKE_CXX_COMPILER_VERSION}，不是 v142(19.29)。"
            "如需使用 v142，请在 VS 中选择 \"vs142\" 配置预设，或使用 -T v142 重新配置。")
    endif()
endif()

# 测试：顶层启用 CTest（SignalCoreLib 冒烟测试与 SignalAnalyzer 自检均注册为测试项）
include(CTest)

# 子目录
add_subdirectory(SignalCoreLib)
add_subdirectory(SignalAnalyzer)
