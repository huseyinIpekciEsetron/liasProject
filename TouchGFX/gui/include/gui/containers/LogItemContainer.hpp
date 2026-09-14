#ifndef LOGITEMCONTAINER_HPP
#define LOGITEMCONTAINER_HPP

#include <gui_generated/containers/LogItemContainerBase.hpp>
#include <gui/model/Model.hpp>

class LogItemContainer : public LogItemContainerBase
{
public:
    LogItemContainer();
    virtual ~LogItemContainer() {}
    virtual void initialize();

    // YENİ: Saati ve metni içeri gömen fonksiyon
    void setupLog(uint8_t h, uint8_t m, uint8_t s, uint16_t textId, int param, Model::LogEventType type);
    void setHighlighted(bool isSelected);
protected:
};

#endif // LOGITEMCONTAINER_HPP
