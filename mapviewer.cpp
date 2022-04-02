#include "mapviewer.h"
#include <QPushButton>


Map::Map(QString filename, QTabWidget * p) : QWidget(p)
{
	map = new QPixmap();
	name = filename;
	parent = p;
	name.replace(".png",""); name.replace(".jpg",""); name.replace(".JPG",""); name.replace(".PNG","");
	if (!map->load(ctrl->dir[DIR_MAP].filePath(filename))) {
		qWarning() << "Map(" << name << "): Error while loading " << filename;
		map->load("icons/broken.png");
	}
	qDebug() << "Map(" << name << "): Widget width" << this->width() << "height" << this->height();
	qDebug() << "Map(" << name << "): Map width" << map->width() << "height" << map->height();
	zoom = float(this->width()) / float(map->width());
	zoom = fmin(zoom, float(this->height()) / float(map->height()));
	zoomxcenter = map->width()/2.0;
	zoomycenter = map->height()/2.0;
	qDebug() << "Map: Require initial zoom level" << zoom;
	label = new QLabel(this);
	visibleButton = new VisibleButton(name, this);
	focusButton = new FocusButton(name, this);
	if (ctrl->mode.get() == MODE_MASTER) {
		// Only the gamemaster is allowed to have those mighty buttons
		visibleButton->set(ctrl->mapVisibility.get().contains(name));	
		connect(focusButton, SIGNAL(focusMe(QString)), this, SLOT(focusButtonPressed(QString)));
		connect(visibleButton, SIGNAL(toggled(QString, bool)), this, SLOT(visibleButtonToggled(QString, bool)));
	}
	else connect(bulkbay, SIGNAL(newFocusEvent(QByteArray)), this, SLOT(focusEvent(QByteArray)));
	viewUpdate();
}


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
			if (!limit(zoom, minzoom, maxzoom)) {
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
	viewUpdate();
	event->accept();
}

void Map::resizeEvent(QResizeEvent * event)
{
	viewUpdate();
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
		qDebug() << "Map(" + name + "): Mouse drag";
		zoomxcenter -= float(event->pos().x() - dragstartpos.x())/zoom;
		zoomycenter -= float(event->pos().y() - dragstartpos.y())/zoom;
		dragstartpos = event->pos();
		viewUpdate();
	}
}

void Map::viewUpdate()
{
	qDebug("Map: Widget width %d height %d", this->width(), this->height());
	qDebug("Map: Map width %d height %d", map->width(), map->height());
	qDebug("Map: Zoomcenter is at %5f / %5f", zoomxcenter, zoomycenter);
	float minzoom = fmax(float(this->width())/float(map->width()), float(this->height())/float(map->height())); // Zoomlevel that is needed to scale the picture to fill the widget completely
	float maxzoom = fmax(2.0, minzoom); // Limit the zoomlevel to 2 unless more is needed to fill the widget
	limitinplace(&zoom, minzoom, maxzoom);
	limitinplace(&zoomxcenter, width()/2.0f/zoom, map->width()-width()/2.0f/zoom);
	limitinplace(&zoomycenter, height()/2.0f/zoom, map->height()-height()/2.0f/zoom);
	qDebug("Map: Zoom is %1.4f", zoom);
	//if (zoomxcenter < width()/2.0*zoom) 
	label->resize(this->width(), this->height());
	label->setPixmap(map->copy(zoomxcenter-width()/2.0/zoom, zoomycenter-height()/2.0/zoom, float(width())/zoom, float(height())/zoom).scaledToWidth(width()));	
	visibleButton->move(10,10);
	focusButton->move(40,10);
	if (ctrl->mode.get() == MODE_MASTER) {visibleButton->show(); focusButton->show();}
	else {visibleButton->hide(); focusButton->hide();}
}

void Map::focusButtonPressed(QString focusname)
{
	qDebug() << "Map:" << name << "wants to be focused";
	QByteArray n(1, (char)FOCUS_MAP);
	n += (char)(unsigned int)(name.toUtf8().size());
	n += name.toUtf8();
	n += toBytes(zoom);
	n += toBytes(zoomxcenter);
	n += toBytes(zoomycenter);
	for (int p=0; p<MAXPLAYERLINKS; p++) bulkbay->addToTxBuffer(p, FOCUS, n);
}

void Map::visibleButtonToggled(QString name, bool status)
{
	qDebug("Map(%s): Visiblity toggled to %s", (char*)name.data(), status ? "visible":"invisible");
	QStringList names;
	QStringList visibleMaps = ctrl->mapVisibility.get();
	if (status)	visibleMaps.append(name);
	else visibleMaps.removeAll(name);
	ctrl->mapVisibility.set(visibleMaps);
	qDebug() << "Map: mapVisibility is" << visibleMaps;
}

void Map::focusEvent(QByteArray f)
{
	qDebug("Map(%s): Got focus event", (char*)name.data());
	if (f.size()<1) return;
	if (f.at(0) != (Focus)FOCUS_MAP) return; // No map focus event
	parent->setCurrentWidget(this);
	QByteArray n = f.mid(2, f.at(1)); // The length of the name (one byte) then the name
	qDebug("Map(%s): Focussing", (char*)name.data());
	if (n == name.toUtf8()) {
		qDebug("Map(%s): And that's us!", (char*)name.data());
		f = f.mid(2 + f.at(1)); // Cut off the name
		fromBytes(zoom, f);
		fromBytes(zoomxcenter, f);
		fromBytes(zoomycenter, f);
		parent->setCurrentWidget(this);
		viewUpdate();
	}
}

MapViewer::MapViewer(QTabWidget * p = NULL) : QWidget(NULL)
{
	parent = p;
	mapList = ctrl->dir[DIR_MAP].entryList(QStringList() << "*.png" << "*.PNG" << "*.jpg" << "*.JPG", QDir::Files);
	qDebug () << "MapViewer: Files in directory:" << mapList;
	/*
	for (int i=0; i<mapList.size(); i++) {
		QString name = mapList.at(i);
		name.replace(".png",""); name.replace(".jpg",""); name.replace(".JPG",""); name.replace(".PNG","");
		parent->addTab(new Map(mapList.at(i)), name);
	}*/
	connect(ctrl, SIGNAL(mapVisibilityChange()), this, SLOT(ctrlUpdate()));
	connect(facilitymanager, SIGNAL(newFileUpdate(int)), this, SLOT(fileUpdate(int)));
	readFiles();
}

void MapViewer::fileUpdate(int dir)
{
	qDebug("MapViewer: File update signal received for dir %d", dir);
	if (dir != DIR_MAP) return;
	qDebug("MapViewer: Valid file update signal received");
	readFiles();
}

void MapViewer::readFiles()
{
	int k;
	// This is called, when a new file has been received or the configuration changes
	qDebug("MapViewer: File change update");
	mapList = ctrl->dir[DIR_MAP].entryList(QStringList() << "*.png" << "*.PNG" << "*.jpg" << "*.JPG", QDir::Files);
	qDebug () << "MapViewer: Files in directory:" << mapList;
	for (int i=0; i<mapList.size(); i++) {
		QString name = mapList.at(i);
		name.replace(".png",""); name.replace(".jpg",""); name.replace(".JPG",""); name.replace(".PNG","");
		for (k=0; k<indexList.size(); k++) if (parent->tabText(k) == name) {qDebug("MapViewer: Map already exists"); break;}
		if (k == indexList.size()) {
			// Nothing in existance
			if (ctrl->mode.get() == MODE_MASTER || ctrl->mapVisibility.get().contains(name)) indexList.append(parent->addTab(new Map(mapList.at(i), parent), name)); 
			//if (ctrl->localPlayerName.get() != "GAMEMASTER") parent->setTabVisible(indexList.last(), false); // Do not show directly!
		}
	}
	// FIXME: If we're gamemaster, remove nonexistant files from the visible list! Otherwise deleting a file at the master results in an
	// always visible map at the players
}

void MapViewer::ctrlUpdate()
{
	qDebug("MapViewer: Control update received");
	if (ctrl->localPlayerName.get() != "GAMEMASTER") {
		readFiles(); // Add recently visible
		// Now remove hidden ones, FIXME: Do this with show and hide in the future
		for (int i=0; i<indexList.size(); i++) {
			if (!ctrl->mapVisibility.get().contains(parent->tabText(indexList.at(i)))) {
				//Map * map;
				//map = (Map*)parent->widget(i);
				parent->removeTab(indexList.at(i)); 
				indexList.removeAt(i);
				//delete map; Causes a crash! think of a different way to delete an invisible map. Runs now but produces a memory leak when hidden/shown
			}
		}
		//for (int i=0; i<indexList.size(); i++) parent->setTabVisible(indexList(i), ctrl->mapVisibility.get().contains(parent->tabText(indexList(i))); Works only from Qt5.15 - which I do not use!
	}
}

void MapViewer::viewUpdate()
{
	qDebug("MapViewer: View update");
	
}



