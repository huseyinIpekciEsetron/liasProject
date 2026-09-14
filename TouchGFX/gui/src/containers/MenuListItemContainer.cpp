#include <gui/containers/MenuListItemContainer.hpp>
#include <touchgfx/Color.hpp>


MenuListItemContainer::MenuListItemContainer()
{

}

void MenuListItemContainer::initialize()
{
    MenuListItemContainerBase::initialize();
}
void MenuListItemContainer::setupItem(touchgfx::TypedTextId textId, uint16_t svgId)
{
	// 1. Yazıyı Güncelle
	indisTextArea.setTypedText(touchgfx::TypedText(textId));

	// 2. İkonu (SVG) Güncelle
	indisSvgImage.setSVG(svgId);

	indisSvgImage.setImagePosition(2, 2);
	indisSvgImage.setScale(1.48, 1.48);

	// 3. Ekranda Yenile
	indisTextArea.invalidate();
	indisSvgImage.invalidate();
}

void MenuListItemContainer::setHighlighted(bool isSelected)
{
    if (isSelected)
    {
        // SEÇİLİ DURUM: Arka planı farklı bir renk yap (Örn: Turuncu veya daha açık mavi)
         indisboxWithBorder.setColor(touchgfx::Color::getColorFromRGB(255, 81, 0)); // Turuncu
        //indisboxWithBorder.setColor(touchgfx::Color::getColorFromRGB(0, 90, 120)); // Açık Mavi
    }
    else
    {
        // NORMAL DURUM: Mevcut koyu mavi arka plan rengin
        indisboxWithBorder.setColor(touchgfx::Color::getColorFromRGB(0, 46, 64));
    }

    indisboxWithBorder.invalidate();
}
