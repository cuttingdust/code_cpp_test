#include "DecryptCommandBuilder.h"
#include "XTool.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <numeric>
#include <regex>
#include <ranges>

/// 支持的解密算法
const std::vector<std::string> DecryptCommandBuilder::SUPPORTED_CIPHERS = {
    "cenc-aes-ctr", /// MP4 Common Encryption AES-CTR
    "cenc-aes-cbc", /// MP4 Common Encryption AES-CBC
    "aes-128-cbc",  /// 用于OpenSSL解密
    "aes-256-cbc"   /// 用于OpenSSL解密
};

auto DecryptCommandBuilder::parseOptions(const std::map<std::string, ParameterValue>& params) const
        -> DecryptCommandBuilder::DecryptOptions
{
    DecryptOptions options;

    /// 必需参数
    options.input  = params.at("--input").asString();
    options.output = params.at("--output").asString();

    /// 解密密钥（必需）
    if (params.contains("--key"))
    {
        options.key = params.at("--key").asString();
    }
    else if (params.contains("--password")) /// 兼容密码参数
    {
        options.key = params.at("--password").asString();
    }

    /// Key ID（可选）
    if (params.contains("--kid"))
    {
        options.kid = params.at("--kid").asString();
    }

    /// 初始化向量（可选）
    if (params.contains("--iv"))
    {
        options.iv = params.at("--iv").asString();
    }

    /// 解密方法 - 默认为cenc-aes-ctr
    options.method = "cenc-aes-ctr";
    if (params.contains("--method"))
    {
        std::string requestedMethod = params.at("--method").asString();

        /// 标准化方法名
        if (requestedMethod.find("AES-128") != std::string::npos)
        {
            options.method = "cenc-aes-ctr";
        }
        else if (requestedMethod.find("AES-256") != std::string::npos)
        {
            options.method = "cenc-aes-ctr";
        }
        else if (requestedMethod.find("cenc") != std::string::npos)
        {
            options.method = "cenc-aes-ctr";
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

auto DecryptCommandBuilder::cleanHexString(const std::string& str) const -> std::string
{
    std::string result;

    /// 移除0x前缀
    std::string temp = str;
    if (temp.length() > 2 && (temp.starts_with("0x") || temp.starts_with("0X")))
    {
        temp = temp.substr(2);
    }

    /// 移除所有非十六进制字符
    std::regex hexRegex(R"([^0-9a-fA-F])");
    result = std::regex_replace(temp, hexRegex, "");

    /// 转换为小写
    std::ranges::transform(result, result.begin(), ::tolower);

    return result;
}

auto DecryptCommandBuilder::validateKeyFormat(const std::string& key, const std::string& keyName,
                                              std::string& errorMsg) const -> bool
{
    if (key.empty())
    {
        errorMsg = keyName + "不能为空";
        return false;
    }

    /// 检查是否为有效的hex字符串
    static std::regex hexRegex(R"(^[0-9a-fA-F]+$)");
    if (!std::regex_match(key, hexRegex))
    {
        errorMsg = keyName + "必须是十六进制字符串（只包含0-9, a-f, A-F）";
        return false;
    }

    /// 对于AES加密，需要特定长度
    if (key.length() < 32) /// 16字节 = 32 hex字符
    {
        errorMsg = keyName + "长度不足: 需要至少 32 个十六进制字符 (16字节)，当前长度: " + std::to_string(key.length());
        return false;
    }

    return true;
}

auto DecryptCommandBuilder::validateCipher(const std::string& cipher, std::string& errorMsg) const -> bool
{
    auto it = std::ranges::find(SUPPORTED_CIPHERS, cipher);
    if (it == SUPPORTED_CIPHERS.end())
    {
        errorMsg = "不支持的解密方法: " + cipher + "\n支持的解密方法: " +
                std::accumulate(SUPPORTED_CIPHERS.begin(), SUPPORTED_CIPHERS.end(), std::string(),
                                [](const std::string& a, const std::string& b) -> std::string
                                { return a + (a.empty() ? "" : ", ") + b; });
        return false;
    }
    return true;
}

auto DecryptCommandBuilder::validate(const std::map<std::string, ParameterValue>& params, std::string& errorMsg) const
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

    /// 检查解密密钥
    if (!params.contains("--key") && !params.contains("--password"))
    {
        errorMsg = "需要解密密钥(--key)或密码(--password)";
        return false;
    }

    /// 获取并验证密钥
    std::string key;
    if (params.contains("--key"))
    {
        key = params.at("--key").asString();
    }
    else
    {
        key = params.at("--password").asString();
    }

    /// 清理密钥字符串
    key = cleanHexString(key);

    if (!validateKeyFormat(key, "解密密钥", errorMsg))
    {
        return false;
    }

    /// 验证解密方法
    std::string method = "cenc-aes-ctr";
    if (params.contains("--method"))
    {
        std::string requestedMethod = params.at("--method").asString();

        /// 标准化方法名
        if (requestedMethod.find("AES") != std::string::npos)
        {
            method = "cenc-aes-ctr";
        }

        if (!validateCipher(method, errorMsg))
        {
            return false;
        }
    }

    /// 验证IV格式（如果提供）
    if (params.contains("--iv"))
    {
        std::string iv = cleanHexString(params.at("--iv").asString());
        if (!validateKeyFormat(iv, "初始化向量(IV)", errorMsg))
        {
            return false;
        }
    }

    /// 验证KID格式（如果提供）
    if (params.contains("--kid"))
    {
        std::string kid = cleanHexString(params.at("--kid").asString());
        if (!validateKeyFormat(kid, "Key ID(KID)", errorMsg))
        {
            return false;
        }
    }

    /// 检查输入文件是否存在
    std::string inputFile = params.at("--input").asString();
    if (!std::filesystem::exists(inputFile))
    {
        errorMsg = "输入文件不存在: " + inputFile;
        return false;
    }

    /// 检查文件是否可能已加密（简单检查）
    try
    {
        auto fileSize = std::filesystem::file_size(inputFile);
        if (fileSize == 0)
        {
            errorMsg = "输入文件为空: " + inputFile;
            return false;
        }

        /// 检查是否为MP4格式（CENC加密主要支持MP4）
        std::string ext = std::filesystem::path(inputFile).extension().string();
        std::ranges::transform(ext, ext.begin(), ::tolower);

        if (ext != ".mp4" && ext != ".m4v" && ext != ".mov")
        {
            std::cout << "警告: 输入文件可能不是MP4格式，CENC解密主要支持MP4文件" << std::endl;
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        errorMsg = "无法访问输入文件: " + std::string(e.what());
        return false;
    }

    return true;
}

auto DecryptCommandBuilder::build(const std::map<std::string, ParameterValue>& params) const -> std::string
{
    DecryptOptions options = parseOptions(params);

    /// 清理和准备参数
    std::string key = cleanHexString(options.key);
    std::string kid;
    std::string iv;

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
    if (!options.kid.empty())
    {
        kid = cleanHexString(options.kid);
        if (kid.length() < 32)
        {
            kid.append(32 - kid.length(), '0');
        }
        else if (kid.length() > 32)
        {
            kid = kid.substr(0, 32);
        }
    }
    else
    {
        /// 如果没有提供KID，使用默认值
        /// 注意：实际解密时需要正确的KID，这里只是示例
        kid = key.substr(0, 32); /// 使用密钥的一部分作为KID
    }

    /// 准备IV（如果提供）
    if (!options.iv.empty())
    {
        iv = cleanHexString(options.iv);
        if (iv.length() < 32)
        {
            iv.append(32 - iv.length(), '0');
        }
        else if (iv.length() > 32)
        {
            iv = iv.substr(0, 32);
        }
    }

    /// 构建输出文件名
    std::string outputFile = options.output;

    /// 如果输出文件扩展名与输入相同，添加_decrypted后缀
    std::string inputExt  = std::filesystem::path(options.input).extension().string();
    std::string outputExt = std::filesystem::path(outputFile).extension().string();

    if (outputExt.empty() || outputExt == inputExt)
    {
        size_t dotPos = outputFile.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            outputFile = outputFile.substr(0, dotPos) + "_decrypted" + inputExt;
        }
        else
        {
            outputFile += "_decrypted" + inputExt;
        }
    }

    std::stringstream cmd;
    cmd << "\"" << XTool::getFFmpegPath() << "\" ";

    /// 解密参数
    /// 注意：根据你的FFmpeg版本，解密参数可能不同
    /// 以下是通用的CENC解密参数
    cmd << "-decryption_key " << key << " ";

    /// 基本参数
    // cmd << "-hide_banner -progress pipe:1 -nostats -loglevel info ";
    cmd << "-y "; /// 覆盖输出文件

    /// 输入文件
    cmd << "-i \"" << options.input << "\" ";

    /// 编码选项
    bool reencode = false; /// 默认复制流，保持原质量
    if (params.contains("--reencode"))
    {
        std::string reencodeVal = params.at("--reencode").asString();
        std::ranges::transform(reencodeVal, reencodeVal.begin(), ::tolower);
        reencode = (reencodeVal == "true" || reencodeVal == "1" || reencodeVal == "yes" || reencodeVal == "on");
    }

    if (reencode)
    {
        /// 解密后重新编码（如果需要转换格式或修复文件）
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

    // /// MP4优化标记
    // cmd << "-movflags +faststart ";

    /// 输出文件
    cmd << "\"" << outputFile << "\"";

    return cmd.str();
}

auto DecryptCommandBuilder::getTitle(const std::map<std::string, ParameterValue>& params) const -> std::string
{
    std::string input  = params.at("--input").asString();
    std::string output = params.at("--output").asString();

    std::filesystem::path inputPath(input);
    std::filesystem::path outputPath(output);

    std::string method = "MP4 CENC-AES-CTR";
    if (params.contains("--method"))
    {
        std::string userMethod = params.at("--method").asString();
        method                 = "MP4 CENC (" + userMethod + ")";
    }

    return "解密: " + inputPath.filename().string() + " → " + outputPath.filename().string() + " (" + method + ")";
}

IMPLEMENT_CREATE(DecryptCommandBuilder);
