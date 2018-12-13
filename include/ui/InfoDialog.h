#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include <QMainWindow>
#include <QPlainTextEdit>

namespace Ui {
class InfoDialog;
}

class InfoDialog : public QDialog
{
    Q_OBJECT

	public:
		explicit InfoDialog(QWidget *parent = 0);
		~InfoDialog();

		void write(QString string);

		QTextEdit* get_logger() const;

	private:
		Ui::InfoDialog *ui;
};

#endif // INFODIALOG_H
