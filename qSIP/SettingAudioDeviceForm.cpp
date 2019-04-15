#include "SettingAudioDeviceForm.h"
#include "ui_SettingAudioDeviceForm.h"

#include <QAudioDeviceInfo>

SettingAudioDeviceForm::SettingAudioDeviceForm(QWidget *parent)
	: AbstractSettingForm(parent)
	, ui(new Ui::SettingAudioDeviceForm)
{
	ui->setupUi(this);

	QString def = "default";
	ui->comboBox_input->addItem(def);
	ui->comboBox_output->addItem(def);

	QList<QAudioDeviceInfo> inputdevs = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
	for (auto const &info : inputdevs) {
		if (info.deviceName() == def) continue;
		ui->comboBox_input->addItem(info.deviceName());
	}
	QList<QAudioDeviceInfo> outputdevs = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
	for (auto const &info : outputdevs) {
		if (info.deviceName() == def) continue;
		ui->comboBox_output->addItem(info.deviceName());
	}
}

void SettingAudioDeviceForm::exchange(bool save)
{
	ApplicationSettings *as = settings();
	if (save) {
		as->audio_input = ui->comboBox_input->currentText();
		as->audio_output = ui->comboBox_output->currentText();
	} else {
		ui->comboBox_input->setCurrentText(as->audio_input);
		ui->comboBox_output->setCurrentText(as->audio_output);
	}
}
