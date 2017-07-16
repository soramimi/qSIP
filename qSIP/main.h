#ifndef MAIN_H
#define MAIN_H

#include "Account.h"

#include <QColor>
#include <QString>

#define ORGANIZTION_NAME "soramimi.jp"
#define APPLICATION_NAME "qSIP"

extern QString application_data_dir;

struct ApplicationSettings {
	SIP::Account account;
};

#endif // MAIN_H
