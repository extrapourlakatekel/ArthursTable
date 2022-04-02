#ifndef MESSAGES_H
#define MESSAGES_H
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include "control.h"


class MessageWidget : public QWidget 
{
Q_OBJECT
typedef struct {
	int index;
	int player;
	QString transfer;
	int percentage;
} TransferInfo;

public:
	MessageWidget(QWidget * parent);
public slots:
	void fileTransferStart(int, QString, int);
	void fileTransferProgress(int index, int percentage);
	void fileTransferComplete(int);
	void resetConnection(int player);
private:
	QVector<TransferInfo> transferInfo;
protected:
	void paintEvent(QPaintEvent * event);
};

class Messages : public QWidget
{
public:
	Messages(QWidget * parent);
protected:
	void resizeEvent(QResizeEvent * event);
private:
	QGroupBox * frame;
	MessageWidget * messageWidget;	
};
#endif