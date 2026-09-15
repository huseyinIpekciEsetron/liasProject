#ifndef SCREEN5PRESENTER_HPP
#define SCREEN5PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen5View;

class Screen5Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen5Presenter(Screen5View& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~Screen5Presenter() {}

    // ModelListener'dan Gelen Tuş ve Donanım Sinyalleri
    virtual void onUpPressed() override;
    virtual void onDownPressed() override;
    virtual void onHomePressed() override;
    virtual void onSmokePressed() override;
    virtual void onMenuPressed() override;
    virtual void onVolumeMenuPressed() override;     // ENTER
    virtual void onBrightnessMenuPressed() override; // BACK
    virtual void onHardwareButtonStateChanged(uint32_t state) override;


    // Modelden Gelen Sistem Uyarıları (Popup)
    virtual void onShowWarning(Model::WarningType warning, int tubeIndex) override;
    virtual void onTimeDateUpdated(const TLUS::TimeData& time) override;

    // Menü ve Ekran Yönlendirmeleri
    void gotoAyarlarScreen();
    void gotoArizaScreen();
    void gotoTestScreen();
    void gotoLoglarScreen();
    void onHomePressedReal();
    void gotoZeroizeScreen();
	void gotoSoftResetScreen();

    // Model'e Erişim Fonksiyonları
	int getBrightness();
	void saveBrightness(int b);
	int getVolume();
	void saveVolume(int v);
	int getLedBrightness();
	void saveLedBrightness(int l);
	void setBuzzerLevel(AlarmLevel_t level);
	void setLedTestMode(int mode);
	void setTestModeActive(bool active);
	Model::BmbTelemetryData getBmbTelemetry();
	Model::CommTelemetryData getCommTelemetry();
	bool getCommLostFlag();
	void setBuzzerOverride(bool on) { model->setBuzzerOverride(on); }

private:
    Screen5Presenter();
    Screen5View& view;
};

#endif // SCREEN5PRESENTER_HPP
