#include "MainWindow.h"
#include <QApplication>
#include "MySettings.h"
#include "main.h"
#include <time.h>
#include <QMessageBox>
#include <QDebug>

QString application_data_dir;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	a.setOrganizationName(ORGANIZTION_NAME);
	a.setApplicationName(APPLICATION_NAME);

	application_data_dir = makeApplicationDataDir();
	if (application_data_dir.isEmpty()) {
		QMessageBox::warning(0, qApp->applicationName(), "Preparation of data storage folder failed.");
		return 1;
	}

	MainWindow w;
	w.show();

	return a.exec();
}
