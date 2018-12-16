/*
 * Plugin Includes
 */
#include "include/ui/InfoDialog.h"
#include "ui/ui_InfoDialog.h"

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

	show();
}

InfoDialog::~InfoDialog()
{
    delete ui;
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
