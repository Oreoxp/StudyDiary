// ffmpeg_codec.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#include <fstream>

#include <cerrno>
#include <cstring>
#include <filesystem>
extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

#define AUDIO_INBUF_SIZE 20480


static void print_sample_format(const AVFrame* frame) {
  printf("ar-samplerate: %uHz\n", frame->sample_rate);
  printf("ac-channel: %u\n", frame->channels);
  printf("f-format: %u\n", frame->format);// 格式需要注意，实际存储到本地文件时已经改成交错模式
}


void codec_audio() {
  std::string file_name = "believe.aac";
  std::string out_file_name = "test.pcm";

  AVFormatContext* context = avformat_alloc_context(); 

  avformat_open_input(&context, file_name.c_str(), NULL, NULL);

  av_dump_format(context, 0, file_name.c_str(), 0);


  //read packet
  AVPacket* pkt = av_packet_alloc();
  
  AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
  if (!codec) {
    std::cout << "Codec not found" << std::endl;
    return;
  }

  AVCodecParserContext * parser_context =  av_parser_init(AV_CODEC_ID_AAC);
  AVCodecContext* codec_context = avcodec_alloc_context3(codec);

  avcodec_open2(codec_context, codec, NULL);

  std::ifstream file_in;
  file_in.open(file_name, std::ios::binary);
  if (!file_in) {
    std::cout << "err open failed" << std::endl;
  }
  
  std::ofstream file_out(out_file_name, std::ios::binary);

  uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
  uint8_t* data = NULL;
  data = inbuf;

  AVFrame* decoded_frame = NULL;

  int read_size = AUDIO_INBUF_SIZE;
  int file_size = std::filesystem::file_size(file_name);
  int read_pos = 0;
  int data_size = 0;
  do {
    if (data_size < 4096) {
      if (data_size != 0) {
        memmove(inbuf, data, data_size);
        data = inbuf;
      }

      read_size = AUDIO_INBUF_SIZE;
      file_in.seekg(read_pos);
      if (read_pos + read_size - file_size > 0) {
        read_size = file_size - read_pos;
      }
      file_in.read((char*)data + data_size, read_size);
      read_pos += read_size;
      data_size += read_size;
    }

    if (!decoded_frame) {
      decoded_frame = av_frame_alloc();
    }
    int ret = av_parser_parse2(parser_context, codec_context, &pkt->data, &pkt->size,
      data, data_size,
      AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    if (ret < 0) {
      std::cout << "parse failed" << std::endl;
      return;
    }
    data += ret;   // 跳过已经解析的数据
    data_size -= ret;   // 对应的缓存大小也做相应减小

    if (pkt->size) {
      ret = avcodec_send_packet(codec_context, pkt);
      if (ret == AVERROR(EAGAIN)) {
        std::cout << "EAGAIN" << std::endl;
      } else if (ret < 0) {
        std::cout << "< 0" << ret << std::endl;
        return;
      }

      while (ret >= 0) {
        // 对于frame, avcodec_receive_frame内部每次都先调用

        int smp_size = 0;
        ret = avcodec_receive_frame(codec_context, decoded_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        }
        else if (ret < 0) {
          fprintf(stderr, "Error during decoding\n");
          exit(1);
        }
        smp_size = av_get_bytes_per_sample(codec_context->sample_fmt);
        if (smp_size < 0) {
          /* This should not occur, checking just for paranoia */
          fprintf(stderr, "Failed to calculate data size\n");
          exit(1);
        }
        static int s_print_format = 0;
        if (s_print_format == 0) {
          s_print_format = 1;
          print_sample_format(decoded_frame);
        }
        /**
            P表示Planar（平面），其数据格式排列方式为 :
            LLLLLLRRRRRRLLLLLLRRRRRRLLLLLLRRRRRRL...（每个LLLLLLRRRRRR为一个音频帧）
            而不带P的数据格式（即交错排列）排列方式为：
            LRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRL...（每个LR为一个音频样本）
         播放范例：   ffplay -ar 48000 -ac 2 -f f32le believe.pcm
          */
        for (int i = 0; i < decoded_frame->nb_samples; i++) {
          for (int ch = 0; ch < codec_context->channels; ch++)  // 交错的方式写入, 大部分float的格式输出
            file_out.write((char*)decoded_frame->data[ch] + smp_size * i, smp_size);
        }
      }
    }

  } while (read_size == AUDIO_INBUF_SIZE);


  /* 冲刷解码器 */
  pkt->data = NULL;   // 让其进入drain mode
  pkt->size = 0;


  avcodec_free_context(&codec_context);
  av_parser_close(parser_context);
  av_frame_free(&decoded_frame);
  av_packet_free(&pkt);
  //avcodec_find_decoder();

}



int main() {
  std::cout << "Hello World!" << av_version_info() << std::endl;
  codec_audio();
  //codec_audio2("believe.aac", "test.pcm");
}