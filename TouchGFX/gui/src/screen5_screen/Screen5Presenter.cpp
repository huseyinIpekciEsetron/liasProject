#include <gui/screen5_screen/Screen5View.hpp>
#include <gui/screen5_screen/Screen5Presenter.hpp>
#include <gui/common/FrontendApplication.hpp>

Screen5Presenter::Screen5Presenter(Screen5View& v)
    : view(v)
{
}

void Screen5Presenter::activate()
{
    // Modele şu an Test Ekranında olduğumuzu bildir
    model->setActiveScreen(Model::SCREEN_TESTS);
    // EKRAN HAZIR OLDUĞU AN MODEL'DEN GÜNCEL SAATİ KENDİN ÇEK
	TLUS::TimeData currentTime = model->getSystemTime();

	// Eğer Model'in elinde geçerli bir saat varsa hemen View'a bas
	if (currentTime.isValid) {
		view.updateTimeDate(currentTime);
	}
}

void Screen5Presenter::deactivate()
{
}

// ========================================================
// AKILLI TUŞ YÖNLENDİRİCİLERİ (POPUP KORUMALI)
// ========================================================

void Screen5Presenter::onUpPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.buttonUpPressed();
}

void Screen5Presenter::onDownPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.buttonDownPressed();
}

void Screen5Presenter::onMenuPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.menuButtonPressed();
}

// SES TUŞU -> ENTER OLARAK ÇALIŞIR
void Screen5Presenter::onVolumeMenuPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.enterPressed();
}

// PARLAKLIK TUŞU -> BACK (İPTAL/GERİ) OLARAK ÇALIŞIR
void Screen5Presenter::onBrightnessMenuPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.backPressed();
}
void Screen5Presenter::gotoZeroizeScreen()
{
    model->setPendingAction(Model::ACTION_ZEROIZE);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}
void Screen5Presenter::gotoSoftResetScreen()
{
    model->setPendingAction(Model::ACTION_SOFT_RESET);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}
void Screen5Presenter::onHomePressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }

    // Eğer Test Ekranının Yan Menüsü açıksa önce onu kapat,
    // değilse doğrudan Ana Ekrana (Radar) dön.
    if (view.isMenuOpen()) {
        view.closeMenus();
        return;
    }
    onHomePressedReal();
}

void Screen5Presenter::onSmokePressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }

    // Sis Havanı (Screen 2) ekranına geç
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen2ScreenNoTransition();
}

// ========================================================
// DONANIM VE UYARI YÖNLENDİRİCİLERİ
// ========================================================
void Screen5Presenter::setLedTestMode(int mode) {
    model->setLedTestMode(mode);
}
int Screen5Presenter::getBrightness() { return model->getBrightness(); }
void Screen5Presenter::saveBrightness(int b) { model->saveBrightness(b); }
int Screen5Presenter::getVolume() { return model->getVolume(); }
void Screen5Presenter::saveVolume(int v) { model->saveVolume(v); }
int Screen5Presenter::getLedBrightness() { return model->getLedBrightness(); }
void Screen5Presenter::saveLedBrightness(int l) { model->saveLedBrightness(l); }
void Screen5Presenter::setBuzzerLevel(AlarmLevel_t level) { model->setBuzzerLevel(level); }
void Screen5Presenter::setTestModeActive(bool active) { model->setTestModeActive(active); }
bool Screen5Presenter::getCommLostFlag() { return model->getCommLostFlag(); }
Model::BmbTelemetryData Screen5Presenter::getBmbTelemetry() { return model->getBmbTelemetry(); }
Model::CommTelemetryData Screen5Presenter::getCommTelemetry() {return model->getCommTelemetry(); }

void Screen5Presenter::onHardwareButtonStateChanged(uint32_t state) {
    view.updateButtonVisuals(state);
}

void Screen5Presenter::onShowWarning(Model::WarningType warning, int tubeIndex) {
    view.showWarningPopup(warning, tubeIndex);
}

void Screen5Presenter::onHomePressedReal() {
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen1ScreenNoTransition();
}

void Screen5Presenter::gotoAyarlarScreen() {
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen3ScreenNoTransition();
}

void Screen5Presenter::gotoArizaScreen() {
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen4ScreenNoTransition();
}

void Screen5Presenter::gotoTestScreen() {
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen5ScreenNoTransition();
}

void Screen5Presenter::gotoLoglarScreen() {
     static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen6ScreenNoTransition();
}

void Screen5Presenter::onTimeDateUpdated(const TLUS::TimeData& time)
{
    view.updateTimeDate(time);
}
