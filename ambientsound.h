#ifndef AMBIENTSOUND_H
#define AMBIENTSOUND_H
#include <QComboBox>
#include <QGroupBox>
#include <QToolButton>
#include <QWidget>
#include "control.h"
#include "config.h"

class AmbientSound : public QWidget
{
Q_OBJECT
public:
	AmbientSound(QWidget * parent);
};


#endif