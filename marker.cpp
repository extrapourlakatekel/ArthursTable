#include "marker.h"

Marker::Marker(int id, QWidget *parent) : QLabel(parent)
{
	this->parent = parent;
	this->id = id;
	areawidth = 0;
	areaheight = 0;
	xpos = 0;
	ypos = 0;
	active = false; // Must be activated prior to use
	colornr = 0;
	setAutoFillBackground(false);
	setFixedSize(MARKER_SIZE, MARKER_SIZE); 
	cursor.setShape(Qt::PointingHandCursor);
	setCursor(cursor);
	deleteAct = new QAction("Delete");
	connect(deleteAct, SIGNAL(triggered(bool)), this, SLOT(deletePressed()));
	hide(); // Don not show unless told differently
	timer.setSingleShot(false);
	timer.setInterval(500);
	connect(&timer, SIGNAL(timeout()), this, SLOT(timerTick()));
	timer.start();
}

Marker::~Marker()
{
	// FIXME: Probably required for saving
}

void Marker::setStyle(MarkerStyle style, int colornr) {
	this->colornr = colornr;
	this->style = style;
}

void Marker::updateZoom(float zoom, float zoomxcenter, float zoomycenter)
{
	this->zoom = zoom;
	this->zoomxcenter = zoomxcenter;
	this->zoomycenter = zoomycenter;
}


void Marker::updatePosition()
{
	move((xpos - zoomxcenter) * zoom + parent->width()/2 - MARKER_SIZE/2, 
	     (ypos - zoomycenter) * zoom + parent->height()/2 - MARKER_SIZE/2);
}

QPoint Marker::getPos()
{
	return QPoint(xpos, ypos);
}

void Marker::setAbsPos(int x, int y)
{
	xpos = x;
	ypos = y;
}

void Marker::setRelPos(int x, int y)
{
	xpos = (x - parent->width()/2) / zoom + zoomxcenter;
	ypos = (y - parent->height()/2) / zoom + zoomycenter;
	qDebug("Marker: Rel pos x:%d, y:%d", xpos, ypos);
}

void Marker::updateBorder(int width, int height)
// Call when the image underneath changes to ensure that no marker is outside the image
{
	areawidth = width;
	areaheight = height;
	limitinplace(&xpos, 0, areawidth-1);
	limitinplace(&ypos, 0, areaheight-1); 
}

void Marker::draw(QPainter * painter, MarkerStyle style, int colorindex, int animator)
{
	assert(colorindex < MARKER_COLORS),
	assert(colorindex >= 0);
	assert(style>=0); assert(style < MARKER_STYLES);
	QPen pen;
	QBrush brush(Qt::SolidPattern);
	switch (style) {
		case MARKER_TEAM:
			pen.setColor(Qt::black);
			pen.setWidth(5);
			painter->setPen(pen);
			painter->drawEllipse(4,4,13,13);
			pen.setColor(Qt::yellow);
			pen.setWidth(1);
			brush.setColor(Qt::yellow);
			painter->setPen(pen);
			painter->setBrush(brush);
			painter->drawPie(3,3,MARKER_SIZE-6, MARKER_SIZE-6, 0, 360*16);
			pen.setWidth(3);
			pen.setColor(Qt::black);
			painter->setPen(pen);
			painter->drawText(0,0, MARKER_SIZE-1, MARKER_SIZE-1, Qt::AlignHCenter | Qt::AlignCenter, QString('A'+colorindex));
			break;
		case MARKER_CIRCLE:
			pen.setColor(Qt::black);
			pen.setWidth(5);
			painter->setPen(pen);
			painter->drawEllipse(4,4,13,13);
			pen.setColor(markerPal[colorindex]);
			pen.setWidth(1);
			brush.setColor(markerPal[colorindex]);
			painter->setPen(pen);
			painter->setBrush(brush);
			painter->drawPie(3,3,MARKER_SIZE-6, MARKER_SIZE-6, 0, 360*16);
			break;
		case MARKER_SQUARE:
			break;
		case MARKER_TRIANGLE:
			break;
		case MARKER_HEXAGON:
			break;
		case MARKER_CROSS:
			break;
		default:
			qWarning("Marker: Unknown marker style to draw");
	}
}

QByteArray Marker::getConfig()
{
	return toNBO(xpos) + toNBO(ypos) + toNBO(team) + toNBO(active) + toNBO(style) + toNBO(colornr);	
}

void Marker::setConfig(QByteArray * config)
{
	if(!( fromNBO(config, &xpos) && fromNBO(config, &ypos) && fromNBO(config, &team) && fromNBO(config, &active) &&
	      fromNBO(config, &style) && fromNBO(config, &colornr))) {qWarning("Marker %d: Error setting config!", id);}
}

void Marker::paintEvent(QPaintEvent * event)
{
	qDebug("Marker %d: Paint event", id);
	QPainter painter(this);
	painter.beginNativePainting();
	painter.setRenderHint(QPainter::Antialiasing);
	draw(&painter, style, colornr);
	event->accept();
}

void Marker::mousePressEvent(QMouseEvent * event)
{
	if (ctrl->mode.get() != MODE_MASTER) return;
	cursor.setShape(Qt::CrossCursor);
	setCursor(cursor);
	event->accept();
}

void Marker::mouseReleaseEvent(QMouseEvent * event)
{
	cursor.setShape(Qt::PointingHandCursor);
	setCursor(cursor);
	event->accept();
}

void Marker::mouseMoveEvent(QMouseEvent * event)
{
	if (ctrl->mode.get() != MODE_MASTER) return; // Only mater may move markers
	if (areawidth == 0 || areaheight == 0) return; // No image, no move!
	QPoint point = mapTo(parent, event->pos());
	xpos = (point.x() - parent->width()/2) / zoom + zoomxcenter;
	ypos = (point.y() - parent->height()/2) / zoom + zoomycenter;
	limitinplace(&xpos, 0, areawidth-1);
	limitinplace(&ypos, 0, areaheight-1);
	updatePosition();
	emit(dragged(id));
}

void Marker::contextMenuEvent(QContextMenuEvent *event)
{
	if (ctrl->mode.get() != MODE_MASTER || id < MAXTEAMS) return; // Sorry, no context menu for players and thou shalt not delete team markers!
	QMenu menu;
	menu.addAction(deleteAct);
	menu.exec(event->globalPos());
}

void Marker::timerTick()
{
	//if (mycolor == Qt::red) mycolor = Qt::black; else mycolor = Qt::red;
	//update();
}

void Marker::deletePressed()
{
	qDebug("Marker: Removing");
	active = false;
	emit(deleted(id));
}

//FIXME:  Move markerselection to own file!

MarkerSelection::MarkerSelection(QObject* parent) : QWidgetAction(parent)
{
	qDebug("MarkerSelection: Constructor finally called");
}

QWidget * MarkerSelection::createWidget(QWidget * parent)
{
	qDebug("MarkerSelection: Creating widget");
	this->parent = parent;
	QWidget * widget = new QWidget(parent);
	widget->setAutoFillBackground(true);
	widget->setFixedSize(MARKER_COLORS * MARKER_SIZE + 10, (MARKER_STYLES - 1) * MARKER_SIZE + 10);
	QIcon icon;
	QPixmap pixmap(MARKER_SIZE, MARKER_SIZE);
	QPainter painter(&pixmap);
	for (int s=1; s<MARKER_STYLES; s++) {
		for (int c=0; c<MARKER_COLORS; c++) {
			pixmap.fill(widget->palette().color(QPalette::Window));
			button[s][c] = new QToolButton(widget);
			button[s][c]->resize(MARKER_SIZE, MARKER_SIZE);
			button[s][c]->move(c * (MARKER_SIZE) + 5, s * (MARKER_SIZE) + 5);
			Marker::draw(&painter, (MarkerStyle)s, c);
			button[s][c]->setIcon(QIcon(pixmap));
			button[s][c]->setAutoRaise(true);
			button[s][c]->setCheckable(false);
			button[s][c]->setStyleSheet("QPushButton { background-color: transparent }");
			connect(button[s][c], SIGNAL(pressed()), this, SLOT(buttonPressed()));
		}
	}
	qDebug("MarkerSelection: Done creating widget");
	return widget;
}

void MarkerSelection::buttonPressed()
{
	// First: Find out which button had been pressed
	QToolButton * btn = qobject_cast<QToolButton*>(sender());
	qDebug("MarkerSelection: Marker selected");
	for (int s=0; s<MARKER_STYLES; s++) {
		for (int c=0; c<MARKER_COLORS; c++) {
			if (button[s][c] == btn) {button[s][c]->setDown(false); button[s][c]->repaint(); emit(newMarker(s,c)); break;}
		}
	}
	trigger();
	//parent->close();
}
