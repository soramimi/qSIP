#include "PhoneThread.h"
#include "PhoneWidget.h"
#include "ui_PhoneWidget.h"
#include <QDateTime>
#include <QDebug>
#include <memory>

enum {
	PAD_ASTER = '*',
	PAD_SHARP = '#',
};


struct PhoneWidget::Private {
	std::shared_ptr<PhoneThread> phone;
	QTime registration_time;
	int registration_seconds = 0;
	QString dtmf;
	QTime ringing_time;
	int ringing_timeout_sec = 30;
	bool answer_enabled = true;

	handler_fn_t outgoing_established_handler_fn;
	handler_fn_t closed_handler_fn;
	handler_fn_t absence_handler_fn;
};

PhoneWidget::PhoneWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::PhoneWidget)
	, m(new Private)
{
	ui->setupUi(this);

	setStatusText(QString());

	updateUI();
	startTimer(10);
}

PhoneWidget::~PhoneWidget()
{
	close();
	delete ui;
	delete m;
}

void PhoneWidget::close()
{
	if (m->phone) {
		m->phone->close();
	}
	m->phone.reset();
}

void PhoneWidget::hangup()
{
	if (m->phone) m->phone->hangup();
}

void PhoneWidget::setup(SIP::Account const &account)
{
	close();

	m->phone = std::shared_ptr<PhoneThread>(new PhoneThread("qSIP"));
	m->phone->setAccount(account);

	connect(m->phone.get(), SIGNAL(registered(bool)), this, SLOT(onRegistered(bool)));
	connect(m->phone.get(), SIGNAL(state_changed(int)), this, SLOT(onStateChanged(int)));
	connect(m->phone.get(), SIGNAL(call_incoming(QString)), this, SLOT(onCallIncoming(QString)));
	connect(m->phone.get(), SIGNAL(incoming_established()), this, SLOT(onIncomingEstablished()));
	connect(m->phone.get(), SIGNAL(outgoing_established()), this, SLOT(onOutgoingEstablished()));
	connect(m->phone.get(), SIGNAL(closed(int, int)), this, SLOT(onClosed(int, int)));
	connect(m->phone.get(), SIGNAL(dtmf_input(QString)), this, SLOT(onDTMF(QString)));

	m->phone->start();

	m->registration_seconds = 0;
	m->registration_time = QTime();
	m->registration_time.start();

	updateUI();
}

void PhoneWidget::setCallingTimeout(int sec)
{
	m->ringing_timeout_sec = sec;
}

void PhoneWidget::enableAnswer(bool f)
{
	m->answer_enabled = f;
	updateUI();
}

void PhoneWidget::setText(QString const &text)
{
	ui->lineEdit_phone_number->setText(text);
}

void PhoneWidget::setRegistrationStatusText(QString const &text)
{
	ui->label_regitration_status->setText(text);
}

void PhoneWidget::setStatusText(QString const &text)
{
	ui->label_message->setText(text.isEmpty() ? "-" : text);
}

bool PhoneWidget::isRegistered() const
{
	return m->phone && m->phone->isRegistered();
}

void PhoneWidget::updateRegistrationStatus()
{
	if (isRegistered()) {
		QString s = "%1 Ready.";
		s = s.arg(m->phone->account().user);
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

void PhoneWidget::updateUI()
{
	bool idle = m->phone && m->phone->peerNumber().isEmpty();
	if (idle) {
		setStatusText(QString());
	}
	ui->toolButton_answer->setEnabled(m->answer_enabled && idle);
	updateRegistrationStatus();
}

void PhoneWidget::onStateChanged(int state)
{
	PhoneState s = (PhoneState)state;
	if (s != PhoneState::Outgoing) {
		m->ringing_time = QTime();
	}

	updateUI();
}

void PhoneWidget::restart()
{
	if (!m->phone->reregister()) {
		SIP::Account a = m->phone->account();
		setup(a);
	}
}

void PhoneWidget::onRegistered(bool f)
{
	updateUI();
}



void PhoneWidget::onCallIncoming(QString const &from)
{
	QString text = from;
	int i = text.indexOf(':');
	if (i > 0) {
		i++;
		int j = text.indexOf('@', i);
		if (j > i) {
			text = text.mid(i, j - i);
		}
	}
	text = tr("Incoming call from: ") + text;
	setStatusText(text);
	ui->checkBox_hold->setChecked(false);
}

void PhoneWidget::onIncomingEstablished()
{
	m->dtmf.clear();

	QString s = "Incoming call established from %1";
	s = s.arg(m->phone->account().user);
	setStatusText(s);
}

void PhoneWidget::onOutgoingEstablished()
{
	m->dtmf.clear();

	QString s = "Outgoing call established to %1";
	s = s.arg(m->phone->peerNumber());
	setStatusText(s);

	if (m->outgoing_established_handler_fn) {
		m->outgoing_established_handler_fn(true, QString());
	}
}

void PhoneWidget::setOutgoingEstablishedHandler(handler_fn_t handler)
{
	m->outgoing_established_handler_fn = handler;
}

void PhoneWidget::setClosedHandler(handler_fn_t handler)
{
	m->closed_handler_fn = handler;
}



QString PhoneWidget::dtmftext() const
{
	return m->dtmf;
}

void PhoneWidget::onClosed(int dir, int condition)
{
	setStatusText(QString());
	ui->checkBox_hold->setChecked(false);
	updateUI();

	int cond = 0;
	QString text;
	if (condition == (int)Condition::Rejected) {
		cond = (int)Condition::Rejected;
	}
	if (m->closed_handler_fn) {
		m->closed_handler_fn(cond, text);
	}
}

void PhoneWidget::onDTMF(QString const &text)
{
	m->dtmf += text;
	qDebug() << m->dtmf;
}

void PhoneWidget::push(int n)
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

void PhoneWidget::on_toolButton_clear_clicked()
{
	ui->lineEdit_phone_number->clear();
}

void PhoneWidget::on_toolButton_pad_0_clicked()
{
	push(0);
}

void PhoneWidget::on_toolButton_pad_1_clicked()
{
	push(1);
}

void PhoneWidget::on_toolButton_pad_2_clicked()
{
	push(2);
}

void PhoneWidget::on_toolButton_pad_3_clicked()
{
	push(3);
}

void PhoneWidget::on_toolButton_pad_4_clicked()
{
	push(4);
}

void PhoneWidget::on_toolButton_pad_5_clicked()
{
	push(5);
}

void PhoneWidget::on_toolButton_pad_6_clicked()
{
	push(6);
}

void PhoneWidget::on_toolButton_pad_7_clicked()
{
	push(7);
}

void PhoneWidget::on_toolButton_pad_8_clicked()
{
	push(8);
}

void PhoneWidget::on_toolButton_pad_9_clicked()
{
	push(9);
}

void PhoneWidget::on_toolButton_pad_aster_clicked()
{
	push(PAD_ASTER);
}

void PhoneWidget::on_toolButton_pad_sharp_clicked()
{
	push(PAD_SHARP);
}

void PhoneWidget::on_toolButton_hangup_clicked()
{
	hangup();
	setStatusText(QString());
}

void PhoneWidget::call(QString const &to)
{
	m->dtmf.clear();
	m->ringing_time = QTime();

	if (m->phone->call(to)) {
		QString s = "Calling to %1";
		s = s.arg(m->phone->peerNumber());
		setStatusText(s);
		m->ringing_time.start();
	}
	updateUI();
}

void PhoneWidget::on_toolButton_call_clicked()
{
	if (!m->phone) return;
	QString text = ui->lineEdit_phone_number->text();
	call(text);
}

void PhoneWidget::on_toolButton_answer_clicked()
{
	if (!m->phone) return;
	m->phone->answer();
}

void PhoneWidget::on_checkBox_hold_toggled(bool checked)
{
	(void)checked;
	if (!m->phone) return;
	m->phone->answer();
	bool hold = ui->checkBox_hold->isChecked();
	m->phone->hold(hold);
}

void PhoneWidget::timerEvent(QTimerEvent *event)
{
	if (isRegistered()) {
		if (m->phone->state() == PhoneState::Outgoing) {
			if (m->ringing_time.isValid()) {
				int s = m->ringing_time.elapsed() / 1000;
				if (s >= m->ringing_timeout_sec) {
					hangup();
					if (m->closed_handler_fn) {
						m->closed_handler_fn((int)Condition::Absence, QString());
					}
				}
			}
		}
		if (m->phone->voice() && m->phone->isEndOfVoice()) {
			hangup();
		}
	} else {
		int elapsed = m->registration_time.elapsed() / 1000;
		if (m->registration_seconds != elapsed) {
			m->registration_seconds = elapsed;
			updateUI();
		}
	}
}

void PhoneWidget::setVoice(VoicePtr voice)
{
	m->phone->setVoice(voice);
}

void PhoneWidget::setVisibleHoldCheckBox(bool f)
{
	ui->checkBox_hold->setVisible(f);
}

