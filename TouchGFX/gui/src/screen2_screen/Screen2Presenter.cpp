#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/common/FrontendApplication.hpp>

Screen2Presenter::Screen2Presenter(Screen2View& v)
    : view(v)
{

}

void Screen2Presenter::activate()
{
	// Modele "Şu an Sis Havanı Ekranındayız" bilgisini gönder
	model->setActiveScreen(Model::SCREEN_SMOKE);

	// EKRAN HAZIR OLDUĞU AN MODEL'DEN GÜNCEL SAATİ KENDİN ÇEK
	TLUS::TimeData currentTime = model->getSystemTime();

	// Eğer Model'in elinde geçerli bir saat varsa hemen View'a bas
	if (currentTime.isValid) {
		view.updateTimeDate(currentTime);
	}
}

void Screen2Presenter::deactivate()
{

}

void Screen2Presenter::onHardwareButtonStateChanged(uint32_t state)
{
    view.updateButtonVisuals(state);
}
void Screen2Presenter::onVolumeMenuPressed()
{
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.handleVolumeOrEnter(); // VIEW'DAKİ AKILLI YÖNLENDİRİCİ
}

void Screen2Presenter::onBrightnessMenuPressed()
{
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.handleBrightnessOrBack(); // VIEW'DAKİ AKILLI YÖNLENDİRİCİ
}

void Screen2Presenter::onMenuPressed()
{
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.menuButtonPressed();
}

void Screen2Presenter::onUpPressed()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
	view.buttonUpPressed();
}
void Screen2Presenter::onDownPressed()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
	view.buttonDownPressed();
}
void Screen2Presenter::saveBrightness(int brightness)
{
    model->saveBrightness(brightness);
}
void Screen2Presenter::saveVolume(int volume)
{
    model->saveVolume(volume);
}
void Screen2Presenter::gotoZeroizeScreen()
{
    model->setPendingAction(Model::ACTION_ZEROIZE);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}

void Screen2Presenter::gotoSoftResetScreen()
{
    model->setPendingAction(Model::ACTION_SOFT_RESET);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}
void Screen2Presenter::onBlackoutStateChanged(bool isBlackout)
{
    // Eğer karartma moduna geçildiyse, açık olan menüleri zorla kapat
    /*if(isBlackout)
    {
        view.closeMenus();
    }*/
}

void Screen2Presenter::onHomePressed()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
	view.homeButtonPressed();
}

void Screen2Presenter::onShowWarning(Model::WarningType warning, int tubeIndex)
{
    // Modelden gelen uyarıyı View'a pasla[cite: 1]
    view.showWarningPopup(warning, tubeIndex);
}

// Model'den gelen veriyi View'a ilet
void Screen2Presenter::onSmokeDataUpdated(Model::TubeState* states)
{
    view.updateSmokeStatus(states);
}

void Screen2Presenter::onHomePressedReal()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
    // Screen2'den Ana Ekrana (Radar) animasyonsuz geri dönüş (Asıl ekran geçişi)
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen1ScreenNoTransition();
}
void Screen2Presenter::onTimeDateUpdated(const TLUS::TimeData& time)
{
    view.updateTimeDate(time);
}

void Screen2Presenter::gotoAyarlarScreen()
{
     static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen3ScreenNoTransition();
}

void Screen2Presenter::gotoArizaScreen()
{
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen4ScreenNoTransition();
}

void Screen2Presenter::gotoTestScreen()
{
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen5ScreenNoTransition();
}

void Screen2Presenter::gotoLoglarScreen()
{
     static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen6ScreenNoTransition();
}
int Screen2Presenter::getBrightness()
{
    return model->getBrightness();
}

int Screen2Presenter::getVolume()
{
    return model->getVolume();
}
