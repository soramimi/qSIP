#ifndef NETWORK_H
#define NETWORK_H

#include <QStringList>

class Network {
public:
	Network();
	static QStringList resolveHostAddress(QString const &name);
};

#endif // NETWORK_H
