#ifndef SCREEN7PRESENTER_HPP
#define SCREEN7PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;
class Screen7View;

class Screen7Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen7Presenter(Screen7View& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~Screen7Presenter() {}

    virtual void onHardwareButtonStateChanged(uint32_t state) override;
    virtual void onTimeDateUpdated(const TLUS::TimeData& time) override;

    // --- MENÜ VE UYARI DİNLEYİCİLERİ ---
    virtual void onHomePressed() override;
    virtual void onSmokePressed() override;
    virtual void onMenuPressed() override;
    virtual void onUpPressed() override;
    virtual void onVolumeMenuPressed() override;
	virtual void onBrightnessMenuPressed() override;
    virtual void onDownPressed() override;
    virtual void onShowWarning(Model::WarningType warning, int tubeIndex) override;
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

    Model::PendingAction getPendingAction();
    void executePendingAction();
    void cancelAndGoBack();

private:
    Screen7Presenter();
    Screen7View& view;
};

#endif // SCREEN7PRESENTER_HPP
