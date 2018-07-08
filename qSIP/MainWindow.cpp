#include "MainWindow.h"
#include "Network.h"
#include "ui_MainWindow.h"

#include "PhoneThread.h"
#include "SettingsDialog.h"
#include "StatusLabel.h"
#include "PhoneWidget.h"
#include <QDateTime>
#include <QDebug>
#include "memory"

enum {
	PAD_ASTER = '*',
	PAD_SHARP = '#',
};

struct MainWindow::Private {
	StatusLabel *status_label;
	ApplicationSettings appsettings;
//	std::shared_ptr<PhoneThread> phone;
//	QTime registration_time;
//	int registration_seconds = 0;
};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m(new Private)
{
	ui->setupUi(this);
	m->status_label = new StatusLabel;
	ui->statusBar->addWidget(m->status_label);

	SettingsDialog::loadSettings(&m->appsettings);

	reregister();
}

MainWindow::~MainWindow()
{
	close();
	delete m;
	delete ui;
}

QString MainWindow::statusText() const
{
	return m->status_label->text();
}

void MainWindow::setStatusText(QString const &text)
{
	m->status_label->setText(text);
}

void MainWindow::close()
{
	ui->widget_phone->close();
}

#include <QKeyEvent>
void MainWindow::keyPressEvent(QKeyEvent *event)
{
	switch (event->key()) {
	case Qt::Key_T:
		if (QApplication::queryKeyboardModifiers() & Qt::ControlModifier) {
			test();
		}
		return;
	}
	return QMainWindow::keyPressEvent(event);
}

void MainWindow::reregister(SIP::Account const &a)
{
	ui->widget_phone->setup(a);
}

void MainWindow::reregister()
{
	reregister(m->appsettings.account);
}

void MainWindow::on_action_settings_triggered()
{
	SettingsDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted) {
		ApplicationSettings const &newsettings = dlg.settings();
		bool eq = m->appsettings.account == newsettings.account;
		m->appsettings = newsettings;
//		if (!eq) {
			reregister();
//		}
	}
}

void MainWindow::test()
{
	reregister();

	qDebug() << Network::resolveHostAddress(m->appsettings.account.server);
}
