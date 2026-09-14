#include <gui/containers/TestItemContainer.hpp>
#include <touchgfx/Color.hpp>

TestItemContainer::TestItemContainer() {}

void TestItemContainer::initialize() {
    TestItemContainerBase::initialize();
}

void TestItemContainer::setupTest(touchgfx::TypedTextId textId) {
    touchgfx::Unicode::strncpy(textArea1Buffer, touchgfx::TypedText(textId).getText(), TEXTAREA1_SIZE);
    textArea1.invalidate();
}

void TestItemContainer::setHighlighted(bool isSelected) {
    if (isSelected) {
        // Seçiliyken: Arka Plan Turuncu (#FF5100), Yazı Beyaz (#FFFFFF)
        boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(255, 81, 0));
        textArea1.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    } else {
        // Normal durum: Arka Plan Açık Gri (#CFCFCF), Yazı Siyah (#000000)
        boxWithBorder.setColor(touchgfx::Color::getColorFromRGB(207, 207, 207));
        textArea1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    }

    // Değişiklikleri ekrana yansıt (Yazıyı da yenilemeyi unutmuyoruz!)
    boxWithBorder.invalidate();
    textArea1.invalidate();
}
