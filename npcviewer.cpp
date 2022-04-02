#include "npcviewer.h"

#define NPCTEXTHEIGHT 200
#define NPCFRAMEWIDTH 200

Npc::Npc(QString name, QWidget * parent) : QWidget(parent)
{
	qDebug("Npc: Creating new NPC");
	n = name;
	frame = new QGroupBox(name, this);
	picLabel = new QLabel(frame);
	picLabel->move(5,25);
	overlayLabel = new QLabel(frame);
	overlayLabel->move(5,20);
	opacityEffect = new QGraphicsOpacityEffect;
	opacityEffect->setOpacity(0.5);
	overlayLabel->setGraphicsEffect(opacityEffect);
	textLabel = new QLabel(frame);
	textLabel->setWordWrap(true);
	textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	portrait = new QPixmap("icons/unknown.png");
	invisible = new QPixmap("icons/invisible.png");
	focusButton = new FocusButton(name, frame);	
	focusButton->setFixedSize(30, 30);
	visibleButton = new VisibleButton(name, frame);
	visibleButton->setFixedSize(30, 30);
	visibleButton->set(ctrl->npcVisibility.get().contains(n));	
	connect(focusButton, SIGNAL(focusMe(QString)), this, SLOT(focusButtonPressed(QString)));
	connect(visibleButton, SIGNAL(toggled(QString, bool)), this, SLOT(visibleButtonToggled(QString, bool)));
	viewUpdate();
}

void Npc::setPortrait(QString filename)
{
	if (!portrait->load(ctrl->dir[DIR_NPCS].filePath(filename))) portrait->load("icons/broken.png");
}

void Npc::setText(QString text)
{
	textLabel->setText(text.replace("/","\n"));
}

void Npc::setSecret(QString text)
{
	textLabel->setText(textLabel->text() + text.replace("/","\n"));
}

void Npc::resizeEvent(QResizeEvent * event)
{
	viewUpdate();
	event->accept();
}

void Npc::viewUpdate()
{
	qDebug("Npc: Updating view");
	frame->resize(width()-10, min(height(), width() * 3) -10); // Do not set the height more that tree times the width
	frame->move(5,5);
	picLabel->resize(frame->width()-10, frame->height()-35-NPCTEXTHEIGHT);
	picLabel->setPixmap(portrait->scaledToWidth(frame->width()-10));
	overlayLabel->resize(frame->width()-10, frame->height()-35-NPCTEXTHEIGHT);
	overlayLabel->setPixmap(invisible->scaledToWidth(frame->width()-10));
	
	if (ctrl->npcVisibility.get().contains(n)) {qDebug("Npc: Hiding overlay"); overlayLabel->hide();}
	else {qDebug("Npc: Showing overlay");overlayLabel->show();}
	
	textLabel->resize(frame->width()-10, NPCTEXTHEIGHT);
	textLabel->move(5, frame->height()-10-NPCTEXTHEIGHT);
	
	visibleButton->move(5,25);
	focusButton->move(frame->width()-35,25);
	if (ctrl->mode.get() == MODE_MASTER) {visibleButton->show(); focusButton->show();}
	else {visibleButton->hide(); focusButton->hide();}
}

QString Npc::getName()
{
	return n;
}

void Npc::visibleButtonToggled(QString name, bool status)
{
	qDebug() << "Npc: npcVisibility is" << ctrl->npcVisibility.get();
	QStringList names;
	qDebug() << "Npc: visible button of" << name << "toggled to status" << status;
	QStringList visibleNpcs = ctrl->npcVisibility.get();
	if (status)	visibleNpcs.append(name);
	else visibleNpcs.removeAll(name);
	ctrl->npcVisibility.set(visibleNpcs);
	qDebug() << "Npc: npcVisibility is" << ctrl->npcVisibility.get();
	viewUpdate();
}

void Npc::focusButtonPressed(QString name)
{
	qDebug() << "Npc:" << name << "wants to be focused";
	QByteArray n(1, (char)FOCUS_NPC);
	for (int p=0; p<MAXPLAYERLINKS; p++) bulkbay->addToTxBuffer(p, FOCUS, n + name.toUtf8());
}



NpcViewer::NpcViewer(QTabWidget * p = NULL) : QScrollArea(p)
{
	parent = p;
	parent->addTab(this, "NPCs");
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	frame = new QWidget;
	this->setWidget(frame);
	xpos = 0;
	fileUpdate(DIR_NPCS, "");
	ctrlUpdate();
	connect(ctrl,            SIGNAL(npcVisibilityChange()),     this, SLOT(ctrlUpdate()));
	connect(bulkbay,         SIGNAL(newFocusEvent(QByteArray)), this, SLOT(focusEvent(QByteArray)));
	connect(ctrl,            SIGNAL(fileUpdate(int, QString)),  this, SLOT(fileUpdate(int, QString)));
}


void NpcViewer::fileUpdate(int dir, QString filename)
{
	if (dir != DIR_NPCS) return; // Not for us
	qDebug("NpcViewer: File update received");
	// New files can be found - process them!
	QFile file(ctrl->dir[DIR_NPCS].filePath("npcs.cfg"));
	if (!file.open(QIODevice::ReadOnly)) {qDebug("NpcViewer: Could not open info file"); return;}
	// For simplicity we discard all entries and load new when called.
	// If anybody feels bored, please modify this code so only new or enabled NPCs are added and only obsolete or disabled ones are removed
	for (int i=0; i<npcs.size(); i++) {delete npcs.at(i);}
	npcs.clear();
	QByteArray line;
	while (!file.atEnd()) {
		line = file.readLine();
		if (line.startsWith("name=")) {
			QByteArray name = line.replace("name=","").simplified();
			qDebug() << "NpcViewer: Adding new NPC:" << name;
			for (int i=0; i<npcs.size(); i++) if (npcs.at(i)->getName() == name) {
				qDebug() << "NpcViewer: Duplicate NPC name found" << name << "! Deleting old occurance!"; 
				delete npcs.at(i);
				npcs.remove(i);
			}
			npcs.append(new Npc(name, frame));
		}
		else if (line.startsWith("pic=")) {
			QByteArray pic = line.replace("pic=", "").simplified();
			qDebug() << "NpcViewer: Adding portrait:" << pic;
			if (npcs.isEmpty()) {qDebug("NpcViewer: Found portrait information without NPC!");}
			else (npcs.last()->setPortrait(pic));
		}
		else if (line.startsWith("text=")) {
			QByteArray text = line.replace("text=", "").simplified();
			qDebug() << "NpcViewer: Adding text:" << text;
			if (npcs.isEmpty()) {qDebug("NpcViewer: Found text information without NPC!");}
			else (npcs.last()->setText(text));
		}
		else if (line.startsWith("secret=") && ctrl->mode.get() == MODE_MASTER) {
			QByteArray text = line.replace("secret=", "").simplified();
			qDebug() << "NpcViewer: Adding secret:" << text;
			if (npcs.isEmpty()) {qDebug("NpcViewer: Found secret information without NPC!");}
			else (npcs.last()->setSecret(text));
		}
	}
	viewUpdate();
}

void NpcViewer::ctrlUpdate()
{
	xpos = 0;
	qDebug("NpcViewer: ctrlUpdate received");
	for (int i=0; i<npcs.size(); i++) {
		// Show the NPC if we're gamemaster or when it's visible
		npcs.at(i)->move(xpos, 0);
		if (ctrl->mode.get() == MODE_MASTER || ctrl->npcVisibility.get().contains(npcs.at(i)->getName())) {
			npcs.at(i)->show();
			xpos += NPCFRAMEWIDTH;
			qDebug("NpcViewer: xpos %d", xpos);
		} else npcs.at(i)->hide();
	}
	viewUpdate();
}

void NpcViewer::viewUpdate()
{
	qDebug("NpcViewer: Updating view, %d NPCs found", npcs.size());
	qDebug("NpcViewer: Parent height %d", parent->height());
	frame->resize(xpos, parent->height()-60);
	this->resize(parent->width()-8, parent->height()-34);
	for (int i=0; i<npcs.size(); i++) npcs.at(i)->resize(NPCFRAMEWIDTH, parent->height()-60);
}

void NpcViewer::focusEvent(QByteArray data)
{
	if (data.size()<1) {qWarning("NpcViewer: Received a too short focus event"); return;}
	qDebug() << "NpcViewer: Focus event" << data.mid(1);
	if (data.at(0) != FOCUS_NPC) return; // Not for us!
	parent->setCurrentWidget(this);
	int showpos = 0, totalsize = 0;
	for (int i=0; i<npcs.size(); i++) {
		if (npcs.at(i)->isVisible()) totalsize += NPCFRAMEWIDTH;
		if (npcs.at(i)->getName().toUtf8() == data.mid(1)) showpos = totalsize - NPCFRAMEWIDTH/2;
	}
	if (totalsize == 0) {qWarning("NpcViewer: Could not find the NPC to focus on!"); return;}
	if (totalsize < 0) totalsize = 0; // Can only happen when we focus on a hidden NPC, but may be a feature
	//horizontalScrollBar()->setValue(horizontalScrollBar()->maximum() - horizontalScrollBar()->minimum()) * showpos / totalsize + horizontalScrollBar()->minimum());
	horizontalScrollBar()->setValue(showpos-width()/2); // Probably this works as well
}

void NpcViewer::resizeEvent(QResizeEvent * event)
{
	qDebug("NpcViewer: Resize event");
	viewUpdate();
	event->accept();
}

/*
void Map::wheelEvent(QWheelEvent * event)
{
	qDebug("Map: Mouse Wheel");
	Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
	qDebug("Map: Mouse scrolled %d degree in y", event->angleDelta().y());
	qDebug("Map: Mouse scrolled %d degree in x", event->angleDelta().x());
	qDebug() << "Map: Mouse position while scrolling" << label->mapFromGlobal(QCursor::pos());
	float deltax = label->mapFromGlobal(QCursor::pos()).x() - width()/2.0;
	float deltay = label->mapFromGlobal(QCursor::pos()).y() - height()/2.0;
	if (modifiers & Qt::ControlModifier) {
		qDebug("Map: Scroll with Ctrl means zoom");
		float minzoom = fmax(float(this->width())/float(map->width()), float(this->height())/float(map->height())); // Zoomlevel that is needed to scale the picture to fill the widget completely
		float maxzoom = fmax(2.0, minzoom); // Limit the zoomlevel to 2 unless more is needed to fill the widget
		if (event->angleDelta().y() > 0) {
			zoom *= (1.0+event->angleDelta().y()/240.0);
			if (!flim(zoom, minzoom, maxzoom)) {
				zoomycenter += deltay/2 / zoom;
				zoomxcenter += deltax/2 / zoom;
			}
		}
		else {
			zoomycenter -= deltay/2 / zoom;
			zoomxcenter -= deltax/2 / zoom;
			zoom /= (1.0-event->angleDelta().y()/240.0);
		}

	} else if (modifiers & Qt::AltModifier) {
		qDebug("Map: Scroll with Alt");
	} else if (modifiers & Qt::ShiftModifier) {
		qDebug("Map: Scroll with Shift");
		zoomxcenter += float(event->angleDelta().y())/3/zoom;
	} else {
		qDebug("Map: Normal scroll");
		zoomycenter -= float(event->angleDelta().y())/3/zoom;
		zoomxcenter += float(event->angleDelta().x())/3/zoom;
	}
	update();
	event->accept();
}


void Map::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton) {
		qDebug("Map: Mouse Button Pressed");
		dragstartpos = event->pos();
	}
}

void Map::mouseMoveEvent(QMouseEvent * event)
{
	if (event->buttons() & Qt::LeftButton) {
		qDebug("Map: Mouse drag");
		zoomxcenter -= float(event->pos().x() - dragstartpos.x())/zoom;
		zoomycenter -= float(event->pos().y() - dragstartpos.y())/zoom;
		dragstartpos = event->pos();
		update();
	}
}

*/