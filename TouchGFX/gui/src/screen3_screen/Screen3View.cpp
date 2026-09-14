#include <gui/screen3_screen/Screen3View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <images/SVGDatabase.hpp>
#include <stdio.h>

Screen3View::Screen3View()
{
}

void Screen3View::setupScreen()
{
	Screen3ViewBase::setupScreen();

	popupbackground.setVisible(false);
	popupbox.setVisible(false);
	popupheadertext.setVisible(false);
	popuptext.setVisible(false);

	scrollList.setNumberOfItems(MAX_ITEMS);
	scrollList.invalidate();

	buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_ENTER_ID, 2.5f, 2.5f);
	buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BACK_ID, 2.5f, 2.5f);
	buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
	buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);

	remove(menuContainer);
	add(menuContainer);
	menuContainer.setVisible(false);

	remove(warningPopupContainer);
	add(warningPopupContainer);

	remove(buttonContainer);
	add(buttonContainer);
	updateHeaderBoxSize();
}

void Screen3View::tearDownScreen()
{
    Screen3ViewBase::tearDownScreen();
}

void Screen3View::scrollListUpdateItem(SettingsItemContainer& item, int16_t itemIndex)
{
    item.setHighlighted((itemIndex == selectedIndex), (currentState == STATE_EDITING));

    switch (itemIndex)
    {
        case 0: item.setupItemValue(T_SETTINGMODE, presenter->getSalvoMode() == Model::SALVO_SINGLE_ROUND ? T_MODESINGLE : T_MODESALVO); break;
        case 1: item.setupItemValue(T_SETTINGWARNINGS, presenter->getWarningsEnabled() ? T_WARNON : T_WARNOFF); break;
        case 2: item.setupItemNumber(T_SETTINGBRIGHTNESS, presenter->getBrightness()); break;
        case 3: item.setupItemNumber(T_SETTINGLED, presenter->getLedBrightness()); break;
        case 4: item.setupItemNumber(T_SETTINGVOLUME, presenter->getVolume()); break;
        case 5: item.setupItemValue(T_SETTINGNETWORK, T_SHOW); break;
        case 6: item.setupItemValue(T_SETTINGVERSION, T_VERSION); break;
        case 7: item.setupItemValue(T_SETTINGLANGUAGE, touchgfx::Texts::getLanguage() == GB ? T_LANGTR : T_LANGEN); break;
    }

}

// ============================================
// TİCK (ZAMAN AŞIMI) MOTORU
// ============================================
void Screen3View::handleTickEvent()
{
    if (menuTimeout > 0)
    {
        menuTimeout--;
        if (menuTimeout == 0)
        {
            // Süre dolduğunda güvenli duruma dön!
            if (currentState == STATE_MENU_OPEN) {
                closeMenus(); // Yan menüyü kapat
            }
            else if (currentState == STATE_SHOWING_INFO || currentState == STATE_EDITING) {
                backPressed(); // Popup'ı kapat veya Düzenlemeyi iptal et
            }
        }
    }
}

// ============================================
// YENİ: MENÜ BUTONU İŞLEYİCİSİ
// ============================================
void Screen3View::menuButtonPressed()
{
    if (currentState == STATE_MENU_OPEN) {
        closeMenus(); // Zaten açıksa kapat
    }
    else {
        // Menü açılmadan önce açık olan Popup veya Düzenleme Modu varsa güvenlik için kapat
        if (currentState == STATE_SHOWING_INFO || currentState == STATE_EDITING) {
            backPressed();
        }

        currentState = STATE_MENU_OPEN;
        menuContainer.resetSelection();
        menuContainer.setVisible(true);
        menuContainer.invalidate();
        menuTimeout = 500;
    }
}

void Screen3View::closeMenus()
{
    if (currentState == STATE_MENU_OPEN) {
        currentState = STATE_NAVIGATING;
        menuContainer.setVisible(false);
        menuContainer.invalidate();
        menuTimeout = 0;
    }
}

// ============================================
// DİĞER TUŞLARIN GÜNCELLENMESİ
// ============================================
void Screen3View::enterPressed()
{
    menuTimeout = 500;

    // EĞER MENÜ AÇIKSA, ENTER TUŞU MENÜYÜ SEÇER!
    if (currentState == STATE_MENU_OPEN) {
        int selected = menuContainer.getSelectedItem();
        if (selected == 0)      presenter->gotoAyarlarScreen();
        else if (selected == 1) presenter->gotoArizaScreen();
        else if (selected == 2) presenter->gotoTestScreen();
        else if (selected == 3) presenter->gotoLoglarScreen();
        else if (selected == 4) presenter->gotoSoftResetScreen(); // Soft Reset menüsü
		else if (selected == 5) presenter->gotoZeroizeScreen();   // Acil Silme menüsü
        return;
    }

    if (currentState == STATE_NAVIGATING)
    {
        if ((selectedIndex >= 0 && selectedIndex <= 4) || selectedIndex == 7) {
            // İLK 5 SEÇENEK (Atış, Uyarı, Ekran Parlaklık, LED Parlaklık, Ses) İÇİN DÜZENLEME MODUNA GİR!
            currentState = STATE_EDITING;
            scrollList.itemChanged(selectedIndex);
        }
        else if (selectedIndex == 5) {
            // SADECE 6. SEÇENEKTE POPUP AÇ
            currentState = STATE_SHOWING_INFO;
            popupbackground.setVisible(true);
            popupbox.setVisible(true);
            popupheadertext.setVisible(true);
            popuptext.setVisible(true);

            popupbackground.invalidate();
            popupbox.invalidate();
            popupheadertext.invalidate();
            popuptext.invalidate();
        }
    }
    else if (currentState == STATE_EDITING)
    {
        // Düzenleme modundayken ENTER'a basılırsa KAYDEDİP ÇIKAR
        currentState = STATE_NAVIGATING;
        scrollList.itemChanged(selectedIndex);
    }
    else if (currentState == STATE_SHOWING_INFO)
    {
    	currentState = STATE_NAVIGATING;
    	popupbackground.setVisible(false);
		popupbox.setVisible(false);
		popupheadertext.setVisible(false);
		popuptext.setVisible(false);

		popupbackground.invalidate();
		popupbox.invalidate();
		popupheadertext.invalidate();
		popuptext.invalidate();
    }
}

void Screen3View::backPressed()
{
    // EĞER MENÜ AÇIKSA, BACK TUŞU MENÜYÜ KAPATIR!
    if (currentState == STATE_MENU_OPEN) {
        closeMenus();
        return;
    }

    if (currentState == STATE_SHOWING_INFO) {
        currentState = STATE_NAVIGATING;
        popupbackground.setVisible(false);
        popupbox.setVisible(false);
        popupheadertext.setVisible(false);
        popuptext.setVisible(false);

        popupbackground.invalidate();
        popupbox.invalidate();
        popupheadertext.invalidate();
        popuptext.invalidate();
    }
    else if (currentState == STATE_EDITING) {
        currentState = STATE_NAVIGATING;
        scrollList.itemChanged(selectedIndex);
    }
}

void Screen3View::buttonUpPressed()
{
    menuTimeout = 500;

    if (currentState == STATE_MENU_OPEN) {
        menuContainer.moveUp();
        return;
    }

    if (currentState == STATE_NAVIGATING) {
        int oldIndex = selectedIndex;
        int numItems = scrollList.getNumberOfItems();

        if (selectedIndex > 0) {
            selectedIndex--;
        } else {
            selectedIndex = numItems - 1;
        }

        scrollList.itemChanged(oldIndex);
        scrollList.itemChanged(selectedIndex);
    }
    else if (currentState == STATE_EDITING) {
        // DÜZENLEME MODUNDA YUKARI TUŞUNA BASILIRSA DEĞERLERİ DEĞİŞTİR
        if (selectedIndex == 0) {
            Model::SalvoMode current = presenter->getSalvoMode();
            presenter->setSalvoMode(current == Model::SALVO_SINGLE_ROUND ? Model::SALVO_FULL_EMPTY : Model::SALVO_SINGLE_ROUND);
        }
        else if (selectedIndex == 1) {
            presenter->setWarningsEnabled(!presenter->getWarningsEnabled());
        }
        else if (selectedIndex == 2) { // Ekran Parlaklığı Artır
            int b = presenter->getBrightness();
            if (b < 5) presenter->saveBrightness(b + 1);
        }
        else if (selectedIndex == 3) { // LED Parlaklığı Artır
            int l = presenter->getLedBrightness();
            if (l < 5) presenter->saveLedBrightness(l + 1);
        }
        else if (selectedIndex == 4) { // Ses Artır
            int v = presenter->getVolume();
            if (v < 5) presenter->saveVolume(v + 1);
        }
        else if (selectedIndex == 7) { // DİL DEĞİŞTİRİLDİ
			if (touchgfx::Texts::getLanguage() == ENG) {
				touchgfx::Texts::setLanguage(GB);
			} else {
				touchgfx::Texts::setLanguage(ENG);
			}
			for (int i = 0; i < scrollList.getNumberOfItems(); i++) {
				scrollList.itemChanged(i);
			}
			headerText.invalidate();
			updateHeaderBoxSize();
		}
        scrollList.itemChanged(selectedIndex); // Ekranı hemen güncelle
    }
}

void Screen3View::buttonDownPressed()
{
    menuTimeout = 500;

    if (currentState == STATE_MENU_OPEN) {
        menuContainer.moveDown();
        return;
    }

    if (currentState == STATE_NAVIGATING) {
        int oldIndex = selectedIndex;
        int numItems = scrollList.getNumberOfItems();

        if (selectedIndex < numItems - 1) {
            selectedIndex++;
        } else {
            selectedIndex = 0;
        }

        scrollList.itemChanged(oldIndex);
        scrollList.itemChanged(selectedIndex);
    }
    else if (currentState == STATE_EDITING) {
        // DÜZENLEME MODUNDA AŞAĞI TUŞUNA BASILIRSA DEĞERLERİ DEĞİŞTİR
        if (selectedIndex == 0) {
            Model::SalvoMode current = presenter->getSalvoMode();
            presenter->setSalvoMode(current == Model::SALVO_SINGLE_ROUND ? Model::SALVO_FULL_EMPTY : Model::SALVO_SINGLE_ROUND);
        }
        else if (selectedIndex == 1) {
            presenter->setWarningsEnabled(!presenter->getWarningsEnabled());
        }
        else if (selectedIndex == 2) { // Ekran Parlaklığı Azalt
            int b = presenter->getBrightness();
            if (b > 1) presenter->saveBrightness(b - 1);
        }
        else if (selectedIndex == 3) { // LED Parlaklığı Azalt
            int l = presenter->getLedBrightness();
            if (l > 0) presenter->saveLedBrightness(l - 1);
        }
        else if (selectedIndex == 4) { // Ses Azalt
            int v = presenter->getVolume();
            if (v > 0) presenter->saveVolume(v - 1);
        }
        else if (selectedIndex == 7) { // DİL DEĞİŞTİRİLDİ
			if (touchgfx::Texts::getLanguage() == ENG) {
				touchgfx::Texts::setLanguage(GB);
			} else {
				touchgfx::Texts::setLanguage(ENG);
			}
			for (int i = 0; i < scrollList.getNumberOfItems(); i++) {
				scrollList.itemChanged(i);
			}
			headerText.invalidate();
		}
        scrollList.itemChanged(selectedIndex); // Ekranı hemen güncelle
    }
}

void Screen3View::updateButtonVisuals(uint32_t state)
{
    // override parametresi vermiyoruz, Ayarlar ikonları sabit
    buttonContainer.updateButtonVisuals(state, 0);
}

void Screen3View::showWarningPopup(Model::WarningType warning, int tubeIndex)
{
    warningPopupContainer.showWarningMessage(warning, tubeIndex);
}

void Screen3View::updateHeaderBoxSize()
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

void Screen3View::updateTimeDate(const TLUS::TimeData& time)
{
    // Container senin yazdığın eski parametre yapısını kullandığı için veriyi burada açıyoruz
    timeDayContainer.updateTimeAndDate(time.hour, time.minute, time.day, time.month, time.year);
}
