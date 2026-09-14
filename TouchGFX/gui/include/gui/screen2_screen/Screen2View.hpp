#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <touchgfx/widgets/canvas/Circle.hpp> // SHAPE YERİNE CIRCLE KULLANIYORUZ
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/Color.hpp>
#include <cmath>
#include <texts/TextKeysAndLanguages.hpp>


class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    // Her karede çalışıp silinme efektini (Fading) yapacak fonksiyon
	virtual void handleTickEvent();
    void openVolumeMenu();
	void openBrightnessMenu();
	void handleVolumeOrEnter();
	void handleBrightnessOrBack();
	void buttonUpPressed();
	void buttonDownPressed();
	void closeMenus();
	void updateButtonVisuals(uint32_t state);
	void updateSmokeStatus(Model::TubeState* states);
	void menuButtonPressed();
	void homeButtonPressed();
	void showWarningPopup(Model::WarningType warning, int tubeIndex);
	void updateTimeDate(const TLUS::TimeData& time);
	bool isPopupVisible() { return warningPopupContainer.isVisible(); }
	void hidePopup() { warningPopupContainer.hideWarning(); }
protected:
	// Sistemin o an hangi modda olduğunu tutan State Machine Enum'u
	enum ActiveMenuState {
		MENU_NONE,
		MENU_VOLUME,
		MENU_BRIGHTNESS,
		MENU_MAIN_LIST
	};

	ActiveMenuState currentMenu = MENU_NONE;
	int menuTimeout = 0; // Menünün ekranda kalma süresi

	int currentVolume ;
	int currentBrightness ;
	void updateHeaderBoxSize();
};

#endif // SCREEN2VIEW_HPP
