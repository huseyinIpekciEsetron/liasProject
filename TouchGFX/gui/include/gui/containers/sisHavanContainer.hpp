#ifndef SISHAVANCONTAINER_HPP
#define SISHAVANCONTAINER_HPP

#include <gui_generated/containers/sisHavanContainerBase.hpp>
#include <gui/model/Model.hpp>

class sisHavanContainer : public sisHavanContainerBase
{
public:
    sisHavanContainer();
    virtual ~sisHavanContainer() {}

    virtual void initialize();
    void setStatus(Model::TubeState t1, Model::TubeState t2, Model::TubeState t3, Model::TubeState t4);
protected:
};

#endif // SISHAVANCONTAINER_HPP
