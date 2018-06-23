#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QString>

namespace SIP {

struct Account {
	QString phone_number;
	QString server;
	int port = 5060;
	QString service_domain;
	QString user;
	QString password;

	bool operator == (Account const &r) const
	{
		return server == r.server && port == r.port && user == r.user && password == r.password;
	}
	bool operator != (Account const &r) const
	{
		return !operator == (r);
	}
};

} // namespace sip

#endif // ACCOUNT_H
