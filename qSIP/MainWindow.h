#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Account.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
private:
	struct Private;
	Private *m;
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	QString statusText() const;
	void setStatusText(const QString &text);
	void reregister(const SIP::Account &a);
	void reregister();
private slots:
	void on_action_settings_triggered();
	void on_action_test_triggered();
private:
	Ui::MainWindow *ui;
	void close();
};

#endif // MAINWINDOW_H
