#ifndef FAULTITEMCONTAINER_HPP
#define FAULTITEMCONTAINER_HPP

#include <gui_generated/containers/FaultItemContainerBase.hpp>

class FaultItemContainer : public FaultItemContainerBase
{
public:
    FaultItemContainer();
    virtual ~FaultItemContainer() {}
    virtual void initialize();

    void setupFault(touchgfx::TypedTextId textId, int tubeIndex = -1, uint16_t timeVal = 0xFFFF);
    void setHighlighted(bool isSelected);
protected:
};

#endif // FAULTITEMCONTAINER_HPP
