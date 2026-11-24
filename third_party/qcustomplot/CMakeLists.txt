# QCustomPlot 第三方库
# 直接以静态库方式编译 qcustomplot.cpp（自带 .h/.cpp，无需额外依赖）

add_library(qcustomplot STATIC
    qcustomplot.cpp
    qcustomplot.h
)

# qcustomplot 需要链接 Qt PrintSupport（QPrinter）
target_link_libraries(qcustomplot PUBLIC ${QT_PKG}::PrintSupport)

# 自动 MOC 处理 qcustomplot.h（含 Q_OBJECT）
set_target_properties(qcustomplot PROPERTIES AUTOMOC ON)
