#include "ProgressBarConfigManager.h"
#include <indicators/progress_bar.hpp>
#include <nlohmann/json.hpp>
#include <fstream>

using namespace indicators;
using json = nlohmann::json;


class ProgressBarConfigManager::PImpl
{
public:
    PImpl(ProgressBarConfigManager* owenr);
    ~PImpl() = default;

public:
    auto initPresets() -> void;

    auto registerPreset(const std::string_view& name, const ProgressBarConfig::Ptr& config) -> void;

    auto registerPreset(const ProgressBarStyle& style, const ProgressBarConfig::Ptr& config) -> void;

    auto getConfig(const std::string_view& name) -> ProgressBarConfig::Ptr;

    auto getConfig(const ProgressBarStyle& style) -> ProgressBarConfig::Ptr;

public:
    ProgressBarConfigManager*                          owenr_ = nullptr;
    std::mutex                                         mtx_;
    std::map<std::string_view, ProgressBarConfig::Ptr> namedConfigs_;
    std::map<ProgressBarStyle, ProgressBarConfig::Ptr> styleConfigs_;
    ProgressBarConfig::Ptr                             defaultConfig_;
    std::string                                        barConfig_ = ".bar_config";
};

ProgressBarConfigManager::PImpl::PImpl(ProgressBarConfigManager* owenr) : owenr_(owenr)
{
    initPresets();
}

auto ProgressBarConfigManager::PImpl::initPresets() -> void
{
    /// 默认配置
    defaultConfig_ = ProgressBarConfig::create();
    registerPreset("default", defaultConfig_);
    registerPreset(ProgressBarStyle::AVTask, defaultConfig_);

    /// AV任务配置
    auto cvConfig             = defaultConfig_->clone();
    cvConfig->barWidth        = 60;
    cvConfig->foregroundColor = BarColor::Cyan;
    cvConfig->backgroundColor = BarColor::Grey;
    cvConfig->style           = ProgressBarStyle::AVTask;
    cvConfig->themeName       = "av";
    registerPreset("av", cvConfig);
    registerPreset(ProgressBarStyle::AVTask, cvConfig);

    // /// 分析任务配置
    // auto analysisConfig             = std::make_shared<ProgressBarConfig>();
    // analysisConfig->barWidth        = 50;
    // analysisConfig->foregroundColor = BarColor::Green;
    // analysisConfig->backgroundColor = BarColor::Grey;
    // analysisConfig->style           = ProgressBarStyle::AnalysisTask;
    // analysisConfig->themeName       = "analysis";
    // analysisConfig->showTime        = false; /// 分析任务可能不需要显示时间
    // registerPreset("analysis", analysisConfig);
    // registerPreset(ProgressBarStyle::AnalysisTask, analysisConfig);
    //
    // /// 下载任务配置
    // auto downloadConfig               = std::make_shared<ProgressBarConfig>();
    // downloadConfig->barWidth          = 55;
    // downloadConfig->foregroundColor   = BarColor::Blue;
    // downloadConfig->backgroundColor   = BarColor::Grey;
    // downloadConfig->style             = ProgressBarStyle::DownloadTask;
    // downloadConfig->themeName         = "download";
    // downloadConfig->showPercentage    = true;
    // downloadConfig->showElapsedTime   = true;
    // downloadConfig->showRemainingTime = true;
    // downloadConfig->showBar           = true;
    // registerPreset("download", downloadConfig);
    // registerPreset(ProgressBarStyle::DownloadTask, downloadConfig);

    /// 错误/警告配置
    auto errorConfig             = defaultConfig_->clone();
    errorConfig->barWidth        = 50;
    errorConfig->foregroundColor = BarColor::Red;
    errorConfig->backgroundColor = BarColor::Grey;
    errorConfig->style           = ProgressBarStyle::Custom;
    errorConfig->themeName       = "error";
    errorConfig->blink           = true; /// 错误时闪烁
    registerPreset("error", errorConfig);
}

auto ProgressBarConfigManager::PImpl::registerPreset(const std::string_view& name, const ProgressBarConfig::Ptr& config)
        -> void
{
    std::lock_guard<std::mutex> lock(mtx_);
    namedConfigs_[name] = config;
}

auto ProgressBarConfigManager::PImpl::registerPreset(const ProgressBarStyle&       style,
                                                     const ProgressBarConfig::Ptr& config) -> void
{
    std::lock_guard<std::mutex> lock(mtx_);
    styleConfigs_[style] = config;
}

auto ProgressBarConfigManager::PImpl::getConfig(const std::string_view& name) -> ProgressBarConfig::Ptr
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto                        it = namedConfigs_.find(name);
    if (it != namedConfigs_.end())
    {
        return it->second->clone(); /// 返回副本
    }
    return defaultConfig_->clone();
}

auto ProgressBarConfigManager::PImpl::getConfig(const ProgressBarStyle& style) -> ProgressBarConfig::Ptr
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto                        it = styleConfigs_.find(style);
    if (it != styleConfigs_.end())
    {
        return it->second->clone(); /// 返回副本
    }
    return defaultConfig_->clone();
}

ProgressBarConfigManager::ProgressBarConfigManager() : impl_(std::make_unique<ProgressBarConfigManager::PImpl>(this))
{
    loadFromFile(impl_->barConfig_);
}

ProgressBarConfigManager::~ProgressBarConfigManager()
{
    saveToFile(impl_->barConfig_);
}

auto ProgressBarConfigManager::registerPreset(const std::string_view& name, const ProgressBarConfig::Ptr& config)
        -> void
{
    impl_->registerPreset(name, config);
}

auto ProgressBarConfigManager::registerPreset(ProgressBarStyle style, const ProgressBarConfig::Ptr& config) -> void
{
    impl_->registerPreset(style, config);
}

auto ProgressBarConfigManager::getConfig(const std::string_view& name) -> ProgressBarConfig::Ptr
{
    return impl_->getConfig(name);
}

auto ProgressBarConfigManager::getConfig(const ProgressBarStyle& style) -> ProgressBarConfig::Ptr
{
    return impl_->getConfig(style);
}

auto ProgressBarConfigManager::createConfig(const std::string_view& name) const -> ProgressBarConfig::Ptr
{
    auto config = impl_->defaultConfig_->clone();
    if (!name.empty())
    {
        config->themeName = name;
    }
    return config;
}

auto ProgressBarConfigManager::saveToFile(const std::string_view& filename) -> bool
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    json j;

    /// 保存所有命名配置
    json namedConfigsJson = json::object();
    for (const auto& [name, config] : impl_->namedConfigs_)
    {
        json configJson;
        configJson["barWidth"]          = config->barWidth;
        configJson["startSymbol"]       = config->startSymbol;
        configJson["fillSymbol"]        = config->fillSymbol;
        configJson["leadSymbol"]        = config->leadSymbol;
        configJson["remainderSymbol"]   = config->remainderSymbol;
        configJson["endSymbol"]         = config->endSymbol;
        configJson["showPercentage"]    = config->showPercentage;
        configJson["showElapsedTime"]   = config->showElapsedTime;
        configJson["showRemainingTime"] = config->showRemainingTime;
        configJson["foregroundColor"]   = static_cast<int>(config->foregroundColor);
        configJson["backgroundColor"]   = static_cast<int>(config->backgroundColor);
        configJson["bold"]              = config->bold;
        configJson["blink"]             = config->blink;
        configJson["updateIntervalMs"]  = config->updateIntervalMs;
        configJson["hideCursor"]        = config->hideCursor;
        configJson["themeName"]         = config->themeName;

        namedConfigsJson[name] = configJson;
    }

    j["namedConfigs"] = namedConfigsJson;

    try
    {
        std::ofstream file(std::string{ filename });
        file << j.dump(4); /// 缩进4个空格，便于阅读
        return true;
    }
    catch (...)
    {
        return false;
    }
}

auto ProgressBarConfigManager::loadFromFile(const std::string_view& filename) -> bool
{
    std::lock_guard<std::mutex> lock(impl_->mtx_);

    try
    {
        std::ifstream file(std::string{ filename });
        if (!file.is_open())
        {
            return false;
        }

        json j;
        file >> j;

        // /// 清空现有配置
        // impl_->namedConfigs_.clear();

        /// 加载命名配置
        if (j.contains("namedConfigs"))
        {
            auto namedConfigsJson = j["namedConfigs"];
            for (auto& [name, configJson] : namedConfigsJson.items())
            {
                auto config = std::make_shared<ProgressBarConfig>();

                config->barWidth          = configJson.value("barWidth", 50);
                config->startSymbol       = configJson.value("startSymbol", "[");
                config->fillSymbol        = configJson.value("fillSymbol", "=");
                config->leadSymbol        = configJson.value("leadSymbol", ">");
                config->remainderSymbol   = configJson.value("remainderSymbol", " ");
                config->endSymbol         = configJson.value("endSymbol", "]");
                config->showPercentage    = configJson.value("showPercentage", true);
                config->showElapsedTime   = configJson.value("showElapsedTime", true);
                config->showRemainingTime = configJson.value("showRemainingTime", true);
                config->foregroundColor   = static_cast<BarColor>(configJson.value("foregroundColor", 0));
                config->backgroundColor   = static_cast<BarColor>(configJson.value("backgroundColor", 6));
                config->bold              = configJson.value("bold", true);
                config->blink             = configJson.value("blink", false);
                config->updateIntervalMs  = configJson.value("updateIntervalMs", 100);
                config->hideCursor        = configJson.value("hideCursor", true);
                config->themeName         = configJson.value("themeName", "");

                impl_->namedConfigs_[name] = config;
            }
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename ProgressBarType>
auto ProgressBarConfigManager::applyConfig(ProgressBarType& progressBar, const ProgressBarConfig::Ptr& config) -> void
{
    /// 设置基本样式
    progressBar.set_option(option::BarWidth{ config->barWidth });
    progressBar.set_option(option::Start{ config->startSymbol });
    progressBar.set_option(option::Fill{ config->fillSymbol });
    progressBar.set_option(option::Lead{ config->leadSymbol });
    progressBar.set_option(option::Remainder{ config->remainderSymbol });
    progressBar.set_option(option::End{ config->endSymbol });

    /// 设置显示选项
    progressBar.set_option(option::ShowPercentage{ config->showPercentage });
    progressBar.set_option(option::ShowElapsedTime{ config->showElapsedTime });
    progressBar.set_option(option::ShowRemainingTime{ config->showRemainingTime });

    /// 设置颜色
    auto toIndicatorColor = [](BarColor color) -> Color
    {
        switch (color)
        {
            case BarColor::Cyan:
                return Color::cyan;
            case BarColor::Green:
                return Color::green;
            case BarColor::Blue:
                return Color::blue;
            case BarColor::Red:
                return Color::red;
            case BarColor::Yellow:
                return Color::yellow;
            case BarColor::White:
                return Color::white;
            case BarColor::Grey:
                return Color::grey;
            case BarColor::Magenta:
            default:
                return Color::magenta;
        }
    };

    progressBar.set_option(option::ForegroundColor{ toIndicatorColor(config->foregroundColor) });

    /// 设置字体样式
    std::vector<FontStyle> fontStyles;
    if (config->bold)
    {
        fontStyles.push_back(FontStyle::bold);
    }
    if (config->blink)
    {
        fontStyles.push_back(FontStyle::blink);
    }
    if (!fontStyles.empty())
    {
        progressBar.set_option(option::FontStyles{ fontStyles });
    }
}

template auto ProgressBarConfigManager::applyConfig(indicators::ProgressBar&      progressBar,
                                                    const ProgressBarConfig::Ptr& config) -> void;
