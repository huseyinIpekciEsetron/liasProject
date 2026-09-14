#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

class ModelListener
{
public:
    ModelListener() : model(0) {}

    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }
    virtual void onRadarTargetsReceived(const TLUS::ThreatMessagePayload& payload) {}

    virtual void onVolumeMenuPressed() {}
	virtual void onBrightnessMenuPressed() {}
	virtual void onEnterPressed() {}
	virtual void onBackPressed() {}
    virtual void onUpPressed() {}
    virtual void onDownPressed() {}
    virtual void onHomePressed() {}
    virtual void onSmokePressed() {}
    virtual void onMenuPressed() {}

    virtual void onHardwareButtonStateChanged(uint32_t state) {}
    virtual void onSmokeDataUpdated(Model::TubeState* states) {}
    virtual void onBlackoutStateChanged(bool isBlackout) {}

    virtual void onShowWarning(Model::WarningType warning, int tubeIndex) {}
    virtual void onTimeDateUpdated(const TLUS::TimeData& time) {}
    virtual void onSystemStatusUpdated(const TLUS::SystemStatusPayload& status) {}
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
