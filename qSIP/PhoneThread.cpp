#include "PhoneThread.h"

#include <QDebug>



struct PhoneThread::Private {
	struct ua *ua = nullptr;
	SIP::Account account;
};

PhoneThread::PhoneThread()
	: m(new Private)
{

}

PhoneThread::~PhoneThread()
{
	delete m;
}

void PhoneThread::setAccount(const SIP::Account &account)
{
	m->account = account;
}

void PhoneThread::signal_handler(int sig)
{
}

void PhoneThread::control_handler()
{
}

void PhoneThread::onEvent(struct ua *ua, ua_event ev, call *call, const char *prm)
{
	std::string peername;
	char const *p = call_peername(call);;
	if (p) peername = p;

	qDebug() << ev;
	switch (ev) {
	case UA_EVENT_CALL_INCOMING:
		{
			QString from = QString::fromLatin1(prm);
			if (!from.isEmpty()) {
				from = tr("Incoming call from") + "\n" + from;
				emit incoming(from);
			}
		}
		break;
	case UA_EVENT_CALL_CLOSED:
		emit incoming(QString());
		break;
	}
}

void PhoneThread::event_handler(struct ua *ua, ua_event ev, call *call, const char *prm, void *arg)
{
	PhoneThread *me = (PhoneThread *)arg;
	me->onEvent(ua, ev, call, prm);
}

static QString makeServerAddress(SIP::Account const &a)
{
	QString addr = a.server;
	int i = addr.indexOf(':');
	if (i >= 0) {
		addr = addr.mid(0, i);
	}
	addr += ':';
	addr += QString::number(a.port);
	return addr;
}

bool PhoneThread::dial(const QString &text)
{
	if (m->account.server.isEmpty()) return false;

	for (int i = 0; i < text.size(); i++) {
		ushort c = text.utf16()[i];
		if (!QChar(c).isDigit()) {
			return false;
		}
	}

	QString url = "sip:%1@%2";
	url = url.arg(text).arg(makeServerAddress(m->account));
	ua_connect((struct ua *)m->ua, nullptr, nullptr, url.toStdString().c_str(), nullptr, VIDMODE_OFF);
	return true;
}

void PhoneThread::init()
{
	int r = 0;
	libre_init();
	mod_init();
	r = configure();
}

void PhoneThread::run()
{
	int r = 0;
	r = uag_event_register(event_handler, this);

	if (m->account.server.isEmpty()) {
		// nop
	} else {
		QString aor = "sip:%1@%2";
		aor = aor.arg(m->account.user).arg(makeServerAddress(m->account));
		r = ua_init("SIP", false, true, true, true, false);
		r = ua_alloc((struct ua **)&m->ua, aor.toStdString().c_str(), m->account.password.toStdString().c_str(), m->account.user.toStdString().c_str());
		r = ua_reregister((struct ua *)m->ua);
	}
	re_main(signal_handler, control_handler);
}

void PhoneThread::hangup()
{
	ua_hangup(m->ua, nullptr, 0, nullptr);
}

void PhoneThread::answer()
{
	ua_answer(m->ua, nullptr, 0, nullptr);
}

