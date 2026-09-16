/*
 * tlus_protocol.cpp
 *
 *  Created on: Aug 26, 2026
 *      Author: HUSEYIN
 */


#include <gui/model/tlus_protocol.hpp>

namespace TLUS {

uint8_t Protocol::buildChecksum(const uint8_t* data, uint16_t lenWithoutCks)
{
    uint8_t sum = 0U;
    for (uint16_t i = 0U; i < lenWithoutCks; i++) sum += data[i];
    return (uint8_t)((uint8_t)(~sum) + 1U);      /* ICD: 2's complement */
}

void Protocol::feedDatagram(const uint8_t* data, uint16_t len)
{
    /* 1. Asgari cerceve: kaynak(1) + tip(1) + uzunluk(2) + saglama(1) */
    if (data == nullptr || len < 5U || len > 512U) { stats.badSize++; return; }

    /* 2. Kaynak: yalnizca TLUS kabul edilir.
     *    Ayni porta yayin yapan baska cihazlari eler. */
    if (data[0] != SRC_TLUS) { stats.badSource++; return; }

    /* 3. Beyan edilen uzunluk - BIG-ENDIAN (ICD'deki tum 16-bit alanlar boyle) */
    uint16_t declaredLen = ((uint16_t)data[2] << 8) | (uint16_t)data[3];
    if (declaredLen != len) { stats.lenMismatch++; return; }

    /* 4. Mesaj tipinin gerektirdigi uzunluk.
     *    Bu gectikten sonra parsePacket her alanin var oldugunu BILIR;
     *    sabit offsetlerden sinir kontrolsuz okumak artik guvenli. */
    if (expectedLenFor(data[1], data, len) != len) { stats.lenBadForType++; return; }

    /* 5. Saglama: ICD'ye gore cks = -(bayt1..baytN-1 toplami),
     *    dolayisiyla SON BAYT DAHIL toplam sifir olmali. */
    uint8_t sum = 0U;
    for (uint16_t i = 0U; i < len; i++) sum += data[i];
    if (sum != 0U) { stats.badChecksum++; return; }

    stats.accepted++;
    parsePacket(data, len);
}

uint16_t Protocol::expectedLenFor(uint8_t msgId, const uint8_t* d, uint16_t len)
{
    switch (msgId)
    {
        case MSG_TLUS_TEHDITLERI:
        {
            /* ICD: 7 bayt (tehdit yok) ... 287 bayt (20 tehdit) = 7 + count*14 */
            if (len < 7U) return 0U;
            uint8_t cnt = d[5] & 0x3FU;          /* bayt 6, bit 5-0 */
            if (cnt > 20U) return 0U;
            return (uint16_t)(7U + (uint16_t)cnt * 14U);
        }
        case MSG_SISTEM_DURUMU:  return 60U;
        case MSG_TARIH_ZAMAN:    return 15U;
        case MSG_GVD_BILGILERI:  return 287U;
        default:                 return 0U;      /* bilinmeyen tip -> reddet */
    }
}

// ICD'den Gelen Mesajları Okuma
void Protocol::parsePacket(const uint8_t* msg, uint16_t len){
    if (!listener) return;

    /* NOT: Uzunluk dogrulamasi feedDatagram icinde tamamlandi.
     * Buraya gelen her mesajin tum alanlari mevcuttur, bu yuzden
     * asagidaki sabit offsetler guvenlidir. */
    (void)len;

    switch(msg[1]) {
        case MSG_TARIH_ZAMAN: {
        	TimeData t;
			t.isValid = false;

			// 1. Geçerlilik Kontrolü (Byte 5-6, Bit 1-0)
			uint16_t word5_6 = (msg[4] << 8) | msg[5];
			uint8_t validity = word5_6 & 0x03; // Son iki bit
			if (validity == 0) {
				/* TLUS saatinin gecersiz oldugunu bildiriyor. Sessizce yok
				 * saymak yerine arayuze haber ver; aksi halde ekranda son
				 * gecerli saat donup kalir ve operator onu canli sanir. */
				listener->onTimeDataParsed(t);   // t.isValid == false
				break;
			}
			t.isValid = true;

			// Yardımcı Lambda Fonksiyonu: BCD Byte'ını Decimal'e Çevir
			auto bcd2dec = [](uint8_t bcd) -> uint8_t {
				return ((bcd >> 4) * 10) + (bcd & 0x0F);
			};

			// 2. Saat ve Dakika (Byte 7-8)
			t.hour   = bcd2dec(msg[6]);
			t.minute = bcd2dec(msg[7]);

			// 3. Saniye ve Yılın Günü (MSB Kısmı) (Byte 9-10)
			t.second = bcd2dec(msg[8]);

			// Yılın günü 12 bitlik bir sayıdır. (Byte 9'un alt 4 biti ve Byte 10)
			// Lütfen ICD tablosuna dikkat et: Yılın günü 10, 11 ve 12. bytelara dağılmış.
			// Yüzler ve Onlar basamağı Byte 10'da (BCD)
			// Birler basamağı Byte 11'in üst 4 bitinde (BCD)
			uint8_t dayOfYear_100_10 = bcd2dec(msg[9]); // Yüzler ve Onlar
			uint8_t dayOfYear_1      = (msg[10] >> 4) & 0x0F; // Birler
			uint16_t dayOfYear = (dayOfYear_100_10 * 10) + dayOfYear_1;

			// 4. Yıl Bilgisi (Byte 13-14) LSB 1 formatında Unsigned Short
			t.year = (msg[12] << 8) | msg[13];

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
            uint16_t word5_6 = (msg[4] << 8) | msg[5];

            tMsg.isValid = (word5_6 >> 15) & 0x01; // Bit 15: Geçerlilik
            tMsg.count   = word5_6 & 0x3F;         // Bit 5-0: Tehdit Sayısı

            if (tMsg.count > 20) tMsg.count = 20;  // Taşma koruması (Max 20 tehdit)

            int offset = 6; // İlk tehdidin başladığı byte (0 index'e göre 6. byte, yani Bayt 7)

            // 2. Tehditleri Döngüyle Çıkar (Her tehdit 14 Byte uzunluğunda)
            for(int i = 0; i < tMsg.count; i++) {

				// Bayt 7-8: Sınıf, AgeOut, Öncelik
				uint16_t word7_8 = (msg[offset] << 8) | msg[offset+1];

				// YENİ HALİ: Enum'a cast ediyoruz
				tMsg.threats[i].threatClass = static_cast<ThreatClass>((word7_8 >> 12) & 0x0F);
				tMsg.threats[i].ageOut      = (word7_8 >> 11) & 0x01;
				tMsg.threats[i].priority    = (word7_8 >> 6)  & 0x1F;

				// Bayt 9-10: Tehdit Numarası
				tMsg.threats[i].threatNumber = (msg[offset+2] << 8) | msg[offset+3];

				// Bayt 11-12: Band ve Açı
				uint16_t word11_12 = (msg[offset+4] << 8) | msg[offset+5];

				// YENİ HALİ: Enum'a cast ediyoruz
				tMsg.threats[i].band  = static_cast<ThreatBand>((word11_12 >> 12) & 0x0F);

				uint16_t rawAngle     = word11_12 & 0x0FFF;
				tMsg.threats[i].angle = (float)rawAngle * 0.1f;

                // Bayt 13-16: Tehdit Tanımlama Kodu
                tMsg.threats[i].trackCode = (msg[offset+6] << 24) | (msg[offset+7] << 16) |
                                            (msg[offset+8] << 8)  | msg[offset+9];

                // Bayt 17-20: Darbe Tekrar Periyodu
                tMsg.threats[i].prf       = (msg[offset+10] << 24) | (msg[offset+11] << 16) |
                                            (msg[offset+12] << 8)  | msg[offset+13];

                offset += 14; // Bir sonraki tehdit 14 byte ileride
            }

            // Hazırlanan paketi dinleyiciye (Model'e) gönder
            listener->onThreatsParsed(tMsg);
            break;
        }
        case MSG_SISTEM_DURUMU:{
            SystemStatusPayload s;

            // 1. Genel Durum (Byte 5-6)
            uint16_t w5_6 = (msg[4] << 8) | msg[5];
            s.isValid          = (w5_6 >> 15) & 0x01;
            s.isPbitDone       = (w5_6 >> 14) & 0x01;
            s.sensor1GenelHata = (w5_6 >> 13) & 0x01;
            s.sensor2GenelHata = (w5_6 >> 12) & 0x01;
            s.sensor3GenelHata = (w5_6 >> 11) & 0x01;
            s.sensor4GenelHata = (w5_6 >> 10) & 0x01;
            s.islemciGenelHata = (w5_6 >> 5)  & 0x01;
            s.tlusState        = w5_6 & 0x03;

            // 2. İşlemci Birimi Hataları (Byte 7-12)
            uint16_t w7_8 = (msg[6] << 8) | msg[7];
            s.processorFaults.ramTesti           = (w7_8 >> 15) & 0x01;
            s.processorFaults.kaliciBellekTesti  = (w7_8 >> 11) & 0x01;
            s.processorFaults.bellekDosyasiTesti = (w7_8 >> 7)  & 0x01;
            s.processorFaults.nvsramTesti        = (w7_8 >> 3)  & 0x01;
            s.processorFaults.bellekDoluluk      = (w7_8 >> 2)  & 0x01;

            uint16_t w9_10 = (msg[8] << 8) | msg[9];
            s.processorFaults.seriKanal1 = (w9_10 >> 15) & 0x01;
            s.processorFaults.seriKanal2 = (w9_10 >> 11) & 0x01;
            s.processorFaults.seriKanal3 = (w9_10 >> 7)  & 0x01;
            s.processorFaults.seriKanal4 = (w9_10 >> 3)  & 0x01;

            uint16_t w11_12 = (msg[10] << 8) | msg[11];
            s.processorFaults.arayuzKarti          = (w11_12 >> 15) & 0x01;
            s.processorFaults.anaBesleme           = (w11_12 >> 11) & 0x01;
            s.processorFaults.islemciDurumuKapanma = (w11_12 >> 7)  & 0x01;
            s.processorFaults.gucKartiSeriKanal    = (w11_12 >> 3)  & 0x01;
            s.processorFaults.sicaklikEsikAsimi    = (w11_12 >> 2)  & 0x01;

            // 3. Sensör Hataları (Byte 15-30 arası, her sensör için 4 byte)
            for (int i = 0; i < 4; i++) {
                int offset = 14 + (i * 4); // Sensör 1 Byte 15'ten (index 14) başlar
                uint16_t msb = (msg[offset] << 8) | msg[offset+1];
                uint16_t lsb = (msg[offset+2] << 8) | msg[offset+3];

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
            uint16_t w47_48 = (msg[46] << 8) | msg[47];
			s.tlusMode = static_cast<TlusMode>(w47_48 & 0x0FFF);

			s.heartbeat = (msg[48] << 8) | msg[49];

			// Konfigürasyon Durumu (0xAA veya 0x01)
			s.configStatus = static_cast<ConfigAcceptance>(msg[51]);

			// Acil Silme Durumları
			uint16_t w53_54 = (msg[52] << 8) | msg[53];
			s.acilSilmeYazilim = static_cast<EraseStatus>((w53_54 >> 4) & 0x03);
			s.acilSilmeGVD     = static_cast<EraseStatus>((w53_54 >> 2) & 0x03);
			s.acilSilmeKayit   = static_cast<EraseStatus>((w53_54 >> 0) & 0x03);

			// GVD ve Disk Durumları
			uint16_t w55_56 = (msg[54] << 8) | msg[55];
			s.activeGvd              = (w55_56 >> 2) & 0x0F;
			s.isDiskOverwriteEnabled = (w55_56 >> 1) & 0x01;
			s.isBlankingActive       = (w55_56 >> 0) & 0x01;

			// Dosya Sistemi Durumu
			s.fileSysFormatting = static_cast<FileSystemStatus>(msg[56] & 0x01); // Byte 57, Bit 0

			// Hazırlanan bu devasa ve tertemiz paketi Model'e fırlat!
			listener->onSystemStatusParsed(s);
            break;
        }
        case MSG_GVD_BILGILERI: {
			GvdMessagePayload gvdMsg;

			// 1. GVD Geçerlilik Bilgisi (Byte 5-6)
			uint16_t w5_6 = (msg[4] << 8) | msg[5];

			// 2. 5 Adet GVD için döngü
			for (int i = 0; i < 5; i++) {
				// Bit 0: GVD1, Bit 1: GVD2 ... şeklinde gider
				gvdMsg.gvds[i].isValid = (w5_6 >> i) & 0x01;

				// Her GVD bloğu 56 Byte sürer (32 İsim + 24 Versiyon).
				// İlk blok Byte 7'den başlar (Array index: 6)
				int offset = 6 + (i * 56);

				// İsim Bilgisini Kopyala (32 Byte)
				for (int j = 0; j < 32; j++) {
					gvdMsg.gvds[i].name[j] = static_cast<char>(msg[offset + j]);
				}
				gvdMsg.gvds[i].name[32] = '\0'; // String bitiş karakteri (UI için şart)

				// Versiyon Bilgisini Kopyala (24 Byte)
				for (int j = 0; j < 24; j++) {
					gvdMsg.gvds[i].version[j] = static_cast<char>(msg[offset + 32 + j]);
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
    txBuf[2] = 0x00;               // Uzunluk MSB (Byte 3) - BIG-ENDIAN
    txBuf[3] = 0x07;               // Uzunluk LSB (Byte 4)

    // Enum değerini al, Bit 15-12 arasını rezerve (0) bırakarak 12-bit maskele
    uint16_t modeData = static_cast<uint16_t>(mode) & 0x0FFF;

    txBuf[4] = ((modeData >> 8) & 0xFF);  // Byte 5 (MSB) - BIG-ENDIAN
    txBuf[5] = (modeData & 0xFF);         // Byte 6 (LSB)

    txBuf[6] = buildChecksum(txBuf, 6); // Byte 7 (Sağlama Toplamı)

    txFunction(txBuf, 7); // Byte'ları Donanıma Fırlat!
}

void Protocol::sendSoftReset() {
    if (!txFunction) return;
    uint8_t txBuf[7] = {SRC_VYS_CB, MSG_SOFT_RESET, 0x00, 0x07, 0x00, 0x00, 0};
    txBuf[6] = buildChecksum(txBuf, 6);
    txFunction(txBuf, 7);
}

void Protocol::sendAcilSilme(EraseTarget target) {
    if (!txFunction) return;

    uint8_t txBuf[7];
    txBuf[0] = SRC_VYS_CB;       // Kaynak (0x01)
    txBuf[1] = MSG_ACIL_SILME;   // Mesaj Tipi (0x0E)
    txBuf[2] = 0x00;             // Uzunluk MSB - BIG-ENDIAN
    txBuf[3] = 0x07;             // Uzunluk LSB

    // Acil Silme Komutları (Byte 5 ve 6)
    // target değişkeni zaten Bit 0, 1 ve 2'ye uygun şekilde ayarlandı
    uint16_t commandWord = static_cast<uint16_t>(target);

    txBuf[4] = ((commandWord >> 8) & 0xFF);  // Byte 5 (MSB) - BIG-ENDIAN
    txBuf[5] = (commandWord & 0xFF);         // Byte 6 (LSB)

    // Sağlama Toplamı
    txBuf[6] = buildChecksum(txBuf, 6);

    // Donanıma Gönder
    txFunction(txBuf, 7);
}

// 0x0D - GVD Seçimi Mesajı (9 Byte)
void Protocol::sendGvdSecimi(GvdNumber gvdNo) {
    if (!txFunction) return;

    uint8_t txBuf[9];
    txBuf[0] = SRC_VYS_CB;       // Kaynak (0x01)
    txBuf[1] = MSG_GVD_SECIMI;   // Mesaj Tipi (0x0D)
    txBuf[2] = 0x00;             // Uzunluk MSB - BIG-ENDIAN
    txBuf[3] = 0x09;             // Uzunluk LSB (9 Byte)

    // Byte 5-6: Geçerlilik (Sadece Bit 0 = 1 olacak)
    uint16_t validity = 0x0001;
    txBuf[4] = ((validity >> 8) & 0xFF);  // Byte 5 (MSB) - BIG-ENDIAN
    txBuf[5] = (validity & 0xFF);         // Byte 6 (LSB)

    // Enum'ı uint16_t'ye çevir ve Byte 7-8'e yerleştir
    uint16_t gvdValue = static_cast<uint16_t>(gvdNo);
    txBuf[6] = ((gvdValue >> 8) & 0xFF);  // Byte 7 (MSB) - BIG-ENDIAN
    txBuf[7] = (gvdValue & 0xFF);         // Byte 8 (LSB)

    // Byte 9: Sağlama Toplamı (Checksum)
    txBuf[8] = buildChecksum(txBuf, 8);

    // 9 Byte'ı Donanıma Fırlat!
    txFunction(txBuf, 9);
}
// 0x26 - GVD Bilgileri İsteği (5 Byte)
void Protocol::sendGvdBilgiIstek() {
    if (!txFunction) return;

    uint8_t txBuf[5];
    txBuf[0] = SRC_VYS_CB;          // Kaynak (0x01)
    txBuf[1] = MSG_GVD_BILGI_ISTEK; // Mesaj Tipi (0x26)
    txBuf[2] = 0x00;                // Uzunluk MSB - BIG-ENDIAN
    txBuf[3] = 0x05;                // Uzunluk LSB (5 Byte)

    // Byte 5: Sağlama Toplamı (Checksum)
    txBuf[4] = buildChecksum(txBuf, 4);

    // 5 Byte'ı Donanıma Fırlat!
    txFunction(txBuf, 5);
}

// 0x30 - Özel Tüp Durumu Mesajı (9 Byte)
void Protocol::sendTupDurumu(const uint8_t* tubeStates) {
    if (!txFunction) return;

    uint8_t txBuf[9];
    txBuf[0] = SRC_VYS_CB;       // Kaynak (0x01)
    txBuf[1] = MSG_TUP_DURUMU;   // Mesaj Tipi (0x30)
    txBuf[2] = 0x00;             // Uzunluk MSB - BIG-ENDIAN
    txBuf[3] = 0x09;             // Uzunluk LSB (9 Byte)

    // 16 tüpü 4'lü gruplar halinde Byte 5, 6, 7 ve 8'e paketle
    for(int i = 0; i < 4; i++) {
        txBuf[4 + i] = (tubeStates[i * 4]     & 0x03)       | // Tüp 1, 5, 9, 13
                       ((tubeStates[i * 4 + 1] & 0x03) << 2) | // Tüp 2, 6, 10, 14
                       ((tubeStates[i * 4 + 2] & 0x03) << 4) | // Tüp 3, 7, 11, 15
                       ((tubeStates[i * 4 + 3] & 0x03) << 6);  // Tüp 4, 8, 12, 16
    }

    // Byte 9: Sağlama Toplamı
    txBuf[8] = buildChecksum(txBuf, 8);

    // Donanıma Gönder
    txFunction(txBuf, 9);
}
// Diğer gönderme fonksiyonları buraya eklenecek...
}
