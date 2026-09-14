#ifndef BUTTONCONTAINER_HPP
#define BUTTONCONTAINER_HPP

#include <gui_generated/containers/ButtonContainerBase.hpp>
#include "hc165_driver.h"

class ButtonContainer : public ButtonContainerBase
{
public:
    ButtonContainer();
    virtual ~ButtonContainer() {}

    virtual void initialize();
    void updateButtonVisuals(uint32_t state, uint32_t override_active_btn = 0);
    void setButtonIcon(HC165_Buttons button, uint16_t svgId, float scaleX = 1.0f, float scaleY = 1.0f);
protected:
};

#endif // BUTTONCONTAINER_HPP
