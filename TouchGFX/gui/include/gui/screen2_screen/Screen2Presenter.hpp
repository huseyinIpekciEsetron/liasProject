#ifndef SCREEN2PRESENTER_HPP
#define SCREEN2PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen2View;

class Screen2Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen2Presenter(Screen2View& v);

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

    virtual ~Screen2Presenter() {}
    /* modelden view e */
    virtual void onHomePressed()override;
    virtual void onVolumeMenuPressed() override;
	virtual void onBrightnessMenuPressed() override;
	virtual void onUpPressed() override;
	virtual void onDownPressed() override;
	virtual void onHardwareButtonStateChanged(uint32_t state) override;
	virtual void onSmokeDataUpdated(Model::TubeState* states) override;
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
	void onHomePressedReal();
	void gotoZeroizeScreen();
	void gotoSoftResetScreen();
	int getBrightness();
	int getVolume();

private:
    Screen2Presenter();

    Screen2View& view;
};

#endif // SCREEN2PRESENTER_HPP
