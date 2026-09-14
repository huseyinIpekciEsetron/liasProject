#ifndef SCREEN6PRESENTER_HPP
#define SCREEN6PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen6View;

class Screen6Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen6Presenter(Screen6View& v);

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

    virtual ~Screen6Presenter() {}
    virtual void onTimeDateUpdated(const TLUS::TimeData& time) override;
    virtual void onUpPressed() override;
	virtual void onDownPressed() override;
	virtual void onHomePressed() override;
	virtual void onMenuPressed() override;
	virtual void onVolumeMenuPressed() override;
	virtual void onBrightnessMenuPressed() override;
	virtual void onHardwareButtonStateChanged(uint32_t state) override;
	virtual void onSmokePressed() override;
	virtual void onShowWarning(Model::WarningType warning, int tubeIndex) override;

	// LOG KÖPRÜLERİ
	int getLogCount();
	int getLogHead();
	Model::LogEntry getLog(int index);

	// EKRAN VE MENÜ KÖPRÜLERİ
	int getBrightness();
	void saveBrightness(int b);
	int getVolume();
	void saveVolume(int v);

	void gotoAyarlarScreen();
	void gotoArizaScreen();
	void gotoTestScreen();
	void gotoLoglarScreen();
	void onHomePressedReal();
	void gotoZeroizeScreen();
	void gotoSoftResetScreen();

private:
    Screen6Presenter();

    Screen6View& view;
};

#endif // SCREEN6PRESENTER_HPP
