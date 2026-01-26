#include "EncryptProgressBar.h"
#include "ParameterValue.h"
#include "XFile.h"
#include "XExec.h"

#include <iostream>
#include <sstream>
#include <iomanip>

class EncryptProgressBar::PImpl
{
public:
    PImpl(EncryptProgressBar *owner);
    ~PImpl() = default;

public:
    /// 显示加密信息
    auto showEncryptionDetails(const std::string &key, const std::string &method, const std::string &iv = "",
                               bool useHmac = false) -> void;

public:
    EncryptProgressBar *owner_ = nullptr;
    std::string         encryptionKey_;
    std::string         encryptionMethod_;
    std::string         initializationVector_;
    bool                useHmac_ = false;
};

EncryptProgressBar::PImpl::PImpl(EncryptProgressBar *owner) : owner_(owner)
{
}

auto EncryptProgressBar::PImpl::showEncryptionDetails(const std::string &key, const std::string &method,
                                                      const std::string &iv, bool useHmac) -> void
{
    std::cout << "\n=== 加密配置信息 ===" << std::endl;
    std::cout << "加密方法: " << method << std::endl;
    std::cout << "加密密钥: " << key << std::endl;

    if (!iv.empty())
    {
        std::cout << "初始化向量: " << iv << std::endl;
    }

    if (useHmac)
    {
        std::cout << "HMAC验证: 启用" << std::endl;
    }

    /// 安全提示
    std::cout << "\n[安全提示]" << std::endl;
    std::cout << "1. 请妥善保存加密密钥" << std::endl;
    std::cout << "2. 解密时需要相同的密钥" << std::endl;
    std::cout << "3. 丢失密钥将无法恢复视频" << std::endl;
    std::cout << "========================\n" << std::endl;
}

EncryptProgressBar::EncryptProgressBar(const ProgressBarConfig::Ptr &config) :
    AVProgressBar(config), impl_(std::make_unique<PImpl>(this))
{
}

EncryptProgressBar::EncryptProgressBar(ProgressBarStyle style) :
    AVProgressBar(style), impl_(std::make_unique<PImpl>(this))
{
}

EncryptProgressBar::EncryptProgressBar(const std::string_view &configName) :
    AVProgressBar(configName), impl_(std::make_unique<PImpl>(this))
{
}

EncryptProgressBar::~EncryptProgressBar() = default;

auto EncryptProgressBar::updateProgress(XExec &exec, const std::string_view &taskName,
                                        const std::map<std::string, ParameterValue> &inputParams) -> void
{
    if (inputParams.contains("--input") && inputParams.contains("--output"))
    {
        auto srcPath = inputParams.at("--input").asString();
        auto dstPath = inputParams.at("--output").asString();

        /// 获取加密参数
        std::string key = inputParams.contains("--key") ? inputParams.at("--key").asString() : "自动生成";

        std::string method = inputParams.contains("--method") ? inputParams.at("--method").asString() : "AES-128-CBC";

        std::string iv = inputParams.contains("--iv") ? inputParams.at("--iv").asString() : "";

        bool useHmac = false;
        if (inputParams.contains("--hmac"))
        {
            std::string hmacVal = inputParams.at("--hmac").asString();
            std::ranges::transform(hmacVal, hmacVal.begin(), ::tolower);
            useHmac = (hmacVal == "true" || hmacVal == "1" || hmacVal == "yes" || hmacVal == "on");
        }

        /// 显示加密信息
        impl_->showEncryptionDetails(key, method, iv, useHmac);

        /// 保存加密信息
        impl_->encryptionKey_        = key;
        impl_->encryptionMethod_     = method;
        impl_->initializationVector_ = iv;
        impl_->useHmac_              = useHmac;

        /// 获取视频总时长
        double totalDuration = estimateTotalDuration(srcPath);

        /// 创建进度状态
        auto progressState = std::make_shared<AVProgressState>();
        setProgressState(progressState, 0.0, totalDuration, "");

        /// 开始进度监控
        startProgressMonitoring(exec, progressState, srcPath, dstPath);
    }
}

auto EncryptProgressBar::markAsCompleted(const std::string_view &message) -> void
{
    AVProgressBar::markAsCompleted(message);

    /// 显示解密说明
    showDecryptionInstructions();
}

auto EncryptProgressBar::markAsFailed(const std::string_view &message) -> void
{
    std::string finalMessage = "加密失败: " + std::string(message);
    AVProgressBar::markAsFailed(finalMessage);
}

auto EncryptProgressBar::showEncryptionInfo(const std::string &key, const std::string &method) -> void
{
    impl_->showEncryptionDetails(key, method);
}

auto EncryptProgressBar::showDecryptionInstructions() -> void
{
    std::cout << "\n=== 解密说明 ===" << std::endl;
    std::cout << "解密命令:" << std::endl;

    if (!impl_->encryptionKey_.empty() && impl_->encryptionKey_ != "自动生成")
    {
        std::cout << "ffmpeg -decryption_key " << impl_->encryptionKey_ << " -i encrypted_video.mp4 decrypted_video.mp4"
                  << std::endl;
    }
    else
    {
        std::cout << "请使用在加密过程中显示的密钥进行解密" << std::endl;
    }

    std::cout << "\n播放加密视频:" << std::endl;
    std::cout << "ffplay -decryption_key <密钥> encrypted_video.mp4" << std::endl;
    std::cout << "==================\n" << std::endl;
}
