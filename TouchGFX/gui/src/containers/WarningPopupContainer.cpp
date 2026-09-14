#include <gui/containers/WarningPopupContainer.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Utils.hpp>
#include <touchgfx/Color.hpp>

// Model.cpp'deki tüp numarasını buraya alıyoruz
extern int last_warning_tube_index;

WarningPopupContainer::WarningPopupContainer() : timeoutCounter(0), queueCount(0)
{
}

void WarningPopupContainer::initialize()
{
    WarningPopupContainerBase::initialize();
}

// Dışarıdan yeni bir uyarı geldiğinde doğrudan kuyruğa ekle
void WarningPopupContainer::showWarningMessage(Model::WarningType warning, int tubeIndex)
{
    // Kuyrukta yer varsa yeni uyarıyı ekle
    if (queueCount < MAX_QUEUE_SIZE)
    {
        warningQueue[queueCount].type = warning;
        warningQueue[queueCount].tubeIndex = tubeIndex;
        queueCount++;
    }

    // Eğer şu an ekranda GÖSTERİLEN HİÇBİR UYARI YOKSA, hemen kuyruğu işletmeye başla
    if (!isVisible())
    {
        showNextWarning();
    }
}

// Kuyruktan sıradakini çekip ekrana basan asıl motor
// Kuyruktan sıradakini çekip ekrana basan asıl motor
void WarningPopupContainer::showNextWarning()
{
    if (queueCount == 0) return;

    Model::WarningType currentWarning = warningQueue[0].type;
    int currentTubeIndex = warningQueue[0].tubeIndex;

    for (int i = 0; i < queueCount - 1; i++) {
        warningQueue[i] = warningQueue[i + 1];
    }
    queueCount--;

    uint8_t r = 139; uint8_t g = 0; uint8_t b = 0;
    touchgfx::TypedTextId msgId = T_WARNLOCKED;

    // YENİ: Sadece sensörler değil, içinde %d olan HER ŞEY için kullanacağız
    bool hasWildcard = false;

    switch (currentWarning) {
        case Model::WARN_SYSTEM_LOCKED:       msgId = T_WARNLOCKED; break;
        case Model::WARN_NO_AMMO_SELECTED:    msgId = T_WARNNOAMMO; r=184; g=134; b=11; break;
        case Model::WARN_GROUP_TOTALLY_EMPTY: msgId = T_WARNEMPTY; break;
        case Model::WARN_WRONG_AMMO_IN_GROUP: msgId = T_WARNWRONGAMMO; break;
        case Model::WARN_COMM_ERROR:          msgId = T_WARNCOMMERR; break;
        case Model::WARN_COMM_LOST:           msgId = T_WARNFATALCOMM; break;
        case Model::WARN_VOLTAGE_ERROR:       msgId = T_WARNVOLTAGE; break;

        // --- YENİ ATIŞ MESAJLARI (Hepsinde %d olduğu için hasWildcard = true) ---
        case Model::WARN_BLASTING_FAILED_SMOKE: msgId = T_WARNBLASTFAIL_SMOKE; hasWildcard = true; break;
        case Model::WARN_BLASTING_FAILED_FRAG:  msgId = T_WARNBLASTFAIL_FRAG; hasWildcard = true; break;
        case Model::WARN_MISFIRE_SMOKE:         msgId = T_WARNMISFIRE_SMOKE; hasWildcard = true; break;
        case Model::WARN_MISFIRE_FRAG:          msgId = T_WARNMISFIRE_FRAG; hasWildcard = true; break;
        case Model::WARN_FIRE_SUCCESS_SMOKE:    msgId = T_WARNSUCCESS_SMOKE; r=85; g=107; b=47; hasWildcard = true; break;
        case Model::WARN_FIRE_SUCCESS_FRAG:     msgId = T_WARNSUCCESS_FRAG; r=85; g=107; b=47; hasWildcard = true; break;

        // --- İŞLEMCİ BİRİMİ HATALARI ---
        case Model::WARN_PROC_RAM:       msgId = T_FLT_PROC_RAM; break;
        case Model::WARN_PROC_NVRAM:     msgId = T_FLT_PROC_NVRAM; break;
        case Model::WARN_PROC_MEMFILE:   msgId = T_FLT_PROC_MEMFILE; break;
        case Model::WARN_PROC_NVSRAM:    msgId = T_FLT_PROC_NVSRAM; break;
        case Model::WARN_PROC_MEMFULL:   msgId = T_FLT_PROC_MEMFULL; break;
        case Model::WARN_PROC_SER1:      msgId = T_FLT_PROC_SER1; break;
        case Model::WARN_PROC_SER2:      msgId = T_FLT_PROC_SER2; break;
        case Model::WARN_PROC_SER3:      msgId = T_FLT_PROC_SER3; break;
        case Model::WARN_PROC_SER4:      msgId = T_FLT_PROC_SER4; break;
        case Model::WARN_PROC_IFACE:     msgId = T_FLT_PROC_IFACE; break;
        case Model::WARN_PROC_PWR:       msgId = T_FLT_PROC_PWR; break;
        case Model::WARN_PROC_SHUTDOWN:  msgId = T_FLT_PROC_SHUTDOWN; break;
        case Model::WARN_PROC_PWR_SER:   msgId = T_FLT_PROC_PWR_SER; break;
        case Model::WARN_PROC_TEMP:      msgId = T_FLT_PROC_TEMP; break;

        // --- SENSÖR HATALARI (Hepsinde %d olduğu için hasWildcard = true) ---
        case Model::WARN_SENS_B12:       msgId = T_FLT_SENS_B12; hasWildcard = true; break;
        case Model::WARN_SENS_B3_0:      msgId = T_FLT_SENS_B3_0; hasWildcard = true; break;
        case Model::WARN_SENS_B3_1:      msgId = T_FLT_SENS_B3_1; hasWildcard = true; break;
        case Model::WARN_SENS_B3_2:      msgId = T_FLT_SENS_B3_2; hasWildcard = true; break;
        case Model::WARN_SENS_CTRL:      msgId = T_FLT_SENS_CTRL; hasWildcard = true; break;
        case Model::WARN_SENS_SHUTDOWN:  msgId = T_FLT_SENS_SHUTDOWN; hasWildcard = true; break;
        case Model::WARN_SENS_PWR_SER:   msgId = T_FLT_SENS_PWR_SER; hasWildcard = true; break;
        case Model::WARN_SENS_PRESS:     msgId = T_FLT_SENS_PRESS; hasWildcard = true; break;
        case Model::WARN_SENS_V3_7:      msgId = T_FLT_SENS_V3_7; hasWildcard = true; break;
        case Model::WARN_SENS_V7_4:      msgId = T_FLT_SENS_V7_4; hasWildcard = true; break;
        case Model::WARN_SENS_V16:       msgId = T_FLT_SENS_V16; hasWildcard = true; break;
        case Model::WARN_SENS_V80:       msgId = T_FLT_SENS_V80; hasWildcard = true; break;
        case Model::WARN_SENS_VN7_4:     msgId = T_FLT_SENS_VN7_4; hasWildcard = true; break;
        case Model::WARN_SENS_VN3_7:     msgId = T_FLT_SENS_VN3_7; hasWildcard = true; break;
        case Model::WARN_SENS_PWR:       msgId = T_FLT_SENS_PWR; hasWildcard = true; break;
        case Model::WARN_SENS_TEMP_LIM:  msgId = T_FLT_SENS_TEMP_LIM; hasWildcard = true; break;
        case Model::WARN_SENS_TEMP_SNS:  msgId = T_FLT_SENS_TEMP_SNS; hasWildcard = true; break;
    }

    if (hasWildcard)
    {
        // İçinde %d olan metinler için tüp/sensör numarasını göm (Eski karmaşık string eklemeleri tamamen silindi)
        touchgfx::Unicode::snprintf(textArea1Buffer, TEXTAREA1_SIZE, touchgfx::TypedText(msgId).getText(), currentTubeIndex);
    }
    else
    {
        // İçinde %d OLMAYAN normal metinler
        touchgfx::Unicode::strncpy(textArea1Buffer, touchgfx::TypedText(msgId).getText(), TEXTAREA1_SIZE);

        // Yalnızca "Bölge Tamamen Boş" veya "İstenen Tip Yok" hatalarında sonuna bölgeyi ekle
        if (currentTubeIndex != -1 && (currentWarning == Model::WARN_GROUP_TOTALLY_EMPTY || currentWarning == Model::WARN_WRONG_AMMO_IN_GROUP))
        {
            uint16_t currentLen = touchgfx::Unicode::strlen(textArea1Buffer);
            if (currentLen < TEXTAREA1_SIZE)
            {
                touchgfx::TypedTextId regionId = msgId;
                if (currentTubeIndex >= 1 && currentTubeIndex <= 4)       regionId = T_WARNFL;
                else if (currentTubeIndex >= 5 && currentTubeIndex <= 8)  regionId = T_WARNRL;
                else if (currentTubeIndex >= 9 && currentTubeIndex <= 12) regionId = T_WARNFR;
                else if (currentTubeIndex >= 13 && currentTubeIndex <= 16) regionId = T_WARNRR;

                touchgfx::Unicode::snprintf(&textArea1Buffer[currentLen], TEXTAREA1_SIZE - currentLen, " - ");
                currentLen = touchgfx::Unicode::strlen(textArea1Buffer);
                touchgfx::Unicode::strncpy(&textArea1Buffer[currentLen], touchgfx::TypedText(regionId).getText(), (TEXTAREA1_SIZE - currentLen));
            }
        }
    }

    boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(r, g, b));
    boxWithBorder.invalidate();
    textArea1.invalidate();

    setVisible(true);
    invalidate();

    timeoutCounter = 180;
    Application::getInstance()->registerTimerWidget(this);
}

void WarningPopupContainer::handleTickEvent()
{
    if (timeoutCounter > 0)
    {
        timeoutCounter--;
        if (timeoutCounter == 0)
        {
            hideWarning(); // Kod tekrarını önlemek için doğrudan hide fonksiyonunu çağırdık
        }
    }
}

void WarningPopupContainer::hideWarning()
{
	if (isVisible())
	{
		setVisible(false);
		invalidate();
		timeoutCounter = 0;
		// last_warning_tube_index = -1; <-- SİLİNDİ
		Application::getInstance()->unregisterTimerWidget(this);

		//Kapanır kapanmaz kuyrukta bekleyen var mı bak!
		if (queueCount > 0)
		{
			showNextWarning(); // Varsa anında sonrakini çağır
		}
	}
}
