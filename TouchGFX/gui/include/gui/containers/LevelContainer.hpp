#ifndef LEVELCONTAINER_HPP
#define LEVELCONTAINER_HPP

#include <gui_generated/containers/LevelContainerBase.hpp>

class LevelContainer : public LevelContainerBase
{
public:
    LevelContainer();
    virtual ~LevelContainer() {}

    virtual void initialize();
    void setLevel(int level, uint16_t svgImageId);
protected:
};

#endif // LEVELCONTAINER_HPP
