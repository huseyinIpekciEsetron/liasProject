#ifndef SCREEN5VIEW_HPP
#define SCREEN5VIEW_HPP

#include <gui_generated/screen5_screen/Screen5ViewBase.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <gui/screen5_screen/Screen5Presenter.hpp>

class Screen5View : public Screen5ViewBase
{
public:
    Screen5View();
    virtual ~Screen5View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    void enterPressed();
    void backPressed();
    void buttonUpPressed();
    void buttonDownPressed();
    void menuButtonPressed();
    void closeMenus();

    void updateButtonVisuals(uint32_t state);
    virtual void scrollListUpdateItem(TestItemContainer& item, int16_t itemIndex) override;

    bool isMenuOpen() { return currentState == STATE_MENU_OPEN; }
    void showWarningPopup(Model::WarningType warning, int tubeIndex);
    bool isPopupVisible() { return warningPopupContainer.isVisible(); }
    void hidePopup() { warningPopupContainer.hideWarning(); }
    void updateTimeDate(const TLUS::TimeData& time);

protected:
    enum TestState {
        STATE_NAVIGATING,
        STATE_MENU_OPEN,
        STATE_TEST_RUNNING,
		STATE_TEST_FINISHED
    };

    enum TestTypes {
		TEST_BRIGHTNESS = 0,
		TEST_LED,
		TEST_BUZZER,
		TEST_BUTTON,
		TEST_BMB_POWER,
		TEST_COMMUNICATION,
		MAX_TEST // Otomatik olarak 6 olur
	};

    TestState currentState = STATE_NAVIGATING;
    int menuTimeout = 0;

    int currentVolume;
    int currentBrightness;
    int currentLedBrightness;
    uint32_t last_test_button_state = 0;

    int selectedIndex = 0;
    static const int MAX_TESTS = 6;
    const touchgfx::TypedTextId testNames[MAX_TESTS] = {
		T_TESTBRIGHTNESS, T_TESTLED, T_TESTBUZZER, T_TESTBUTTON, T_TESTBMB, T_TESTCOMM
	};

    // Parlaklık Testi Değişkenleri
    int brightnessTestLevel = 5;
    int brightnessTestStep = -1;
    int testTickCounter = 0; // Hızı kontrol edecek
    int brightnessTestCycle = 0; // Kaç kere (0-5) yapıldığını sayacak
    int testPauseCounter = 0; // Uç noktalarda (0 ve 5'te) beklemeyi sağlayacak
    int ledTestStep = 0;
    int buzzerTestStep = 0;
    void runSelectedTest();
    void stopCurrentTest();
    void updateHeaderBoxSize();
};

#endif // SCREEN5VIEW_HPP
