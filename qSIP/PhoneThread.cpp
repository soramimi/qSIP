
#include "PhoneThread.h"
#include "Network.h"
#include "main.h"
#include <QDebug>
#include <re.h>
#include <baresip.h>
#include <string.h>
#include <QAudioInput>
#include <QAudioOutput>

#ifdef Q_OS_WIN
#define _USE_MATH_DEFINES
#else
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#endif

#include <QMutex>
#include <math.h>

// dtmf detection

static const double  PI2 = M_PI * 2;

static const int dtmf_fq[8] = { 697, 770, 852, 941, 1209, 1336, 1477, 1633 };

double goertzel_(int size, int16_t const *data, int sample_fq, int detect_fq)
{
	if (size < 2) return 0;

	double omega = PI2 * detect_fq / sample_fq;
	double sine = sin(omega);
	double cosine = cos(omega);
	double coeff = cosine * 2;
	double q0 = 0;
	double q1 = 0;
	double q2 = 0;

	for (int i = 0; i < size; i++) {
		q0 = coeff * q1 - q2 + data[i];
		q2 = q1;
		q1 = q0;
	}

	double real = (q1 - q2 * cosine) / (size / 2.0);
	double imag = (q2 * sine) / (size / 2.0);

	return sqrt(real * real + imag * imag);
}

const int DTMF_MIN_LEVEL = 500;
const int DTMF_MIN_COUNT = 300;

//

struct PhoneThread::Private {
#ifndef Q_OS_WIN
	pthread_t thread_id = 0;
#endif
	QMutex mutex;
	std::string user_agent;
	PhoneState state = PhoneState::None;
	Direction direction = Direction::None;
	struct ua *ua = nullptr;
	struct call *call = nullptr;
	user_extra_data_t user_extra_data = {};
	ApplicationSettings settings;
	QString server_ip_address;
	QString peer_number;
	VoicePtr voice;

	std::shared_ptr<QAudioInput> audio_input;
	std::shared_ptr<QAudioOutput> audio_output;
	QIODevice *audio_input_device = nullptr;
	QIODevice *audio_output_device = nullptr;

	int dtmf_value = 0;
	int dtmf_count = 0;

	const int sample_fq = 8000;
	double dtmf_levels[8] = {};
};

PhoneThread::PhoneThread(ApplicationSettings const *as, std::string const &user_agent)
	: m(new Private)
{
	m->user_agent = user_agent;

	QAudioDeviceInfo idev = QAudioDeviceInfo::defaultInputDevice();
	QAudioDeviceInfo odev = QAudioDeviceInfo::defaultOutputDevice();

	for (auto const &info : QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
		if (info.deviceName() == as->audio_input) {
			idev = info;
		}
	}
	for (auto const &info : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
		if (info.deviceName() == as->audio_output) {
			odev = info;
		}
	}

	QAudioFormat format;
	format.setChannelCount(1);
	format.setSampleRate(8000);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setSampleType(QAudioFormat::SignedInt);
	format.setByteOrder(QAudioFormat::LittleEndian);
	m->audio_input = std::shared_ptr<QAudioInput>(new QAudioInput(idev, format));
	m->audio_output = std::shared_ptr<QAudioOutput>(new QAudioOutput(odev, format));
	m->audio_output->setBufferSize(800);
	m->audio_input_device = m->audio_input->start();
	m->audio_output_device = m->audio_output->start();

	startTimer(10);
}

PhoneThread::~PhoneThread()
{
	resetCallbackPtr();
	delete m;
}

void PhoneThread::resetCallbackPtr()
{
	QMutexLocker lock(&m->mutex);
	m->user_extra_data.callback_input_p = nullptr;
	m->user_extra_data.callback_output_p = nullptr;
	m->user_extra_data.callback_input_f = nullptr;
	m->user_extra_data.callback_output_f = nullptr;
}

void PhoneThread::customNotify(char const *ptr, int len)
{
	if (len > 6 && strncmp(ptr, "<dtmf>", 6) == 0) {
		char tmp[2];
		tmp[0] = ptr[6];
		tmp[1] = 0;
		emit inputDTMF(tmp);
	}
}

void PhoneThread::detectDTMF(int size, int16_t const *data)
{
	double (&levels)[8] = m->dtmf_levels;

	for (int i = 0; i < 8; i++) {
		levels[i] = goertzel_(size, data, m->sample_fq, dtmf_fq[i]);
	}

	int v = 0;
	struct Tone {
		int index;
		int level;
		Tone(int i, int v)
			: index(i)
			, level(v)
		{
		}
	};
	Tone lo[] = { Tone(0, levels[0]), Tone(1, levels[1]), Tone(2, levels[2]), Tone(3, levels[3]) };
	Tone hi[] = { Tone(0, levels[4]), Tone(1, levels[5]), Tone(2, levels[6]), Tone(3, levels[7]) };
	std::sort(lo, lo + 4, [](Tone const &l, Tone const &r){ return l.level > r.level; });
	std::sort(hi, hi + 4, [](Tone const &l, Tone const &r){ return l.level > r.level; });
	if (lo[0].level > DTMF_MIN_LEVEL && hi[0].level > DTMF_MIN_LEVEL && lo[0].level > lo[1].level * 3 && hi[0].level > hi[1].level * 3) {
		int i = lo[0].index * 4 + hi[0].index;
		v = "123A456B789C*0#D"[i];
	}
	if (v != 0 && v == m->dtmf_value) {
		if (m->dtmf_count < DTMF_MIN_COUNT) {
			m->dtmf_count += size;
			if (m->dtmf_count >= DTMF_MIN_COUNT) {
				char tmp[100];
				sprintf(tmp, "<dtmf>%c", m->dtmf_value);
				customNotify(tmp, strlen(tmp));
			}
		}
	} else {
		m->dtmf_value = v;
		m->dtmf_count = 0;
	}
}

int PhoneThread::input(char *ptr, int len)
{
	if (m->voice) {
		int l = len / sizeof(int16_t);
		int n = m->voice->count;
		if (m->voice->pos < n) {
			n -= m->voice->pos;
			if (n > l) n = l;
			int16_t *dst = (int16_t *)ptr;
			if (m->voice->pos == 0) {
				dst += l - n;
			}
			int16_t const *p = (int16_t const *)(m->voice->ba.data() + m->voice->offset) + m->voice->pos;
			memcpy(dst, p, n * sizeof(int16_t));
			m->voice->pos += n;
		} else {
			len = 0;
		}
	} else {
		if (audioInput() && audioInputDevice()) {
			if (audioInput()->bytesReady() < len) {
				len = 0;
			} else {
				len = audioInputDevice()->read(ptr, len);
			}
		} else {
			len = 0;
		}
	}
	return len;
}

int PhoneThread::output(char *ptr, int len)
{
	if (!ptr) {
		return audioOutput()->bytesFree();
	}

	audioOutputDevice()->write(ptr, len);

	int l = len / 2;
	if (l > 0) {
		detectDTMF(l, (int16_t *)ptr);
	}

	return len;
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
		emit stateChanged((int)m->state);
	}
}

struct ua *PhoneThread::ua()
{
	return m->ua;
}

void PhoneThread::setAccount(ApplicationSettings const &as)
{
	m->settings = as;

	QString addr = m->settings.account.server;
	{
		QStringList list = Network::resolveHostAddress(addr);
		if (!list.empty()) {
			addr = list.front();
		}
	}
	m->server_ip_address = addr;
}

ApplicationSettings const &PhoneThread::settings() const
{
	return m->settings;
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

void PhoneThread::clearPeerUser()
{
	m->peer_number = QString();
}

void PhoneThread::onPhoneEvent(struct ua *ua, ua_event ev, struct call *call, const char *prm)
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
		if (prm && *prm) {
			emit callIncoming(prm);
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
			emit incomingEstablished();
			break;
		case Direction::Outgoing:
			emit outgoingEstablished();
			break;
		}
		break;
	case UA_EVENT_CALL_CLOSED:
		resetCallbackPtr();
		m->call = nullptr;
		clearPeerUser();
		emit closed((int)direction(), (int)(m->state == PhoneState::Outgoing ? Condition::Rejected : Condition::None));
		m->direction = Direction::None;
		setState(PhoneState::Idle);
		break;
	case UA_EVENT_CALL_DTMF_START:
		emit inputDTMF(prm);
		qDebug() << prm;
		break;
	}
}

void PhoneThread::eventHandler(struct ua *ua, ua_event ev, struct call *call, const char *prm, void *arg)
{
	PhoneThread *me = (PhoneThread *)arg;
	me->onPhoneEvent(ua, ev, call, prm);
}

static QString makeServerAddress(SIP::Account const &a)
{

	QString addr = a.service_domain;
	int i = addr.indexOf(':');
	if (i >= 0) {
		addr = addr.mid(0, i);
	}
	return addr;
}

bool PhoneThread::call(const QString &text)
{
	clearPeerUser();

	if (m->settings.account.server.isEmpty()) return false;

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
	url = url.arg(peer_number).arg(makeServerAddress(m->settings.account));
	int r = ua_connect((struct ua *)m->ua, &m->call, nullptr, url.toLatin1(), nullptr, VIDMODE_OFF);
	if (r == 0) {
		m->peer_number = peer_number;
	}
	return r == 0;
}

void PhoneThread::signalHandler(int sig)
{
}

void PhoneThread::controlHandler()
{
}

void PhoneThread::run()
{
#ifndef Q_OS_WIN
    m->thread_id = pthread_self();
#endif

	libre_init();
	mod_init();

	configure();

	struct config *cfg = conf_config();
	strcpy(cfg->audio.src_mod, "qtaudio");
	strcpy(cfg->audio.play_mod, "qtaudio");
	strcpy(cfg->audio.alert_mod, "qtaudio");

	uag_event_register(eventHandler, this);

	if (m->settings.account.server.isEmpty()) {
		// nop
	} else {
		QString aor = "<sip:%1@%2;transport=udp>;auth_user=%4;outbound1=%5;audio_codecs=PCMU/8000/1,PCMA/8000/1";
		aor = aor
				.arg(m->settings.account.phone_number)
				.arg(makeServerAddress(m->settings.account))
				.arg(m->settings.account.user)
				.arg(QString("sip:%1:%2").arg(m->server_ip_address).arg(m->settings.account.port))
				;
		ua_init(uaName(), false, true, true, true, false);
		ua_alloc((struct ua **)&m->ua, aor.toLatin1(), m->settings.account.password.toLatin1(), m->settings.account.phone_number.toLatin1());
	}
	setState(PhoneState::Idle);

	while (1) {
        if (isInterruptionRequested()) {
            qDebug() << "interrupted";
            break;
        }
		resetCallbackPtr();
		int r = re_main(signalHandler, controlHandler, &m->user_extra_data);
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

void PhoneThread::timerEvent(QTimerEvent *)
{
	QMutexLocker lock(&m->mutex);
	if (m->user_extra_data.callback_input_f) {
		m->user_extra_data.callback_input_f(m->user_extra_data.callback_input_p, static_cast<AudioIO *>(this));
	}
	if (m->user_extra_data.callback_output_f) {
		m->user_extra_data.callback_output_f(m->user_extra_data.callback_output_p, static_cast<AudioIO *>(this));
	}
}

void PhoneThread::close()
{
	hangup();
    requestInterruption();
    re_cancel();
#ifndef Q_OS_WIN
	while (isRunning()) {
        pthread_kill(m->thread_id, SIGINT);
        msleep(1);
    }
#else
	wait();
#endif
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

QAudioInput *PhoneThread::audioInput()
{
	return m->audio_input.get();
}
QIODevice *PhoneThread::audioInputDevice()
{
	return m->audio_input_device;
}

QAudioOutput *PhoneThread::audioOutput()
{
	return m->audio_output.get();
}
QIODevice *PhoneThread::audioOutputDevice()
{
	return m->audio_output_device;
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

