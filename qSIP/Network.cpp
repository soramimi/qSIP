#include "Network.h"

#ifdef _WIN32
#include "WinSock2.h"
#else
#include "netdb.h"
#endif

#include <QString>
#include <QStringList>
#include <stdint.h>

Network::Network()
{

}

QStringList Network::resolveHostAddress(const QString &name)
{
	QStringList list;

	QByteArray ba = name.toLatin1();
	char const *nameptr = ba;

	struct hostent *host;
	uint32_t addr = inet_addr(nameptr);
	if (addr == INADDR_NONE) {
		host = gethostbyname(nameptr);
	} else {
		host = gethostbyaddr((const char *)&addr , 4 , AF_INET);
	}
	if (host) {
		for (int i = 0; host->h_addr_list[i]; i++) {
			int a = *(uint8_t *)(host->h_addr_list[i] + 0);
			int b = *(uint8_t *)(host->h_addr_list[i] + 1);
			int c = *(uint8_t *)(host->h_addr_list[i] + 2);
			int d = *(uint8_t *)(host->h_addr_list[i] + 3);
			QString s = QString().sprintf("%u.%u.%u.%u", a, b, c, d);
			list.append(s);
		}
	}
	return list;
}

