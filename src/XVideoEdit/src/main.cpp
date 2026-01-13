#include <iostream>

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "zh_CN.UTF-8");

#ifdef FFMPEG_PATH
    std::string ffmpeg_cmd = FFMPEG_PATH;
#else
    std::string ffmpeg_cmd = "ffmpeg";
#endif

    std::string command = ffmpeg_cmd + " -version";
    std::cout << "检查FFmpeg: " << command << std::endl;

    int result = std::system(command.c_str());

    if (result == 0)
    {
        std::cout << "FFmpeg可用！" << std::endl;
    }
    else
    {
        std::cout << "FFmpeg不可用" << std::endl;
    }


    getchar();
    return 0;
}
