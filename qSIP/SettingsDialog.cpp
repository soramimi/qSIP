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
		item->setData(0, Qt::UserRole, QVariant((uintptr_t)(QWidget *)page));
		ui->treeWidget->addTopLevelItem(item);
	};
	AddPage(tr("Account"), ui->page_account);

	ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(page_number));
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}

void SettingsDialog::loadSettings(ApplicationSettings *as)
{
	MySettings s;

//	s.beginGroup("Global");
//	s.endGroup();

	s.beginGroup("Account");
	as->account.server = s.value("Server").toString();
	as->account.user = s.value("User").toString();
	as->account.password = s.value("Password").toString();
	s.endGroup();
}

void SettingsDialog::saveSettings()
{
	MySettings s;

//	s.beginGroup("Global");
//	s.endGroup();

	s.beginGroup("Account");
	s.setValue("Server", set.account.server);
	s.setValue("User", set.account.user);
	s.setValue("Password", set.account.password);
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
	saveSettings();
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

