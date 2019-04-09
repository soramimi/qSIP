#include "Network.h"

#ifdef _WIN32
#include "WinSock2.h"
#else
#include <netdb.h>
#include <netinet/in.h>
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

#if 0
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
			QString s = QString::asprintf("%u.%u.%u.%u", a, b, c, d);
			list.append(s);
		}
	}
#else
	struct addrinfo hints = {};
	struct addrinfo *res = nullptr;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	getaddrinfo(nameptr, nullptr, &hints, &res);
	if (res && res->ai_family == AF_INET) {
		in_addr_t addr = ntohl(reinterpret_cast<struct sockaddr_in *>(res->ai_addr)->sin_addr.s_addr);
		freeaddrinfo(res);
		uint8_t a = (uint8_t)(addr >> 24);
		uint8_t b = (uint8_t)(addr >> 16);
		uint8_t c = (uint8_t)(addr >> 8);
		uint8_t d = (uint8_t)(addr >> 0);
		QString s = QString::asprintf("%u.%u.%u.%u", a, b, c, d);
		list.append(s);
	}
#endif
	return list;
}

