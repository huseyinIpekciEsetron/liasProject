#include <gui/screen6_screen/Screen6View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <images/SVGDatabase.hpp>
#include <stdio.h>

Screen6View::Screen6View() {}

void Screen6View::setupScreen()
{
    Screen6ViewBase::setupScreen();

    currentVolume = presenter->getVolume();
    currentBrightness = presenter->getBrightness();

    lastLogHead = presenter->getLogHead();

    // Menüleri hazırlayıp gizle
    remove(VolumeLevelContainer); add(VolumeLevelContainer); VolumeLevelContainer.setVisible(false);
    remove(BrighnessLevelContainer); add(BrighnessLevelContainer); BrighnessLevelContainer.setVisible(false);
    remove(menuContainer); add(menuContainer); menuContainer.setVisible(false);
    remove(warningPopupContainer); add(warningPopupContainer);
    remove(buttonContainer); add(buttonContainer);

    refreshLogList();
}

void Screen6View::tearDownScreen()
{
    Screen6ViewBase::tearDownScreen();
}

void Screen6View::refreshLogList()
{
    activeLogCount = presenter->getLogCount();

    if (activeLogCount > 0) {
        scrollList.setVisible(true);
        scrollList.setNumberOfItems(activeLogCount);

        // Liste ilk açıldığında en üstteki (en yeni) eleman seçili olsun
        if (selectedIndex >= activeLogCount) selectedIndex = activeLogCount - 1;

    } else {
        // LOG YOKSA LİSTEYİ VE ÇUBUĞU GİZLE
        scrollList.setVisible(false);
        scrollBarTrack.setVisible(false);
        scrollBarThumb.setVisible(false);
    }

    for (int i = 0; i < activeLogCount; i++) {
        scrollList.itemChanged(i);
    }

    scrollList.invalidate();
    updateContextualIcons();
    updateScrollBar();
}

// TouchGFX'in listeye veri basarken çağırdığı asıl fonksiyon
void Screen6View::scrollListUpdateItem(LogItemContainer& item, int16_t itemIndex)
{
    if (itemIndex < activeLogCount)
    {
        Model::LogEntry logData = presenter->getLog(itemIndex);

        item.setupLog(logData.hour, logData.minute, logData.second, logData.textId, logData.param, logData.type);
        item.setHighlighted(itemIndex == selectedIndex);
    }
}

void Screen6View::updateScrollBar()
{
    const int VISIBLE_ITEMS = 10; // Ekrana sığan log satırı sayısı
    const int MAX_TRACK_HEIGHT = 360; // Tasarımdaki Track objesinin Height değeri

    const int ITEM_HEIGHT = 36;

    if (activeLogCount == 0) {
        scrollBarTrack.setVisible(false);
        scrollBarThumb.setVisible(false);
        return;
    }

    scrollBarTrack.setVisible(true);
    scrollBarThumb.setVisible(true);
    int trackY = scrollBarTrack.getY();

    if (activeLogCount <= VISIBLE_ITEMS) {
        int currentHeight = activeLogCount * ITEM_HEIGHT;
        scrollBarTrack.setHeight(currentHeight);
        scrollBarThumb.setHeight(currentHeight - 2);
        scrollBarThumb.setY(trackY + 1);
    } else {
        scrollBarTrack.setHeight(MAX_TRACK_HEIGHT);
        int thumbHeight = (MAX_TRACK_HEIGHT * VISIBLE_ITEMS) / activeLogCount;
        if (thumbHeight < 20) thumbHeight = 20;

        scrollBarThumb.setHeight(thumbHeight);

        int maxTravel = MAX_TRACK_HEIGHT - thumbHeight;
        int newY = trackY + ((selectedIndex * maxTravel) / (activeLogCount - 1));
        scrollBarThumb.setY(newY);
    }

    scrollBarTrack.invalidate();
    scrollBarThumb.invalidate();
}

void Screen6View::updateContextualIcons()
{
    if (currentState == STATE_MENU_OPEN) {
        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_ENTER_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BACK_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
    }
    else {
        // Normal gezinme sırasında ikonlar SES/PARLAKLIK olur
        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_SOUND_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BRIGHT_ID, 2.5f, 2.5f);
        if(activeLogCount > 0)
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

void Screen6View::handleTickEvent()
{
	int currentHead = presenter->getLogHead();
	if (currentHead != lastLogHead) {
		lastLogHead = currentHead;
		refreshLogList(); // Yeni log geldiğinde anında ekranı yenile!
	}
    if (menuTimeout > 0) {
        menuTimeout--;
        if (menuTimeout == 0) {
            closeMenus();
        }
    }
}

void Screen6View::updateTimeDate(const TLUS::TimeData& time)
{
    timeDayContainer.updateTimeAndDate(time.hour, time.minute, time.day, time.month, time.year);
}

void Screen6View::handleVolumeOrEnter()
{
    if (currentState == STATE_MENU_OPEN) {
        int selected = menuContainer.getSelectedItem();
        if (selected == 0)      presenter->gotoAyarlarScreen();
        else if (selected == 1) presenter->gotoArizaScreen();
        else if (selected == 2) presenter->gotoTestScreen();
        else if (selected == 3) presenter->gotoLoglarScreen();
        else if (selected == 4) presenter->gotoSoftResetScreen();
        else if (selected == 5) presenter->gotoZeroizeScreen();
    } else {
        if (currentState != STATE_VOLUME_OPEN) openVolumeMenu();
        else closeMenus();
    }
}

void Screen6View::handleBrightnessOrBack()
{
    if (currentState == STATE_MENU_OPEN) {
        closeMenus();
    } else {
        if (currentState != STATE_BRIGHTNESS_OPEN) openBrightnessMenu();
        else closeMenus();
    }
}

void Screen6View::menuButtonPressed()
{
    if (currentState == STATE_MENU_OPEN) {
        closeMenus();
    } else {
        closeMenus();
        currentState = STATE_MENU_OPEN;
        menuContainer.resetSelection();
        menuContainer.setVisible(true);
        menuContainer.invalidate();
        menuTimeout = 500;
        updateContextualIcons();
    }
}

void Screen6View::openVolumeMenu()
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

void Screen6View::openBrightnessMenu()
{
    closeMenus();
    currentState = STATE_BRIGHTNESS_OPEN;
    menuTimeout = 150;
    BrighnessLevelContainer.setVisible(true);
    BrighnessLevelContainer.setLevel(currentBrightness, SVG_BRIGHT_ID);
    BrighnessLevelContainer.invalidate();
    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
    buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);
    buttonContainer.invalidate();
}

void Screen6View::buttonUpPressed()
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
    else if (currentState == STATE_NAVIGATING && activeLogCount > 0) {
        int oldIndex = selectedIndex;

        if (selectedIndex > 0) selectedIndex--;
        else selectedIndex = activeLogCount - 1;

        scrollList.itemChanged(oldIndex);
        scrollList.itemChanged(selectedIndex);
        scrollList.animateToItem(selectedIndex, 10);

        updateContextualIcons();
        updateScrollBar();
    }
    else if(currentState == STATE_NAVIGATING && activeLogCount == 0) {
        presenter->gotoAyarlarScreen();
    }
}

void Screen6View::buttonDownPressed()
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
    else if (currentState == STATE_NAVIGATING && activeLogCount > 0) {
        int oldIndex = selectedIndex;

        if (selectedIndex < activeLogCount - 1) selectedIndex++;
        else selectedIndex = 0;

        scrollList.itemChanged(oldIndex);
        scrollList.itemChanged(selectedIndex);
        scrollList.animateToItem(selectedIndex, 10);

        updateContextualIcons();
        updateScrollBar();
    }
    else if(currentState == STATE_NAVIGATING && activeLogCount == 0) {
        presenter->gotoTestScreen();
    }
}

void Screen6View::closeMenus()
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

        updateContextualIcons();
    }
}

void Screen6View::enterPressed()
{
    if (currentState == STATE_MENU_OPEN) {
        int selected = menuContainer.getSelectedItem();
        if (selected == 0)      presenter->gotoAyarlarScreen();
        else if (selected == 1) presenter->gotoArizaScreen();
        else if (selected == 2) presenter->gotoTestScreen();
        else if (selected == 3) presenter->gotoLoglarScreen();
    }
}

void Screen6View::backPressed()
{
    if (currentState == STATE_MENU_OPEN) {
        closeMenus();
    }
}

void Screen6View::updateButtonVisuals(uint32_t state)
{
    buttonContainer.updateButtonVisuals(state, 0);
}

void Screen6View::showWarningPopup(Model::WarningType warning, int tubeIndex)
{
    warningPopupContainer.showWarningMessage(warning, tubeIndex);
}
