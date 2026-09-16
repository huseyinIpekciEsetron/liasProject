#ifndef SCREEN1PRESENTER_HPP
#define SCREEN1PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen1View;

class Screen1Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen1Presenter(Screen1View& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~Screen1Presenter() {}
    virtual void onRadarTargetsReceived(const TLUS::ThreatMessagePayload& payload) override;
    virtual void onVolumeMenuPressed() override;
    virtual void onBrightnessMenuPressed() override;
    virtual void onUpPressed() override;
    virtual void onDownPressed() override;
    virtual void onHardwareButtonStateChanged(uint32_t state) override;
    virtual void onSmokePressed() override;
    virtual void onHomePressed() override;
    virtual void onBlackoutStateChanged(bool isBlackout) override;
    virtual void onMenuPressed() override;
    virtual void onShowWarning(Model::WarningType warning, int tubeIndex) override;
    virtual void onTimeDateUpdated(const TLUS::TimeData& time) override;

    /* wiev den modele */
    void saveBrightness(int brightness);
    void saveVolume(int volume);
    void gotoAyarlarScreen();
	void gotoArizaScreen();
	void gotoTestScreen();
	void gotoLoglarScreen();
	void gotoZeroizeScreen();
	void gotoSoftResetScreen();

	int getBrightness();
	int getVolume();
	bool getFaultStatus(int index);
	bool getFaultIsFrag(int index);
	bool getCommLostFlag();
	bool getHwErrorFlag();
	bool getTlusCommLostFlag();
	bool getTlusSwDeadFlag();
	bool getTlusFrozenFlag();

	// --- ADD THESE NEW FUNCTIONS ---
	int getFadeTicksRemaining(int index);
	void setFadeTicksRemaining(int index, int ticks);
	TLUS::SystemStatusPayload getActiveSystemStatus();

private:
    Screen1Presenter();

    Screen1View& view;
};

#endif // SCREEN1PRESENTER_HPP
