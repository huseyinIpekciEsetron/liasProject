#include <gui/screen4_screen/Screen4View.hpp>
#include <gui/screen4_screen/Screen4Presenter.hpp>
#include <gui/common/FrontendApplication.hpp>

Screen4Presenter::Screen4Presenter(Screen4View& v) : view(v) {}

void Screen4Presenter::activate() {
    model->setActiveScreen(Model::SCREEN_FAULTS);

    TLUS::TimeData currentTime = model->getSystemTime();
    if (currentTime.isValid) {
        view.updateTimeDate(currentTime);
    }
}

void Screen4Presenter::deactivate() {}

void Screen4Presenter::onUpPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.buttonUpPressed();
}

void Screen4Presenter::onDownPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.buttonDownPressed();
}

void Screen4Presenter::onMenuPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.menuButtonPressed();
}

void Screen4Presenter::onVolumeMenuPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.handleVolumeOrEnter();
}

void Screen4Presenter::onBrightnessMenuPressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    view.handleBrightnessOrBack();
}

void Screen4Presenter::gotoZeroizeScreen() {
    model->setPendingAction(Model::ACTION_ZEROIZE);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}

void Screen4Presenter::gotoSoftResetScreen() {
    model->setPendingAction(Model::ACTION_SOFT_RESET);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}

int Screen4Presenter::getBrightness() { return model->getBrightness(); }
void Screen4Presenter::saveBrightness(int b) { model->saveBrightness(b); }
int Screen4Presenter::getVolume() { return model->getVolume(); }
void Screen4Presenter::saveVolume(int v) { model->saveVolume(v); }

void Screen4Presenter::onTimeDateUpdated(const TLUS::TimeData& time) {
    view.updateTimeDate(time);
}

void Screen4Presenter::goBackToSettings() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    //static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen3ScreenNoTransition();
}

void Screen4Presenter::onHardwareButtonStateChanged(uint32_t state) { view.updateButtonVisuals(state); }

void Screen4Presenter::onShowWarning(Model::WarningType warning, int tubeIndex) {
    view.showWarningPopup(warning, tubeIndex);
}

void Screen4Presenter::onHomePressed() {
    if (view.isMenuOpen()) { view.closeMenus(); return; }
    onHomePressedReal();
}

void Screen4Presenter::onSmokePressed() {
    if (view.isPopupVisible()) { view.hidePopup(); return; }
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen2ScreenNoTransition();
}

void Screen4Presenter::onHomePressedReal() {
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen1ScreenNoTransition();
}

void Screen4Presenter::gotoAyarlarScreen() {
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen3ScreenNoTransition();
}

void Screen4Presenter::gotoArizaScreen() {
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen4ScreenNoTransition();
}

void Screen4Presenter::gotoTestScreen() {
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen5ScreenNoTransition();
}

void Screen4Presenter::gotoLoglarScreen() {
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen6ScreenNoTransition();
}

// =========================================================
// MODEL'DEN VİEW'A ARIZA VE SAAT BİLGİSİ TAŞIYAN KÖPRÜLER
// =========================================================

// Yeni Arıza Geldiğinde Ekranı Tetikleyen Olay (Event)
void Screen4Presenter::onSystemStatusUpdated(const TLUS::SystemStatusPayload& status)
{
    view.refreshFaultList();
}

// Mühimmat Hataları (Sadece Aktif olanlar okunur)
bool Screen4Presenter::getFaultStatus(int index) { return model->getFaultStatus(index); }
bool Screen4Presenter::getFaultIsFrag(int index) { return model->getFaultIsFrag(index); }

// Donanım ve Sistem Hataları (Geçmiş/Hafıza okunur)
bool Screen4Presenter::getStoredCommLostFlag() { return model->getStoredCommLostFlag(); }
bool Screen4Presenter::getStoredHwErrorFlag()  { return model->getStoredHwErrorFlag(); }
TLUS::SystemStatusPayload Screen4Presenter::getStoredSystemStatus() { return model->getStoredSystemStatus(); }

// Saat Bilgileri
uint16_t Screen4Presenter::getCommFaultTime() { return model->getCommFaultTime(); }
uint16_t Screen4Presenter::getHwFaultTime()   { return model->getHwFaultTime(); }
uint16_t Screen4Presenter::getProcFaultTime(int index) { return model->getProcFaultTime(index); }
uint16_t Screen4Presenter::getSensFaultTime(int s, int index) { return model->getSensFaultTime(s, index); }
uint16_t Screen4Presenter::getTubeFaultTime(int index) { return model->getTubeFaultTime(index); }

// Silme İşlemi
void Screen4Presenter::clearAllFaults() { model->clearAllFaults(); }
