#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include "MySettings.h"

#include <QFileDialog>
#include "misc.h"

static int page_number = 0;

SettingsDialog::SettingsDialog(MainWindow *parent) :
	QDialog(parent),
	ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);
	Qt::WindowFlags flags = windowFlags();
	flags &= ~Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags);

	mainwindow_ = parent;

	loadSettings();

	QTreeWidgetItem *item;

	auto AddPage = [&](QString const &name, QWidget *page){
		item = new QTreeWidgetItem();
		item->setText(0, name);
        item->setData(0, Qt::UserRole, QVariant((qulonglong)(uintptr_t)(QWidget *)page));
		ui->treeWidget->addTopLevelItem(item);
	};
	AddPage(tr("Account"), ui->page_account);
	AddPage(tr("Audio Device"), ui->page_audio_device);

	ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(page_number));
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

namespace {

template <typename T> class GetValue {
private:
public:
	MySettings &settings;
	QString name;
	GetValue(MySettings &s, QString const &name)
		: settings(s)
		, name(name)
	{
	}
	void operator >> (T &value)
	{
		value = settings.value(name, value).template value<T>();
	}
};

template <typename T> class SetValue {
private:
public:
	MySettings &settings;
	QString name;
	SetValue(MySettings &s, QString const &name)
		: settings(s)
		, name(name)
	{
	}
	void operator << (T const &value)
	{
		settings.setValue(name, value);
	}
};

} // namespace

void SettingsDialog::loadSettings(ApplicationSettings *as)
{
	MySettings s;

	s.beginGroup("Account");
	GetValue<QString>(s, "PhoneNumber")                                >> as->account.phone_number;
	GetValue<QString>(s, "Server")                                     >> as->account.server;
	GetValue<int>(s, "Port")                                           >> as->account.port;
	GetValue<QString>(s, "ServiceDomain")                              >> as->account.service_domain;
	GetValue<QString>(s, "User")                                       >> as->account.user;
	GetValue<QString>(s, "Password")                                   >> as->account.password;
	s.endGroup();

	if (as->account.port < 1 || as->account.port > 65535) {
		as->account.port = 5060;
	}

	s.beginGroup("AudioDevice");
	GetValue<QString>(s, "Input")                                      >> as->audio_input;
	GetValue<QString>(s, "Output")                                     >> as->audio_output;
	s.endGroup();
}

void SettingsDialog::saveSettings(ApplicationSettings *as)
{
	MySettings s;

	s.beginGroup("Account");
	SetValue<QString>(s, "PhoneNumber")                                << as->account.phone_number;
	SetValue<QString>(s, "Server")                                     << as->account.server;
	SetValue<int>(s, "Port")                                           << as->account.port;
	SetValue<QString>(s, "ServiceDomain")                              << as->account.service_domain;
	SetValue<QString>(s, "User")                                       << as->account.user;
	SetValue<QString>(s, "Password")                                   << as->account.password;
	s.endGroup();

	s.beginGroup("AudioDevice");
	SetValue<QString>(s, "Input")                                      << as->audio_input;
	SetValue<QString>(s, "Output")                                     << as->audio_output;
	s.endGroup();
}

void SettingsDialog::exchange(bool save)
{
	QList<AbstractSettingForm *> forms = ui->stackedWidget->findChildren<AbstractSettingForm *>();
	for (AbstractSettingForm *form : forms) {
		form->exchange(save);
	}
}

void SettingsDialog::loadSettings()
{
	loadSettings(&set);
	exchange(false);
}

void SettingsDialog::done(int r)
{
	page_number = ui->stackedWidget->currentIndex();
	QDialog::done(r);
}

void SettingsDialog::accept()
{
	exchange(true);
	saveSettings(&set);
	done(QDialog::Accepted);
}

void SettingsDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	(void)previous;
	if (current) {
		uintptr_t p = current->data(0, Qt::UserRole).value<uintptr_t>();
		QWidget *w = reinterpret_cast<QWidget *>(p);
		Q_ASSERT(w);
		ui->stackedWidget->setCurrentWidget(w);
	}
}

