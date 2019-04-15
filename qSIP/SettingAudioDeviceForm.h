#ifndef SETTINGAUDIODEVICEFORM_H
#define SETTINGAUDIODEVICEFORM_H

#include "AbstractSettingForm.h"

namespace Ui {
class SettingAudioDeviceForm;
}

class SettingAudioDeviceForm : public AbstractSettingForm {
	Q_OBJECT
private:
	Ui::SettingAudioDeviceForm *ui;
protected:
	void exchange(bool save);
public:
	explicit SettingAudioDeviceForm(QWidget *parent = nullptr);
};

#endif // SETTINGAUDIODEVICEFORM_H
