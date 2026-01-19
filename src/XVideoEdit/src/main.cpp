#include "XUserInput.h"

#include <iostream>
#include <Windows.h>

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "zh_CN.UTF-8");

    XUserInput user_input;

    /// 示例1：支持类型的copy任务
    user_input
            .registerTask(
                    "copy",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params)
                    {
                        std::cout << "[复制操作]" << std::endl;
                        auto src = params.at("-s").asString();
                        auto dst = params.at("-d").asString();
                        std::cout << "  从 " << src << " 复制到 " << dst << std::endl;
                        /// 实际复制逻辑
                    },
                    "复制文件")
            .addFileParam("-s", "源文件路径", true,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              /// 如果看起来像路径，返回空，让路径补全处理
                              if (partial.find('/') != std::string::npos || partial.find('\\') != std::string::npos ||
                                  partial.find('.') != std::string::npos)
                              {
                                  return {};
                              }
                              /// 否则提供一些常用建议
                              return { "file.txt", "data.dat", "source" };
                          })
            .addFileParam("-d", "目标路径", true,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              if (partial.find('/') != std::string::npos || partial.find('\\') != std::string::npos)
                              {
                                  return {};
                              }
                              return { "backup/", "output/", "dest/" };
                          });

    /// 示例2：数学计算任务（演示数值类型）
    user_input
            .registerTask(
                    "calculate",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params)
                    {
                        std::cout << "[计算操作]" << std::endl;
                        double x       = params.at("-x").asDouble();
                        int    n       = params.at("-n").asInt();
                        bool   verbose = params.contains("-v") ? params.at("-v").asBool() : false;

                        double result = 1.0;
                        for (int i = 0; i < n; ++i)
                            result *= x;

                        std::cout << "  结果: " << x << " ^ " << n << " = " << result << std::endl;
                        if (verbose)
                            std::cout << "  详细模式: 计算完成" << std::endl;
                    },
                    "数学计算")
            .addDoubleParam("-x", "基数", true,
                            [](std::string_view partial) -> std::vector<std::string>
                            {
                                std::vector<std::string>              suggestions;
                                static const std::vector<std::string> commonValues = { "2.0", "2.5",  "3.0",  "3.14",
                                                                                       "5.0", "10.0", "0.5",  "1.0",
                                                                                       "1.5", "0.1",  "0.25", "0.75" };
                                for (const auto& value : commonValues)
                                {
                                    if (value.starts_with(partial))
                                    {
                                        suggestions.push_back(value);
                                    }
                                }
                                return suggestions;
                            })
            .addIntParam("-n", "指数", true,
                         [](std::string_view partial) -> std::vector<std::string>
                         {
                             std::vector<std::string>              suggestions;
                             static const std::vector<std::string> commonValues = { "1",  "2",  "3",  "5",
                                                                                    "10", "20", "50", "100" };
                             for (const auto& value : commonValues)
                             {
                                 if (value.starts_with(partial))
                                 {
                                     suggestions.push_back(value);
                                 }
                             }
                             return suggestions;
                         })
            .addBoolParam("-v", "详细模式", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              std::vector<std::string>              suggestions;
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          });

    /// 示例3：服务器启动任务（演示多种类型）
    user_input
            .registerTask(
                    "start",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params)
                    {
                        std::cout << "[启动服务器]" << std::endl;
                        std::string host  = params.at("-host").asString();
                        int         port  = params.at("-port").asInt();
                        bool        debug = params.contains("-debug") ? params.at("-debug").asBool() : false;

                        std::cout << "  主机: " << host << ":" << port << std::endl;
                        std::cout << "  调试模式: " << (debug ? "开启" : "关闭") << std::endl;

                        if (params.contains("-timeout"))
                        {
                            double timeout = params.at("-timeout").asDouble();
                            std::cout << "  超时设置: " << timeout << "秒" << std::endl;
                        }
                    },
                    "启动服务器")
            .addStringParam("-host", "主机地址", true,
                            [](std::string_view partial) -> std::vector<std::string>
                            {
                                std::vector<std::string>              suggestions;
                                static const std::vector<std::string> commonHosts = { "localhost", "127.0.0.1",
                                                                                      "0.0.0.0",   "192.168.",
                                                                                      "10.0.",     "172.16." };
                                for (const auto& host : commonHosts)
                                {
                                    if (host.starts_with(partial))
                                    {
                                        suggestions.push_back(host);
                                    }
                                }
                                return suggestions;
                            })
            .addIntParam("-port", "端口号", true,
                         [](std::string_view partial) -> std::vector<std::string>
                         {
                             std::vector<std::string>              suggestions;
                             static const std::vector<std::string> commonPorts = { "80",   "443",   "8080", "3000",
                                                                                   "5000", "8000",  "3306", "5432",
                                                                                   "6379", "27017", "9200" };
                             for (const auto& port : commonPorts)
                             {
                                 if (port.starts_with(partial))
                                 {
                                     suggestions.push_back(port);
                                 }
                             }
                             return suggestions;
                         })
            .addBoolParam("-debug", "调试模式", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              std::vector<std::string>              suggestions;
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          })
            .addDoubleParam("-timeout", "超时时间(秒)", false,
                            [](std::string_view partial) -> std::vector<std::string>
                            {
                                std::vector<std::string>              suggestions;
                                static const std::vector<std::string> commonTimeouts = { "0.5",  "1.0",   "2.0",
                                                                                         "5.0",  "10.0",  "30.0",
                                                                                         "60.0", "300.0", "600.0" };
                                for (const auto& timeout : commonTimeouts)
                                {
                                    if (timeout.starts_with(partial))
                                    {
                                        suggestions.push_back(timeout);
                                    }
                                }
                                return suggestions;
                            });

    /// 示例4：回显任务（保持简单）
    user_input
            .registerTask(
                    "echo",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params)
                    {
                        if (params.contains("-m"))
                        {
                            std::cout << "回显: " << params.at("-m").asString() << std::endl;
                        }
                        else
                        {
                            std::cout << "(未指定消息，使用 -m 参数)" << std::endl;
                        }
                    },
                    "回显消息")
            .addStringParam("-m", "要回显的消息", false,
                            [](std::string_view partial) -> std::vector<std::string>
                            {
                                std::vector<std::string>              suggestions;
                                static const std::vector<std::string> commonMessages = {
                                    "Hello World", "Test", "Debug", "Error", "Warning", "Info", "Success", "Failure"
                                };
                                for (const auto& msg : commonMessages)
                                {
                                    if (msg.starts_with(partial))
                                    {
                                        suggestions.push_back(msg);
                                    }
                                }
                                return suggestions;
                            });

    /// 示例5：视频转码任务
    user_input
            .registerTask(
                    "cv",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params)
                    {
                        std::cout << "[转码操作]" << std::endl;
                        auto src = params.at("--input").asString();
                        auto dst = params.at("--output").asString();
                        std::cout << "  从 " << src << " 转码到 " << dst << std::endl;
                    },
                    "转码视频文件")
            .addFileParam("--input", "源文件路径", true,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              /// 如果是路径，返回空让路径补全处理
                              if (partial.find('/') != std::string::npos || partial.find('\\') != std::string::npos ||
                                  partial.find('.') != std::string::npos)
                              {
                                  return {};
                              }
                              /// 否则提供常见视频文件扩展名
                              static const std::vector<std::string> videoExtensions = { ".mp4", ".avi", ".mov", ".mkv",
                                                                                        ".wmv", ".flv", ".webm" };
                              std::vector<std::string>              suggestions;
                              for (const auto& ext : videoExtensions)
                              {
                                  suggestions.push_back("video" + ext);
                              }
                              return suggestions;
                          })
            .addFileParam("--output", "目标路径", true,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              /// 如果是路径，返回空让路径补全处理
                              if (partial.find('/') != std::string::npos || partial.find('\\') != std::string::npos ||
                                  partial.find('.') != std::string::npos)
                              {
                                  return {};
                              }
                              /// 否则提供常见输出文件建议
                              static const std::vector<std::string> outputSuggestions = {
                                  "output.mp4", "result.mp4", "converted.mp4", "output/", "result/", "converted/"
                              };
                              std::vector<std::string> suggestions;
                              for (const auto& suggestion : outputSuggestions)
                              {
                                  if (suggestion.starts_with(partial))
                                  {
                                      suggestions.push_back(suggestion);
                                  }
                              }
                              return suggestions;
                          });

    std::cout << "=== 增强型任务处理器示例 ===" << std::endl;
    std::cout << "支持参数类型: 字符串、整数、浮点数、布尔值" << std::endl;
    std::cout << "智能补全功能已启用！" << std::endl;
    std::cout << "可以尝试以下命令:" << std::endl;
    std::cout << "  1. task copy -s [Tab] 然后输入 -d [Tab]" << std::endl;
    std::cout << "  2. task calculate -x [Tab] 选择数值" << std::endl;
    std::cout << "  3. task start -host [Tab] 选择主机" << std::endl;
    std::cout << "  4. task start -port [Tab] 选择端口" << std::endl;
    std::cout << "  5. task echo -m [Tab] 选择消息" << std::endl;
    std::cout << "  6. task cv --input [Tab] (支持路径补全)" << std::endl;
    std::cout << "  7. help (查看帮助) 或 list (列出任务)\n" << std::endl;

    user_input.start();

    return 0;
}
