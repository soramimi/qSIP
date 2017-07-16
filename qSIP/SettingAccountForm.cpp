#include "SettingAccountForm.h"
#include "ui_SettingAccountForm.h"

SettingAccountForm::SettingAccountForm(QWidget *parent) :
	AbstractSettingForm(parent),
	ui(new Ui::SettingAccountForm)
{
	ui->setupUi(this);
}

SettingAccountForm::~SettingAccountForm()
{
	delete ui;
}

void SettingAccountForm::exchange(bool save)
{
	int port;
	auto FixPortNumber = [&](){
		if (port < 1 || port > 65535) {
			port = 5060;
		}
	};
	if (save) {
		settings()->account.server = ui->lineEdit_sip_server->text();
		port = ui->lineEdit_port->text().toInt();
		settings()->account.user = ui->lineEdit_user->text();
		settings()->account.password = ui->lineEdit_password->text();
		FixPortNumber();
		settings()->account.port = port;
	} else {
		port = settings()->account.port;
		FixPortNumber();
		ui->lineEdit_sip_server->setText(settings()->account.server);
		ui->lineEdit_port->setText(QString::number(port));
		ui->lineEdit_user->setText(settings()->account.user);
		ui->lineEdit_password->setText(settings()->account.password);
	}
}

