#pragma once
#include "XConst.h"
#include "ISingleton.hpp"
#include "ProgressBarConfig.h"

#include <map>
#include <mutex>

class ProgressBarConfigManager : public ISingleton<ProgressBarConfigManager>
{
public:
    ProgressBarConfigManager();

    /// 禁用拷贝
    ProgressBarConfigManager(const ProgressBarConfigManager&)            = delete;
    ProgressBarConfigManager& operator=(const ProgressBarConfigManager&) = delete;

public:
    auto initPresets() -> void;

    /// 注册预设配置
    auto registerPreset(const std::string_view& name, const ProgressBarConfig::Ptr& config) -> void;
    auto registerPreset(ProgressBarStyle style, const ProgressBarConfig::Ptr& config) -> void;

    /// 获取配置
    auto getConfig(const std::string_view& name) -> ProgressBarConfig::Ptr;
    auto getConfig(ProgressBarStyle style) -> ProgressBarConfig::Ptr;

    /// 创建新配置
    auto createConfig(const std::string& name = "") const -> ProgressBarConfig::Ptr;

    /// 保存/加载配置到文件
    auto saveToFile(const std::string& filename) -> bool;
    auto loadFromFile(const std::string& filename) -> bool;

    /// 应用配置到进度条
    template <typename ProgressBarType>
    auto applyConfig(ProgressBarType& progressBar, const ProgressBarConfig::Ptr& config) -> void;

private:
    std::mutex                                         mutex_;
    std::map<std::string_view, ProgressBarConfig::Ptr> namedConfigs_;
    std::map<ProgressBarStyle, ProgressBarConfig::Ptr> styleConfigs_;
    ProgressBarConfig::Ptr                             defaultConfig_;
};
