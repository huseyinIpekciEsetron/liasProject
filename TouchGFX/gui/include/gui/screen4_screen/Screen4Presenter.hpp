#ifndef SCREEN4PRESENTER_HPP
#define SCREEN4PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen4View;

class Screen4Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen4Presenter(Screen4View& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~Screen4Presenter() {}

    virtual void onUpPressed() override;
    virtual void onDownPressed() override;
    virtual void onHomePressed() override;
    virtual void onMenuPressed() override;
    virtual void onVolumeMenuPressed() override;     // ENTER
    virtual void onBrightnessMenuPressed() override; // BACK
    virtual void onHardwareButtonStateChanged(uint32_t state) override;
    virtual void onSmokePressed() override;

    virtual void onShowWarning(Model::WarningType warning, int tubeIndex) override;
    virtual void onTimeDateUpdated(const TLUS::TimeData& time) override;
    virtual void onSystemStatusUpdated(const TLUS::SystemStatusPayload& status) override;

    // --- ARIZA VE HAFIZA VERİLERİNE ERİŞİM KÖPRÜLERİ ---
	bool getFaultStatus(int index);
	bool getFaultIsFrag(int index);

	bool getStoredCommLostFlag();
	bool getStoredHwErrorFlag();
	TLUS::SystemStatusPayload getStoredSystemStatus();

	uint16_t getCommFaultTime();
	uint16_t getHwFaultTime();
	uint16_t getProcFaultTime(int index);
	uint16_t getSensFaultTime(int s, int index);
	uint16_t getTubeFaultTime(int index);

	void clearAllFaults();

    int getBrightness();
	void saveBrightness(int b);
	int getVolume();
	void saveVolume(int v);
	void goBackToSettings();

    // Menü Yönlendirmeleri
    void gotoAyarlarScreen();
    void gotoArizaScreen();
    void gotoTestScreen();
    void gotoLoglarScreen();
    void onHomePressedReal();
    void gotoZeroizeScreen();
	void gotoSoftResetScreen();


private:
    Screen4Presenter();
    Screen4View& view;
};

#endif // SCREEN4PRESENTER_HPP
