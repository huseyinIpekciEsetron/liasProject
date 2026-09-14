#ifndef MENULISTCONTAINER_HPP
#define MENULISTCONTAINER_HPP

#include <gui_generated/containers/MenuListContainerBase.hpp>
#include <gui/containers/MenuListItemContainer.hpp>

class MenuListContainer : public MenuListContainerBase
{
public:
    MenuListContainer();
    virtual ~MenuListContainer() {}

    virtual void initialize();

    void moveUp();
	void moveDown();
	int getSelectedItem() { return selectedIndex; }
	void resetSelection();

	// ScrollList için gereken TouchGFX fonksiyonu
	void menuScrollListUpdateItem(MenuListItemContainer& item, int16_t itemIndex);
protected:
	int selectedIndex;
	static const int MAX_ITEMS = 6; // Listede kaç eleman varsa (Ayarlar, Arıza, Test, Geçmiş)
};

#endif // MENULISTCONTAINER_HPP
