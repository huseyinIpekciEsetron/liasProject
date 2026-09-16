#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <gui/common/FrontendApplication.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{

}

void Screen1Presenter::activate()
{
	// Modele "Şu an Radar Ekranındayız" bilgisini gönder
	model->setActiveScreen(Model::SCREEN_RADAR);

	// EKRAN HAZIR OLDUĞU AN MODEL'DEN GÜNCEL SAATİ KENDİN ÇEK
	TLUS::TimeData currentTime = model->getSystemTime();

	// Eğer Model'in elinde geçerli bir saat varsa hemen View'a bas
	if (currentTime.isValid) {
		view.updateTimeDate(currentTime);
	}
}

void Screen1Presenter::deactivate()
{

}

int Screen1Presenter::getFadeTicksRemaining(int index)
{
    return model->getFadeTicksRemaining(index);
}

void Screen1Presenter::setFadeTicksRemaining(int index, int ticks)
{
    model->setFadeTicksRemaining(index, ticks);
}

void Screen1Presenter::onRadarTargetsReceived(const TLUS::ThreatMessagePayload& payload)
{
    view.updateTargets(payload);
}
void Screen1Presenter::onHardwareButtonStateChanged(uint32_t state)
{
    view.updateButtonVisuals(state);
}

void Screen1Presenter::onVolumeMenuPressed()
{
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.handleVolumeOrEnter(); // VIEW'DAKİ AKILLI YÖNLENDİRİCİ
}

void Screen1Presenter::onBrightnessMenuPressed()
{
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.handleBrightnessOrBack(); // VIEW'DAKİ AKILLI YÖNLENDİRİCİ
}

void Screen1Presenter::onMenuPressed()
{
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.menuButtonPressed();
}

void Screen1Presenter::onUpPressed()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
	view.buttonUpPressed();
}
void Screen1Presenter::onDownPressed()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
	view.buttonDownPressed();
}
void Screen1Presenter::saveBrightness(int brightness)
{
    model->saveBrightness(brightness);
}
void Screen1Presenter::saveVolume(int volume)
{
    model->saveVolume(volume);
}
bool Screen1Presenter::getTlusCommLostFlag() { return model->getTlusCommLostFlag(); }
bool Screen1Presenter::getTlusSwDeadFlag()   { return model->getTlusSwDeadFlag(); }
bool Screen1Presenter::getTlusFrozenFlag()   { return model->getTlusFrozenFlag(); }

void Screen1Presenter::onBlackoutStateChanged(bool isBlackout)
{
    // Eğer karartma moduna geçildiyse, açık olan menüleri zorla kapat
   /* if(isBlackout)
    {
        view.closeMenus();
    }*/
}
void Screen1Presenter::onHomePressed()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
    // Artık direkt "closeMenus()" demiyoruz, View'a Home tuşuna basıldığını söylüyoruz
    view.homeButtonPressed();
}

void Screen1Presenter::onShowWarning(Model::WarningType warning, int tubeIndex)
{
    // Modelden gelen uyarıyı View'a pasla[cite: 1]
    view.showWarningPopup(warning, tubeIndex);
}

void Screen1Presenter::onSmokePressed()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
    // Screen1'den Screen2'ye (Sis Havanına) animasyonsuz geçiş
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen2ScreenNoTransition();
}
void Screen1Presenter::onTimeDateUpdated(const TLUS::TimeData& time)
{
    view.updateTimeDate(time);
}
void Screen1Presenter::gotoAyarlarScreen()
{
     static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen3ScreenNoTransition();
}

void Screen1Presenter::gotoArizaScreen()
{
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen4ScreenNoTransition();
}

void Screen1Presenter::gotoTestScreen()
{
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen5ScreenNoTransition();
}

void Screen1Presenter::gotoLoglarScreen()
{
     static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen6ScreenNoTransition();
}
void Screen1Presenter::gotoZeroizeScreen()
{
    model->setPendingAction(Model::ACTION_ZEROIZE);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}

void Screen1Presenter::gotoSoftResetScreen()
{
    model->setPendingAction(Model::ACTION_SOFT_RESET);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}

int Screen1Presenter::getBrightness()
{
    return model->getBrightness();
}

int Screen1Presenter::getVolume()
{
    return model->getVolume();
}

bool Screen1Presenter::getFaultStatus(int index)
{
	return model->getFaultStatus(index);
}
bool Screen1Presenter::getFaultIsFrag(int index)
{
	return model->getFaultIsFrag(index);
}
bool Screen1Presenter::getCommLostFlag()
{
	return model->getCommLostFlag();
}
bool Screen1Presenter::getHwErrorFlag()
{
	return model->getHwErrorFlag();
}

TLUS::SystemStatusPayload Screen1Presenter::getActiveSystemStatus()
{
    return model->getActiveSystemStatus();
}
