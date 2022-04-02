#ifndef BUFFER_H
#define BUFFER_H

#include <QByteArray>
#include "config.h"
#include "control.h"

class SendBuf
{
public:
	SendBuf();
	void clear(QByteArray packetheader);
	QByteArray get();
	bool append(uint8_t type, uint16_t count, QByteArray data);
	bool append(QByteArray data);
	int freeSpace();
	int size();
private:
	QByteArray d;
};

class StreamFifoArray
{
public:
	StreamFifoArray(int p, int dp);
	void append(int p, QByteArray frame);
	QByteArray get(int p);
	int fillLevel(int p);
	void reset(int p);
	bool isPrefilling(int p);
private:
	int depth, players;
	int * fillcontrol;
	bool * prefill;
	uint16_t * expectedserial;
	int * readpointer, * writepointer;
	QByteArray **d;	
};

#endif