#pragma once


class BaseController {
public:
    BaseController(/* args */);
    virtual ~BaseController();

    virtual void Draw() = 0;
    virtual void Update() = 0;

    int X() const { return x_; }
    int Y() const { return y_; }
    int Width() const { return width_; }
    int Height() const { return height_; }

    void SetX(int x);
    void SetY(int y);
    void SetWidth(int width);
    void SetHeight(int height);
private:
    int x_ = 0;
    int y_ = 0;
    int width_ = 0;
    int height_ = 0;
};

