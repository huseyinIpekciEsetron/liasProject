#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <gui/common/FrontendApplication.hpp>

Screen3Presenter::Screen3Presenter(Screen3View& v) : view(v) {}

void Screen3Presenter::activate() {
    model->setActiveScreen(Model::SCREEN_SETTING);

    // EKRAN HAZIR OLDUĞU AN MODEL'DEN GÜNCEL SAATİ KENDİN ÇEK
	TLUS::TimeData currentTime = model->getSystemTime();

	// Eğer Model'in elinde geçerli bir saat varsa hemen View'a bas
	if (currentTime.isValid) {
		view.updateTimeDate(currentTime);
	}
}

void Screen3Presenter::deactivate() {}


void Screen3Presenter::onUpPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.buttonUpPressed();
}
void Screen3Presenter::onDownPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.buttonDownPressed();
}
void Screen3Presenter::onHomePressed()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen1ScreenNoTransition();
}
void Screen3Presenter::onSmokePressed()
{
	if (view.isPopupVisible()) { view.hidePopup(); return; }
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen2ScreenNoTransition();
}
void Screen3Presenter::onMenuPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.menuButtonPressed();
}

void Screen3Presenter::onVolumeMenuPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.enterPressed();
}

void Screen3Presenter::onBrightnessMenuPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.backPressed();
}

void Screen3Presenter::onHardwareButtonStateChanged(uint32_t state) {
    view.updateButtonVisuals(state);
}
void Screen3Presenter::onShowWarning(Model::WarningType warning, int tubeIndex) {
    view.showWarningPopup(warning, tubeIndex);
}

int Screen3Presenter::getBrightness() { return model->getBrightness(); }
void Screen3Presenter::saveBrightness(int b) { model->saveBrightness(b); }
int Screen3Presenter::getVolume() { return model->getVolume(); }
void Screen3Presenter::saveVolume(int v) { model->saveVolume(v); }
int Screen3Presenter::getLedBrightness() { return model->getLedBrightness(); }
void Screen3Presenter::saveLedBrightness(int l) { model->saveLedBrightness(l); }
void Screen3Presenter::gotoZeroizeScreen()
{
    model->setPendingAction(Model::ACTION_ZEROIZE);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}
void Screen3Presenter::gotoSoftResetScreen()
{
    model->setPendingAction(Model::ACTION_SOFT_RESET);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}
Model::SalvoMode Screen3Presenter::getSalvoMode() { return model->getSalvoMode(); }
void Screen3Presenter::setSalvoMode(Model::SalvoMode mode) { model->setSalvoMode(mode); }

bool Screen3Presenter::getWarningsEnabled() { return model->getWarningsEnabled(); }
void Screen3Presenter::setWarningsEnabled(bool state) { model->setWarningsEnabled(state); }
void Screen3Presenter::onTimeDateUpdated(const TLUS::TimeData& time)
{
    view.updateTimeDate(time);
}
void Screen3Presenter::gotoAyarlarScreen()
{
     static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen3ScreenNoTransition();
}

void Screen3Presenter::gotoArizaScreen()
{
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen4ScreenNoTransition();
}

void Screen3Presenter::gotoTestScreen()
{
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen5ScreenNoTransition();
}

void Screen3Presenter::gotoLoglarScreen()
{
     static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen6ScreenNoTransition();
}
