#include "visualizer.h"
#include <math.h>

StatusIndicator::StatusIndicator(QWidget * parent) : QWidget(parent)
{
	statuslabel = new QLabel(this);
	statuslabel->resize(4,4);
	statuslabel->move (0,0);
	fillinglabel = new QLabel(this);
	fillinglabel->resize(140, 4);
	fillinglabel->move(6,0);
	this->resize(150,4);
	oldstatus = VOID;
	oldfilling = -1;
	set(EMPTY, 0);
}


void StatusIndicator::set(Status status, float filling)
{
	QPixmap fpixmap(140,2);
	QImage fimage(140, 1, QImage::Format_RGB888);
	QPixmap spixmap(4,4);
	QImage simage(1, 1, QImage::Format_RGB888);
	if (oldstatus != status) {
		switch (status) {
			case EMPTY: simage.setPixel(0, 0, QColor("lightgreen").rgb()); break;
			case FILL: simage.setPixel(0, 0, QColor("yellow").rgb()); break;
			case FULL: simage.setPixel(0, 0, QColor("red").rgb()); break;
			case WAIT: simage.setPixel(0, 0, QColor("blue").rgb()); break;
			default: break;
		}
		oldstatus = status;
		spixmap.convertFromImage(simage.scaled(4, 4));
		statuslabel->setPixmap(spixmap);	
	}
	if (fabs(oldfilling - filling) > 0.005) {
		for (int i=0; i<140; i++) {
			if (filling > (float)i/140.0) fimage.setPixel(i,0, QColor("darkgrey").rgb()); else fimage.setPixel(i,0, QColor("lightgrey").rgb());
		}
		fpixmap.convertFromImage(fimage.scaled(140, 4));
		fillinglabel->setPixmap(fpixmap);	
		oldfilling = filling;
	}
}



Plot::Plot(QWidget * parent) : QWidget(parent)
{
	rxp = new int[PLOTWIDTH];
	for (int i=0; i<PLOTWIDTH; i++) rxp[i] = 0;
	txb = new int[PLOTWIDTH];
	for (int i=0; i<PLOTWIDTH; i++) txb[i] = 0;
	rxb = new int[PLOTWIDTH];	
	for (int i=0; i<PLOTWIDTH; i++) rxb[i] = 0;
	pos = 0;

}

void Plot::addData(int rxpackets, int txbytes, int rxbytes)
{
	pos = (pos+1) % PLOTWIDTH;
	rxp[pos] = rxpackets;
	txb[pos] = txbytes;
	rxb[pos] = rxbytes;
	update();
}

void Plot::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	QBrush brush(Qt::SolidPattern);
	QPen pen;
	painter.setRenderHint(QPainter::Antialiasing);
	pen.setWidth(2);
	painter.beginNativePainting();
	brush.setColor(QColor::fromRgb(0xFF, 0xFF, 0xFF));
	painter.fillRect(0,0, width() -1, height() -1, brush);
	int oldx;
	int newx;
	pen.setColor(Qt::blue);		
	painter.setPen(pen);
	oldx = 1;
	for (int i=0; i<PLOTWIDTH; i++) {
		newx = (width()-2) * i / PLOTWIDTH + 1;
		painter.drawLine(oldx, height()-2-rxp[(i+pos  )%PLOTWIDTH]*(height()*3/4-2)/LOGSIZE,  
						 newx, height()-2-rxp[(i+pos+1)%PLOTWIDTH]*(height()*3/4-2)/LOGSIZE);
		oldx = newx;
	}
	pen.setColor(Qt::green);		
	painter.setPen(pen);
	oldx = 1;
	for (int i=0; i<PLOTWIDTH-1; i++) {
		newx = (width()-2) * i / PLOTWIDTH + 1;
		painter.drawLine(oldx, height()-2-rxb[(i+pos  )%PLOTWIDTH]*(height()*3/4-2)/LOGSIZE/MAXPACKETSIZE,  
						 newx, height()-2-rxb[(i+pos+1)%PLOTWIDTH]*(height()*3/4-2)/LOGSIZE/MAXPACKETSIZE);
		oldx = newx;
	}
	pen.setColor(Qt::red);		
	painter.setPen(pen);
	oldx = 1;
	for (int i=0; i<PLOTWIDTH-1; i++) {
		newx = (width()-2) * i / PLOTWIDTH + 1;
		painter.drawLine(oldx, height()-2-txb[(i+pos  )%PLOTWIDTH]*(height()*3/4-2)/LOGSIZE/MAXPACKETSIZE,  
						 newx, height()-2-txb[(i+pos+1)%PLOTWIDTH]*(height()*3/4-2)/LOGSIZE/MAXPACKETSIZE);
		oldx = newx;
	}
	pen.setColor(Qt::black);
	painter.setPen(pen);
	painter.drawLine(1,1,1,height()-2);
	painter.drawLine(1,height()-2, width()-2, height() - 2);

	pen.setStyle(Qt::DotLine);
	pen.setColor(Qt::darkGray);
	painter.setPen(pen);
	painter.drawLine(1,height()/4, width()-2, height()/4);
	painter.drawText(5, height()/4 - 20, 60, 20, Qt::AlignLeft, "100%");
	pen.setColor(Qt::blue);
	painter.setPen(pen);
	painter.drawText(5, height()-60 , 100, 20, Qt::AlignLeft, "RX Packets");
	pen.setColor(Qt::red);
	painter.setPen(pen);
	painter.drawText(5, height()-40 , 100, 20, Qt::AlignLeft, "TX Traffic");
	pen.setColor(Qt::green);
	painter.setPen(pen);
	painter.drawText(5, height()-20 , 100, 20, Qt::AlignLeft, "RX Traffic");
	event->accept();
}




Visualizer::Visualizer(QWidget * parent) : QDialog(parent)
{
	int ypos = 0;
	this->setWindowTitle("Visualized Data Transfer");
	this->setModal(false);
	rxlabel = new QLabel("Transmissions", this);
	rxlabel->move(20,0);
	txlabel = new QLabel("Receptions", this);
	txlabel->move(335, 0);
	ypos += rxlabel->height();
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		playerlabel[p] = new QLabel("P\nL\nA\nY\nE\nR\n\n" + QString::number(p+1), this);
		playerlabel[p]->move(5, ypos);
		plot[p] = new Plot(this);
		plot[p]->resize(200, MAXBULKBAYS/2*5);
		plot[p]->move(650, ypos);
		for (int bay=0; bay<MAXBULKBAYS/2; bay++){
			txindicator[p][bay] = new StatusIndicator(this);
			txindicator[p][bay]->move(25,ypos);
			txindicator[p][bay]->show();
			txindicator[p][bay+MAXBULKBAYS/2] = new StatusIndicator(this);
			txindicator[p][bay+MAXBULKBAYS/2]->move(175,ypos);
			txindicator[p][bay+MAXBULKBAYS/2]->show();
			rxindicator[p][bay] = new StatusIndicator(this);
			rxindicator[p][bay]->move(335,ypos);
			rxindicator[p][bay+MAXBULKBAYS/2] = new StatusIndicator(this);
			rxindicator[p][bay+MAXBULKBAYS/2]->move(485,ypos);
			ypos += 5;
		}
		ypos += 5;
	}
	timer = new QTimer();
	timer->setSingleShot(false);
	timer->setInterval(100);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start();
	connect(ctrl, SIGNAL(statistics(QList<int>, QList<int>, QList<int>)), this, SLOT(statistics(QList<int>, QList<int>, QList<int>)));
	resize(850, ypos+5);
	show();
}

Visualizer::~Visualizer()
{
	qDebug("Visualizer: Destructor called");
	timer->stop();
	disconnect(timer, SIGNAL(timeout()), this, SLOT(update()));
}

void Visualizer::update()
{
	static int filling = 0;
	filling++;
	//qDebug("Visualizer: Update");
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		for (int bay=0; bay<MAXBULKBAYS; bay++) {
			//txindicator[p][bay]->set(WAIT, filling);
			txindicator[p][bay]->set(bulkbay->txBulk[p][bay]->getStatus(), bulkbay->txBulk[p][bay]->getFilling());
			rxindicator[p][bay]->set(bulkbay->rxBulk[p][bay]->getStatus(), bulkbay->rxBulk[p][bay]->getFilling());
		}
	}
}

void Visualizer::statistics(QList<int> rxpackets, QList<int> txbytes, QList<int> rxbytes)
{
	//qDebug("Visualizer: Got network usage");
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		assert(p<rxpackets.size());
		assert(p<rxbytes.size());
		assert(p<txbytes.size());
		plot[p]->addData(rxpackets.at(p), txbytes.at(p), rxbytes.at(p));
	}

}