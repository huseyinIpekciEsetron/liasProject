#ifndef TESTITEMCONTAINER_HPP
#define TESTITEMCONTAINER_HPP

#include <gui_generated/containers/TestItemContainerBase.hpp>

class TestItemContainer : public TestItemContainerBase
{
public:
    TestItemContainer();
    virtual ~TestItemContainer() {}

    virtual void initialize();
    void setHighlighted(bool isSelected);
    void setupTest(touchgfx::TypedTextId textId);
protected:
};

#endif // TESTITEMCONTAINER_HPP
