#ifndef TICKERCONTAINER_HPP
#define TICKERCONTAINER_HPP

#include <gui_generated/containers/TickerContainerBase.hpp>

class TickerContainer : public TickerContainerBase
{
public:
    TickerContainer();
    virtual ~TickerContainer() {}

    virtual void initialize();

    // Sönme ve Sayaç Kapsülleme Metotları
    void updateFaultText(touchgfx::TypedTextId textId, int tubeIndex);
    void updateFaultCount(int count);

    // Animasyon Metotları
    void setFadeCallback(touchgfx::GenericCallback<const touchgfx::FadeAnimator<touchgfx::TextAreaWithOneWildcard>&>& callback);
    void startFadeAnimation(uint8_t endAlpha, uint16_t duration);
    void setAlpha(uint8_t alpha);
    uint8_t getAlpha();

    void setBoxVisible(bool visible);
    void invalidateBox();
};

#endif // TICKERCONTAINER_HPP
