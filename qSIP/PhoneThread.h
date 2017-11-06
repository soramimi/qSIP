#ifndef PHONETHREAD_H
#define PHONETHREAD_H

#include <QThread>
#include "Account.h"
#include "memory"

#ifndef Q_OS_WIN
#include <re_list.h>
#include <re_fmt.h>
#include <re_sdp.h>
#include <baresip.h>
#endif

#ifdef bool
#undef bool
#endif

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

class PhoneThread : public QThread {
	Q_OBJECT
private:
	struct Private;
	Private *m;
	static void signal_handler(int sig);

	static void control_handler(void);
	void onEvent(struct ua *ua, enum ua_event ev, struct call *call, const char *prm);

	static void event_handler(struct ua *ua, enum ua_event ev, struct call *call, const char *prm, void *arg);

	static void custom_filter_handler(void *cookie, int16_t *ptr, int len);
	void custom_filter(int16_t *ptr, int len);
	void setState(PhoneState s);
	void clearPeerUser();
	const char *uaName() const;
public:
	PhoneThread(const std::string &user_agent);
	~PhoneThread();

	PhoneState state() const;
	Direction direction() const;

	bool call(const QString &text);
	void hangup();
	void answer();
	void hold(bool f);
	void setAccount(SIP::Account const &account);
	SIP::Account const &account() const;

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
signals:
	void registered(bool reg);
	void unregistering();
	void call_incoming(QString const &from);
	void closed(int dir, int condition);
	void incoming_established();
	void outgoing_established();
	void dtmf_input(QString const &text);
	void state_changed(int);
};


#endif // PHONETHREAD_H
