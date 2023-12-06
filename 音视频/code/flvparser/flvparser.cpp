// flvparser.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>
#include <string>
#include <fstream>

#include <filesystem>
#include <bit>

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

double readDouble(const char* bytes) {
  // 确保您的系统是大端序的；如果不是，您需要调整字节的顺序
  double value;
  char* valuePtr = reinterpret_cast<char*>(&value);

  // 复制字节到double变量中
  // 由于IEEE 754双精度浮点数是大端序，所以可能需要反转字节顺序
  if (std::endian::native == std::endian::little) {
    // 如果系统是小端序，反转字节顺序
    std::reverse_copy(bytes, bytes + 8, valuePtr);
  }
  else {
    // 如果系统是大端序，直接复制
    std::copy(bytes, bytes + 8, valuePtr);
  }

  return value;
}

struct FlvHeader {
  char signature[3];
  char version;
  bool has_audio;
  bool has_video;
  int head_size;
};

struct FlvTag_Header {
  int pre_tag_size;
  char type;
  int data_size;
  int timestamp;
  int streamid;
};

struct VideoTag {
  int data_size;
  char* data;
};

#define  CONBIG2SMALL(buf, index)  ((buf[index++] << 24) | (buf[index++] << 16) | (buf[index++] << 8) | buf[index++])

static unsigned int ShowU16(unsigned char* pBuf) { return (pBuf[0] << 8) | (pBuf[1]); }
static unsigned int ShowU32(unsigned char* pBuf) { return (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3]; }

static const unsigned int nH264StartCode = 0x01000000;

int main() {
  std::cout << "Hello World!" << av_version_info() << std::endl;

  std::string filename = "test.flv";
  std::filesystem::path file_path("test.flv");
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cout << "file error" << std::endl;
    return 0;
  }

  //dump info
  //read flv head
  char head[9] = { 0 };
  FlvHeader flv_head = { 0 };
  file.read(head, 9);
  memcpy(flv_head.signature, head, 3);
  flv_head.version = head[3];
  flv_head.has_audio = head[4] & 0x04;
  flv_head.has_video = head[4] & 0x01;
  flv_head.head_size = (head[5] << 24) | (head[6] << 16) | (head[7] << 8) | head[8];

  std::string signal(flv_head.signature, 3);
  if (signal != "FLV") {
    std::cout << "not flv file" << std::endl;
    return 0;
  }
  int read_file_size = flv_head.head_size;
  int file_size = std::filesystem::file_size(file_path);
  //read tag

  std::vector<VideoTag> video_list;

  int h264lengthSizeMinusOne = 0;
  while (read_file_size < file_size) {
    unsigned char tag_head[11] = { 0 };//注意uchar
    FlvTag_Header flv_tag_head = { 0 };
    file.seekg(read_file_size);
    file.read(reinterpret_cast<char*>(tag_head), 11);
    read_file_size += 11 + 4;
    flv_tag_head.pre_tag_size = (tag_head[0] << 24) | (tag_head[1] << 16) | (tag_head[2] << 8) | tag_head[3];
    flv_tag_head.type = tag_head[4]; //8:audeo 9 : video 18 : Script data
    flv_tag_head.data_size = (static_cast<unsigned int>(tag_head[5]) << 16) |
      (static_cast<unsigned int>(tag_head[6]) << 8) |
      static_cast<unsigned char>(tag_head[7]);//当前tag的数据域的⼤⼩，不包含 tag header

    flv_tag_head.streamid = (static_cast<unsigned int>(tag_head[8]) << 16) |
    (static_cast<unsigned int>(tag_head[9]) << 8) |
      static_cast<unsigned char>(tag_head[10]);

    switch (flv_tag_head.type) {
    case 18: {
      std::cout << "========start process script data" << std::endl;
      char* script_tag_data = new char[flv_tag_head.data_size];
      file.seekg(read_file_size);
      file.read(script_tag_data, flv_tag_head.data_size);
      int p = 0;
      char amf_type = script_tag_data[p++];
      int  amf_size = (script_tag_data[p++] << 8) | script_tag_data[p++];
      char* amf_string = new char[amf_size + 1];
      memcpy(amf_string, script_tag_data + p, amf_size);
      amf_string[amf_size] = '\0';
      p += amf_size;

      std::cout << "===============================" << std::endl;
      std::cout << "amf1: type=" << amf_type
        << "\nsize=" << amf_size
        << "\nvalue=" << std::string(amf_string) << std::endl;
      std::cout << "===============================" << std::endl;

      char amf2_type = script_tag_data[p++];
      int amf2_arr_size = CONBIG2SMALL(script_tag_data, p);

      for (int i = 0; i < amf2_arr_size; ++i) {
        int str_len = (script_tag_data[p++] << 8) | script_tag_data[p++];
        char* str = new char[str_len + 1];
        memcpy(str, script_tag_data + p, str_len);
        p += str_len;
        str[str_len] = '\0';

        std::cout << " key : " << std::setw(20) << std::left << str;

        char val_type = script_tag_data[p++];

        switch (val_type) {
        case 0x00: {  // Number
          //std::cout << "Number" ;
          double num;
          char* val_p = (char*)&num;
          std::reverse_copy(script_tag_data + p, script_tag_data + p + 8, val_p);
          p += 8;
          std::cout << " val : " << num << std::endl;
          break;
        }
        case 0x01: { // Boolean
          //std::cout << "Boolean" ;
          // 代码来读取1字节的布尔值
          bool bo = script_tag_data[p++];
          std::cout << " val : " << bo << std::endl;
          break;
        }
        case 0x02: {// String
          //std::cout << "String" ;
          // 代码来读取字符串（首先读取2字节的长度，然后读取字符串）
          int str_len = (script_tag_data[p++] << 8) | script_tag_data[p++];
          char* str = new char[str_len + 1];
          // 然后根据长度读取字符串
          memcpy(str, script_tag_data + p, str_len);
          p += str_len;
          str[str_len] = '\0';
          std::cout << " val : " << str << std::endl;
          break;
        }
        case 0x03:  // Object
          std::cout << "Object" << std::endl;
          // 代码来读取对象
          break;
        case 0x05:  // Null
          std::cout << "Null" << std::endl;
          break;
        case 0x06:  // Undefined
          std::cout << "Undefined" << std::endl;
          break;
        case 0x08:  // ECMA Array
          std::cout << "ECMA Array" << std::endl;
          // 代码来读取数组（首先读取4字节的长度，然后读取键值对）
          p += 4; // 跳过数组长度
          break;
        case 0x0A:  // Strict Array
          std::cout << "Strict Array" << std::endl;
          // 代码来读取严格数组
          p += 4; // 跳过数组长度
          break;
        case 0x0B:  // Date
          std::cout << "Date" << std::endl;
          // 代码来读取日期
          p += 10; // 8字节日期加2字节时区偏移量
          break;
        default:
          std::cout << "Unknown type" << std::endl;
          break;
        }

        std::cout << "-------------" << std::endl;
        delete[] str;
      }

      std::cout << "===============================" << std::endl;
      delete[] amf_string;
      delete[] script_tag_data;
      break;
    }
    case 8: {
      std::cout << "========start process audio data" << std::endl;
      break;
    }
    case 9: {
      std::cout << "========start process video data" << std::endl;
      unsigned char* vedio_tag_data =  new unsigned char[flv_tag_head.data_size];
      file.seekg(read_file_size);
      file.read(reinterpret_cast<char*>(vedio_tag_data), flv_tag_head.data_size);

      int p = 0;
      char frame_type = vedio_tag_data[p] >> 4;
      char codec_id = vedio_tag_data[p++] & 0x0F;
      std::cout << "frame_type:" << (int)frame_type << " codec_id:" << (int)codec_id << std::endl;

      switch (codec_id) {
      case 7: {
        std::cout << "start process aac data" << std::endl;
        int aac_packet_type = vedio_tag_data[p];
        std::cout << "aac_packet_type = " << aac_packet_type << std::endl;
        if (aac_packet_type == 0) {//h264 config
          std::cout << "read avc head" << std::endl;
          int data_size = flv_tag_head.data_size - 4;
          h264lengthSizeMinusOne = (vedio_tag_data[9] & 0x03) + 1;


          int sps_size, pps_size;
          sps_size = (vedio_tag_data[11] << 8) | (vedio_tag_data[12]);
          pps_size = ShowU16(vedio_tag_data + 11 + (2 + sps_size) + 1);

          int _nMediaLen = 4 + sps_size + 4 + pps_size;
          char* data = new char[_nMediaLen];

          memcpy(data, &nH264StartCode, 4);
          memcpy(data + 4, vedio_tag_data + 11 + 2, sps_size);
          memcpy(data + 4 + sps_size, &nH264StartCode, 4);
          memcpy(data + 4 + sps_size + 4, vedio_tag_data + 11 + 2 + sps_size + 2 + 1, pps_size);
          video_list.push_back({ _nMediaLen, data });


        } else if(aac_packet_type == 1) {//nalu
          int data_size = flv_tag_head.data_size + 10;
          char* data = new char[data_size];
          memset(data, data_size, 0);

          int _nMediaLen = 0;
          int nOffset = 5;
          while (1) {
            if (nOffset >= flv_tag_head.data_size)
              break;

            int nNaluLen = 0;
            switch (h264lengthSizeMinusOne) {
            case 4:
              nNaluLen = ShowU32(vedio_tag_data + nOffset);
              break;
            default:
              std::cout << "[warning]!";
              break;
            }
            memcpy(data + _nMediaLen, &nH264StartCode, 4);
            memcpy(data + _nMediaLen + 4, vedio_tag_data + nOffset + h264lengthSizeMinusOne, nNaluLen);
            _nMediaLen += (4 + nNaluLen);
            nOffset += (h264lengthSizeMinusOne + nNaluLen);
          }

          video_list.push_back({ data_size, data });
        }
        break;
      }
      default:
        std::cout << "[warning] other vedio codec id" << std::endl;
        break;
      }


      delete[] vedio_tag_data;
      break;
    }
    default:
      std::cout << "read tag:" << flv_tag_head.type << std::endl;
      break;
    }
    read_file_size += flv_tag_head.data_size;
  }
  //dump aac h264
  std::string output_name = "out.h264";

  std::ofstream output_s;
  output_s.open(output_name, std::ios::binary);
  std::cout << video_list.size();

  for (auto& it : video_list) {
    output_s.write(it.data, it.data_size);
    delete[] it.data;
  }
  output_s.close();
  //dump flv
  return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
