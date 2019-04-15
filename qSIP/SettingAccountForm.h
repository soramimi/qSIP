#ifndef SETTINGACCOUNTFORM_H
#define SETTINGACCOUNTFORM_H

#include "AbstractSettingForm.h"

#include <QWidget>

namespace Ui {
class SettingAccountForm;
}

class SettingAccountForm : public AbstractSettingForm {
	Q_OBJECT
private:
	Ui::SettingAccountForm *ui;

	void exchangeAccount(bool save, SIP::Account *a);
protected:
	void exchange(bool save);
public:
	explicit SettingAccountForm(QWidget *parent = 0);
	~SettingAccountForm();
private slots:
	void on_pushButton_reregister_clicked();
};

#endif // SETTINGACCOUNTFORM_H
