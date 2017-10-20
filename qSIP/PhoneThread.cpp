#include "PhoneThread.h"
#include <string.h>
#include <QDebug>

#include <re.h>
#include "baresip.h"


struct PhoneThread::Private {
	std::string user_agent;
	PhoneState state = PhoneState::None;
	Direction direction = Direction::None;
	struct ua *ua = nullptr;
	struct call *call = nullptr;
	user_extra_data_t user_extra_data;
	SIP::Account account;
	QString peer_number;
	VoicePtr voice;
};

PhoneThread::PhoneThread(std::string const &user_agent)
	: m(new Private)
{
	m->user_agent = user_agent;
}

PhoneThread::~PhoneThread()
{
	delete m;
}

char const *PhoneThread::uaName() const
{
	return m->user_agent.empty() ? "qSIP" : m->user_agent.c_str();
}

PhoneState PhoneThread::state() const
{
	return m->state;
}

Direction PhoneThread::direction() const
{
	return m->direction;
}

void PhoneThread::setState(PhoneState s)
{
	if (m->state != s) {
		m->state = s;
		emit state_changed((int)m->state);
	}
}

struct ua *PhoneThread::ua()
{
	return m->ua;
}

void PhoneThread::setAccount(const SIP::Account &account)
{
	m->account = account;
}

SIP::Account const &PhoneThread::account() const
{
	return m->account;
}

QString PhoneThread::peerNumber() const
{
	return m->peer_number;
}

bool PhoneThread::reregister()
{
	if (m->ua) {
		if (ua_reregister(m->ua) == 0) {
			return true;
		}
	}
	return false;
}

bool PhoneThread::isRegistered() const
{
	return ua_isregistered(m->ua);
}

bool PhoneThread::isIdling() const
{
	return state() == PhoneState::Idle;
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

void PhoneThread::clearPeerUser()
{
	m->peer_number = QString();
}

void PhoneThread::onEvent(struct ua *ua, ua_event ev, struct call *call, const char *prm)
{
	static char const *eventname[] = {
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
		"UA_EVENT_CALL_TRANSFER",
		"UA_EVENT_CALL_TRANSFER_OOD",
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
	case UA_EVENT_UNREGISTERING:
		emit unregistering();
		break;
	case UA_EVENT_CALL_INCOMING:
		setState(PhoneState::Incoming);
		m->direction = Direction::Incoming;
		{
			if (prm && *prm) {
				emit call_incoming(prm);
			}
		}
		break;
	case UA_EVENT_CALL_OUTGOING:
		m->direction = Direction::Outgoing;
		break;
	case UA_EVENT_CALL_TRYING:
		setState(PhoneState::Outgoing);
		break;
	case UA_EVENT_CALL_RINGING:
		setState(PhoneState::Outgoing);
		break;
	case UA_EVENT_CALL_ESTABLISHED:
		setState(PhoneState::Established);
		switch (m->direction) {
		case Direction::Incoming:
			emit incoming_established();
			break;
		case Direction::Outgoing:
			emit outgoing_established();
			break;
		}
		break;
	case UA_EVENT_CALL_CLOSED:
		m->call = nullptr;
		clearPeerUser();
		emit closed((int)direction(), (int)(m->state == PhoneState::Outgoing ? Condition::Rejected : Condition::None));
		m->direction = Direction::None;
		setState(PhoneState::Idle);
		break;
	case UA_EVENT_CALL_DTMF_START:
		emit dtmf_input(prm);
		qDebug() << prm;
		break;
	}
}

void PhoneThread::event_handler(struct ua *ua, ua_event ev, struct call *call, const char *prm, void *arg)
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

bool PhoneThread::call(const QString &text)
{
	clearPeerUser();

	if (m->account.server.isEmpty()) return false;

	QString peer_number;

	for (int i = 0; i < text.size(); i++) {
		ushort c = text.utf16()[i];
		if (QChar(c).isDigit()) {
			peer_number += c;
		} else if (c == '-' || QChar(c).isSpace()) {
			// nop
		} else {
			return false;
		}
	}

	if (peer_number.isEmpty()) return false;

	QString url = "sip:%1@%2";
	url = url.arg(peer_number).arg(makeServerAddress(m->account));
	int r = ua_connect((struct ua *)m->ua, &m->call, nullptr, url.toStdString().c_str(), nullptr, VIDMODE_OFF);
	if (r == 0) {
		m->peer_number = peer_number;
	}
	return r == 0;
}

void PhoneThread::run()
{
	libre_init();
	mod_init();

	configure();

	uag_event_register(event_handler, this);

	if (m->account.server.isEmpty()) {
		// nop
	} else {
		QString aor = "<sip:%1@%2;transport=udp>;audio_codecs=PCMU/8000/1,PCMA/8000/1";
		aor = aor
				.arg(m->account.user)
				.arg(makeServerAddress(m->account))
				;
		ua_init(uaName(), false, true, true, true, false);
		ua_alloc((struct ua **)&m->ua, aor.toStdString().c_str(), m->account.password.toStdString().c_str(), m->account.user.toStdString().c_str());
	}
//	m->user_extra_data.audio_source_filter_fn = custom_filter_handler;
//	m->user_extra_data.cookie = this;
	setState(PhoneState::Idle);

	while (1) {
		m->user_extra_data.filter = custom_filter_handler;
		m->user_extra_data.cookie = this;
		int r = re_main(signal_handler, control_handler, &m->user_extra_data);
		if (r == EINTR || r == ENOENT) {
			// continue
		} else {
			break;
		}
	}

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
	m->ua = nullptr;
	m->call = nullptr;
}

void PhoneThread::hangup()
{
	ua_hangup(m->ua, nullptr, 0, nullptr);
	clearPeerUser();
	m->direction = Direction::None;
	setState(PhoneState::Idle);
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
	ua_answer(m->ua, nullptr, 0, nullptr, nullptr);
}

void PhoneThread::hold(bool f)
{
	struct call *c = ua_call(m->ua);
	if (c) {
		call_hold(c, f);
	}
}

