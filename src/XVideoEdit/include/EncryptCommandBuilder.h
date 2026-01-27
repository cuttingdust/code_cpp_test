#pragma once

#ifndef ENCRYPT_COMMAND_BUILDER_H
#define ENCRYPT_COMMAND_BUILDER_H

#include "AVTask.h"
#include <filesystem>

class EncryptCommandBuilder : public AVTask::ICommandBuilder
{
    DECLARE_CREATE(EncryptCommandBuilder)

public:
    auto build(const std::map<std::string, ParameterValue> &params) const -> std::string override;
    auto validate(const std::map<std::string, ParameterValue> &params, std::string &errorMsg) const -> bool override;
    auto getTitle(const std::map<std::string, ParameterValue> &params) const -> std::string override;

private:
    struct EncryptOptions
    {
        std::string input;
        std::string output;
        std::string key;                           ///< 加密密钥
        std::string iv;                            ///< 初始化向量（可选）
        std::string method        = "AES-128-CBC"; ///< 加密方法
        bool        encrypt_audio = true;          ///< 是否加密音频
        bool        encrypt_video = true;          ///< 是否加密视频
        bool        use_hmac      = false;         ///< 是否使用HMAC验证
        std::string hmac_key;                      ///< HMAC密钥（如果启用）
        std::string keyfile;                       ///< 密钥保存文件路径（可选）
        std::string kid;                           ///< Key ID（可选）
    };

    /// 支持的加密方法
    static const std::vector<std::string> SUPPORTED_CIPHERS;

    auto parseOptions(const std::map<std::string, ParameterValue> &params) const -> EncryptOptions;
    auto generateRandomKey(size_t length) const -> std::string;
    auto validateCipher(const std::string &cipher, std::string &errorMsg) const -> bool;

    /// 保存密钥到文件
    auto saveKeyToFile(const std::string &key, const std::string &kid, const std::string &method,
                       const std::string &keyfile) const -> bool;

    /// 生成密钥文件名（如果未指定）
    auto generateKeyFileName(const std::string &outputFile) const -> std::string;

    /// 清理十六进制字符串
    auto cleanHexString(const std::string &str) const -> std::string;

    /// 验证密钥格式
    auto validateKeyFormat(const std::string &key, const std::string &keyName, std::string &errorMsg) const -> bool;
};

#endif // ENCRYPT_COMMAND_BUILDER_H
