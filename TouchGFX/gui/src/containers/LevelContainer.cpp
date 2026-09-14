#include <gui/containers/LevelContainer.hpp>
#include <touchgfx/Color.hpp>
#include <images/SVGDatabase.hpp>

LevelContainer::LevelContainer()
{

}

void LevelContainer::initialize()
{
    LevelContainerBase::initialize();
}

// =========================================================
//  Kutuları Turuncu ve Beyaz Yapar
// =========================================================
void LevelContainer::setLevel(int level, uint16_t svgImageId)
{
	if (svgImageId == SVG_SOUND_ID) {
		svgBright.setVisible(false);
		svgVolume.setVisible(true);
	}
	else if (svgImageId == SVG_BRIGHT_ID)
	{
		svgBright.setVisible(true);
		svgVolume.setVisible(false);
	}

    // Kutuları bir diziye alıyoruz (Tek tek yazmaktan kurtuluruz)
    touchgfx::Box* blocks[5] = { &block1, &block2, &block3, &block4, &block5 };

    for (int i = 0; i < 5; i++)
    {
        if (level >= (i + 1)) {
            // Dolu kutular: TURUNCU (#FF5100 -> R:255, G:81, B:0)
            blocks[i]->setColor(touchgfx::Color::getColorFromRGB(255, 81, 0));
        } else {
            // Boş kutular: BEYAZ (#FFFFFF)
            blocks[i]->setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
        }

        // Kutunun Alpha'sını tam görünür yapıyoruz (önceden sönük yapıyorduk)
        blocks[i]->setAlpha(255);

        // Ekranı güncelle
        blocks[i]->invalidate();
    }
    svgBright.invalidate();
	svgVolume.invalidate();
}
