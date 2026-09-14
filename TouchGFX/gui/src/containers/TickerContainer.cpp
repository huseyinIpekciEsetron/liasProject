#include <gui/containers/TickerContainer.hpp>
#include <touchgfx/Utils.hpp>

TickerContainer::TickerContainer()
{
}

void TickerContainer::initialize()
{
    TickerContainerBase::initialize();

    // Ekran ilk açıldığında yazının alpha'sını 0 yap,
    // ama Container'ın tamamı (kutu dahil) zaten Screen1View'da "setVisible(false)" yapılıyor.
    faultTicker.setAlpha(0);
}

void TickerContainer::updateFaultText(touchgfx::TypedTextId textId, int tubeIndex)
{
    // tubeIndex -1 değilse (yani bir sensör/mühimmat numarası varsa) %d yerine sayıyı göm:
    if (tubeIndex == -1) {
        touchgfx::Unicode::strncpy(faultTickerBuffer, touchgfx::TypedText(textId).getText(), FAULTTICKER_SIZE);
    } else {
        touchgfx::Unicode::snprintf(faultTickerBuffer, FAULTTICKER_SIZE, touchgfx::TypedText(textId).getText(), tubeIndex);
    }

    faultTicker.invalidate();
}

void TickerContainer::updateFaultCount(int count)
{
    // Üçgenin içindeki sayıyı güncelle
    Unicode::snprintf(faultCountTextBuffer, 4, "%d", count);
    faultCountText.invalidate();
}

void TickerContainer::setFadeCallback(touchgfx::GenericCallback<const touchgfx::FadeAnimator<touchgfx::TextAreaWithOneWildcard>&>& callback)
{
    faultTicker.setFadeAnimationEndedAction(callback);
}

void TickerContainer::startFadeAnimation(uint8_t endAlpha, uint16_t duration)
{
    // SADECE YAZI SÖNÜP BELİRSİN (Arka plan kutusu ve üçgen hep sabit kalsın)
    faultTicker.startFadeAnimation(endAlpha, duration);
}

void TickerContainer::setAlpha(uint8_t alpha)
{
    faultTicker.setAlpha(alpha);
    faultTicker.invalidate();
}

uint8_t TickerContainer::getAlpha()
{
    return faultTicker.getAlpha();
}

void TickerContainer::setBoxVisible(bool visible)
{
    // Arka planı, üçgeni ve tüm container parçalarını komple görünür/görünmez yapar
    boxWithBorder1.setVisible(visible);
    faultCountText.setVisible(visible);
    // Varsa arka plan SVG üçgeninin adını da buraya ekleyebilirsin:
    // warningTriangleImage.setVisible(visible);

    boxWithBorder1.invalidate();
}

void TickerContainer::invalidateBox()
{
    boxWithBorder1.invalidate();
}
