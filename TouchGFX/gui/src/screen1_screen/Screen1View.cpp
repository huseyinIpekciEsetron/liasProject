#include <gui/screen1_screen/Screen1View.hpp>
#include "hc165_driver.h"
#include <images/SVGDatabase.hpp>
#include <stm32h7xx_hal.h> // Delta Time İçin Eklendi

static const int FADE_DURATION_MS = 60000;

static const float CENTER_X = 400.0f;
static const float CENTER_Y = 240.0f;
static const float RING_RADIUS = 175.0f;

static const float FIXED_INNER_RADIUS = 45.0f;
static const float FIXED_OUTER_RADIUS = 173.0f;
static const float FIXED_ARC_DEGREE   = 15.0f;

struct ThreatVisualCache {
    int threatNumber;
    int angle;
    int threatClass;
    int band;
    int priority;
};

static ThreatVisualCache slotCache[22];
static bool cacheInit = false;

static touchgfx::TypedTextId getThreatClassTextId(int threatClass)
{
    switch (threatClass)
    {
    	case 0: return T_THREAT_SRCH;
        case 1: return T_THREAT_LRF;
        case 2: return T_THREAT_LD;
        case 4: return T_THREAT_LBR;
        default: return T_THREAT_NONE;
    }
}

static colortype getBandColor(int band)
{
    switch (band)
    {
        case 1: return Color::getColorFromRGB(0, 210, 20);
        case 2: return Color::getColorFromRGB(245, 130, 0);
        case 3: return Color::getColorFromRGB(140, 40, 240);
        default: return Color::getColorFromRGB(200, 210, 220);
    }
}

Screen1View::Screen1View() : fadeAnimationEndedCallback(this, &Screen1View::fadeAnimationEndedHandler)
{
	if (!cacheInit) {
		for (int i = 0; i < MAX_TARGETS; i++) {
			slotCache[i].threatNumber = -1;
		}
		cacheInit = true;
	}

	for (int i = 0; i < MAX_TARGETS; i++) {
		currentAngles[i] = slotCache[i].angle;
		currentThreatNumbers[i] = slotCache[i].threatNumber;
	}
}

void Screen1View::setupScreen()
{

    Screen1ViewBase::setupScreen();

    remove(tickerContainer);
    add(tickerContainer);

    currentVolume = presenter->getVolume();
	currentBrightness = presenter->getBrightness();

	// Ticker Başlangıç Ayarları ve Animasyon Yönlendirmesi
	tickerContainer.setFadeCallback(fadeAnimationEndedCallback);
	tickerContainer.setVisible(false);
	tickerContainer.setBoxVisible(false); // Başlangıçta arka planları da gizle
	tickerContainer.setAlpha(0);          // Yazıyı da gizle

	buildFaultList();
	if (activeFaultCount > 0) {
		currentTickerIndex = 0;
		tickerContainer.setVisible(true);
		tickerContainer.setBoxVisible(true);
		tickerContainer.updateFaultCount(activeFaultCount);
		showNextFaultInTicker();
	}

	for (int i = 0; i < MAX_TARGETS; i++)
	{
	    arcPainters[i].setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
	    threatArcs[i].setPosition(0, 0, 800, 480);
	    threatArcs[i].setCenter(400, 240);
	    threatArcs[i].setPainter(arcPainters[i]);
	    threatArcs[i].setAlpha(0);
	    add(threatArcs[i]);

	    threatTextBoxes[i].setAlpha(0);
	    threatTextBoxes[i].setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
	    threatTextBoxes[i].setBorderSize(2);
	    add(threatTextBoxes[i]);

	    threatTexts[i].setWildcard1(threatTextBuffers[i]);
	    threatTexts[i].setTypedText(touchgfx::TypedText(T_RADARANGLE));
	    threatTexts[i].setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	    threatTexts[i].setAlpha(0);
	    add(threatTexts[i]);

	    threatClassTexts[i].setTypedText(touchgfx::TypedText(T_THREAT_SRCH));
	    threatClassTexts[i].setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	    threatClassTexts[i].setAlpha(0);
	    add(threatClassTexts[i]);

		threatPriorityTexts[i].setWildcard1(threatPriorityBuffers[i]);
		threatPriorityTexts[i].setTypedText(touchgfx::TypedText(T_PRIONUM));
		threatPriorityTexts[i].setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		threatPriorityTexts[i].setAlpha(0);
		add(threatPriorityTexts[i]);

		if (slotCache[i].threatNumber != -1)
		{
			int targetAngle = slotCache[i].angle;
			int band = slotCache[i].band;
			int threatClass = slotCache[i].threatClass;
			int priority = slotCache[i].priority;

			int remaining = presenter->getFadeTicksRemaining(i);
			uint8_t alpha = (remaining < 0) ? 245 : (uint8_t)(245 * remaining / FADE_DURATION_MS);

			threatArcs[i].setRadius((FIXED_INNER_RADIUS + FIXED_OUTER_RADIUS) / 2.0f);
			threatArcs[i].setLineWidth(FIXED_OUTER_RADIUS - FIXED_INNER_RADIUS);
			threatArcs[i].setArc(targetAngle - FIXED_ARC_DEGREE / 2.0f, targetAngle + FIXED_ARC_DEGREE / 2.0f);
			arcPainters[i].setColor(getBandColor(band));
			threatArcs[i].setAlpha(alpha);
			threatArcs[i].setVisible(true);

			const float PI = 3.1415926535f;
			float rad = (targetAngle - 90.0f) * PI / 180.0f;

            Unicode::snprintf(threatTextBuffers[i], 8, "%d\x00B0", targetAngle);
            threatTexts[i].resizeToCurrentText();
            int tW = threatTexts[i].getWidth();
            int tH = threatTexts[i].getHeight();
            int textX = (int)(CENTER_X + (135.0f * cosf(rad))) - (tW / 2);
            int textY = (int)(CENTER_Y + (135.0f * sinf(rad))) - (tH / 2);
            threatTexts[i].setPosition(textX, textY, tW, tH);
            threatTexts[i].setAlpha(alpha);
            threatTexts[i].setVisible(true);

			int boxW = 90; int boxH = 28;
            float fixedGap = 5.0f;
			float dynamicOffset = (boxW / 2.0f) * fabsf(cosf(rad)) + (boxH / 2.0f) * fabsf(sinf(rad));
			float boxR = RING_RADIUS + fixedGap + dynamicOffset;

			int boxX = (int)(CENTER_X + (boxR * cosf(rad))) - (boxW / 2);
			int boxY = (int)(CENTER_Y + (boxR * sinf(rad))) - (boxH / 2);

			threatTextBoxes[i].setPosition(boxX, boxY, boxW, boxH);
			threatTextBoxes[i].setAlpha(alpha);
			threatTextBoxes[i].setBorderSize(2); // Çerçeve kalınlığı
			threatTextBoxes[i].setBorderColor(touchgfx::Color::getColorFromRGB(0, 0, 0)); // Siyah çerçeve rengi
			threatTextBoxes[i].setVisible(true);

			threatClassTexts[i].setTypedText(touchgfx::TypedText(getThreatClassTextId(threatClass)));
            threatClassTexts[i].resizeToCurrentText();
            int classW = threatClassTexts[i].getWidth();

            Unicode::snprintf(threatPriorityBuffers[i], 8, "- %d", priority);
            threatPriorityTexts[i].resizeToCurrentText();
            int prioW = threatPriorityTexts[i].getWidth();

            int totalTextW = classW + prioW;
            int startX = boxX + (boxW - totalTextW) / 2;
            int textCenterY = boxY + (boxH - threatClassTexts[i].getHeight()) / 2;
            int prioCenterY = boxY + (boxH - threatPriorityTexts[i].getHeight()) / 2;

			threatClassTexts[i].setPosition(startX, textCenterY, classW, threatClassTexts[i].getHeight());
			threatClassTexts[i].setAlpha(alpha);
			threatClassTexts[i].setVisible(true);

			threatPriorityTexts[i].setPosition(startX + classW, prioCenterY, prioW, threatPriorityTexts[i].getHeight());
			threatPriorityTexts[i].setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
			threatPriorityTexts[i].setAlpha(alpha);
			threatPriorityTexts[i].setVisible(true);
		}
	}

	remove(VolumeLevelContainer); add(VolumeLevelContainer);
	remove(BrighnessLevelContainer); add(BrighnessLevelContainer);
	remove(menuContainer); add(menuContainer);
	remove(warningPopupContainer); add(warningPopupContainer);
	remove(buttonContainer); add(buttonContainer);

}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleVolumeOrEnter()
{
    if (currentMenu == MENU_MAIN_LIST) {
        int selected = menuContainer.getSelectedItem();
        if (selected == 0)      presenter->gotoAyarlarScreen();
        else if (selected == 1) presenter->gotoArizaScreen();
        else if (selected == 2) presenter->gotoTestScreen();
        else if (selected == 3) presenter->gotoLoglarScreen();
        else if (selected == 4) presenter->gotoSoftResetScreen(); // Soft Reset menüsü
		else if (selected == 5) presenter->gotoZeroizeScreen();   // Acil Silme menüsü
    } else {
        if (currentMenu != MENU_VOLUME) openVolumeMenu();
        else closeMenus();
    }
}

void Screen1View::handleBrightnessOrBack()
{
    if (currentMenu == MENU_MAIN_LIST) {
        closeMenus();
    } else {
        if (currentMenu != MENU_BRIGHTNESS) openBrightnessMenu();
        else closeMenus();
    }
}

void Screen1View::menuButtonPressed()
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

void Screen1View::openVolumeMenu()
{
	if (currentMenu == MENU_MAIN_LIST) return;
    currentMenu = MENU_VOLUME;
    menuTimeout = 75;
    BrighnessLevelContainer.setVisible(false);
    VolumeLevelContainer.setVisible(true);
    VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);
    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
	buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
    BrighnessLevelContainer.invalidate();
    VolumeLevelContainer.invalidate();
    buttonContainer.invalidate();
}

void Screen1View::openBrightnessMenu()
{
	if (currentMenu == MENU_MAIN_LIST) return;
    currentMenu = MENU_BRIGHTNESS;
    menuTimeout = 75;
    VolumeLevelContainer.setVisible(false);
    BrighnessLevelContainer.setVisible(true);
    BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
	buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
    VolumeLevelContainer.invalidate();
    BrighnessLevelContainer.invalidate();
    buttonContainer.invalidate();
}

void Screen1View::buttonUpPressed()
{
    if (currentMenu == MENU_VOLUME) {
        if (currentVolume < 5) currentVolume++;
        VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);
        menuTimeout = 75;
        presenter->saveVolume(currentVolume);
    }
    else if (currentMenu == MENU_BRIGHTNESS) {
        if (currentBrightness < 5) currentBrightness++;
        BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
        menuTimeout = 75;
        presenter->saveBrightness(currentBrightness);
    }
    else if (currentMenu == MENU_MAIN_LIST) {
    	menuTimeout = 250;
		menuContainer.moveUp();
	}
    else if(currentMenu == MENU_NONE) {
    	presenter->gotoAyarlarScreen();
    }
}

void Screen1View::buttonDownPressed()
{
    if (currentMenu == MENU_VOLUME) {
        if (currentVolume > 0) currentVolume--;
        VolumeLevelContainer.setLevel(currentVolume, SVG_SOUND_ID);
        menuTimeout = 75;
        presenter->saveVolume(currentVolume);
    }
    else if (currentMenu == MENU_BRIGHTNESS) {
        if (currentBrightness > 0) currentBrightness--;
        BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
        menuTimeout = 75;
        presenter->saveBrightness(currentBrightness);
    }
    else if (currentMenu == MENU_MAIN_LIST) {
    	menuTimeout = 250;
		menuContainer.moveDown();
	}
    else if(currentMenu == MENU_NONE) {
		presenter->gotoTestScreen();
	}
}

void Screen1View::updateButtonVisuals(uint32_t state)
{
	buttonContainer.updateButtonVisuals(state, BTN_IDLE);
}

void Screen1View::closeMenus()
{
    if (currentMenu != MENU_NONE)
    {
        currentMenu = MENU_NONE;
        menuTimeout = 0;

        VolumeLevelContainer.setVisible(false);
        BrighnessLevelContainer.setVisible(false);
        menuContainer.setVisible(false);

        buttonContainer.setButtonIcon(BTN_MENU, SVG_MENU_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_HOME, SVG_HOME_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_SOUND_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BRIGHT_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_UP, SVG_AYARLAR_ID, 2.5f, 2.5f);
		buttonContainer.setButtonIcon(BTN_DOWN, SVG_TEST_ID, 2.5f, 2.5f);

        VolumeLevelContainer.invalidate();
        BrighnessLevelContainer.invalidate();
        menuContainer.invalidate();
        buttonContainer.invalidate();
    }
}

void Screen1View::homeButtonPressed()
{
    if (currentMenu != MENU_NONE) closeMenus();
}

void Screen1View::handleTickEvent()
{
	for (int i = 0; i < MAX_TARGETS; i++)
	{
		int remaining = presenter->getFadeTicksRemaining(i);

		if (remaining < 0) continue;

		if (remaining == 0)
		{
			freeThreatSlot(i);
			presenter->setFadeTicksRemaining(i, -1);
			continue;
		}

		uint8_t newAlpha = (uint8_t)(245 * remaining / FADE_DURATION_MS);

		threatArcs[i].setAlpha(newAlpha);
		threatTextBoxes[i].setAlpha(newAlpha);
		threatTexts[i].setAlpha(newAlpha);
		threatPriorityTexts[i].setAlpha(newAlpha);
		threatClassTexts[i].setAlpha(newAlpha);

		threatArcs[i].invalidate();
		threatTextBoxes[i].invalidate();
		threatTexts[i].invalidate();
		threatPriorityTexts[i].invalidate();
		threatClassTexts[i].invalidate();
	}

	if (menuTimeout > 0) {
		menuTimeout--;
		if (menuTimeout == 0) closeMenus();
	}

	// ============================================
    // ARIZA ALT YAZI (FADE LOOP MANTIĞI)
    // ============================================
	faultCheckTimer++;
	if (faultCheckTimer > 60)
	{
		faultCheckTimer = 0;
		int oldCount = activeFaultCount;
		buildFaultList();

		if (activeFaultCount == 0) {
			// ARIZA YOKSA HER ŞEYİ KAPAT
			tickerContainer.setAlpha(0);
			tickerContainer.setBoxVisible(false);
			tickerContainer.setVisible(false);
			tickerContainer.invalidate();
		}
		else {
			// ARIZA VARSA KUTUYU AÇ VE SAYACI GÜNCELLE
			tickerContainer.setVisible(true);
			tickerContainer.setBoxVisible(true);
			tickerContainer.updateFaultCount(activeFaultCount);

			if (activeFaultCount == 1) {
				// SADECE 1 ARIZA VARSA SABİT TUT (Animasyonu iptal et)
				currentTickerIndex = 0;
				updateTickerText();
				tickerContainer.setAlpha(255);
				isFadingOut = false;
			}
			else if (oldCount <= 1) {
				// YENİDEN ÇOKLU ARIZAYA GEÇTİYSE LİSTEYİ BAŞTAN SAR
				currentTickerIndex = 0;
				showNextFaultInTicker();
			}
		}
	}

	// ÇOKLU ARIZA VARSA: 3 saniyelik (90 tick) Fade OUT sayacını işlet
	if (activeFaultCount > 1 && tickerContainer.getAlpha() == 255 && !isFadingOut) {
		tickerDisplayTimer++;
		if (tickerDisplayTimer > 90) {
			tickerDisplayTimer = 0;
			isFadingOut = true;
			tickerContainer.startFadeAnimation(0, 15); // Yarım saniyede sol
		}
	}
}

void Screen1View::updateTickerText()
{
    tickerContainer.updateFaultText(activeFaults[currentTickerIndex].textId, activeFaults[currentTickerIndex].tubeIndex);
}

void Screen1View::showNextFaultInTicker()
{
    if (activeFaultCount <= 1) return;

    updateTickerText();
    isFadingOut = false;
    tickerDisplayTimer = 0;

    tickerContainer.startFadeAnimation(255, 15); // 15 tick (yarım saniye) süresinde aydınlanır
}

void Screen1View::fadeAnimationEndedHandler(const touchgfx::FadeAnimator<touchgfx::TextAreaWithOneWildcard>& src)
{
    if (isFadingOut && tickerContainer.getAlpha() == 0) {
        if (activeFaultCount > 1) {
            currentTickerIndex++;
            if (currentTickerIndex >= activeFaultCount) currentTickerIndex = 0;
            showNextFaultInTicker();
        }
    }
}

void Screen1View::freeThreatSlot(int slotIndex)
{
    currentThreatNumbers[slotIndex] = -1;
    currentAngles[slotIndex] = -1;
    slotCache[slotIndex].threatNumber = -1;
    presenter->setFadeTicksRemaining(slotIndex, -1);

    threatArcs[slotIndex].setVisible(false);
    threatTextBoxes[slotIndex].setVisible(false);
    threatTexts[slotIndex].setVisible(false);
    threatPriorityTexts[slotIndex].setVisible(false);
    threatClassTexts[slotIndex].setVisible(false);

    threatClassTexts[slotIndex].invalidate();
    threatArcs[slotIndex].invalidate();
    threatTextBoxes[slotIndex].invalidate();
    threatTexts[slotIndex].invalidate();
    threatPriorityTexts[slotIndex].invalidate();
}

void Screen1View::updateTargets(const TLUS::ThreatMessagePayload& payload)
{
	// Paket içindeki hedef sayısı kadar dön
	for (int t = 0; t < payload.count; t++)
	{
		int threatNumber = payload.threats[t].threatNumber;
		int targetAngle  = (int)payload.threats[t].angle;
		int threatClass  = payload.threats[t].threatClass;
		int band         = payload.threats[t].band;
		int priority     = payload.threats[t].priority;
		int ageOut       = payload.threats[t].ageOut;

		if (targetAngle < 0) targetAngle = 0;
		if (targetAngle > 360) targetAngle = 360;
		if (priority < 1) priority = 1;
		if (priority > 31) priority = 31;

		int slotIndex = -1;
		for (int i = 0; i < MAX_TARGETS; i++) {
			if (currentThreatNumbers[i] == threatNumber) {
				slotIndex = i;
				break;
			}
		}

		if (slotIndex == -1) {
			if (ageOut == 1) continue;
			for (int i = 0; i < MAX_TARGETS; i++) {
				if (currentThreatNumbers[i] == -1) {
					slotIndex = i;
					break;
				}
			}
		}
		if (slotIndex == -1) continue;

		// 1. FADE (AgeOut) KONTROLÜ
		if (ageOut == 1) {
			if (presenter->getFadeTicksRemaining(slotIndex) < 0) {
				presenter->setFadeTicksRemaining(slotIndex, FADE_DURATION_MS);
			}
		} else {
			presenter->setFadeTicksRemaining(slotIndex, -1);
		}

		// 2. GÖRSELLİK (SADECE DEĞİŞİKLİK VARSA GÜNCELLE - İŞLEMCİYİ BOĞMAMAK İÇİN)
		bool angleChanged = (currentAngles[slotIndex] != targetAngle);
		bool dataChanged = (slotCache[slotIndex].threatClass != threatClass ||
							slotCache[slotIndex].priority != priority ||
							slotCache[slotIndex].band != band);
		bool isNew = (currentThreatNumbers[slotIndex] != threatNumber);

		// EĞER HERHANGİ BİR ŞEY DEĞİŞTİYSE ÇİZİM YAP (Aksi halde hiçbir şey yapma, titremesin!)
		if (angleChanged || dataChanged || isNew)
		{
			// Eski yerdeki çizimleri ekrandan temizle
			if (!isNew) {
				threatArcs[slotIndex].invalidate();
				threatTextBoxes[slotIndex].invalidate();
				threatTexts[slotIndex].invalidate();
				threatPriorityTexts[slotIndex].invalidate();
				threatClassTexts[slotIndex].invalidate();
			}

			// Yeni verileri kaydet
			currentThreatNumbers[slotIndex] = threatNumber;
			currentAngles[slotIndex] = targetAngle;
			slotCache[slotIndex].threatNumber = threatNumber;
			slotCache[slotIndex].angle = targetAngle;
			slotCache[slotIndex].threatClass = threatClass;
			slotCache[slotIndex].band = band;
			slotCache[slotIndex].priority = priority;

			int remaining = presenter->getFadeTicksRemaining(slotIndex);
			uint8_t alpha = (remaining < 0) ? 255 : (uint8_t)(255 * remaining / FADE_DURATION_MS);

			threatArcs[slotIndex].setRadius((FIXED_INNER_RADIUS + FIXED_OUTER_RADIUS) / 2.0f);
			threatArcs[slotIndex].setLineWidth(FIXED_OUTER_RADIUS - FIXED_INNER_RADIUS);
			threatArcs[slotIndex].setArc(targetAngle - FIXED_ARC_DEGREE / 2.0f, targetAngle + FIXED_ARC_DEGREE / 2.0f);
			arcPainters[slotIndex].setColor(getBandColor(band));
			threatArcs[slotIndex].setAlpha(alpha);
			threatArcs[slotIndex].setVisible(true);

			const float PI = 3.1415926535f;
			float rad = (targetAngle - 90.0f) * PI / 180.0f;

			Unicode::snprintf(threatTextBuffers[slotIndex], 8, "%d\x00B0", targetAngle);
			threatTexts[slotIndex].resizeToCurrentText();
			int tW = threatTexts[slotIndex].getWidth();
			int tH = threatTexts[slotIndex].getHeight();
			int textX = (int)(CENTER_X + (135.0f * cosf(rad))) - (tW / 2);
			int textY = (int)(CENTER_Y + (135.0f * sinf(rad))) - (tH / 2);
			threatTexts[slotIndex].setPosition(textX, textY, tW, tH);
			threatTexts[slotIndex].setAlpha(alpha);
			threatTexts[slotIndex].setVisible(true);

			int boxW = 90; int boxH = 24;
			float fixedGap = 5.0f;
			float dynamicOffset = (boxW / 2.0f) * fabsf(cosf(rad)) + (boxH / 2.0f) * fabsf(sinf(rad));
			float boxR = RING_RADIUS + fixedGap + dynamicOffset;

			int boxX = (int)(CENTER_X + (boxR * cosf(rad))) - (boxW / 2) ;
			int boxY = (int)(CENTER_Y + (boxR * sinf(rad))) - (boxH / 2);

			threatTextBoxes[slotIndex].setPosition(boxX, boxY, boxW, boxH);
			threatTextBoxes[slotIndex].setAlpha(alpha);
			threatTextBoxes[slotIndex].setVisible(true);

			threatClassTexts[slotIndex].setTypedText(touchgfx::TypedText(getThreatClassTextId(threatClass)));
			threatClassTexts[slotIndex].resizeToCurrentText();
			int classW = threatClassTexts[slotIndex].getWidth();

			Unicode::snprintf(threatPriorityBuffers[slotIndex], 8, "- %d", priority);
			threatPriorityTexts[slotIndex].resizeToCurrentText();
			int prioW = threatPriorityTexts[slotIndex].getWidth();

			int totalTextW = classW + prioW;
			int startX = boxX + (boxW - totalTextW) / 2;
			int textCenterY = boxY + (boxH - threatClassTexts[slotIndex].getHeight()) / 2;
			int prioCenterY = boxY + (boxH - threatPriorityTexts[slotIndex].getHeight()) / 2;

			threatClassTexts[slotIndex].setPosition(startX, textCenterY, classW, threatClassTexts[slotIndex].getHeight());
			threatClassTexts[slotIndex].setAlpha(alpha);
			threatClassTexts[slotIndex].setVisible(true);

			threatPriorityTexts[slotIndex].setPosition(startX + classW, prioCenterY, prioW, threatPriorityTexts[slotIndex].getHeight());
			threatPriorityTexts[slotIndex].setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
			threatPriorityTexts[slotIndex].setAlpha(alpha);
			threatPriorityTexts[slotIndex].setVisible(true);

			// Yeni konumları ekrana çiz (SADECE DEĞİŞİKLİK OLDUĞUNDA!)
			threatArcs[slotIndex].invalidate();
			threatTextBoxes[slotIndex].invalidate();
			threatTexts[slotIndex].invalidate();
			threatPriorityTexts[slotIndex].invalidate();
			threatClassTexts[slotIndex].invalidate();
		}
		else if (presenter->getFadeTicksRemaining(slotIndex) == -1 && threatArcs[slotIndex].getAlpha() != 255)
		{
			// EĞER AÇI DEĞİŞMEDİYSE AMA HEDEF FADE-OUT (SİLİNME) ESNASINDA TEKRAR AGEOUT=0 OLARAK GELDİYSE:
			// Sadece Alpha değerini tam parlaklığa çek ve güncelleyip bırak.
			threatArcs[slotIndex].setAlpha(255);
			threatTextBoxes[slotIndex].setAlpha(255);
			threatTexts[slotIndex].setAlpha(255);
			threatPriorityTexts[slotIndex].setAlpha(255);
			threatClassTexts[slotIndex].setAlpha(255);

			threatArcs[slotIndex].invalidate();
			threatTextBoxes[slotIndex].invalidate();
			threatTexts[slotIndex].invalidate();
			threatPriorityTexts[slotIndex].invalidate();
			threatClassTexts[slotIndex].invalidate();
		}
	}
}

void Screen1View::showWarningPopup(Model::WarningType warning, int tubeIndex)
{
    warningPopupContainer.showWarningMessage(warning, tubeIndex);
}

void Screen1View::buildFaultList()
{
    activeFaultCount = 0;

    // Kritik Hatalar
    if (presenter->getCommLostFlag()) activeFaults[activeFaultCount++] = { T_WARNCOMMERR, -1 };
    if (presenter->getHwErrorFlag()) activeFaults[activeFaultCount++] = { T_WARNVOLTAGE, -1 };

    // SADECE AKTİF SİSTEM VE SENSÖR HATALARINI OKUYORUZ
    TLUS::SystemStatusPayload status = presenter->getActiveSystemStatus();

    if (status.processorFaults.ramTesti) activeFaults[activeFaultCount++] = { T_FLT_PROC_RAM, -1 };
    if (status.processorFaults.kaliciBellekTesti) activeFaults[activeFaultCount++] = { T_FLT_PROC_NVRAM, -1 };
    if (status.processorFaults.bellekDosyasiTesti) activeFaults[activeFaultCount++] = { T_FLT_PROC_MEMFILE, -1 };
    if (status.processorFaults.nvsramTesti) activeFaults[activeFaultCount++] = { T_FLT_PROC_NVSRAM, -1 };
    if (status.processorFaults.bellekDoluluk) activeFaults[activeFaultCount++] = { T_FLT_PROC_MEMFULL, -1 };
    if (status.processorFaults.seriKanal1) activeFaults[activeFaultCount++] = { T_FLT_PROC_SER1, -1 };
    if (status.processorFaults.seriKanal2) activeFaults[activeFaultCount++] = { T_FLT_PROC_SER2, -1 };
    if (status.processorFaults.seriKanal3) activeFaults[activeFaultCount++] = { T_FLT_PROC_SER3, -1 };
    if (status.processorFaults.seriKanal4) activeFaults[activeFaultCount++] = { T_FLT_PROC_SER4, -1 };
    if (status.processorFaults.arayuzKarti) activeFaults[activeFaultCount++] = { T_FLT_PROC_IFACE, -1 };
    if (status.processorFaults.anaBesleme) activeFaults[activeFaultCount++] = { T_FLT_PROC_PWR, -1 };
    if (status.processorFaults.islemciDurumuKapanma) activeFaults[activeFaultCount++] = { T_FLT_PROC_SHUTDOWN, -1 };
    if (status.processorFaults.gucKartiSeriKanal) activeFaults[activeFaultCount++] = { T_FLT_PROC_PWR_SER, -1 };
    if (status.processorFaults.sicaklikEsikAsimi) activeFaults[activeFaultCount++] = { T_FLT_PROC_TEMP, -1 };

    for (int i = 0; i < 4; i++) {
        int sNo = i + 1;
        if (status.sensorFaults[i].bant_I_II_Karti) activeFaults[activeFaultCount++] = { T_FLT_SENS_B12, sNo };
        if (status.sensorFaults[i].bant_III_Sensor0) activeFaults[activeFaultCount++] = { T_FLT_SENS_B3_0, sNo };
        if (status.sensorFaults[i].bant_III_Karti_Sensor1) activeFaults[activeFaultCount++] = { T_FLT_SENS_B3_1, sNo };
        if (status.sensorFaults[i].bant_III_Karti_Sensor2) activeFaults[activeFaultCount++] = { T_FLT_SENS_B3_2, sNo };
        if (status.sensorFaults[i].sensor_Birimi_Kontrol_Karti) activeFaults[activeFaultCount++] = { T_FLT_SENS_CTRL, sNo };
        if (status.sensorFaults[i].kontrollu_Kapanma) activeFaults[activeFaultCount++] = { T_FLT_SENS_SHUTDOWN, sNo };
        if (status.sensorFaults[i].guc_Karti_Seri_Kanal) activeFaults[activeFaultCount++] = { T_FLT_SENS_PWR_SER, sNo };
        if (status.sensorFaults[i].basinc_Durumu) activeFaults[activeFaultCount++] = { T_FLT_SENS_PRESS, sNo };
        if (status.sensorFaults[i].volt_3_7V) activeFaults[activeFaultCount++] = { T_FLT_SENS_V3_7, sNo };
        if (status.sensorFaults[i].volt_7_4V) activeFaults[activeFaultCount++] = { T_FLT_SENS_V7_4, sNo };
        if (status.sensorFaults[i].volt_16V) activeFaults[activeFaultCount++] = { T_FLT_SENS_V16, sNo };
        if (status.sensorFaults[i].volt_80V) activeFaults[activeFaultCount++] = { T_FLT_SENS_V80, sNo };
        if (status.sensorFaults[i].volt_neg7_4V) activeFaults[activeFaultCount++] = { T_FLT_SENS_VN7_4, sNo };
        if (status.sensorFaults[i].volt_neg3_7V) activeFaults[activeFaultCount++] = { T_FLT_SENS_VN3_7, sNo };
        if (status.sensorFaults[i].ana_Besleme) activeFaults[activeFaultCount++] = { T_FLT_SENS_PWR, sNo };
        if (status.sensorFaults[i].sicaklik_Durumu) activeFaults[activeFaultCount++] = { T_FLT_SENS_TEMP_LIM, sNo };
        if (status.sensorFaults[i].sicaklik_Sensoru) activeFaults[activeFaultCount++] = { T_FLT_SENS_TEMP_SNS, sNo };
    }

    // Tüp (Mühimmat) Arızaları (Bunlar zaten sadece Aktif olarak yaşar)
    for (int i = 0; i < 16; i++) {
        if (presenter->getFaultStatus(i)) {
            touchgfx::TypedTextId id = presenter->getFaultIsFrag(i) ? T_FAULTFRAG : T_FAULTSMOKE;
            activeFaults[activeFaultCount++] = { id, i + 1 };
        }
    }
}

void Screen1View::updateTimeDate(const TLUS::TimeData& time)
{
    // Container senin yazdığın eski parametre yapısını kullandığı için veriyi burada açıyoruz
    timeDayContainer.updateTimeAndDate(time.hour, time.minute, time.day, time.month, time.year);
}
