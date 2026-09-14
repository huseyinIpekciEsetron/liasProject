#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    // Tuş Yönlendiricileri
    void enterPressed();
    void backPressed();
    void buttonUpPressed();
    void buttonDownPressed();

    void updateButtonVisuals(uint32_t state);
    void showWarningPopup(Model::WarningType warning, int tubeIndex);

    // ScrollList için gereken TouchGFX fonksiyonu
    virtual void scrollListUpdateItem(SettingsItemContainer& item, int16_t itemIndex);

    virtual void handleTickEvent();
    void updateTimeDate(const TLUS::TimeData& time);
    void menuButtonPressed();
	void closeMenus();
	bool isPopupVisible() { return warningPopupContainer.isVisible(); }
	void hidePopup() { warningPopupContainer.hideWarning(); }

protected:
    // Ayarlar Ekranı Durum Makinesi
    enum SettingsState {
        STATE_NAVIGATING,   // Listede Gezinme
        STATE_EDITING,      // Parlaklık veya Ses Düzenleme
        STATE_SHOWING_INFO,  // Popup Açık
		STATE_MENU_OPEN     // YAN MENÜ AÇIK
    };

    SettingsState currentState = STATE_NAVIGATING;
    int selectedIndex = 0;
    static const int MAX_ITEMS = 8;
    int menuTimeout = 0;

    void updateHeaderBoxSize();
};

#endif // SCREEN3VIEW_HPP
