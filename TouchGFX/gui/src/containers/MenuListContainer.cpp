#include <gui/containers/MenuListContainer.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <images/SVGDatabase.hpp>

MenuListContainer::MenuListContainer() : selectedIndex(0)
{
}

void MenuListContainer::initialize()
{
    MenuListContainerBase::initialize();

    // Designer'daki menuScrollList bileşenine eleman sayısını bildiriyoruz
    menuScrollList.setNumberOfItems(MAX_ITEMS);

    int itemHeight = 50;
	int padding = 90;
	int newHeight = (MAX_ITEMS * itemHeight) + padding;
	menuBackboxWithBorder.setHeight(newHeight);

    resetSelection();
}

void MenuListContainer::resetSelection()
{
    selectedIndex = 0; // Her açılışta 1. eleman (Ayarlar) seçili olsun

    for(int i = 0; i < MAX_ITEMS; i++) {
		menuScrollList.itemChanged(i);
	}
}

void MenuListContainer::moveUp()
{
    int oldIndex = selectedIndex;
    int numItems = menuScrollList.getNumberOfItems(); // OTOMATİK TESPİT

    if (selectedIndex > 0) {
        selectedIndex--;
    } else {
        selectedIndex = numItems - 1; // BAŞA SAR (En alta git)
    }

    menuScrollList.itemChanged(oldIndex);
    menuScrollList.itemChanged(selectedIndex);
}

void MenuListContainer::moveDown()
{
    int oldIndex = selectedIndex;
    int numItems = menuScrollList.getNumberOfItems(); // OTOMATİK TESPİT

    if (selectedIndex < numItems - 1) {
        selectedIndex++;
    } else {
        selectedIndex = 0; // BAŞA SAR (En üste git)
    }

    menuScrollList.itemChanged(oldIndex);
    menuScrollList.itemChanged(selectedIndex);
}

void MenuListContainer::menuScrollListUpdateItem(MenuListItemContainer& item, int16_t itemIndex)
{
    // 1. SEÇİM VURGUSU KONTROLÜ
    if (itemIndex == selectedIndex) {
        item.setHighlighted(true);
    } else {
        item.setHighlighted(false);
    }

    switch (itemIndex)
    {
        case 0:
            // T_MENU_AYARLAR: TouchGFX Designer'da Texts sekmesinde verdiğin ID
            item.setupItem(T_MENU_AYARLAR, SVG_AYARLAR_ID);
            break;

        case 1:
            item.setupItem(T_MENU_ARIZA, SVG_HATALAR_ID);
            break;

        case 2:
            item.setupItem(T_MENU_TEST, SVG_TEST_ID);
            break;

        case 3:
            item.setupItem(T_MENU_LOGLAR, SVG_LOG_ID);
            break;
        case 4:
        	item.setupItem(T_MENU_SOFTRESET, SVG_SOFTRESET_ID);
			break;
        case 5:
        	item.setupItem(T_MENU_ZEROIZE, SVG_ACILSIFIRLAMA_ID);
        	break;
        default:
            break;
    }
}
