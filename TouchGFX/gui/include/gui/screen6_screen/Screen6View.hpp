#ifndef SCREEN6VIEW_HPP
#define SCREEN6VIEW_HPP

#include <gui_generated/screen6_screen/Screen6ViewBase.hpp>
#include <gui/screen6_screen/Screen6Presenter.hpp>

class Screen6View : public Screen6ViewBase
{
public:
    Screen6View();
    virtual ~Screen6View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent(); // Menülerin otomatik kapanması için eklendi

    void updateTimeDate(const TLUS::TimeData& time);

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

    // DİKKAT: Arıza değil, Log satırı kullanıyoruz!
    virtual void scrollListUpdateItem(LogItemContainer& item, int16_t itemIndex);

    bool isMenuOpen() { return currentState == STATE_MENU_OPEN; }

    void showWarningPopup(Model::WarningType warning, int tubeIndex);
    bool isPopupVisible() { return warningPopupContainer.isVisible(); }
    void hidePopup() { warningPopupContainer.hideWarning(); }

protected:
    enum LogState {
        STATE_MENU_NONE = 0,
        STATE_NAVIGATING,
        STATE_MENU_OPEN,
        STATE_VOLUME_OPEN,
        STATE_BRIGHTNESS_OPEN
    };

    LogState currentState = STATE_NAVIGATING;
    int menuTimeout = 0;
    int currentVolume;
    int currentBrightness;

    // Log Değişkenleri (Fault değil!)
    int activeLogCount = 0;
    int selectedIndex = 0;
    int lastLogHead = -1;

    void refreshLogList();
    void updateContextualIcons();
    void updateScrollBar();
};

#endif // SCREEN6VIEW_HPP
