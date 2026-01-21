#include "XFile.h"

#include <algorithm>
#include <regex>
#include <chrono>
#include <iomanip>
#include <map>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <userenv.h>
#else
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

/// 静态配置
static bool g_showHiddenFiles = false;

/// ==================== 路径操作 ====================
auto XFile::extractPathPart(const std::string_view& input) -> std::string
{
    if (input.empty())
        return "";

    /// 情况1：输入以路径分隔符结尾（快速判断）
    if (input.back() == '/' || input.back() == '\\')
    {
        /// 向前查找空格以确定路径开始位置
        size_t start = input.length() - 1;
        while (start > 0 && input[start - 1] != ' ')
        {
            --start;
        }
        return std::string(input.substr(start));
    }

    /// 查找最后一个空格位置
    size_t           lastSpace = input.find_last_of(' ');
    size_t           pathStart = (lastSpace != std::string_view::npos) ? lastSpace + 1 : 0;
    std::string_view lastPart  = input.substr(pathStart);

    /// 情况2：最后一个部分本身就是路径
    if (isPathInput(lastPart))
    {
        return std::string(lastPart);
    }

    /// 情况3：检查是否包含路径分隔符但可能不完整
    size_t lastSlash = input.find_last_of("/\\");
    if (lastSlash != std::string_view::npos && lastSlash >= pathStart)
    {
        /// 从最后一个分隔符开始，向前找空格或开头
        size_t slashStart = lastSlash;
        while (slashStart > pathStart && input[slashStart - 1] != ' ')
        {
            --slashStart;
        }

        std::string_view candidate = input.substr(slashStart);
        /// 验证候选字符串是否包含有效的路径模式
        if (candidate.find('/') != std::string_view::npos || candidate.find('\\') != std::string_view::npos ||
            candidate.starts_with(".") || candidate.starts_with(".."))
        {
            return std::string(candidate);
        }
    }

    /// 情况4：检查相对路径的特殊情况
    if (lastPart.starts_with(".") || lastPart.starts_with(".."))
    {
        return std::string(lastPart);
    }

    /// 情况5：检查Windows驱动器模式
    if (lastPart.length() >= 2 && lastPart[1] == ':' &&
        ((lastPart[0] >= 'A' && lastPart[0] <= 'Z') || (lastPart[0] >= 'a' && lastPart[0] <= 'z')))
    {
        return std::string(lastPart);
    }

    return "";
}

auto XFile::isPathInput(const std::string_view& input) -> bool
{
    if (input.empty())
        return false;

    /// 检查常见路径模式
    /// 1. 包含路径分隔符
    if (input.find('/') != std::string::npos || input.find('\\') != std::string::npos)
    {
        return true;
    }

    /// 2. 相对路径标识
    if (input == "." || input == ".." || input.starts_with("./") || input.starts_with("../") ||
        input.starts_with(".\\") || input.starts_with("..\\"))
    {
        return true;
    }

    /// 3. Windows驱动器路径 (C:, D:, etc.)
    if (input.length() >= 2 && input[1] == ':' &&
        ((input[0] >= 'A' && input[0] <= 'Z') || (input[0] >= 'a' && input[0] <= 'z')))
    {
        return true;
    }

    /// 4. 带有扩展名的文件名（可能是部分输入）
    if (input.find('.') != std::string::npos)
    {
        /// 排除纯数字带点的情况（如版本号）
        std::regex versionRegex(R"(^\d+(\.\d+)*$)");
        if (!std::regex_match(std::string(input), versionRegex))
        {
            /// 检查是否可能是文件扩展名
            size_t lastDot = input.find_last_of('.');
            if (lastDot > 0 && lastDot < input.length() - 1)
            {
                std::string_view extension = input.substr(lastDot + 1);
                /// 简单的扩展名检查
                bool looksLikeExtension = true;
                for (char c : extension)
                {
                    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-')
                    {
                        looksLikeExtension = false;
                        break;
                    }
                }
                if (looksLikeExtension)
                {
                    return true;
                }
            }
        }
    }

    /// 5. 检查是否是Unix隐藏文件
    if (input.starts_with(".") && input.length() > 1)
    {
        return true;
    }

    return false;
}

bool XFile::isRelativePath(const std::string& path)
{
    if (path.empty())
        return false;

    // 检查是否是绝对路径
    // Windows绝对路径: C:\ 或 \\server\share
    // Unix绝对路径: 以 / 开头
#ifdef _WIN32
    if (path.length() >= 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
    {
        return false;
    }
    if (path.starts_with("\\\\") || path.starts_with("//"))
    {
        return false;
    }
#else
    if (path.starts_with("/"))
    {
        return false;
    }
#endif

    // 相对路径标识
    return path.starts_with("./") || path.starts_with("../") || path.starts_with(".\\") || path.starts_with("..\\") ||
            path == "." || path == "..";
}

std::string XFile::normalizePath(const std::string& path)
{
    try
    {
        fs::path p(path);

        // 如果是相对路径且不是空，转换为绝对路径
        if (!p.empty() && isRelativePath(path))
        {
            p = fs::absolute(p);
        }

        // 标准化路径格式
        p = p.lexically_normal();

        // 确保目录路径以分隔符结尾
        if (fs::is_directory(p) && !path.empty() && path.back() != '/' && path.back() != '\\')
        {
            p = p / ""; // 添加空路径以保留目录分隔符
        }

        return p.string();
    }
    catch (...)
    {
        return path; // 出错时返回原路径
    }
}

std::string XFile::getParentPath(const std::string& path)
{
    try
    {
        fs::path p(path);
        if (p.has_parent_path())
        {
            return p.parent_path().string();
        }
    }
    catch (...)
    {
        // 忽略错误
    }
    return "";
}

std::string XFile::getFileName(const std::string& path)
{
    try
    {
        fs::path p(path);
        return p.filename().string();
    }
    catch (...)
    {
        return "";
    }
}

std::string XFile::getFileExtension(const std::string& path)
{
    try
    {
        fs::path    p(path);
        std::string ext = p.extension().string();
        if (!ext.empty() && ext[0] == '.')
        {
            ext = ext.substr(1); // 移除开头的点
        }
        return ext;
    }
    catch (...)
    {
        return "";
    }
}

// ==================== 文件检测 ====================

bool XFile::fileExists(const std::string& path)
{
    try
    {
        return fs::exists(path);
    }
    catch (...)
    {
        return false;
    }
}

bool XFile::isDirectory(const std::string& path)
{
    try
    {
        return fs::is_directory(path);
    }
    catch (...)
    {
        return false;
    }
}

bool XFile::isRegularFile(const std::string& path)
{
    try
    {
        return fs::is_regular_file(path);
    }
    catch (...)
    {
        return false;
    }
}

bool XFile::isHiddenFile(const std::string& path)
{
    try
    {
        fs::path    p(path);
        std::string name = p.filename().string();

        // Unix隐藏文件以点开头
        if (!name.empty() && name[0] == '.')
        {
            return true;
        }

#ifdef _WIN32
        // Windows隐藏文件属性检查
        DWORD attrs = GetFileAttributesW(p.wstring().c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES)
        {
            return (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
        }
#endif

        return false;
    }
    catch (...)
    {
        return false;
    }
}

#ifdef _WIN32
bool XFile::isWindowsExecutable(const fs::path& path)
{
    std::string ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext == ".exe" || ext == ".bat" || ext == ".cmd" || ext == ".com" || ext == ".msi";
}
#else
bool XFile::isUnixExecutable(const fs::path& path)
{
    try
    {
        fs::perms p = fs::status(path).permissions();
        return (p & fs::perms::owner_exec) != fs::perms::none || (p & fs::perms::group_exec) != fs::perms::none ||
                (p & fs::perms::others_exec) != fs::perms::none;
    }
    catch (...)
    {
        return false;
    }
}
#endif

auto XFile::isExecutable(const std::string_view& path) -> bool
{
    try
    {
        fs::path p(path);

        /// 先检查是否是常规文件
        if (!fs::is_regular_file(p))
        {
            return false;
        }

#ifdef _WIN32
        return isWindowsExecutable(p);
#else
        return isUnixExecutable(p);
#endif
    }
    catch (...)
    {
        return false;
    }
}

// ==================== 目录遍历 ====================

std::vector<XFile::FileEntry> XFile::listDirectory(const std::string& dirPath, bool showHidden,
                                                   const std::string& prefix)
{
    std::vector<FileEntry> entries;

    try
    {
        fs::path dir(dirPath);
        if (!fs::exists(dir) || !fs::is_directory(dir))
        {
            return entries;
        }

        for (const auto& entry : fs::directory_iterator(dir,
                                                        fs::directory_options::skip_permission_denied |
                                                                fs::directory_options::follow_directory_symlink))
        {
            try
            {
                std::string name = entry.path().filename().string();

                // 过滤隐藏文件
                if (!showHidden && !prefix.empty() && name[0] == '.')
                {
                    continue;
                }

                // 过滤前缀不匹配的
                if (!prefix.empty() && !name.starts_with(prefix))
                {
                    continue;
                }

                FileEntry fileEntry;
                fileEntry.path        = entry.path().string();
                fileEntry.name        = name;
                fileEntry.isDirectory = entry.is_directory();

                if (entry.is_regular_file())
                {
                    fileEntry.size         = entry.file_size();
                    fileEntry.isExecutable = isExecutable(entry.path().string());
                }
                else
                {
                    fileEntry.size         = 0;
                    fileEntry.isExecutable = false;
                }

                entries.push_back(fileEntry);
            }
            catch (...)
            {
                // 跳过无法访问的文件
                continue;
            }
        }

        // 排序：目录在前，文件在后，按名称排序
        std::sort(entries.begin(), entries.end(),
                  [](const FileEntry& a, const FileEntry& b)
                  {
                      if (a.isDirectory != b.isDirectory)
                      {
                          return a.isDirectory > b.isDirectory; // 目录在前
                      }
                      return a.name < b.name;
                  });
    }
    catch (...)
    {
        // 忽略目录访问错误
    }

    return entries;
}

std::vector<std::string> XFile::findFiles(const std::string&                           dirPath,
                                          const std::function<bool(const FileEntry&)>& filter)
{
    std::vector<std::string> results;

    try
    {
        auto entries = listDirectory(dirPath, true);
        for (const auto& entry : entries)
        {
            if (filter(entry))
            {
                results.push_back(entry.path);
            }
        }
    }
    catch (...)
    {
        // 忽略错误
    }

    return results;
}

// ==================== 路径补全 ====================

std::vector<XFile::PathCompletion> XFile::getPathCompletions(const std::string& partialPath, bool showHidden)
{
    std::vector<PathCompletion> completions;

    try
    {
        fs::path inputPath(partialPath);

        // 处理空输入
        if (partialPath.empty())
        {
            inputPath = ".";
        }

        // 确定基路径和前缀
        fs::path    basePath;
        std::string prefix;

        if (fs::exists(inputPath) && fs::is_directory(inputPath))
        {
            basePath = inputPath;
            prefix   = "";
        }
        else
        {
            basePath = inputPath.parent_path();
            prefix   = inputPath.filename().string();

            if (basePath.empty())
            {
                basePath = ".";
            }
        }

        // 确保基路径存在
        if (!fs::exists(basePath) || !fs::is_directory(basePath))
        {
            return completions;
        }

        // 遍历目录
        auto entries = listDirectory(basePath.string(), showHidden, prefix);

        for (const auto& entry : entries)
        {
            PathCompletion completion;

            // 构建补全字符串
            if (basePath.string() == ".")
            {
                // 当前目录
                completion.completion = entry.name;
            }
            else
            {
                // 需要构建相对或绝对路径
                fs::path fullPath(basePath);
                fullPath /= entry.name;

                // 根据输入决定返回格式
                if (partialPath.empty() || fs::path(partialPath).is_relative())
                {
                    // 保持相对路径
                    completion.completion = fullPath.lexically_relative(fs::current_path()).string();
                }
                else
                {
                    // 使用绝对路径
                    completion.completion = fullPath.string();
                }
            }

            // 添加目录分隔符
            if (entry.isDirectory)
            {
                if (!completion.completion.empty() && completion.completion.back() != '/' &&
                    completion.completion.back() != '\\')
                {
                    completion.completion += separator();
                }
            }

            completion.isDirectory  = entry.isDirectory;
            completion.isExecutable = entry.isExecutable;

            completions.push_back(completion);
        }

        // 排序
        std::sort(completions.begin(), completions.end(),
                  [](const PathCompletion& a, const PathCompletion& b)
                  {
                      if (a.isDirectory != b.isDirectory)
                      {
                          return a.isDirectory > b.isDirectory; // 目录在前
                      }
                      return a.completion < b.completion;
                  });
    }
    catch (...)
    {
        // 忽略文件系统错误
    }

    return completions;
}

// ==================== 智能路径补全 ====================

std::vector<XFile::PathCompletion> XFile::smartPathCompletion(const std::string& input,
                                                              const std::string& currentWorkingDir)
{
    std::vector<PathCompletion> completions;

    // 提取路径部分
    std::string pathPart = extractPathPart(input);
    if (pathPart.empty())
    {
        // 如果没有明确的路径部分，检查整个输入
        if (isPathInput(input))
        {
            pathPart = input;
        }
        else
        {
            return completions; // 不是路径输入
        }
    }

    // 计算输入中的前缀部分（非路径部分）
    std::string prefixPart;
    size_t      pathPos = input.find(pathPart);
    if (pathPos != std::string::npos)
    {
        prefixPart = input.substr(0, pathPos);
    }

    // 获取路径补全
    auto pathCompletions = getPathCompletions(pathPart, shouldShowHiddenFiles());

    // 将补全结果与前缀合并
    for (auto& comp : pathCompletions)
    {
        // 智能处理补全字符串
        if (!prefixPart.empty())
        {
            // 如果前缀以空格结尾，直接拼接
            if (!prefixPart.empty() && prefixPart.back() == ' ')
            {
                comp.completion = prefixPart + comp.completion;
            }
            // 否则，我们需要替换路径部分
            else if (pathPos != std::string::npos)
            {
                std::string newInput = input;
                newInput.replace(pathPos, pathPart.length(), comp.completion);
                comp.completion = newInput;
            }
        }

        completions.push_back(comp);
    }

    return completions;
}

// ==================== 文件信息 ====================

std::optional<uintmax_t> XFile::getFileSize(const std::string& path)
{
    try
    {
        if (fs::exists(path) && fs::is_regular_file(path))
        {
            return fs::file_size(path);
        }
    }
    catch (...)
    {
        // 忽略错误
    }
    return std::nullopt;
}

std::string XFile::formatFileSize(uintmax_t size)
{
    const char* units[]       = { "B", "KB", "MB", "GB", "TB", "PB" };
    int         unitIndex     = 0;
    double      formattedSize = static_cast<double>(size);

    while (formattedSize >= 1024.0 && unitIndex < 5)
    {
        formattedSize /= 1024.0;
        unitIndex++;
    }

    std::stringstream ss;
    if (unitIndex == 0)
    {
        ss << size << " " << units[unitIndex];
    }
    else if (formattedSize < 10.0)
    {
        ss << std::fixed << std::setprecision(1) << formattedSize << " " << units[unitIndex];
    }
    else
    {
        ss << std::fixed << std::setprecision(0) << formattedSize << " " << units[unitIndex];
    }

    return ss.str();
}

std::string XFile::getFileTypeDescription(const std::string& path)
{
    try
    {
        fs::path p(path);

        if (!fs::exists(p))
        {
            return "不存在的文件";
        }

        if (fs::is_directory(p))
        {
            return "目录";
        }

        if (fs::is_symlink(p))
        {
            return "符号链接";
        }

        if (fs::is_regular_file(p))
        {
            if (isExecutable(path))
            {
                return "可执行文件";
            }

            // 根据扩展名判断文件类型
            std::string ext = getFileExtension(path);
            if (ext.empty())
            {
                return "文件";
            }

            // 常见的文件类型
            static const std::map<std::string, std::string> typeMap = { { "txt", "文本文件" },   { "cpp", "C++源文件" },
                                                                        { "h", "头文件" },       { "jpg", "JPEG图像" },
                                                                        { "png", "PNG图像" },    { "mp4", "MP4视频" },
                                                                        { "mp3", "MP3音频" },    { "pdf", "PDF文档" },
                                                                        { "zip", "ZIP压缩文件" } };

            std::string lowerExt = ext;
            std::ranges::transform(lowerExt, lowerExt.begin(), ::tolower);

            auto it = typeMap.find(lowerExt);
            if (it != typeMap.end())
            {
                return it->second;
            }

            return ext + " 文件";
        }

        return "特殊文件";
    }
    catch (...)
    {
        return "未知类型";
    }
}

std::string XFile::separator()
{
    std::string dirPath;
#ifdef _WIN32
    dirPath += '\\';
#else
    dirPath += '/';
#endif

    return dirPath;
}

std::string XFile::getHomeDirectory()
{
#ifdef _WIN32
    wchar_t* path = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path) == S_OK)
    {
        std::wstring wpath(path);
        CoTaskMemFree(path);

        // 转换为UTF-8
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), static_cast<int>(wpath.length()), nullptr, 0,
                                              nullptr, nullptr);
        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), static_cast<int>(wpath.length()), &result[0], size_needed,
                            nullptr, nullptr);
        return result;
    }
#else
    const char* home = getenv("HOME");
    if (home)
    {
        return home;
    }

    // 回退到passwd文件
    struct passwd* pw = getpwuid(getuid());
    if (pw)
    {
        return pw->pw_dir;
    }
#endif

    return ".";
}

std::string XFile::getCurrentWorkingDirectory()
{
    try
    {
        return fs::current_path().string();
    }
    catch (...)
    {
        return ".";
    }
}

bool XFile::setCurrentWorkingDirectory(const std::string& path)
{
    try
    {
        if (fs::exists(path) && fs::is_directory(path))
        {
            fs::current_path(path);
            return true;
        }
    }
    catch (...)
    {
        // 忽略错误
    }
    return false;
}

/// ==================== 配置 ====================

auto XFile::shouldShowHiddenFiles() -> bool
{
    return g_showHiddenFiles;
}

void XFile::setShowHiddenFiles(bool show)
{
    g_showHiddenFiles = show;
}
