#ifndef HEADTRACKER_H
#define HEADTRACKER_H

#include <QSerialPort>
#include <QGroupBox>
#include <QToolButton>
#include <QTimer>
#include <QLabel>
#include "control.h"
#include "types.h"


class Headtracker : public QWidget
{
	Q_OBJECT
public: 
	Headtracker(QWidget * parent = NULL);
	~Headtracker();
	void close();
	float getHeading();
private:
	QGroupBox * headtrackerFrame; 
	QToolButton * zeroButton;
	QIcon  * zeroIcon;
	QTimer * timer;
	QLabel * heading;
	enum {RESET, ZERO, WAIT_1, WAIT_2, WAIT_3, INIT_1, INIT_2, INIT_3, INIT_4, INIT_5, CAL_1, CAL_2, CAL_3, READY, ERROR};
	void bno055_WriteByte(uint8_t reg, uint8_t data);
	int  bno055_CheckAck();
	void bno055_RequestData(uint8_t reg, int length);
	int  bno055_ReadByte(uint8_t &data);
	int  bno055_ReadWord(uint16_t &data);
	float bno055_getHeading();
	bool bno055_zero();
	int bno055_GetAnswer(char * data, int maxlength);
	QSerialPort * serial;
	int type;
	int state;
	int wait;
	float lastraw, offset;
public slots:
	void init();
private slots:
	void heartbeat();
	void zero();
};

#endif
