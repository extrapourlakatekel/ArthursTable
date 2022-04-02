#include "buffer.h"
#include "assert.h"

SendBuf::SendBuf()
{
	d.clear();
}
	
void SendBuf::clear(QByteArray packetheader)
{
	// Clear means: Start with a clear array with the validation code only
	d.clear();
	d += packetheader;
}

QByteArray SendBuf::get()
{
	return d;
}

bool SendBuf::append(uint8_t type, uint16_t packetnr, QByteArray data)
{
	if (d.size() + data.size() + 5 > MAXPACKETSIZE) {
		qWarning("SendBuf: Exceeded maximum packet size while storing %d bytes in buffer already %d bytes long!", d.size(), data.size());
		return false; // Packet would become to large!
	}
	//qDebug() << "Actual packet length" << d.size();
	// First Byte: Packet type.
	d += (char) type;
	// Second and third byte: Packet length
	d += (char)((uint16_t)data.length() >> 8);
	d += (char)((uint16_t)data.length() & 0xFF);
	// forth and fifth byte: Packet counter
	d += (char) (packetnr >> 8);
	d += (char) (packetnr & 0XFF);
	// Finally add the data
	d += data;
	//qDebug() << "Added packet with length" << data.length() << "and packet nr" << packetnr << "to send buffer. Has now" << d.length() << "bytes.";
	return true;
}

bool SendBuf::append(QByteArray data)
{
	if (d.size() + data.size() > MAXPACKETSIZE) {
		qWarning("SendBuf: Exceeded maximum packet size while storing %d bytes in buffer already %d bytes long!", data.size(), d.size());
		return false; // Packet would become to large!
	}
	//qDebug() << "Actual packet length" << d.size();
	d += data;
	//qDebug() << "Added packet with length" << data.length() << "to send buffer. Has now" << d.length() << "bytes.";
	return true;
	
}

int SendBuf::freeSpace()
{
	return MAXPACKETSIZE-d.size();
}

int SendBuf::size()
{
	return d.size();
}

StreamFifoArray::StreamFifoArray(int p, int dp)
{
	depth = dp;
	players = p;
	d = (QByteArray**)malloc(sizeof(QByteArray*) * p);
	prefill = new bool[players];
	fillcontrol = new int[players];
	readpointer = new int[players];
	writepointer = new int[players];
	expectedserial = new uint16_t[players];
	for (int i=0; i<players; i++) {
		d[i] = new QByteArray[depth];
		prefill[i] = true;
		fillcontrol[i] = 0;
		readpointer[i] = 0;
		writepointer[i] = 0;
		expectedserial[i] = 0;
	}
}

void StreamFifoArray::append(int p, QByteArray segment)
{
	assert (p>=0 && p<players);
	uint16_t serial;
	assert(segment.size() >= 2);
	serial = (unsigned char)segment.at(0) * 256 + (unsigned char)segment.at(1); // Serial number of the packet
	int age = (int)expectedserial[p] - (int)serial;
	if (age < -32768) age +=65536;
	if (age > 32767) age -= 65536;
	segment.remove(0,2); // Remove the serial number of the packet - data will be left
	//qDebug() << "Received frame from player" << p << "with serial" << serial << "meaning age" << age << "and length" << segment.length() << "for fifo.";
	//qDebug() << "Actual fifo filling for player" << p << "is" << fillLevel(p) << "Prefilling is " << prefill[p];
	if (age <= 0) {
		if (-age >= depth - fillLevel(p)) {
			// We've got a very new packet and we're missing many old ones! Something must have gone woefully wrong. Reset buffer!
			//qWarning() << "StreamFifoArray: Missed too many packets - resetting buffer";
			reset(p);
		}
		// When age is negative, seems that we skipped some packages - maybe they come in later...
		for (int i=0; i < (-age); i++) {
			d[p][writepointer[p]].clear(); // Clear the expected packets between the received one and the last one
			writepointer[p] = (writepointer[p] + 1 ) % depth; // Increase pointer with wrapping
			if (writepointer[p] == readpointer[p]) {
				//qWarning() << "StreamFifoArray: Bufferoverrun while filling StreamFifo - resetting buffer";
				reset(p);
			}
		}
		// When the fillcontroller indicates a constantly too full buffer, skip one packet
		if (fillcontrol[p] > LEVELBALANCE) {
			fillcontrol[p] = 0; 
			qWarning() << "StreamFifoArray: Skipping one stream packet for sync, player" << p;
		}
		else {
			d[p][writepointer[p]] = segment; 
			writepointer[p] = (writepointer[p] + 1 ) % depth; // Increase pointer with wrapping
			if (writepointer[p] == readpointer[p]) {
				//qWarning() << "StreamFifoArray: Bufferoverrun while filling StreamFifo - resetting buffer";
				reset(p);
			}
		}
		expectedserial[p] = serial + 1;
	} else {
		// Age is positive - we got one delayed package
		if (age < fillLevel(p)) d[p][(writepointer[p]-age+depth)%depth] = segment; // When the package is not too old we can keep it
		else {
			reset(p); // Try to resync
			expectedserial[p] = serial;
		}
	}
	if (fillLevel(p) >= depth / 2) prefill[p] = false; // When buffer is half full, prefill is finished
}

QByteArray StreamFifoArray::get(int p)
{
	assert (p>=0 && p<players);
	QByteArray r;
	// Check whether we are in the prefill phase and return an empty packet if true
	if (prefill[p]) return QByteArray();
	// When the buffer is empty, return an ampty packet and restart prefilling
	if (readpointer[p] == writepointer[p]) {
		prefill[p] = true;
		return QByteArray();
	}
	if (abs(fillLevel(p) - depth / 2) > 1) fillcontrol[p] += fillLevel(p) - depth / 2; // Fill controller with a little deadband

	if (fillcontrol[p] < -LEVELBALANCE) {
		// When the buffer is constantly too empty skip the delivery of one packet to restore balance
		fillcontrol[p] = 0;
		qWarning() << "StreamFifoArray: Doubling one stream packet for sync, player" << p;
		return QByteArray();
	}
	r = d[p][readpointer[p]];
	readpointer[p]++;
	if (readpointer[p] >= depth) readpointer[p] = 0;
	return r;
}

int StreamFifoArray::fillLevel(int p)
{
	assert (p>=0 && p<players);
	return (writepointer[p] - readpointer[p] + depth) % depth;
}

void StreamFifoArray::reset(int p)
{
	assert (p>=0 && p<players);
	prefill[p] = true; // Buffer overrun - restart with new filling!
	writepointer[p] = 0;
	readpointer[p] = 0;
}

bool StreamFifoArray::isPrefilling(int p)
{
	return prefill[p];
}