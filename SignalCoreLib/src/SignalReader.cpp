#include "SignalCoreLib/SignalReader.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cmath>

namespace SignalCoreLib {

namespace {

// 去除字符串首尾空白。
std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// 解析首行键值对元数据，返回 fs（采样率）；缺失或非法返回 nullopt。
// 元数据格式约定：逗号（或空格）分隔的键值对，如 "fs=2560,N=4096"。
std::optional<double> parseSampleRate(const std::string& line) {
    // 先按逗号切分，再对每段按 '=' 拆键值（兼容逗号与空格两种分隔）。
    std::stringstream ss(line);
    std::string segment;
    while (std::getline(ss, segment, ',')) {
        const std::string seg = trim(segment);
        if (seg.empty()) {
            continue;
        }
        // 段内可能仍是空格分隔的多个键值对，按空格再切一次。
        std::stringstream segss(seg);
        std::string token;
        while (segss >> token) {
            const auto eq = token.find('=');
            if (eq == std::string::npos) {
                continue;
            }
            const std::string key = trim(token.substr(0, eq));
            const std::string val = trim(token.substr(eq + 1));
            if (key == "fs") {
                char* end = nullptr;
                const double v = std::strtod(val.c_str(), &end);
                if (end != val.c_str() && *end == '\0') {
                    return v;
                }
                return std::nullopt; // fs 存在但非法
            }
        }
    }
    return std::nullopt; // 缺失 fs
}

} // namespace

ReadResult SignalReader::parse(const std::string& text) {
    ReadResult result;
    std::stringstream ss(text);
    std::string line;

    // 首行：元数据。
    if (!std::getline(ss, line)) {
        result.error = "empty file";
        return result;
    }
    const auto fs = parseSampleRate(line);
    if (!fs.has_value()) {
        result.error = "missing or invalid fs metadata";
        return result;
    }

    // 数据区：收集有效样本及其在"非空数据行"中的位置，坏行用前后有效样本线性插值填补。
    struct Sample { std::size_t index; double value; };
    std::vector<Sample> valid;     // 有效样本（index 为非空行序号，从 0 起）
    std::size_t rowCount = 0;      // 非空数据行总数（含坏行，不含空白行）
    std::size_t skipped = 0;       // 坏行数（无法解析或非有限值）

    while (std::getline(ss, line)) {
        const std::string t = trim(line);
        if (t.empty()) {
            continue; // 空白行忽略：不占位、不计数
        }
        char* end = nullptr;
        const double v = std::strtod(t.c_str(), &end);
        if (end == t.c_str() || *end != '\0' || !std::isfinite(v)) {
            ++skipped; // 坏行：占一个采样位置，稍后插值
        } else {
            valid.push_back(Sample{rowCount, v});
        }
        ++rowCount;
    }

    if (valid.empty()) {
        result.error = "no valid data rows";
        return result;
    }

    // 线性插值填补坏行，保持采样序列等间隔（时间轴不错位）。
    std::vector<double> samples(rowCount, 0.0);

    // 1) 有效样本填入对应位置。
    for (const Sample& s : valid) {
        samples[s.index] = s.value;
    }
    // 2) 首部坏行（第一个有效样本之前）：用第一个有效值填充。
    for (std::size_t i = 0; i < valid.front().index; ++i) {
        samples[i] = valid.front().value;
    }
    // 3) 尾部坏行（最后一个有效样本之后）：用最后一个有效值填充。
    for (std::size_t i = valid.back().index + 1; i < rowCount; ++i) {
        samples[i] = valid.back().value;
    }
    // 4) 有效样本之间的坏行：按位置比例线性插值。
    for (std::size_t k = 0; k + 1 < valid.size(); ++k) {
        const Sample& a = valid[k];
        const Sample& b = valid[k + 1];
        const std::size_t span = b.index - a.index; // >= 1
        for (std::size_t p = a.index + 1; p < b.index; ++p) {
            const double t = static_cast<double>(p - a.index) / static_cast<double>(span);
            samples[p] = a.value + (b.value - a.value) * t;
        }
    }

    result.ok = true;
    result.skippedLines = skipped;
    result.data = SignalData(fs.value(), std::move(samples));
    return result;
}

ReadResult SignalReader::readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        ReadResult result;
        result.error = "cannot open file: " + path;
        return result;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return parse(ss.str());
}

ProbeResult SignalReader::probe(const std::string& text) {
    ProbeResult result;
    std::stringstream ss(text);
    std::string line;

    if (!std::getline(ss, line)) {
        result.error = "empty file";
        return result;
    }
    if (!parseSampleRate(line).has_value()) {
        result.error = "missing or invalid fs metadata";
        return result;
    }
    // 只需存在至少一行非空数据行即视为可用；不验证数值、不检测坏行。
    while (std::getline(ss, line)) {
        if (!trim(line).empty()) {
            result.ok = true;
            return result;
        }
    }
    result.error = "no valid data rows";
    return result;
}

ProbeResult SignalReader::probeFile(const std::string& path) {
    ProbeResult result;
    std::ifstream in(path);
    if (!in.is_open()) {
        result.error = "cannot open file: " + path;
        return result;
    }
    // 流式读取：读到第一行非空数据行即返回，不读入整个文件、不构造样本、不检测坏行。
    std::string line;
    if (!std::getline(in, line)) {
        result.error = "empty file";
        return result;
    }
    if (!parseSampleRate(line).has_value()) {
        result.error = "missing or invalid fs metadata";
        return result;
    }
    while (std::getline(in, line)) {
        if (!trim(line).empty()) {
            result.ok = true;
            return result;
        }
    }
    result.error = "no valid data rows";
    return result;
}

} // namespace SignalCoreLib
