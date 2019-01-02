/*
 * Plugin Includes
 */
#include "include/ui/InfoDialog.h"
#include "ui/ui_InfoDialog.h"
#include "include/services/Service.hpp"
#include "include/obs/ItemGroup.hpp"

/*
========================================================================================================
	Constructors / Destructor
========================================================================================================
*/

InfoDialog::InfoDialog(QWidget *parent) : QDialog(parent), ui(new Ui::InfoDialog)
{
    ui->setupUi(this);
	QString label = QString("Elgato Remote Control for OBS Studio - "
		"Plugin Version: %1").arg(VERSION_STR);
    setWindowTitle(label);

	connect(ui->validate, &QPushButton::clicked, this, &InfoDialog::clicked);

	show();
}

InfoDialog::~InfoDialog()
{
    delete ui;
}

/*
========================================================================================================
	Handle modification
========================================================================================================
*/

void
InfoDialog::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);
	ui->hideGroup->setChecked(Service::_obs_manager->configuration & Service::_obs_manager->HIDE_GROUP);
	ui->directTransition->setChecked(
		Service::_obs_manager->configuration & Service::_obs_manager->DIRECT_TRANSITION
	);
	ui->doubleTap->setChecked(
		Service::_obs_manager->configuration & Service::_obs_manager->DOUBLE_TRANSITION
	);
	ui->showFilters->setChecked(
		Service::_obs_manager->configuration & Service::_obs_manager->LIST_FILTER
	);

}

void
InfoDialog::accept() {
	Service::_obs_manager->configuration =
		((ui->hideGroup->isChecked() * 0x0F) & Service::_obs_manager->HIDE_GROUP) |
		((ui->directTransition->isChecked() * 0x0F) & Service::_obs_manager->DIRECT_TRANSITION) |
		(
			((ui->doubleTap->isChecked() && !ui->directTransition->isChecked()) * 0x0F) &
			Service::_obs_manager->DOUBLE_TRANSITION
		) |
		((ui->showFilters->isChecked() * 0x0F) & Service::_obs_manager->LIST_FILTER);

	ItemGroup::_toggle_subitems =
		(Service::_obs_manager->configuration & Service::_obs_manager->HIDE_GROUP);

	QDialog::accept();
}

void
InfoDialog::clicked(bool checked) {
	accept();
}

/*
========================================================================================================
	Log Handling
========================================================================================================
*/

void
InfoDialog::write(QString str) {
	ui->_logger->append(str);
}

QTextEdit*
InfoDialog::logger() const {
	return ui->_logger;
}
