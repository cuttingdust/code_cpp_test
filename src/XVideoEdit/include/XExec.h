#ifndef XEXEC_H
#define XEXEC_H

#include <string>
#include <functional>
#include <memory>

class XExec
{
public:
    enum class ExecutionMode
    {
        Direct, ///< 直接执行命令（默认）
        Shell   ///< 通过shell执行（支持&&, ||, |等）
    };

    struct XResult
    {
        int         exitCode = -1;
        std::string stdoutOutput;
        std::string stderrOutput;
    };

    XExec();
    ~XExec();
    XExec(XExec&&) noexcept;
    XExec& operator=(XExec&&) noexcept;

    /// 删除拷贝构造和赋值
    XExec(const XExec&)            = delete;
    XExec& operator=(const XExec&) = delete;
    using OutputCallback           = std::function<void(const std::string_view& line, bool isStderr)>;

public:
    auto setOutputCallback(const OutputCallback& callback) -> void;

    /// 设置执行模式
    auto setExecutionMode(ExecutionMode mode) -> void;

    auto start(const std::string_view& cmd, bool redirectStderr = true) -> bool;

    auto isRunning() const -> bool;

    /// \brief  等待
    /// \return 错误码
    auto wait() -> int;

    auto terminate() -> bool;

    auto getOutput() const -> std::string;

    auto getOutError() const -> std::string;

    auto getOutAll() const -> std::string;

public:
    static auto execute(const std::string_view& command, bool redirectStderr = true, int timeoutMs = 0) -> XResult;

private:
    class PImpl;
    std::unique_ptr<PImpl> impl_;
};

#endif // XEXEC_H
