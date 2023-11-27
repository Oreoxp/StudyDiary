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

    SDL_SetRenderTarget(renderer, texture);//���� ��ȾĿ��Ϊ����
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);//����������ɫ
    SDL_RenderClear(renderer);//����

    SDL_RenderDrawRect(renderer, &rect);//���Ƴ�����
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);//���ó�������ɫ
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderTarget(renderer, NULL);//�ָ�Ĭ�ϣ���ȾĿ��Ϊ����
    SDL_RenderCopy(renderer, texture, NULL, NULL);//��������CPU

    SDL_RenderPresent(renderer);//�����������
    SDL_Delay(300);
  }

}




// ÿ�ζ�ȡ2֡����, ��1024��������һ֡ 2ͨ�� 16bit������Ϊ��
#define PCM_BUFFER_SIZE (1024*2*2*2)

// ��ƵPCM���ݻ���
static Uint8* s_audio_buf = NULL;
// Ŀǰ��ȡ��λ��
static Uint8* s_audio_pos = NULL;
// �������λ��
static Uint8* s_audio_end = NULL;


//��Ƶ�豸�ص�����
void fill_audio_pcm(void* udata, Uint8* stream, int len)
{
  SDL_memset(stream, 0, len);

  if (s_audio_pos >= s_audio_end) {// ���ݶ�ȡ���
    return;
  }

  // ���ݹ��˾Ͷ�Ԥ�賤�ȣ����ݲ�����ֻ�����֣�������ʱ��ʣ���پͶ�ȡ���٣�
  int remain_buffer_len = s_audio_end - s_audio_pos;
  len = (len < remain_buffer_len) ? len : remain_buffer_len;
  // �������ݵ�stream����������
  SDL_MixAudio(stream, s_audio_pos, len, SDL_MIX_MAXVOLUME / 8);
  printf("len = %d\n", len);
  s_audio_pos += len;  // �ƶ�����ָ��
}

// ��ȡPCM�ļ�
// ffmpeg -i input.mp4 -t 20 -codec:a pcm_s16le -ar 44100 -ac 2 -f s16le 44100_16bit_2ch.pcm
// ����PCM�ļ�
// ffplay -ar 44100 -ac 2 -f s16le 44100_16bit_2ch.pcm
void testPcm() {
  int ret = -1;
  FILE* audio_fd = NULL;
  SDL_AudioSpec spec;
  const char* path = "44100_16bit_2ch.pcm";
  // ÿ�λ���ĳ���
  size_t read_buffer_len = 0;

  //SDL initialize
  if (SDL_Init(SDL_INIT_AUDIO)) {   // ֧��AUDIO
    std::cout << "error : Could not initialize SDL - %s\n" << SDL_GetError() << std::endl;
  }

  //��PCM�ļ�
  audio_fd = fopen(path, "rb");
  if (!audio_fd) {
    fprintf(stderr, "Failed to open pcm file!\n");
  }

  s_audio_buf = (uint8_t*)malloc(PCM_BUFFER_SIZE);

  // ��Ƶ��������SDL_AudioSpec
  spec.freq = 44100;          // ����Ƶ��
  spec.format = AUDIO_S16SYS; // �������ʽ
  spec.channels = 2;          // 2ͨ��
  spec.silence = 0;
  spec.samples = 1024;       // 23.2ms -> 46.4ms ÿ�ζ�ȡ�Ĳ�����������ò���һ�λص��� samples
  spec.callback = fill_audio_pcm; // �ص�����
  spec.userdata = NULL;

  //����Ƶ�豸
  if (SDL_OpenAudio(&spec, NULL)) {
    std::cout << "error : Failed to open audio device,%s\n" << SDL_GetError() << std::endl;
  }

  //play audio
  SDL_PauseAudio(0);

  int data_count = 0;
  while (1) {
    // ���ļ���ȡPCM����
    read_buffer_len = fread(s_audio_buf, 1, PCM_BUFFER_SIZE, audio_fd);
    if (read_buffer_len == 0) {
      break;
    }
    data_count += read_buffer_len; // ͳ�ƶ�ȡ���������ֽ���
    std::cout << "now playing " << data_count << " bytes data." << std::endl;
    s_audio_end = s_audio_buf + read_buffer_len;    // ����buffer�Ľ���λ��
    s_audio_pos = s_audio_buf;  // ����buffer����ʼλ��
    //the main thread wait for a moment
    while (s_audio_pos < s_audio_end) {
      SDL_Delay(10);  // �ȴ�PCM��������
    } 
  }
  printf("play PCM finish\n");
  // �ر���Ƶ�豸
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
