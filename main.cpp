#include <iostream>

// Need this for CPlusPlus development with this C library
#ifdef __cplusplus
extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
}
#endif

#include <unistd.h>

int main(int, char**)
{
    std::cout << "Lets load an MP3 with FFMPEG! Using version " << av_version_info() << std::endl;
    AVFormatContext* s = NULL;
    av_register_all();
    const char* url = "file:Peanuts-Theme.mp3";
    int ret = avformat_open_input(&s, url, NULL, NULL);
    if (ret < 0) {
        abort();
    } else {
        std::cout << "Successfully loaded " << s->filename << std::endl;
    }

    sleep(5);

}
