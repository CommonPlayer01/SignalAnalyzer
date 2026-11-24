#pragma once

#include "SignalCoreLib/SignalData.h"

#include <string>
#include <optional>
#include <cstddef>

namespace SignalCoreLib {

// 文件读取结果：包含解析状态与（可选的）SignalData。
struct ReadResult {
    bool ok = false;          // 是否解析成功
    std::string error;        // 失败原因（ok=false 时有效）
    std::size_t skippedLines = 0; // 坏行数（无法解析或非有限值，已线性插值填补）
    SignalData data;          // 解析出的数据（ok=true 时有效）
};

// 轻量校验结果：只判断文件是否可用，不含数据、不统计坏行。
struct ProbeResult {
    bool ok = false;          // 是否可用
    std::string error;        // 不可用原因（ok=false 时有效）
};

// txt 信号文件读取器。
// 文件格式约定：首行为键值对元数据（如 fs=2560,N=4096），第二行起每行一个 double 幅值。
// 留白点处理：
//   - 缺失 fs → 判为无效（ok=false）；
//   - 空文件（无有效数据行）→ 判为无效；
//   - 坏行（无法解析为 double，或为 nan/inf）→ 用前后有效样本线性插值填补并计数，
//     保持采样序列等间隔（时间轴不错位）。
class SignalReader {
public:
    // 从文件路径读取（完整解析：逐行解析数值、坏行线性插值）。
    static ReadResult readFile(const std::string& path);

    // 从文本内容解析（便于测试注入，避免依赖真实文件）。
    static ReadResult parse(const std::string& text);

    // 轻量校验：只检查「可读 + 首行含合法 fs + 至少一行非空数据行」，
    // 不逐行解析数值、不检测坏行、不插值。用于导入文件夹时快速筛选，
    // 避免对所有文件逐行查坏行（坏行/有效性留到实际解析时由 readFile/parse 判定）。
    static ProbeResult probeFile(const std::string& path);
    static ProbeResult probe(const std::string& text);
};

} // namespace SignalCoreLib
