#pragma once
#include "XConst.h"

// // 示例1：使用预设配置
// auto progressBar1 = std::make_unique<TaskProgressBar>(ProgressBarStyle::AVTask);
// // 或者
// auto progressBar2 = std::make_unique<TaskProgressBar>("cv");
//
// // 示例2：自定义配置
// auto config             = ProgressBarConfigManager::instance().createConfig("my-config");
// config->barWidth        = 60;
// config->foregroundColor = BarColor::Blue;
// config->backgroundColor = BarColor::White;
// config->bold            = true;
//
// auto progressBar3 = std::make_unique<TaskProgressBar>(config);
//
// // 示例3：运行时切换配置
// progressBar3->setConfig(otherConfig);
// progressBar3->applyConfig();
//
// // 示例4：保存/加载配置
// ProgressBarConfigManager::instance().saveToFile("progress-bars.json");
// ProgressBarConfigManager::instance().loadFromFile("progress-bars.json");

enum class ProgressBarStyle
{
    Default,
    AVTask,       ///< 视频转码任务
    AnalysisTask, ///< 分析任务
    DownloadTask, ///< 下载任务
    Custom        ///< 自定义
};

enum class BarColor
{
    Magenta, ///< 默认
    Cyan,    ///< CV任务
    Green,   ///< 分析任务
    Blue,    ///< 下载任务
    Red,     ///< 错误/警告
    Yellow,  ///< 警告/注意
    White,   ///< 普通
    Grey     ///< 禁用
};


struct ProgressBarConfig
{
    DECLARE_CREATE(ProgressBarConfig)
    /// 基本配置
    int         barWidth{ 50 };
    std::string startSymbol{ "[" };
    std::string fillSymbol{ "=" };
    std::string leadSymbol{ ">" };
    std::string remainderSymbol{ " " };
    std::string endSymbol{ "]" };

    /// 显示选项
    bool showPercentage{ true };
    bool showElapsedTime{ true };
    bool showRemainingTime{ true };
    bool showBar{ true };
    bool showTime{ true };

    /// 样式选项
    BarColor foregroundColor{ BarColor::Magenta };
    BarColor backgroundColor{ BarColor::Grey };
    bool     bold{ true };
    bool     blink{ false };

    /// 行为选项
    int  updateIntervalMs{ 100 }; /// 更新间隔
    bool hideCursor{ true };      /// 隐藏光标

    /// 主题相关
    ProgressBarStyle style{ ProgressBarStyle::Default };
    std::string      themeName{ "default" };

    /// 创建配置副本
    auto clone() const -> ProgressBarConfig::Ptr;
};
