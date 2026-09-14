#include <gui/screen7_screen/Screen7View.hpp>
#include <gui/screen7_screen/Screen7Presenter.hpp>
#include <gui/common/FrontendApplication.hpp>

Screen7Presenter::Screen7Presenter(Screen7View& v)
    : view(v)
{
}

void Screen7Presenter::activate()
{
    model->setActiveScreen(Model::SCREEN_ZEROIZE);
    TLUS::TimeData currentTime = model->getSystemTime();
    if (currentTime.isValid) {
        view.updateTimeDate(currentTime);
    }
}

void Screen7Presenter::deactivate()
{
}

void Screen7Presenter::onHardwareButtonStateChanged(uint32_t state)
{
    view.updateButtonState(state);
}

// --- DİĞER BUTONLARA BASILDIĞINDA ANA EKRANLARA DÖN ---
void Screen7Presenter::onMenuPressed()
{
    if (view.isWarningPopupVisible()) { view.closeWarningSafe(); return; } // GÜNCELLENDİ
    if (view.isActionPopupVisible()) return;

    model->setPendingAction(Model::ACTION_NONE);
    view.menuButtonPressed();
}

void Screen7Presenter::onHomePressed()
{
    if (view.isWarningPopupVisible()) { view.closeWarningSafe(); return; } // GÜNCELLENDİ
    if (view.isActionPopupVisible()) return;

    model->setPendingAction(Model::ACTION_NONE);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen1ScreenNoTransition();
}

void Screen7Presenter::onSmokePressed()
{
    if (view.isWarningPopupVisible()) { view.closeWarningSafe(); return; } // GÜNCELLENDİ
    if (view.isActionPopupVisible()) return;

    model->setPendingAction(Model::ACTION_NONE);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen2ScreenNoTransition();
}

void Screen7Presenter::onUpPressed()
{
    if (view.isWarningPopupVisible()) { view.closeWarningSafe(); return; } // GÜNCELLENDİ
    if (view.isActionPopupVisible()) return;

    view.buttonUpPressed();
}

void Screen7Presenter::onDownPressed()
{
    if (view.isWarningPopupVisible()) { view.closeWarningSafe(); return; } // GÜNCELLENDİ
    if (view.isActionPopupVisible()) return;

    view.buttonDownPressed();
}

void Screen7Presenter::onVolumeMenuPressed() // ENTER TUŞU
{
    if (view.isWarningPopupVisible()) { view.closeWarningSafe(); return; } // GÜNCELLENDİ

    if (view.isMenuActive()) {
        view.handleVolumeOrEnter();
    }
}

void Screen7Presenter::onBrightnessMenuPressed() // BACK (İPTAL) TUŞU
{
    if (view.isWarningPopupVisible()) { view.closeWarningSafe(); return; } // GÜNCELLENDİ

    if (view.isMenuActive()) {
        view.handleBrightnessOrBack();
    }
}
void Screen7Presenter::saveBrightness(int brightness) { model->saveBrightness(brightness); }
void Screen7Presenter::saveVolume(int volume) { model->saveVolume(volume); }
int Screen7Presenter::getBrightness() { return model->getBrightness(); }
int Screen7Presenter::getVolume() { return model->getVolume(); }

void Screen7Presenter::gotoAyarlarScreen() { static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen3ScreenNoTransition(); }
void Screen7Presenter::gotoArizaScreen() { static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen4ScreenNoTransition(); }
void Screen7Presenter::gotoTestScreen() { static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen5ScreenNoTransition(); }
void Screen7Presenter::gotoLoglarScreen() { static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen6ScreenNoTransition(); }
void Screen7Presenter::gotoZeroizeScreen() {
	model->setPendingAction(Model::ACTION_ZEROIZE);
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}
void Screen7Presenter::gotoSoftResetScreen() {
	model->setPendingAction(Model::ACTION_SOFT_RESET);
	static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen7ScreenNoTransition();
}
// --- UYARI (WARNING) YAKALAYICI ---
void Screen7Presenter::onShowWarning(Model::WarningType warning, int tubeIndex)
{
    view.showWarningPopup(warning, tubeIndex);
}
// --------------------------------------------------------

Model::PendingAction Screen7Presenter::getPendingAction()
{
    return model->getPendingAction();
}

void Screen7Presenter::executePendingAction()
{
    Model::PendingAction action = model->getPendingAction();

    if (action == Model::ACTION_ZEROIZE) {
        model->uiSendAcilSilme(TLUS::ERASE_ALL);
    } else if (action == Model::ACTION_SOFT_RESET) {
        model->uiSendSoftReset();
    }

    model->setPendingAction(Model::ACTION_NONE);
}

void Screen7Presenter::cancelAndGoBack()
{
    model->setPendingAction(Model::ACTION_NONE);
    static_cast<FrontendApplication*>(Application::getInstance())->gotoScreen1ScreenNoTransition();
}

void Screen7Presenter::onTimeDateUpdated(const TLUS::TimeData& time)
{
    view.updateTimeDate(time);
}
