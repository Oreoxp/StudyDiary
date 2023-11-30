#include <QtCore/QCoreApplication>
#include <iostream>
#include <string>
#include <fstream>

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}


void funcDemux() {
  std::string filename = "believe.mp4";

  //AVFormatContext 是描述一个媒体文件或媒体流的构成和基本信息的结构体
  AVFormatContext * pFormatCtx = NULL; //输入文件的demux

  int videoindex = -1; //视频流索引
  int audioindex = -1; //音频流索引

  //打开文件
  int ret = avformat_open_input(&pFormatCtx, filename.c_str(), NULL, NULL);
  if (ret != 0) {
    std::cout << "[error] avformat_open_input open failed :" << ret << std::endl;
    return;
  }

  ret = avformat_find_stream_info(pFormatCtx, NULL);
  if (ret != 0) {
    std::cout << "[error] avformat_find_stream_info failed :" << ret << std::endl;
    return;
  }

  std::cout << "open file success" << std::endl;
  av_dump_format(pFormatCtx, 0, filename.c_str(), 0);

  std::cout << std::endl;
  std::cout << "dump format========:" << std::endl;
  std::cout << "medianame(媒体名称)        :" << pFormatCtx->url << std::endl;
  std::cout << "nb_streams(流媒体数量)     :" << pFormatCtx->nb_streams << std::endl;
  std::cout << "bit_rate(比特率)           :" << pFormatCtx->bit_rate << std::endl;
  std::cout << "media average rate(码率)   :" << pFormatCtx->bit_rate / 1024 << "kbps" << std::endl;
  std::cout << "format name(封装格式名称)  :" << pFormatCtx->iformat->name << std::endl;
  std::cout << "duration(时长)             :" << (pFormatCtx->duration/AV_TIME_BASE)/3600 << "h"
                                              << ((pFormatCtx->duration/AV_TIME_BASE)%3600)/60 << "m"
                                              << ((pFormatCtx->duration/AV_TIME_BASE)%3600)%60 << "s" 
                                              << std::endl;

  std::cout << std::endl;
  std::cout << "---------start read [stream]-------" << std::endl;
  std::cout << std::endl;
  for (uint32_t i = 0; i < pFormatCtx->nb_streams; ++i) {
    AVStream* stream = pFormatCtx->streams[i];
    std::cout << "--------------------------------" << std::endl;
    if (AVMEDIA_TYPE_AUDIO == stream->codecpar->codec_type) {
      audioindex = i;
      std::cout << "read audio info========:" << std::endl;
      std::cout << "frame index(index)              :" << stream->index << std::endl;
      std::cout << "audio sample rate(采样率)       :" << stream->codecpar->sample_rate << std::endl;
      std::cout << "audio channels(通道数)          :" << stream->codecpar->channels << std::endl;
      std::cout << "audio bit rate(比特率)          :" << stream->codecpar->bit_rate << std::endl;
      std::cout << "audio frame size(帧长)          :" << stream->codecpar->frame_size << std::endl;
      std::cout << "audio codec name(压缩编码格式)  :" << avcodec_get_name(stream->codecpar->codec_id) << std::endl;
      std::cout << "audio codec format(格式)        :" << av_get_sample_fmt_name((AVSampleFormat)stream->codecpar->format) << std::endl;
      if (stream->duration != AV_NOPTS_VALUE) {
        int duration = stream->duration * av_q2d(stream->time_base);
        std::cout << "audio duration(时长)            :" << (duration) / 3600 << "h"
                                                         << ((duration) % 3600) / 60 << "m"
                                                         << ((duration) % 3600) % 60 << "s"
                                                         << std::endl;
      }
      else {
        std::cout << "audio duration(时长)            :" << "unknown" << std::endl;
      }
    } else if (AVMEDIA_TYPE_VIDEO == stream->codecpar->codec_type) {
      videoindex = i;
      std::cout << "read video info========:" << std::endl;
      std::cout << "frame index(index)              :" << stream->index << std::endl;
      std::cout << "video fps                       :" << av_q2d(stream->avg_frame_rate) << std::endl;
      std::cout << "video width(宽,高)              :" << stream->codecpar->width << "," << stream->codecpar->height << std::endl;
      std::cout << "video bit rate(比特率)          :" << stream->codecpar->bit_rate << std::endl;
      std::cout << "video codec name(压缩编码格式)  :" << avcodec_get_name(stream->codecpar->codec_id) << std::endl;
      if (stream->duration != AV_NOPTS_VALUE) {
        int duration = stream->duration * av_q2d(stream->time_base);
        std::cout << "audio duration(时长)            :" << (duration) / 3600 << "h"
                                                         << ((duration) % 3600) / 60 << "m"
                                                         << ((duration) % 3600) % 60 << "s"
                                                         << std::endl;
      }
      else {
        std::cout << "audio duration(时长)            :" << "unknown" << std::endl;
      }
    } else {
      std::cout << "read other stream" << std::endl;
    }
    std::cout << std::endl;
  }

  std::cout << std::endl;
  std::cout << "---------start read frame-------" << std::endl;
  std::cout << std::endl;

  AVPacket *packet = av_packet_alloc();
  while (av_read_frame(pFormatCtx, packet) >= 0) {
    if (packet->stream_index == videoindex) {
      std::cout << "read video frame======" << std::endl;
      std::cout << "frame pts(时间戳)              :" << packet->pts << std::endl;
      std::cout << "frame dts(解码时间戳)          :" << packet->dts << std::endl;
      std::cout << "frame duration(帧时长s)        :" << packet->duration * av_q2d(pFormatCtx->streams[packet->stream_index]->time_base) << std::endl;
      std::cout << "frame size(帧大小)             :" << packet->size << std::endl;
      std::cout << "frame pos(帧在文件中的位置)    :" << packet->pos << std::endl;
      std::cout << "frame flags(标志)              :" << packet->flags << std::endl;
    } else if (packet->stream_index == audioindex) {
      std::cout << "read audio frame======" << std::endl;
      std::cout << "frame pts(时间戳)              :" << packet->pts << std::endl;
      std::cout << "frame dts(解码时间戳)          :" << packet->dts << std::endl;
      std::cout << "frame duration(帧时长s)        :" << packet->duration * av_q2d(pFormatCtx->streams[packet->stream_index]->time_base) << std::endl;
      std::cout << "frame size(帧大小)             :" << packet->size << std::endl;
      std::cout << "frame pos(帧在文件中的位置)    :" << packet->pos << std::endl;
      std::cout << "frame flags(标志)              :" << packet->flags << std::endl;
    } else {
      std::cout << "read other frame" << std::endl;
    }
    std::cout << std::endl;
    av_packet_unref(packet);
  }

  if (packet){
    av_packet_free(&packet);
  }
}

void funcExtractAAC() {
  std::string input_filename = "believe.mp4";
  std::string output_filename = "out.aac";

  AVFormatContext * pFormatCtx = NULL; //输入文件的demux

  int ret = avformat_open_input(&pFormatCtx, input_filename.c_str(), NULL, NULL);
  if (ret != 0) {
    std::cout << "[error] open input failed";
    return;
  }

  ret = avformat_find_stream_info(pFormatCtx, NULL);
  if (ret != 0) {
    std::cout << "[error] find stream info failed";
    return;
  }

  av_dump_format(pFormatCtx, 0, input_filename.c_str(), 0);

  int audio_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

  if (audio_index < 0) {
    std::cout << "[error] cant find audio stream";
    return;
  }

  std::cout << std::endl;
  std::cout << "find a audio stream index:" << audio_index << std::endl;
  auto audio_stream = pFormatCtx->streams[audio_index];
  std::cout << "info:" << std::endl;
  std::cout << "采样率:" << audio_stream->codecpar->sample_rate << std::endl;
  std::cout << "type:" << av_get_media_type_string(audio_stream->codecpar->codec_type) << std::endl;
  std::cout << "压缩编码格式  :" << avcodec_get_name(audio_stream->codecpar->codec_id) << std::endl;
  std::cout << "audio profile  :" << audio_stream->codecpar->profile<< std::endl;

  if (audio_stream->codecpar->codec_id != AV_CODEC_ID_AAC) {
    std::cout << "[error] is not aac";
    return;
  }

  std::ofstream  output_s;
  output_s.open(output_filename, std::ios::binary);

  AVPacket* pakt = av_packet_alloc();
  while (av_read_frame(pFormatCtx, pakt) >= 0) {
    if (pakt->stream_index == audio_index) {
      //写入head
      char head[7] = { 0 };
      int adtsLen = pakt->size + 7;//包的大小加上 ADTS 头的固定长度（7字节
      head[0] = 0xFF;//syncword  高8bits

      head[1] = 0xf0;         //syncword:0xfff                          低 4 bits
      head[1] |= (0 << 3);        //MPEG Version:0 for MPEG-4,1 for MPEG-2  1 bit
      head[1] |= (0 << 1);    //Layer:0                                 2 bits
      head[1] |= 1;           //protection absent:1                     1 bit

      head[2] = (audio_stream->codecpar->profile) << 6;            //profile:profile               2bits
      head[2] |= (3 & 0x0f) << 2; //sampling frequency index:sampling_frequency_index  4bits
      head[2] |= (0 << 1);             //private bit:0                   1bit
      head[2] |= (audio_stream->codecpar->channels & 0x04) >> 2; //channel configuration:channels  高1bit

      head[3] = (audio_stream->codecpar->channels & 0x03) << 6; //channel configuration:channels 低2bits
      head[3] |= (0 << 5);               //original：0                1bit
      head[3] |= (0 << 4);               //home：0                    1bit
      head[3] |= (0 << 3);               //copyright id bit：0        1bit
      head[3] |= (0 << 2);               //copyright id start：0      1bit
      head[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits

      head[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
      head[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
      head[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
      head[6] = 0xfc;      //11111100       //buffer fullness:0x7ff 低6bits
      // number_of_raw_data_blocks_in_frame：
      //    表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧。


      output_s.write(head, 7);// 写adts header , ts流不适用，ts流分离出来的packet带了adts header

      char buf[1024] = { 0 };
      memcpy(buf,pakt->data, pakt->size);
      output_s.write(buf, pakt->size);   // 写adts data
      output_s.flush();
    }
  }
  av_packet_free(&pakt);
  output_s.close();

}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    std::cout << "Hello World!" << av_version_info() << std::endl;


    //funcDemux();
    funcExtractAAC();

    return a.exec();
}
