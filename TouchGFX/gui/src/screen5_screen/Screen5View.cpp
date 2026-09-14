#include <gui/screen5_screen/Screen5View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <images/SVGDatabase.hpp>
#include <stdio.h>
#include "hc165_driver.h"

Screen5View::Screen5View() {}

void Screen5View::setupScreen()
{
    Screen5ViewBase::setupScreen();

    currentVolume = presenter->getVolume();
	currentBrightness = presenter->getBrightness();
	currentLedBrightness = presenter->getLedBrightness();

    // İkonları sabitle: Ses -> Başlat(Enter), Parlaklık -> İptal(Back)
    buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_ENTER_ID, 2.5f, 2.5f);
    buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BACK_ID, 2.5f, 2.5f);
    buttonContainer.setButtonIcon(BTN_UP, SVG_UP_ID, 2.5f, 2.5f);
	buttonContainer.setButtonIcon(BTN_DOWN, SVG_DOWN_ID, 2.5f, 2.5f);

    remove(menuContainer); add(menuContainer); menuContainer.setVisible(false);
    remove(warningPopupContainer); add(warningPopupContainer);
    remove(buttonContainer); add(buttonContainer);

    // Popup ve arka plan karartması başlangıçta kapalı
    popupbackground.setVisible(false);
    testPopupBox.setVisible(false);
    testPopupText.setVisible(false);
    testPopupHeaderText.setVisible(false);
    exitInfoText.setVisible(false);

    scrollList.setNumberOfItems(MAX_TESTS);
    scrollList.invalidate();

    updateHeaderBoxSize();
}

void Screen5View::tearDownScreen()
{
	presenter->setTestModeActive(false);
	// GÜVENLİK (FAIL-SAFE) KORUMASI
	// Eğer kullanıcı testin ortasında HOME (Radar) veya SMOKE (Sis) tuşuna basıp
	// ekranı aniden terk ederse, donanım o anki test durumunda (örn: buzzer ötük,
	// ledler kırmızı) takılı kalmasın diye çıkışta donanımı asıl ayarlarına ZORLA döndürüyoruz.

	if (currentState == STATE_TEST_RUNNING || currentState == STATE_TEST_FINISHED)
	{
		if (selectedIndex == TEST_BRIGHTNESS) {
			presenter->saveBrightness(currentBrightness);
		}
		else if (selectedIndex == TEST_LED) {
			presenter->saveLedBrightness(currentLedBrightness);
			presenter->setLedTestMode(0);
		}
		else if (selectedIndex == TEST_BUZZER) {
			presenter->setBuzzerLevel(ALARM_NONE);
			presenter->saveVolume(currentVolume);
		}
	}

	Screen5ViewBase::tearDownScreen();
}

void Screen5View::scrollListUpdateItem(TestItemContainer& item, int16_t itemIndex)
{
    if (itemIndex < MAX_TESTS) {
        item.setupTest(testNames[itemIndex]);
        item.setHighlighted(itemIndex == selectedIndex);
    }
}

void Screen5View::handleTickEvent()
{
    if (menuTimeout > 0) {
        menuTimeout--;
        if (menuTimeout == 0) closeMenus();
    }

    // CANLI TEST MOTORU
    if (currentState == STATE_TEST_RUNNING)
    {
        switch(selectedIndex)
        {
            case TEST_BRIGHTNESS: // EKRAN PARLAKLIK TESTİ
            {
                if (brightnessTestStep != 0)
                {
                    if (testPauseCounter > 0) {
                        testPauseCounter--;
                        break;
                    }

                    testTickCounter++;
                    if (testTickCounter >= 12)
                    {
                        testTickCounter = 0;
                        brightnessTestLevel += brightnessTestStep;

                        if (brightnessTestLevel <= 0) {
                            brightnessTestLevel = 0;
                            brightnessTestStep = 1;
                            testPauseCounter = 30;
                        }
                        else if (brightnessTestLevel >= 5 && brightnessTestStep == 1) {
                            brightnessTestLevel = 5;
                            brightnessTestStep = -1;
                            testPauseCounter = 30;

                            brightnessTestCycle++;

                            if (brightnessTestCycle >= 5) {
                                brightnessTestStep = 0; // Motoru durdur
                                testPauseCounter = 0;

                                presenter->saveBrightness(currentBrightness);
                                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPDONE).getText(), TESTPOPUPTEXT_SIZE);
                                testPopupText.invalidate();

                                // Test Bitti Moduna Geç!
                                currentState = STATE_TEST_FINISHED;
                            }
                        }

                        if (brightnessTestStep != 0) {
                            presenter->saveBrightness(brightnessTestLevel);
                        }
                    }
                }
                break;
            }
            case TEST_LED: // KONSOL LED TESTİ
            {
                if (ledTestStep != 0)
                {
                    if (testPauseCounter > 0) {
                        testPauseCounter--;
                        break;
                    }

                    testTickCounter++;
                    if (testTickCounter >= 100)
                    {
                        testTickCounter = 0;
                        ledTestStep++;

                        if (ledTestStep == 2) {
                            presenter->setLedTestMode(2); // YEŞİL
                        }
                        else if (ledTestStep == 3) {
                            presenter->setLedTestMode(3); // TURUNCU
                        }
                        else if (ledTestStep >= 4) {
                            brightnessTestCycle++;

                            if (brightnessTestCycle >= 3) {
                                ledTestStep = 0;
                                presenter->saveLedBrightness(currentLedBrightness);
                                presenter->setLedTestMode(0);

                                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPDONE).getText(), TESTPOPUPTEXT_SIZE);
                                testPopupText.invalidate();

                                // Test Bitti Moduna Geç!
                                currentState = STATE_TEST_FINISHED;
                            } else {
                                ledTestStep = 1;
                                presenter->setLedTestMode(1);
                            }
                        }
                    }
                }
                break;
            }
            case TEST_BUZZER: // BUZZER SES TESTİ
            {
                if (buzzerTestStep != 0)
                {
                    // 1. BEKLEME (ES VERME) VE BAŞLATMA MANTIĞI
                    if (testPauseCounter > 0) {
                        testPauseCounter--;

                        // Es bittiği an, sıradaki sesi başlat ve ekrana ÇOKLU DİL ile yaz!
                        if (testPauseCounter == 0) {
                            if (buzzerTestStep == 1) {
                                presenter->setBuzzerLevel(ALARM_LOW);
                                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPSESLOW).getText(), TESTPOPUPTEXT_SIZE);
                            }
                            else if (buzzerTestStep == 2) {
                                presenter->setBuzzerLevel(ALARM_MEDIUM);
                                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPSESMID).getText(), TESTPOPUPTEXT_SIZE);
                            }
                            else if (buzzerTestStep == 3) {
                                presenter->setBuzzerLevel(ALARM_HIGH);
                                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPSESHIGH).getText(), TESTPOPUPTEXT_SIZE);
                            }
                            testPopupText.invalidate();
                        }
                        break;
                    }

                    // 2. SESİ ÇALMA SÜRESİ VE BİTİRME MANTIĞI
                    testTickCounter++;
                    if (testTickCounter >= 90) // 1.5 Saniye Çal
                    {
                        testTickCounter = 0;
                        buzzerTestStep++;

                        // SESİ KAPAT (ES VER!)
                        presenter->setBuzzerLevel(ALARM_NONE);

                        if (buzzerTestStep <= 3) {
                            testPauseCounter = 90; // SESSİZLİK
                            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPSESSILENCE).getText(), TESTPOPUPTEXT_SIZE);
                            testPopupText.invalidate();
                        }
                        else {
                            // TEST TAMAMEN BİTTİ
                            buzzerTestStep = 0;
                            presenter->saveVolume(currentVolume);

                            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPDONE).getText(), TESTPOPUPTEXT_SIZE);
                            testPopupText.invalidate();

                            // Test Bitti Moduna Geç!
                            currentState = STATE_TEST_FINISHED;
                        }
                    }
                }
                break;
            }
            case TEST_BMB_POWER: // BMB GÜÇ VE VOLTAJ TESTİ
			{
				testTickCounter++;
				if (testTickCounter >= 30)
				{
					testTickCounter = 0;

					// 1. İletişim Kopuksa
					if (presenter->getCommLostFlag())
					{
						if (touchgfx::Texts::getLanguage() == GB) {
							touchgfx::Unicode::fromUTF8((const uint8_t*)"BMB COMM LOST!\n\nCheck cables. Cannot read data.", testPopupTextBuffer, TESTPOPUPTEXT_SIZE);
						} else {
							touchgfx::Unicode::fromUTF8((const uint8_t*)"BMB İLETİŞİMİ KOPTU!\n\nKabloları kontrol edin. Veri yok.", testPopupTextBuffer, TESTPOPUPTEXT_SIZE);
						}
					}
					// 2. İletişim Varsa Model'den "Güvenli" Struct'ı İste ve Ekrana Bas!
					else
					{
						// Model'den donanımdan soyutlanmış veriyi çekiyoruz!
						Model::BmbTelemetryData tele = presenter->getBmbTelemetry();

						char buffer[256];

						if (touchgfx::Texts::getLanguage() == ENG)
						{
							snprintf(buffer, sizeof(buffer),
								"COMMUNICATION: ACTIVE\n"
								"Console In : %lu mV [%s]\n"
								"Blast Volt : %u mV [%s]\n"
								"Main 5V    : %u mV [%s]\n"
								"Sensor 3.3V: %u mV [%s]",
								tele.console_input_voltage, (tele.console_volt_err == 0 ? "OK" : "ERR"),
								tele.blast_volt_reg1,       (tele.blast_ready_err == 0 ? "OK" : "ERR"),
								tele.main_5v_reg,           (tele.main_5v_err == 0 ? "OK" : "ERR"),
								tele.power_3v3_reg,         (tele.mcu_3v3_err == 0 ? "OK" : "ERR")
							);
						}
						else
						{
							snprintf(buffer, sizeof(buffer),
								"İLETİŞİM: AKTİF\n"
								"Konsol Giriş: %lu mV [%s]\n"
								"Patlatma    : %u mV [%s]\n"
								"Ana 5V      : %u mV [%s]\n"
								"Sensör 3.3V : %u mV [%s]",
								tele.console_input_voltage, (tele.console_volt_err == 0 ? "OK" : "HATA"),
								tele.blast_volt_reg1,       (tele.blast_ready_err == 0 ? "OK" : "HATA"),
								tele.main_5v_reg,           (tele.main_5v_err == 0 ? "OK" : "HATA"),
								tele.power_3v3_reg,         (tele.mcu_3v3_err == 0 ? "OK" : "HATA")
							);
						}

						touchgfx::Unicode::fromUTF8((const uint8_t*)buffer, testPopupTextBuffer, TESTPOPUPTEXT_SIZE);
					}

					testPopupText.invalidate();
				}
				break;
			}
            case TEST_COMMUNICATION: // İLETİŞİM DURUMU TESTİ
			{
				// Her yarım saniyede bir (30 tick) ekranı canlı olarak güncelle
				testTickCounter++;
				if (testTickCounter >= 30)
				{
					testTickCounter = 0;

					// Model'den iletişim durumlarını al
					Model::CommTelemetryData comm = presenter->getCommTelemetry();

					char buffer[256];

					if (touchgfx::Texts::getLanguage() == ENG)
					{
						// İNGİLİZCE ÇIKTI
						snprintf(buffer, sizeof(buffer),
							"BMB (UART)   : [%s]\n"
							"Radar (ETH)  : [%s]\n"
							"Radar(RS422) : [%s] (BACKUP)\n"
							"Logger (CAN) : [%s]",
							(comm.bmb_uart_ok ? "ONLINE" : "OFFLINE"),
							(comm.radar_eth_ok ? "ONLINE" : "OFFLINE"),
							(comm.radar_rs422_ok ? "ONLINE" : "OFFLINE"),
							(comm.can_bus_ok ? "ONLINE" : "ERROR")
						);
					}
					else
					{
						// TÜRKÇE ÇIKTI
						snprintf(buffer, sizeof(buffer),
							"BMB (UART)   : [%s]\n"
							"Radar (ETH)  : [%s]\n"
							"Radar(RS422) : [%s] (YEDEK)\n"
							"Logger (CAN) : [%s]",
							(comm.bmb_uart_ok ? "BAĞLI" : "KOPUK"),
							(comm.radar_eth_ok ? "BAĞLI" : "KOPUK"),
							(comm.radar_rs422_ok ? "BAĞLI" : "KOPUK"),
							(comm.can_bus_ok ? "BAĞLI" : "HATA") // CAN sadece TX olduğu için "Bağlı" yerine "Hata" ibaresi daha mantıklıdır
						);
					}

					// Arayüze bas
					touchgfx::Unicode::fromUTF8((const uint8_t*)buffer, testPopupTextBuffer, TESTPOPUPTEXT_SIZE);
					testPopupText.invalidate();
				}
				break;
			}
            default :
                break;
        }
    }
}
void Screen5View::menuButtonPressed()
{
    if (currentState == STATE_TEST_RUNNING) return; // Test sırasında menü açılmasın

    if (currentState == STATE_MENU_OPEN) {
        closeMenus();
    } else {
        currentState = STATE_MENU_OPEN;
        menuContainer.resetSelection();
        menuContainer.setVisible(true);
        menuContainer.invalidate();
        menuTimeout = 500;

        buttonContainer.setButtonIcon(BTN_VOLUME_MENU, SVG_ENTER_ID, 2.5f, 2.5f);
        buttonContainer.setButtonIcon(BTN_BRIGHTNESS_MENU, SVG_BACK_ID, 2.5f, 2.5f);
    }
}

void Screen5View::closeMenus()
{
    if (currentState == STATE_MENU_OPEN) {
        currentState = STATE_NAVIGATING;
        menuContainer.setVisible(false);
        menuContainer.invalidate();
        menuTimeout = 0;
    }
}

void Screen5View::enterPressed()
{
    menuTimeout = 500;

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

    if (currentState == STATE_NAVIGATING) {
        // Testi Başlat
        runSelectedTest();
    }
    else if (currentState == STATE_TEST_FINISHED) {
		// Test bitmişse ve ENTER'a basıldıysa testi yeniden başlat!
		runSelectedTest();
	}
}

void Screen5View::backPressed()
{
    if (currentState == STATE_MENU_OPEN) {
        closeMenus();
    }
    else if (currentState == STATE_TEST_RUNNING || currentState == STATE_TEST_FINISHED) {
        // Testi Durdur / Kapat
        stopCurrentTest();
    }
    else {
        // Ana ekrana dön
       // presenter->onHomePressedReal();
    }
}

void Screen5View::runSelectedTest()
{
    currentState = STATE_TEST_RUNNING;

    presenter->setTestModeActive(true);

    // 1. Listeyi Gizle, Popup'ı ve Karartmayı Aç
    scrollList.setVisible(false);
    popupbackground.setVisible(true);
    testPopupBox.setVisible(true);
    testPopupHeaderText.setVisible(true);
    testPopupText.setVisible(true);
    exitInfoText.setVisible(true);

    touchgfx::Unicode::strncpy(testPopupHeaderTextBuffer, touchgfx::TypedText(testNames[selectedIndex]).getText(), TESTPOPUPHEADERTEXT_SIZE);

    switch(selectedIndex)
    {
        case 0: // EKRAN PARLAKLIK TESTİ
        	touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPBRIGHTNESS).getText(), TESTPOPUPTEXT_SIZE);
            // Test Değerlerini Kur
            brightnessTestLevel = 5; // En üst parlaklıktan başla
            brightnessTestStep = -1; // Aşağı doğru azalt
            testTickCounter = 0;
            brightnessTestCycle = 0; // Döngüyü sıfırla
            testPauseCounter = 30; // Test başlamadan önce ekrandaki yazının okunabilmesi için yarım saniye bekle
            break;

        case 1: // KONSOL LED TESTİ
        	touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPLED).getText(), TESTPOPUPTEXT_SIZE);

			ledTestStep = 1; // 1:Kırmızı, 2:Yeşil, 3:Turuncu
			brightnessTestCycle = 0; // 3 Tur dönecek
			testTickCounter = 0;
			testPauseCounter = 30; // Test başlamadan yarım saniye bekle

			presenter->saveLedBrightness(5);
			presenter->setLedTestMode(1); // Donanımı Kırmızıya Çek
            break;

        case 2: // BUZZER (SES) TESTİ
        	touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPSESSTART).getText(), TESTPOPUPTEXT_SIZE);
			buzzerTestStep = 1; // 1:Düşük, 2:Orta, 3:Yüksek
			testTickCounter = 0;
			testPauseCounter = 30; // Test başlamadan yarım saniye SESSİZ bekle

			// Sesi fulle ama henüz çalma (Sessizlikle başla)
			presenter->saveVolume(5);
			presenter->setBuzzerLevel(ALARM_NONE);
            break;

        case 3: // BUTON / ŞALTER TESTİ
        	touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPBUTTON).getText(), TESTPOPUPTEXT_SIZE);

            break;

        case 4: // BMB GÜÇ VE VOLTAJ TESTİ
        	touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPBMB).getText(), TESTPOPUPTEXT_SIZE);

            break;

        case 5: // İLETİŞİM DURUMU
        	touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_POPUPCOMM).getText(), TESTPOPUPTEXT_SIZE);

            break;
    }

    // 4. Değişiklikleri Ekranda Yenile
    scrollList.invalidate();
    popupbackground.invalidate();
    testPopupBox.invalidate();
    testPopupHeaderText.invalidate();
    testPopupText.invalidate();
    exitInfoText.invalidate();
}

void Screen5View::stopCurrentTest()
{
    currentState = STATE_NAVIGATING;

    presenter->setTestModeActive(false);

    popupbackground.setVisible(false);
    testPopupBox.setVisible(false);
    testPopupHeaderText.setVisible(false);
    testPopupText.setVisible(false);
    scrollList.setVisible(true);
    exitInfoText.setVisible(false);

    // EĞER İPTAL EDİLEN VEYA BİTEN TEST PARLAKLIK TESTİYSE:
    // Cihazın kendi kayıtlı parlaklık ayarını donanıma kesin olarak geri yükle!
    if (selectedIndex == 0) {
        presenter->saveBrightness(currentBrightness);
        brightnessTestStep = 0; // Motoru durdur
    }
    else if (selectedIndex == 1) { // EĞER LED TESTİ İPTAL EDİLDİYSE
    	presenter->saveLedBrightness(currentLedBrightness);
		presenter->setLedTestMode(0); // Anında normal duruma dön
		ledTestStep = 0;
	}
    else if(selectedIndex == TEST_BUZZER)
    {
    	presenter->setBuzzerLevel(ALARM_NONE);
		presenter->saveVolume(currentVolume);
		buzzerTestStep = 0;
    }
    popupbackground.invalidate();
    testPopupBox.invalidate();
    testPopupHeaderText.invalidate();
    testPopupText.invalidate();
    scrollList.invalidate();
    exitInfoText.invalidate();
}

void Screen5View::buttonUpPressed()
{
    if (currentState == STATE_TEST_RUNNING) return; // Test sırasında listede gezinme

    menuTimeout = 500;
    if (currentState == STATE_MENU_OPEN) {
        menuContainer.moveUp();
        return;
    }

    if (currentState == STATE_NAVIGATING) {
        int oldIndex = selectedIndex;
        if (selectedIndex > 0) selectedIndex--;
        else selectedIndex = MAX_TESTS - 1; // Başa Sar

        scrollList.itemChanged(oldIndex);
        scrollList.itemChanged(selectedIndex);
    }
}

void Screen5View::buttonDownPressed()
{
    if (currentState == STATE_TEST_RUNNING) return; // Test sırasında listede gezinme

    menuTimeout = 500;
    if (currentState == STATE_MENU_OPEN) {
        menuContainer.moveDown();
        return;
    }

    if (currentState == STATE_NAVIGATING) {
        int oldIndex = selectedIndex;
        if (selectedIndex < MAX_TESTS - 1) selectedIndex++;
        else selectedIndex = 0; // Başa Sar

        scrollList.itemChanged(oldIndex);
        scrollList.itemChanged(selectedIndex);
    }
}

void Screen5View::updateButtonVisuals(uint32_t state)
{
    // Mevcut görsel ikonları (sol/sağ menü tuşlarını) yakma/söndürme işlemi
    buttonContainer.updateButtonVisuals(state, 0);

    // ==============================================================
    // BUTON VE ŞALTER TESTİ CANLI GERİ BİLDİRİM MOTORU (EDGE DETECTION)
    // ==============================================================
    if (currentState == STATE_TEST_RUNNING && selectedIndex == TEST_BUTTON)
    {
        // Önceki durum ile yeni durum arasındaki "FARKLI" olan bitleri bul (XOR)
        uint32_t changed_bits = state ^ last_test_button_state;

        // Sadece 0'dan 1'e geçenleri (Basılanları) bul (AND)
        uint32_t pressed_bits = changed_bits & state;

        bool textUpdated = false;

        // ÖNCELİK 1: ANLIK BUTONLAR (SADECE BASILDIĞINDA EKRANA YAZ, BIRAKILINCA SİLME!)
        if (pressed_bits & BTN_HOME) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNHOME).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_MENU) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNMENU).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_SMOKE) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNSMOKE).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_BLACKOUT) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNBLACKOUT).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_FRNT_LEFT) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNFRNTLEFT).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_FRNT_RIGHT) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNFRNTRIGHT).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_REAR_LEFT) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNREARLEFT).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_REAR_RIGHT) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNREARRIGHT).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_FRNT_REAR_ALL) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNSALVO).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_UP) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNUP).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_DOWN) {
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNDOWN).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        } else if (pressed_bits & BTN_VOLUME_MENU) { // ENTER TUŞU
            touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_BTNENTER).getText(), TESTPOPUPTEXT_SIZE);
            textUpdated = true;
        }
        // NOT: BTN_BRIGHTNESS_MENU (BACK) tuşuna basıldığında zaten testten çıkacağı için onu eklemedik.

        // ÖNCELİK 2: KALICI ŞALTERLER (LATCH) -> Değişim (changed_bits) varsa ekrana yaz
        else if (changed_bits & KOM2_UP) {
            if (state & KOM2_UP) {
                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_SWARMED).getText(), TESTPOPUPTEXT_SIZE);
            } else {
                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_SWDISARMED).getText(), TESTPOPUPTEXT_SIZE);
            }
            textUpdated = true;
        }
        else if (changed_bits & KOM1_UP) {
            if (state & KOM1_UP) {
                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_SWFRAG).getText(), TESTPOPUPTEXT_SIZE);
            } else {
                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_SWNEUTRAL).getText(), TESTPOPUPTEXT_SIZE);
            }
            textUpdated = true;
        }
        else if (changed_bits & KOM1_DOWN) {
            if (state & KOM1_DOWN) {
                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_SWSMOKE).getText(), TESTPOPUPTEXT_SIZE);
            } else {
                touchgfx::Unicode::strncpy(testPopupTextBuffer, touchgfx::TypedText(T_SWNEUTRAL).getText(), TESTPOPUPTEXT_SIZE);
            }
            textUpdated = true;
        }

        // Eğer yeni bir tuşa basıldıysa veya şalter oynatıldıysa ekranı yenile
        if (textUpdated) {
            testPopupText.invalidate();
        }
    }

    // Her halükarda donanımın son durumunu hafızaya kaydet ki bir sonraki sefer farkı (edge) bulabilelim!
    last_test_button_state = state;
}

void Screen5View::showWarningPopup(Model::WarningType warning, int tubeIndex)
{
    warningPopupContainer.showWarningMessage(warning, tubeIndex);
}

void Screen5View::updateHeaderBoxSize()
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

void Screen5View::updateTimeDate(const TLUS::TimeData& time)
{
    // Container senin yazdığın eski parametre yapısını kullandığı için veriyi burada açıyoruz
    timeDayContainer.updateTimeAndDate(time.hour, time.minute, time.day, time.month, time.year);
}
