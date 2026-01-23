#include "XExec.h"
#include <iostream>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define WIN32_LEAN_AND_MEAN
#else
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#endif

class XExec::PImpl
{
public:
    PImpl() = default;
    ~PImpl();

public:
    auto start(const std::string_view& cmd, bool redirectStderr, OutputCallback callback) -> bool;

    auto isRunning() const -> bool;

    auto wait() -> int;

    auto terminate() -> bool;

    auto getStdout() const -> std::string;

    auto getStderr() const -> std::string;

    auto getAllOutput() const -> std::string;

public:
    auto cleanup() -> void;
    auto readOutput(bool isStderr) -> void;

#ifdef _WIN32
    void closeAllHandles();
    bool checkProcessExited();
#else
    void closeAllFds();
    bool checkProcessExited();
#endif

#ifdef _WIN32
    struct ProcessHandles
    {
        HANDLE hProcess  = INVALID_HANDLE_VALUE;
        HANDLE hStdoutRd = INVALID_HANDLE_VALUE;
        HANDLE hStderrRd = INVALID_HANDLE_VALUE;
        HANDLE hStdoutWr = INVALID_HANDLE_VALUE;
        HANDLE hStderrWr = INVALID_HANDLE_VALUE;
        HANDLE hStdinRd  = INVALID_HANDLE_VALUE;
        HANDLE hStdinWr  = INVALID_HANDLE_VALUE;
    };

    ProcessHandles handles_;
#else
    struct ProcessHandles
    {
        pid_t pid      = -1;
        int   stdoutFd = -1;
        int   stderrFd = -1;
        int   stdinFd  = -1;
    };

    ProcessHandles handles_;
#endif

    mutable std::mutex mutex_;
    std::string        stdout_;
    std::string        stderr_;
    std::atomic<bool>  isRunning_{ false };
    std::atomic<bool>  terminated_{ false };
    std::atomic<int>   exitCode_{ -1 };
    OutputCallback     outputCallback_;
    std::thread        stdoutThread_;
    std::thread        stderrThread_;
};

XExec::XExec() : impl_(std::make_unique<PImpl>())
{
}
XExec::~XExec()                           = default;
XExec::XExec(XExec&&) noexcept            = default;
XExec& XExec::operator=(XExec&&) noexcept = default;

auto XExec::setOutputCallback(const OutputCallback& callback) -> void
{
    impl_->outputCallback_ = callback;
}

auto XExec::start(const std::string_view& cmd, bool redirectStderr) -> bool
{
    return impl_->start(cmd, redirectStderr, impl_->outputCallback_);
}

auto XExec::getOutput() const -> std::string
{
    return impl_->getStdout();
}
auto XExec::getOutError() const -> std::string
{
    return impl_->getStderr();
}

auto XExec::getOutAll() const -> std::string
{
    return impl_->getAllOutput();
}

auto XExec::wait() -> int
{
    return impl_->wait();
}

auto XExec::isRunning() const -> bool
{
    return impl_->isRunning();
}

auto XExec::terminate() -> bool
{
    return impl_->terminate();
}

/// 静态方法实现
auto XExec::execute(const std::string_view& command, bool redirectStderr, int timeoutMs) -> XExec::XResult
{
    XExec   exec;
    XResult result;

    /// 启动命令
    if (!exec.start(command, redirectStderr))
    {
        result.exitCode     = -1;
        result.stderrOutput = "启动命令失败";
        return result;
    }

    /// 设置超时
    if (timeoutMs > 0)
    {
        auto startTime = std::chrono::steady_clock::now();
        while (exec.isRunning())
        {
            auto elapsed =
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime);

            if (elapsed.count() >= timeoutMs)
            {
                exec.terminate();
                result.exitCode = -2; /// 超时
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    /// 等待完成
    result.exitCode     = exec.wait();
    result.stdoutOutput = exec.getOutput();
    result.stderrOutput = exec.getOutError();

    return result;
}

#ifdef _WIN32
/// ==================== Windows 实现 ====================

void XExec::PImpl::closeAllHandles()
{
    auto closeHandle = [](HANDLE& h)
    {
        if (h != INVALID_HANDLE_VALUE && h != nullptr)
        {
            ::CloseHandle(h);
            h = INVALID_HANDLE_VALUE;
        }
    };

    closeHandle(handles_.hStdoutRd);
    closeHandle(handles_.hStderrRd);
    closeHandle(handles_.hStdoutWr);
    closeHandle(handles_.hStderrWr);
    closeHandle(handles_.hStdinRd);
    closeHandle(handles_.hStdinWr);
    /// 注意：hProcess 在 wait() 中单独处理
}

auto XExec::PImpl::checkProcessExited() -> bool
{
    if (handles_.hProcess == INVALID_HANDLE_VALUE)
        return true;

    DWORD exitCode = STILL_ACTIVE;
    if (::GetExitCodeProcess(handles_.hProcess, &exitCode))
    {
        return exitCode != STILL_ACTIVE;
    }
    return true; /// 获取失败也认为已退出
}

XExec::PImpl::~PImpl()
{
    cleanup();
}

auto XExec::PImpl::start(const std::string_view& cmd, bool redirectStderr, OutputCallback callback) -> bool
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (isRunning_)
    {
        std::cerr << "已有命令正在执行" << std::endl;
        return false;
    }

    outputCallback_ = std::move(callback);
    exitCode_       = -1;
    stdout_.clear();
    stderr_.clear();
    terminated_ = false;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength              = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle       = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    /// 创建stdout管道
    if (!::CreatePipe(&handles_.hStdoutRd, &handles_.hStdoutWr, &saAttr, 0))
    {
        std::cerr << "创建stdout管道失败" << std::endl;
        cleanup();
        return false;
    }

    if (!::SetHandleInformation(handles_.hStdoutRd, HANDLE_FLAG_INHERIT, 0))
    {
        std::cerr << "设置stdout管道信息失败" << std::endl;
        cleanup();
        return false;
    }

    /// 创建stderr管道（如果不重定向到stdout）
    if (!redirectStderr)
    {
        if (!CreatePipe(&handles_.hStderrRd, &handles_.hStderrWr, &saAttr, 0))
        {
            std::cerr << "创建stderr管道失败" << std::endl;
            cleanup();
            return false;
        }

        if (!::SetHandleInformation(handles_.hStderrRd, HANDLE_FLAG_INHERIT, 0))
        {
            std::cerr << "设置stderr管道信息失败" << std::endl;
            cleanup();
            return false;
        }
    }

    /// 创建stdin管道
    if (!CreatePipe(&handles_.hStdinRd, &handles_.hStdinWr, &saAttr, 0))
    {
        std::cerr << "创建stdin管道失败" << std::endl;
        cleanup();
        return false;
    }

    if (!::SetHandleInformation(handles_.hStdinWr, HANDLE_FLAG_INHERIT, 0))
    {
        std::cerr << "设置stdin管道信息失败" << std::endl;
        cleanup();
        return false;
    }

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOA        siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));

    siStartInfo.cb         = sizeof(STARTUPINFOA);
    siStartInfo.hStdInput  = handles_.hStdinRd;
    siStartInfo.hStdOutput = handles_.hStdoutWr;
    siStartInfo.hStdError  = redirectStderr ? handles_.hStdoutWr : handles_.hStderrWr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmdCopy = std::string{ cmd }; /// 创建副本，CreateProcessA可能修改字符串
    BOOL bSuccess = CreateProcessA(nullptr, cmdCopy.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr,
                                   &siStartInfo, &piProcInfo);

    /// 关闭子进程不用的句柄（重要！）
    ::CloseHandle(handles_.hStdoutWr);
    handles_.hStdoutWr = INVALID_HANDLE_VALUE;
    ::CloseHandle(handles_.hStdinRd);
    handles_.hStdinRd = INVALID_HANDLE_VALUE;

    if (!redirectStderr)
    {
        ::CloseHandle(handles_.hStderrWr);
        handles_.hStderrWr = INVALID_HANDLE_VALUE;
    }

    if (!bSuccess)
    {
        DWORD error = GetLastError();
        std::cerr << "创建进程失败，错误码: " << error << std::endl;
        cleanup();
        return false;
    }

    handles_.hProcess = piProcInfo.hProcess;
    ::CloseHandle(piProcInfo.hThread); /// 线程句柄不需要

    isRunning_.store(true, std::memory_order_release);

    /// 直接启动线程，不需要检查
    stdoutThread_ = std::thread([this]() { readOutput(false); });

    if (!redirectStderr)
    {
        stderrThread_ = std::thread([this]() { readOutput(true); });
    }

    return true;
}

auto XExec::PImpl::readOutput(bool isStderr) -> void
{
    HANDLE hPipe = isStderr ? handles_.hStderrRd : handles_.hStdoutRd;
    char   buffer[4096];
    DWORD  bytesRead;

    while (isRunning_.load(std::memory_order_acquire))
    {
        BOOL readResult = ::ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);

        if (readResult && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            std::string output(buffer);

            std::lock_guard<std::mutex> lock(mutex_);
            if (isStderr)
            {
                stderr_ += output;
            }
            else
            {
                stdout_ += output;
            }

            if (outputCallback_)
            {
                /// 按行回调
                std::istringstream stream(output);
                std::string        line;
                while (std::getline(stream, line))
                {
                    if (!line.empty() && line.back() == '\r')
                    {
                        line.pop_back();
                    }
                    if (!line.empty())
                    {
                        outputCallback_(line, isStderr);
                    }
                }
            }
        }
        else
        {
            /// 检查是否应该退出
            DWORD error = GetLastError();
            if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA || bytesRead == 0)
            {
                /// 管道已关闭，正常退出
                isRunning_.store(false, std::memory_order_release);
                break;
            }

            /// 短暂休眠避免忙等待
            ::Sleep(10);
        }
    }
}

auto XExec::PImpl::wait() -> int
{
    if (!isRunning_ && exitCode_ != -1)
    {
        return exitCode_;
    }

    /// 步骤1：等待进程退出
    if (handles_.hProcess != INVALID_HANDLE_VALUE)
    {
        ::WaitForSingleObject(handles_.hProcess, INFINITE);

        DWORD dwExitCode;
        if (::GetExitCodeProcess(handles_.hProcess, &dwExitCode))
        {
            exitCode_ = static_cast<int>(dwExitCode);
        }
        else
        {
            exitCode_ = -1;
        }
    }

    /// 步骤2：关闭管道，让读取线程自然退出
    closeAllHandles();

    /// 步骤3：等待线程结束
    if (stdoutThread_.joinable())
    {
        stdoutThread_.join();
    }

    if (stderrThread_.joinable())
    {
        stderrThread_.join();
    }

    /// 步骤4：关闭进程句柄
    if (handles_.hProcess != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(handles_.hProcess);
        handles_.hProcess = INVALID_HANDLE_VALUE;
    }

    /// 步骤5：最后更新状态
    isRunning_.store(false, std::memory_order_release);

    return exitCode_;
}

auto XExec::PImpl::terminate() -> bool
{
    if (!isRunning_ || handles_.hProcess == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    terminated_ = true;

    if (!TerminateProcess(handles_.hProcess, 1))
    {
        std::cerr << "终止进程失败" << std::endl;
        return false;
    }

    wait(); /// 等待清理
    return true;
}

#else
// ==================== Linux/macOS 实现 ====================

void XExec::PImpl::closeAllFds()
{
    auto closeFd = [](int& fd)
    {
        if (fd != -1)
        {
            close(fd);
            fd = -1;
        }
    };

    closeFd(handles_.stdoutFd);
    closeFd(handles_.stderrFd);
    closeFd(handles_.stdinFd);
}

bool XExec::PImpl::checkProcessExited()
{
    if (handles_.pid <= 0)
        return true;

    int   status;
    pid_t result = waitpid(handles_.pid, &status, WNOHANG);
    return result > 0;
}

bool XExec::PImpl::start(const std::string& cmd, bool redirectStderr, OutputCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (isRunning_)
    {
        std::cerr << "已有命令正在执行" << std::endl;
        return false;
    }

    outputCallback_ = std::move(callback);
    exitCode_       = -1;
    stdout_.clear();
    stderr_.clear();
    terminated_ = false;
    // 删除：reading_ = false;

    int stdoutPipe[2] = { -1, -1 };
    int stderrPipe[2] = { -1, -1 };
    int stdinPipe[2]  = { -1, -1 };

    // 创建管道
    if (pipe(stdoutPipe) == -1)
    {
        std::cerr << "创建stdout管道失败" << std::endl;
        return false;
    }

    if (pipe(stdinPipe) == -1)
    {
        std::cerr << "创建stdin管道失败" << std::endl;
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        return false;
    }

    if (!redirectStderr && pipe(stderrPipe) == -1)
    {
        std::cerr << "创建stderr管道失败" << std::endl;
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        return false;
    }

    handles_.pid = fork();

    if (handles_.pid == -1)
    {
        std::cerr << "fork失败" << std::endl;
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);
        close(stdinPipe[0]);
        close(stdinPipe[1]);
        if (!redirectStderr)
        {
            close(stderrPipe[0]);
            close(stderrPipe[1]);
        }
        return false;
    }

    if (handles_.pid == 0) // 子进程
    {
        // 关闭父进程用的读端
        close(stdoutPipe[0]);
        close(stdinPipe[1]);

        // 重定向stdin
        if (dup2(stdinPipe[0], STDIN_FILENO) == -1)
        {
            exit(127);
        }
        close(stdinPipe[0]);

        // 重定向stdout
        if (dup2(stdoutPipe[1], STDOUT_FILENO) == -1)
        {
            exit(127);
        }
        close(stdoutPipe[1]);

        // 重定向stderr
        if (redirectStderr)
        {
            if (dup2(stdoutPipe[1], STDERR_FILENO) == -1)
            {
                exit(127);
            }
        }
        else
        {
            close(stderrPipe[0]);
            if (dup2(stderrPipe[1], STDERR_FILENO) == -1)
            {
                exit(127);
            }
            close(stderrPipe[1]);
        }

        // 执行命令
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        exit(127); // exec失败
    }
    else // 父进程
    {
        // 关闭子进程用的写端
        close(stdoutPipe[1]);
        handles_.stdoutFd = stdoutPipe[0];
        close(stdinPipe[0]);
        handles_.stdinFd = stdinPipe[1];

        if (!redirectStderr)
        {
            close(stderrPipe[1]);
            handles_.stderrFd = stderrPipe[0];
        }

        // 设置管道非阻塞
        fcntl(handles_.stdoutFd, F_SETFL, O_NONBLOCK);
        if (!redirectStderr)
        {
            fcntl(handles_.stderrFd, F_SETFL, O_NONBLOCK);
        }

        // 关键：先设置 isRunning_，然后立即启动线程
        isRunning_.store(true, std::memory_order_release);

        // 直接启动线程
        stdoutThread_ = std::thread([this]() { readOutput(false); });

        if (!redirectStderr)
        {
            stderrThread_ = std::thread([this]() { readOutput(true); });
        }
    }

    return true;
}

void XExec::PImpl::readOutput(bool isStderr)
{
    int  fd = isStderr ? handles_.stderrFd : handles_.stdoutFd;
    char buffer[4096];

    // 使用 isRunning_ 作为条件
    while (isRunning_.load(std::memory_order_acquire))
    {
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            std::string output(buffer);

            std::lock_guard<std::mutex> lock(mutex_);
            if (isStderr)
            {
                stderr_ += output;
            }
            else
            {
                stdout_ += output;
            }

            if (outputCallback_)
            {
                std::istringstream stream(output);
                std::string        line;
                while (std::getline(stream, line))
                {
                    if (!line.empty())
                    {
                        outputCallback_(line, isStderr);
                    }
                }
            }
        }
        else if (bytesRead == 0)
        {
            // EOF，管道已关闭
            break;
        }
        else if (bytesRead == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 非阻塞，没有数据
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            else
            {
                // 其他错误
                break;
            }
        }
    }
}

int XExec::PImpl::wait()
{
    if (!isRunning_ && exitCode_ != -1)
    {
        return exitCode_;
    }

    // 步骤1：等待进程退出
    if (handles_.pid > 0)
    {
        int status;
        waitpid(handles_.pid, &status, 0);

        if (WIFEXITED(status))
        {
            exitCode_ = WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            exitCode_ = 128 + WTERMSIG(status); // 按照bash惯例
        }
        else
        {
            exitCode_ = -1;
        }
    }

    // 步骤2：关闭文件描述符，让读取线程自然退出
    closeAllFds();

    // 步骤3：等待线程结束
    if (stdoutThread_.joinable())
    {
        stdoutThread_.join();
    }

    if (stderrThread_.joinable())
    {
        stderrThread_.join();
    }

    // 步骤4：最后更新状态
    isRunning_.store(false, std::memory_order_release);

    // 步骤5：清理
    cleanup();

    return exitCode_;
}

bool XExec::PImpl::terminate()
{
    if (!isRunning_ || handles_.pid <= 0)
    {
        return false;
    }

    terminated_ = true;

    if (kill(handles_.pid, SIGTERM) == -1)
    {
        std::cerr << "发送SIGTERM失败" << std::endl;
        return false;
    }

    // 等待一段时间
    for (int i = 0; i < 50; ++i) // 最多等待5秒
    {
        if (!isRunning())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 强制终止
    if (kill(handles_.pid, SIGKILL) == -1)
    {
        std::cerr << "发送SIGKILL失败" << std::endl;
        return false;
    }

    return true;
}

#endif

// ==================== 跨平台通用实现 ====================

auto XExec::PImpl::isRunning() const -> bool
{
    return isRunning_;
}

auto XExec::PImpl::getStdout() const -> std::string
{
    std::lock_guard<std::mutex> lock(mutex_);
    return stdout_;
}

std::string XExec::PImpl::getStderr() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return stderr_;
}

auto XExec::PImpl::getAllOutput() const -> std::string
{
    std::lock_guard<std::mutex> lock(mutex_);
    return stdout_ + stderr_;
}

auto XExec::PImpl::cleanup() -> void
{
#ifdef _WIN32
    auto closeHandle = [](HANDLE& h)
    {
        if (h != INVALID_HANDLE_VALUE && h != nullptr)
        {
            ::CloseHandle(h);
            h = INVALID_HANDLE_VALUE;
        }
    };

    closeHandle(handles_.hProcess);
    closeHandle(handles_.hStdoutRd);
    closeHandle(handles_.hStderrRd);
    closeHandle(handles_.hStdoutWr);
    closeHandle(handles_.hStderrWr);
    closeHandle(handles_.hStdinRd);
    closeHandle(handles_.hStdinWr);
#else
    auto closeFd = [](int& fd)
    {
        if (fd != -1)
        {
            close(fd);
            fd = -1;
        }
    };

    closeFd(handles_.stdoutFd);
    closeFd(handles_.stderrFd);
    closeFd(handles_.stdinFd);
    handles_.pid = -1;
#endif

    isRunning_.store(false, std::memory_order_release);
}
