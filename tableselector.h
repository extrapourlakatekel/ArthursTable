#ifndef TABLESELECTOR_H
#define TABLESELECTOR_H
#include <QWidget>
#include <QDir>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCloseEvent>
#include <QDialog>
#include "control.h"


class TableSelector : public QDialog
{
	Q_OBJECT
public:
	TableSelector(QWidget * parent = NULL);
private:
	QStringList tables;
	QComboBox * existingTables;
	QPushButton * newButton, * okButton;
	QLineEdit * tableNameEdit;
	QLabel * chooseLabel, * newLabel;
public slots:
	void closeEvent(QCloseEvent * event);
private slots:
	void open();
	void create();
signals:
	void quit();
	void selected();
};



#endif
