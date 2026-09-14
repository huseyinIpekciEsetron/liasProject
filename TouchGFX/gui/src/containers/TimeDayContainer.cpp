#include <gui/containers/TimeDayContainer.hpp>

TimeDayContainer::TimeDayContainer()
{

}

void TimeDayContainer::initialize()
{
    TimeDayContainerBase::initialize();
}

void TimeDayContainer::updateTimeAndDate(uint8_t hour, uint8_t minute, uint8_t day, uint8_t month, uint16_t year)
{
	// 1. Önce eski yazıların bulunduğu alanı ekrandan temizle
	ClockText.invalidate();
	DayText.invalidate();

	// 2. Yeni değerleri Buffer'lara yazdır
	Unicode::snprintf(ClockTextBuffer, 10, "%02d:%02d", hour, minute);
	Unicode::snprintf(DayTextBuffer, 20, "%02d.%02d.%04d", day, month, year);


	// 4. Yeni metinleri ekrana çiz
	ClockText.invalidate();
	DayText.invalidate();
}
