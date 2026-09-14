#include <gui/screen4_screen/Screen4View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <images/SVGDatabase.hpp>
#include <stdio.h>

Screen4View::Screen4View() {}

void Screen4View::setupScreen()
{
    Screen4ViewBase::setupScreen();

    currentVolume = presenter->getVolume();
	currentBrightness = presenter->getBrightness();


	remove(VolumeLevelContainer); add(VolumeLevelContainer); VolumeLevelContainer.setVisible(false);
	remove(BrighnessLevelContainer); add(BrighnessLevelContainer); BrighnessLevelContainer.setVisible(false);
	remove(menuContainer); add(menuContainer); menuContainer.setVisible(false);
	remove(warningPopupContainer); add(warningPopupContainer);
	remove(buttonContainer); add(buttonContainer);


	last_comm_lost_flag = presenter->getStoredCommLostFlag();
	last_hw_error_flag = presenter->getStoredHwErrorFlag();
	for (int i = 0; i < 16; i++) {
		last_ariza_hafizasi[i] = presenter->getFaultStatus(i);
	}

	refreshFaultList();

	updateHeaderBoxSize();
}

void Screen4View::tearDownScreen()
{
    Screen4ViewBase::tearDownScreen();
}

// ============================================
// DİNAMİK İKON DEĞİŞTİRİCİ
// ============================================
void Screen4View::updateContextualIcons()
{
    bool isClearRowSelected = (activeFaultCount > 0 && faultList[selectedIndex].isClearAction);

    // Eğer Yan Menü açıksa VEYA "Tüm Arızaları Sil" satırındaysak ikonlar ENTER/BACK olur
    if (currentState == STATE_MENU_OPEN || (currentState == STATE_NAVIGATING && isClearRowSelected)) {
        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_ENTER_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BACK_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
		buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
    }
    else {
        // Normal gezinme sırasında ikonlar SES/PARLAKLIK olur
        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_SOUND_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BRIGHT_ID, 2.5f, 2.5f);
        if(activeFaultCount > 0)
        {
        	buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
			buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
        }
        else
        {
        	buttonContainer.setButtonIcon(BTN_UP, SVG_AYARLAR_ID, 2.5f, 2.5f);
			buttonContainer.setButtonIcon(BTN_DOWN, SVG_TEST_ID, 2.5f, 2.5f);
        }
    }
}

// ============================================
// AKILLI YÖNLENDİRİCİLER
// ============================================
void Screen4View::handleVolumeOrEnter()
{
    bool isClearRowSelected = (activeFaultCount > 0 && faultList[selectedIndex].isClearAction);

    if (currentState == STATE_MENU_OPEN) {
        int selected = menuContainer.getSelectedItem();
        if (selected == 0)      presenter->gotoAyarlarScreen();
        else if (selected == 1) presenter->gotoArizaScreen();
        else if (selected == 2) presenter->gotoTestScreen();
        else if (selected == 3) presenter->gotoLoglarScreen();
        else if (selected == 4) presenter->gotoSoftResetScreen(); // Soft Reset menüsü
		else if (selected == 5) presenter->gotoZeroizeScreen();   // Acil Silme menüsü
    }
    else if (currentState == STATE_NAVIGATING && isClearRowSelected) {
        // "Arızaları Sil" satırındayız -> ENTER çalışır!
        presenter->clearAllFaults();

        // Model güncellenince checkAndRefreshFaults() ekranı otomatik yenileyecek
    }
    else {
        // Normal satırdayız -> SES çalışır!
        if (currentState != STATE_VOLUME_OPEN) openVolumeMenu();
        else closeMenus();
    }
}

void Screen4View::handleBrightnessOrBack()
{
    bool isClearRowSelected = (activeFaultCount > 0 && faultList[selectedIndex].isClearAction);

    if (currentState != STATE_NAVIGATING && currentState != STATE_MENU_OPEN) {
        closeMenus(); // Ses/Parlaklık açıksa kapat
    }
    else if (currentState == STATE_MENU_OPEN) {
        closeMenus();
    }
    else if (currentState == STATE_NAVIGATING && isClearRowSelected) {
        // "Arızaları Sil" satırındayız -> BACK çalışır (Ayarlara döndürür)
        presenter->goBackToSettings();
    }
    else {
        // Normal satırdayız -> PARLAKLIK çalışır!
        if (currentState != STATE_BRIGHTNESS_OPEN) openBrightnessMenu();
        else closeMenus();
    }
}

void Screen4View::openVolumeMenu()
{
    closeMenus();
    currentState = STATE_VOLUME_OPEN;
    menuTimeout = 150;
    VolumeLevelContainer.setVisible(true);
    VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);
    VolumeLevelContainer.invalidate();
    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
	buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
	buttonContainer.invalidate();
}

void Screen4View::openBrightnessMenu()
{
    closeMenus();
    currentState = STATE_BRIGHTNESS_OPEN;
    menuTimeout =150;
    BrighnessLevelContainer.setVisible(true);
    BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
    BrighnessLevelContainer.invalidate();
    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
	buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
	buttonContainer.invalidate();
}

// Canlı Güncelleme Dedektörü
void Screen4View::checkAndRefreshFaults()
{
    bool changed = false;

    // ESKİ: getCommLostFlag() -> YENİ: getStoredCommLostFlag()
	if (presenter->getStoredCommLostFlag() != last_comm_lost_flag) {
		last_comm_lost_flag = presenter->getStoredCommLostFlag();
		changed = true;
	}

	// ESKİ: getHwErrorFlag() -> YENİ: getStoredHwErrorFlag()
	if (presenter->getStoredHwErrorFlag() != last_hw_error_flag) {
		last_hw_error_flag = presenter->getStoredHwErrorFlag();
		changed = true;
	}

    for (int i = 0; i < 16; i++) {
        if (presenter->getFaultStatus(i) != last_ariza_hafizasi[i]) {
            last_ariza_hafizasi[i] = presenter->getFaultStatus(i);
            changed = true;
        }
    }

    // Eğer herhangi bir arıza durumu değişmişse ekranı canlı olarak güncelle!
    if (changed) {
        refreshFaultList();
    }
}

void Screen4View::refreshFaultList()
{
    activeFaultCount = 0;

    // Kritik Hatalar
    if (presenter->getStoredCommLostFlag()) {
        faultList[activeFaultCount++] = { T_FAULTCOMMLOST, -1, presenter->getCommFaultTime(), false };
    }
    if (presenter->getStoredHwErrorFlag()) {
        faultList[activeFaultCount++] = { T_FAULTHWERR, -1, presenter->getHwFaultTime(), false };
    }

    // SİSTEM VE SENSÖR HATALARI (HAFIZADAKİLER OKUNUYOR!)
    TLUS::SystemStatusPayload status = presenter->getStoredSystemStatus();

    if (status.processorFaults.ramTesti) { faultList[activeFaultCount++] = { T_FLT_PROC_RAM, -1, presenter->getProcFaultTime(0), false }; }
    if (status.processorFaults.kaliciBellekTesti) { faultList[activeFaultCount++] = { T_FLT_PROC_NVRAM, -1, presenter->getProcFaultTime(1), false }; }
    if (status.processorFaults.bellekDosyasiTesti) { faultList[activeFaultCount++] = { T_FLT_PROC_MEMFILE, -1, presenter->getProcFaultTime(2), false }; }
    if (status.processorFaults.nvsramTesti) { faultList[activeFaultCount++] = { T_FLT_PROC_NVSRAM, -1, presenter->getProcFaultTime(3), false }; }
    if (status.processorFaults.bellekDoluluk) { faultList[activeFaultCount++] = { T_FLT_PROC_MEMFULL, -1, presenter->getProcFaultTime(4), false }; }
    if (status.processorFaults.seriKanal1) { faultList[activeFaultCount++] = { T_FLT_PROC_SER1, -1, presenter->getProcFaultTime(5), false }; }
    if (status.processorFaults.seriKanal2) { faultList[activeFaultCount++] = { T_FLT_PROC_SER2, -1, presenter->getProcFaultTime(6), false }; }
    if (status.processorFaults.seriKanal3) { faultList[activeFaultCount++] = { T_FLT_PROC_SER3, -1, presenter->getProcFaultTime(7), false }; }
    if (status.processorFaults.seriKanal4) { faultList[activeFaultCount++] = { T_FLT_PROC_SER4, -1, presenter->getProcFaultTime(8), false }; }
    if (status.processorFaults.arayuzKarti) { faultList[activeFaultCount++] = { T_FLT_PROC_IFACE, -1, presenter->getProcFaultTime(9), false }; }
    if (status.processorFaults.anaBesleme) { faultList[activeFaultCount++] = { T_FLT_PROC_PWR, -1, presenter->getProcFaultTime(10), false }; }
    if (status.processorFaults.islemciDurumuKapanma) { faultList[activeFaultCount++] = { T_FLT_PROC_SHUTDOWN, -1, presenter->getProcFaultTime(11), false }; }
    if (status.processorFaults.gucKartiSeriKanal) { faultList[activeFaultCount++] = { T_FLT_PROC_PWR_SER, -1, presenter->getProcFaultTime(12), false }; }
    if (status.processorFaults.sicaklikEsikAsimi) { faultList[activeFaultCount++] = { T_FLT_PROC_TEMP, -1, presenter->getProcFaultTime(13), false }; }

    for (int i = 0; i < 4; i++) {
        int sNo = i + 1; // 1,2,3,4
        if (status.sensorFaults[i].bant_I_II_Karti) { faultList[activeFaultCount++] = { T_FLT_SENS_B12, sNo, presenter->getSensFaultTime(i,0), false }; }
        if (status.sensorFaults[i].bant_III_Sensor0) { faultList[activeFaultCount++] = { T_FLT_SENS_B3_0, sNo, presenter->getSensFaultTime(i,1), false }; }
        if (status.sensorFaults[i].bant_III_Karti_Sensor1) { faultList[activeFaultCount++] = { T_FLT_SENS_B3_1, sNo, presenter->getSensFaultTime(i,2), false }; }
        if (status.sensorFaults[i].bant_III_Karti_Sensor2) { faultList[activeFaultCount++] = { T_FLT_SENS_B3_2, sNo, presenter->getSensFaultTime(i,3), false }; }
        if (status.sensorFaults[i].sensor_Birimi_Kontrol_Karti) { faultList[activeFaultCount++] = { T_FLT_SENS_CTRL, sNo, presenter->getSensFaultTime(i,4), false }; }
        if (status.sensorFaults[i].kontrollu_Kapanma) { faultList[activeFaultCount++] = { T_FLT_SENS_SHUTDOWN, sNo, presenter->getSensFaultTime(i,5), false }; }
        if (status.sensorFaults[i].guc_Karti_Seri_Kanal) { faultList[activeFaultCount++] = { T_FLT_SENS_PWR_SER, sNo, presenter->getSensFaultTime(i,6), false }; }
        if (status.sensorFaults[i].basinc_Durumu) { faultList[activeFaultCount++] = { T_FLT_SENS_PRESS, sNo, presenter->getSensFaultTime(i,7), false }; }
        if (status.sensorFaults[i].volt_3_7V) { faultList[activeFaultCount++] = { T_FLT_SENS_V3_7, sNo, presenter->getSensFaultTime(i,8), false }; }
        if (status.sensorFaults[i].volt_7_4V) { faultList[activeFaultCount++] = { T_FLT_SENS_V7_4, sNo, presenter->getSensFaultTime(i,9), false }; }
        if (status.sensorFaults[i].volt_16V) { faultList[activeFaultCount++] = { T_FLT_SENS_V16, sNo, presenter->getSensFaultTime(i,10), false }; }
        if (status.sensorFaults[i].volt_80V) { faultList[activeFaultCount++] = { T_FLT_SENS_V80, sNo, presenter->getSensFaultTime(i,11), false }; }
        if (status.sensorFaults[i].volt_neg7_4V) { faultList[activeFaultCount++] = { T_FLT_SENS_VN7_4, sNo, presenter->getSensFaultTime(i,12), false }; }
        if (status.sensorFaults[i].volt_neg3_7V) { faultList[activeFaultCount++] = { T_FLT_SENS_VN3_7, sNo, presenter->getSensFaultTime(i,13), false }; }
        if (status.sensorFaults[i].ana_Besleme) { faultList[activeFaultCount++] = { T_FLT_SENS_PWR, sNo, presenter->getSensFaultTime(i,14), false }; }
        if (status.sensorFaults[i].sicaklik_Durumu) { faultList[activeFaultCount++] = { T_FLT_SENS_TEMP_LIM, sNo, presenter->getSensFaultTime(i,15), false }; }
        if (status.sensorFaults[i].sicaklik_Sensoru) { faultList[activeFaultCount++] = { T_FLT_SENS_TEMP_SNS, sNo, presenter->getSensFaultTime(i,16), false }; }
    }

    // Tüp (Mühimmat) Arızaları (Bunlar sadece Aktif olarak yaşar)
    for (int i = 0; i < 16; i++) {
        if (presenter->getFaultStatus(i)) {
            touchgfx::TypedTextId id = presenter->getFaultIsFrag(i) ? T_FAULTFRAG : T_FAULTSMOKE;
            faultList[activeFaultCount++] = { id, i + 1, presenter->getTubeFaultTime(i), false };
        }
    }

    // Arızaları Sil Butonu
    if (activeFaultCount > 0) {
        faultList[activeFaultCount++] = { T_FAULTCLEARALL, -1, 0, true };

        popupbox.setVisible(false);
        popuptext.setVisible(false);
        scrollList.setVisible(true);
        scrollList.setNumberOfItems(activeFaultCount);
    } else {
        popupbox.setVisible(true);
        popuptext.setVisible(true);
        scrollList.setVisible(false);
    }

    if (selectedIndex >= activeFaultCount && activeFaultCount > 0) {
        selectedIndex = activeFaultCount - 1;
    } else if (activeFaultCount == 0) {
        selectedIndex = 0;
    }

    for (int i = 0; i < activeFaultCount; i++) {
        scrollList.itemChanged(i);
    }

    scrollList.invalidate();
    popupbox.invalidate();
    popuptext.invalidate();

    updateContextualIcons();
    updateScrollBar();
}

void Screen4View::scrollListUpdateItem(FaultItemContainer& item, int16_t itemIndex)
{
	if (itemIndex < activeFaultCount) {

		// Eğer satır "Tüm Arızaları Sil" butonuysa saati 0xFFFF (Gizli) gönder, değilse hafızadaki saati gönder
		uint16_t tVal = faultList[itemIndex].isClearAction ? 0xFFFF : faultList[itemIndex].timeVal;

		// Yeni fonksiyonumuzu 3 parametreyle (ID, Tüp/Sensör No, Saat) çağırıyoruz
		item.setupFault(faultList[itemIndex].textId, faultList[itemIndex].tubeIndex, tVal);

		item.setHighlighted(itemIndex == selectedIndex);
	}
}

void Screen4View::handleTickEvent()
{
	checkAndRefreshFaults();

    if (menuTimeout > 0) {
        menuTimeout--;
        if (menuTimeout == 0) {
            closeMenus();
        }
    }
}

void Screen4View::menuButtonPressed()
{
    if (currentState == STATE_MENU_OPEN) {
        closeMenus();
    } else {
        closeMenus(); // Önce açık olan Ses/Parlaklık varsa kapat
        currentState = STATE_MENU_OPEN;
        menuContainer.resetSelection();
        menuContainer.setVisible(true);
        menuContainer.invalidate();
        menuTimeout = 500;
        updateContextualIcons(); // İkonları Enter/Back yap
    }
}

void Screen4View::buttonUpPressed()
{
    if (currentState == STATE_MENU_OPEN) {
        menuContainer.moveUp();
        menuTimeout = 500;
    }
    else if (currentState == STATE_VOLUME_OPEN) {
        if (currentVolume < 5) currentVolume++;
        VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);
        presenter->saveVolume(currentVolume);
        menuTimeout = 150;
    }
    else if (currentState == STATE_BRIGHTNESS_OPEN) {
        if (currentBrightness < 5) currentBrightness++;
        BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
        presenter->saveBrightness(currentBrightness);
        menuTimeout = 150;
    }
    else if (currentState == STATE_NAVIGATING && activeFaultCount > 0) {
        int oldIndex = selectedIndex;

        if (selectedIndex > 0) selectedIndex--;
        else selectedIndex = activeFaultCount - 1;
        scrollList.itemChanged(oldIndex);
        scrollList.itemChanged(selectedIndex);

        scrollList.animateToItem(selectedIndex, 10);
        updateContextualIcons(); // YENİ: İkonları duruma göre değiştir
        updateScrollBar();
    }
    else if(currentState == STATE_NAVIGATING && activeFaultCount == 0) {
    	presenter->gotoAyarlarScreen();
    }
}

void Screen4View::buttonDownPressed()
{

    if (currentState == STATE_MENU_OPEN) {
        menuContainer.moveDown();
        menuTimeout = 500;
    }
    else if (currentState == STATE_VOLUME_OPEN) {
        if (currentVolume > 0) currentVolume--;
        VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);
        presenter->saveVolume(currentVolume);
        menuTimeout = 150;
    }
    else if (currentState == STATE_BRIGHTNESS_OPEN) {
        if (currentBrightness > 0) currentBrightness--;
        BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
        presenter->saveBrightness(currentBrightness);
        menuTimeout = 150;
    }
    else if (currentState == STATE_NAVIGATING && activeFaultCount > 0) {
        int oldIndex = selectedIndex;

        if (selectedIndex < activeFaultCount - 1) selectedIndex++;
        else selectedIndex = 0;
        scrollList.itemChanged(oldIndex);
        scrollList.itemChanged(selectedIndex);

        scrollList.animateToItem(selectedIndex, 10);
        updateContextualIcons(); // YENİ: İkonları duruma göre değiştir
        updateScrollBar();
    }
    else if(currentState == STATE_NAVIGATING && activeFaultCount == 0) {
		presenter->gotoTestScreen();
	}
}

void Screen4View::closeMenus()
{
    if (currentState != STATE_NAVIGATING) {
        currentState = STATE_NAVIGATING;
        menuContainer.setVisible(false);
        VolumeLevelContainer.setVisible(false);
        BrighnessLevelContainer.setVisible(false);

        menuContainer.invalidate();
        VolumeLevelContainer.invalidate();
        BrighnessLevelContainer.invalidate();
        menuTimeout = 0;

        updateContextualIcons(); // Menü kapanınca ikonları geri yükle
    }
}

void Screen4View::enterPressed()
{
    if (currentState == STATE_MENU_OPEN) {
        int selected = menuContainer.getSelectedItem();
        if (selected == 0)      presenter->gotoAyarlarScreen();
        else if (selected == 1) presenter->gotoArizaScreen();
        else if (selected == 2) presenter->gotoTestScreen();
        else if (selected == 3) presenter->gotoLoglarScreen();
        return;
    }

    if (currentState == STATE_NAVIGATING && activeFaultCount > 0)
    {
        // Eğer seçili olan eleman "TÜM ARIZALARI SİL" ise
        if (faultList[selectedIndex].isClearAction) {
            presenter->clearAllFaults();
        }
    }
}

void Screen4View::backPressed()
{
    if (currentState == STATE_MENU_OPEN) {
        closeMenus();
    } else {
        // Ana ekrana dön
        //presenter->onHomePressedReal();
    }
}



void Screen4View::updateButtonVisuals(uint32_t state)
{
    buttonContainer.updateButtonVisuals(state, 0);
}

// uayrı Popup Gösterme Fonksiyonu
void Screen4View::showWarningPopup(Model::WarningType warning, int tubeIndex)
{
    warningPopupContainer.showWarningMessage(warning, tubeIndex);
}

void Screen4View::updateHeaderBoxSize()
{
    // 1. Seçili dildeki metnin ekranda kaplayacağı GERÇEK piksel genişliğini al
    uint16_t textWidth = headerText.getTextWidth();

    // 2. Kutunun metne yapışmaması için sağdan ve soldan boşluk (padding) belirle
    uint16_t horizontalPadding = 80; // (Örn: Soldan 20px, Sağdan 20px)

    // 3. Kutunun yeni genişliğini hesapla ve ayarla
    uint16_t newBoxWidth = textWidth + horizontalPadding;
    headerBox.setWidth(newBoxWidth);

    // 4. (KRİTİK) Kutu büyüdüğü veya küçüldüğü için merkezinin kaymaması lazım.
    // Kutuyu metnin tam ortasına hizalayalım:
    int textCenterX = headerText.getX() + (headerText.getWidth() / 2);
    int newBoxX = textCenterX - (newBoxWidth / 2) - 30;
    headerBox.setX(newBoxX);

    // 5. Ekrandaki değişiklikleri çizdir
    headerBox.invalidate();
    headerText.invalidate();
}

void Screen4View::updateScrollBar()
{
    const int VISIBLE_ITEMS = 10;

    // DİKKAT: TouchGFX Designer'da çizdiğin kaydırma çubuğunun (Rayın)
    // orijinal tam yüksekliğini (Height) buraya girmelisin! (Örn: 350, 400 vb.)
    const int MAX_TRACK_HEIGHT = 360;

    // Hiç arıza yoksa gizle
    if (activeFaultCount == 0)
    {
        scrollBarTrack.setVisible(false);
        scrollBarThumb.setVisible(false);
        scrollBarTrack.invalidate();
		scrollBarThumb.invalidate();
        return;
    }

    scrollBarTrack.setVisible(true);
    scrollBarThumb.setVisible(true);

    int trackY = scrollBarTrack.getY(); // Tasarımdaki başlangıç Y konumu sabit kalır

    if (activeFaultCount <= VISIBLE_ITEMS)
    {
        // =======================================================
        // ARIZA SAYISI 10'DAN AZ İSE:
        // Ray ve Çubuk sadece ekrandaki arızaların kapladığı alan kadar uzar.
        // =======================================================
        int currentHeight = (MAX_TRACK_HEIGHT / VISIBLE_ITEMS) * activeFaultCount;

        scrollBarTrack.setHeight(currentHeight);
        scrollBarThumb.setHeight(currentHeight-2);
        scrollBarThumb.setY(trackY+1); // Çubuk en üstte durur
    }
    else
    {
        // =======================================================
        // ARIZA SAYISI 10'U GEÇTİYSE (KAYDIRMA BAŞLADIYSA):
        // Ray tam boyuna ulaşır, çubuk ise oransal olarak küçülür.
        // =======================================================
        scrollBarTrack.setHeight(MAX_TRACK_HEIGHT);

        // Çubuğun (Thumb) boyunu hesapla
        int thumbHeight = (MAX_TRACK_HEIGHT * VISIBLE_ITEMS) / activeFaultCount;

        // Arıza sayısı devasa olursa çubuk kaybolmasın diye minimum 20 piksel sınırı
        if (thumbHeight < 20) {
            thumbHeight = 20;
        }
        scrollBarThumb.setHeight(thumbHeight);

        // Seçili satıra göre Y ekseninde ne kadar aşağı kayacağını hesapla
        int maxTravel = MAX_TRACK_HEIGHT - thumbHeight;
        int newY = trackY + ((selectedIndex * maxTravel) / (activeFaultCount - 1));

        scrollBarThumb.setY(newY);
    }

    // Ekranı güncelle
    scrollBarTrack.invalidate();
    scrollBarThumb.invalidate();
}
void Screen4View::updateTimeDate(const TLUS::TimeData& time)
{
    // Container senin yazdığın eski parametre yapısını kullandığı için veriyi burada açıyoruz
    timeDayContainer.updateTimeAndDate(time.hour, time.minute, time.day, time.month, time.year);
}
