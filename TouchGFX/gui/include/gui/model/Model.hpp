#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>
#include "buzzer_driver.h"
#include <gui/model/tlus_protocol.hpp>
#include <texts/TextKeysAndLanguages.hpp>

class ModelListener;

class Model : public TLUS::ICDListener
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();
    void saveBrightness(int brightness);
    void saveVolume(int volume);
    int getBrightness() const { return currentBrightness; }
	int getVolume() const { return currentVolume; }

	// AYARLAR EKRANI İÇİN: Uyarıları Aç/Kapat
	void setWarningsEnabled(bool state) { warningsEnabled = state; }
	bool getWarningsEnabled() const { return warningsEnabled; }

	// ARIZA EKRANI İÇİN: Arızaları Manuel Sil
	void clearAllFaults();

	int getLedBrightness() const { return currentLedBrightness; }
	void saveLedBrightness(int level);

    // Popup ile göstereceğimiz Hata/Uyarı Kodları
	enum WarningType {
		WARN_SYSTEM_LOCKED = 0,     // "SİSTEM KİLİTLİ! ATIŞ YAPILAMAZ"
		WARN_NO_AMMO_SELECTED,      // "MÜHİMMAT TİPİ SEÇİLMEDİ!"
		WARN_GROUP_TOTALLY_EMPTY,   // "SEÇİLİ BÖLGE TAMAMEN BOŞ!"
		WARN_WRONG_AMMO_IN_GROUP,   // "BÖLGEDE İSTENEN TİP MÜHİMMAT YOK!"
		WARN_COMM_ERROR,            // "BMB İLETİŞİM HATASI (READY ALINAMADI)"
		WARN_COMM_LOST,             // "SİSTEM İLETİŞİMİ KOPTU (KABLO HATASI)!"
		WARN_VOLTAGE_ERROR,         // "DONANIM ARIZASI: PATLATMA VOLTAJI HATALI!"

		WARN_MISFIRE_SMOKE,
		WARN_MISFIRE_FRAG,
		WARN_BLASTING_FAILED_SMOKE,
		WARN_BLASTING_FAILED_FRAG,
		WARN_FIRE_SUCCESS_SMOKE,
		WARN_FIRE_SUCCESS_FRAG,
		WARN_TLUS_SW_DEAD,          // "TLUS YAZILIMI YANIT VERMİYOR"
		WARN_TLUS_FROZEN,           // "TLUS VERİSİ GÜNCELLENMİYOR"

		// --- YENİ EKLENEN İŞLEMCİ UYARILARI ---
		WARN_PROC_RAM,
		WARN_PROC_NVRAM,
		WARN_PROC_MEMFILE,
		WARN_PROC_NVSRAM,
		WARN_PROC_MEMFULL,
		WARN_PROC_SER1,
		WARN_PROC_SER2,
		WARN_PROC_SER3,
		WARN_PROC_SER4,
		WARN_PROC_IFACE,
		WARN_PROC_PWR,
		WARN_PROC_SHUTDOWN,
		WARN_PROC_PWR_SER,
		WARN_PROC_TEMP,

		// --- YENİ EKLENEN SENSÖR UYARILARI ---
		WARN_SENS_B12,
		WARN_SENS_B3_0,
		WARN_SENS_B3_1,
		WARN_SENS_B3_2,
		WARN_SENS_CTRL,
		WARN_SENS_SHUTDOWN,
		WARN_SENS_PWR_SER,
		WARN_SENS_PRESS,
		WARN_SENS_V3_7,
		WARN_SENS_V7_4,
		WARN_SENS_V16,
		WARN_SENS_V80,
		WARN_SENS_VN7_4,
		WARN_SENS_VN3_7,
		WARN_SENS_PWR,
		WARN_SENS_TEMP_LIM,
		WARN_SENS_TEMP_SNS
	};

    enum ActiveScreenType {
		SCREEN_RADAR,  // Screen 1
		SCREEN_SMOKE,  // Screen 2
		SCREEN_SETTING,
		SCREEN_FAULTS,
		SCREEN_TESTS,
		SCREEN_LOGS,
		SCREEN_ZEROIZE,
		SCREEN_SOFTRESET,
		SCREEN_OTHER
	};

    enum TubeState {
		TUBE_EMPTY = 0, // 0x55
		TUBE_SMOKE,     // Sis (Yeşil)
		TUBE_FRAG,      // Frag (Turuncu)
		TUBE_FAULT      // Arızalı (Kırmızı)
	};


	// AYARLAR EKRANI İÇİN: Salvo (Seri Atış) Modu Ayarı
	enum SalvoMode {
		SALVO_SINGLE_ROUND = 0, // Her köşeden 1 tane (Toplam 4 atış)
		SALVO_FULL_EMPTY        // Tanktaki mühimmatlar bitene kadar
	};

	enum PendingAction {
		ACTION_NONE,
		ACTION_ZEROIZE,
		ACTION_SOFT_RESET
	};

	// Arayüz (View) için donanımdan izole edilmiş Telemetri Struct'ı
	struct BmbTelemetryData {
		uint32_t console_input_voltage;
		uint16_t blast_volt_reg1;
		uint16_t main_5v_reg;
		uint16_t power_3v3_reg;
		uint8_t  console_volt_err;
		uint8_t  blast_ready_err;
		uint8_t  main_5v_err;
		uint8_t  mcu_3v3_err;
	};

	//Dış Dünya İletişim Durumu Telemetrisi
	struct CommTelemetryData {
		bool bmb_uart_ok;       // BMB Güç Kartı (UART)
		bool radar_eth_ok;      // Radar Birincil Hat (Ethernet RX)
		bool radar_rs422_ok;    // Radar Yedek Hat (RS422 RX)
		bool can_bus_ok;        // Log Çıkış Hattı (CAN TX)
	};

	CommTelemetryData getCommTelemetry();
	BmbTelemetryData getBmbTelemetry(); // Arayüzün veriyi çekeceği fonksiyon

	void setSalvoMode(SalvoMode mode) { currentSalvoMode = mode; }
	SalvoMode getSalvoMode() const { return currentSalvoMode; }

	void setActiveScreen(ActiveScreenType screen) ;
	bool getFaultStatus(int index) const { return ariza_hafizasi[index]; }
	bool getFaultIsFrag(int index) const { return ariza_is_frag[index]; }
	bool getCommLostFlag() const { return comm_lost_flag; }
	bool getHwErrorFlag() const { return hw_error_flag; }

	void setLedTestMode(int mode);
	void setBuzzerLevel(AlarmLevel_t level);

	// TEST MODU KİLİDİ: Aktifken ateşleme, karartma ve ekran geçişleri engellenir
	void setTestModeActive(bool active) { isTestModeActive = active; }
	bool getTestModeActive() const { return isTestModeActive; }

	static const int MAX_TARGETS = 22;

	int getFadeTicksRemaining(int index) { return fadeTicksRemaining[index]; }
	void setFadeTicksRemaining(int index, int ticks) { fadeTicksRemaining[index] = ticks; }

	// Gelen Verileri Yakalayan Callback'ler
	virtual void onTimeDataParsed(const TLUS::TimeData& time) override;
	virtual void onThreatsParsed(const TLUS::ThreatMessagePayload& payload) override;
	virtual void onSystemStatusParsed(const TLUS::SystemStatusPayload& status) override;
	virtual void onGvdDataParsed(const TLUS::GvdMessagePayload& payload) override;

	// Arayüz Butonlarının Tetikleyeceği TX Fonksiyonları
	void uiSendModeChange(TLUS::TlusMode mode);
	void uiSendSoftReset();
	void uiSendAcilSilme(TLUS::EraseTarget target);
	void uiSendGvdSecimi(TLUS::GvdNumber gvdNo);
	void uiSendGvdBilgiIstek();

	// Protocol motorunu donanımın (main.c) besleyebilmesi için bir kapı
	TLUS::Protocol& getProtocolManager() { return icdManager; }
	TLUS::TimeData getSystemTime() const { return systemTime; }
	TLUS::SystemStatusPayload getSystemStatus() const { return latestSystemStatus; }

	void setPendingAction(PendingAction action) { pendingAction = action; }
	PendingAction getPendingAction() const { return pendingAction; }

	TLUS::SystemStatusPayload getActiveSystemStatus() const { return activeSystemStatus; }
	TLUS::SystemStatusPayload getStoredSystemStatus() const { return storedSystemStatus; }

	bool getStoredCommLostFlag() const { return stored_comm_lost_flag; }
	bool getStoredHwErrorFlag() const { return stored_hw_error_flag; }
	uint16_t getProcFaultTime(int index) { return procFaultTime[index]; }
	uint16_t getSensFaultTime(int s, int index) { return sensFaultTime[s][index]; }
	uint16_t getTubeFaultTime(int index) { return tubeFaultTime[index]; }
	uint16_t getCommFaultTime() { return commFaultTime; }
	uint16_t getHwFaultTime() { return hwFaultTime; }

	// =======================================================
	// EKRAN İÇİN DİNAMİK LOG (KAYIT) MİMARİSİ
	// =======================================================
	enum LogEventType {
		LOG_EVENT_FAULT_OCCURRED, // Arıza Oluştu
		LOG_EVENT_FAULT_CLEARED,  // Arıza Giderildi (Fiziksel)
		LOG_EVENT_CLEAR_ALL,      // Operatör "Tümünü Sil"e bastı
		LOG_EVENT_FIRE,           // Atış Yapıldı
		LOG_EVENT_SYSTEM_STATE    // Sistem Şalter Durumu (Armed/Smoke/Frag)
	};

	struct LogEntry {
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
		LogEventType type;
		uint16_t textId; // YENİ: Ekrana direkt basılacak Text ID'si
		int param;       // Tüp Numarası vb.
	};

	static const int MAX_LOG_COUNT = 64; // Ekranda tutulacak son 64 olay

	void addUILog(LogEventType type, uint16_t textId, int param = -1);
	int getLogCount() const { return logCount; }
	int getLogHead() const { return logHead; }
	LogEntry getLog(int index) const; // Arayüzün okuyacağı fonksiyon

	 void setBuzzerOverride(bool on) { buzzerManualOverride = on; }

protected:
    ModelListener* modelListener;
    uint16_t azimuth = 0;
    uint8_t elevation = 0;

    ActiveScreenType currentScreen; // Hangi ekranda olduğumuzu tutan değişken

    uint32_t lastModelTickTime;
    int fadeTicksRemaining[MAX_TARGETS];

    bool isBlackoutMode;
	uint8_t savedBrightnessPWM;
	int currentBrightness;
	int currentVolume;

	bool ariza_hafizasi[16];
	// View'a gönderilecek 16'lı nihai durum dizisi
	TubeState tube_states[16];
	int currentLedBrightness = 3; // Varsayılan LED Parlaklığı
	bool ariza_is_frag[16]; // Arızalanan tüpün tipini tutar

	TLUS::SystemStatusPayload activeSystemStatus; // Ticker (Alt yazı) için
	TLUS::SystemStatusPayload storedSystemStatus; // Screen 4 için

	// SAAT BİLGİLERİNİ TUTACAK HAFIZALAR (Format: (Saat << 8) | Dakika )
	uint16_t procFaultTime[14] = {0};
	uint16_t sensFaultTime[4][17] = {0};
	uint16_t tubeFaultTime[16] = {0};
	uint16_t commFaultTime = 0;
	uint16_t hwFaultTime = 0;

	bool stored_comm_lost_flag = false;
	bool stored_hw_error_flag = false;

	// Log Halka Tamponu (Circular Buffer)
	LogEntry logHistory[MAX_LOG_COUNT];
	int logHead = 0;  // Yeni logun yazılacağı index
	int logCount = 0; // Toplam kayıtlı log sayısı

private:
	// Ateşleme Makinesi Durumları
	enum FiringState {
		STATE_IDLE = 0,
		STATE_WAIT_READY,
		STATE_FIRING,
		STATE_WAIT_VERIFY,
		STATE_WAIT_COOLDOWN //BMB'nin 0x00'a dönmesini beklediğimiz durum
	};

	// -- ARKA PLAN DOĞRULAMA KUYRUĞU ---
	struct VerificationTask {
	    uint16_t tubeMask;       // Hangi tüp ateşlendi?
	    uint32_t timestamp;      // Ne zaman ateşlendi?
	    bool isFrag;             // FRAG mı SMOKE mu?
	    bool blast_ok;           // 250ms içinde patlatma sinyali alındı mı?
	    bool active;             // Bu görev aktif mi?
	};

	VerificationTask verifQueue[16] = {0}; // Aynı anda 16 tüpün sonucunu arka planda bekleyebiliriz
	bool any_blasting_occurred = false;

	FiringState currentFiringState = STATE_IDLE;
	uint32_t firingTimer;

	uint16_t currentFiringMask;
	bool targetIsFrag;

	bool warningsEnabled = true; // Varsayılan olarak uyarılar AÇIK
	bool blasting_occurred = false; // Güç kartının patlatma yapıp yapmadığını takip eder

	uint32_t last_raw_button_state;
	int currentLedTestMode = 0; // 0: Normal, 1: Kırmızı, 2: Yeşil, 3: Turuncu

	int last_warning_tube_index = -1; // Arayüze aktarılacak tüp numarası

	void processFiringStateMachine();
	int findNextAvailableTube(int startIdx, int endIdx, bool isFrag);
	void fireGroup(int startIdx, int endIdx);

	// KOM1: Ateşleme Kilidi (Arming)
	bool isSystemArmed() const;  // Kilit Açık mı? (Atışa İzin Ver)

	// KOM2: Mühimmat Seçimi (Ammo Selection)
	bool isSmokeSelected() const; // Sis mi seçili?
	bool isFragSelected() const;  // Tahrip mi seçili?

	// Tick process function
    void processButtons();
    void processSmokeScreen();
    void processRadarScreen();
    void processBackgroundVerifications();
    void processTimeDateContainer();
    void processWarnings();

    // Uyarıları filtreleyip View'a gönderen yardımcı fonksiyon
	void dispatchWarning(WarningType warning, int tubeIndex = -1);

	// --- CBIT (Cihaz İçi Sürekli Test) Değişkenleri ---
	bool comm_lost_flag;
	bool hw_error_flag;

	void processCBIT(); // Arka planda donanımı sürekli denetleyen fonksiyon

	// --- Salvo (Seri Atış) Takip Değişkenleri ---
	bool isSalvoMode = false;
	int currentSalvoGroup = 0;
	int salvoGroups[4][2] = { {0, 3}, {8, 11}, {4, 7}, {12, 15} };

	SalvoMode currentSalvoMode = SALVO_SINGLE_ROUND; // Varsayılan olarak 1 Tur (4 Atış) olsun
	int salvoShotCount = 0; // O anki salvoda kaç atış yapıldığını sayar

	void fireSalvoAll();

	bool isTestModeActive = false; // Başlangıçta kilit kapalı



	uint32_t blast_start_time = 0;

	bool is_measuring_blast = false;
	uint8_t last_blasting_state = 0x00;

	TLUS::TimeData systemTime;
	uint8_t last_minute = 255; // Saatin değişip değişmediğini anlamak için

	TLUS::ThreatMessagePayload tehdidler;
	bool threatsUpdated = false;

	TLUS::Protocol icdManager;
	TLUS::SystemStatusPayload latestSystemStatus;
	bool faultUpdate = false;

	bool timeUpdated = false;
	PendingAction pendingAction = ACTION_NONE;

	 /* --- Tehdit kaynakli sesli ikaz --- */
	AlarmLevel_t threatAlarmLevel   = ALARM_NONE;
	uint32_t     lastThreatMsgMs    = 0;
	uint32_t     alarmHoldUntilMs   = 0;
	bool         threatDataFresh    = true;
	bool         buzzerManualOverride = false;   /* test ekrani icin */

	void processThreatAlarm();
	uint32_t lastBurstMs           = 0;
	uint32_t lastCounterMeasureMs  = 0;
	 bool tlus_comm_lost_flag = false;

	 uint32_t lastSysStatusMs   = 0;
	 uint16_t lastHeartbeat     = 0;
	 uint8_t  heartbeatStall    = 0;      /* ardisik degismeyen mesaj sayisi */
	 bool     tlusSwDeadFlag    = false;  /* 5 sn mesaj yok */
	 bool     tlusFrozenFlag    = false;  /* heartbeat ilerlemiyor */
};

#endif // MODEL_HPP
