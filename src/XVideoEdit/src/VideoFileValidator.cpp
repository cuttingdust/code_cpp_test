#include "VideoFileValidator.h"

#include "XFile.h"
#include "XExec.h"

#include <fstream>
#include <algorithm>
#include <iostream>

auto VideoFileValidator::isVideoFile(const std::string& filePath, std::string& errorMsg, ValidationLevel level) -> bool
{
    try
    {
        /// 首先检查文件是否存在且可读
        if (!fs::exists(filePath))
        {
            errorMsg = "文件不存在: " + filePath;
            return false;
        }

        if (!fs::is_regular_file(filePath))
        {
            errorMsg = "不是普通文件: " + filePath;
            return false;
        }

        /// 按指定级别进行检查
        switch (level)
        {
            case ValidationLevel::ExtensionOnly:
                return isVideoFileByExtension(filePath, errorMsg);

            case ValidationLevel::MagicNumber:
                if (!isVideoFileByExtension(filePath, errorMsg))
                {
                    return false;
                }
                return isVideoFileByMagicNumber(filePath, errorMsg);

            case ValidationLevel::FFmpegProbe:
            default:
                if (!isVideoFileByExtension(filePath, errorMsg))
                {
                    return false;
                }
                if (!isVideoFileByMagicNumber(filePath, errorMsg))
                {
                    /// 魔数检查失败，但继续尝试FFmpeg检测
                    std::cout << "警告: 魔数检查失败，尝试FFmpeg检测..." << std::endl;
                }
                return isVideoFileByFFmpeg(filePath, errorMsg);
        }
    }
    catch (const std::exception& e)
    {
        errorMsg = "视频文件验证异常: " + std::string(e.what());
        return false;
    }
}

auto VideoFileValidator::isVideoFileByExtension(const std::string& filePath, std::string& errorMsg) -> bool
{
    fs::path    filePathObj(filePath);
    std::string extension = filePathObj.extension().string();

    if (extension.empty())
    {
        errorMsg = "文件没有扩展名: " + filePath;
        return false;
    }

    /// 转换为小写
    std::string lowerExtension = extension;
    std::ranges::transform(lowerExtension, lowerExtension.begin(), [](unsigned char c) { return std::tolower(c); });

    const auto& videoExtensions = getVideoExtensions();
    if (!videoExtensions.contains(lowerExtension))
    {
        errorMsg = "文件扩展名 '" + extension + "' 不是已知的视频格式";
        return false;
    }

    return true;
}

auto VideoFileValidator::isVideoFileByMagicNumber(const std::string& filePath, std::string& errorMsg) -> bool
{
    try
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            errorMsg = "无法打开文件: " + filePath;
            return false;
        }

        /// 读取文件头部（最多16字节）
        unsigned char header[16];
        file.read(reinterpret_cast<char*>(header), sizeof(header));

        if (file.gcount() < 4)
        { /// 至少需要4字节
            errorMsg = "文件太小或无法读取文件头";
            return false;
        }

        /// 检查常见的视频文件魔数

        /// 1. MP4/MOV/QuickTime (ftyp)
        /// 格式: ....ftypxxxx
        if (file.gcount() >= 8 && header[4] == 'f' && header[5] == 't' && header[6] == 'y' && header[7] == 'p')
        {
            return true;
        }

        /// 2. AVI (RIFF....AVI)
        if (file.gcount() >= 12 && header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'F' &&
            header[8] == 'A' && header[9] == 'V' && header[10] == 'I' && header[11] == ' ')
        {
            return true;
        }

        /// 3. AVI (RIFF....AVIX)
        if (file.gcount() >= 12 && header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'F' &&
            header[8] == 'A' && header[9] == 'V' && header[10] == 'I' && header[11] == 'X')
        {
            return true;
        }

        /// 4. WMV/ASF
        static const unsigned char wmvMagic[] = { 0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11,
                                                  0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C };
        if (file.gcount() >= 12 && std::memcmp(header, wmvMagic, 12) == 0)
        {
            return true;
        }

        /// 5. FLV (前3字节是"FLV")
        if (header[0] == 'F' && header[1] == 'L' && header[2] == 'V')
        {
            return true;
        }

        /// 6. MKV/WebM (Matroska格式)
        if (header[0] == 0x1A && header[1] == 0x45 && header[2] == 0xDF && header[3] == 0xA3)
        {
            return true;
        }

        /// 7. MPEG/MPG (以0x000001Bx开头)
        if (header[0] == 0x00 && header[1] == 0x00 && header[2] == 0x01 && (header[3] >= 0xB0 && header[3] <= 0xBF))
        {
            return true;
        }

        /// 8. 3GP/3G2 (ftyp3g)
        if (file.gcount() >= 12 && header[4] == 'f' && header[5] == 't' && header[6] == 'y' && header[7] == 'p' &&
            header[8] == '3' && header[9] == 'g')
        {
            return true;
        }

        /// 9. OGG/OGV (前4字节是"OggS")
        if (header[0] == 'O' && header[1] == 'g' && header[2] == 'g' && header[3] == 'S')
        {
            return true;
        }

        /// 10. RM/RMVB (RealMedia)
        /// RealMedia文件通常以".RMF"开头
        if (header[0] == '.' && header[1] == 'R' && header[2] == 'M' && header[3] == 'F')
        {
            return true;
        }

        /// 11. SWF (Flash视频)
        /// SWF文件以"FWS"或"CWS"开头
        if ((header[0] == 'F' && header[1] == 'W' && header[2] == 'S') ||
            (header[0] == 'C' && header[1] == 'W' && header[2] == 'S'))
        {
            return true;
        }

        /// 12. VOB (DVD视频文件)
        /// VOB文件通常以MPEG-PS格式开头
        if (header[0] == 0x00 && header[1] == 0x00 && header[2] == 0x01 && header[3] == 0xBA)
        {
            return true;
        }

        /// 13. MOD/TOD (某些摄像机格式)
        /// 这些格式可能需要更复杂的检查

        errorMsg = "文件头魔数与已知视频格式不匹配";
        return false;
    }
    catch (const std::exception& e)
    {
        errorMsg = "魔数检查异常: " + std::string(e.what());
        return false;
    }
}

bool VideoFileValidator::isVideoFileByFFmpeg(const std::string& filePath, std::string& errorMsg)
{
    try
    {
        std::string command = std::string(FFPROBE_PATH) +
                " -v error -select_streams v:0 -show_entries stream=codec_type "
                "-of default=noprint_wrappers=1:nokey=1 \"" +
                filePath + "\"";

        XExec::XResult result = XExec::execute(command);
        if (result.exitCode != 0)
        {
            errorMsg = "FFmpeg探测失败: " + result.stderrOutput;
            return false;
        }

        std::string output = result.stdoutOutput;
        std::erase(output, '\n');
        std::erase(output, '\r');
        if (output == "video")
        {
            return true;
        }
        else
        {
            errorMsg = "FFmpeg未检测到视频流";
            return false;
        }
    }
    catch (const std::exception& e)
    {
        errorMsg = "FFmpeg探测异常: " + std::string(e.what());
        return false;
    }
}


const std::set<std::string>& VideoFileValidator::getVideoExtensions()
{
    static const std::set<std::string> videoExtensions = { /// MP4 相关
                                                           ".mp4", ".m4v", ".m4a", ".m4b", ".m4p", ".m4r",
                                                           /// AVI
                                                           ".avi",
                                                           /// MOV/QuickTime
                                                           ".mov", ".qt",
                                                           /// WMV/ASF
                                                           ".wmv", ".asf", ".asx",
                                                           /// FLV
                                                           ".flv", ".f4v", ".f4p", ".f4a", ".f4b",
                                                           /// MKV
                                                           ".mkv",
                                                           /// WebM
                                                           ".webm",
                                                           /// MPEG
                                                           ".mpeg", ".mpg", ".mpe", ".m1v", ".m2v", ".mpv", ".mp2",
                                                           ".m2p",
                                                           /// MPEG Program Stream
                                                           ".vob", ".evo",
                                                           /// MPEG Transport Stream
                                                           ".ts", ".mts", ".m2ts", ".tsv", ".tsa",
                                                           /// OGG
                                                           ".ogv", ".ogg", ".oga",
                                                           /// 3GP/3G2
                                                           ".3gp", ".3g2", ".3gpp", ".3gpp2",
                                                           /// RM/RMVB
                                                           ".rm", ".rmvb",
                                                           /// DV
                                                           ".dv", ".dif",
                                                           /// AMV
                                                           ".amv",
                                                           /// MXF
                                                           ".mxf",
                                                           /// ROQ
                                                           ".roq",
                                                           /// NSV
                                                           ".nsv",
                                                           /// Flic
                                                           ".fli", ".flc",
                                                           /// RealMedia
                                                           ".ra", ".ram",
                                                           /// VIVO
                                                           ".viv",
                                                           /// yuv4mpegpipe
                                                           ".y4m",
                                                           /// Matroska
                                                           ".mk3d", ".mka", ".mks",
                                                           /// Bink
                                                           ".bik", ".bk2",
                                                           /// Smacker
                                                           ".smk",
                                                           /// TechSmith Capture
                                                           ".camrec",
                                                           /// Flash Video
                                                           ".swf", ".fla",
                                                           /// Google WebP
                                                           ".webp", /// 虽然不是传统视频，但可以是动画
                                                           /// GIF (动画GIF)
                                                           ".gif",
                                                           /// APNG
                                                           ".apng",
                                                           /// MJPEG
                                                           ".mjpeg", ".mjpg",
                                                           /// Apple iPhone
                                                           ".mqv",
                                                           /// Sony PSP
                                                           ".psp",
                                                           /// Wii
                                                           ".thp",
                                                           /// Xbox
                                                           ".wmv", ".wma",
                                                           /// HDV
                                                           ".m2t", ".m2ts",
                                                           /// AVCHD
                                                           ".mts",
                                                           /// MOD (某些摄像机格式)
                                                           ".mod", ".tod",
                                                           /// AV1
                                                           ".av1",
                                                           /// VP8/VP9
                                                           ".ivf",
                                                           /// 其他
                                                           ".dat", ".vcd", ".svcd", ".divx", ".xvid"
    };

    return videoExtensions;
}
