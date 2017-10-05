#ifndef PHONETHREAD_H
#define PHONETHREAD_H

#include <QThread>
#include <re.h>
#include "baresip.h"
#include "Account.h"
#include "memory"

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
public:
	PhoneThread();
	~PhoneThread();

	bool dial(const QString &text);
	void hangup();
	void answer();
	void hold(bool f);
	void setAccount(SIP::Account const &account);

	bool isRegistered() const;

	void setVoice(VoicePtr voice);

	static void init();
	void resetVoice();
	const Voice *voice() const;
	void close();
	bool isEndOfVoice() const;
	struct ua *ua();
protected:
	void run();
signals:
	void registered(bool reg);
	void incoming(QString const &from);
	void closed();
	void incoming_established();
	void calling_established();
	void dtmf_input(QString const &text);
};


#endif // PHONETHREAD_H
