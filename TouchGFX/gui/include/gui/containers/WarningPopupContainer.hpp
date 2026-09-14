#ifndef WARNINGPOPUPCONTAINER_HPP
#define WARNINGPOPUPCONTAINER_HPP

#include <gui_generated/containers/WarningPopupContainerBase.hpp>
#include <gui/model/Model.hpp>

class WarningPopupContainer : public WarningPopupContainerBase
{
public:
    WarningPopupContainer();
    virtual ~WarningPopupContainer() {}

    virtual void initialize();
    // Her 60 FPS'de çalışacak ve sayacı düşürecek
	virtual void handleTickEvent();

	// Presenter'dan çağırılacak ana fonksiyon
	void showWarningMessage(Model::WarningType warning, int tubeIndex = -1);
	void hideWarning();

protected:
	int timeoutCounter; // Popup'ın ekranda kalma süresi

	// UYARI KUYRUĞU (QUEUE) YAPISI
	struct QueuedWarning {
		Model::WarningType type;
		int tubeIndex;
	};

	static const int MAX_QUEUE_SIZE = 6;
	QueuedWarning warningQueue[MAX_QUEUE_SIZE];
	uint8_t queueCount;

	// Kuyruktaki ilk elemanı alıp ekrana basan yardımcı fonksiyon
	void showNextWarning();
};

#endif // WARNINGPOPUPCONTAINER_HPP
