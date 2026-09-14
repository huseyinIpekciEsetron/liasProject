#ifndef SCREEN7VIEW_HPP
#define SCREEN7VIEW_HPP

#include <gui_generated/screen7_screen/Screen7ViewBase.hpp>
#include <gui/screen7_screen/Screen7Presenter.hpp>

class Screen7View : public Screen7ViewBase
{
public:
    Screen7View();
    virtual ~Screen7View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    void menuButtonPressed();
	void closeMenus();
	void handleVolumeOrEnter();
	void handleBrightnessOrBack();
	void buttonUpPressed();
	void buttonDownPressed();
	void openVolumeMenu();
	void openBrightnessMenu();

    void updateButtonState(uint32_t state);
    void updateTimeDate(const TLUS::TimeData& time);

    // --- UYARI (WARNING) FONKSİYONLARI ---
    void showWarningPopup(Model::WarningType warning, int tubeIndex);
    bool isWarningPopupVisible() { return warningPopupContainer.isVisible(); }
    void hideWarningPopup() { warningPopupContainer.hideWarning(); }
    bool isMenuActive() { return currentMenu != MENU_NONE; }
    bool isActionPopupVisible() { return currentState == STATE_POPUP; }
    void closeWarningSafe();
protected:
    enum ScreenState {
        STATE_INITIAL,
        STATE_POPUP
    };

    ScreenState currentState;

    enum ActiveMenuState {
		MENU_NONE,
		MENU_VOLUME,
		MENU_BRIGHTNESS,
		MENU_MAIN_LIST
	};

	ActiveMenuState currentMenu = MENU_NONE;
	int menuTimeout = 0;
	int currentVolume;
	int currentBrightness;

    uint32_t currentButtonState = 0;
    uint32_t lastButtonState = 0;

    int holdTimer = 0;
    int successTimer = 0;
    bool actionFired = false;

    bool waitingForRelease = false;

    void showPopup();
    void hidePopup();
    int warningCooldown = 0;
};

#endif // SCREEN7VIEW_HPP
