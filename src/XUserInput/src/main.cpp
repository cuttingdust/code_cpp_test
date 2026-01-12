#include <algorithm>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <functional>
#include <memory>

/// \brief 分割字符串为子字符串向量
/// \param input 要分割的输入字符串
/// \param delimiter 分隔符，默认为空格
/// \param trimWhitespace 是否修剪每个子字符串两端的空白字符，默认为true
/// \return std::vector<std::string> 分割后的子字符串向量
static auto split(const std::string& input, char delimiter = ' ', bool trimWhitespace = true)
        -> std::vector<std::string>
{
    std::vector<std::string> ret;

    /// 如果输入为空，直接返回空向量
    if (input.empty())
    {
        return ret;
    }

    std::string        tmp;
    std::istringstream iss(input);

    while (std::getline(iss, tmp, delimiter))
    {
        /// 可选：修剪空白字符
        if (trimWhitespace)
        {
            /// 删除左侧空白
            tmp.erase(tmp.begin(), std::ranges::find_if(tmp, [](unsigned char ch) { return !std::isspace(ch); }));

            /// 删除右侧空白
            tmp.erase(std::ranges::find_if(std::ranges::reverse_view(tmp),
                                           [](unsigned char ch) { return !std::isspace(ch); })
                              .base(),
                      tmp.end());
        }

        /// 跳过空字符串（可能由于连续分隔符导致）
        if (!tmp.empty())
        {
            ret.push_back(tmp);
        }
    }

    return ret;
}

/// 重载版本：支持字符串作为分隔符（更复杂的分割）
static auto split(const std::string& input, const std::string& delimiter, bool trimWhitespace = true)
        -> std::vector<std::string>
{
    std::vector<std::string> ret;

    if (input.empty() || delimiter.empty())
    {
        ret.push_back(input);
        return ret;
    }

    size_t start = 0;
    size_t end   = input.find(delimiter);

    while (end != std::string::npos)
    {
        std::string token = input.substr(start, end - start);

        if (trimWhitespace)
        {
            token.erase(token.begin(), std::ranges::find_if(token, [](unsigned char ch) { return !std::isspace(ch); }));
            token.erase(std::ranges::find_if(std::ranges::reverse_view(token),
                                             [](unsigned char ch) { return !std::isspace(ch); })
                                .base(),
                        token.end());
        }

        if (!token.empty())
        {
            ret.push_back(token);
        }

        start = end + delimiter.length();
        end   = input.find(delimiter, start);
    }

    /// 添加最后一个部分
    std::string lastToken = input.substr(start);
    if (trimWhitespace)
    {
        lastToken.erase(lastToken.begin(),
                        std::ranges::find_if(lastToken, [](unsigned char ch) { return !std::isspace(ch); }));
        lastToken.erase(std::ranges::find_if(std::ranges::reverse_view(lastToken),
                                             [](unsigned char ch) { return !std::isspace(ch); })
                                .base(),
                        lastToken.end());
    }

    if (!lastToken.empty())
    {
        ret.push_back(lastToken);
    }

    return ret;
}

using MsgCallback = std::function<void(const std::string&)>;

class XUserInput
{
public:
    void start()
    {
        std::cout << "命令处理器已启动。输入 'exit' 退出，'help' 查看帮助。" << std::endl;

        while (is_running_)
        {
            std::cout << "\n>> " << std::flush;
            std::string input;
            std::getline(std::cin, input);

            if (input.empty())
            {
                continue;
            }

            /// 处理特殊命令
            if (input == "exit")
            {
                std::cout << "goodbye." << std::endl;
                is_running_ = false;
                continue;
            }

            if (input == "help")
            {
                printHelp();
                continue;
            }

            if (input == "list")
            {
                listCommands();
                continue;
            }

            /// 解析并执行命令
            processCommand(input);
        }
    };

    void stop()
    {
        is_running_ = false;
    };

    XUserInput& reg(const std::string& cmd, const MsgCallback& call, const std::string& description = "")
    {
        callback_list_[cmd] = call;
        if (!description.empty())
        {
            descriptions_[cmd] = description;
        }
        return *this;
    }

private:
    void processCommand(const std::string& input)
    {
        const auto& cmds = split(input);
        if (cmds.empty())
        {
            return;
        }

        /// 第一种处理方式：直接命令
        std::string mainCmd = cmds[0];
        if (callback_list_.contains(mainCmd))
        {
            /// 如果有参数，合并为字符串传给回调
            std::string args;
            if (cmds.size() > 1)
            {
                for (size_t i = 1; i < cmds.size(); ++i)
                {
                    args += cmds[i];
                    if (i < cmds.size() - 1)
                    {
                        args += " ";
                    }
                }
            }
            try
            {
                callback_list_[mainCmd](args);
            }
            catch (const std::exception& e)
            {
                std::cout << "执行命令时出错: " << e.what() << std::endl;
            }
            return;
        }

        /// 第二种处理方式：参数化命令（如 -s source -d dest）
        if (cmds.size() >= 2)
        {
            std::map<std::string, std::string> params;
            std::string                        currentKey;

            for (size_t i = 1; i < cmds.size(); ++i)
            {
                const auto& token = cmds[i];

                if (callback_list_.contains(token))
                {
                    /// 如果这是一个已注册的关键字
                    currentKey = token;
                    /// 检查下一个token是否是值（不是关键字）
                    if (i + 1 < cmds.size() && !callback_list_.contains(cmds[i + 1]))
                    {
                        params[currentKey] = cmds[i + 1];
                        i++; /// 跳过值
                    }
                }
            }

            /// 执行所有找到的参数回调
            for (const auto& [key, value] : params)
            {
                if (callback_list_.contains(key))
                {
                    try
                    {
                        callback_list_[key](value);
                    }
                    catch (const std::exception& e)
                    {
                        std::cout << "执行参数 '" << key << "' 时出错: " << e.what() << std::endl;
                    }
                }
            }

            if (!params.empty())
            {
                return;
            }
        }

        /// 如果都没匹配到
        std::cout << "未知命令: " << mainCmd << " (输入 'help' 查看可用命令)" << std::endl;
    }

    void printHelp() const
    {
        std::cout << "\n=== 命令帮助 ===" << std::endl;
        std::cout << "直接命令格式: <命令> [参数]" << std::endl;
        std::cout << "参数化格式: <动作> -参数1 值1 -参数2 值2 ..." << std::endl;
        std::cout << "\n可用命令:" << std::endl;

        for (const auto& [cmd, desc] : descriptions_)
        {
            std::cout << "  " << cmd << ": " << desc << std::endl;
        }

        std::cout << "\n特殊命令:" << std::endl;
        std::cout << "  exit: 退出程序" << std::endl;
        std::cout << "  help: 显示此帮助" << std::endl;
        std::cout << "  list: 列出所有注册的命令" << std::endl;
        std::cout << "================\n" << std::endl;
    }

    void listCommands() const
    {
        std::cout << "\n已注册的命令 (" << callback_list_.size() << "):" << std::endl;
        for (const auto& cmd : callback_list_ | std::views::keys)
        {
            std::cout << "  - " << cmd;
            if (descriptions_.contains(cmd))
            {
                std::cout << " (" << descriptions_.at(cmd) << ")";
            }
            std::cout << std::endl;
        }
    }

private:
    bool                               is_running_ = true;
    std::map<std::string, MsgCallback> callback_list_;
    std::map<std::string, std::string> descriptions_;
};

int main(int argc, char* argv[])
{
    /// 设置本地化（支持中文输出）
    setlocale(LC_ALL, "zh_CN.UTF-8");

    XUserInput userinput;

    /// 注册命令和参数
    userinput.reg(
                     "-s", [](const std::string& value) { std::cout << "[源路径] " << value << std::endl; },
                     "设置源路径")
            .reg(
                    "-d", [](const std::string& value) { std::cout << "[目标路径] " << value << std::endl; },
                    "设置目标路径")
            .reg(
                    "copy",
                    [](const std::string& args)
                    {
                        if (args.empty())
                        {
                            std::cout << "copy命令需要参数，格式: copy -s 源路径 -d 目标路径" << std::endl;
                        }
                        else
                        {
                            std::cout << "执行复制操作: " << args << std::endl;
                        }
                    },
                    "复制文件")
            .reg(
                    "move", [](const std::string& args) { std::cout << "执行移动操作: " << args << std::endl; },
                    "移动文件")
            .reg(
                    "delete",
                    [](const std::string& args)
                    {
                        if (args.empty())
                        {
                            std::cout << "警告: 删除操作需要指定目标!" << std::endl;
                        }
                        else
                        {
                            std::cout << "删除: " << args << std::endl;
                        }
                    },
                    "删除文件")
            .reg("echo", [](const std::string& args) { std::cout << "回显: " << args << std::endl; }, "回显输入的内容");

    std::cout << "=== 命令行处理器示例 ===" << std::endl;
    std::cout << "可以尝试以下命令:" << std::endl;
    std::cout << "  1. 直接命令: echo Hello World" << std::endl;
    std::cout << "  2. 参数命令: copy -s /path/src -d /path/dst" << std::endl;
    std::cout << "  3. 仅参数: -s /tmp/file.txt" << std::endl;
    std::cout << "  4. 帮助: help 或 list\n" << std::endl;

    userinput.start();

    return 0;
}
