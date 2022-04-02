#include "messages.h"



MessageWidget::MessageWidget(QWidget * parent) : QWidget(parent)
{
	// Harnessing
	connect(ctrl, SIGNAL(fileTransferStart(int, QString, int)), this, SLOT(fileTransferStart(int, QString, int)));
	connect(ctrl, SIGNAL(fileTransferProgress(int, int)), this, SLOT(fileTransferProgress(int, int)));
	connect(ctrl, SIGNAL(fileTransferComplete(int)), this, SLOT(fileTransferComplete(int)));
	connect(ctrl, SIGNAL(resetConnection(int)), this, SLOT(resetConnection(int)));
	//TransferInfo info; info.index = 0; info.transfer = QString("Hallo?"); info.percentage = 33; transferInfo.append(info); update();
}

void MessageWidget::fileTransferStart(int index, QString filename, int player)
{
	qDebug("MessageWidget: Got transfer start, player %d, index %d", player, index);
	TransferInfo info;
	info.index = index;
	info.player = player;
	//if (filename.length() > 14) filename = filename.mid(0, 12)+"...";
	info.transfer = QString(ctrl->playerSign.get(player+1)) + " " + QString(QChar(0x21E6)) + " " + filename;
	info.percentage = 0;
	transferInfo.append(info);
	update();
}

void MessageWidget::fileTransferProgress(int index, int percentage)
{
	//qDebug("MessageWidget: Got transfer progress update, index %d: %d", index, percentage);
	for (int i=0; i<transferInfo.size(); i++) {
		if (transferInfo.at(i).index == index) {transferInfo[i].percentage = percentage; break;}
	}
	update();
}

void MessageWidget::fileTransferComplete(int index)
{
	for (int i=0; i<transferInfo.size(); i++) {
		if (transferInfo.at(i).index == index) {transferInfo.remove(i); break;}
	}
	update();
}

void MessageWidget::resetConnection(int player)
{
	for (int i=transferInfo.size()-1; i>=0; i--) {
		if (transferInfo.at(i).player == player) transferInfo.remove(i);
	}
	update();
}

void MessageWidget::paintEvent(QPaintEvent * event)
{
	QPainter painter(this);
	QBrush brush;
	painter.beginNativePainting();
	painter.setRenderHint(QPainter::Antialiasing);
	brush.setColor(Qt::gray);
	brush.setStyle(Qt::SolidPattern);
	painter.setBrush(brush);
	for (int i=0; i<transferInfo.length() && i < height()/20; i++) {
		painter.setPen(Qt::gray);
		painter.drawRect(0, i*20+1, width() * transferInfo[i].percentage / 100, 18);
		painter.setPen(Qt::black);
		painter.drawText(5, i*20, width()-10, 20, Qt::AlignLeft, transferInfo[i].transfer);
	}
	event->accept();
}


Messages::Messages(QWidget * parent) : QWidget(parent)
{
	this->setAutoFillBackground(true);
	frame = new QGroupBox("File Transfers", this);
	frame->move(5,5);
	messageWidget = new MessageWidget(frame);
	messageWidget->move(5,25);
	messageWidget->resize(frame->width()-10, frame->height()-30);
}

void Messages::resizeEvent(QResizeEvent * event)
{
	frame->resize(width()-10, height()-10);
	messageWidget->resize(frame->width()-10, frame->height()-25);
	event->accept();
}

