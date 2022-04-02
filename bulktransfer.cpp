#include "bulktransfer.h"
#include <assert.h>

TxBulk::TxBulk(char id) : QObject()
{
	myid = id;
	status = EMPTY;
	next = NULL;
	previous = NULL;
	sniplets = 0;
}

bool TxBulk::set(Type type, QByteArray bulk, int transferNr)
{
	lock.lockForWrite();
	nr = transferNr;
	lastp = 0;
	if (status == WAIT) {
		if (timer.hasExpired(TXBLANKTIME)) status = EMPTY; 
		else {lock.unlock(); return false;}
	}
	if (status == EMPTY) {
		status = FILL;
		this->type = type;
		uint32_t len = bulk.size() + 1; // One bytes will be used for the bulk header: service type (slot) & 3 bytes for number of sniplets
		sniplets = (len + (SNIPLETSIZE - 8 -1)) / (SNIPLETSIZE - 8); // Each sniplet has an own header too: one byte for bay ID, one byte for length, 3 byte for sniplet nr., 3 bytes for total number of sniplets Maaan, what a waste of bandwidth!
		assert(sniplets < (1<<24));
		assert(sniplets > 0);
		bulk.insert(0, type); // Insert the bulk header
		//qDebug("Will need %d sniplet(s) to store the message", sniplets);
		assert (next == NULL);
		assert (previous == NULL);
		next = new int32_t[sniplets]; // Acquire memory for the chained list 
		previous = new int32_t[sniplets]; // Acquire memory for the chained list 
		data.clear();
		// Cut into sniplets and glue them to one chunk for fast and easy access.
		// Remark: Of course this could be processed during readout, but the readout-process is the streammanager and it is always short on CPU
		QByteArray sniplet;
		for (int i=0; i<sniplets; i++) {
			sniplet = bulk.mid(i * (SNIPLETSIZE - 8), SNIPLETSIZE - 8); // Get the segment of the data
			// Add a sniplet header
			data += myid;
			data += char(uint32_t(sniplet.size()));
			data += char(uint32_t(i) >> 16);
			data += char((uint32_t(i) >> 8) & 0xFF);
			data += char(uint32_t(i) & 0xFF);
			data += char(sniplets >> 16);
			data += char((sniplets >> 8) & 0xFF);
			data += char(sniplets & 0xFF);
			data += sniplet;
 			if (i>0) previous[i] = i - 1;
			if (i<sniplets - 1) next[i] = i + 1;
		}
		previous[0] = -1;
		next[sniplets-1] = -1;
		first = 0;
		actual = 0;
		stopat = -1;
		length = sniplets;
		//qDebug() << "Bulk now looks like:" << data.toHex();
		status = FULL;
		lock.unlock();
		return true;
	} 
	lock.unlock();
	return false; // Bulk cannot be written - already in use!
}

bool TxBulk::get(QByteArray & sniplet)
{
//	qDebug("TxBulk: Asked for sniplet");
	lock.lockForWrite();
	if (status != FULL) {lock.unlock(); return false;} // Sorry,this one is empty or is beeing filled this moment
	assert (actual != -1);
	assert (actual < length);
	if (actual == stopat) {lock.unlock(); return false;} // We don't have permission to send the same data again
	if (stopat == -1) stopat = actual; // When we reach this entry again, don't send it untill we are allowed to resend!
	sniplet = data.mid(actual * SNIPLETSIZE, SNIPLETSIZE); // Find the proper segment
	assert(next != NULL);
	actual = next[actual];
	if (actual == -1) actual = first; // This is the end - start new
	lock.unlock();
	//qDebug() << "Sending sniplet" << sniplet.toHex();
	return true;	
}

void TxBulk::ack(int s)
{	
	// One sniplet got acknowledged. That means we have to cut it out of the list...
	lock.lockForWrite();
	assert (s >= 0);
	if (s >= length || status != FULL) {lock.unlock(); qWarning("TXBulk: Nonexistent sniplet has been acknowledged!"); return;} // Got an acknowledge for nonexisting sniplet - strange, but don't risk a segfault for this.
	if (previous[s] == -1 && next[s] == -1 && s != first) {lock.unlock(); return;} // Got an acknowledge for the second time! Ignore!
	if (previous[s] != -1) next[previous[s]] = next[s];
	if (next[s] != -1) previous[next[s]] = previous[s];
	if (s == first) first = next[s];
	sniplets--;
	if (first == -1) {
		//qDebug("Last sniplet of bay acknowledged!");
		// We're through! Finally! Nothing more to send!
		assert (next != NULL);
		assert(previous != NULL);
		delete[] next;
		next = NULL;
		delete[] previous;
		previous = NULL;
		status = WAIT; // Set the bulk into quarantine to avoid confusion with delayed packets
		sniplets = 0;
		length = 0;
		if (nr != -1) emit(ctrl->fileTransferComplete(nr));
		timer.start(); 
	}
	else {
		//qDebug("TxBulk: Acknowledged sniplet #%d", s);
		if (s == actual) {
			actual = next[s]; // It may happen that we will process the one next to that has been ack'ed now.
			if (actual == -1) actual = first;
		}
		assert(s<length);
		assert(next != NULL);
		next[s] = -1; 
		previous[s] = -1; // Mark it as nonexistent
		if (nr != -1) { // When we have no specific number nobody is interrested in our status
			int newp = 100l - 100l*sniplets/length;
			if (newp != lastp) {lastp = newp; emit(ctrl->fileTransferProgress(nr, newp));} // Only emit when percentage counter changed to avoid useless signals
		}
	}
	lock.unlock();
	return;
}

void TxBulk::clear()
{
	//qDebug("TxBulk: cleared bulk #%d", (int)myid);
	lock.lockForWrite();
	if (status != EMPTY) {
		status = EMPTY; 
		delete[] next;
		next = NULL;
		delete[] previous;
		previous = NULL;
		sniplets = 0;
	}
	lock.unlock();
}

void TxBulk::allowResend()
{
	// There is IMHO no locking required...
	stopat = -1;
}

Status TxBulk::getStatus()
{
	Status s;
	lock.lockForRead();
	s = status;
	if (s == WAIT && timer.hasExpired(TXBLANKTIME)) s = EMPTY; 
	lock.unlock();
	return s;
}

float TxBulk::getFilling()
{
	uint32_t filling;
	lock.lockForRead();
	filling = sniplets;
	lock.unlock();
	if (filling == 0) return 0.0;
	return (float)filling/(float)length;
}

Type TxBulk::getType()
{
	Type t;
	lock.lockForRead();
	t = type;
	if (status != FULL) t = UNKNOWN;
	lock.unlock();
	return t;
}
	
RxBulk::RxBulk()
{
	gotit = QByteArray("");
	status = EMPTY;
	space = 0;
	toreceive = 0;
	data = QByteArray("");
}

bool RxBulk::set(QByteArray sniplet)
{
	// RxBulk does not need locking, since only one task writes data to it. The readout is done with signals.
	assert(sniplet.size()>8); // Must contain header plus at least one byte of data
	uint32_t size = sniplet.at(1);
	//qDebug("Sorting in sniplet with reported size %d and total size %d", size, sniplet.size());
//	char id = sniplet.at(0);
	int32_t pos = (uint32_t((uint8_t)sniplet.at(2)) << 16) + (uint32_t((uint8_t)sniplet.at(3)) << 8) + (uint32_t((uint8_t)sniplet.at(4)));
	// Each bulk is deactivated for a few 100 ms to avoid confusion with double older packets
	if (status == WAIT) {
		if (timer.hasExpired(RXBLANKTIME)) status = EMPTY;
		else return false; // We've received a stray packet but the bay is still blocking incoming data -> ignore it
	}
	uint32_t sniplets = (uint32_t((uint8_t)sniplet.at(5)) << 16) + (uint32_t((uint8_t)sniplet.at(6)) << 8) + (uint32_t((uint8_t)sniplet.at(7)));
	if (status == EMPTY || sniplets != space) {
		// Bulk is empty or received a sniplet that has different size? Restructure the buffer!
		//qDebug("Preparing bulk for %d sniplets", sniplets);
		data = QByteArray(sniplets*(SNIPLETSIZE-8), 0); // Generate the space for the buffer to avoid frequent reallocs
		gotit = QByteArray(sniplets, (char)EMPTY);
		status = FILL;
		space = sniplets;
		toreceive = sniplets;
	} 
	assert(pos < gotit.size());
	data.replace(pos*(SNIPLETSIZE-8), (SNIPLETSIZE-8), sniplet.mid(8,size)); // Write the data anyway whether it's already written or not. Seems more stable to me...
	if (gotit.at(pos) == (char)EMPTY) toreceive--;// We've got a sniplet for the first time
	gotit.data()[pos] = FULL;
	//qDebug("Still to receive: %d", toreceive);
	if (toreceive == 0) {
		status = WAIT;
		space = 0;
		toreceive = 0;
		timer.start();
		return true;
	}
	return false;
}

QByteArray RxBulk::get()
{
	return data;
}

void RxBulk::clear()
{
	gotit = QByteArray("");
	status = WAIT;
	space = 0;
	toreceive = 0;
	timer.start();
	data = QByteArray("");
}

Status RxBulk::getStatus()
{
	Status s;
	s = status;
	if (s == WAIT && timer.hasExpired(RXBLANKTIME)) s = EMPTY; 
	return s;
}

float RxBulk::getFilling()
{
	if (toreceive == 0) return 0.0;
	return (float)toreceive/(float)space;
}

BulkBay::BulkBay() : QObject()
{
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		for (int i=0; i<MAXBULKBAYS; i++) {
			rxBulk[p][i] = new RxBulk;
			txBulk[p][i] = new TxBulk(i);
		}
		nextbaytofill[p] = 0;
		nextbaytoread[p] = 0;
	}
	connect(this, SIGNAL(newRemoteControl(int, QByteArray)), ctrl, SLOT(newRemoteControl(int, QByteArray)));
	connect(ctrl, SIGNAL(resetConnection(int)), this, SLOT(resetConnection(int)));
}

int BulkBay::addToTxBuffer(int player, Type type, QByteArray data, bool report, bool serialize)
// Returns an (among all valid transmission) unique id when successful, otherwise a number < 0
{
	assert(player >= 0);
	assert(player < MAXPLAYERLINKS);
	if (!ctrl->connectionStatus.get(player)) return PLAYERNOTCONNECTED; // We do not accept packets for unconnected players!
	// if (data.size() == 0) {return true;} // Do not send empty packets // Do send empty packages!
	// Search for a free bay for messages to the desired player.
	// It's important to leave a recently used bulk unused for a while not to risk getting a delayed and doubled acknowledgement for
	// a bay that holds new data meanwhile
	// If serialisation is desired, check whether a block with same type for that player already exists and report as error
	if (serialize) {
		for (int t=0; t<MAXBULKBAYS; t++) {
			if (txBulk[player][t]->getType() == type) return BLOCKFORSERIALISATION;
		}
	}
	for (int t=0; t<MAXBULKBAYS; t++) {
		nextbaytofill[player] = (nextbaytofill[player] + 1) % MAXBULKBAYS;
		//qDebug("BulkBay::addToTxBuffer: Trying bay %d for player %d type %d", nextbaytofill[player], player, int(type));
		int transferNr = player * MAXBULKBAYS + nextbaytofill[player];
		if (report) {
			if (txBulk[player][nextbaytofill[player]]->set(type, data, transferNr)) return transferNr;
		} else {
			if (txBulk[player][nextbaytofill[player]]->set(type, data, -1)) return 0;
		}
	}
	return NOFREEBAY; // No free bay!
}


void BulkBay::addToRxBuffer(int player, QByteArray sniplet)
{
	//qDebug() << "Got sniplet" << sniplet.toHex() << "from player" << player;
	assert(sniplet.size()>0);
	int bay = int((uint8_t)sniplet.at(0));
	if (rxBulk[player][bay]->set(sniplet)) {
		// We have one full buffer
		QByteArray data = rxBulk[player][bay]->get();
		if (data.size() < 1) return; // Only the first byte has to be present. For example: An empty file info packet is also an information
		//qDebug("BulkBay: We have a full bulk of type %d!", data.at(0));
		switch (data.at(0)) {
			case CHATMESSAGE: emit newChatMessage(player, data.mid(1)); break;
			case DICEROLL: emit newDiceRoll(player, data.mid(1)); break;
			case FILEINFO: emit newFileInfo(player, data.mid(1)); break;
			case FILETRANSFER: emit newFileTransfer(player, data.mid(1)); break;
			case PLAYERICON: emit newPlayerIcon(player, data.mid(1)); break;
			case VIEWER: emit newViewerEvent(player, data.mid(1)); break;
			case REMOTECONTROL: emit newRemoteControl(player, data.mid(1)); break;
			case FOCUS: 
				//qDebug("BulkBay: Received a focus request.");
				if (ctrl->remotePlayerName.get(player) == "GAMEMASTER") emit newFocusEvent(data.mid(1)); 
				else qWarning("BulkBay: Player %d who is not the gamemaster dared to send a focus event. Ignoring it", player);
				break;
			case SKETCH: emit newSketch(player, data.mid(1)); break;
			default: qWarning("BulkBay: Got an unknown data packet");
		}
		rxBulk[player][bay]->clear();
	}
}

void BulkBay::broadcast(Type type, QByteArray data)
{
	//qDebug("BulkBay: Broadcasting type %d", type);
	// Transfers the sniplet to all connected remote players
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		//qDebug("BulkBay: Broadcasting to player %d",p);
		addToTxBuffer(p, type, data);
	}
}

int BulkBay::getBufferUsage(int player)
{
	// FIXME: To save time do it incrementally...
	int u = 0;
	for (int i=0; i<MAXBULKBAYS; i++) {if (txBulk[player][i]->getStatus() == FULL) u++;}
	//qDebug("BulkBay: %d buffers in use", u);
	return u;
}

bool BulkBay::getNextSniplet(int player, QByteArray & sniplet)
{
	for (int t=0; t<MAXBULKBAYS; t++) {
		nextbaytoread[player] = (nextbaytoread[player] +1) % MAXBULKBAYS;
		if (txBulk[player][nextbaytoread[player]]->get(sniplet)) return true;
	}
	return false; // Nothing to send!
}

void BulkBay::acknowledgeSniplet(int player, int bay, int nr)
{
	assert (player >= 0);
	assert (player < MAXPLAYERLINKS);
	assert (bay >= 0);
	assert (bay < MAXBULKBAYS);
	txBulk[player][bay]->ack(nr);
}

void BulkBay::allowResend()
{
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		for (int i=0; i<MAXBULKBAYS; i++) txBulk[p][i]->allowResend();
	}
}

void BulkBay::resetConnection(int player)
{
	assert(player<MAXPLAYERLINKS);
	qDebug("BulkBay: Reset request received for player %d", player);
	for (int i=0; i<MAXBULKBAYS; i++) {
		txBulk[player][i]->clear();
		rxBulk[player][i]->clear();
	}
}
