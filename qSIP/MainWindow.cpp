#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "PhoneThread.h"
#include "SettingsDialog.h"
#include "StatusLabel.h"

#include <QDateTime>
#include <QDebug>
#include "memory"

enum {
	PAD_ASTER = '*',
	PAD_SHARP = '#',
};

struct MainWindow::Private {
	StatusLabel *status_label;
	ApplicationSettings appsettings;
	struct ua *ua = nullptr;
	std::shared_ptr<PhoneThread> phone;
	bool registered = false;
	QTime registration_time;
	int registration_seconds = 0;
};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m(new Private)
{
	ui->setupUi(this);
	m->status_label = new StatusLabel;
	ui->statusBar->addWidget(m->status_label);

	updateRegistrationStatus();

	SettingsDialog::loadSettings(&m->appsettings);
	startTimer(10);

	reregister();
}

MainWindow::~MainWindow()
{
	close();
	delete m;
	delete ui;
}

QString MainWindow::statusText() const
{
	return m->status_label->text();
}

void MainWindow::setStatusText(QString const &text)
{
	m->status_label->setText(text);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
	if (m->registered) {
		// ok
	} else {
		int elapsed = m->registration_time.elapsed() / 1000;
		if (m->registration_seconds != elapsed) {
			m->registration_seconds = elapsed;
			updateRegistrationStatus();
		}
	}
}

void MainWindow::close()
{
	if (m->phone) {
		m->phone->close();
	}
	m->phone.reset();
}

void MainWindow::reregister()
{
	close();
	PhoneThread::init();

	m->phone = std::shared_ptr<PhoneThread>(new PhoneThread);
	m->phone->setAccount(m->appsettings.account);

	connect(&*m->phone, SIGNAL(incoming(QString)), this, SLOT(onIncoming(QString)));
	connect(&*m->phone, SIGNAL(calling_established()), this, SLOT(onCallingEstablished()));
	connect(&*m->phone, SIGNAL(closed()), this, SLOT(onClosed()));
	connect(&*m->phone, SIGNAL(dtmf_input(QString)), this, SLOT(onDTMF(QString)));
	connect(&*m->phone, SIGNAL(registered(bool)), this, SLOT(onRegistered(bool)));

	m->phone->start();

	m->registered = false;
	m->registration_seconds = 0;
	m->registration_time = QTime();
	m->registration_time.start();
}

void MainWindow::setRegistrationStatusText(QString const &text)
{
	ui->label_regitration_status->setText(text);
}

void MainWindow::updateRegistrationStatus()
{
	if (m->registered) {
		QString s = "%1 Ready.";
		s = s.arg(m->appsettings.account.user);
		setRegistrationStatusText(s);
	} else {
		if (m->registration_seconds < 3) {
			setRegistrationStatusText("NOT REGISTERD !");
		} else {
			QString s = "Wait for registration %1s";
			s = s.arg(m->registration_seconds);
			setRegistrationStatusText(s);
		}
	}
}

void MainWindow::onRegistered(bool f)
{
	m->registered = f;
	updateRegistrationStatus();
}

void MainWindow::onIncoming(QString const &text)
{
	ui->label_message->setText(text);
	setStatusText(QString());
	ui->checkBox_hold->setChecked(false);
}

void MainWindow::onCallingEstablished()
{
	qDebug() << Q_FUNC_INFO;
}

void MainWindow::onClosed()
{
	ui->label_message->clear();
	setStatusText(QString());
	ui->checkBox_hold->setChecked(false);
}

void MainWindow::onDTMF(QString const &text)
{
	QString s = statusText();
	s += text;
	setStatusText(s);
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
	if (!m->phone) return;
	QString text = ui->lineEdit_phone_number->text();
	m->phone->dial(text);
}

void MainWindow::on_toolButton_hangup_clicked()
{
	if (!m->phone) return;
	m->phone->hangup();
}

void MainWindow::on_toolButton_answer_clicked()
{
	if (!m->phone) return;
	m->phone->answer();
}

void MainWindow::on_action_settings_triggered()
{
	SettingsDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted) {
		ApplicationSettings const &newsettings = dlg.settings();
		bool eq = m->appsettings.account == newsettings.account;
		m->appsettings = newsettings;
		if (!eq) {
			reregister();
		}
	}
}

void MainWindow::on_checkBox_hold_clicked()
{
	if (!m->phone) return;
	m->phone->answer();
	bool hold = ui->checkBox_hold->isChecked();
	m->phone->hold(hold);
}

