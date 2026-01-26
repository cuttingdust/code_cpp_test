#include "ConvertCommandBuilder.h"
#include "CutCommandBuilder.h"
#include "AnalyzeCommandBuilder.h"

#include "AVTask.h"

#include "CVProgressBar.h"
#include "CutProgressBar.h"

#include "XUserInput.h"

#include <iostream>

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "zh_CN.UTF-8");

    XUserInput user_input;

    /// 设置事件回调
    user_input.setOnCommandStart([](const std::string_view& cmd) { std::cout << "开始执行命令: " << cmd << "\n"; });
    user_input.setOnCommandComplete([](const std::string_view& cmd) { std::cout << "命令执行完成\n"; });
    user_input.setOnError([](const std::string_view& error) { std::cerr << "执行出错: " << error << "\n"; });

    user_input.getTaskManager().registerType<AVTask, CVProgressBar>("av", "音视频处理任务");

    /// 示例1：支持类型的copy任务
    user_input
            .registerTask(
                    "copy",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params, const std::string& msg)
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
                    [](const std::map<std::string, XUserInput::ParameterValue>& params, const std::string& msg)
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
                    [](const std::map<std::string, XUserInput::ParameterValue>& params, const std::string& result)
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
                    [](const std::map<std::string, XUserInput::ParameterValue>& params, const std::string& result)
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
            .registerTask<ConvertCommandBuilder>(
                    "cv", "av",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params, const std::string& result)
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
                              suggestions.reserve(videoExtensions.size());
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

    user_input
            .registerTask<CutCommandBuilder, CutProgressBar>(
                    "cut", "av",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params, const std::string& result)
                    {
                        std::cout << "[剪切视频操作]" << std::endl;
                        auto src = params.at("--input").asString();
                        auto dst = params.at("--output").asString();

                        /// 处理开始时间
                        std::string startTime = "00:00:00";
                        if (params.contains("--start"))
                        {
                            startTime = params.at("--start").asString();
                        }

                        /// 处理结束时间或持续时间
                        std::string duration;
                        std::string endTime;

                        if (params.contains("--duration") && params.contains("--end"))
                        {
                            std::cout << "警告: 同时指定了 --duration 和 --end 参数，优先使用 --duration" << std::endl;
                            duration = params.at("--duration").asString();
                            std::cout << "  剪切设置: 从 " << startTime << " 开始，持续 " << duration << std::endl;
                        }
                        else if (params.contains("--duration"))
                        {
                            duration = params.at("--duration").asString();
                            std::cout << "  剪切设置: 从 " << startTime << " 开始，持续 " << duration << std::endl;
                        }
                        else if (params.contains("--end"))
                        {
                            endTime = params.at("--end").asString();
                            std::cout << "  剪切设置: 从 " << startTime << " 到 " << endTime << std::endl;
                        }
                        else
                        {
                            std::cout << "错误: 必须指定 --duration 或 --end 参数之一" << std::endl;
                            return;
                        }

                        std::cout << "  源文件: " << src << std::endl;
                        std::cout << "  目标文件: " << dst << std::endl;
                    },
                    "剪切视频文件，支持时间点和持续时间")
            .addFileParam("--input", "源视频文件路径", true,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              /// 如果是路径，返回空让路径补全处理
                              if (partial.find('/') != std::string::npos || partial.find('\\') != std::string::npos ||
                                  partial.find('.') != std::string::npos)
                              {
                                  return {};
                              }
                              /// 否则提供常见视频文件扩展名
                              static const std::vector<std::string> videoExtensions = { ".mp4", ".avi", ".mov",  ".mkv",
                                                                                        ".wmv", ".flv", ".webm", ".MP4",
                                                                                        ".AVI", ".MOV", ".MKV",  ".WMV",
                                                                                        ".FLV", ".WEBM" };
                              std::vector<std::string>              suggestions;
                              for (const auto& ext : videoExtensions)
                              {
                                  suggestions.push_back("video" + ext);
                              }
                              return suggestions;
                          })
            .addFileParam("--output", "输出文件路径", true,
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
                                  "output.mp4", "cut_video.mp4", "trimmed.mp4", "clip.mp4",
                                  "result.mp4", "output/",       "result/"
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
                          })
            .addStringParam("--start", "开始时间 (HH:MM:SS 或秒数)", false,
                            [](std::string_view partial) -> std::vector<std::string>
                            {
                                /// 提供常用的时间点建议
                                static const std::vector<std::string> timeSuggestions = {
                                    "00:00:00", "00:00:05", "00:00:10", "00:00:30", "00:01:00", "00:02:00",
                                    "00:05:00", "00:10:00", "00:30:00", "01:00:00", "0",        "5",
                                    "10",       "30",       "60",       "120",      "300",      "600"
                                };
                                std::vector<std::string> suggestions;
                                for (const auto& time : timeSuggestions)
                                {
                                    if (time.starts_with(partial))
                                    {
                                        suggestions.push_back(time);
                                    }
                                }
                                return suggestions;
                            })
            .addStringParam("--end", "结束时间 (HH:MM:SS 或秒数)", false,
                            [](std::string_view partial) -> std::vector<std::string>
                            {
                                /// 提供常用的结束时间建议
                                static const std::vector<std::string> timeSuggestions = {
                                    "00:00:30", "00:01:00", "00:02:00", "00:05:00", "00:10:00", "00:30:00",
                                    "01:00:00", "05:00:00", "10:00:00", "30",       "60",       "120",
                                    "300",      "600",      "1800",     "3600"
                                };
                                std::vector<std::string> suggestions;
                                for (const auto& time : timeSuggestions)
                                {
                                    if (time.starts_with(partial))
                                    {
                                        suggestions.push_back(time);
                                    }
                                }
                                return suggestions;
                            })
            .addStringParam("--duration", "持续时间 (HH:MM:SS 或秒数)", false,
                            [](std::string_view partial) -> std::vector<std::string>
                            {
                                /// 提供常用的持续时间建议
                                static const std::vector<std::string> durationSuggestions = {
                                    "00:00:05", "00:00:10", "00:00:30", "00:01:00", "00:02:00", "00:05:00",
                                    "00:10:00", "00:30:00", "01:00:00", "5",        "10",       "30",
                                    "60",       "120",      "300",      "600",      "1800"
                                };
                                std::vector<std::string> suggestions;
                                for (const auto& duration : durationSuggestions)
                                {
                                    if (duration.starts_with(partial))
                                    {
                                        suggestions.push_back(duration);
                                    }
                                }
                                return suggestions;
                            })
            .addBoolParam("--copy", "使用流复制模式 (快速但不精确)", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",
                                                                                   "0",    "yes",   "no" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          })
            .addBoolParam("--reencode", "重新编码模式 (精确但较慢)", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",
                                                                                   "0",    "yes",   "no" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          });

    /// 示例6：分析视频信息任务
    user_input
            .registerTask<AnalyzeCommandBuilder>(
                    "analyze", "av",
                    [](const std::map<std::string, XUserInput::ParameterValue>& params, const std::string& msg)
                    {
                        std::cout << "[分析视频信息操作]" << std::endl;
                        auto src         = params.at("--input").asString();
                        bool json_output = params.contains("--json") ? params.at("--json").asBool() : false;

                        std::cout << "  分析文件: " << src << std::endl;
                        std::cout << "  输出格式: " << (json_output ? "JSON" : "默认") << std::endl;

                        if (params.contains("--show-frames") && params.at("--show-frames").asBool())
                        {
                            std::cout << "  显示帧信息: 是" << std::endl;
                        }

                        if (params.contains("--pretty") && json_output)
                        {
                            std::cout << "  美化JSON输出: 是" << std::endl;
                        }

                        std::cout << msg << std::endl;
                    },
                    "分析视频/音频文件信息")
            .addFileParam("--input", "源文件路径", true,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              /// 如果是路径，返回空让路径补全处理
                              if (partial.find('/') != std::string::npos || partial.find('\\') != std::string::npos ||
                                  partial.find('.') != std::string::npos)
                              {
                                  return {};
                              }
                              /// 否则提供常见媒体文件扩展名
                              static const std::vector<std::string> mediaExtensions = { ".mp4", ".avi", ".mov",  ".mkv",
                                                                                        ".wmv", ".flv", ".webm", ".mp3",
                                                                                        ".wav", ".aac", ".flac", ".ogg",
                                                                                        ".m4a" };
                              std::vector<std::string>              suggestions;
                              suggestions.reserve(mediaExtensions.size());
                              for (const auto& ext : mediaExtensions)
                              {
                                  suggestions.push_back("media" + ext);
                              }
                              return suggestions;
                          })
            .addBoolParam("--json", "以JSON格式输出", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          })
            .addBoolParam("--show-format", "显示容器格式信息", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          })
            .addBoolParam("--show-streams", "显示流信息", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          })
            .addBoolParam("--show-frames", "显示帧信息（详细模式）", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          })
            .addStringParam("--select-streams", "选择特定流（如v:0,a:0）", false,
                            [](std::string_view partial) -> std::vector<std::string>
                            {
                                static const std::vector<std::string> streamSuggestions = { "v:0", "a:0", "v:0,a:0",
                                                                                            "a",   "v",   "s" };
                                std::vector<std::string>              suggestions;
                                for (const auto& stream : streamSuggestions)
                                {
                                    if (stream.starts_with(partial))
                                    {
                                        suggestions.push_back(stream);
                                    }
                                }
                                return suggestions;
                            })
            .addBoolParam("--count-frames", "计算帧数", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          })
            .addBoolParam("--count-packets", "计算包数", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          })
            .addBoolParam("--pretty", "美化JSON输出（需要python）", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          })
            .addBoolParam("--force", "强制分析非标准文件", false,
                          [](std::string_view partial) -> std::vector<std::string>
                          {
                              static const std::vector<std::string> boolValues = { "true", "false", "1",  "0",
                                                                                   "yes",  "no",    "on", "off" };
                              std::vector<std::string>              suggestions;
                              for (const auto& value : boolValues)
                              {
                                  if (value.starts_with(partial))
                                  {
                                      suggestions.push_back(value);
                                  }
                              }
                              return suggestions;
                          });


    /// 注册自定义命令
    user_input.registerCommandHandler("hello",
                                      [](const CommandParser::ParsedCommand& cmd)
                                      {
                                          std::string name = cmd.args.empty() ? "World" : cmd.args[0];
                                          std::cout << "Hello, " << name << "!\n";
                                      });

    user_input.registerCommandHandler("stats",
                                      [&user_input](const CommandParser::ParsedCommand&)
                                      {
                                          std::cout << "=== 统计信息 ===\n";
                                          std::cout << "任务数量: " << user_input.getTaskCount() << "\n";
                                          std::cout << "命令数量: " << user_input.getCommandCount() << "\n";
                                          std::cout << "系统状态: " << user_input.getStateString() << "\n";
                                      });

    user_input.start();

    return 0;
}
