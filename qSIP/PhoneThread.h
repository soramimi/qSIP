#ifndef PHONETHREAD_H
#define PHONETHREAD_H

#include <QThread>
#include <re.h>
#include "baresip.h"
#include "Account.h"

#ifdef bool
#undef bool
#endif

class PhoneThread : public QThread {
	Q_OBJECT
private:
	struct Private;
	Private *m;
	static void signal_handler(int sig);

	static void control_handler(void);
	void onEvent(struct ua *ua, enum ua_event ev, struct call *call, const char *prm);

	static void event_handler(struct ua *ua, enum ua_event ev, struct call *call, const char *prm, void *arg);

public:
	PhoneThread();
	~PhoneThread();

	bool dial(const QString &text);
	void hangup();
	void answer();
	void setAccount(SIP::Account const &account);
	static void init();
protected:
	void run();
signals:
	void incoming(QString const &from);
	void dtmf_input(QString const &text);
};


#endif // PHONETHREAD_H
