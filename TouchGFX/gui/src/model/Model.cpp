#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "hc165_driver.h"
#include "lp5036_led_driver.h"
#include "can_logger.h"
#include "uart2_driver.h"
#include "rtc.h"
#include <gui/common/FrontendApplication.hpp>
#include <texts/TextKeysAndLanguages.hpp> // BÜTÜN METİNLERİ TANIMASI İÇİN EKLENDİ!

extern "C" {
    void Set_LCD_Brightness(uint8_t value);
    extern volatile uint8_t radar_flag;
	void Hardware_Transmit_Data(uint8_t* data, uint16_t length);

}

/* ICD: tehdit mesaji 40 ms periyotlu. 250 ms ~ 6 kacirilmis mesaj.
 * UDP jitter'ina tolerans birakiyor ama operatoru uzun sure
 * bayat veriyle bas basa birakmiyor. */
#define THREAT_STALE_TIMEOUT_MS   250U

/* Tehdit 100 ms surup kaybolsa bile operator duysun diye
 * alarm en az bu sure boyunca tutulur. */
#define ALARM_MIN_HOLD_MS        3000U

#define CM_ACK_WINDOW_MS   20000U   /* karsi tedbir sonrasi sessizlestirme */
#define CM_ACK_REMINDER_MS  5000U   /* o pencerede hatirlatma araligi */

static Model* globalModelInstance = nullptr;

uint32_t measured_blast_duration = 0;
AlarmLevel_t buzzerlevel = ALARM_NONE;

struct ThreatResponse {
    AlarmLevel_t level;
    bool         continuous;
    uint32_t     reminderMs;
};

/* KARAR NOTU: "continuous" olanlar operatorden ANINDA aksiyon
 * (karsi tedbir + manevra) isteyen siniflardir. Sesin kesilmesi
 * "durum cozuldu" mesaji verir; LD/LBR icin bu yanlistir. */
static ThreatResponse classResponse(TLUS::ThreatClass c)
{
    switch (c)
    {
        case TLUS::THREAT_LBR:  return { ALARM_HIGH,   true,      0U };
        case TLUS::THREAT_LD:   return { ALARM_HIGH,   true,      0U };
        case TLUS::THREAT_LRF:  return { ALARM_MEDIUM, false, 15000U };
        case TLUS::THREAT_SRCH: return { ALARM_LOW,    false, 30000U };
        default:                return { ALARM_LOW,    false, 30000U };
    }
}

Model::Model() : modelListener(0), isBlackoutMode(false), savedBrightnessPWM(60), currentBrightness(3), currentVolume(1)
{

    // Başlangıçta hiçbir tüpte arıza yok
    for(int i = 0; i < 16; i++) {
        ariza_hafizasi[i] = false;
        ariza_is_frag[i] = false;
        tube_states[i] = TUBE_EMPTY;
    }
    last_raw_button_state = 0;

    // CBIT Başlangıç değerleri
    comm_lost_flag = false;
    hw_error_flag = false;
    for (int i = 0; i < MAX_TARGETS; i++)
	{
		fadeTicksRemaining[i] = -1; // Initialize timers to off
	}
    lastModelTickTime = HAL_GetTick();

    globalModelInstance = this;

    icdManager.setListener(this);

	icdManager.setTxFunction(Hardware_Transmit_Data);
	addUILog(LOG_EVENT_SYSTEM_STATE, T_LOG_SYSTEM_BOOT);
}

// ==============================================================
// DONANIM ŞALTER DURUMLARI
// ==============================================================
bool Model::isSystemArmed() const {
    return (HC165_Get_Button_State() & KOM2_UP) != 0; // KOM2_UP = ARMED 6
}

bool Model::isSmokeSelected() const {
    return (HC165_Get_Button_State() & KOM1_DOWN) != 0; // KOM1_DOWN = SMOKE
}

bool Model::isFragSelected() const {
    return (HC165_Get_Button_State() & KOM1_UP) != 0; // KOM1_UP = FRAG
}

void  Model::saveLedBrightness(int level) {
	currentLedBrightness = level;
	LP5036_SetGlobalBrightness(currentLedBrightness);
}
/* feyzullah abim 05.08.26 tarihinde saat 11.30 sularında tam ı tamına 3 kilo sıçarak bilinen esetron rekorunu kırmıştır */
void Model::setLedTestMode(int mode)
{
    currentLedTestMode = mode;

    if (mode == 0)
    {
        // 1. TEST BİTTİ: Önce tüm ışıkları sıfırla
        LP5036_AllLedsOff();
        LP5036_AllLedsOn();

        // 2. Şalter LED'lerini (Arm/Smoke/Frag) yeniden çizmeye zorlamak için buton hafızasını bozuyoruz:
        // (Böylece processButtons() fonksiyonu değişiklik algılayıp buton LED'lerini geri yakacak)
        last_raw_button_state = 0xFFFFFFFF;

        // 3. Tüp (Yuva) LED'lerini mevcut durumlarına göre ZORLA yeniden yak!
        for (int i = 0; i < 16; i++) {
            LP5036_LedState_t fiziksel_led_renk = LP5036_LED_OFF;
            switch (tube_states[i]) {
                case TUBE_SMOKE: fiziksel_led_renk = LP5036_LED_GREEN;  break;
                case TUBE_FRAG:  fiziksel_led_renk = LP5036_LED_ORANGE; break;
                case TUBE_FAULT: fiziksel_led_renk = LP5036_LED_RED;    break;
                case TUBE_EMPTY:
                default:         fiziksel_led_renk = LP5036_LED_OFF;    break;
            }
            LP5036_SetLedState(i, fiziksel_led_renk);
        }
        for(uint8_t i = 20; i < 34; i++) {
			LP5036_SetLedState(i, LP5036_LED_DIM);
		}
    }
    else
    {
        // TEST ÇALIŞIYOR: İstenen rengi tüm LED'lere bas
        LP5036_LedState_t color = LP5036_LED_OFF;
        if (mode == 1) color = LP5036_LED_RED;
        if (mode == 2) color = LP5036_LED_GREEN;
        if (mode == 3) color = LP5036_LED_ORANGE;

        // LP5036_PHYSICAL_LED_COUNT sürücüde 36 olarak tanımlıdır.
        for (int i = 0; i < int(LP5036_PHYSICAL_LED_COUNT); i++) {
            LP5036_SetLedState(i, color);
        }
    }
}

// ==============================================================
// AYARLAR VE SIFIRLAMA
// ==============================================================
void Model::saveBrightness(int brightness)
{
    if(!isBlackoutMode)
    {
        currentBrightness = brightness;
        savedBrightnessPWM = (brightness < 1) ? 5 : ((uint8_t)(brightness * 20));
        Set_LCD_Brightness(savedBrightnessPWM);
    }
}

void Model::saveVolume(int volume)
{
    currentVolume = volume;
    Buzzer_SetVolume((uint8_t)volume);
}

void Model::dispatchWarning(WarningType warning, int tubeIndex)
{
    // CAN LOGLAMA İÇİN SAAT BİLGİSİ
    uint8_t h = systemTime.isValid ? systemTime.hour : 0;
    uint8_t m = systemTime.isValid ? systemTime.minute : 0;
    uint8_t s = systemTime.isValid ? systemTime.second : 0;
    uint8_t tIdx = (tubeIndex > 0) ? (uint8_t)tubeIndex : 0;

    // CAN HATTINA FIRLAT
    CAN_Log_Add(LOG_SYS_WARNING, (uint8_t)warning, tIdx, h, m, s, 0, 0);

    // EKRAN LOGU İÇİN TAM ID EŞLEŞTİRMESİ
    touchgfx::TypedTextId msgId = T_WARNLOCKED;

    switch (warning) {
        case WARN_SYSTEM_LOCKED:       msgId = T_WARNLOCKED; break;
        case WARN_NO_AMMO_SELECTED:    msgId = T_WARNNOAMMO; break;
        case WARN_GROUP_TOTALLY_EMPTY: msgId = T_WARNEMPTY; break;
        case WARN_WRONG_AMMO_IN_GROUP: msgId = T_WARNWRONGAMMO; break;
        case WARN_COMM_ERROR:          msgId = T_WARNCOMMERR; break;
        case WARN_COMM_LOST:           msgId = T_WARNFATALCOMM; break;
        case WARN_VOLTAGE_ERROR:       msgId = T_WARNVOLTAGE; break;

        // ATIŞ/MÜHİMMAT DURUMLARI
        case WARN_BLASTING_FAILED_SMOKE: msgId = T_WARNBLASTFAIL_SMOKE; break;
        case WARN_BLASTING_FAILED_FRAG:  msgId = T_WARNBLASTFAIL_FRAG; break;
        case WARN_MISFIRE_SMOKE:         msgId = T_WARNMISFIRE_SMOKE; break;
        case WARN_MISFIRE_FRAG:          msgId = T_WARNMISFIRE_FRAG; break;
        case WARN_FIRE_SUCCESS_SMOKE:    msgId = T_WARNSUCCESS_SMOKE; break;
        case WARN_FIRE_SUCCESS_FRAG:     msgId = T_WARNSUCCESS_FRAG; break;

        // İŞLEMCİ BİRİMİ HATALARI
        case WARN_PROC_RAM:       msgId = T_FLT_PROC_RAM; break;
        case WARN_PROC_NVRAM:     msgId = T_FLT_PROC_NVRAM; break;
        case WARN_PROC_MEMFILE:   msgId = T_FLT_PROC_MEMFILE; break;
        case WARN_PROC_NVSRAM:    msgId = T_FLT_PROC_NVSRAM; break;
        case WARN_PROC_MEMFULL:   msgId = T_FLT_PROC_MEMFULL; break;
        case WARN_PROC_SER1:      msgId = T_FLT_PROC_SER1; break;
        case WARN_PROC_SER2:      msgId = T_FLT_PROC_SER2; break;
        case WARN_PROC_SER3:      msgId = T_FLT_PROC_SER3; break;
        case WARN_PROC_SER4:      msgId = T_FLT_PROC_SER4; break;
        case WARN_PROC_IFACE:     msgId = T_FLT_PROC_IFACE; break;
        case WARN_PROC_PWR:       msgId = T_FLT_PROC_PWR; break;
        case WARN_PROC_SHUTDOWN:  msgId = T_FLT_PROC_SHUTDOWN; break;
        case WARN_PROC_PWR_SER:   msgId = T_FLT_PROC_PWR_SER; break;
        case WARN_PROC_TEMP:      msgId = T_FLT_PROC_TEMP; break;

        // SENSÖR HATALARI
        case WARN_SENS_B12:       msgId = T_FLT_SENS_B12; break;
        case WARN_SENS_B3_0:      msgId = T_FLT_SENS_B3_0; break;
        case WARN_SENS_B3_1:      msgId = T_FLT_SENS_B3_1; break;
        case WARN_SENS_B3_2:      msgId = T_FLT_SENS_B3_2; break;
        case WARN_SENS_CTRL:      msgId = T_FLT_SENS_CTRL; break;
        case WARN_SENS_SHUTDOWN:  msgId = T_FLT_SENS_SHUTDOWN; break;
        case WARN_SENS_PWR_SER:   msgId = T_FLT_SENS_PWR_SER; break;
        case WARN_SENS_PRESS:     msgId = T_FLT_SENS_PRESS; break;
        case WARN_SENS_V3_7:      msgId = T_FLT_SENS_V3_7; break;
        case WARN_SENS_V7_4:      msgId = T_FLT_SENS_V7_4; break;
        case WARN_SENS_V16:       msgId = T_FLT_SENS_V16; break;
        case WARN_SENS_V80:       msgId = T_FLT_SENS_V80; break;
        case WARN_SENS_VN7_4:     msgId = T_FLT_SENS_VN7_4; break;
        case WARN_SENS_VN3_7:     msgId = T_FLT_SENS_VN3_7; break;
        case WARN_SENS_PWR:       msgId = T_FLT_SENS_PWR; break;
        case WARN_SENS_TEMP_LIM:  msgId = T_FLT_SENS_TEMP_LIM; break;
        case WARN_SENS_TEMP_SNS:  msgId = T_FLT_SENS_TEMP_SNS; break;
    }

    // EKRAN HAFIZASINA KAYDET
    addUILog(LOG_EVENT_FAULT_OCCURRED, msgId, tubeIndex);

    // POPUP İÇİN TETİKLE
    if (warningsEnabled && modelListener != 0)
    {
        modelListener->onShowWarning(warning, tubeIndex);
    }
}

void Model::clearAllFaults()
{
  /*  for(int i = 0; i < 16; i++) {
        ariza_hafizasi[i] = false;
    }
    if (currentScreen == SCREEN_SMOKE && modelListener != 0) {
        modelListener->onSmokeDataUpdated(tube_states);
    }*/

    // Hafızayı siliyoruz ama AKTİF (o an devam eden) hatalar varsa hemen geri yüklüyoruz:
	storedSystemStatus = activeSystemStatus;
	stored_comm_lost_flag = comm_lost_flag;
	stored_hw_error_flag = hw_error_flag;

	if (currentScreen == SCREEN_FAULTS && modelListener != 0) {
		modelListener->onSystemStatusUpdated(activeSystemStatus);
	}
	addUILog(LOG_EVENT_CLEAR_ALL, T_LOG_CLEAR_ALL);
}

void Model::setBuzzerLevel(AlarmLevel_t level)
{
    // Buzzer sürücüsündeki AlarmLevel_t enum'una göre eşleştiriyoruz
    // 0: ALARM_NONE, 1: ALARM_LOW, 2: ALARM_MID, 3: ALARM_HIGH
    buzzerlevel = level;
}

// ==============================================================
// ANA DÖNGÜ (TICK)
// ==============================================================
void Model::tick()
{
    // 1. Butonları Kontrol Et
    processButtons();

    // 2. UART Verilerini Oku ve Ekranı Güncelle
    processSmokeScreen();

    // 3. Radar Güncellemeleri
    processRadarScreen();

    // 4. KARTIN HAZIR OLMA DURUMUNU (POWER READY) SÜREKLİ GÜNCELLE
    // Sistem atış yapmıyorsa ARM şalterinin durumunu doğrudan karşıya yansıtıyoruz
    if (currentFiringState == STATE_IDLE) {
        BMB_Set_Tx_Command(isSystemArmed(), 0, 0);
    }

    // 5. Ateşleme Durum Makinesi
    processFiringStateMachine();

    // Arka Plan Doğrulamalarını İşlet
	processBackgroundVerifications();

    // 6. Sürekli Cihaz İçi Test (CBIT)
    processCBIT();

    // buzzer
    /* --- Sesli ikaz --- */
    if (buzzerManualOverride) {
		Buzzer_SetAlarmLevel(buzzerlevel);     /* test ekrani */
	} else {
		processThreatAlarm();                  /* buzzer'i kendisi surer */
	}

    processTimeDateContainer();

    processWarnings();


}

void Model::processThreatAlarm()
{
    uint32_t now = HAL_GetTick();

    /* --- 1. TAZELIK: TLUS tehdit akisi kesildi mi? --- */
    if ((now - lastThreatMsgMs) > THREAT_STALE_TIMEOUT_MS)
    {
        if (threatDataFresh)          /* yeni kopus - bir kere tetikle */
        {
            threatDataFresh   = false;
            tehdidler.isValid = false;
            tehdidler.count   = 0;
            tlus_comm_lost_flag = true;

            dispatchWarning(WARN_COMM_LOST);

            if (modelListener != 0 && currentScreen == SCREEN_RADAR) {
                modelListener->onRadarTargetsReceived(tehdidler);  /* radari temizle */
            }
        }

        /* Veri yokken tehdit alarmi CALMAZ; bunun yerine gorsel
         * "ILETISIM KOPTU" hatasi aktif kalir. Bayat veriyle
         * otmek, otmemekten daha tehlikeli. */
        threatAlarmLevel = ALARM_NONE;
        lastBurstMs      = 0U;
		Buzzer_Stop();
        return;
    }

    if (!threatDataFresh) { threatDataFresh = true; tlus_comm_lost_flag = false; }

    /* --- 2. EN YUKSEK SIDDETLI AKTIF TEHDIT --- */
	ThreatResponse worst = { ALARM_NONE, false, 0U };
	bool sawAgedOut = false;

	if (tehdidler.isValid)
	{
		uint8_t n = (tehdidler.count > 20U) ? 20U : tehdidler.count;
		for (uint8_t i = 0U; i < n; i++)
		{
			if (tehdidler.threats[i].ageOut != 0U) { sawAgedOut = true; continue; }

			ThreatResponse r = classResponse(tehdidler.threats[i].threatClass);
			if (r.level > worst.level) worst = r;    /* EN KOTUSU KAZANIR */
		}
	}

	/* --- 3. AKTIF TEHDIT YOKSA SUSTUR --- */
	if (worst.level == ALARM_NONE)
	{
		if (sawAgedOut || (int32_t)(now - alarmHoldUntilMs) >= 0)
		{
			threatAlarmLevel = ALARM_NONE;
			alarmHoldUntilMs = now;
			lastBurstMs      = 0U;
			Buzzer_Stop();
		}
		return;
	}

	/* --- 4. SEVIYE DEGISIMI --- */
	bool levelChanged = false;

	if (worst.level > threatAlarmLevel)
	{
		levelChanged = true;                    /* tirmanma: aninda */
	}
	else if (worst.level < threatAlarmLevel)
	{
		/* Mevcut seviye artik hicbir aktif tehditle dogrulanmiyor.
		 * Hold'u YENILEME - dolmasini bekle, sonra in. */
		if ((int32_t)(now - alarmHoldUntilMs) >= 0) {
			levelChanged = true;
		}
	}
	else
	{
		/* Ayni seviye devam ediyor - hold'u tazele */
		alarmHoldUntilMs = now + ALARM_MIN_HOLD_MS;
	}

	if (levelChanged) {
		threatAlarmLevel = worst.level;
		alarmHoldUntilMs = now + ALARM_MIN_HOLD_MS;
		lastBurstMs      = 0U;
		Buzzer_Stop();
	}

	/* --- 5. KARSI TEDBIR ONAYI ---
	 * Operator sis/frag attiysa uyariyi almis ve aksiyon almistir.
	 * Surekli alarmi gecici olarak hatirlatmaya dusur. Sure dolunca
	 * tehdit hala varsa surekliye DONER: "duman attim ama hala
	 * uzerimdeler" ilk uyaridan daha kritik bir bilgidir. */
	bool cmAck = (lastCounterMeasureMs != 0U) &&
				 ((now - lastCounterMeasureMs) < CM_ACK_WINDOW_MS);

	/* --- 6. CALDIR --- */
	if (worst.continuous && !cmAck)
	{
		if (!Buzzer_IsPlaying()) {
			Buzzer_Play(worst.level, 0U);          /* sonsuz dongu */
		}
	}
	else
	{
		uint32_t interval = worst.continuous ? CM_ACK_REMINDER_MS : worst.reminderMs;

		if (lastBurstMs == 0U || (now - lastBurstMs) >= interval) {
			Buzzer_Play(worst.level, 0U);
			lastBurstMs = now;
		}
	}
}

void Model::setActiveScreen(ActiveScreenType screen)
{
    currentScreen = screen;
    if (screen == SCREEN_SMOKE && modelListener != 0)
    {
        modelListener->onSmokeDataUpdated(tube_states);
    }
    else if (screen == SCREEN_RADAR && modelListener != 0) {
	   modelListener->onRadarTargetsReceived(tehdidler);   /* taze veriyle senkronla */
   }
}

// ==============================================================
// BUTON İŞLEYİCİ
// ==============================================================
void Model::processButtons()
{
    uint32_t current_raw_state = HC165_Get_Button_State();
    uint32_t pressed_buttons = (current_raw_state ^ last_raw_button_state) & current_raw_state;

    // Arayüz (View) Güncellemeleri ve LED Kontrolleri
    if (current_raw_state != last_raw_button_state)
    {

    	uint8_t h = systemTime.isValid ? systemTime.hour : 0;
		uint8_t m = systemTime.isValid ? systemTime.minute : 0;
		uint8_t s = systemTime.isValid ? systemTime.second : 0;

		// --- 1. ATIŞ KİLİDİ (ARMED) DEĞİŞİMİ KONTROLÜ ---
		bool wasArmed = (last_raw_button_state & KOM2_UP) != 0;
		bool isArmed = (current_raw_state & KOM2_UP) != 0;
		if (wasArmed != isArmed) {
			// Data Format: [1]: 0x01 (Silah Kilidi Tipi), [2]: Durum (1=Açık, 0=Kilitli)
			CAN_Log_Add(LOG_SYS_STATE, 0x01, isArmed ? 1 : 0, h, m, s, 0, 0);

			// İleride UI tarafına metin basmak için (Örn: T_LOG_ARMED veya T_LOG_LOCKED eklenince)
			addUILog(LOG_EVENT_SYSTEM_STATE, isArmed ? T_LOG_SYS_ARMED : T_LOG_SYS_LOCKED);
		}

		// --- 2. SİS MÜHİMMATI SEÇİM DEĞİŞİMİ ---
		bool wasSmoke = (last_raw_button_state & KOM1_DOWN) != 0;
		bool isSmoke = (current_raw_state & KOM1_DOWN) != 0;
		if (!wasSmoke && isSmoke) {
			// Data Format: [1]: 0x02 (Mühimmat Seçim Tipi), [2]: 0x01 (Sis Seçildi)
			CAN_Log_Add(LOG_SYS_STATE, 0x02, 0x01, h, m, s, 0, 0);
			addUILog(LOG_EVENT_SYSTEM_STATE, T_LOG_AMMO_SMOKE);
		}

		// --- 3. TAHRİP (FRAG) MÜHİMMATI SEÇİM DEĞİŞİMİ ---
		bool wasFrag = (last_raw_button_state & KOM1_UP) != 0;
		bool isFrag = (current_raw_state & KOM1_UP) != 0;
		if (!wasFrag && isFrag) {
			// Data Format: [1]: 0x02 (Mühimmat Seçim Tipi), [2]: 0x02 (Tahrip Seçildi)
			CAN_Log_Add(LOG_SYS_STATE, 0x02, 0x02, h, m, s, 0, 0);
			addUILog(LOG_EVENT_SYSTEM_STATE, T_LOG_AMMO_FRAG);
		}

        if(modelListener) {
            modelListener->onHardwareButtonStateChanged(current_raw_state);
        }

        if (currentLedTestMode == 0)
		{
			if (isSystemArmed()) {
				LP5036_SetLedState(19, LP5036_LED_GREEN);
				LP5036_SetLedState(18, LP5036_LED_OFF);
			} else {
				LP5036_SetLedState(19, LP5036_LED_OFF);
				LP5036_SetLedState(18, LP5036_LED_RED);
			}

			if (isSmokeSelected()) {
				LP5036_SetLedState(17, LP5036_LED_GREEN);
				LP5036_SetLedState(16, LP5036_LED_OFF);
			} else if (isFragSelected()) {
				LP5036_SetLedState(17, LP5036_LED_OFF);
				LP5036_SetLedState(16, LP5036_LED_ORANGE);
			} else {
				LP5036_SetLedState(17, LP5036_LED_OFF);
				LP5036_SetLedState(16, LP5036_LED_OFF);
			}
		}
    }

    // Tuşlara Basılma (Edge Detection)
    if (pressed_buttons != 0)
    {
    	if (isTestModeActive)
		{
			if ((pressed_buttons & BTN_BRIGHTNESS_MENU) && !isBlackoutMode) { if(modelListener) modelListener->onBrightnessMenuPressed(); } // ÇIKIŞ (BACK)
			if ((pressed_buttons & BTN_VOLUME_MENU)     && !isBlackoutMode) { if(modelListener) modelListener->onVolumeMenuPressed(); }     // TEKRAR (ENTER)
			// DİĞER HİÇBİR TUŞ İŞLEM YAPMAZ!
		}
		else
		{
			if (pressed_buttons & BTN_BLACKOUT) {
				isBlackoutMode = !isBlackoutMode;
				if (isBlackoutMode) {
					Set_LCD_Brightness(0);
					LP5036_AllLedsOff();
				} else {
					Set_LCD_Brightness(savedBrightnessPWM);
					LP5036_AllLedsOn();
				}
				if(modelListener) modelListener->onBlackoutStateChanged(isBlackoutMode);
			}

			if ((pressed_buttons & BTN_MENU)            && !isBlackoutMode) { if(modelListener) modelListener->onMenuPressed(); }
			if ((pressed_buttons & BTN_VOLUME_MENU)     && !isBlackoutMode) { if(modelListener) modelListener->onVolumeMenuPressed(); }
			if ((pressed_buttons & BTN_BRIGHTNESS_MENU) && !isBlackoutMode) { if(modelListener) modelListener->onBrightnessMenuPressed(); }
			if (pressed_buttons & BTN_UP)   { if(modelListener) modelListener->onUpPressed(); }
			if (pressed_buttons & BTN_DOWN) { if(modelListener) modelListener->onDownPressed(); }

			if (pressed_buttons & BTN_HOME) {
				if(modelListener) modelListener->onHomePressed();

			}

			if (pressed_buttons & BTN_SMOKE) {
				if(modelListener) modelListener->onSmokePressed();
			}

			// ==============================================================
			// ATIŞ BUTONLARI
			// ŞART: Atış Makinesi Boşta + İletişim Var + HW Hatası Yok
			// ==============================================================
			if (currentFiringState == STATE_IDLE && comm_lost_flag == false && hw_error_flag == false)
			{

				if (pressed_buttons & BTN_FRNT_LEFT)     { addUILog(LOG_EVENT_FIRE, T_LOG_FIRE_BTN, 1); fireGroup(0, 3); }
				if (pressed_buttons & BTN_FRNT_RIGHT)    { addUILog(LOG_EVENT_FIRE, T_LOG_FIRE_BTN, 2); fireGroup(8, 11); }
				if (pressed_buttons & BTN_REAR_LEFT)     { addUILog(LOG_EVENT_FIRE, T_LOG_FIRE_BTN, 3); fireGroup(4, 7); }
				if (pressed_buttons & BTN_REAR_RIGHT)    { addUILog(LOG_EVENT_FIRE, T_LOG_FIRE_BTN, 4); fireGroup(12, 15); }
				if (pressed_buttons & BTN_FRNT_REAR_ALL) { addUILog(LOG_EVENT_FIRE, T_LOG_FIRE_BTN, 5); fireSalvoAll(); }
			}
		}
    }

    last_raw_button_state = current_raw_state;
}

// ==============================================================
// GÜNCELLEME İŞLEYİCİLERİ (UART & RADAR)
// ==============================================================
void Model::processSmokeScreen()
{
    if (BMB_Check_New_Data() == true)
    {

        BMB_RxPacket_t rxData;
        BMB_Get_Latest_Rx_Data(&rxData);


        // ==============================================================
                // BMB BLASTING_STATE DÖNÜŞ SÜRESİ ÖLÇÜMÜ (DEBUG)
                // ==============================================================
                // YÜKSELEN KENAR: 0x00'dan 0xAA'ya geçtiği anı yakala ve süreyi başlat
                if (rxData.blasting_state == 0xAA && last_blasting_state != 0xAA) {
                    blast_start_time = HAL_GetTick();
                    is_measuring_blast = true;
                }
                // DÜŞEN KENAR: 0xAA'dan tekrar 0x00'a (veya başka bir şeye) düştüğü anı yakala
                else if (rxData.blasting_state != 0xAA && is_measuring_blast == true) {
                    measured_blast_duration = HAL_GetTick() - blast_start_time;
                    is_measuring_blast = false;

                    // İSTEĞE BAĞLI: Süreyi görmek için CAN Bus'a basabilirsin
                    // Örn: CAN_Log_Add(LOG_DEBUG_INFO, (uint8_t)measured_blast_duration, ...);
                }

                // Bir sonraki turda karşılaştırmak için mevcut durumu kaydet
                last_blasting_state = rxData.blasting_state;
                // =


        bool ui_needs_update = false;

        for(int i = 0; i < 16; i++)
        {
            // MASKELEME: Atış yapılan tüp sensör körlüğüne karşı dondurulur
        	if (currentFiringState != STATE_IDLE && (currentFiringMask & (1 << i))) {
                continue;
            }

            TubeState hesaplanan_durum = TUBE_EMPTY;

            if (ariza_hafizasi[i] == true)
            {
                // Akıllı Arıza Silme (Fiziksel Çıkarılma Durumu)
                if (rxData.sis_status[i] == 0x55 && rxData.frag_status[i] == 0x55) {
                    ariza_hafizasi[i] = false;
                    hesaplanan_durum = TUBE_EMPTY;
                } else {
                    hesaplanan_durum = TUBE_FAULT;
                }
            }
            else if (rxData.sis_status[i] == 0xAA) {
                hesaplanan_durum = TUBE_SMOKE;
            }
            else if (rxData.frag_status[i] == 0xAA) {
                hesaplanan_durum = TUBE_FRAG;
            }

            // Arayüz ve Donanım LED Güncellemesi
            if (tube_states[i] != hesaplanan_durum)
            {
                tube_states[i] = hesaplanan_durum;
                ui_needs_update = true;

                LP5036_LedState_t fiziksel_led_renk;
                switch (hesaplanan_durum)
                {
                    case TUBE_SMOKE: fiziksel_led_renk = LP5036_LED_GREEN;  break;
                    case TUBE_FRAG:  fiziksel_led_renk = LP5036_LED_ORANGE; break;
                    case TUBE_FAULT: fiziksel_led_renk = LP5036_LED_RED;    break;
                    case TUBE_EMPTY:
                    default:         fiziksel_led_renk = LP5036_LED_OFF;    break;
                }
                LP5036_SetLedState(i, fiziksel_led_renk);
            }
        }

        if (ui_needs_update == true)
		{
			// Arayüzü (Ekrandaki tüp grafiklerini) güncelle
			if (modelListener != 0 && currentScreen == SCREEN_SMOKE) {
				modelListener->onSmokeDataUpdated(tube_states);
			}
			icdManager.sendTupDurumu(reinterpret_cast<const uint8_t*>(tube_states));
		}

    }
}

void Model::processRadarScreen()
{
	// Radar zamanlayıcıları
	uint32_t currentTime = HAL_GetTick();
	uint32_t deltaTime = currentTime - lastModelTickTime;
	lastModelTickTime = currentTime;

	for (int i = 0; i < MAX_TARGETS; i++)
	{
		if (fadeTicksRemaining[i] > 0)
		{
			fadeTicksRemaining[i] -= (int)deltaTime;
			if (fadeTicksRemaining[i] < 0) {
				fadeTicksRemaining[i] = 0;
			}
		}
	}

	if (threatsUpdated == true)
	{
		threatsUpdated = false; // İşleme aldık, bayrağı indir.

		// Arayüz Radardaysa ve paket Geçerliyse aktar
		if (currentScreen == SCREEN_RADAR && modelListener != 0) {
			if (tehdidler.isValid) {
				modelListener->onRadarTargetsReceived(tehdidler);
			}
		}
	}

}

void Model::processTimeDateContainer()
{
	// Fonksiyon her çağrıldığında hafızada kalacak statik sayacımız
	static uint32_t last_rtc_read_time = 0;
	static bool first_read = true;
	uint32_t current_time = HAL_GetTick();

	// RTC DONANIMINA SADECE 1 SANİYEDE (1000 ms) BİR SOR!
	if (current_time - last_rtc_read_time >= 1000 || timeUpdated == true || first_read)
	{
		first_read = false;
		last_rtc_read_time = current_time;
		timeUpdated = false;

		uint8_t h, m, s, d, mo;
		uint16_t y;

		// Donanımdaki güncel zamanı oku
		Read_RTC_Time(&h, &m, &s, &d, &mo, &y);

		// Arka planda saati sürekli taze tut (Log saniyeleri doğru çıksın diye)
		systemTime.hour = h;
		systemTime.minute = m;
		systemTime.second = s;
		systemTime.day = d;
		systemTime.month = mo;
		systemTime.year = y;
		systemTime.isValid = true;

		// Eğer okunan "dakika" bizim ekrandaki dakikadan farklıysa
		if (m != last_minute)
		{
			last_minute = m; // Yeni dakikayı kaydet ki sürekli ekranı tetiklemesin

			if (modelListener != 0) {
				modelListener->onTimeDateUpdated(systemTime);
			}
		}
	}
}

void Model::processWarnings()
{
    if (faultUpdate)
    {
        faultUpdate = false; // Bayrağı hemen indir

        uint16_t now = (systemTime.hour << 8) | systemTime.minute;

        // 1. İŞLEMCİ BİRİMİ HATALARI (latestSystemStatus ile storedSystemStatus karşılaştırılır)
        if (latestSystemStatus.processorFaults.ramTesti && !storedSystemStatus.processorFaults.ramTesti) {
            dispatchWarning(WARN_PROC_RAM); procFaultTime[0] = now; storedSystemStatus.processorFaults.ramTesti = true; }
        if (latestSystemStatus.processorFaults.kaliciBellekTesti && !storedSystemStatus.processorFaults.kaliciBellekTesti) {
            dispatchWarning(WARN_PROC_NVRAM); procFaultTime[1] = now; storedSystemStatus.processorFaults.kaliciBellekTesti = true; }
        if (latestSystemStatus.processorFaults.bellekDosyasiTesti && !storedSystemStatus.processorFaults.bellekDosyasiTesti) {
            dispatchWarning(WARN_PROC_MEMFILE); procFaultTime[2] = now; storedSystemStatus.processorFaults.bellekDosyasiTesti = true; }
        if (latestSystemStatus.processorFaults.nvsramTesti && !storedSystemStatus.processorFaults.nvsramTesti) {
            dispatchWarning(WARN_PROC_NVSRAM); procFaultTime[3] = now; storedSystemStatus.processorFaults.nvsramTesti = true; }
        if (latestSystemStatus.processorFaults.bellekDoluluk && !storedSystemStatus.processorFaults.bellekDoluluk) {
            dispatchWarning(WARN_PROC_MEMFULL); procFaultTime[4] = now; storedSystemStatus.processorFaults.bellekDoluluk = true; }
        if (latestSystemStatus.processorFaults.seriKanal1 && !storedSystemStatus.processorFaults.seriKanal1) {
            dispatchWarning(WARN_PROC_SER1); procFaultTime[5] = now; storedSystemStatus.processorFaults.seriKanal1 = true; }
        if (latestSystemStatus.processorFaults.seriKanal2 && !storedSystemStatus.processorFaults.seriKanal2) {
            dispatchWarning(WARN_PROC_SER2); procFaultTime[6] = now; storedSystemStatus.processorFaults.seriKanal2 = true; }
        if (latestSystemStatus.processorFaults.seriKanal3 && !storedSystemStatus.processorFaults.seriKanal3) {
            dispatchWarning(WARN_PROC_SER3); procFaultTime[7] = now; storedSystemStatus.processorFaults.seriKanal3 = true; }
        if (latestSystemStatus.processorFaults.seriKanal4 && !storedSystemStatus.processorFaults.seriKanal4) {
            dispatchWarning(WARN_PROC_SER4); procFaultTime[8] = now; storedSystemStatus.processorFaults.seriKanal4 = true; }
        if (latestSystemStatus.processorFaults.arayuzKarti && !storedSystemStatus.processorFaults.arayuzKarti) {
            dispatchWarning(WARN_PROC_IFACE); procFaultTime[9] = now; storedSystemStatus.processorFaults.arayuzKarti = true; }
        if (latestSystemStatus.processorFaults.anaBesleme && !storedSystemStatus.processorFaults.anaBesleme) {
            dispatchWarning(WARN_PROC_PWR); procFaultTime[10] = now; storedSystemStatus.processorFaults.anaBesleme = true; }
        if (latestSystemStatus.processorFaults.islemciDurumuKapanma && !storedSystemStatus.processorFaults.islemciDurumuKapanma) {
            dispatchWarning(WARN_PROC_SHUTDOWN); procFaultTime[11] = now; storedSystemStatus.processorFaults.islemciDurumuKapanma = true; }
        if (latestSystemStatus.processorFaults.gucKartiSeriKanal && !storedSystemStatus.processorFaults.gucKartiSeriKanal) {
            dispatchWarning(WARN_PROC_PWR_SER); procFaultTime[12] = now; storedSystemStatus.processorFaults.gucKartiSeriKanal = true; }
        if (latestSystemStatus.processorFaults.sicaklikEsikAsimi && !storedSystemStatus.processorFaults.sicaklikEsikAsimi) {
            dispatchWarning(WARN_PROC_TEMP); procFaultTime[13] = now; storedSystemStatus.processorFaults.sicaklikEsikAsimi = true; }

        // 2. SENSÖR BİRİMİ HATALARI
        for (int i = 0; i < 4; i++) {
            int sNo = i + 1; // 1,2,3,4
            if (latestSystemStatus.sensorFaults[i].bant_I_II_Karti && !storedSystemStatus.sensorFaults[i].bant_I_II_Karti) {
                dispatchWarning(WARN_SENS_B12, sNo); sensFaultTime[i][0] = now; storedSystemStatus.sensorFaults[i].bant_I_II_Karti = true; }
            if (latestSystemStatus.sensorFaults[i].bant_III_Sensor0 && !storedSystemStatus.sensorFaults[i].bant_III_Sensor0) {
                dispatchWarning(WARN_SENS_B3_0, sNo); sensFaultTime[i][1] = now; storedSystemStatus.sensorFaults[i].bant_III_Sensor0 = true; }
            if (latestSystemStatus.sensorFaults[i].bant_III_Karti_Sensor1 && !storedSystemStatus.sensorFaults[i].bant_III_Karti_Sensor1) {
                dispatchWarning(WARN_SENS_B3_1, sNo); sensFaultTime[i][2] = now; storedSystemStatus.sensorFaults[i].bant_III_Karti_Sensor1 = true; }
            if (latestSystemStatus.sensorFaults[i].bant_III_Karti_Sensor2 && !storedSystemStatus.sensorFaults[i].bant_III_Karti_Sensor2) {
                dispatchWarning(WARN_SENS_B3_2, sNo); sensFaultTime[i][3] = now; storedSystemStatus.sensorFaults[i].bant_III_Karti_Sensor2 = true; }
            if (latestSystemStatus.sensorFaults[i].sensor_Birimi_Kontrol_Karti && !storedSystemStatus.sensorFaults[i].sensor_Birimi_Kontrol_Karti) {
                dispatchWarning(WARN_SENS_CTRL, sNo); sensFaultTime[i][4] = now; storedSystemStatus.sensorFaults[i].sensor_Birimi_Kontrol_Karti = true; }
            if (latestSystemStatus.sensorFaults[i].kontrollu_Kapanma && !storedSystemStatus.sensorFaults[i].kontrollu_Kapanma) {
                dispatchWarning(WARN_SENS_SHUTDOWN, sNo); sensFaultTime[i][5] = now; storedSystemStatus.sensorFaults[i].kontrollu_Kapanma = true; }
            if (latestSystemStatus.sensorFaults[i].guc_Karti_Seri_Kanal && !storedSystemStatus.sensorFaults[i].guc_Karti_Seri_Kanal) {
                dispatchWarning(WARN_SENS_PWR_SER, sNo); sensFaultTime[i][6] = now; storedSystemStatus.sensorFaults[i].guc_Karti_Seri_Kanal = true; }
            if (latestSystemStatus.sensorFaults[i].basinc_Durumu && !storedSystemStatus.sensorFaults[i].basinc_Durumu) {
                dispatchWarning(WARN_SENS_PRESS, sNo); sensFaultTime[i][7] = now; storedSystemStatus.sensorFaults[i].basinc_Durumu = true; }
            if (latestSystemStatus.sensorFaults[i].volt_3_7V && !storedSystemStatus.sensorFaults[i].volt_3_7V) {
                dispatchWarning(WARN_SENS_V3_7, sNo); sensFaultTime[i][8] = now; storedSystemStatus.sensorFaults[i].volt_3_7V = true; }
            if (latestSystemStatus.sensorFaults[i].volt_7_4V && !storedSystemStatus.sensorFaults[i].volt_7_4V) {
                dispatchWarning(WARN_SENS_V7_4, sNo); sensFaultTime[i][9] = now; storedSystemStatus.sensorFaults[i].volt_7_4V = true; }
            if (latestSystemStatus.sensorFaults[i].volt_16V && !storedSystemStatus.sensorFaults[i].volt_16V) {
                dispatchWarning(WARN_SENS_V16, sNo); sensFaultTime[i][10] = now; storedSystemStatus.sensorFaults[i].volt_16V = true; }
            if (latestSystemStatus.sensorFaults[i].volt_80V && !storedSystemStatus.sensorFaults[i].volt_80V) {
                dispatchWarning(WARN_SENS_V80, sNo); sensFaultTime[i][11] = now; storedSystemStatus.sensorFaults[i].volt_80V = true; }
            if (latestSystemStatus.sensorFaults[i].volt_neg7_4V && !storedSystemStatus.sensorFaults[i].volt_neg7_4V) {
                dispatchWarning(WARN_SENS_VN7_4, sNo); sensFaultTime[i][12] = now; storedSystemStatus.sensorFaults[i].volt_neg7_4V = true; }
            if (latestSystemStatus.sensorFaults[i].volt_neg3_7V && !storedSystemStatus.sensorFaults[i].volt_neg3_7V) {
                dispatchWarning(WARN_SENS_VN3_7, sNo); sensFaultTime[i][13] = now; storedSystemStatus.sensorFaults[i].volt_neg3_7V = true; }
            if (latestSystemStatus.sensorFaults[i].ana_Besleme && !storedSystemStatus.sensorFaults[i].ana_Besleme) {
                dispatchWarning(WARN_SENS_PWR, sNo); sensFaultTime[i][14] = now; storedSystemStatus.sensorFaults[i].ana_Besleme = true; }
            if (latestSystemStatus.sensorFaults[i].sicaklik_Durumu && !storedSystemStatus.sensorFaults[i].sicaklik_Durumu) {
                dispatchWarning(WARN_SENS_TEMP_LIM, sNo); sensFaultTime[i][15] = now; storedSystemStatus.sensorFaults[i].sicaklik_Durumu = true; }
            if (latestSystemStatus.sensorFaults[i].sicaklik_Sensoru && !storedSystemStatus.sensorFaults[i].sicaklik_Sensoru) {
                dispatchWarning(WARN_SENS_TEMP_SNS, sNo); sensFaultTime[i][16] = now; storedSystemStatus.sensorFaults[i].sicaklik_Sensoru = true; }
        }

        // Ticker (Alt yazı) için o anki fiziksel (aktif) durumu sakla
        activeSystemStatus = latestSystemStatus;

        // Ekran 4 açıkken listeyi yenilemesi için Presenter'a haber ver
        if (modelListener != 0) {
            modelListener->onSystemStatusUpdated(activeSystemStatus);
        }
    }
}

// ==============================================================
// ATIŞ VE SİLAH YÖNETİMİ
// ==============================================================
int Model::findNextAvailableTube(int startIdx, int endIdx, bool isFrag)
{
	BMB_RxPacket_t rxData;
	BMB_Get_Latest_Rx_Data(&rxData);

	for (int i = startIdx; i <= endIdx; i++)
	{
		if (ariza_hafizasi[i] == true) continue;

		// YENİ: Bu tüp şu an arka planda doğrulama bekliyor mu?
		bool isPendingVerification = false;
		for (int q = 0; q < 16; q++) {
			if (verifQueue[q].active && (verifQueue[q].tubeMask & (1 << i))) {
				isPendingVerification = true;
				break;
			}
		}
		if (isPendingVerification) continue; // Doğrulama bekleyen tüpü atla, sıradakine geç!

		if (isFrag) {
			if (rxData.frag_status[i] == 0xAA) return i;
		} else {
			if (rxData.sis_status[i] == 0xAA) return i;
		}
	}
	return -1;
}

void Model::fireGroup(int startIdx, int endIdx)
{
    if (currentFiringState != STATE_IDLE) return;

    if (comm_lost_flag == true) {
        dispatchWarning(WARN_COMM_LOST);
        return;
    }

    if (hw_error_flag == true) {
        dispatchWarning(WARN_VOLTAGE_ERROR);
        return;
    }

    if (isSystemArmed() == false) {
        dispatchWarning(WARN_SYSTEM_LOCKED);
        return;
    }

    bool isFrag = isFragSelected();
    bool isSmoke = isSmokeSelected();

    if (!isFrag && !isSmoke) {
        dispatchWarning(WARN_NO_AMMO_SELECTED);
        return;
    }

    int targetIdx = findNextAvailableTube(startIdx, endIdx, isFrag);

    if (targetIdx == -1)
    {
    	// İstenen yok. Peki diğer tipten (zıt mühimmattan) var mı?
		int oppositeIdx = findNextAvailableTube(startIdx, endIdx, !isFrag);

		if (oppositeIdx == -1) {
			// Zıt mühimmat da yok! Bölge tamamen boş!
			dispatchWarning(WARN_GROUP_TOTALLY_EMPTY, startIdx + 1);
		} else {
			// Kutu boş değil ama bizim istediğimiz mühimmattan yok!
			dispatchWarning(WARN_WRONG_AMMO_IN_GROUP, startIdx + 1);
		}
		return;
    }

    targetIsFrag = isFrag;
	currentFiringMask = (1 << targetIdx); // Sadece o tüpü maskeye ekle
	isSalvoMode = false; // Bu tekli atış

	currentFiringState = STATE_WAIT_READY;
	firingTimer = HAL_GetTick();
}

void Model::fireSalvoAll()
{
    if (currentFiringState != STATE_IDLE) return;

    if (comm_lost_flag == true) {
        dispatchWarning(WARN_COMM_LOST);
        return;
    }

    if (hw_error_flag == true) {
        dispatchWarning(WARN_VOLTAGE_ERROR);
        return;
    }

    if (isSystemArmed() == false) {
        dispatchWarning(WARN_SYSTEM_LOCKED);
        return;
    }

    bool isFrag = isFragSelected();
    bool isSmoke = isSmokeSelected();

    if (!isFrag && !isSmoke) {
        dispatchWarning(WARN_NO_AMMO_SELECTED);
        return;
    }

    // 4 KÖŞEYİ AYNI ANDA TARA VE MASKELERİNİ ÇIKAR
	currentFiringMask = 0;
	for(int i = 0; i < 4; i++) {
		int idx = findNextAvailableTube(salvoGroups[i][0], salvoGroups[i][1], isFrag);
		if (idx != -1) {
			currentFiringMask |= (1 << idx); // Bulunanı maskeye ekle!
		}
	}

	if (currentFiringMask == 0) {
		bool anyOppositeAmmo = false;

		// Sistemi diğer mühimmat için tara
		for(int i = 0; i < 4; i++) {
			if (findNextAvailableTube(salvoGroups[i][0], salvoGroups[i][1], !isFrag) != -1) {
				anyOppositeAmmo = true;
				break;
			}
		}

		if (anyOppositeAmmo) {
			dispatchWarning(WARN_WRONG_AMMO_IN_GROUP);
		} else {
			dispatchWarning(WARN_GROUP_TOTALLY_EMPTY);
		}
		return;
	}

	targetIsFrag = isFrag;
	isSalvoMode = true; // Salvo döngüsü devrede

	currentFiringState = STATE_WAIT_READY;
	firingTimer = HAL_GetTick();
}

// ==============================================================
// ATIŞ DURUM MAKİNESİ (STATE MACHINE)
// ==============================================================
void Model::processFiringStateMachine()
{
	if (currentFiringState == STATE_IDLE) return;

	uint32_t current_time = HAL_GetTick();
	BMB_RxPacket_t rxData;
	BMB_Get_Latest_Rx_Data(&rxData);

	switch(currentFiringState)
	{
		case STATE_WAIT_READY:
			if (!isSystemArmed()) {
				currentFiringState = STATE_IDLE;
				dispatchWarning(WARN_SYSTEM_LOCKED);
				break;
			}

			if (rxData.bmb_ready == 0xAA)
			{
				currentFiringState = STATE_FIRING;
				lastCounterMeasureMs = HAL_GetTick();
				firingTimer = current_time;
				any_blasting_occurred = false;

				if (targetIsFrag) BMB_Set_Tx_Command(isSystemArmed(), 0, currentFiringMask);
				else              BMB_Set_Tx_Command(isSystemArmed(), currentFiringMask, 0);
			}
			else if (current_time - firingTimer > 2000)
			{
				currentFiringState = STATE_IDLE;
				dispatchWarning(WARN_COMM_ERROR);
			}
			break;

		case STATE_FIRING:
			if (!isSystemArmed()) {
				currentFiringState = STATE_IDLE;
				BMB_Set_Tx_Command(false, 0, 0);
				dispatchWarning(WARN_SYSTEM_LOCKED);
				break;
			}

			if (rxData.blasting_state == 0xAA) {
				any_blasting_occurred = true;
			}

			// TAM 30 MS BOYUNCA ATEŞLE
			if (current_time - firingTimer >= 30)
			{
				// 1. Akımı kes
				BMB_Set_Tx_Command(isSystemArmed(), 0, 0);

				// 2. Doğrulamayı arka plan kuyruğuna (Verification Queue) ekle
				for (int q = 0; q < 16; q++) {
					if (!verifQueue[q].active) {
						verifQueue[q].tubeMask = currentFiringMask;
						verifQueue[q].timestamp = current_time;
						verifQueue[q].isFrag = targetIsFrag;
						verifQueue[q].blast_ok = any_blasting_occurred;
						verifQueue[q].active = true;
						break;
					}
				}

				// 3. YENİ: IDLE YERİNE SOĞUMA (DONANIM SIFIRLANMA) DURUMUNA GEÇ
				currentFiringState = STATE_WAIT_COOLDOWN;
				firingTimer = current_time;
			}
			break;

		case STATE_WAIT_COOLDOWN:
			// 1. ÖNCE ZAMAN AŞIMI (TIMEOUT) KONTROLÜ
			// Eğer 1000 ms (1 saniye) geçmesine rağmen hala 0x00 gelmediyse BMB kilitlenmiş demektir!
			if (current_time - firingTimer > 1000)
			{
				// Sistemi kilitli kalmaktan kurtar ve güvenli moda (IDLE) al
				isSalvoMode = false;
				currentFiringState = STATE_IDLE;

				// OPERATÖRE KESİNLİKLE UYARI VER! (Örn: İletişim Hatası veya Donanım Hatası)
				dispatchWarning(WARN_COMM_ERROR); // Veya sisteminde varsa WARN_VOLTAGE_ERROR
			}

			// 2. EĞER TIMEOUT OLMADIYSA VE ZAMANINDA 0x00 GELDİYSE (NORMAL İŞLEYİŞ)
			else if (rxData.blasting_state == 0x00 && (current_time - firingTimer > 50))
			{
				// ==============================================================
				// BMB BAŞARIYLA 0x00'A DÖNDÜ! ŞİMDİ SALVO SÜREKLİLİĞİNİ KONTROL ET
				// ==============================================================
				if (isSalvoMode && currentSalvoMode == SALVO_FULL_EMPTY)
				{
					// Sıradaki atılacak 4'lüyü bul
					currentFiringMask = 0;
					for(int i = 0; i < 4; i++) {
						int idx = findNextAvailableTube(salvoGroups[i][0], salvoGroups[i][1], targetIsFrag);
						if (idx != -1) currentFiringMask |= (1 << idx);
					}

					if (currentFiringMask != 0) {
						// Yeni tüpler bulundu, beklemeden bir sonraki atışa geç!
						currentFiringState = STATE_WAIT_READY;
						firingTimer = current_time;
					} else {
						// Tank tamamen boşaldı, sistemi serbest bırak!
						isSalvoMode = false;
						currentFiringState = STATE_IDLE;
					}
				}
				else
				{
					// Salvo değilse veya "Tek Tur" ise sistemi serbest bırak (Yeni butona basılabilir)
					isSalvoMode = false;
					currentFiringState = STATE_IDLE;
				}
			}
			break;

		default:
			currentFiringState = STATE_IDLE;
			break;
	}
}


// Arka planda 1.5 saniyesi dolan atışları kontrol eder
void Model::processBackgroundVerifications()
{
    uint32_t current_time = HAL_GetTick();
    BMB_RxPacket_t rxData;
    BMB_Get_Latest_Rx_Data(&rxData);

    for (int q = 0; q < 16; q++)
    {
        // Görev aktifse ve atışın üzerinden tam 1.5 saniye (1500ms) geçmişse
        if (verifQueue[q].active && (current_time - verifQueue[q].timestamp >= 1500))
        {
            bool anyMisfire = false;

            for (int i = 0; i < 16; i++)
			{
				if (verifQueue[q].tubeMask & (1 << i))
				{
					bool isTubeEmpty = verifQueue[q].isFrag ? (rxData.frag_status[i] == 0x55) : (rxData.sis_status[i] == 0x55);

					if (verifQueue[q].blast_ok == false) {
						if (!ariza_hafizasi[i]) {
							tubeFaultTime[i] = (systemTime.hour << 8) | systemTime.minute;
						}
						ariza_hafizasi[i] = true;
						ariza_is_frag[i] = verifQueue[q].isFrag;
						anyMisfire = true;

						dispatchWarning(verifQueue[q].isFrag ? WARN_BLASTING_FAILED_FRAG : WARN_BLASTING_FAILED_SMOKE, i + 1);
					}
					else if (isTubeEmpty == false) {
						if (!ariza_hafizasi[i]) {
							tubeFaultTime[i] = (systemTime.hour << 8) | systemTime.minute;
						}
						ariza_hafizasi[i] = true;
						ariza_is_frag[i] = verifQueue[q].isFrag;
						anyMisfire = true;

						dispatchWarning(verifQueue[q].isFrag ? WARN_MISFIRE_FRAG : WARN_MISFIRE_SMOKE, i + 1);
					}
					else {
						ariza_hafizasi[i] = false;
						ariza_is_frag[i] = verifQueue[q].isFrag;

						dispatchWarning(verifQueue[q].isFrag ? WARN_FIRE_SUCCESS_FRAG : WARN_FIRE_SUCCESS_SMOKE, i + 1);
					}
				}
			}

            uint8_t h = systemTime.isValid ? systemTime.hour : 0;
			uint8_t m = systemTime.isValid ? systemTime.minute : 0;
			uint8_t s = systemTime.isValid ? systemTime.second : 0;
			// CAN Bus logları (Arayüz gibi tek tek değil, maske olarak tek bir CAN paketiyle 4'ünü birden bildiririz, bus'ı boğmamak için)
			if (verifQueue[q].blast_ok == false) {
				// 0x00: Patlatma Sinyali Alınamadı
				CAN_Log_Add(LOG_FIRE_EVENT, 0x00, 0, 0, 0, h, m, s);
			} else if (anyMisfire) {
				// 0x01: Mühimmat Çıkmadı
				CAN_Log_Add(LOG_FIRE_EVENT, 0x01, 0, 0, 0, h, m, s);
			} else {
				// 0x02: Atış Başarılı
				uint8_t mask_lsb = (uint8_t)(verifQueue[q].tubeMask & 0xFF);
				uint8_t mask_msb = (uint8_t)((verifQueue[q].tubeMask >> 8) & 0xFF);
				CAN_Log_Add(LOG_FIRE_EVENT, 0x02, verifQueue[q].isFrag, mask_lsb, mask_msb, h, m, s);
			}

			// İşlem bitti, görevi kuyruktan sil
			verifQueue[q].active = false;
		}
	}
}

// ==============================================================
// CBIT (CONTINUOUS BUILT-IN TEST) - SÜREKLİ CİHAZ İÇİ TEST
// ==============================================================
void Model::processCBIT()
{
    // 1. İLETİŞİM KOPTU KONTROLÜ
    if (HAL_GetTick() - BMB_Get_Last_Valid_Rx_Time() > 3000)
    {
        if (!comm_lost_flag) {
            comm_lost_flag = true;
            dispatchWarning(WARN_COMM_LOST);
        }
    }
    else
    {
        comm_lost_flag = false;
    }

    // 2. DONANIMSAL VOLTAJ HATASI KONTROLÜ
    if (!comm_lost_flag)
    {
        // MASKELEME: Atış sırasında voltaj çökmesi normaldir, hata sayma!
        if (currentFiringState == STATE_IDLE)
        {
            BMB_RxPacket_t rxData;
            BMB_Get_Latest_Rx_Data(&rxData);

            bool current_hw_error = (rxData.console_volt_err != 0 || rxData.blast_ready_err != 0 || rxData.main_5v_err != 0);

            if (current_hw_error && !hw_error_flag) {
                hw_error_flag = true;
                dispatchWarning(WARN_VOLTAGE_ERROR);
            } else if (!current_hw_error) {
                hw_error_flag = false;
            }
        }
    }
}


Model::BmbTelemetryData Model::getBmbTelemetry()
{
    BmbTelemetryData data = {0};

    // Donanımdan ham veriyi çek
    BMB_RxPacket_t rxData;
    BMB_Get_Latest_Rx_Data(&rxData);

    // Arayüzün kullanacağı temiz yapıya aktar
    data.console_input_voltage = rxData.console_input_voltage;
    data.blast_volt_reg1       = rxData.blast_volt_reg1;
    data.main_5v_reg           = rxData.main_5v_reg;
    data.power_3v3_reg         = rxData.power_3v3_reg;

    data.console_volt_err      = rxData.console_volt_err;
    data.blast_ready_err       = rxData.blast_ready_err;
    data.main_5v_err           = rxData.main_5v_err;
    data.mcu_3v3_err           = rxData.mcu_3v3_err;

    return data;
}

Model::CommTelemetryData Model::getCommTelemetry()
{
    CommTelemetryData data;

    // 1. BMB Güç Kartı (UART)
    // comm_lost_flag false ise bağlantı OK demektir.
    data.bmb_uart_ok = !comm_lost_flag;

    // 2. Radar Birincil Hat (Ethernet - Sadece RX)
    // TODO: İleride Ethernet link status veya RX timeout bayrağını buraya bağla
    // Örn: data.radar_eth_ok = !eth_link_down_flag;
    data.radar_eth_ok = true;

    // 3. Radar Yedek Hat (RS422 - Sadece RX)
    // TODO: İleride RS422 RX kesmesinden gelen bir timeout bayrağını buraya bağla
    data.radar_rs422_ok = true;

    // 4. Log Çıkışı (CAN Bus - Sadece TX)
    // TODO: İleride CAN TEC (Transmit Error Counter) veya Mailbox hata bayrağını bağla
    data.can_bus_ok = true;

    return data;
}



// ICD'den Gelen Zaman Bilgisi Yakalanınca
void Model::onTimeDataParsed(const TLUS::TimeData& time)
{
    if (time.isValid)
    {
    	Sync_RTC_Time(time.hour, time.minute, time.second, time.day, time.month, time.year);
    }
    timeUpdated = true;
}

// ICD'den Tehdit Bilgisi Yakalanınca
void Model::onThreatsParsed(const TLUS::ThreatMessagePayload& payload)
{
	/* Gecerli olsun olmasin, mesajin GELDIGI bilgisi tazelik icin degerli */
	lastThreatMsgMs = HAL_GetTick();
	threatDataFresh = true;

	if (payload.isValid)
	{
		tehdidler = payload;
	}
	else
	{
		/* TLUS "tehdit bilgilerim guvenilir degil" diyor.
		 * Eski listeyi TUTMA - operatore hayalet tehdit gosterme. */
		tehdidler.isValid = false;
		tehdidler.count   = 0;
	}
	threatsUpdated = true;

}

// ICD'den sistem durumları gelince
void Model::onSystemStatusParsed(const TLUS::SystemStatusPayload& status)
{
	if(status.isValid)
	{
		// En taze veriyi geçici bir değişkene (latestSystemStatus) al ve Tick'e haber ver
		latestSystemStatus = status;
		faultUpdate = true;
	}
}

void Model::onGvdDataParsed(const TLUS::GvdMessagePayload& payload)
{
    // Arayüze anında pasla
    if (modelListener != 0) {
        // Not: Bunu ModelListener.hpp içine "virtual void onGvdDataReceived(const TLUS::GvdMessagePayload& payload) {}"
        // olarak eklemeyi unutma!
       // modelListener->onGvdDataReceived(payload);
    }
}


// Arayüzden Giden İsteği ICD'ye Yolla
void Model::uiSendSoftReset()
{
    icdManager.sendSoftReset(); // O kendi kendine byte'ları dizecek ve donanıma fırlatacak
    // LOGLAMA İŞLEMİ
	uint8_t h = systemTime.isValid ? systemTime.hour : 0;
	uint8_t m = systemTime.isValid ? systemTime.minute : 0;
	uint8_t s = systemTime.isValid ? systemTime.second : 0;

	// CAN Bus: [1]=0x03 (Soft Reset Kodu)
	CAN_Log_Add(LOG_SYS_STATE, 0x03, 0x00, h, m, s, 0, 0);

	// Ekran Logu
	addUILog(LOG_EVENT_SYSTEM_STATE, T_LOG_SOFT_RESET);
}
// Ekrandan "Acil Silme (Zeroize)" komutu geldiğinde ICD'ye yolla
void Model::uiSendAcilSilme(TLUS::EraseTarget target)
{
    icdManager.sendAcilSilme(target);
    // LOGLAMA İŞLEMİ
	uint8_t h = systemTime.isValid ? systemTime.hour : 0;
	uint8_t m = systemTime.isValid ? systemTime.minute : 0;
	uint8_t s = systemTime.isValid ? systemTime.second : 0;

	// CAN Bus: [1]=0x04 (Zeroize Kodu), [2]=Silme Hedefi (Target)
	CAN_Log_Add(LOG_SYS_STATE, 0x04, (uint8_t)target, h, m, s, 0, 0);

	// Ekran Logu
	addUILog(LOG_EVENT_SYSTEM_STATE, T_LOG_ZEROIZE);
}
// Ekrandan "Acil Silme (Zeroize)" butonuna basıldığında
void Model::uiSendModeChange(TLUS::TlusMode mode)
{
    icdManager.sendModDegistirme(mode);
    uint8_t h = systemTime.isValid ? systemTime.hour : 0;
	uint8_t m = systemTime.isValid ? systemTime.minute : 0;
	uint8_t s = systemTime.isValid ? systemTime.second : 0;

	CAN_Log_Add(LOG_SYS_STATE, 0x05, (uint8_t)mode, h, m, s, 0, 0); // 0x05 = Mod Değişimi
	addUILog(LOG_EVENT_SYSTEM_STATE, T_LOG_MOD_CHNGE, (int)mode);
}
void Model::uiSendGvdSecimi(TLUS::GvdNumber gvdNo)
{
	icdManager.sendGvdSecimi(gvdNo);
	uint8_t h = systemTime.isValid ? systemTime.hour : 0;
	uint8_t m = systemTime.isValid ? systemTime.minute : 0;
	uint8_t s = systemTime.isValid ? systemTime.second : 0;

	CAN_Log_Add(LOG_SYS_STATE, 0x06, (uint8_t)gvdNo, h, m, s, 0, 0); // 0x06 = GVD Seçimi
	addUILog(LOG_EVENT_SYSTEM_STATE, T_LOG_SEND_GVD_SELECT, (int)gvdNo);
}
void Model::uiSendGvdBilgiIstek()
{
    icdManager.sendGvdBilgiIstek();
    uint8_t h = systemTime.isValid ? systemTime.hour : 0;
	uint8_t m = systemTime.isValid ? systemTime.minute : 0;
	uint8_t s = systemTime.isValid ? systemTime.second : 0;

	CAN_Log_Add(LOG_SYS_STATE, 0x07, 0x00, h, m, s, 0, 0); // 0x07 = GVD İstek
	addUILog(LOG_EVENT_SYSTEM_STATE, T_LOG_SEND_GVD_INFO);
}
// ==============================================================
// C'DEN C++'A GEÇİŞ KÖPRÜSÜ (DONANIM -> TOUCHGFX)
// ==============================================================
extern "C" {
    void TLUS_Donanimdan_Gelen_Veri(uint8_t* buffer, uint16_t length)
    {
        // Eğer Model oluşturulmuşsa, veriyi doğrudan ICD motoruna bas!
        if (globalModelInstance != nullptr) {
            globalModelInstance->getProtocolManager().feedBuffer(buffer, length);
        }
    }
}

// ==============================================================
// EKRAN İÇİN LOG (KAYIT) MOTORU FONKSİYONLARI
// ==============================================================
void Model::addUILog(LogEventType type, uint16_t textId, int param)
{
    // O anki saati RTC'den al (Eğer geçerli değilse 00:00:00 yazar)
    uint8_t h = systemTime.isValid ? systemTime.hour : 0;
    uint8_t m = systemTime.isValid ? systemTime.minute : 0;
    uint8_t s = systemTime.isValid ? systemTime.second : 0;

    // Logu tampona yaz
    logHistory[logHead].hour = h;
    logHistory[logHead].minute = m;
    logHistory[logHead].second = s;
    logHistory[logHead].type = type;
    logHistory[logHead].textId = textId;
    logHistory[logHead].param = param;

    // Halka Tampon (Circular Buffer) Mantığı:
    logHead++;
    if (logHead >= MAX_LOG_COUNT) {
        logHead = 0; // Başa sar, en eskisinin üzerine yaz
    }

    if (logCount < MAX_LOG_COUNT) {
        logCount++; // Max limite kadar sayıyı artır
    }
}

Model::LogEntry Model::getLog(int index) const
{
    LogEntry emptyLog = {0};
    if (index < 0 || index >= logCount) return emptyLog;

    // Ekranda en yeni logu en üstte göstermek için tersten hesaplama
    int actualIndex = logHead - 1 - index;
    if (actualIndex < 0) {
        actualIndex += MAX_LOG_COUNT; // Halka tam tur döndüyse tersten dön
    }

    return logHistory[actualIndex];
}
