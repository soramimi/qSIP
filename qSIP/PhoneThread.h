#ifndef PHONETHREAD_H
#define PHONETHREAD_H

#include <QThread>
#include "Account.h"
#include <memory>
#include <stdint.h>

#ifndef Q_OS_WIN
#include <re_list.h>
#include <re_fmt.h>
#include <re_sdp.h>
#include <baresip.h>
#endif

#include <baresip/modules/qtaudio/audioio.h>

#ifdef bool
#undef bool
#endif

class ApplicationSettings;
class QAudioInput;
class QAudioOutput;
class QIODevice;

struct Voice {
	QByteArray ba;
	int offset = 0;
	int count = 0;
	int pos = 0;
};
typedef std::shared_ptr<Voice> VoicePtr;

enum class PhoneState {
	None,
	Idle,
	Outgoing,
	Incoming,
	Established,
};

enum class Direction {
	None,
	Incoming,
	Outgoing,
};

enum class Condition {
	None,
	Absence,
	Rejected,
};

class PhoneThread : public QThread, public AudioIO {
	Q_OBJECT
private:
	struct Private;
	Private *m;
	static void signalHandler(int sig);

	static void controlHandler(void);
	void onPhoneEvent(struct ua *ua, enum ua_event ev, struct call *call, const char *prm);

	static void eventHandler(struct ua *ua, enum ua_event ev, struct call *call, const char *prm, void *arg);

	void setState(PhoneState s);
	void clearPeerUser();
	const char *uaName() const;
	QAudioInput *audioInput();
	QIODevice *audioInputDevice();
	QAudioOutput *audioOutput();
	QIODevice *audioOutputDevice();
	void customNotify(const char *ptr, int len);
	void detectDTMF(int size, const int16_t *data);
	void resetCallbackPtr();
public:
	PhoneThread(const ApplicationSettings *as, const std::string &user_agent);
	~PhoneThread();

	PhoneState state() const;
	Direction direction() const;

	bool call(const QString &text);
	void hangup();
	void answer();
	void hold(bool f);
	void setAccount(const ApplicationSettings &settings);
	const ApplicationSettings &settings() const;

	bool isRegistered() const;
	bool isIdling() const;

	void setVoice(VoicePtr voice);

	void resetVoice();
	const Voice *voice() const;
	void close();
	bool isEndOfVoice() const;
	struct ua *ua();
	QString peerNumber() const;
	bool reregister();

protected:
	void run();
	void timerEvent(QTimerEvent *);
signals:
	void registered(bool reg);
	void unregistering();
	void callIncoming(QString const &from);
	void closed(int dir, int condition);
	void incomingEstablished();
	void outgoingEstablished();
	void inputDTMF(QString const &text);
	void stateChanged(int);


	// AudioIO interface
public:
	int input(char *ptr, int len);
	int output(char *ptr, int len);
};


#endif // PHONETHREAD_H
