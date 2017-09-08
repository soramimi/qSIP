#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
private slots:
	void on_toolButton_pad_1_clicked();

	void on_toolButton_pad_2_clicked();

	void on_toolButton_pad_3_clicked();

	void on_toolButton_pad_4_clicked();

	void on_toolButton_pad_5_clicked();

	void on_toolButton_pad_6_clicked();

	void on_toolButton_pad_7_clicked();

	void on_toolButton_pad_8_clicked();

	void on_toolButton_pad_9_clicked();

	void on_toolButton_pad_0_clicked();

	void on_toolButton_pad_aster_clicked();

	void on_toolButton_pad_sharp_clicked();

	void on_toolButton_clear_clicked();

	void on_toolButton_call_clicked();

	void on_toolButton_hangup_clicked();

	void on_toolButton_answer_clicked();

	void on_action_settings_triggered();

	void on_checkBox_hold_clicked();

	void onIncoming(const QString &text);
	void onClosed();
	void onDTMF(const QString &text);

private:
	Ui::MainWindow *ui;
	void push(int n);
	//	bool dial(QString const &text);
	void reregister();
	void unregister();
};

#endif // MAINWINDOW_H
