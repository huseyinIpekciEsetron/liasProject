#ifndef SCREEN4VIEW_HPP
#define SCREEN4VIEW_HPP

#include <gui_generated/screen4_screen/Screen4ViewBase.hpp>
#include <gui/screen4_screen/Screen4Presenter.hpp>

class Screen4View : public Screen4ViewBase
{
public:
    Screen4View();
    virtual ~Screen4View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    void enterPressed();
    void backPressed();
    void buttonUpPressed();
    void buttonDownPressed();
    void menuButtonPressed();
    void closeMenus();
    void handleVolumeOrEnter();
    void handleBrightnessOrBack();
    void openVolumeMenu();
    void openBrightnessMenu();

    void updateButtonVisuals(uint32_t state);
    virtual void scrollListUpdateItem(FaultItemContainer& item, int16_t itemIndex);

    bool isMenuOpen() { return currentState == STATE_MENU_OPEN; }

    void showWarningPopup(Model::WarningType warning, int tubeIndex);
    bool isPopupVisible() { return warningPopupContainer.isVisible(); }
	void hidePopup() { warningPopupContainer.hideWarning(); }
	void updateTimeDate(const TLUS::TimeData& time);
	 void refreshFaultList();
protected:
    enum FaultState {
    	STATE_MENU_NONE = 0,
        STATE_NAVIGATING,
        STATE_MENU_OPEN,
		STATE_VOLUME_OPEN,
		STATE_BRIGHTNESS_OPEN
    };

    FaultState currentState = STATE_NAVIGATING;
    int menuTimeout = 0;

    int currentVolume;
	int currentBrightness;

    // Arıza Listesi Değişkenleri
    int activeFaultCount = 0;
    int selectedIndex = 0;

    struct FaultData {
		touchgfx::TypedTextId textId;
		int tubeIndex;
		uint16_t timeVal; // YENİ
		bool isClearAction;
	};
    FaultData faultList[20]; // Maksimum 20 satırlık dinamik dizi

    void updateContextualIcons();


    void checkAndRefreshFaults();
	bool last_ariza_hafizasi[16];
	bool last_comm_lost_flag;
	bool last_hw_error_flag;

	void updateHeaderBoxSize();
	void updateScrollBar();
};

#endif // SCREEN4VIEW_HPP
