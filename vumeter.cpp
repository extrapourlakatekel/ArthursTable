#include <QPixmap>
#include <QImage>
#include <QColor>
#include "vumeter.h"

VuMeter::VuMeter(QString l, QWidget * parent) : QWidget(parent)
{
	label = new QLabel(l, parent);
	meter = new QLabel(parent);
	minLevel = 0.0;
	maxLevel = 1.0;
	value = 0.0;
}

VuMeter::VuMeter(QIcon icon, QWidget * parent) : QWidget(parent)
{
	label = new QLabel(parent);
	label->setPixmap(icon.pixmap(16,16));
	meter = new QLabel(parent);
	minLevel = 0.0;
	maxLevel = 1.0;
	value = 0.0;
}


VuMeter::~VuMeter()
{
	

}

void VuMeter::repos(int xpos, int ypos, int width, int height)
{
	label->resize(20,height);
	meter->resize(width-20, height/2);
	label->move(xpos, ypos);
	meter->move(xpos+20,ypos+height/4);
	draw();
}

void VuMeter::setMinMax(double minL, double maxL)
{
	minLevel = minL;
	maxLevel = maxL;
}

void VuMeter::setValue(double v, bool active)
{
	act = active;
	value = (v - minLevel) / (maxLevel - minLevel); // normalize;
	draw();
}

void VuMeter::draw()
{
	QPixmap pixmap(meter->width(), meter->height());
	QImage image(meter->width(), 1, QImage::Format_RGB888);

	for (int i=0; i<meter->width(); i++) {
		double k = (double) i / (double) meter->width();
		if (act) {
			if (value > k) {
				if      (k > 0.9) image.setPixel(i, 0, QColor("red"   ).rgb());
				else if (k > 0.7) image.setPixel(i, 0, QColor("yellow").rgb());
				else              image.setPixel(i, 0, QColor("green" ).rgb());
			} else image.setPixel(i, 0, QColor("darkgrey" ).rgb());
		}
		else {
			if (value > k) {
				if      (k > 0.9) image.setPixel(i, 0, QColor("darkgrey").rgb());
				else if (k > 0.7) image.setPixel(i, 0, QColor("lightgrey").rgb());
				else              image.setPixel(i, 0, QColor("grey" ).rgb());
			} else image.setPixel(i, 0, QColor("darkgrey" ).rgb());
		}
	}
	pixmap.convertFromImage(image.scaled(meter->width(), meter->height()));
	meter->setPixmap(pixmap);
}