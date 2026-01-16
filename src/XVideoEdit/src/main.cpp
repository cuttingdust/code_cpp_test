#include "XUserInput.h"

#include <iostream>
#include <Windows.h>

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "zh_CN.UTF-8");
    // // 设置控制台输入输出编码为UTF-8
    // SetConsoleOutputCP(CP_UTF8);
    // SetConsoleCP(CP_UTF8);


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
            .addStringParam("-s", "源文件路径", true)
            .addStringParam("-d", "目标路径", true);

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
            .addDoubleParam("-x", "基数", true)
            .addIntParam("-n", "指数", true)
            .addBoolParam("-v", "详细模式", false);

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
            .addStringParam("-host", "主机地址", true)
            .addIntParam("-port", "端口号", true)
            .addBoolParam("-debug", "调试模式", false)
            .addDoubleParam("-timeout", "超时时间(秒)", false);

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
            .addStringParam("-m", "要回显的消息", false);

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
            .addStringParam("--input", "源文件路径", true)
            .addStringParam("--output", "目标路径", true);

    std::cout << "=== 增强型任务处理器示例 ===" << std::endl;
    std::cout << "支持参数类型: 字符串、整数、浮点数、布尔值" << std::endl;
    std::cout << "可以尝试以下命令:" << std::endl;
    std::cout << "  1. task copy -s /home/file.txt -d /backup/" << std::endl;
    std::cout << "  2. task calculate -x 2.5 -n 3 -v true" << std::endl;
    std::cout << "  3. task start -host localhost -port 8080 -debug yes -timeout 30.5" << std::endl;
    std::cout << "  4. task echo -m \"Hello World\"" << std::endl;
    std::cout << "  5. help (查看帮助) 或 list (列出任务)\n" << std::endl;

    user_input.start();

    return 0;
}
