#include <gui/containers/ButtonContainer.hpp>
#include <images/SVGDatabase.hpp>

ButtonContainer::ButtonContainer()
{

}

void ButtonContainer::initialize()
{
    ButtonContainerBase::initialize();
}
void ButtonContainer::updateButtonVisuals(uint32_t state, uint32_t override_active_btn)
{
    // SOL TUŞLAR
    box_sol_1.setColor(((state & BTN_BLACKOUT)        || (override_active_btn & BTN_BLACKOUT))        ? (0xFF5100U) : (0xFFFFFFU));
    box_sol_2.setColor(((state & BTN_HOME)            || (override_active_btn & BTN_HOME))            ? (0xFF5100U) : (0xFFFFFFU));
    box_sol_3.setColor(((state & BTN_BRIGHTNESS_MENU) || (override_active_btn & BTN_BRIGHTNESS_MENU)) ? (0xFF5100U) : (0xFFFFFFU));
    box_sol_4.setColor(((state & BTN_UP)            || (override_active_btn & BTN_UP))            	  ? (0xFF5100U) : (0xFFFFFFU));

    // SAĞ TUŞLAR
    box_sag_1.setColor(((state & BTN_MENU)        	  || (override_active_btn & BTN_MENU))        	  ? (0xFF5100U) : (0xFFFFFFU));
    box_sag_2.setColor(((state & BTN_SMOKE)           || (override_active_btn & BTN_SMOKE))           ? (0xFF5100U) : (0xFFFFFFU));
    box_sag_3.setColor(((state & BTN_VOLUME_MENU)     || (override_active_btn & BTN_VOLUME_MENU))     ? (0xFF5100U) : (0xFFFFFFU));
    box_sag_4.setColor(((state & BTN_DOWN)            || (override_active_btn & BTN_DOWN))            ? (0xFF5100U) : (0xFFFFFFU));

    // Hepsini ekranda yenile
    box_sol_1.invalidate();
    box_sol_2.invalidate();
    box_sol_3.invalidate();
    box_sol_4.invalidate();
    box_sag_1.invalidate();
    box_sag_2.invalidate();
    box_sag_3.invalidate();
    box_sag_4.invalidate();
}

void ButtonContainer::setButtonIcon(HC165_Buttons button, uint16_t svgId, float scaleX, float scaleY)
{
    // Hedef SVG objesini tutacak bir pointer oluşturuyoruz
    touchgfx::SVGImage* targetSvg = 0;

    // Hangi donanım tuşunun ikonunu değiştireceğiz?
    switch (button)
    {
        case BTN_BLACKOUT:        targetSvg = &svgImage5; break; // Designer'daki kendi ismine göre düzelt
        case BTN_HOME:            targetSvg = &svgImage3; break;
        case BTN_BRIGHTNESS_MENU: targetSvg = &svgImage1; break;
        case BTN_DOWN:            targetSvg = &DownSvgImage; break;
        case BTN_MENU:            targetSvg = &svgImage6; break; // Senin örneğindeki Menü ikonu
        case BTN_SMOKE:           targetSvg = &svgImage4; break;
        case BTN_VOLUME_MENU:     targetSvg = &svgImage2; break;
        case BTN_UP:              targetSvg = &UpSvgImage; break;
        default: return; // Geçersiz bir buton geldiyse hiçbir şey yapma
    }

    // Eğer doğru bir obje bulduysak, ikonu ve boyutunu ayarla
    if (targetSvg != 0)
    {
        targetSvg->setSVG(svgId);
        targetSvg->setScale(scaleX, scaleY);
        targetSvg->invalidate(); // Ekranda yenile
    }
}
