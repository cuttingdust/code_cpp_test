/// XFile.h
#pragma once
#include "XConst.h"

class XFile
{
public:
    /// ==================== 路径操作 ====================

    /// 提取路径
    static auto extractPathPart(const std::string_view& input) -> std::string;

    /// 是否是目录输入
    static auto isPathInput(const std::string_view& input) -> bool;

    /// 是否是相对路径
    static auto isRelativePath(const std::string_view& path) -> bool;

    /// \brief 格式化目录
    /// \param path
    /// \return 格式化的绝对路径
    static auto normalizePath(const std::string_view& path) -> std::string;

    static auto getParentPath(const std::string_view& path) -> std::string;

    static auto getFileName(const std::string_view& path) -> std::string;

    static auto getFileExtension(const std::string_view& path) -> std::string;

public:
    /// ==================== 文件检测 ====================
    static auto fileExists(const std::string_view& path) -> bool;

    static auto isDirectory(const std::string_view& path) -> bool;

    static auto isRegularFile(const std::string_view& path) -> bool;

    static auto isExecutable(const std::string_view& path) -> bool;

    static auto isHiddenFile(const std::string_view& path) -> bool;

public:
    /// ==================== 目录遍历 ====================
    struct FileEntry
    {
        std::string path;
        std::string name;
        bool        isDirectory;
        bool        isExecutable;
        uintmax_t   size;
    };

    static auto listDirectory(const std::string_view& dirPath, bool showHidden = false,
                              const std::string_view& prefix = "") -> std::vector<FileEntry>;

    static auto findFiles(const std::string_view& dirPath, const std::function<bool(const FileEntry&)>& filter)
            -> std::vector<std::string>;

    /// ==================== 文件信息 ====================
    static auto getFileSize(const std::string_view& path) -> std::optional<uintmax_t>;
    static auto formatFileSize(uintmax_t size) -> std::string;
    static auto getFileTypeDescription(const std::string_view& path) -> std::string;

    /// ==================== 平台相关 ====================
    static auto separator() -> std::string;
    static auto getHomeDirectory() -> std::string;
    static auto getCurrentWorkingDirectory() -> std::string;
    static auto setCurrentWorkingDirectory(const std::string& path) -> bool;

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
