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
	auto FixPortNumber = [&](int port){
		if (port < 1 || port > 65535) {
			port = 5060;
		}
		return port;
	};
	if (save) {
		settings()->account.server = ui->lineEdit_sip_server->text();
		settings()->account.port = FixPortNumber(ui->lineEdit_port->text().toInt());
		settings()->account.user = ui->lineEdit_user->text();
		settings()->account.password = ui->lineEdit_password->text();
	} else {
		ui->lineEdit_sip_server->setText(settings()->account.server);
		ui->lineEdit_port->setText(QString::number(FixPortNumber(settings()->account.port)));
		ui->lineEdit_user->setText(settings()->account.user);
		ui->lineEdit_password->setText(settings()->account.password);
	}
}

SIP::Account SettingAccountForm::account()
{
	SIP::Account a;
	a.server = ui->lineEdit_sip_server->text();
	a.port = ui->lineEdit_port->text().toInt();
	a.user = ui->lineEdit_user->text();
	a.password = ui->lineEdit_password->text();
	return a;
}

void SettingAccountForm::on_pushButton_reregister_clicked()
{
	mainwindow()->reregister(account());
}
