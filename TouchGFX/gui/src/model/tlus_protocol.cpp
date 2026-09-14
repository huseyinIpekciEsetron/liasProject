/*
 * tlus_protocol.cpp
 *
 *  Created on: Aug 26, 2026
 *      Author: HUSEYIN
 */


#include <gui/model/tlus_protocol.hpp>

namespace TLUS {

uint8_t Protocol::calculateChecksum(uint8_t* data, uint16_t len) {
    uint16_t sum = 0;
    for(uint16_t i = 0; i < len; i++) sum += data[i];
    return (uint8_t)(sum & 0xFF); // Basit 8-bit checksum varsayımı (ICD'ye göre düzenlenebilir)
}

void Protocol::feedBuffer(uint8_t* buf, uint16_t len) {
    for(uint16_t i = 0; i < len; i++) {
        feedByte(buf[i]);
    }
}

void Protocol::feedByte(uint8_t b) {
    if(rxIndex >= sizeof(rxBuffer)) rxIndex = 0; // Taşma koruması
    rxBuffer[rxIndex++] = b;

    // Minimum header uzunluğu (Örn: Src + MsgId + Length(2) = 4 Byte)
    if (rxIndex >= 4) {
        uint16_t expectedLen = (rxBuffer[3] << 8) | rxBuffer[2]; // Little Endian Uzunluk

        if (expectedLen < 5 || expectedLen > 500) {
            // Hatalı paket, kaydırıp baştan dene
            rxBuffer[0] = rxBuffer[rxIndex-1];
            rxIndex = 1;
            return;
        }

        // Tüm paket geldiyse
        if (rxIndex == expectedLen) {
            uint8_t msgChecksum = rxBuffer[rxIndex - 1];
            uint8_t calcChecksum = calculateChecksum(rxBuffer, rxIndex - 1);

            if (msgChecksum == calcChecksum) {
                // Paket DOĞRU! İçeriğini ayrıştır.
                parsePacket(rxBuffer, expectedLen, rxBuffer[1]);
            }
            rxIndex = 0; // Yeni mesaj için sıfırla
        }
    }
}

// ICD'den Gelen Mesajları Okuma
void Protocol::parsePacket(uint8_t* payload, uint16_t length, uint8_t msgId) {
    if (!listener) return;

    switch(msgId) {
        case MSG_TARIH_ZAMAN: {
        	TimeData t;
			t.isValid = false;

			// 1. Geçerlilik Kontrolü (Byte 5-6, Bit 1-0)
			uint16_t word5_6 = (payload[4] << 8) | payload[5];
			uint8_t validity = word5_6 & 0x03; // Son iki bit
			if (validity == 0) break; // Geçersiz ise (0), veriyi işleme
			t.isValid = true;

			// Yardımcı Lambda Fonksiyonu: BCD Byte'ını Decimal'e Çevir
			auto bcd2dec = [](uint8_t bcd) -> uint8_t {
				return ((bcd >> 4) * 10) + (bcd & 0x0F);
			};

			// 2. Saat ve Dakika (Byte 7-8)
			t.hour   = bcd2dec(payload[6]);
			t.minute = bcd2dec(payload[7]);

			// 3. Saniye ve Yılın Günü (MSB Kısmı) (Byte 9-10)
			t.second = bcd2dec(payload[8]);

			// Yılın günü 12 bitlik bir sayıdır. (Byte 9'un alt 4 biti ve Byte 10)
			// Lütfen ICD tablosuna dikkat et: Yılın günü 10, 11 ve 12. bytelara dağılmış.
			// Yüzler ve Onlar basamağı Byte 10'da (BCD)
			// Birler basamağı Byte 11'in üst 4 bitinde (BCD)
			uint8_t dayOfYear_100_10 = bcd2dec(payload[9]); // Yüzler ve Onlar
			uint8_t dayOfYear_1      = (payload[10] >> 4) & 0x0F; // Birler
			uint16_t dayOfYear = (dayOfYear_100_10 * 10) + dayOfYear_1;

			// 4. Yıl Bilgisi (Byte 13-14) LSB 1 formatında Unsigned Short
			t.year = (payload[12] << 8) | payload[13];

			// 5. Yılın Gününü -> Ay ve Güne Çevirme Algoritması
			bool isLeapYear = ((t.year % 4 == 0 && t.year % 100 != 0) || (t.year % 400 == 0));
			uint8_t daysInMonth[] = { 31, (uint8_t)(isLeapYear ? 29 : 28), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

			t.month = 1;
			while (t.month <= 12 && dayOfYear > daysInMonth[t.month - 1]) {
				dayOfYear -= daysInMonth[t.month - 1];
				t.month++;
			}
			t.day = dayOfYear;
			if (t.day == 0) t.day = 1; // Güvenlik önlemi

			// Veriyi Model'e (Arayüze) gönder
			listener->onTimeDataParsed(t);
			break;
		}
        case MSG_TLUS_TEHDITLERI: {
            ThreatMessagePayload tMsg;

            // 1. Byte 5 ve 6: Geçerlilik ve Tehdit Sayısı
            uint16_t word5_6 = (payload[4] << 8) | payload[5];

            tMsg.isValid = (word5_6 >> 15) & 0x01; // Bit 15: Geçerlilik
            tMsg.count   = word5_6 & 0x3F;         // Bit 5-0: Tehdit Sayısı

            if (tMsg.count > 20) tMsg.count = 20;  // Taşma koruması (Max 20 tehdit)

            int offset = 6; // İlk tehdidin başladığı byte (0 index'e göre 6. byte, yani Bayt 7)

            // 2. Tehditleri Döngüyle Çıkar (Her tehdit 14 Byte uzunluğunda)
            for(int i = 0; i < tMsg.count; i++) {

				// Bayt 7-8: Sınıf, AgeOut, Öncelik
				uint16_t word7_8 = (payload[offset] << 8) | payload[offset+1];

				// YENİ HALİ: Enum'a cast ediyoruz
				tMsg.threats[i].threatClass = static_cast<ThreatClass>((word7_8 >> 12) & 0x0F);
				tMsg.threats[i].ageOut      = (word7_8 >> 11) & 0x01;
				tMsg.threats[i].priority    = (word7_8 >> 6)  & 0x1F;

				// Bayt 9-10: Tehdit Numarası
				tMsg.threats[i].threatNumber = (payload[offset+2] << 8) | payload[offset+3];

				// Bayt 11-12: Band ve Açı
				uint16_t word11_12 = (payload[offset+4] << 8) | payload[offset+5];

				// YENİ HALİ: Enum'a cast ediyoruz
				tMsg.threats[i].band  = static_cast<ThreatBand>((word11_12 >> 12) & 0x0F);

				uint16_t rawAngle     = word11_12 & 0x0FFF;
				tMsg.threats[i].angle = (float)rawAngle * 0.1f;

                // Bayt 13-16: Tehdit Tanımlama Kodu
                tMsg.threats[i].trackCode = (payload[offset+6] << 24) | (payload[offset+7] << 16) |
                                            (payload[offset+8] << 8)  | payload[offset+9];

                // Bayt 17-20: Darbe Tekrar Periyodu
                tMsg.threats[i].prf       = (payload[offset+10] << 24) | (payload[offset+11] << 16) |
                                            (payload[offset+12] << 8)  | payload[offset+13];

                offset += 14; // Bir sonraki tehdit 14 byte ileride
            }

            // Hazırlanan paketi dinleyiciye (Model'e) gönder
            listener->onThreatsParsed(tMsg);
            break;
        }
        case MSG_SISTEM_DURUMU:{
            SystemStatusPayload s;

            // 1. Genel Durum (Byte 5-6)
            uint16_t w5_6 = (payload[4] << 8) | payload[5];
            s.isValid          = (w5_6 >> 15) & 0x01;
            s.isPbitDone       = (w5_6 >> 14) & 0x01;
            s.sensor1GenelHata = (w5_6 >> 13) & 0x01;
            s.sensor2GenelHata = (w5_6 >> 12) & 0x01;
            s.sensor3GenelHata = (w5_6 >> 11) & 0x01;
            s.sensor4GenelHata = (w5_6 >> 10) & 0x01;
            s.islemciGenelHata = (w5_6 >> 5)  & 0x01;
            s.tlusState        = w5_6 & 0x03;

            // 2. İşlemci Birimi Hataları (Byte 7-12)
            uint16_t w7_8 = (payload[6] << 8) | payload[7];
            s.processorFaults.ramTesti           = (w7_8 >> 15) & 0x01;
            s.processorFaults.kaliciBellekTesti  = (w7_8 >> 11) & 0x01;
            s.processorFaults.bellekDosyasiTesti = (w7_8 >> 7)  & 0x01;
            s.processorFaults.nvsramTesti        = (w7_8 >> 3)  & 0x01;
            s.processorFaults.bellekDoluluk      = (w7_8 >> 2)  & 0x01;

            uint16_t w9_10 = (payload[8] << 8) | payload[9];
            s.processorFaults.seriKanal1 = (w9_10 >> 15) & 0x01;
            s.processorFaults.seriKanal2 = (w9_10 >> 11) & 0x01;
            s.processorFaults.seriKanal3 = (w9_10 >> 7)  & 0x01;
            s.processorFaults.seriKanal4 = (w9_10 >> 3)  & 0x01;

            uint16_t w11_12 = (payload[10] << 8) | payload[11];
            s.processorFaults.arayuzKarti          = (w11_12 >> 15) & 0x01;
            s.processorFaults.anaBesleme           = (w11_12 >> 11) & 0x01;
            s.processorFaults.islemciDurumuKapanma = (w11_12 >> 7)  & 0x01;
            s.processorFaults.gucKartiSeriKanal    = (w11_12 >> 3)  & 0x01;
            s.processorFaults.sicaklikEsikAsimi    = (w11_12 >> 2)  & 0x01;

            // 3. Sensör Hataları (Byte 15-30 arası, her sensör için 4 byte)
            for (int i = 0; i < 4; i++) {
                int offset = 14 + (i * 4); // Sensör 1 Byte 15'ten (index 14) başlar
                uint16_t msb = (payload[offset] << 8) | payload[offset+1];
                uint16_t lsb = (payload[offset+2] << 8) | payload[offset+3];

                s.sensorFaults[i].bant_I_II_Karti        = (msb >> 15) & 0x01;
                s.sensorFaults[i].bant_III_Sensor0     = (msb >> 11) & 0x01;
                s.sensorFaults[i].bant_III_Karti_Sensor1 = (msb >> 7)  & 0x01;
                s.sensorFaults[i].bant_III_Karti_Sensor2  = (msb >> 3)  & 0x01;

                s.sensorFaults[i].sensor_Birimi_Kontrol_Karti = (lsb >> 15) & 0x01;
                s.sensorFaults[i].kontrollu_Kapanma           = (lsb >> 13) & 0x01;
                s.sensorFaults[i].guc_Karti_Seri_Kanal        = (lsb >> 12) & 0x01;
                s.sensorFaults[i].basinc_Durumu               = (lsb >> 9)  & 0x01;
                s.sensorFaults[i].volt_3_7V                   = (lsb >> 8)  & 0x01;
                s.sensorFaults[i].volt_7_4V                   = (lsb >> 7)  & 0x01;
                s.sensorFaults[i].volt_16V                    = (lsb >> 6)  & 0x01;
                s.sensorFaults[i].volt_80V                   = (lsb >> 5)  & 0x01;
                s.sensorFaults[i].volt_neg7_4V               = (lsb >> 4)  & 0x01;
                s.sensorFaults[i].volt_neg3_7V                = (lsb >> 3)  & 0x01;
                s.sensorFaults[i].ana_Besleme                 = (lsb >> 2)  & 0x01;
                s.sensorFaults[i].sicaklik_Durumu             = (lsb >> 1)  & 0x01;
                s.sensorFaults[i].sicaklik_Sensoru            = (lsb >> 0)  & 0x01;
            }

            // 4. TLUS Modu, Heartbeat, GVD vb. Diğer Bilgiler
            uint16_t w47_48 = (payload[46] << 8) | payload[47];
			s.tlusMode = static_cast<TlusMode>(w47_48 & 0x0FFF);

			s.heartbeat = (payload[48] << 8) | payload[49];

			// Konfigürasyon Durumu (0xAA veya 0x01)
			s.configStatus = static_cast<ConfigAcceptance>(payload[51]);

			// Acil Silme Durumları
			uint16_t w53_54 = (payload[52] << 8) | payload[53];
			s.acilSilmeYazilim = static_cast<EraseStatus>((w53_54 >> 4) & 0x03);
			s.acilSilmeGVD     = static_cast<EraseStatus>((w53_54 >> 2) & 0x03);
			s.acilSilmeKayit   = static_cast<EraseStatus>((w53_54 >> 0) & 0x03);

			// GVD ve Disk Durumları
			uint16_t w55_56 = (payload[54] << 8) | payload[55];
			s.activeGvd              = (w55_56 >> 2) & 0x0F;
			s.isDiskOverwriteEnabled = (w55_56 >> 1) & 0x01;
			s.isBlankingActive       = (w55_56 >> 0) & 0x01;

			// Dosya Sistemi Durumu
			s.fileSysFormatting = static_cast<FileSystemStatus>(payload[56] & 0x01); // Byte 57, Bit 0

			// Hazırlanan bu devasa ve tertemiz paketi Model'e fırlat!
			listener->onSystemStatusParsed(s);
            break;
        }
        case MSG_GVD_BILGILERI: {
			GvdMessagePayload gvdMsg;

			// 1. GVD Geçerlilik Bilgisi (Byte 5-6)
			uint16_t w5_6 = (payload[4] << 8) | payload[5];

			// 2. 5 Adet GVD için döngü
			for (int i = 0; i < 5; i++) {
				// Bit 0: GVD1, Bit 1: GVD2 ... şeklinde gider
				gvdMsg.gvds[i].isValid = (w5_6 >> i) & 0x01;

				// Her GVD bloğu 56 Byte sürer (32 İsim + 24 Versiyon).
				// İlk blok Byte 7'den başlar (Array index: 6)
				int offset = 6 + (i * 56);

				// İsim Bilgisini Kopyala (32 Byte)
				for (int j = 0; j < 32; j++) {
					gvdMsg.gvds[i].name[j] = static_cast<char>(payload[offset + j]);
				}
				gvdMsg.gvds[i].name[32] = '\0'; // String bitiş karakteri (UI için şart)

				// Versiyon Bilgisini Kopyala (24 Byte)
				for (int j = 0; j < 24; j++) {
					gvdMsg.gvds[i].version[j] = static_cast<char>(payload[offset + 32 + j]);
				}
				gvdMsg.gvds[i].version[24] = '\0'; // String bitiş karakteri
			}

			// Ayrıştırılan metin paketini Model'e fırlat!
			listener->onGvdDataParsed(gvdMsg);
			break;
		}

    }
}
// ICD'ye Göre Giden Mesajların Oluşturulması
void Protocol::sendModDegistirme(TlusMode mode) {
    if (!txFunction) return;

    uint8_t txBuf[7];
    txBuf[0] = SRC_VYS_CB;         // Kaynak (0x01)
    txBuf[1] = MSG_MOD_DEGISTIRME; // Mesaj Tipi (0x06)
    txBuf[2] = 0x07;               // Uzunluk LSB (Byte 3)
    txBuf[3] = 0x00;               // Uzunluk MSB (Byte 4)

    // Enum değerini al, Bit 15-12 arasını rezerve (0) bırakarak 12-bit maskele
    uint16_t modeData = static_cast<uint16_t>(mode) & 0x0FFF;

    txBuf[4] = (modeData & 0xFF);         // Byte 5 (LSB)
    txBuf[5] = ((modeData >> 8) & 0xFF);  // Byte 6 (MSB)

    txBuf[6] = calculateChecksum(txBuf, 6); // Byte 7 (Sağlama Toplamı)

    txFunction(txBuf, 7); // Byte'ları Donanıma Fırlat!
}

void Protocol::sendSoftReset() {
    if (!txFunction) return;
    uint8_t txBuf[7] = {SRC_VYS_CB, MSG_SOFT_RESET, 0x07, 0x00, 0x00, 0x00, 0};
    txBuf[6] = calculateChecksum(txBuf, 6);
    txFunction(txBuf, 7);
}

void Protocol::sendAcilSilme(EraseTarget target) {
    if (!txFunction) return;

    uint8_t txBuf[7];
    txBuf[0] = SRC_VYS_CB;       // Kaynak (0x01)
    txBuf[1] = MSG_ACIL_SILME;   // Mesaj Tipi (0x0E)
    txBuf[2] = 0x07;             // Uzunluk LSB
    txBuf[3] = 0x00;             // Uzunluk MSB

    // Acil Silme Komutları (Byte 5 ve 6)
    // target değişkeni zaten Bit 0, 1 ve 2'ye uygun şekilde ayarlandı
    uint16_t commandWord = static_cast<uint16_t>(target);

    txBuf[4] = (commandWord & 0xFF);         // Byte 5 (LSB)
    txBuf[5] = ((commandWord >> 8) & 0xFF);  // Byte 6 (MSB)

    // Sağlama Toplamı
    txBuf[6] = calculateChecksum(txBuf, 6);

    // Donanıma Gönder
    txFunction(txBuf, 7);
}

// 0x0D - GVD Seçimi Mesajı (9 Byte)
void Protocol::sendGvdSecimi(GvdNumber gvdNo) {
    if (!txFunction) return;

    uint8_t txBuf[9];
    txBuf[0] = SRC_VYS_CB;       // Kaynak (0x01)
    txBuf[1] = MSG_GVD_SECIMI;   // Mesaj Tipi (0x0D)
    txBuf[2] = 0x09;             // Uzunluk LSB (9 Byte)
    txBuf[3] = 0x00;             // Uzunluk MSB

    // Byte 5-6: Geçerlilik (Sadece Bit 0 = 1 olacak)
    uint16_t validity = 0x0001;
    txBuf[4] = (validity & 0xFF);         // LSB
    txBuf[5] = ((validity >> 8) & 0xFF);  // MSB

    // Enum'ı uint16_t'ye çevir ve Byte 7-8'e yerleştir
    uint16_t gvdValue = static_cast<uint16_t>(gvdNo);
    txBuf[6] = (gvdValue & 0xFF);         // LSB
    txBuf[7] = ((gvdValue >> 8) & 0xFF);  // MSB

    // Byte 9: Sağlama Toplamı (Checksum)
    txBuf[8] = calculateChecksum(txBuf, 8);

    // 9 Byte'ı Donanıma Fırlat!
    txFunction(txBuf, 9);
}
// 0x26 - GVD Bilgileri İsteği (5 Byte)
void Protocol::sendGvdBilgiIstek() {
    if (!txFunction) return;

    uint8_t txBuf[5];
    txBuf[0] = SRC_VYS_CB;          // Kaynak (0x01)
    txBuf[1] = MSG_GVD_BILGI_ISTEK; // Mesaj Tipi (0x26)
    txBuf[2] = 0x05;                // Uzunluk LSB (5 Byte)
    txBuf[3] = 0x00;                // Uzunluk MSB

    // Byte 5: Sağlama Toplamı (Checksum)
    txBuf[4] = calculateChecksum(txBuf, 4);

    // 5 Byte'ı Donanıma Fırlat!
    txFunction(txBuf, 5);
}

// 0x30 - Özel Tüp Durumu Mesajı (9 Byte)
void Protocol::sendTupDurumu(const uint8_t* tubeStates) {
    if (!txFunction) return;

    uint8_t txBuf[9];
    txBuf[0] = SRC_VYS_CB;       // Kaynak (0x01)
    txBuf[1] = MSG_TUP_DURUMU;   // Mesaj Tipi (0x30)
    txBuf[2] = 0x09;             // Uzunluk LSB (9 Byte)
    txBuf[3] = 0x00;             // Uzunluk MSB

    // 16 tüpü 4'lü gruplar halinde Byte 5, 6, 7 ve 8'e paketle
    for(int i = 0; i < 4; i++) {
        txBuf[4 + i] = (tubeStates[i * 4]     & 0x03)       | // Tüp 1, 5, 9, 13
                       ((tubeStates[i * 4 + 1] & 0x03) << 2) | // Tüp 2, 6, 10, 14
                       ((tubeStates[i * 4 + 2] & 0x03) << 4) | // Tüp 3, 7, 11, 15
                       ((tubeStates[i * 4 + 3] & 0x03) << 6);  // Tüp 4, 8, 12, 16
    }

    // Byte 9: Sağlama Toplamı
    txBuf[8] = calculateChecksum(txBuf, 8);

    // Donanıma Gönder
    txFunction(txBuf, 9);
}
// Diğer gönderme fonksiyonları buraya eklenecek...
}
