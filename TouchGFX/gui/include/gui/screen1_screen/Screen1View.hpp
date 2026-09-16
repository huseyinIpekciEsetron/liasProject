#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/widgets/canvas/Circle.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/Color.hpp>
#include <cmath>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/widgets/BoxWithBorder.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <gui/model/Model.hpp>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    void updateTargets(const TLUS::ThreatMessagePayload& payload);
    void openVolumeMenu();
	void openBrightnessMenu();
	void handleVolumeOrEnter();
	void handleBrightnessOrBack();
	void buttonUpPressed();
	void buttonDownPressed();
	void updateButtonVisuals(uint32_t state);
	void closeMenus();
	void menuButtonPressed();
	void homeButtonPressed();
	void showWarningPopup(Model::WarningType warning, int tubeIndex);
	bool isPopupVisible() { return warningPopupContainer.isVisible(); }
	void hidePopup() { warningPopupContainer.hideWarning(); }
	void updateTimeDate(const TLUS::TimeData& time);

protected:
	enum ActiveMenuState {
		MENU_NONE,
		MENU_VOLUME,
		MENU_BRIGHTNESS,
		MENU_MAIN_LIST
	};

	static const int MAX_TARGETS = Model::MAX_TARGETS;
	int currentAngles[MAX_TARGETS];
	int currentThreatNumbers[MAX_TARGETS];

	ActiveMenuState currentMenu = MENU_NONE;
	int menuTimeout = 0;
	int currentVolume;
	int currentBrightness;

	touchgfx::Circle threatArcs[MAX_TARGETS];
	touchgfx::PainterRGB565 arcPainters[MAX_TARGETS];
	touchgfx::BoxWithBorder threatTextBoxes[MAX_TARGETS];
	touchgfx::TextAreaWithOneWildcard threatTexts[MAX_TARGETS];
	touchgfx::Unicode::UnicodeChar threatTextBuffers[MAX_TARGETS][8];

	TextAreaWithOneWildcard threatPriorityTexts[MAX_TARGETS];
	touchgfx::Unicode::UnicodeChar threatPriorityBuffers[MAX_TARGETS][8];
	touchgfx::TextArea threatClassTexts[MAX_TARGETS];

	void freeThreatSlot(int slotIndex);

	// --- ARIZA ALT YAZI (FADE LOOP) SİSTEMİ DEĞİŞKENLERİ ---
	struct ActiveFault {
		touchgfx::TypedTextId textId;
		int tubeIndex;
	};
	static const int MAX_ACTIVE_FAULTS = 100;
	ActiveFault activeFaults[MAX_ACTIVE_FAULTS];
	int activeFaultCount = 0;
	int currentTickerIndex = -1;
	int faultCheckTimer = 0;

    int tickerDisplayTimer = 0;
    bool isFadingOut = false;

    touchgfx::Callback<Screen1View, const touchgfx::FadeAnimator<touchgfx::TextAreaWithOneWildcard>&> fadeAnimationEndedCallback;
    void fadeAnimationEndedHandler(const touchgfx::FadeAnimator<touchgfx::TextAreaWithOneWildcard>& src);

	void buildFaultList();
	void showNextFaultInTicker();
    void updateTickerText();
    void addFault(touchgfx::TypedTextId id, int idx);

};

#endif // SCREEN1VIEW_HPP
