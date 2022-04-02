#include <QWidget>
#include <assert.h>
//#include <unistd.h>
#include "headtracker.h"

// Sure we're mixing GUI and control here, but QSerial doesn't seem to work in threads anyway...

	
Headtracker::Headtracker(QWidget * parent) : QWidget(parent)
{
	this->resize(200,50);
	this->setAutoFillBackground(true);

	headtrackerFrame = new QGroupBox("Headtracker", this);
	headtrackerFrame->resize(190,45);
	headtrackerFrame->move(5,5);

	zeroButton = new QToolButton(headtrackerFrame);
	zeroButton->setCheckable(false);
	zeroButton->setFocusPolicy(Qt::NoFocus);
	zeroButton->setIcon(QIcon("icons/zero.png"));
	zeroButton->resize(20,20);
	zeroButton->move(165,20);
	connect(zeroButton, SIGNAL(clicked()), this, SLOT(zero()));
	heading = new QLabel(headtrackerFrame);
	heading->move(50,20);
	heading->resize(50,20);
	heading->setText("none");
	heading->setAlignment(Qt::AlignRight);
	
	serial = new QSerialPort(ctrl->headTrackerDevice.get());

	connect(ctrl, SIGNAL(initHeadtracker()), this, SLOT(init()));
	
	timer = new QTimer;
	timer->setInterval(30);
	timer->setSingleShot(false);
	connect(timer, SIGNAL(timeout()), this, SLOT(heartbeat()));
	offset = 0.0;
	init();
}

Headtracker::~Headtracker()
{
	qDebug("Headtracker: Closing");
	if (type != TRACKER_NONE) {
		serial->clear();
		serial->close();
	}
	delete serial;
}

void Headtracker::heartbeat()
{
	char const *statename[] = {"RESET", "ZERO", "WAIT_1", "WAIT_2", "WAIT_3", "INIT_1", "INIT_2", "INIT_3", "INIT_4", "INIT_5", "CAL_1", "CAL_2", "CAL_3", "READY", "ERROR"};

	// QSerialPort does not like to be run in another thread, so we'll do it here
	float h;
	if (type == TRACKER_NONE) heading->setText("NONE");
	else {
		lastraw = getHeading();
		h = lastraw - offset;
		while (h > 180.0) h = h - 360;
		while (h < -180.0) h = h + 360;
		ctrl->heading.set(h);
		if (state == READY) heading->setText(QString::number(h, 'f', 1)+"°");
		else heading->setText(statename[state]);
	}
}

void Headtracker::init()
{
	qDebug() << "Headtracker: Initializing";
	heading->setText("none");
	timer->stop();
	state = RESET;
	type = ctrl->headTrackerType.get();
	serial->close();
	if (type == TRACKER_NONE) return; // No tracker, no initialisation!
	// FIXME: Clean this up when another tracker is implemented
	serial->setBaudRate(QSerialPort::Baud115200);
	serial->setDataBits(QSerialPort::Data8);
	serial->setParity(QSerialPort::NoParity);
	serial->setFlowControl(QSerialPort::NoFlowControl);
	if (!serial->open(QIODevice::ReadWrite)) {
		type = TRACKER_NONE; 
		qWarning() << "Headtracker: Could not open serial port for tracker" << ctrl->headTrackerDevice.get();
		return;
		}
	else serial->clear();
	// When successful, start timer
	timer->start();
	return;
}

float Headtracker::getHeading()
{
	// This routine is called every run of the streammanager
	// It descides which tracker type is used and calls the proper function
	switch (type) {
		case TRACKER_NONE: return 0.0;
		case TRACKER_BNO055: return bno055_getHeading();
	}
	qWarning("Headtracker: Unknown tracker requested!");
	return 0.0;
}

void Headtracker::zero()
{
	qDebug("Headtracker: Zeriong");
	// Reset the heading of the tracker to zero
	offset = lastraw;
}

void Headtracker::close()
{
}

#define WRITE (char)0x00
#define READ (char)0x01

void Headtracker::bno055_WriteByte(uint8_t reg, uint8_t data)
{
	QByteArray d;
	d += (char)0xAA; d += WRITE; d += (char)reg; d += (char)1; d += (char) data;
	if (serial->write(d) < 5) qWarning("Headtracker: Could not write to tracker");
}

int Headtracker::bno055_CheckAck()
{
	QByteArray d = serial->read(2);
	qDebug() << "Headtracker: CheckAck read" << d.toHex();
	if (d.length() < 2) return 0xFF; // Not enough characters received
	if (d.at(0) != (char) 0xEE) return 0xFE; // Wrong response header
	return d.at(1); // Native sensor error code
}

void Headtracker::bno055_RequestData(uint8_t reg, int length)
{
	assert (length >=0 && length <= 128);
	QByteArray d;
	d.clear();
	d += (char)0xAA; d += READ; d += (char)reg; d += (char)length;
	if (serial->write(d) < 4) qWarning("Headtracker: Could not write to tracker");
}

int Headtracker::bno055_ReadByte(uint8_t &data)
{
	QByteArray d = serial->read(3);
	if (d.length() < 3) return 0xFF; // Not enough characters received
	if (d.at(0) != (char) 0xBB) return 0xFE; // Wrong response header
	if (d.at(1) != (char) 0x01) return 0xFD; // Wrong response length
	data = (uint8_t)d.at(2); // Native sensor data
	return 0;
}

int Headtracker::bno055_ReadWord(uint16_t &data)
{
	QByteArray d = serial->read(4);
	data = 0;
	if (d.length() < 4) return 0xFF; // Not enough characters received
	if (d.at(0) == (char) 0xEE) return d.at(1); // Error
	if (d.at(0) != (char) 0xBB) return 0xFD; // Wrong response header
	if (d.at(1) != (char) 0x02) return 0xF0; // Wrong response length
	data = (unsigned char)d.at(3) * 256 + (unsigned char)d.at(2); // Native sensor data
	return 1;	
}

#define NDOF 0b00001100
#define OPR_MODE 0x3D
#define PWR_MODE 0x3E
#define SYS_TRIGGER 0x3F
#define HEADING 0x1A

float Headtracker::bno055_getHeading()
{
	QByteArray out;
	QByteArray in;
	static float heading;
	uint16_t data;
	int err;
	//qDebug() << "Tracker...";
	switch (state) {
		case RESET:
			serial->clear();
			//qDebug() << "Tracker RESET";
			bno055_WriteByte(SYS_TRIGGER, 0x20); // Enable extrnal crystal
			state = INIT_1;
			return 0.0;
		case INIT_1:
			//qDebug() << "Tracker INIT_1";
			bno055_WriteByte(SYS_TRIGGER, 0x80); // Reset
			state = WAIT_1;
			wait = 50;
			return 0.0;
		case WAIT_1:
			//qDebug() << "WAIT_1";
			wait--;
			if (wait == 0) {
				state = INIT_2;
				serial->clear();
			}
			return 0.0;
		case WAIT_2:
			//qDebug() << "WAIT_2";
			wait--;
			if (wait == 0) {
				state = INIT_2;
				serial->clear();
			}
			return 0.0;
		case INIT_2:
			//qDebug() << "Tracker INIT_2";
			bno055_WriteByte(PWR_MODE, 0x00);
			state = INIT_3;
			return 0.0;
		case INIT_3:
			//qDebug() << "Tracker INIT_3";
			err = bno055_CheckAck();
			if (err != 1) {qWarning("Headtracker: Tracker returned error %d during initialisation!", err); state = ERROR; return err;}
			bno055_WriteByte(OPR_MODE, NDOF);
			state = INIT_4;
			return 0.0;
		case INIT_4:
			qDebug() << "Tracker INIT_4";
			err = bno055_CheckAck();
			if (err != 1) {qWarning("Headtracker: Tracker returned error %d during initialisation!", err); state = ERROR; return err;}
			bno055_RequestData(HEADING, 2);
			state = READY;
			return 0.0;
		case READY:
			//qDebug() << "Tracker READY";
			err = bno055_ReadWord(data);
			if (err == 1) heading = (float)(data)/16.0; //else qWarning() << "Tracker reports error" << err;
			bno055_RequestData(HEADING, 2); // Request data for next loop
			//qDebug("Heading: %3.2f", heading);
			return heading;
		case ZERO:
			err = bno055_ReadWord(data);
			bno055_WriteByte(OPR_MODE, NDOF);
			state = INIT_4;
			return 0.0;
		case ERROR:
			state = RESET;
			return 0.0;
	}
	return 0.0;
}

