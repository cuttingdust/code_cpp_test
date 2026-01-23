#pragma once
#include "XConst.h"
#include "ISingleton.hpp"
#include "ProgressBarConfig.h"

class ProgressBarConfigManager : public ISingleton<ProgressBarConfigManager>
{
public:
    ProgressBarConfigManager();
    ~ProgressBarConfigManager() override;
    /// 禁用拷贝
    ProgressBarConfigManager(const ProgressBarConfigManager&)            = delete;
    ProgressBarConfigManager& operator=(const ProgressBarConfigManager&) = delete;

public:
    /// 注册预设配置
    auto registerPreset(const std::string_view& name, const ProgressBarConfig::Ptr& config) -> void;
    auto registerPreset(ProgressBarStyle style, const ProgressBarConfig::Ptr& config) -> void;

    /// 获取配置
    auto getConfig(const std::string_view& name) -> ProgressBarConfig::Ptr;
    auto getConfig(const ProgressBarStyle& style) -> ProgressBarConfig::Ptr;

    /// 创建新配置
    auto createConfig(const std::string_view& name = "") const -> ProgressBarConfig::Ptr;

    /// 保存/加载配置到文件
    auto saveToFile(const std::string_view& filename) -> bool;
    auto loadFromFile(const std::string_view& filename) -> bool;

    /// 应用配置到进度条
    template <typename ProgressBarType>
    auto applyConfig(ProgressBarType& progressBar, const ProgressBarConfig::Ptr& config) -> void;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};
