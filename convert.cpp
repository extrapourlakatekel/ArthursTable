#include "convert.h"

QByteArray toBytes(float f)
{
	QByteArray a(reinterpret_cast<const char *>(&f), sizeof(f)); // FIXME: This is machine dependent! Do this properly
	return a;
}

QByteArray toBytes(ComState f)
{
	return QByteArray(1,(char) f);
}


void fromBytes(float & f, QByteArray & a)
{
	if (a.size() < (int)sizeof(float)) {qWarning("fromBytes: Was fed less data than required to build a float"); f = 0.0;}
	else f = *reinterpret_cast<const float *>(a.mid(0, sizeof(float)).data()); // Use the first relevant bytes only FIXME: Do this in network byte order!
	a = a.mid(sizeof(float)); // And cut off the first bytes
}

void fromBytes(ComState &out, QByteArray &in)
{
	if (in.size()<1) {qWarning("fromBytes: Was fed less data than required to build a ComState"); out = COM_FULL;}
	else out = (ComState)in.at(0);
	in = in.mid(1); // And cut off the first byte
}

QByteArray toText(QByteArray v)
{
	// May contain unreadable characters so simply convert to hex
	return v.toHex(0);
}

QByteArray toText(QString v)
{
	return v.toUtf8();
}

QByteArray toText(bool v)
{
	if (v) return QByteArray("true"); else return QByteArray("false");
}

QByteArray toText(int v)
{
	QByteArray r;
	r.setNum(v);
	return r;
}

QByteArray toText(float v)
{
	QByteArray r;
	r.setNum(v);
	return r;
}

QByteArray toText(ComState v)
{
	switch (v) {
		case COM_NONE: return QByteArray("COM_NONE");
		case COM_PARTIAL: return QByteArray("COM_PARTIAL");
		case COM_FULL: return QByteArray("COM_FULL");
		default: qWarning("Control: Could not convert com enum");
	}
	return QByteArray("FULL_COM"); // If in doupt for free speech
}

QByteArray toText(QStringList v)
{
	QByteArray data("");
	if (v.size() == 0) return data;
	for (int i=0; i<v.size(); i++) {
		QString s = v.at(i);
		s.replace('/','_');  // If the original data has slashes - for whatever reason - remove them to avoid chaos!
		data.append(s);
		if (i < v.size()-1) data.append("/");
	}
	return data;
}

QByteArray toText(TrackerType v)
{
	switch (v) {
		case TRACKER_NONE: return QByteArray("NONE");
		case TRACKER_BNO055: return QByteArray("BNO055");
		default: qWarning("Convert: Could not convert TrackerType");
	}
	return QByteArray("NONE"); // If in doupt return none
}

QByteArray toText(ComEffect v)
{
	switch (v) {
		case COMEFFECT_NONE: return QByteArray("NONE");
		case COMEFFECT_MUTE: return QByteArray("MUTE");
		case COMEFFECT_FAR: return QByteArray("FAR");
		case COMEFFECT_RADIO: return QByteArray("RADIO");
		case COMEFFECT_PHONE: return QByteArray("PHONE");
		default: qWarning("Convert: Could not convert comeffect enum");
	}
	return QByteArray("NONE"); // If in doubt for free speech
}

QByteArray toText(char v)
{
	return QByteArray(1, v);
}

QByteArray toText(QHostAddress v)
{
	return v.toString().toUtf8();
}

bool fromText(bool & out, QByteArray in)
{
	out = (in == QByteArray("true"));
	return true;
}

bool fromText(int &out, QByteArray in)
{
	if (in.size()==0) return false;
	out = in.toInt();
	return true;
}

bool fromText(float &out, QByteArray in)
{
	if (in.size()==0) return false;
	out = in.toFloat();
	return true;
}

bool fromText(QByteArray &out, QByteArray in)
{
	if (in.size()==0) return false;
	out = QByteArray::fromHex(in);
	return true;
}

bool fromText(QString &out, QByteArray in)
{
	if (in.size() == 0) return false;
	out = QString(in);
	return true;
}

bool fromText(TrackerType &out, QByteArray in)
{
	if (in == "NONE") out = TRACKER_NONE; 
	else if (in == "BNO055") out = TRACKER_BNO055;
	else {qWarning("Ctrl: Error reading trackertype from file"); return false;}
	//qDebug("Ctrl: read in com state: %d", out);
	return true;
}

bool fromText(QStringList &out, QByteArray in)
// Converts a QByteArray to a QStringList using / as separators
{
	//qDebug() << "convert: Got" << in;
	out.clear();
	int index;
	while (true) {
		index = in.indexOf('/');
		if (index == -1) {
			// Last or only item
			out.append(in);
			return true;
		}
		//qDebug() << "convert: Found" << in.left(index);
		out.append(in.left(index));
		in = in.mid(index+1);
	}
}

bool fromText(ComState &out, QByteArray in)
{
	out = COM_FULL; // Default!
	if (in == "COM_NONE") out = COM_NONE; 
	else if (in == "COM_PARTIAL") out = COM_PARTIAL;
	else if (in == "COM_FULL") out = COM_FULL;
	else qWarning("Convert: Cannot convert Com State");
	//qDebug("Ctrl: read in com state: %d", out);
	return true;
}

bool fromText(char &out, QByteArray in)
{
	if (in.size() != 1) {qWarning("Convert: Cannot convert to char"); return false;}
	out = in.at(0);
	return true;
}

bool fromText(ComEffect &out, QByteArray in)
{
	if (in == "NONE") out = COMEFFECT_NONE; 
	else if (in == "MUTE") out = COMEFFECT_MUTE;
	else if (in == "FAR") out = COMEFFECT_FAR;
	else if (in == "RADIO") out = COMEFFECT_RADIO;
	else if (in == "PHONE") out = COMEFFECT_PHONE;
	else {qWarning("Convert: Cannot convert to com effect"); return false;}
	//qDebug("Convert: Read in com state: %d", out);
	return true;
}

bool fromText(QHostAddress &out, QByteArray in)
{
	out = QHostAddress(QString(in));
	return true; // Do not know how to detect whether anything went wrong - an ampty address is also a valid entry
}

QByteArray generatePacketHeader(QByteArray passphrase) 
// Sorry, packer header is actually meant to be 8 bytes long - squeeze the passphrase into a sort of an 8 byte CRC to get the header - veeery "secure"
{
	QByteArray header(PACKETHEADER);
	assert (header.length() == 8);
	while (passphrase.length() > 0) {
		for (int i=0; i<min(passphrase.length(), 8); i++) header.data()[i] = (header.data()[i] + passphrase.at(i)) & 0xFF;
		passphrase = passphrase.mid(8);
	}
	return header;
}

