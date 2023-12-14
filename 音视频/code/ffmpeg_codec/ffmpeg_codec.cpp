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


void codec_audio2() {
  std::string file_name = "believe.aac";
  std::string out_file_name = "test.pcm";

  AVFormatContext* context = avformat_alloc_context();
  if (!context) {
    std::cerr << "Could not allocate format context" << std::endl;
    return;
  }

  if (avformat_open_input(&context, file_name.c_str(), NULL, NULL) < 0) {
    std::cerr << "Could not open input file" << std::endl;
    avformat_free_context(context);
    return;
  }

  av_dump_format(context, 0, file_name.c_str(), 0);

  AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
  if (!codec) {
    std::cerr << "Codec not found" << std::endl;
    avformat_free_context(context);
    return;
  }

  AVCodecParserContext* parser_context = av_parser_init(AV_CODEC_ID_AAC);
  if (!parser_context) {
    std::cerr << "Could not allocate parser context" << std::endl;
    avformat_free_context(context);
    return;
  }

  AVCodecContext* codec_context = avcodec_alloc_context3(codec);
  if (!codec_context) {
    std::cerr << "Could not allocate codec context" << std::endl;
    av_parser_close(parser_context);
    avformat_free_context(context);
    return;
  }

  if (avcodec_open2(codec_context, codec, NULL) < 0) {
    std::cerr << "Could not open codec" << std::endl;
    avcodec_free_context(&codec_context);
    av_parser_close(parser_context);
    avformat_free_context(context);
    return;
  }

  std::ifstream file_in(file_name, std::ios::binary);
  if (!file_in) {
    std::cerr << "Could not open input file" << std::endl;
    avcodec_free_context(&codec_context);
    av_parser_close(parser_context);
    avformat_free_context(context);
    return;
  }

  std::ofstream file_out(out_file_name, std::ios::binary);
  if (!file_out) {
    std::cerr << "Could not open output file" << std::endl;
    avcodec_free_context(&codec_context);
    av_parser_close(parser_context);
    avformat_free_context(context);
    return;
  }

  uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE] = { 0 };
  uint8_t* data = inbuf;

  AVPacket* pkt = av_packet_alloc();
  if (!pkt) {
    std::cerr << "Could not allocate packet" << std::endl;
    avcodec_free_context(&codec_context);
    av_parser_close(parser_context);
    avformat_free_context(context);
    return;
  }

  AVFrame* decoded_frame = av_frame_alloc();
  if (!decoded_frame) {
    std::cerr << "Could not allocate frame" << std::endl;
    av_packet_free(&pkt);
    avcodec_free_context(&codec_context);
    av_parser_close(parser_context);
    avformat_free_context(context);
    return;
  }

  int data_size = 0;
  int read_size = AUDIO_INBUF_SIZE;
  int file_size = std::filesystem::file_size(file_name);
  int read_pos = 0;

  while (read_pos < file_size) {
    if (data_size < AUDIO_INBUF_SIZE) {
      if (data_size != 0) {
        memmove(inbuf, data, data_size);
        data = inbuf;
        read_size = AUDIO_INBUF_SIZE - data_size;
      } else {
        read_size = AUDIO_INBUF_SIZE;
      }

      if (read_pos + read_size > file_size) {
        read_size = file_size - read_pos;
      }
      file_in.read(reinterpret_cast<char*>(data + data_size), read_size);
      read_pos += read_size;
      data_size += read_size;
    }

    if (!decoded_frame) {
      decoded_frame = av_frame_alloc();
    }

    int ret = av_parser_parse2(parser_context, codec_context, &pkt->data, &pkt->size,
      data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    if (ret < 0) {
      std::cerr << "Error in parsing" << std::endl;
      break;
    }

    data += ret;
    data_size -= ret;

    if (pkt->size) {
      ret = avcodec_send_packet(codec_context, pkt);
      if (ret < 0) {
        std::cerr << "Error sending a packet for decoding" << std::endl;
        break;
      }

      while (ret >= 0) {
        ret = avcodec_receive_frame(codec_context, decoded_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          break;
        }
        else if (ret < 0) {
          std::cerr << "Error during decoding" << std::endl;
          break;
        }

        int smp_size = av_get_bytes_per_sample(codec_context->sample_fmt);
        if (smp_size < 0) {
          std::cerr << "Failed to calculate data size" << std::endl;
          break;
        }

        static int s_print_format = 0;
        if (s_print_format == 0) {
          s_print_format = 1;
          print_sample_format(decoded_frame);
        }

        for (int i = 0; i < decoded_frame->nb_samples; i++) {
          for (int ch = 0; ch < codec_context->channels; ch++) {
            file_out.write(reinterpret_cast<char*>(decoded_frame->data[ch] + smp_size * i), smp_size);
          }
        }
      }
    }
  }

  av_frame_free(&decoded_frame);
  av_packet_free(&pkt);
  avcodec_free_context(&codec_context);
  av_parser_close(parser_context);
  avformat_free_context(context);
}

int main() {
  std::cout << "Hello World!" << av_version_info() << std::endl;
  //codec_audio();
  codec_audio2();
  //codec_audio2("believe.aac", "test.pcm");
}