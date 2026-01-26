#include "DecryptProgressBar.h"
#include "ParameterValue.h"
#include "XFile.h"
#include "XExec.h"

#include <iostream>
#include <sstream>
#include <iomanip>

class DecryptProgressBar::PImpl
{
public:
    PImpl(DecryptProgressBar *owner);
    ~PImpl() = default;

public:
    /// 显示解密信息
    auto showDecryptionDetails(const std::string &key, const std::string &method, const std::string &kid = "",
                               const std::string &iv = "", bool useHmac = false) -> void;

public:
    DecryptProgressBar *owner_ = nullptr;
    std::string         decryptionKey_;
    std::string         decryptionMethod_;
    std::string         keyId_;
    std::string         initializationVector_;
    bool                useHmac_ = false;
};

DecryptProgressBar::PImpl::PImpl(DecryptProgressBar *owner) : owner_(owner)
{
}

auto DecryptProgressBar::PImpl::showDecryptionDetails(const std::string &key, const std::string &method,
                                                      const std::string &kid, const std::string &iv, bool useHmac)
        -> void
{
    std::cout << "\n=== 解密配置信息 ===" << std::endl;
    std::cout << "解密方法: " << method << std::endl;
    std::cout << "解密密钥: " << key << std::endl;

    if (!kid.empty())
    {
        std::cout << "Key ID: " << kid << std::endl;
    }

    if (!iv.empty())
    {
        std::cout << "初始化向量: " << iv << std::endl;
    }

    if (useHmac)
    {
        std::cout << "HMAC验证: 启用" << std::endl;
    }

    std::cout << "===================\n" << std::endl;
}

DecryptProgressBar::DecryptProgressBar(const ProgressBarConfig::Ptr &config) :
    AVProgressBar(config), impl_(std::make_unique<PImpl>(this))
{
}

DecryptProgressBar::DecryptProgressBar(ProgressBarStyle style) :
    AVProgressBar(style), impl_(std::make_unique<PImpl>(this))
{
}

DecryptProgressBar::DecryptProgressBar(const std::string_view &configName) :
    AVProgressBar(configName), impl_(std::make_unique<PImpl>(this))
{
}

DecryptProgressBar::~DecryptProgressBar() = default;

auto DecryptProgressBar::updateProgress(XExec &exec, const std::string_view &taskName,
                                        const std::map<std::string, ParameterValue> &inputParams) -> void
{
    if (inputParams.contains("--input") && inputParams.contains("--output"))
    {
        auto srcPath = inputParams.at("--input").asString();
        auto dstPath = inputParams.at("--output").asString();

        /// 获取解密参数
        std::string key = inputParams.contains("--key")
                ? inputParams.at("--key").asString()
                : (inputParams.contains("--password") ? inputParams.at("--password").asString() : "");

        std::string method = inputParams.contains("--method") ? inputParams.at("--method").asString() : "AES-128-CBC";

        std::string kid = inputParams.contains("--kid") ? inputParams.at("--kid").asString() : "";

        std::string iv = inputParams.contains("--iv") ? inputParams.at("--iv").asString() : "";

        bool useHmac = false;
        if (inputParams.contains("--hmac"))
        {
            std::string hmacVal = inputParams.at("--hmac").asString();
            std::ranges::transform(hmacVal, hmacVal.begin(), ::tolower);
            useHmac = (hmacVal == "true" || hmacVal == "1" || hmacVal == "yes" || hmacVal == "on");
        }

        /// 显示解密信息
        impl_->showDecryptionDetails(key, method, kid, iv, useHmac);

        /// 保存解密信息
        impl_->decryptionKey_        = key;
        impl_->decryptionMethod_     = method;
        impl_->keyId_                = kid;
        impl_->initializationVector_ = iv;
        impl_->useHmac_              = useHmac;

        /// 显示安全提醒
        showSecurityReminder();

        /// 获取视频总时长
        double totalDuration = estimateTotalDuration(srcPath);

        /// 创建进度状态
        auto progressState = std::make_shared<AVProgressState>();
        setProgressState(progressState, 0.0, totalDuration, "");

        /// 开始进度监控
        startProgressMonitoring(exec, progressState, srcPath, dstPath);
    }
}

auto DecryptProgressBar::markAsCompleted(const std::string_view &message) -> void
{
    std::string finalMessage = "解密完成: " + std::string(message);
    AVProgressBar::markAsCompleted(finalMessage);

    /// 显示成功信息
    std::cout << "\n✅ 解密成功！" << std::endl;
    std::cout << "解密后的视频已保存，可以正常播放。" << std::endl;
}

auto DecryptProgressBar::markAsFailed(const std::string_view &message) -> void
{
    std::string finalMessage = "解密失败: " + std::string(message);
    AVProgressBar::markAsFailed(finalMessage);

    /// 显示失败原因和可能的解决方案
    std::cout << "\n❌ 解密失败，可能的原因：" << std::endl;
    std::cout << "1. 密钥不正确" << std::endl;
    std::cout << "2. Key ID不匹配" << std::endl;
    std::cout << "3. 加密方法不匹配" << std::endl;
    std::cout << "4. 文件已损坏" << std::endl;
}

auto DecryptProgressBar::showDecryptionInfo(const std::string &key, const std::string &method) -> void
{
    impl_->showDecryptionDetails(key, method);
}

auto DecryptProgressBar::showSecurityReminder() -> void
{
    std::cout << "\n[安全提醒]" << std::endl;
    std::cout << "1. 请确保您有合法的解密权限" << std::endl;
    std::cout << "2. 解密完成后请妥善保管解密后的文件" << std::endl;
    std::cout << "3. 不要与他人分享解密密钥" << std::endl;
    std::cout << "4. 解密过程会验证密钥的正确性" << std::endl;
    std::cout << "========================\n" << std::endl;
}
