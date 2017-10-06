#include "PhoneThread.h"
#include <string.h>
#include <QDebug>


enum class Direction {
	None,
	Incoming,
	Calling,
};

struct PhoneThread::Private {
	Direction direction = Direction::None;
	struct ua *ua = nullptr;
	struct call *call = nullptr;
	SIP::Account account;
	VoicePtr voice;
};

PhoneThread::PhoneThread()
	: m(new Private)
{

}

PhoneThread::~PhoneThread()
{
	delete m;
}

struct ua *PhoneThread::ua()
{
	return m->ua;
}

void PhoneThread::setAccount(const SIP::Account &account)
{
	m->account = account;
}

bool PhoneThread::isRegistered() const
{
	return ua_isregistered(m->ua);
}

Voice const *PhoneThread::voice() const
{
	return m->voice.get();
}

void PhoneThread::setVoice(VoicePtr voice)
{
	m->voice = voice;
}

void PhoneThread::resetVoice()
{
	m->voice.reset();
}

bool PhoneThread::isEndOfVoice() const
{
	Voice const *v = voice();
	if (v && v->pos < v->count) {
		return false;
	}
	return true;
}

void PhoneThread::signal_handler(int sig)
{
}

void PhoneThread::control_handler()
{
}

void PhoneThread::onEvent(struct ua *ua, ua_event ev, call *call, const char *prm)
{
	static char *eventname[] = {
		"UA_EVENT_REGISTERING",
		"UA_EVENT_REGISTER_OK",
		"UA_EVENT_REGISTER_FAIL",
		"UA_EVENT_UNREGISTERING",
		"UA_EVENT_UNREGISTER_OK",
		"UA_EVENT_UNREGISTER_FAIL",
		"UA_EVENT_CALL_INCOMING",
		"UA_EVENT_CALL_OUTGOING",
		"UA_EVENT_CALL_TRYING",
		"UA_EVENT_CALL_RINGING",
		"UA_EVENT_CALL_PROGRESS",
		"UA_EVENT_CALL_ESTABLISHED",
		"UA_EVENT_CALL_CLOSED",
		"UA_EVENT_CALL_TRANSFER_FAILED",
		"UA_EVENT_CALL_DTMF_START",
		"UA_EVENT_CALL_DTMF_END",
	};
	const int eventmax = sizeof(eventname) / sizeof(*eventname);

	std::string peername;
	char const *p = call_peername(call);;
	if (p) peername = p;

	{
		if (ev >= 0 && ev < eventmax) {
			qDebug() << eventname[ev];
		} else {
			qDebug() << QString("??? UA EVENT: %1 ???").arg(ev);
		}
	}
	switch (ev) {
	case UA_EVENT_REGISTER_OK:
		emit registered(true);
		break;
	case UA_EVENT_UNREGISTER_OK:
		emit registered(false);
		break;
	case UA_EVENT_CALL_INCOMING:
		{
			QString from = prm;
			if (!from.isEmpty()) {
				from = tr("Incoming call from") + "\n" + from;
				emit incoming(from);
			}
		}
		break;
	case UA_EVENT_CALL_ESTABLISHED:
		switch (m->direction) {
		case Direction::Incoming:
			emit incoming_established();
			break;
		case Direction::Calling:
			emit calling_established();
			break;
		}
		break;
	case UA_EVENT_CALL_CLOSED:
		emit closed();
		m->direction = Direction::None;
		break;
	case UA_EVENT_CALL_DTMF_START:
		emit dtmf_input(prm);
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

	QString nums;

	for (int i = 0; i < text.size(); i++) {
		ushort c = text.utf16()[i];
		if (QChar(c).isDigit()) {
			nums += c;
		} else if (c == '-' || QChar(c).isSpace()) {
			// nop
		} else {
			return false;
		}
	}

	m->direction = Direction::Calling;

	QString url = "sip:%1@%2";
	url = url.arg(nums).arg(makeServerAddress(m->account));
	int r = ua_connect((struct ua *)m->ua, &m->call, nullptr, url.toStdString().c_str(), nullptr, VIDMODE_OFF);
//	qDebug() << r;
	return true;
}

void PhoneThread::init()
{

//	configure();
}

static inline void strncpyz(char *dst, const char *src, int dstsize)
{
	strncpy(dst, src, dstsize);
	dst[dstsize-1] = '\0';
}

void PhoneThread::run()
{
	libre_init();
	mod_init();

	configure();

	int r = 0;
	r = uag_event_register(event_handler, this);

	if (m->account.server.isEmpty()) {
		// nop
	} else {
		QString aor = "<sip:%1@%2;transport=udp>;audio_codecs=PCMU/8000/1,PCMA/8000/1";
		aor = aor.arg(m->account.user).arg(makeServerAddress(m->account));
		r = ua_init("qSIP 0.0", false, true, true, true, false);
		r = ua_alloc((struct ua **)&m->ua, aor.toStdString().c_str(), m->account.password.toStdString().c_str(), m->account.user.toStdString().c_str());
	}
	re_main(signal_handler, control_handler, custom_filter_handler, this);

	ua_close();
	mod_close();
	libre_close();
}

void PhoneThread::close()
{
	hangup();
	re_cancel();
	if (!wait(3000)) {
		terminate();
	}
}

void PhoneThread::hangup()
{
	ua_hangup(m->ua, nullptr, 0, nullptr);
}

void PhoneThread::custom_filter(int16_t *ptr, int len)
{
	if (m->voice) {
		int n = m->voice->count;
		if (m->voice->pos < n) {
			n -= m->voice->pos;
			if (n > len) n = len;
			int16_t *dst = ptr;
			if (m->voice->pos == 0) {
				dst += len - n;
			}
			int16_t const *p = (int16_t const *)(m->voice->ba.data() + m->voice->offset) + m->voice->pos;
			memcpy(dst, p, n * sizeof(int16_t));
			m->voice->pos += n;
		} else {
			memset(ptr, 0, len * sizeof(int16_t));
		}
	}
}

void PhoneThread::custom_filter_handler(void *cookie, int16_t *ptr, int len)
{
#if 0
	(void)cookie;
	int i = 0;
	while (i < len) {
		ptr[i + 0] = (i & 4) ? 8192 : -8192;
		i++;
	}
#else
	PhoneThread *my = reinterpret_cast<PhoneThread *>(cookie);
	if (my) {
		my->custom_filter(ptr, len);
	}
#endif
}

void PhoneThread::answer()
{
	m->direction = Direction::Incoming;
	ua_answer(m->ua, nullptr, 0, nullptr, nullptr, nullptr);
}

void PhoneThread::hold(bool f)
{
	struct call *c = ua_call(m->ua);
	if (c) {
		call_hold(c, f);
	}
}

