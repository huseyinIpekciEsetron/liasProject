#include "buzzer_driver.h"
#include "main.h"

/* Buzzer'in dahili 1 Hz kesicisinin ON fazi ~500 ms.
 * Hicbir ON adimi bunun altinda kalmali ki dahili kesici
 * hic devreye girmesin ve ritmi tamamen biz belirleyelim. */
#define BUZZER_MAX_ON_MS   300U

/* Yuksek DAC = FB'ye cok akim = dusuk cikis gerilimi = dusuk ses.
 * Seviye 1 = ~6 V (datasheet alt siniri 5 V), seviye 5 = ~23 V. */
static const uint16_t buzzer_dac_table[BUZZER_VOL_MAX + 1U] = {
    3850,   /* 0 - kullanilmaz, gate ile susturulur */
    3850,   /* 1 - ~6.0 V  / ~82 dB */
    3450,   /* 2 - ~8.0 V  / ~88 dB */
    2690,   /* 3 - ~12 V   / ~93 dB */
    1900,   /* 4 - ~16 V   / ~96 dB */
     500    /* 5 - ~23 V   / ~99 dB */
};

/* Desenler: [0]=ON, [1]=OFF, [2]=ON, [3]=OFF ... dongusel.
 * Eleman sayisi mutlaka cift olmali. */
static const uint16_t pat_low[]    = { 150, 1850 };
static const uint16_t pat_medium[] = { 100,  120, 100, 680 };
static const uint16_t pat_high[]   = {  80,   80 };

typedef struct {
    const uint16_t *steps;
    uint8_t         stepCount;
    uint8_t         minVolume;   /* bu seviyede zorunlu alt sinir */
} BuzzerPattern_t;

static const BuzzerPattern_t buzzer_patterns[ALARM_LEVEL_COUNT] = {
    [ALARM_NONE]   = { NULL,        0U, 0U },
    [ALARM_LOW]    = { pat_low,     2U, 1U },
    [ALARM_MEDIUM] = { pat_medium,  4U, 2U },
    [ALARM_HIGH]   = { pat_high,    2U, 4U }
};

static DAC_HandleTypeDef *buzzer_dac;
static uint32_t     buzzer_dac_channel;
static AlarmLevel_t current_alarm = ALARM_NONE;
static uint8_t      userVolume    = 1U;
static uint8_t      patStep       = 0U;
static uint32_t     patStepStart  = 0U;

/* --- ic yardimcilar --------------------------------------------- */

static void Buzzer_Gate(bool on)
{
    HAL_GPIO_WritePin(BUZZER_MUTE_GPIO_Port, BUZZER_MUTE_Pin,
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void Buzzer_ApplyVolume(void)
{
    uint8_t v = userVolume;
    uint8_t minV = buzzer_patterns[current_alarm].minVolume;

    if (v < minV)              v = minV;
    if (v > BUZZER_VOL_MAX)    v = BUZZER_VOL_MAX;
    if (v < BUZZER_VOL_MIN)    v = BUZZER_VOL_MIN;

    HAL_DAC_SetValue(buzzer_dac, buzzer_dac_channel,
                     DAC_ALIGN_12B_R, buzzer_dac_table[v]);
}

/* --- API -------------------------------------------------------- */

void Buzzer_Init(DAC_HandleTypeDef *hdac, uint32_t channel)
{
    buzzer_dac         = hdac;
    buzzer_dac_channel = channel;

    /* 1. DAC once baslar ve en dusuk cikisa (en yuksek kod) surulur.
     *    BUZZER_EN'den ONCE olmali; aksi halde bir an FB'ye enjeksiyon
     *    olmadan ~18 V uretilir. */
    HAL_DAC_Start(buzzer_dac, buzzer_dac_channel);
    HAL_DAC_SetValue(buzzer_dac, buzzer_dac_channel,
                     DAC_ALIGN_12B_R, buzzer_dac_table[BUZZER_VOL_MIN]);

    /* 2. Sessiz basla */
    Buzzer_Gate(false);

    /* 3. Boost converter acilir ve BIR DAHA KAPATILMAZ */
    HAL_GPIO_WritePin(BUZZER_EN_GPIO_Port, BUZZER_EN_Pin, GPIO_PIN_SET);

    current_alarm = ALARM_NONE;
    userVolume    = 1U;
    patStep       = 0U;
}

void Buzzer_SetVolume(uint8_t user_level)
{
    if (user_level > BUZZER_VOL_MAX) user_level = BUZZER_VOL_MAX;
    userVolume = user_level;

    if (current_alarm != ALARM_NONE) {
        Buzzer_ApplyVolume();
    }
}

void Buzzer_SetAlarmLevel(AlarmLevel_t level)
{
    if (level >= ALARM_LEVEL_COUNT) return;
    if (level == current_alarm)     return;   /* her tick cagrilabilir */

    current_alarm = level;
    patStep       = 0U;
    patStepStart  = HAL_GetTick();

    if (level == ALARM_NONE) {
        Buzzer_Gate(false);
    } else {
        Buzzer_ApplyVolume();
        Buzzer_Gate(true);      /* her desen ON adimiyla baslar */
    }
}

void Buzzer_ProcessHandler(void)
{
    if (current_alarm == ALARM_NONE) return;

    const BuzzerPattern_t *p = &buzzer_patterns[current_alarm];
    uint32_t now = HAL_GetTick();

    if ((now - patStepStart) >= (uint32_t)p->steps[patStep])
    {
        patStepStart = now;
        patStep++;
        if (patStep >= p->stepCount) patStep = 0U;

        /* cift index = ON adimi, tek index = OFF adimi */
        Buzzer_Gate((patStep & 1U) == 0U);
    }
}
