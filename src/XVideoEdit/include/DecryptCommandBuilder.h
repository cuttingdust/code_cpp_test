#pragma once

#ifndef DECRYPT_COMMAND_BUILDER_H
#define DECRYPT_COMMAND_BUILDER_H

#include "AVTask.h"

class DecryptCommandBuilder : public AVTask::ICommandBuilder
{
    DECLARE_CREATE(DecryptCommandBuilder)

public:
    auto build(const std::map<std::string, ParameterValue> &params) const -> std::string override;
    auto validate(const std::map<std::string, ParameterValue> &params, std::string &errorMsg) const -> bool override;
    auto getTitle(const std::map<std::string, ParameterValue> &params) const -> std::string override;

private:
    struct DecryptOptions
    {
        std::string input;
        std::string output;
        std::string key;                       ///< 解密密钥（必需）
        std::string kid;                       ///< Key ID（可选）
        std::string iv;                        ///< 初始化向量（可选）
        std::string method   = "cenc-aes-ctr"; ///< 解密方法
        bool        use_hmac = false;          ///< 是否使用HMAC验证
        std::string hmac_key;                  ///< HMAC密钥（如果启用）
    };

    /// 支持的解密方法
    static const std::vector<std::string> SUPPORTED_CIPHERS;

    auto parseOptions(const std::map<std::string, ParameterValue> &params) const -> DecryptOptions;
    auto cleanHexString(const std::string &str) const -> std::string;
    auto validateKeyFormat(const std::string &key, const std::string &keyName, std::string &errorMsg) const -> bool;
    auto validateCipher(const std::string &cipher, std::string &errorMsg) const -> bool;
};

#endif // DECRYPT_COMMAND_BUILDER_H
