#ifndef FOCUS_H
#define FOCUS_H

#include <QToolButton>
#include <QString>
#include <QPixmap>
#include "control.h"
#include "bulktransfer.h"

class FocusButton : public QToolButton {
	Q_OBJECT
public:
	FocusButton(QString, QWidget * parent=NULL);
private:
	QString name;
	int type;
private slots:
	void click();
signals:
	void focusMe(QString);
};

class VisibleButton : public QToolButton {
	Q_OBJECT
public:
	VisibleButton(QString, QWidget * parent=NULL);
	void set(bool visible);
private:
	QIcon * visibleIcon, * hiddenIcon;
	QString name;
	bool v;
private slots:
	void click();
signals:
	void toggled(QString, bool);
};
	

#endif