#ifndef SETTINGSITEMCONTAINER_HPP
#define SETTINGSITEMCONTAINER_HPP

#include <gui_generated/containers/SettingsItemContainerBase.hpp>

class SettingsItemContainer : public SettingsItemContainerBase
{
public:
    SettingsItemContainer();
    virtual ~SettingsItemContainer() {}

    virtual void initialize();

    void setupItemValue(touchgfx::TypedTextId titleId, touchgfx::TypedTextId valueId);
	void setupItemNumber(touchgfx::TypedTextId titleId, int value);

    // Satırın seçili olma ve "Düzenleniyor" olma durumuna göre rengini değiştirir
    void setHighlighted(bool isSelected, bool isEditing);
protected:
};

#endif // SETTINGSITEMCONTAINER_HPP
