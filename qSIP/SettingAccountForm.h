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

	SIP::Account account();
protected:
	void exchange(bool save);
private slots:
	void on_pushButton_reregister_clicked();
};

#endif // SETTINGACCOUNTFORM_H
