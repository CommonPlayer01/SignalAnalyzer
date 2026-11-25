#include "SignalCoreLib/WindowFactory.h"
#include "SignalCoreLib/IWindow.h"

namespace SignalCoreLib {

namespace {

// ===== 具体工厂：工厂方法实现（内部实现，不对外暴露） =====

class RectWindowFactory : public IWindowFactory {
public:
    std::unique_ptr<IWindow> createWindow() const override {
        return std::make_unique<RectWindow>();
    }
};

class HannWindowFactory : public IWindowFactory {
public:
    std::unique_ptr<IWindow> createWindow() const override {
        return std::make_unique<HannWindow>();
    }
};

} // namespace

// ===== 注册表（函数内静态，避免静态初始化顺序问题） =====

std::unordered_map<std::string, std::unique_ptr<IWindowFactory>>& WindowFactory::registry() {
    static std::unordered_map<std::string, std::unique_ptr<IWindowFactory>> reg;
    return reg;
}

bool WindowFactory::registerFactory(const std::string& name, std::unique_ptr<IWindowFactory> factory) {
    if (!factory) {
        return false;
    }
    auto& reg = registry();
    // 同名不覆盖，保持注册表确定性。
    if (reg.find(name) != reg.end()) {
        return false;
    }
    reg[name] = std::move(factory);
    return true;
}

std::unique_ptr<IWindow> WindowFactory::create(const std::string& name) {
    auto& reg = registry();
    const auto it = reg.find(name);
    if (it == reg.end()) {
        return nullptr;
    }
    return it->second->createWindow();
}

std::vector<std::string> WindowFactory::names() {
    std::vector<std::string> result;
    const auto& reg = registry();
    result.reserve(reg.size());
    for (const auto& kv : reg) {
        result.push_back(kv.first);
    }
    return result;
}

// ===== 内置窗函数自动注册 =====
//
// 通过静态对象在程序启动时自动注册内置窗类型。
// 扩展点：新增窗函数时，在此追加一行 registerFactory 调用即可，
// 无需修改 WindowFactory 的创建逻辑。
namespace {

struct BuiltinWindowRegistrar {
    BuiltinWindowRegistrar() {
        WindowFactory::registerFactory("rect", std::make_unique<RectWindowFactory>());
        WindowFactory::registerFactory("hann", std::make_unique<HannWindowFactory>());
        // 扩展点：新增窗函数在此登记。
    }
};

// 静态实例：构造时完成内置窗的自动注册。
const BuiltinWindowRegistrar g_builtinWindowRegistrar;

} // namespace

} // namespace SignalCoreLib