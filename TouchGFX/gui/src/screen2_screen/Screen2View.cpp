#include <gui/screen2_screen/Screen2View.hpp>
#include "hc165_driver.h"
#include <images/SVGDatabase.hpp>

Screen2View::Screen2View()
{

}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();

    currentVolume = presenter->getVolume();
	currentBrightness = presenter->getBrightness();

    remove(VolumeLevelContainer);
   	remove(BrighnessLevelContainer);

   	add(VolumeLevelContainer);
   	add(BrighnessLevelContainer);

   	remove(menuContainer); // Tasarımdaki adın menuContainer [cite: 4356]
	add(menuContainer);

	remove(warningPopupContainer);
	add(warningPopupContainer);

	remove(buttonContainer);
	add(buttonContainer);

	updateHeaderBoxSize();
}

void Screen2View::tearDownScreen()
{
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::handleVolumeOrEnter()
{
    if (currentMenu == MENU_MAIN_LIST)
    {
        // =======================================================
        // DURUM 1: MENÜ AÇIK -> ENTER GÖREVİ GÖRÜR!
        // =======================================================
        int selected = menuContainer.getSelectedItem();
        if (selected == 0)      presenter->gotoAyarlarScreen();
        else if (selected == 1) presenter->gotoArizaScreen();
        else if (selected == 2) presenter->gotoTestScreen();
        else if (selected == 3) presenter->gotoLoglarScreen();
        else if (selected == 4) presenter->gotoSoftResetScreen(); // Soft Reset menüsü
		else if (selected == 5) presenter->gotoZeroizeScreen();   // Acil Silme menüsü
    }
    else
    {
        // =======================================================
        // DURUM 2: MENÜ KAPALI VEYA SES MENÜSÜ AÇIK -> SES GÖREVİ GÖRÜR!
        // =======================================================
        if (currentMenu != MENU_VOLUME) {
            openVolumeMenu(); // Ses barını aç
        } else {
            // Ses barı zaten açıksa, aynı tuşa basarak sesi artırabilir (Opsiyonel)
        	closeMenus();
        }
    }
}

void Screen2View::handleBrightnessOrBack()
{
    if (currentMenu == MENU_MAIN_LIST)
    {
        // =======================================================
        // DURUM 1: MENÜ AÇIK -> BACK (GERİ) GÖREVİ GÖRÜR VE MENÜYÜ KAPATIR!
        // =======================================================
        closeMenus();
    }
    else
    {
        // =======================================================
        // DURUM 2: MENÜ KAPALI -> PARLAKLIK GÖREVİ GÖRÜR!
        // =======================================================
        if (currentMenu != MENU_BRIGHTNESS) {
            openBrightnessMenu(); // Parlaklık barını aç
        } else {
        	closeMenus();
        }
    }
}

void Screen2View::openVolumeMenu()
{
	if (currentMenu == MENU_MAIN_LIST) return;

    currentMenu = MENU_VOLUME; // Sistemi Ses Moduna Geçir
    menuTimeout = 150; // Ekranda 4 saniye kalsın (60*5)

    BrighnessLevelContainer.setVisible(false); // Parlaklığı kapat
    VolumeLevelContainer.setVisible(true);      // Sesi aç

    VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);

    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
	buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);

    BrighnessLevelContainer.invalidate();
    VolumeLevelContainer.invalidate();
    buttonContainer.invalidate();
}

void Screen2View::openBrightnessMenu()
{
	if (currentMenu == MENU_MAIN_LIST) return;

    currentMenu = MENU_BRIGHTNESS; // Sistemi Parlaklık Moduna Geçir
    menuTimeout = 150;

    VolumeLevelContainer.setVisible(false);
    BrighnessLevelContainer.setVisible(true);

    BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);

    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
   	buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);

    VolumeLevelContainer.invalidate();
    BrighnessLevelContainer.invalidate();
    buttonContainer.invalidate();
}

void Screen2View::buttonUpPressed()
{

    // Hangi menü açıksa onun değerini arttır
    if (currentMenu == MENU_VOLUME) {
        if (currentVolume < 5) currentVolume++;
        VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);
        menuTimeout = 150; // Tuşa basıldıkça süreyi sıfırla
        presenter->saveVolume(currentVolume);// doanıma ses seviyesini gönder
    }
    else if (currentMenu == MENU_BRIGHTNESS) {
        if (currentBrightness < 5) currentBrightness++;
        BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
        menuTimeout = 150;
        presenter->saveBrightness(currentBrightness);
    }
    else if (currentMenu == MENU_MAIN_LIST) {
    	menuTimeout = 500;
   		menuContainer.moveUp(); // Seçimi yukarı kaydır
   	}
    else if(currentMenu == MENU_NONE)
    {
    	presenter->gotoAyarlarScreen();
    }
}

void Screen2View::buttonDownPressed()
{
    // Hangi menü açıksa onun değerini azalt
    if (currentMenu == MENU_VOLUME) {
        if (currentVolume > 0) currentVolume--;
        VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);
        menuTimeout = 150;
        presenter->saveVolume(currentVolume);// doanıma ses seviyesini gönder
    }
    else if (currentMenu == MENU_BRIGHTNESS) {
        if (currentBrightness > 0) currentBrightness--;
        BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
        menuTimeout = 150;
        presenter->saveBrightness(currentBrightness);
    }
    else if (currentMenu == MENU_MAIN_LIST) {
    	menuTimeout = 500;
		menuContainer.moveDown(); // Seçimi aşağı kaydır
	}
    else if(currentMenu == MENU_NONE)
	{
		presenter->gotoTestScreen();
	}
}

void Screen2View::menuButtonPressed()
{
    if (currentMenu == MENU_NONE || currentMenu == MENU_VOLUME || currentMenu == MENU_BRIGHTNESS)
    {
        // Eğer menü kapalıysa veya ses/parlaklık açıksa hepsini temizle ve Ana Menüyü aç!
        closeMenus();

        currentMenu = MENU_MAIN_LIST;
        menuContainer.resetSelection();
        menuContainer.setVisible(true);
        menuContainer.invalidate();
        menuTimeout = 500; // Ekranda kalma süresi

        // IKONLARI ENTER VE BACK OLARAK DEĞİŞTİR!
        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_ENTER_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BACK_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
		buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
		buttonContainer.invalidate();
    }
    else if (currentMenu == MENU_MAIN_LIST)
    {
        // Menü zaten açıksa ve tekrar Menü tuşuna basılırsa menüyü kapat.
        closeMenus();
    }
}

void Screen2View::closeMenus()
{
    if (currentMenu != MENU_NONE)
    {
        currentMenu = MENU_NONE;
        menuTimeout = 0; // Sayacı sıfırla

        VolumeLevelContainer.setVisible(false);
        BrighnessLevelContainer.setVisible(false);
        menuContainer.setVisible(false);

        // IKONLARI ESKİ (SES VE PARLAKLIK) HALLERİNE GERİ GETİR!
        buttonContainer.setButtonIcon(BTN_MENU, SVG_MENU_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_HOME, SVG_HOME_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_SOUND_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BRIGHT_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_UP, SVG_AYARLAR_ID, 2.5f, 2.5f);
		buttonContainer.setButtonIcon(BTN_DOWN, SVG_TEST_ID, 2.5f, 2.5f);

        VolumeLevelContainer.invalidate();
        BrighnessLevelContainer.invalidate();
        menuContainer.invalidate();
    }
}

void Screen2View::homeButtonPressed()
{
    if (currentMenu != MENU_NONE)
    {
        // 1. DURUM: Ekranda bir menü açık. Home tuşu "ESCAPE (KAPAT)" görevi görür.
        //closeMenus();
    	presenter->onHomePressedReal();
    }
    else
    {
    	presenter->onHomePressedReal();
    }
}

void Screen2View::handleTickEvent()
{
    if (menuTimeout > 0)
	{
		menuTimeout--;
		if (menuTimeout == 0)
		{
			// Süre doldu, menüyü NONE durumuna al ve ekrandakileri gizle
			closeMenus();
		}
	}
}

void Screen2View::updateSmokeStatus(Model::TubeState* states)
{
    // Bitmask mantığı: İlk 4 bit Sol Ön, sonraki 4 bit Sağ Ön vb.
    // İsimler Designer'da verdiğin Container isimleri olmalı.

    /*SolOnsisHavanContainer.setStatus((status & 1), (status & 2), (status & 4), (status & 8));
    SagOnsisHavanContainer.setStatus((status & 16), (status & 32), (status & 64), (status & 128));
    SolArkasisHavanContainer.setStatus((status & 256), (status & 512), (status & 1024), (status & 2048));
    SagArkasisHavanContainer.setStatus((status & 4096), (status & 8192), (status & 16384), (status & 32768));*/

	// Sol Ön Grup (Örn: 0, 1, 2, 3)
	    SolOnsisHavanContainer.setStatus(states[3], states[2], states[1], states[0]);

	    // Sağ Ön Grup (Örn: 4, 5, 6, 7)
	    SolArkasisHavanContainer.setStatus(states[7], states[6], states[5], states[4]);

	    // Sol Arka Grup (Örn: 8, 9, 10, 11)
	    SagOnsisHavanContainer.setStatus(states[11], states[10], states[9], states[8]);

	    // Sağ Arka Grup (Örn: 12, 13, 14, 15)
	    SagArkasisHavanContainer.setStatus(states[15], states[14], states[13], states[12]);
}

void Screen2View::updateButtonVisuals(uint32_t state)
{
	buttonContainer.updateButtonVisuals(state, BTN_SMOKE);
}

void Screen2View::showWarningPopup(Model::WarningType warning, int tubeIndex)
{
    // Ekrana eklediğimiz Custom Container'ın fonksiyonunu tetikle
    warningPopupContainer.showWarningMessage(warning, tubeIndex);
}

void Screen2View::updateHeaderBoxSize()
{
    // 1. Seçili dildeki metnin ekranda kaplayacağı GERÇEK piksel genişliğini al
    uint16_t textWidth = textArea1.getTextWidth();

    // 2. Kutunun metne yapışmaması için sağdan ve soldan boşluk (padding) belirle
    uint16_t horizontalPadding = 80; // (Örn: Soldan 20px, Sağdan 20px)

    // 3. Kutunun yeni genişliğini hesapla ve ayarla
    uint16_t newBoxWidth = textWidth + horizontalPadding;
    headerBox.setWidth(newBoxWidth);

    // 4. (KRİTİK) Kutu büyüdüğü veya küçüldüğü için merkezinin kaymaması lazım.
    // Kutuyu metnin tam ortasına hizalayalım:
    int textCenterX = textArea1.getX() + (textArea1.getWidth() / 2);
    int newBoxX = textCenterX - (newBoxWidth / 2) - 30;
    headerBox.setX(newBoxX);

    // 5. Ekrandaki değişiklikleri çizdir
    headerBox.invalidate();
    textArea1.invalidate();
}

void Screen2View::updateTimeDate(const TLUS::TimeData& time)
{
    // Container senin yazdığın eski parametre yapısını kullandığı için veriyi burada açıyoruz
    timeDayContainer.updateTimeAndDate(time.hour, time.minute, time.day, time.month, time.year);
}
