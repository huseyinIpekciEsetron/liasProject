#include <gui/screen7_screen/Screen7View.hpp>
#include "hc165_driver.h"
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <images/SVGDatabase.hpp>

Screen7View::Screen7View()
{
}

void Screen7View::setupScreen()
{
    Screen7ViewBase::setupScreen();

    holdTimer = 0;
    successTimer = 0;
    actionFired = false;
    currentButtonState = 0;
    lastButtonState = 0;
    waitingForRelease = false;
    currentVolume = presenter->getVolume();
	currentBrightness = presenter->getBrightness();
	currentMenu = MENU_NONE;
	menuTimeout = 0;

    // 1. TÜM BUTON LOGOLARINI AYARLAMA
    buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_ENTER_ID, 2.5f, 2.5f);
    buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BACK_ID, 2.5f, 2.5f);
    buttonContainer.setButtonIcon(BTN_HOME, SVG_HOME_ID, 2.5f, 2.5f);
    buttonContainer.setButtonIcon(BTN_MENU, SVG_MENU_ID, 2.5f, 2.5f);
    buttonContainer.setButtonIcon(BTN_UP, SVG_AYARLAR_ID, 2.5f, 2.5f); // Ayarlar Butonu
    buttonContainer.setButtonIcon(BTN_DOWN, SVG_TEST_ID, 2.5f, 2.5f);  // Test Butonu
    buttonContainer.updateButtonVisuals(0, BTN_IDLE);

    // 2. YAZILARI DİNAMİK DEĞİŞTİRME
    Model::PendingAction action = presenter->getPendingAction();

    if (action == Model::ACTION_ZEROIZE)
    {
        headerText.setTypedText(touchgfx::TypedText(T_ZEROIZE));
        MainTextArea.setTypedText(touchgfx::TypedText(T_ZEROIZETEXT));
        popupText.setTypedText(touchgfx::TypedText(T_EMERGENCYPOPUPTEXT));
    }
    else if (action == Model::ACTION_SOFT_RESET)
    {
        headerText.setTypedText(touchgfx::TypedText(T_SOFT_RESET_HEADER));
        MainTextArea.setTypedText(touchgfx::TypedText(T_SOFT_RESET_MAIN));
        popupText.setTypedText(touchgfx::TypedText(T_SOFT_RESET_POPUP));
    }

    headerText.invalidate();
    MainTextArea.invalidate();
    popupText.invalidate();

    pushBox.setColor(touchgfx::Color::getColorFromRGB(200, 200, 200));
    pushBox.setWidth(0);
    pushBox.invalidate();

    // 1. Önce Menüyü en üste al
        remove(menuContainer);
        add(menuContainer);

        // 2. Sonra Butonları Menünün üstüne al
        remove(buttonContainer);
        add(buttonContainer);

        // 3. UYARI PENCERESİNİ EN SONA AL Kİ KRAL O OLSUN (Her şeyin üstünde çıksın)
        remove(warningPopupContainer);
        add(warningPopupContainer);

    hidePopup();
}

void Screen7View::tearDownScreen()
{
    Screen7ViewBase::tearDownScreen();
}

void Screen7View::updateButtonState(uint32_t state)
{
    currentButtonState = state;
    buttonContainer.updateButtonVisuals(state, BTN_IDLE);
}

// Modelden gelen uyarıyı popup container'a yolla
void Screen7View::showWarningPopup(Model::WarningType warning, int tubeIndex)
{
	if (currentState == STATE_POPUP || holdTimer > 0)
	{
		return;
	}
	remove(warningPopupContainer);
	add(warningPopupContainer);
	warningCooldown = 30;

	// Uyarı mesajını ayarla, görünür yap ve ekrana çiz
	warningPopupContainer.showWarningMessage(warning, tubeIndex);
	warningPopupContainer.setVisible(true);
	warningPopupContainer.invalidate();
}

void Screen7View::showPopup()
{
    currentState = STATE_POPUP;
    holdTimer = 0;
    pushBox.invalidate();
    pushBox.setWidth(0);
    pushBox.setColor(touchgfx::Color::getColorFromRGB(200, 200, 200));

    popupbackground.setVisible(true);
    testPopupBox.setVisible(true);
    popupSvgImage.setVisible(true);
    popupText.setVisible(true);
    pushBoxWithBorder.setVisible(true);
    pushBox.setVisible(true);

    popupbackground.invalidate();
    testPopupBox.invalidate();
}

void Screen7View::hidePopup()
{
    currentState = STATE_INITIAL;
    holdTimer = 0;
    pushBox.invalidate();
    pushBox.setWidth(0);

    popupbackground.setVisible(false);
    testPopupBox.setVisible(false);
    popupSvgImage.setVisible(false);
    popupText.setVisible(false);
    pushBoxWithBorder.setVisible(false);
    pushBox.setVisible(false);

    popupbackground.invalidate();
    testPopupBox.invalidate();
}

void Screen7View::handleTickEvent()
{
    if (actionFired)
    {
        successTimer++;
        if (successTimer > 35) { presenter->cancelAndGoBack(); }
        return;
    }

    // --- MENÜ ZAMANLAYICISI ---
    if (menuTimeout > 0) {
        menuTimeout--;
        if (menuTimeout == 0) closeMenus();
    }

    uint32_t pressed_buttons = (currentButtonState ^ lastButtonState) & currentButtonState;

    // --- UYARI (WARNING) KALKANI ---
    if (isWarningPopupVisible())
    {
    	if (warningCooldown > 0)
		{
			warningCooldown--;
		}
    	else if (pressed_buttons != 0)
		{
			closeWarningSafe(); // Yeni güvenli fonksiyonumuzu çağırıyoruz!
		}
		return;
    }

    // --- AŞAMA 1: POPUP KAPALIYKEN ---
    if (currentState == STATE_INITIAL)
    {
        // 1. EĞER MENÜ AÇIKSA NORMAL MENÜ İŞLEMLERİ ÇALIŞSIN
        if (currentMenu != MENU_NONE) {
            lastButtonState = currentButtonState;
            return; // Menü işlerini Presenter halleder (onUpPressed vs.)
        }

        // 2. MENÜ KAPALIYSA: SADECE ENTER (POPUP AÇMA) VE BACK (ÇIKIŞ) ÇALIŞIR
        if (pressed_buttons & BTN_VOLUME_MENU) {
            showPopup();
            waitingForRelease = true;
        }
    }
    // --- AŞAMA 2: POPUP AÇIKKEN (SADECE ENTER VE BACK ÇALIŞIR, DİĞERLERİ KİLİTLİ) ---
    else if (currentState == STATE_POPUP)
    {
        if (waitingForRelease)
        {
            if ((currentButtonState & BTN_VOLUME_MENU) == 0) {
                waitingForRelease = false;
            }
        }
        else
        {
            if ((currentButtonState & BTN_VOLUME_MENU) != 0)
            {
                holdTimer++;
                int newWidth = (holdTimer * 192) / 100;
                if (newWidth > 192) newWidth = 192;

                if (pushBox.getWidth() != newWidth) {
                    pushBox.setWidth(newWidth);
                    pushBox.invalidate();
                }

                if (holdTimer >= 100)
                {
                    actionFired = true;
                    presenter->executePendingAction();

                    pushBox.setColor(touchgfx::Color::getColorFromRGB(0, 255, 0));
                    pushBox.invalidate();
                }
            }
            else
            {
                if (holdTimer > 0) {
                    pushBox.invalidate();
                    holdTimer = 0;
                    pushBox.setWidth(0);
                    pushBox.invalidate();
                }

                if (pressed_buttons & BTN_BRIGHTNESS_MENU) {
                    hidePopup();
                }
            }
        }
    }

    lastButtonState = currentButtonState;
}

void Screen7View::menuButtonPressed()
{
    if (currentMenu == MENU_NONE || currentMenu == MENU_VOLUME || currentMenu == MENU_BRIGHTNESS)
    {
        closeMenus();
        currentMenu = MENU_MAIN_LIST;
        menuContainer.resetSelection();
        menuContainer.setVisible(true);
        menuContainer.invalidate();
        menuTimeout = 250;

        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_ENTER_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BACK_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
        buttonContainer.invalidate();
    } else {
        closeMenus();
    }
}

void Screen7View::closeMenus()
{
    if (currentMenu != MENU_NONE)
    {
        currentMenu = MENU_NONE;
        menuTimeout = 0;

        // VolumeLevelContainer.setVisible(false);
        // BrighnessLevelContainer.setVisible(false);
        menuContainer.setVisible(false);

        // Ekrandaki standart buton logolarına geri dön
        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_ENTER_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BACK_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_HOME, SVG_HOME_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_MENU, SVG_MENU_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_UP, SVG_AYARLAR_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_DOWN, SVG_TEST_ID, 2.5f, 2.5f);

        // VolumeLevelContainer.invalidate();
        // BrighnessLevelContainer.invalidate();
        menuContainer.invalidate();
        buttonContainer.invalidate();
    }
}

void Screen7View::handleVolumeOrEnter()
{
    if (currentState == STATE_POPUP) return; // KİLİT

    if (currentMenu == MENU_MAIN_LIST) {
        int selected = menuContainer.getSelectedItem();
        if (selected == 0)      presenter->gotoAyarlarScreen();
        else if (selected == 1) presenter->gotoArizaScreen();
        else if (selected == 2) presenter->gotoTestScreen();
        else if (selected == 3) presenter->gotoLoglarScreen();
        else if (selected == 4) presenter->gotoSoftResetScreen(); // Soft Reset menüsü
		else if (selected == 5) presenter->gotoZeroizeScreen();   // Acil Silme menüsü

    }
    // ... Volume menüsü işlemleri ...
}

void Screen7View::handleBrightnessOrBack()
{
	if (currentState == STATE_POPUP) return;
        if (currentMenu != MENU_BRIGHTNESS)
        	closeMenus();

}

void Screen7View::buttonUpPressed()
{
    // EĞER ONAY POPUP'I AÇIKSA BU TUŞU YANITSIZ BIRAK!
    if (currentState == STATE_POPUP) return;

    if (currentMenu == MENU_MAIN_LIST) {
        menuTimeout = 250;
        menuContainer.moveUp();
    }
    // ... Volume ve Brightness kontrollerin varsa buraya gelebilir ...
    else if (currentMenu == MENU_NONE) {
        presenter->gotoAyarlarScreen();
    }
}

void Screen7View::buttonDownPressed()
{
    // EĞER ONAY POPUP'I AÇIKSA BU TUŞU YANITSIZ BIRAK!
    if (currentState == STATE_POPUP) return;

    if (currentMenu == MENU_MAIN_LIST) {
        menuTimeout = 250;
        menuContainer.moveDown();
    }
    // ... Volume ve Brightness kontrollerin varsa buraya gelebilir ...
    else if (currentMenu == MENU_NONE) {
        presenter->gotoTestScreen();
    }
}
void Screen7View::openVolumeMenu()
{
    if (currentMenu == MENU_MAIN_LIST) return;
    currentMenu = MENU_VOLUME;
    menuTimeout = 75;
    // BrighnessLevelContainer.setVisible(false);
    // VolumeLevelContainer.setVisible(true);
    // VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);
    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
    buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
    buttonContainer.invalidate();
}

void Screen7View::openBrightnessMenu()
{
    if (currentMenu == MENU_MAIN_LIST) return;
    currentMenu = MENU_BRIGHTNESS;
    menuTimeout = 75;
    // VolumeLevelContainer.setVisible(false);
    // BrighnessLevelContainer.setVisible(true);
    // BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
    buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
    buttonContainer.invalidate();
}

void Screen7View::closeWarningSafe()
{
    // Hala ilk açılış şokundaysa (yarım saniye dolmadıysa) kazara kapanmayı önle
    if (warningCooldown > 0) return;

    hideWarningPopup();
    warningPopupContainer.setVisible(false);
    warningPopupContainer.invalidate();

    // SİHİRLİ SATIR: Bu tuş basışını "Tüketilmiş" olarak işaretle!
    // Böylece handleTickEvent bu basışı yeni bir olay sanıp işlem başlatamaz.
    lastButtonState = currentButtonState;
}

void Screen7View::updateTimeDate(const TLUS::TimeData& time)
{
    timeDayContainer.updateTimeAndDate(time.hour, time.minute, time.day, time.month, time.year);
}
