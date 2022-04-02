#ifndef BULKTRANSFER_H
#define BULKTRANSFER_H
#include <QByteArray>
#include <QThread>
#include <QReadWriteLock>
#include <QElapsedTimer>
#include "config.h"
#include "control.h"
#include "types.h"

enum Type {UNKNOWN, CHATMESSAGE, DICEROLL, FILEINFO, FILETRANSFER, AUDIOEFFECT, NPCICON, PLAYERICON, VIEWER, REMOTECONTROL, FOCUS, 
           STATISTICS, SKETCH}; // More to come!
enum Status {EMPTY, FILL, FULL, WAIT, VOID};

#define PLAYERNOTCONNECTED -1
#define NOFREEBAY -2
#define BLOCKFORSERIALISATION -3

#define REPORT true
#define NOREPORT false
#define SERIAL true
#define PARALELL false

class TxBulk : public QObject
{
	Q_OBJECT
public:
	TxBulk(char id);
	bool get(QByteArray & sniplet);
	bool set(Type type, QByteArray bulk, int transferNr);
	void ack(int snipletnr);
	void allowResend();
	Status getStatus();
	float getFilling();
	void clear();
	Type getType();
private:
	Status status;
	QByteArray data;
	int32_t * next;
	int32_t * previous;
	int32_t first;
	int32_t actual;
	int32_t stopat;
	int32_t length;
	int32_t sniplets;
	QElapsedTimer timer;
	Type type; // For faster access - memory is cheaper than CPU power
	QReadWriteLock lock;
	char myid;
	bool rep;
	int nr;
	int lastp;
signals:
	void dataSent(void);
};

class RxBulk
{
public:
	RxBulk();
	bool set(QByteArray sniplet);
	QByteArray get();
	void clear();
	Status getStatus();
	float getFilling();
private:
	Status status;
	int32_t length;
	int32_t received;
	QElapsedTimer timer;
	uint32_t space;
	QByteArray gotit;
	int toreceive;
	QByteArray data;
};

class BulkBay : public QObject
{
	Q_OBJECT
public:
	BulkBay();
	// In case anyone asks why I use Qt's signal&slot mechanism to provide a callback: I do not want to have the callback executed in the 
	// emitter's task!
	int addToTxBuffer(int player, Type type, QByteArray data, bool report=false, bool serialize=false);
	void addToRxBuffer(int player, QByteArray sniplet);
	void broadcast(Type type, QByteArray sniplet);
	bool getNextSniplet(int player, QByteArray & sniplet);
	void acknowledgeSniplet(int player, int bay, int nr);
	void allowResend();
	int getBufferUsage(int player);
	RxBulk * rxBulk[MAXPLAYERLINKS][MAXBULKBAYS]; // FIXME: Write wrapper for these!
	TxBulk * txBulk[MAXPLAYERLINKS][MAXBULKBAYS]; // FIXME: Write wrapper for these!
public slots:
	void resetConnection(int player);
private:
	int * actualbay;
	int nextbaytofill[MAXPLAYERLINKS];
	int nextbaytoread[MAXPLAYERLINKS];
signals:
	void newChatMessage(int player, QByteArray data);
	void newDiceRoll(int player, QByteArray data);
	void newFileInfo(int player, QByteArray data);
	void newFileTransfer(int player, QByteArray data);
	void newAudioEffect(int player, QByteArray data);
	void newPlayerIcon(int player, QByteArray data);
	void newViewerEvent(int player, QByteArray data);
	void newRemoteControl(int player, QByteArray data);
	void newFocusEvent(QByteArray data);
	void newSketch(int player, QByteArray data);
};

extern BulkBay * bulkbay;

#endif