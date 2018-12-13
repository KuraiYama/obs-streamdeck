// InfoDialog Include
#include "include/ui/infodialog.h"
#include "ui_infodialog.h"

InfoDialog::InfoDialog(QWidget *parent) : QDialog(parent), ui(new Ui::InfoDialog)
{
    ui->setupUi(this);
	QString label = QString("Elgato Remote Control for OBS Studio - Plugin Version: %1").arg(VERSION_STR);
    setWindowTitle(label);

	show();
}

InfoDialog::~InfoDialog()
{
    delete ui;
}

void InfoDialog::write(QString str) {
	ui->logger->append(str);
}

QTextEdit* InfoDialog::get_logger() const {
	return ui->logger;
}