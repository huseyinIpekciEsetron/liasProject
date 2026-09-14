#include <gui/containers/sisHavanContainer.hpp>
#include <touchgfx/Color.hpp>

sisHavanContainer::sisHavanContainer()
{

}

void sisHavanContainer::initialize()
{
    sisHavanContainerBase::initialize();
}

// Yardımcı fonksiyon: Kutuyu duruma göre boyar ve saydamlığını ayarlar
void applyTubeStyle(touchgfx::BoxWithBorder& box, Model::TubeState state)
{
	// Bütün durumlarda görünür olacak (Saydamlık İptal)
	box.setAlpha(255);

	// Bütün durumlarda Çerçeve (Border) Rengi: #CFCFCF
	box.setBorderColor(touchgfx::Color::getColorFromRGB(207, 207, 207));

	switch (state)
	{
		case Model::TUBE_SMOKE:
			box.setColor(touchgfx::Color::getColorFromRGB(0, 200, 0)); // Dolgu: Yeşil
			break;

		case Model::TUBE_FRAG:
			box.setColor(touchgfx::Color::getColorFromRGB(255, 81, 0)); // Dolgu: Turuncu
			break;

		case Model::TUBE_FAULT:
			box.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0)); // Dolgu: Kırmızı
			break;

		case Model::TUBE_EMPTY:
		default:
			// İçi Boş veya Ateşlendi durumu: #002E40
			box.setColor(touchgfx::Color::getColorFromRGB(0, 46, 64));
			break;
	}
	box.invalidate(); // Değişikliği ekrana yansıt
}

void sisHavanContainer::setStatus(Model::TubeState t1, Model::TubeState t2, Model::TubeState t3, Model::TubeState t4)
{
	// Kutulara renk/saydamlık stillerini uygula
	applyTubeStyle(boxWithBorder1, t1);
	applyTubeStyle(boxWithBorder2, t2);
	applyTubeStyle(boxWithBorder3, t3);
	applyTubeStyle(boxWithBorder4, t4);

	// Text Wildcard güncellemesi "(X/4)"
	// Sadece boş OLMAYAN tüpleri sayıyoruz (Arızalı da olsa fiziksel olarak oradadır)
	int count = (t1 != Model::TUBE_EMPTY) + (t2 != Model::TUBE_EMPTY) +
				(t3 != Model::TUBE_EMPTY) + (t4 != Model::TUBE_EMPTY);

	Unicode::snprintf(sisCountTextBuffer, SISCOUNTTEXT_SIZE, "(%d/4)", count);
	sisCountText.invalidate();

	// HAVAN İKONUNUN RENGİNİ DİNAMİK DEĞİŞTİRME
	// ==============================================================
	if (count == 0) {
		// TAMAMEN BOŞ: Mat/Taktiksel Kırmızı (#962828)
		circle1Painter.setColor(touchgfx::Color::getColorFromRGB(150, 40, 40));
	} else {
		// MÜHİMMAT VAR: Senin Orijinal Mavi Tonun (#417E94)
		circle1Painter.setColor(touchgfx::Color::getColorFromRGB(65, 126, 148));
	}

	// Görünürlüğü her halükarda %100 (Alpha 255) yapıyoruz
	circle1.setAlpha(255);
	circle1.invalidate();
}
