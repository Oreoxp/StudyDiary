#include "BaseController.h"

BaseController::BaseController(/* args */) {
}

BaseController::~BaseController() {
}


void BaseController::SetX(int x) { 
    x_ = x;
    Update();
}

void BaseController::SetY(int y) { 
    y_ = y;
    Update(); 
}

void BaseController::SetWidth(int width) { 
    width_ = width;
    Update(); 
}

void BaseController::SetHeight(int height) { 
    height_ = height;
    Update(); 
}