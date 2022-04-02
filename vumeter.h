#ifndef VUMETER_H
#define VUMETER_H
#include <QLabel>
#include <QIcon>

class VuMeter : public QWidget
{
	Q_OBJECT
public:
	VuMeter(QString l, QWidget * parent = NULL);
	VuMeter(QIcon icon, QWidget * parent = NULL);
	~VuMeter();
	void repos(int xpos, int ypos, int width, int height);
	void setMinMax(double minLevel, double maxLevel);
	void setValue(double value, bool active = true);
	void draw();
private:
	QLabel * label, * meter;
	double minLevel, maxLevel;
	double value;
	bool act;
	
};

#endif