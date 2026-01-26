#include "CVProgressBar.h"
#include "ParameterValue.h"
#include "XFile.h"
#include "XExec.h"
#include "XTool.h"

#include <iostream>

class CVProgressBar::PImpl
{
public:
    PImpl(CVProgressBar *owner);
    ~PImpl() = default;
};

CVProgressBar::PImpl::PImpl(CVProgressBar *owner)
{
    // 空的，因为功能都在基类
}

CVProgressBar::CVProgressBar(const ProgressBarConfig::Ptr &config) :
    AVProgressBar(config), impl_(std::make_unique<PImpl>(this))
{
}

CVProgressBar::CVProgressBar(ProgressBarStyle style) : AVProgressBar(style), impl_(std::make_unique<PImpl>(this))
{
}

CVProgressBar::CVProgressBar(const std::string_view &configName) :
    AVProgressBar(configName), impl_(std::make_unique<PImpl>(this))
{
}

CVProgressBar::~CVProgressBar() = default;

auto CVProgressBar::updateProgress(XExec &exec, const std::string_view &taskName,
                                   const std::map<std::string, ParameterValue> &inputParams) -> void
{
    if (inputParams.contains("--input") && inputParams.contains("--output"))
    {
        auto srcPath = inputParams.at("--input").asString();
        auto dstPath = inputParams.at("--output").asString();

        /// 获取视频总时长
        double totalDuration = estimateTotalDuration(srcPath);

        /// 创建进度状态
        auto progressState = std::make_shared<AVProgressState>();
        setProgressState(progressState, 0.0, totalDuration, "");

        /// 开始进度监控
        startProgressMonitoring(exec, progressState, srcPath, dstPath);

        /// 标记完成
        markAsCompleted("转码完成 ✓");
    }
    else
    {
        /// 参数不完整，调用基类处理
        AVProgressBar::updateProgress(exec, taskName, inputParams);
    }
}
