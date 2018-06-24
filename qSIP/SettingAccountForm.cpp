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

void SettingAccountForm::exchangeAccount(bool save, SIP::Account *a)
{
	auto FixPortNumber = [&](int port){
		if (port < 1 || port > 65535) {
			port = 5060;
		}
		return port;
	};
	if (save) {
		a->phone_number = ui->lineEdit_phone_number->text();
		a->server = ui->lineEdit_sip_server->text();
		a->port = ui->lineEdit_port->text().toInt();
		a->service_domain = ui->lineEdit_service_domain->text();
		a->user = ui->lineEdit_user->text();
		a->password = ui->lineEdit_password->text();
	} else {
		ui->lineEdit_phone_number->setText(a->phone_number);
		ui->lineEdit_sip_server->setText(a->server);
		ui->lineEdit_port->setText(QString::number(FixPortNumber(a->port)));
		ui->lineEdit_service_domain->setText(a->service_domain);
		ui->lineEdit_user->setText(a->user);
		ui->lineEdit_password->setText(a->password);
	}
}

void SettingAccountForm::exchange(bool save)
{
	exchangeAccount(save, &settings()->account);
}

void SettingAccountForm::on_pushButton_reregister_clicked()
{
	SIP::Account a;
	exchangeAccount(true, &a);
	mainwindow()->reregister(a);
}
