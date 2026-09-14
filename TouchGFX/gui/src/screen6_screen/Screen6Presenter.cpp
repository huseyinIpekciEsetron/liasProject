#include <gui/screen6_screen/Screen6View.hpp>
#include <gui/screen6_screen/Screen6Presenter.hpp>

Screen6Presenter::Screen6Presenter(Screen6View& v)
    : view(v)
{

}

void Screen6Presenter::activate()
{
	// Modele şu an Test Ekranında olduğumuzu bildir
	model->setActiveScreen(Model::SCREEN_LOGS);
	// EKRAN HAZIR OLDUĞU AN MODEL'DEN GÜNCEL SAATİ KENDİN ÇEK
	TLUS::TimeData currentTime = model->getSystemTime();

	// Eğer Model'in elinde geçerli bir saat varsa hemen View'a bas
	if (currentTime.isValid) {
		view.updateTimeDate(currentTime);
	}
}

void Screen6Presenter::deactivate()
{

}
void Screen6Presenter::onTimeDateUpdated(const TLUS::TimeData& time)
{
    view.updateTimeDate(time);
}
void Screen6Presenter::onUpPressed() { if (view.isPopupVisible()) { view.hidePopup(); return; } view.buttonUpPressed(); }
void Screen6Presenter::onDownPressed() { if (view.isPopupVisible()) { view.hidePopup(); return; } view.buttonDownPressed(); }
void Screen6Presenter::onMenuPressed() { if (view.isPopupVisible()) { view.hidePopup(); return; } view.menuButtonPressed(); }
void Screen6Presenter::onVolumeMenuPressed() { if (view.isPopupVisible()) { view.hidePopup(); return; } view.handleVolumeOrEnter(); }
void Screen6Presenter::onBrightnessMenuPressed() { if (view.isPopupVisible()) { view.hidePopup(); return; } view.handleBrightnessOrBack(); }
void Screen6Presenter::onHardwareButtonStateChanged(uint32_t state) { view.updateButtonVisuals(state); }
void Screen6Presenter::onSmokePressed() { if (view.isPopupVisible()) { view.hidePopup(); return; } static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen2ScreenNoTransition(); }
void Screen6Presenter::onHomePressed() { if (view.isMenuOpen()) { view.closeMenus(); return; } onHomePressedReal(); }
void Screen6Presenter::onShowWarning(Model::WarningType warning, int tubeIndex) { view.showWarningPopup(warning, tubeIndex); }

int Screen6Presenter::getLogCount() { return model->getLogCount(); }
int Screen6Presenter::getLogHead() { return model->getLogHead(); }
Model::LogEntry Screen6Presenter::getLog(int index) { return model->getLog(index); }

int Screen6Presenter::getBrightness() { return model->getBrightness(); }
void Screen6Presenter::saveBrightness(int b) { model->saveBrightness(b); }
int Screen6Presenter::getVolume() { return model->getVolume(); }
void Screen6Presenter::saveVolume(int v) { model->saveVolume(v); }

void Screen6Presenter::gotoZeroizeScreen() { model->setPendingAction(Model::ACTION_ZEROIZE); static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition(); }
void Screen6Presenter::gotoSoftResetScreen() { model->setPendingAction(Model::ACTION_SOFT_RESET); static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition(); }
void Screen6Presenter::onHomePressedReal() { static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen1ScreenNoTransition(); }
void Screen6Presenter::gotoAyarlarScreen() { static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen3ScreenNoTransition(); }
void Screen6Presenter::gotoArizaScreen() { static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen4ScreenNoTransition(); }
void Screen6Presenter::gotoTestScreen() { static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen5ScreenNoTransition(); }
void Screen6Presenter::gotoLoglarScreen() {static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen6ScreenNoTransition(); }
