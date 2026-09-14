#include <gui/containers/LogItemContainer.hpp>
#include <touchgfx/Color.hpp>

LogItemContainer::LogItemContainer() {}

void LogItemContainer::initialize() {
    LogItemContainerBase::initialize();
}

void LogItemContainer::setupLog(uint8_t h, uint8_t m, uint8_t s, uint16_t textId, int param, Model::LogEventType type)
{
	// ZAMANI YAZDIR
	touchgfx::Unicode::snprintf(TimeTextAreaBuffer, TIMETEXTAREA_SIZE, "%02d:%02d:%02d", h, m, s);

	// LOG METNİNİ YAZDIR
	if (param == -1) {
		touchgfx::Unicode::strncpy(textArea1Buffer, touchgfx::TypedText(textId).getText(), TEXTAREA1_SIZE);
	} else {
		touchgfx::Unicode::snprintf(textArea1Buffer, TEXTAREA1_SIZE, touchgfx::TypedText(textId).getText(), param);
	}

	// RENKLİ DURUM ÇİZGİSİ (BOX) KONTROLÜ
	switch (type) {
		case Model::LOG_EVENT_FIRE:
			statusColorBox.setColor(touchgfx::Color::getColorFromRGB(0, 200, 0)); // Yeşil (Atış)
			break;
		case Model::LOG_EVENT_FAULT_OCCURRED:
			statusColorBox.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0)); // Kırmızı (Arıza)
			break;
		case Model::LOG_EVENT_FAULT_CLEARED:
		case Model::LOG_EVENT_CLEAR_ALL:
			statusColorBox.setColor(touchgfx::Color::getColorFromRGB(0, 150, 255)); // Mavi (Temizleme/Düzeltme)
			break;
		case Model::LOG_EVENT_SYSTEM_STATE:
		default:
			statusColorBox.setColor(touchgfx::Color::getColorFromRGB(120, 120, 120)); // Gri (Standart Bilgi)
			break;
	}

	statusColorBox.invalidate();
	TimeTextArea.invalidate();
	textArea1.invalidate();
}

void LogItemContainer::setHighlighted(bool isSelected)
{
    if (isSelected) {
        // Seçiliyken: Arka Plan Turuncu, Yazılar Beyaz
        boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(255, 81, 0));
        textArea1.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
        TimeTextArea.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    } else {
        // Normal durum: Arka Plan Açık Gri, Yazılar Siyah
        boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(207, 207, 207));
        textArea1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
        TimeTextArea.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    }
    boxWithBorder.invalidate();
    textArea1.invalidate();
    TimeTextArea.invalidate();
}
