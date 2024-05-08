#pragma once
#include "Controller/BaseController/BaseController.h"


class DUIButton : public BaseController {
public:
    DUIButton(/* args */);
    virtual ~DUIButton();

    void Draw() override;
    void Update() override;
};