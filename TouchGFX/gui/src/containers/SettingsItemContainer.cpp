#include <gui/containers/SettingsItemContainer.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Utils.hpp>

SettingsItemContainer::SettingsItemContainer()
{
}

void SettingsItemContainer::initialize()
{
    SettingsItemContainerBase::initialize();
}

void SettingsItemContainer::setupItemValue(touchgfx::TypedTextId titleId, touchgfx::TypedTextId valueId) {
    touchgfx::Unicode::strncpy(textArea1Buffer, touchgfx::TypedText(titleId).getText(), TEXTAREA1_SIZE);
    touchgfx::Unicode::strncpy(textArea2Buffer, touchgfx::TypedText(valueId).getText(), TEXTAREA2_SIZE);
    textArea1.invalidate(); textArea2.invalidate();
}

void SettingsItemContainer::setupItemNumber(touchgfx::TypedTextId titleId, int value) {
    touchgfx::Unicode::strncpy(textArea1Buffer, touchgfx::TypedText(titleId).getText(), TEXTAREA1_SIZE);
    touchgfx::Unicode::snprintf(textArea2Buffer, TEXTAREA2_SIZE, "%d / 5", value);
    textArea1.invalidate(); textArea2.invalidate();
}

void SettingsItemContainer::setHighlighted(bool isSelected, bool isEditing)
{
	if (isEditing && isSelected) {
		// DÜZENLEME MODU: Parlaklık/Ses değiştirilirken Koyu Kırmızı
		boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(139, 0, 0));
		textArea1.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	}
	else if (isSelected) {
		// GEZİNME MODU: Üzerindeyken Turuncu (#FF5100)
		boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(255, 81, 0));
		textArea1.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	}
	else {
		// NORMAL DURUM (Seçili Değil): Açık Gri (#CFCFCF -> 207, 207, 207)
		boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(207, 207, 207));
		textArea1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
	}

	boxWithBorder.invalidate();
	textArea1.invalidate();
}
