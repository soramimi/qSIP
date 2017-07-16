#ifndef SETTINGACCOUNTFORM_H
#define SETTINGACCOUNTFORM_H

#include "AbstractSettingForm.h"

#include <QWidget>

namespace Ui {
class SettingAccountForm;
}

class SettingAccountForm : public AbstractSettingForm
{
	Q_OBJECT

public:
	explicit SettingAccountForm(QWidget *parent = 0);
	~SettingAccountForm();

private:
	Ui::SettingAccountForm *ui;

protected:
	void exchange(bool save);
};

#endif // SETTINGACCOUNTFORM_H
