#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "PhoneThread.h"
#include "SettingsDialog.h"

#include <QDebug>
#include "memory"

enum {
	PAD_ASTER = '*',
	PAD_SHARP = '#',
};

struct MainWindow::Private {
	ApplicationSettings appsettings;
	struct ua *ua = nullptr;
	std::shared_ptr<PhoneThread> re;
};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m(new Private)
{
	ui->setupUi(this);

	SettingsDialog::loadSettings(&m->appsettings);

	PhoneThread::init();

	reregister();
}

MainWindow::~MainWindow()
{
	unregister();
	delete m;
	delete ui;
}

void MainWindow::unregister()
{
	if (m->re) {
		re_cancel();
		m->re->wait();
	}
	m->re.reset();
}

void MainWindow::reregister()
{
	unregister();

	m->re = std::shared_ptr<PhoneThread>(new PhoneThread);
	m->re->setAccount(m->appsettings.account);

	connect(&*m->re, &PhoneThread::incoming, [&](QString const &text){
		ui->label_message->setText(text);
	});

	m->re->start();
}

void MainWindow::push(int n)
{
	if (n >= 0 && n <= 9) {
		n += '0';
	}
	if ((n >= '0' && n <= '9') || (n == PAD_ASTER || n == PAD_SHARP)) {
		QString text = ui->lineEdit_phone_number->text();
		text += n;
		ui->lineEdit_phone_number->setText(text);
	}
}

void MainWindow::on_toolButton_pad_1_clicked()
{
	push(1);
}

void MainWindow::on_toolButton_pad_2_clicked()
{
	push(2);
}

void MainWindow::on_toolButton_pad_3_clicked()
{
	push(3);
}

void MainWindow::on_toolButton_pad_4_clicked()
{
	push(4);
}

void MainWindow::on_toolButton_pad_5_clicked()
{
	push(5);
}

void MainWindow::on_toolButton_pad_6_clicked()
{
	push(6);
}

void MainWindow::on_toolButton_pad_7_clicked()
{
	push(7);
}

void MainWindow::on_toolButton_pad_8_clicked()
{
	push(8);
}

void MainWindow::on_toolButton_pad_9_clicked()
{
	push(9);
}

void MainWindow::on_toolButton_pad_0_clicked()
{
	push(0);
}

void MainWindow::on_toolButton_pad_aster_clicked()
{
//	push(PAD_ASTER);
}

void MainWindow::on_toolButton_pad_sharp_clicked()
{
//	push(PAD_SHARP);
}

void MainWindow::on_toolButton_clear_clicked()
{
	ui->lineEdit_phone_number->clear();
}

void MainWindow::on_toolButton_call_clicked()
{
	if (!m->re) return;
	QString text = ui->lineEdit_phone_number->text();
	m->re->dial(text);
}

void MainWindow::on_toolButton_hungup_clicked()
{
	if (!m->re) return;
	m->re->hangup();
}

void MainWindow::on_toolButton_answer_clicked()
{
	if (!m->re) return;
	m->re->answer();
}

void MainWindow::on_action_settings_triggered()
{
	SettingsDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted) {
		ApplicationSettings const &newsettings = dlg.settings();
		bool rr = m->appsettings.account != newsettings.account;
		m->appsettings = newsettings;
		if (rr) {
			reregister();
		}
	}
}
