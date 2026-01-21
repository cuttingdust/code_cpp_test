/// XFile.h
#pragma once
#include "XConst.h"

class XFile
{
public:
    /// ==================== 路径操作 ====================
    static auto        extractPathPart(const std::string_view& input) -> std::string;
    static auto        isPathInput(const std::string_view& input) -> bool;
    static bool        isRelativePath(const std::string& path);
    static std::string normalizePath(const std::string& path);
    static std::string getParentPath(const std::string& path);
    static std::string getFileName(const std::string& path);
    static std::string getFileExtension(const std::string& path);

    /// ==================== 文件检测 ====================
    static bool fileExists(const std::string& path);
    static bool isDirectory(const std::string& path);
    static bool isRegularFile(const std::string& path);
    static auto isExecutable(const std::string_view& path) -> bool;
    static bool isHiddenFile(const std::string& path);

    /// ==================== 目录遍历 ====================
    struct FileEntry
    {
        std::string path;
        std::string name;
        bool        isDirectory;
        bool        isExecutable;
        uintmax_t   size;
    };

    static std::vector<FileEntry> listDirectory(const std::string& dirPath, bool showHidden = false,
                                                const std::string& prefix = "");

    static std::vector<std::string> findFiles(const std::string&                           dirPath,
                                              const std::function<bool(const FileEntry&)>& filter);

    /// ==================== 路径补全 ====================
    struct PathCompletion
    {
        std::string completion;
        bool        isDirectory;
        bool        isExecutable;
    };

    static std::vector<PathCompletion> getPathCompletions(const std::string& partialPath, bool showHidden = false);

    /// ==================== 智能路径补全 ====================
    static std::vector<PathCompletion> smartPathCompletion(const std::string& input,
                                                           const std::string& currentWorkingDir = ".");

    /// ==================== 文件信息 ====================
    static std::optional<uintmax_t> getFileSize(const std::string& path);
    static std::string              formatFileSize(uintmax_t size);
    static std::string              getFileTypeDescription(const std::string& path);

    /// ==================== 平台相关 ====================
    static std::string separator();
    static std::string getHomeDirectory();
    static std::string getCurrentWorkingDirectory();
    static bool        setCurrentWorkingDirectory(const std::string& path);

    /// ==================== 配置 ====================
    static auto shouldShowHiddenFiles() -> bool;
    static auto setShowHiddenFiles(bool show) -> void;

private:
    /// 私有构造函数，这是一个工具类
    XFile() = delete;

    /// 平台特定的实现
#ifdef _WIN32
    static bool isWindowsExecutable(const fs::path& path);
#else
    static bool isUnixExecutable(const fs::path& path);
#endif
};
