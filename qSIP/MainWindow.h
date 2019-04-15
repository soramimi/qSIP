#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Account.h"

class ApplicationSettings;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
private:
	struct Private;
	Private *m;
	void test();
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	QString statusText() const;
	void setStatusText(const QString &text);
	void reregister(const ApplicationSettings &a);
	void reregister();
private slots:
	void on_action_settings_triggered();
private:
	Ui::MainWindow *ui;
	void close();

	// QWidget interface
protected:
	void keyPressEvent(QKeyEvent *event);
};

#endif // MAINWINDOW_H
