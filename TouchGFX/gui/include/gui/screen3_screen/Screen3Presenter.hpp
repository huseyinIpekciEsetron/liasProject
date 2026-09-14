#ifndef SCREEN3PRESENTER_HPP
#define SCREEN3PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen3View;

class Screen3Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen3Presenter(Screen3View& v);

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

    virtual ~Screen3Presenter() {}


	virtual void onUpPressed() override;
	virtual void onDownPressed() override;
	virtual void onHomePressed() override;
	virtual void onSmokePressed() override;
	virtual void onHardwareButtonStateChanged(uint32_t state) override;
	virtual void onShowWarning(Model::WarningType warning, int tubeIndex) override;
	virtual void onVolumeMenuPressed() override;
	virtual void onBrightnessMenuPressed() override;
	virtual void onMenuPressed() override;
	virtual void onTimeDateUpdated(const TLUS::TimeData& time) override;


	// Model'e Erişim Fonksiyonları
	int getBrightness();
	void saveBrightness(int b);
	int getVolume();
	void saveVolume(int v);
	int getLedBrightness();
	void saveLedBrightness(int l);

	Model::SalvoMode getSalvoMode();
	void setSalvoMode(Model::SalvoMode mode);

	bool getWarningsEnabled();
	void setWarningsEnabled(bool state);


	void gotoAyarlarScreen();
	void gotoArizaScreen();
	void gotoTestScreen();
	void gotoLoglarScreen();
	void gotoZeroizeScreen();
	void gotoSoftResetScreen();


private:
    Screen3Presenter();

    Screen3View& view;
};

#endif // SCREEN3PRESENTER_HPP
