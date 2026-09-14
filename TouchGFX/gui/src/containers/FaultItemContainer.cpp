#include <gui/containers/FaultItemContainer.hpp>
#include <touchgfx/Color.hpp>

FaultItemContainer::FaultItemContainer() {}

void FaultItemContainer::initialize() {
    FaultItemContainerBase::initialize();
}

void FaultItemContainer::setupFault(touchgfx::TypedTextId textId, int tubeIndex, uint16_t timeVal)
{
    // 1. Önce Arıza Mesajını oluştur (%d varsa tubeIndex'i yerleştirir)
    if (tubeIndex == -1) {
        touchgfx::Unicode::strncpy(textArea1Buffer, touchgfx::TypedText(textId).getText(), TEXTAREA1_SIZE);
    } else {
        touchgfx::Unicode::snprintf(textArea1Buffer, TEXTAREA1_SIZE, touchgfx::TypedText(textId).getText(), tubeIndex);
    }

    // 2. Eğer saat geçerliyse (0xFFFF değilse) metnin sonuna saati iliştir!
    if (timeVal != 0xFFFF) {
        uint16_t currentLen = touchgfx::Unicode::strlen(textArea1Buffer);
        uint8_t hour = timeVal >> 8;
        uint8_t min  = timeVal & 0xFF;

        // Mevcut metnin bittiği yerden itibaren "  14:25" formatında ekleme yapıyoruz
        touchgfx::Unicode::snprintf(&textArea1Buffer[currentLen], TEXTAREA1_SIZE - currentLen, "   %02d:%02d", hour, min);
    }

    textArea1.invalidate();
}

void FaultItemContainer::setHighlighted(bool isSelected) {
	if (isSelected) {
		// Seçiliyken: Arka Plan Turuncu (#FF5100), Yazı Beyaz (#FFFFFF)
		boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(255, 81, 0));
		textArea1.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	} else {
		// Normal durum: Arka Plan Açık Gri (#CFCFCF), Yazı Siyah (#000000)
		boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(207, 207, 207));
		textArea1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
	}

	// Değişiklikleri ekrana yansıt (Yazıyı da yenilemeyi unutmuyoruz!)
	boxWithBorder.invalidate();
	textArea1.invalidate();
}
