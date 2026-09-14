#ifndef MENULISTITEMCONTAINER_HPP
#define MENULISTITEMCONTAINER_HPP

#include <gui_generated/containers/MenuListItemContainerBase.hpp>

class MenuListItemContainer : public MenuListItemContainerBase
{
public:
    MenuListItemContainer();
    virtual ~MenuListItemContainer() {}

    virtual void initialize();
    // Satırın içeriğini dışarıdan belirlemek için (Metin, İkon vs.)
    void setupItem(touchgfx::TypedTextId textId, uint16_t svgId);

	// Satırın seçili (vurgulu) olup olmadığını ayarlamak için
	void setHighlighted(bool isSelected);
protected:
};

#endif // MENULISTITEMCONTAINER_HPP
