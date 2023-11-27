#include <QtCore/QCoreApplication>
#include <iostream>
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>

extern "C"{
#include <libavutil/avutil.h>
#include <SDL.h>
}

void testEvent() {
  SDL_Event event;
  while (1) {
    SDL_WaitEvent(&event);
    switch (event.type) {
    case SDL_KEYDOWN:
      std::cout << "key event = " << event.key.keysym.sym << std::endl;
      break;
    }
  }
}

void testRender(SDL_Renderer* renderer, SDL_Texture* texture) {
  SDL_Rect rect = { 0,0,10,20 };

  int count = 0;


  while (++count < 30) {
    rect.x = rand() % 640;
    rect.y = rand() % 480;

    SDL_SetRenderTarget(renderer, texture);//设置 渲染目标为纹理
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);//设置纹理背景色
    SDL_RenderClear(renderer);//清屏

    SDL_RenderDrawRect(renderer, &rect);//绘制长方形
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);//设置长方形颜色
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderTarget(renderer, NULL);//恢复默认，渲染目标为窗口
    SDL_RenderCopy(renderer, texture, NULL, NULL);//拷贝纹理到CPU

    SDL_RenderPresent(renderer);//输出到窗口上
    SDL_Delay(300);
  }

}




// 每次读取2帧数据, 以1024个采样点一帧 2通道 16bit采样点为例
#define PCM_BUFFER_SIZE (1024*2*2*2)

// 音频PCM数据缓存
static Uint8* s_audio_buf = NULL;
// 目前读取的位置
static Uint8* s_audio_pos = NULL;
// 缓存结束位置
static Uint8* s_audio_end = NULL;


//音频设备回调函数
void fill_audio_pcm(void* udata, Uint8* stream, int len)
{
  SDL_memset(stream, 0, len);

  if (s_audio_pos >= s_audio_end) {// 数据读取完毕
    return;
  }

  // 数据够了就读预设长度，数据不够就只读部分（不够的时候剩多少就读取多少）
  int remain_buffer_len = s_audio_end - s_audio_pos;
  len = (len < remain_buffer_len) ? len : remain_buffer_len;
  // 拷贝数据到stream并调整音量
  SDL_MixAudio(stream, s_audio_pos, len, SDL_MIX_MAXVOLUME / 8);
  printf("len = %d\n", len);
  s_audio_pos += len;  // 移动缓存指针
}

// 提取PCM文件
// ffmpeg -i input.mp4 -t 20 -codec:a pcm_s16le -ar 44100 -ac 2 -f s16le 44100_16bit_2ch.pcm
// 测试PCM文件
// ffplay -ar 44100 -ac 2 -f s16le 44100_16bit_2ch.pcm
void testPcm() {
  int ret = -1;
  FILE* audio_fd = NULL;
  SDL_AudioSpec spec;
  const char* path = "44100_16bit_2ch.pcm";
  // 每次缓存的长度
  size_t read_buffer_len = 0;

  //SDL initialize
  if (SDL_Init(SDL_INIT_AUDIO)) {   // 支持AUDIO
    std::cout << "error : Could not initialize SDL - %s\n" << SDL_GetError() << std::endl;
  }

  //打开PCM文件
  audio_fd = fopen(path, "rb");
  if (!audio_fd) {
    fprintf(stderr, "Failed to open pcm file!\n");
  }

  s_audio_buf = (uint8_t*)malloc(PCM_BUFFER_SIZE);

  // 音频参数设置SDL_AudioSpec
  spec.freq = 44100;          // 采样频率
  spec.format = AUDIO_S16SYS; // 采样点格式
  spec.channels = 2;          // 2通道
  spec.silence = 0;
  spec.samples = 1024;       // 23.2ms -> 46.4ms 每次读取的采样数量，多久产生一次回调和 samples
  spec.callback = fill_audio_pcm; // 回调函数
  spec.userdata = NULL;

  //打开音频设备
  if (SDL_OpenAudio(&spec, NULL)) {
    std::cout << "error : Failed to open audio device,%s\n" << SDL_GetError() << std::endl;
  }

  //play audio
  SDL_PauseAudio(0);

  int data_count = 0;
  while (1) {
    // 从文件读取PCM数据
    read_buffer_len = fread(s_audio_buf, 1, PCM_BUFFER_SIZE, audio_fd);
    if (read_buffer_len == 0) {
      break;
    }
    data_count += read_buffer_len; // 统计读取的数据总字节数
    std::cout << "now playing " << data_count << " bytes data." << std::endl;
    s_audio_end = s_audio_buf + read_buffer_len;    // 更新buffer的结束位置
    s_audio_pos = s_audio_buf;  // 更新buffer的起始位置
    //the main thread wait for a moment
    while (s_audio_pos < s_audio_end) {
      SDL_Delay(10);  // 等待PCM数据消耗
    } 
  }
  printf("play PCM finish\n");
  // 关闭音频设备
  SDL_CloseAudio();
}

int main(int argc, char* argv[]) {
  QCoreApplication a(argc, argv);
  printf("nihao,%s\n", av_version_info());

  SDL_Window* window;
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("test window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_OPENGL);

  std::cout << "window = " << window << std::endl;
  if (!window) {
    return 1;
  }
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_Texture* texture = SDL_CreateTexture(renderer, 
                                           SDL_PIXELFORMAT_ABGR8888,
                                           SDL_TEXTUREACCESS_TARGET, 
                                           640, 480);
 
  //testEvent();
  //testRender(renderer, texture);
  testPcm();

  SDL_Delay(5000);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(texture);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return a.exec();
}
