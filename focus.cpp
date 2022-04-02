#include "focus.h"

VisibleButton::VisibleButton(QString n, QWidget * parent) : QToolButton(parent)
{
	name = n;
	this->setFixedSize(30, 30);
	visibleIcon = new QIcon("icons/visible.png");
	hiddenIcon = new QIcon("icons/hidden.png");
	v = false;
	this->setIcon(*hiddenIcon);
	this->setAutoRaise(true);
	connect(this, SIGNAL(pressed()), this, SLOT(click()));
}

void VisibleButton::set(bool visible)
{
	qDebug() << "VisibleButton: Visibility set to" << visible;
	v = visible;
	if (v) this->setIcon(*visibleIcon); else this->setIcon(*hiddenIcon);
	repaint();
}

void VisibleButton::click(void)
{
	v = !v;
	qDebug() << "VisibleButton: Toggled visibility of" << name << "to" << v;
	if (v) this->setIcon(*visibleIcon); else this->setIcon(*hiddenIcon);
	repaint();
	emit(toggled(name, v));
}


FocusButton::FocusButton(QString n, QWidget * parent) : QToolButton(parent)
{
	name = n;
	this->setFixedSize(30, 30);
	this->setIcon(QIcon("icons/focus.png"));
	this->setAutoRaise(true);
	connect(this, SIGNAL(pressed()), this, SLOT(click()));
}

void FocusButton::click(void)
{
	qDebug() << "FocusButtonButton: Button of" << name << "pressed";
	emit(focusMe(name));
}
