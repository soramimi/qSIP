#ifndef PHONEWIDGET_H
#define PHONEWIDGET_H

#include "Account.h"
#include "PhoneThread.h"
#include <QWidget>
#include <functional>

namespace Ui {
class PhoneWidget;
}

typedef std::function<void (int, QString)> handler_fn_t;

class PhoneWidget : public QWidget
{
	Q_OBJECT
private:
	struct Private;
	Private *m;
public:
	explicit PhoneWidget(QWidget *parent = 0);
	virtual ~PhoneWidget();

	void setup(const SIP::Account &account);

	bool isRegistered() const;
	void close();
	void setCallingTimeout(int sec);
	void enableAnswer(bool f);
	void setText(QString const &text);
	void setVoice(VoicePtr voice);
	void hangup();
	void call(const QString &to);
	void setOutgoingEstablishedHandler(handler_fn_t handler);
	void setClosedHandler(handler_fn_t handler);
	QString dtmftext() const;
private slots:
	void on_toolButton_clear_clicked();
	void on_toolButton_pad_0_clicked();
	void on_toolButton_pad_1_clicked();
	void on_toolButton_pad_2_clicked();
	void on_toolButton_pad_3_clicked();
	void on_toolButton_pad_4_clicked();
	void on_toolButton_pad_5_clicked();
	void on_toolButton_pad_6_clicked();
	void on_toolButton_pad_7_clicked();
	void on_toolButton_pad_8_clicked();
	void on_toolButton_pad_9_clicked();
	void on_toolButton_pad_aster_clicked();
	void on_toolButton_pad_sharp_clicked();
	void on_toolButton_hangup_clicked();
	void on_toolButton_call_clicked();
	void on_toolButton_answer_clicked();
	void on_checkBox_hold_toggled(bool checked);
	void onRegistered(bool f);
	void onCallIncoming(const QString &from);
	void onIncomingEstablished();
	void onOutgoingEstablished();
	void onClosed(int dir);
	void onDTMF(const QString &text);
	void onStateChanged(int state);
private:
	Ui::PhoneWidget *ui;
	void setRegistrationStatusText(const QString &text);
	void updateRegistrationStatus();
	void push(int n);
	void setStatusText(const QString &text);

	void updateUI();
	void restart();
protected:
	void timerEvent(QTimerEvent *event);
};

#endif // PHONEWIDGET_H
