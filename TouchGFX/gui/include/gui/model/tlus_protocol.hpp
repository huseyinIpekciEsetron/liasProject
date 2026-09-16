/*
 * tlus_protocol.hpp
 *
 *  Created on: Aug 26, 2026
 *      Author: HUSEYIN
 */

#ifndef TLUS_PROTOCOL_HPP
#define TLUS_PROTOCOL_HPP

#include <stdint.h>

namespace TLUS {

// --- ICD'de Tanımlı Mesaj ID'leri
    enum MsgId {
        MSG_TLUS_TEHDITLERI       = 0x01, // RX
        MSG_KONFIGURASYON         = 0x02, // TX
        MSG_MOD_DEGISTIRME        = 0x06, // TX
        MSG_SISTEM_DURUMU         = 0x08, // RX
        MSG_TARIH_ZAMAN           = 0x0C, // RX
        MSG_GVD_SECIMI            = 0x0D, // TX
        MSG_ACIL_SILME            = 0x0E, // TX
        MSG_SOFT_RESET            = 0x0F, // TX
        MSG_GVD_BILGI_ISTEK       = 0x26, // TX
        MSG_GVD_BILGILERI         = 0x27, // RX
		MSG_TUP_DURUMU            = 0x30  // TX
    };

    enum SourceId {
        SRC_VYS_CB = 0x01,
        SRC_TLUS   = 0x02,
        SRC_TAKS   = 0x03
    };


// --- ZAMAN Ve GUN ---
    struct TimeData {
		bool isValid; // Tarih verisi geçerli mi?
		uint8_t hour, minute, second, day, month;
		uint16_t year;
	};

// --- TEHDIT BILGILERI ---
    enum ThreatClass : uint8_t {
		THREAT_SRCH = 0x00, // 0: Arama (Search)
		THREAT_LRF  = 0x01, // 1: Lazer Mesafe Bulucu
		THREAT_LD   = 0x02, // 2: Lazer İşaretleyici
		THREAT_LBR  = 0x04  // 4: Lazer Işın Yönlendirici (Beam Rider)
	};

	enum ThreatBand : uint8_t {
		BAND_1 = 0x01,
		BAND_2 = 0x02,
		BAND_3 = 0x03
	};

	struct ThreatData {
		ThreatClass threatClass;  // (Güncellendi) 0: SRCH, 1: LRF, 2: LD, 4: LBR
		uint8_t ageOut;           // 1: Süresi doldu, 0: Aktif
		uint8_t priority;         // 1-31 arası öncelik
		uint16_t threatNumber;    // Sistem ID'si (0-65535)
		ThreatBand band;          // (Güncellendi) 1: Band1, 2: Band2, 3: Band3
		float angle;              // 0-360.0 derece (LSB 0.1 olduğu için float)
		uint32_t trackCode;       // Tehdit Tanımlama Kodu
		uint32_t prf;             // Darbe Tekrar Periyodu (ns)
	};

	// Ana Tehdit Mesajı Paketi (Geçerlilik ve Liste)
	struct ThreatMessagePayload {
		bool isValid;                   // Byte 5-6, Bit 15 (Geçerlilik Durumu)
		uint8_t count;                  // Byte 5-6, Bit 5-0 (Tehdit Sayısı, Max 20)
		ThreatData threats[20];         // İçindeki hedeflerin listesi
	};

// --- SİSTEM DURUMU (0x08) İÇİN  ---
	enum TlusMode : uint16_t {
		MODE_STANDBY        = 0x001,
		MODE_OPERATE        = 0x002,
		MODE_RESERVE        = 0x004,
		MODE_IBIT           = 0x008,
		MODE_SW_UPLOAD      = 0x020, // Yazılım Yükleme
		MODE_GVD_UPLOAD     = 0x040, // GVD Yükleme
		MODE_LOG_DOWNLOAD   = 0x080  // Kayıt İndirme
	};

	enum ConfigAcceptance : uint8_t {
		CONFIG_REJECTED_OR_NOT_LOADED = 0x01,
		CONFIG_ACCEPTED               = 0xAA
	};

	enum EraseStatus : uint8_t {
		ERASE_NOT_STARTED   = 0x00, // 00 Binary
		ERASE_IN_PROGRESS   = 0x02, // 10 Binary (Başlamış bitmemiş)
		ERASE_COMPLETED     = 0x03  // 11 Binary (Başlamış ve bitmiş)
	};

	enum FileSystemStatus : uint8_t {
		FS_FORMAT_IN_PROGRESS = 0x00, // Devam ediyor
		FS_FORMAT_COMPLETED   = 0x01  // İklimlendirme Bitmiş
	};

	struct SensorFaultData {
		// MSB Tarafı
		bool bant_I_II_Karti;                // Bit 15
		bool bant_III_Sensor0;           // Bit 11
		bool bant_III_Karti_Sensor1;       // Bit 7
		bool bant_III_Karti_Sensor2;        // Bit 3

		// LSB Tarafı
		bool sensor_Birimi_Kontrol_Karti;   // Bit 15
		bool kontrollu_Kapanma;             // Bit 13
		bool guc_Karti_Seri_Kanal;          // Bit 12
		bool basinc_Durumu;                 // Bit 9
		bool volt_3_7V;                     // Bit 8
		bool volt_7_4V;                     // Bit 7
		bool volt_16V;                      // Bit 6
		bool volt_80V;                     // Bit 5
		bool volt_neg7_4V;                  // Bit 4
		bool volt_neg3_7V;                  // Bit 3
		bool ana_Besleme;                   // Bit 2
		bool sicaklik_Durumu;               // Bit 1
		bool sicaklik_Sensoru;              // Bit 0
	};

	// İşlemci Biriminin Detaylı Hata Bitleri
	struct ProcessorFaultData {
		bool ramTesti;                      // Byte 7-8, Bit 15
		bool kaliciBellekTesti;             // Byte 7-8, Bit 11
		bool bellekDosyasiTesti;            // Byte 7-8, Bit 7
		bool nvsramTesti;                   // Byte 7-8, Bit 3
		bool bellekDoluluk;                 // Byte 7-8, Bit 2

		bool seriKanal1;                    // Byte 9-10, Bit 15
		bool seriKanal2;                    // Byte 9-10, Bit 11
		bool seriKanal3;                    // Byte 9-10, Bit 7
		bool seriKanal4;                    // Byte 9-10, Bit 3

		bool arayuzKarti;                   // Byte 11-12, Bit 15
		bool anaBesleme;                    // Byte 11-12, Bit 11
		bool islemciDurumuKapanma;          // Byte 11-12, Bit 7
		bool gucKartiSeriKanal;             // Byte 11-12, Bit 3
		bool sicaklikEsikAsimi;             // Byte 11-12, Bit 2
	};

	// ANA SİSTEM DURUMU PAKETİ (Tüm Cihaz)
	struct SystemStatusPayload {
		bool isValid;                       // Geçerlilik
		bool isPbitDone;                    // PBIT yapıldı mı? (Normal/Hızlı Açılış)

		bool sensor1GenelHata;
		bool sensor2GenelHata;
		bool sensor3GenelHata;
		bool sensor4GenelHata;
		bool islemciGenelHata;
		uint8_t tlusState;                  // 0: OK, 1: Kısmi Hatalı, 2: Hatalı

		ProcessorFaultData processorFaults; // İşlemci Alt Hataları
		SensorFaultData sensorFaults[4];    // 4 Sensörün Alt Hataları [0]=Sensör1

		TlusMode tlusMode;                  // Cihazın Çalışma Modu
		uint16_t heartbeat;                 // 0'dan 65535'e saniyelik sayaç
		ConfigAcceptance configStatus;      // Konfigürasyon Kabul Durumu

		EraseStatus acilSilmeYazilim;       // Yazılım Silme Durumu
		EraseStatus acilSilmeGVD;           // GVD Silme Durumu
		EraseStatus acilSilmeKayit;         // Kayıt Silme Durumu

		uint8_t activeGvd;                  // Aktif GVD No (0=Kullanılmıyor, 1-5 arası aktif)
		bool isDiskOverwriteEnabled;        // 1: Eskilerin üstüne yazar, 0: Yazmaz
		bool isBlankingActive;              // 1: Blank durumunda, 0: Değil

		FileSystemStatus fileSysFormatting; // Dosya Sistemi İklimlendirme Durumu
	};

// --- ACİL SİLME (ZEROIZE) HEDEF SEÇİMİ ---
	enum EraseTarget : uint16_t {
		ERASE_NONE    = 0x00, // 000
		ERASE_RECORDS = 0x01, // 001 (Bit 0: Kayıt Bilgileri)
		ERASE_GVD     = 0x02, // 010 (Bit 1: Görev Veri Dosyası)
		ERASE_SW      = 0x04, // 100 (Bit 2: Yazılım)
		ERASE_ALL     = 0x07  // 111 (Hepsini Sil)
	};

// --- GVD SEÇİMİ (tlusun kullanması gereken GVD sıra numarasını (gvd seti numarası) bildirir ) ---
	    enum GvdNumber : uint16_t {
	        GVD_1 = 1,
	        GVD_2 = 2,
	        GVD_3 = 3,
	        GVD_4 = 4,
	        GVD_5 = 5
	    };

// --- GVD BILGILERI ---
	    struct GvdDetail {
		bool isValid;         // GVD yüklü ve kullanılabilir mi?
		char name[33];        // 32 karakter İsim + 1 Null Terminator ('\0')
		char version[25];     // 24 karakter Versiyon + 1 Null Terminator ('\0')
	};

	// Tüm GVD'leri Barındıran Ana Paket
	struct GvdMessagePayload {
		GvdDetail gvds[5];    // 5 adet GVD slotu
	};

// Callback (Köprü) Sınıfı - Model bu sınıfı dinleyecek
    class ICDListener {
    public:
        virtual void onTimeDataParsed(const TimeData& time) {}
        virtual void onThreatsParsed(const ThreatMessagePayload& payload) {}
        virtual void onSystemStatusParsed(const SystemStatusPayload& status) {}
        virtual void onGvdDataParsed(const GvdMessagePayload& payload) {}

    };

    // Ana Protokol Sınıfı
    class Protocol {
    private:
        ICDListener* listener = nullptr;
        void (*txFunction)(uint8_t*, uint16_t) = nullptr;

        void parsePacket(const uint8_t* msg, uint16_t len);
	    uint16_t expectedLenFor(uint8_t msgId, const uint8_t* msg, uint16_t len);
	    uint8_t  buildChecksum(const uint8_t* data, uint16_t lenWithoutCks);

    public:
        // Arayüz ve Donanım Bağlantı Fonksiyonları
        void setListener(ICDListener* l) { listener = l; }
        void setTxFunction(void (*tx)(uint8_t*, uint16_t)) { txFunction = tx; }

        /* UDP her datagrami butun bir mesaj olarak tasir.
		* Bayt akisi mantigi (feedByte) UDP icin yanlis modeldi. */
	    void feedDatagram(const uint8_t* data, uint16_t len);

        // TX (Giden) Mesaj Fonksiyonları
        void sendModDegistirme(TlusMode mode);
        void sendSoftReset();
        void sendAcilSilme(EraseTarget target);
        void sendGvdSecimi(GvdNumber gvdNo);
        void sendGvdBilgiIstek();
        void sendTupDurumu(const uint8_t* tubeStates);

        /* Teshis sayaclari - gercek TLUS'a baglanirken vazgecilmez */
		struct Stats {
			uint32_t accepted;
			uint32_t badSize;
			uint32_t badSource;
			uint32_t lenMismatch;
			uint32_t lenBadForType;
			uint32_t badChecksum;
		} stats = {};
    };

} // namespace TLUS

#endif // TLUS_PROTOCOL_HPP
