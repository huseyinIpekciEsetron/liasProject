#ifndef TIMEDAYCONTAINER_HPP
#define TIMEDAYCONTAINER_HPP

#include <gui_generated/containers/TimeDayContainerBase.hpp>

class TimeDayContainer : public TimeDayContainerBase
{
public:
    TimeDayContainer();
    virtual ~TimeDayContainer() {}

    virtual void initialize();

    // Zaman ve Tarih Güncelleme Fonksiyonu
	void updateTimeAndDate(uint8_t hour, uint8_t minute, uint8_t day, uint8_t month, uint16_t year);
protected:
};

#endif // TIMEDAYCONTAINER_HPP
