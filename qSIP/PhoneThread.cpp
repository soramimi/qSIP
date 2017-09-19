#include "PhoneThread.h"

#include <QDebug>


enum class Direction {
	None,
	Incoming,
	Calling,
};

struct PhoneThread::Private {
	Direction direction = Direction::None;
	struct ua *ua = nullptr;
	bool registered = false;
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

void PhoneThread::setAccount(const SIP::Account &account)
{
	m->account = account;
}

bool PhoneThread::isRegistered() const
{
	return m->registered;
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
	case UA_EVENT_REGISTER_OK:
		qDebug() << "REGISTER_OK";
		m->registered = true;
		emit registered(true);
		break;
	case UA_EVENT_UNREGISTER_OK:
		qDebug() << "UNREGISTER_OK";
		m->registered = false;
		emit registered(false);
		break;
	case UA_EVENT_CALL_INCOMING:
		{
			QString from = prm;
			if (!from.isEmpty()) {
				from = tr("Incoming call from") + "\n" + from;
				emit incoming(from);
			}
			{
				struct play *play = nullptr;
//				int r = play_file(&play, "d:/mythbusters.wav", 1);
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

		{
//			struct audio *a = call_audio(call);
//			audio_mute(a, true);
//			struct play *play = nullptr;
//			play_file(&play, "d:/mythbusters.wav", 1);
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

	for (int i = 0; i < text.size(); i++) {
		ushort c = text.utf16()[i];
		if (!QChar(c).isDigit()) {
			return false;
		}
	}

	m->direction = Direction::Calling;

	QString url = "sip:%1@%2";
	url = url.arg(text).arg(makeServerAddress(m->account));
	int r = ua_connect((struct ua *)m->ua, nullptr, nullptr, url.toStdString().c_str(), nullptr, VIDMODE_OFF);
//	qDebug() << r;
	return true;
}

void PhoneThread::init()
{
	int r = 0;
	libre_init();
	mod_init();
	r = configure();
}

void PhoneThread::close()
{
	hangup();
	ua_close();
	re_cancel();
	if (!wait(3000)) {
		terminate();
	}
	mod_close();
	libre_close();
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
	re_main(signal_handler, control_handler, custom_filter_handler, this);
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

