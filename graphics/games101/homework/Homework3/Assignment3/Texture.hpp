//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <eigen3/Eigen/Eigen>
#include <opencv2/opencv.hpp>
class Texture{
private:
    cv::Mat image_data;

public:
    Texture(const std::string& name)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
    }

    int width, height;

    Eigen::Vector3f getColorBilinearInterpolation(float u, float v) {
        int u_img = static_cast<int>(u * width);
        int v_img = static_cast<int>((1 - v) * height);
        if (u_img < 0)
          u_img = 0;
        if (u_img >= width)
          u_img = width - 1;
        if (v_img < 0)
          v_img = 0;
        if (v_img >= height)
          v_img = height - 1;

        //bilinear interpolation
        float u0 = u * width - u_img;
        float v0 = (1 - v) * height - v_img;
        float u1 = 1 - u0;
        float v1 = 1 - v0;
        cv::Vec3b color00 = image_data.at<cv::Vec3b>(v_img, u_img);
        cv::Vec3b color01 = image_data.at<cv::Vec3b>(v_img, u_img + 1);
        cv::Vec3b color10 = image_data.at<cv::Vec3b>(v_img + 1, u_img);
        cv::Vec3b color11 = image_data.at<cv::Vec3b>(v_img + 1, u_img + 1);
        float r = u1 * v1 * color00[0] + u0 * v1 * color01[0] + u1 * v0 * color10[0] + u0 * v0 * color11[0];
        float g = u1 * v1 * color00[1] + u0 * v1 * color01[1] + u1 * v0 * color10[1] + u0 * v0 * color11[1];
        float b = u1 * v1 * color00[2] + u0 * v1 * color01[2] + u1 * v0 * color10[2] + u0 * v0 * color11[2];
        return Eigen::Vector3f(r, g, b);
    }

    Eigen::Vector3f getColor(float u, float v) {
        int u_img = static_cast<int>(u * width);
        int v_img = static_cast<int>((1 - v) * height);
        if (u_img < 0)
          u_img = 0;
        if (u_img >= width)
          u_img = width - 1;
        if (v_img < 0)
          v_img = 0;
        if (v_img >= height)
          v_img = height - 1;

        cv::Vec3b color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }

};
#endif //RASTERIZER_TEXTURE_H
