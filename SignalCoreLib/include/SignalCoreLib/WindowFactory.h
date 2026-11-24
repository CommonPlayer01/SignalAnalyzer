#pragma once

#include "SignalCoreLib/IWindow.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace SignalCoreLib {

// ============================================================================
// 工厂方法模式（Factory Method Pattern）
//
// 抽象工厂接口 IWindowFactory：声明创建窗函数的工厂方法 createWindow()。
// 具体工厂子类属于内部实现，定义在 WindowFactory.cpp 中，不对库外暴露。
//
// 新增窗函数：新增一个 IWindow 实现类，再在 WindowFactory.cpp 内新增一个
// 具体工厂子类并登记一处，即可被按名称创建；既有框架代码无需修改（开闭原则）。
//
// 公共面只有两个：
//   - IWindowFactory：自定义窗类型的注册扩展点（可继承后经 registerFactory 注册）；
//   - WindowFactory：按名称创建窗函数的门面（唯一对外入口）。
// ============================================================================

// 抽象工厂：窗函数的工厂方法接口（对外扩展点）。
class IWindowFactory {
public:
    virtual ~IWindowFactory() = default;

    // 工厂方法：创建对应类型的窗函数实例。
    virtual std::unique_ptr<IWindow> createWindow() const = 0;
};

// 窗函数注册表门面（唯一对外入口）。
// 持有「名称 → 具体工厂」映射，通过工厂方法模式创建窗实例。
class WindowFactory {
public:
    // 注册一个窗函数工厂；返回是否注册成功（同名重复注册返回 false）。
    static bool registerFactory(const std::string& name, std::unique_ptr<IWindowFactory> factory);

    // 创建指定名称的窗函数；名称未知时返回 nullptr。
    static std::unique_ptr<IWindow> create(const std::string& name);

    // 返回所有已注册窗函数的名称列表（供界面下拉框填充）。
    static std::vector<std::string> names();

private:
    static std::unordered_map<std::string, std::unique_ptr<IWindowFactory>>& registry();
};

} // namespace SignalCoreLib
