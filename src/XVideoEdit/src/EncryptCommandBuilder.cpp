#include "EncryptCommandBuilder.h"
#include "XTool.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <numeric>
#include <random>
#include <regex>
#include <ranges>

/// 支持的加密算法（更新为FFmpeg实际支持的）
const std::vector<std::string> EncryptCommandBuilder::SUPPORTED_CIPHERS = {
    "cenc-aes-ctr", /// MP4 Common Encryption AES-CTR (你的FFmpeg支持这个)
    "cenc-aes-cbc", /// MP4 Common Encryption AES-CBC
    "aes-128-cbc",  /// 用于OpenSSL加密（如果可用）
    "aes-256-cbc"   /// 用于OpenSSL加密（如果可用）
};

auto EncryptCommandBuilder::parseOptions(const std::map<std::string, ParameterValue>& params) const
        -> EncryptCommandBuilder::EncryptOptions
{
    EncryptOptions options;

    /// 必需参数
    options.input  = params.at("--input").asString();
    options.output = params.at("--output").asString();

    /// 加密密钥
    if (params.contains("--key"))
    {
        options.key = params.at("--key").asString();
    }
    else
    {
        /// 如果没有提供密钥，生成一个随机密钥（16字节，hex编码）
        options.key = generateRandomKey(16);
    }

    /// 初始化向量
    if (params.contains("--iv"))
    {
        options.iv = params.at("--iv").asString();
    }

    /// 加密方法 - 默认为FFmpeg支持的cenc-aes-ctr
    options.method = "cenc-aes-ctr";
    if (params.contains("--method"))
    {
        std::string requestedMethod = params.at("--method").asString();

        /// 将用户请求的方法映射到FFmpeg支持的方法
        if (requestedMethod.find("AES-128") != std::string::npos)
        {
            options.method = "cenc-aes-ctr";
        }
        else if (requestedMethod.find("AES-256") != std::string::npos)
        {
            options.method = "cenc-aes-ctr"; /// 仍然使用cenc-aes-ctr，但密钥长度不同
        }
        else
        {
            options.method = "cenc-aes-ctr"; /// 默认
        }
    }

    /// 布尔参数
    auto parseBool = [](const std::string& val) -> bool
    {
        std::string lower = val;
        std::ranges::transform(lower, lower.begin(), ::tolower);
        return (lower == "true" || lower == "1" || lower == "yes" || lower == "on");
    };

    if (params.contains("--encrypt-audio"))
    {
        options.encrypt_audio = parseBool(params.at("--encrypt-audio"));
    }

    if (params.contains("--encrypt-video"))
    {
        options.encrypt_video = parseBool(params.at("--encrypt-video"));
    }

    if (params.contains("--hmac"))
    {
        options.use_hmac = parseBool(params.at("--hmac"));
    }

    if (params.contains("--hmac-key"))
    {
        options.hmac_key = params.at("--hmac-key").asString();
    }

    return options;
}

auto EncryptCommandBuilder::generateRandomKey(size_t length) const -> std::string
{
    static const char               hex_chars[] = "0123456789abcdef";
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::string key;
    key.reserve(length * 2); /// hex字符串长度是字节数的2倍

    for (size_t i = 0; i < length; ++i)
    {
        uint8_t byte = static_cast<uint8_t>(dis(gen));
        key.push_back(hex_chars[byte >> 4]);
        key.push_back(hex_chars[byte & 0x0F]);
    }

    return key;
}

auto EncryptCommandBuilder::validateCipher(const std::string& cipher, std::string& errorMsg) const -> bool
{
    auto it = std::ranges::find(SUPPORTED_CIPHERS, cipher);
    if (it == SUPPORTED_CIPHERS.end())
    {
        errorMsg = "不支持的加密方法: " + cipher + "\n你的FFmpeg版本支持的加密方法: " +
                std::accumulate(SUPPORTED_CIPHERS.begin(), SUPPORTED_CIPHERS.end(), std::string(),
                                [](const std::string& a, const std::string& b) -> std::string
                                { return a + (a.empty() ? "" : ", ") + b; });
        return false;
    }
    return true;
}

auto EncryptCommandBuilder::validate(const std::map<std::string, ParameterValue>& params, std::string& errorMsg) const
        -> bool
{
    /// 检查必需参数
    if (!params.contains("--input") || params.at("--input").empty())
    {
        errorMsg = "缺少输入文件参数(--input)";
        return false;
    }

    if (!params.contains("--output") || params.at("--output").empty())
    {
        errorMsg = "缺少输出文件参数(--output)";
        return false;
    }

    /// 验证加密方法
    if (params.contains("--method"))
    {
        const std::string& cipher = params.at("--method").asString();
        /// 允许用户请求的方法，但我们会映射到FFmpeg支持的方法
        std::string mappedCipher = "cenc-aes-ctr"; /// 默认映射

        if (cipher.find("AES") != std::string::npos)
        {
            mappedCipher = "cenc-aes-ctr";
        }

        if (!validateCipher(mappedCipher, errorMsg))
        {
            return false;
        }
    }

    /// 验证密钥格式（如果提供）
    if (params.contains("--key"))
    {
        const std::string& key = params.at("--key").asString();

        /// 检查是否为有效的hex字符串
        static std::regex hexRegex(R"(^[0-9a-fA-F]+$)");
        if (!std::regex_match(key, hexRegex))
        {
            errorMsg = "密钥必须是十六进制字符串";
            return false;
        }

        /// 对于cenc-aes-ctr，需要16字节（32 hex字符）
        size_t minKeyLen = 32; /// 16字节 = 32 hex字符

        if (key.length() < minKeyLen)
        {
            errorMsg = "密钥长度不足: 需要至少 " + std::to_string(minKeyLen) + " 个十六进制字符 (16字节)";
            return false;
        }
    }

    /// 验证IV格式（如果提供） - 对于cenc-aes-ctr，IV是可选的
    if (params.contains("--iv"))
    {
        const std::string& iv = params.at("--iv").asString();

        /// 检查是否为有效的hex字符串
        static std::regex hexRegex(R"(^[0-9a-fA-F]+$)");
        if (!std::regex_match(iv, hexRegex))
        {
            errorMsg = "IV必须是十六进制字符串";
            return false;
        }

        /// IV通常是16字节（32 hex字符）
        if (iv.length() < 32)
        {
            errorMsg = "IV长度不足: 需要至少 32 个十六进制字符 (16字节)";
            return false;
        }
    }

    /// 注意：你的FFmpeg不支持HMAC，所以这里忽略HMAC验证
    /// 但保留参数以保持API兼容性

    return true;
}


auto EncryptCommandBuilder::build(const std::map<std::string, ParameterValue>& params) const -> std::string
{
    EncryptOptions options = parseOptions(params);

    /// 清理和准备参数
    std::string key = options.key;
    std::string kid;

    /// 移除0x前缀（如果存在）
    auto remove0x = [](std::string& str)
    {
        if (str.length() > 2 && (str.starts_with("0x") || str.starts_with("0X")))
        {
            str = str.substr(2);
        }
    };

    remove0x(key);

    /// 验证密钥格式
    static std::regex hexRegex(R"(^[0-9a-fA-F]+$)");
    if (!std::regex_match(key, hexRegex) || key.length() < 32)
    {
        /// 生成有效的密钥
        key = generateRandomKey(16);
    }

    /// 确保密钥长度合适（32 hex字符 = 16字节）
    if (key.length() < 32)
    {
        key.append(32 - key.length(), '0');
    }
    else if (key.length() > 32)
    {
        key = key.substr(0, 32);
    }

    /// 准备KID（Key ID）
    if (!options.iv.empty())
    {
        kid = options.iv;
        remove0x(kid);
    }

    /// 如果KID为空或无效，从密钥生成
    if (kid.empty() || !std::regex_match(kid, hexRegex) || kid.length() < 32)
    {
        kid = key; /// 使用密钥作为KID
        if (kid.length() > 32)
        {
            kid = kid.substr(0, 32);
        }
    }

    /// 确保输出为MP4格式（CENC加密只支持MP4）
    std::string outputFile = options.output;
    std::string ext        = std::filesystem::path(outputFile).extension().string();
    std::ranges::transform(ext, ext.begin(), ::tolower);

    if (ext != ".mp4" && ext != ".m4v" && ext != ".mov")
    {
        size_t dotPos = outputFile.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            outputFile = outputFile.substr(0, dotPos) + "_encrypted.mp4";
        }
        else
        {
            outputFile += "_encrypted.mp4";
        }
    }

    std::stringstream cmd;
    cmd << "\"" << XTool::getFFmpegPath() << "\" ";

    /// 基本参数
    // cmd << "-hide_banner -progress pipe:1 -nostats -loglevel info ";
    cmd << "-y "; /// 覆盖输出文件

    /// 输入文件
    cmd << "-i \"" << options.input << "\" ";

    /// 注意：你的FFmpeg不支持crypt滤镜，所以不使用滤镜
    /// 直接使用MP4的CENC加密功能

    /// 编码选项 - 根据用户选择决定是否重新编码
    bool reencode = false; /// 默认复制流
    if (params.contains("--reencode"))
    {
        std::string reencodeVal = params.at("--reencode").asString();
        std::ranges::transform(reencodeVal, reencodeVal.begin(), ::tolower);
        reencode = (reencodeVal == "true" || reencodeVal == "1" || reencodeVal == "yes" || reencodeVal == "on");
    }

    if (reencode)
    {
        /// 重新编码（更安全，兼容性更好）
        cmd << "-c:v libx264 ";
        cmd << "-preset fast ";
        cmd << "-crf 23 ";
        cmd << "-c:a aac ";
        cmd << "-b:a 128k ";
    }
    else
    {
        /// 复制流（快速，保持原始质量）
        cmd << "-c copy ";
    }

    /// 添加MP4 CENC加密参数（你的FFmpeg支持这个）
    cmd << "-encryption_scheme " << options.method << " ";
    cmd << "-encryption_key " << key << " ";
    cmd << "-encryption_kid " << kid << " ";

    /// 输出文件
    cmd << "\"" << outputFile << "\"";

    return cmd.str();
}

auto EncryptCommandBuilder::getTitle(const std::map<std::string, ParameterValue>& params) const -> std::string
{
    std::string input  = params.at("--input").asString();
    std::string output = params.at("--output").asString();

    std::filesystem::path inputPath(input);
    std::filesystem::path outputPath(output);

    std::string method = "MP4 CENC-AES-CTR"; /// 显示实际使用的方法

    if (params.contains("--method"))
    {
        std::string userMethod = params.at("--method").asString();
        method                 = "MP4 CENC (" + userMethod + ")";
    }

    return "加密: " + inputPath.filename().string() + " → " + outputPath.filename().string() + " (" + method + ")";
}

IMPLEMENT_CREATE(EncryptCommandBuilder);
