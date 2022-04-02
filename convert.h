#ifndef CONVERT_H
#define CONVERT_H
#include <assert.h>
#include <endian.h>
#include <QByteArray>
#include <QStringList>
#include <QHostAddress>
#include "types.h"
#include "tools.h"

template <class T>
QByteArray toNBO(T in) 
{
	QByteArray out = QByteArray((const char *) &in, sizeof(T));
	if (__BYTE_ORDER == __LITTLE_ENDIAN) {
		// We have to resort before sending it over the big bad internet
		for (uint i=0; i<sizeof(T)/2; i++) {
			char temp = out.data()[i];
			out.data()[i] = out.data()[sizeof(T)-1-i];
			out.data()[sizeof(T)-1-i] = temp;
		}
	}
	return out;
}

template <class T>
bool fromNBO(QByteArray * in, T * out)
// Converts the first sizeof(T) bytes of in to the desired data type und cuts them out of the QByteArray in
{
	if ((unsigned)in->size() < sizeof(T)) {qWarning("fromNBO: Not enough bytes to fill desired type!"); return false;}
	if (__BYTE_ORDER == __LITTLE_ENDIAN) {
		// We have to resort when we've got it from the big bad internet
		for (uint i=0; i<sizeof(T)/2; i++) {
			char temp = in->data()[i];
			in->data()[i] = in->data()[sizeof(T)-1-i];
			in->data()[sizeof(T)-1-i] = temp;
		}
	}
	memcpy((void *) out, (void *) in->data(), sizeof(T));
	(*in) = in->mid(sizeof(T));
	return true;
}

QByteArray toBytes(float);
QByteArray toBytes(ComState);

void fromBytes(float &out, QByteArray &in);
void fromBytes(ComState &out, QByteArray &in);

QByteArray toText(QByteArray v);
QByteArray toText(QString v);
QByteArray toText(bool v);
QByteArray toText(int v);
QByteArray toText(float v);
QByteArray toText(ComState v);
QByteArray toText(QStringList v);
QByteArray toText(TrackerType type);
QByteArray toText(ComEffect v);
QByteArray toText(char v);
QByteArray toText(uint8_t v);
QByteArray toText(QHostAddress v);

bool fromText(QByteArray &out, QByteArray in);
bool fromText(bool &out, QByteArray in);
bool fromText(float &out, QByteArray in);
bool fromText(int &out, QByteArray in);
bool fromText(ComState &out, QByteArray in);
bool fromText(QString &out, QByteArray in);
bool fromText(QStringList &out, QByteArray in);
bool fromText(TrackerType &out, QByteArray in);
bool fromText(ComEffect &out, QByteArray in);
bool fromText(char &out, QByteArray in);
bool fromText(QHostAddress &out, QByteArray in);

QByteArray generatePacketHeader(QByteArray passphrase);



#endif