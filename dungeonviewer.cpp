#include "dungeonviewer.h"

DungeonViewer::DungeonViewer(QTabWidget * p) : QWidget(p)
{
	parent = p;
	qDebug("DungeonViewer: ");
	parent->addTab(this, "Dungeons");
	setStyleSheet("Background-color:black");
	radius = 200;
	readFiles();
	ctrlUpdate();
	timer = new QTimer(); // Required for map animation
	timer->setInterval(100);
	timer->setSingleShot(false);
	connect(timer, SIGNAL(timeout()), this, SLOT(timerTick()));
	connect(facilitymanager, SIGNAL(newFileUpdate(int)), this, SLOT(fileUpdate(int)));
	connect(bulkbay, SIGNAL(newFocusEvent(QByteArray)), this, SLOT(focusEvent(QByteArray)));
	repaint();
}

void DungeonViewer::setAlpha(QImage * image, int value)
{
	for (int x=0; x<image->width(); x++) {
		for (int y=0; y<image->height(); y++) {
			QColor c = image->pixelColor(x, y);
			c.setAlpha(value);
			image->setPixelColor(x, y, c);
		}
	}
}

void DungeonViewer::fill(Dungeon dungeon, QPoint f)
{
	qDebug() << "DungeonViewer: Start filling at" << f;
	QVector <QPoint> iterator;
	iterator << f;
	QPoint p;
	while (iterator.size()>0) {
		p = iterator.last();
		if (p.x() >= 0 && p.x() < dungeon.map->width() && p.y() >= 0 && p.y() < dungeon.map->height() &&
			!dungeon.wall[p.y()*dungeon.width+p.x()] && dungeon.map->pixelColor(p).alpha() != 0xFF) {
			// The pixel to fill is within the image and ...
			// ... there is no black wall in the original image and the alpha channel pixel is not white yet, so set it white and spread further!
			QColor c = dungeon.map->pixelColor(p);
			c.setAlpha(0xFF);
			dungeon.map->setPixelColor(p, c);
			iterator.removeLast(); // Is processed
			// Add the sourrounding 4 pixels to the list
			iterator << p + QPoint(1,0);
			iterator << p + QPoint(-1,0);
			iterator << p + QPoint(0,1);
			iterator << p + QPoint(0,-1);
		} else iterator.removeLast(); // Nothing to process
	}
}

void DungeonViewer::enlight(Dungeon dungeon, QPoint f, int radius)
{
	assert(radius < 500);
	qDebug() << "DungeonViewer: Enlighting around" << f << "with radius" << radius;
	QVector <QPoint> iterator;
	iterator << f;
	QPoint p;
	int x, y;
	// First: Do the Lee algorithm to make the diffuse light:
	uint16_t lee[2 * radius][2 * radius];
	memset((void*)lee, 0, 2 * 2 * radius * radius * sizeof(uint16_t));
	lee[radius][radius] = radius * 2;
	int xx, yy;
	int abort;
	// Fast Lee:
	QVector<QPoint> * oldlist, * newlist;
	oldlist = new QVector<QPoint>;
	(*oldlist) << QPoint(radius, radius);
	for (int i=1; i<radius && !abort; i++) {
		newlist = new QVector<QPoint>;
		abort = true;
		for (int k=0; k<oldlist->size(); k++) {
			x = oldlist->at(k).x();
			y = oldlist->at(k).y();
			xx = f.x() + x - radius;
			yy = f.y() + y - radius;
			// When we don't sit in a wall or at the image's border spread further
			if (xx > 0 && yy > 0 && xx < dungeon.width-1 && yy < dungeon.height-1 && lee[x][y] >= 3 && !dungeon.wall[yy*dungeon.width + xx]) {
				// Orthogonal
				if (lee[x][y-1] < lee[x][y] - 2) {lee[x][y-1] = lee[x][y] - 2; (*newlist) << QPoint(x, y-1); abort = false;}
				if (lee[x-1][y] < lee[x][y] - 2) {lee[x-1][y] = lee[x][y] - 2; (*newlist) << QPoint(x-1, y); abort = false;}
				if (lee[x][y+1] < lee[x][y] - 2) {lee[x][y+1] = lee[x][y] - 2; (*newlist) << QPoint(x, y+1); abort = false;}
				if (lee[x+1][y] < lee[x][y] - 2) {lee[x+1][y] = lee[x][y] - 2; (*newlist) << QPoint(x+1, y); abort = false;}
				// Diagonal, well sqrt(2) is almost excatly 1.5...
				if (lee[x-1][y-1] < lee[x][y] - 3) {lee[x-1][y-1] = lee[x][y] - 3; (*newlist) << QPoint(x-1, y-1); abort = false;}
				if (lee[x-1][y+1] < lee[x][y] - 3) {lee[x-1][y+1] = lee[x][y] - 3; (*newlist) << QPoint(x-1, y+1); abort = false;}
				if (lee[x+1][y-1] < lee[x][y] - 3) {lee[x+1][y-1] = lee[x][y] - 3; (*newlist) << QPoint(x+1, y-1); abort = false;}
				if (lee[x+1][y+1] < lee[x][y] - 3) {lee[x+1][y+1] = lee[x][y] - 3; (*newlist) << QPoint(x+1, y+1); abort = false;}
			}
		}
		delete oldlist;
		oldlist = newlist;
	}	
	/*
	// Medium Speed Lee
	for (int i=1; i<radius && !abort; i++) {
		abort = true;
		qDebug() << i;
		for (y = radius-i; y<radius+i; y++) {
			for (x = radius-i; x<radius+i; x++) {
				xx = f.x() + x - radius;
				yy = f.y() + y - radius;
				// When we don't sit in a wall or at the image's border spread further
				if (xx > 0 && yy > 0 && xx < dungeon.width-1 && yy < dungeon.height-1 && lee[x][y] >= 3 && !dungeon.wall[yy*dungeon.width + xx]) {
					// Orthogonal
					lee[x][y-1] = max(lee[x][y-1], lee[x][y] - 2);
					lee[x-1][y] = max(lee[x-1][y], lee[x][y] - 2);
					lee[x+1][y] = max(lee[x+1][y], lee[x][y] - 2);
					lee[x][y+1] = max(lee[x][y+1], lee[x][y] - 2);
					// Diagonal, well sqrt(2) is almost excatly 1.5...
					lee[x-1][y-1] = max(lee[x-1][y-1], lee[x][y] - 3);
					lee[x-1][y+1] = max(lee[x-1][y+1], lee[x][y] - 3);
					lee[x+1][y-1] = max(lee[x+1][y-1], lee[x][y] - 3);
					lee[x+1][y+1] = max(lee[x+1][y+1], lee[x][y] - 3);
					abort = false;
				}
			}
		}
	} */
	for (y=0; y<2*radius; y++) {
		for (x = 0; x<2*radius; x++) {
			xx = f.x() + x - radius;
			yy = f.y() + y - radius;
			if (xx >= 0 && yy >= 0 && xx < dungeon.width && yy < dungeon.height) {
				QColor c = dungeon.map->pixelColor(xx, yy);
				c.setAlpha(max(limit((int)lee[x][y]*120/radius, 0, 80), c.alpha()));
				dungeon.map->setPixelColor(xx, yy, c);
			}
		}
	}
	for (int c =0; c <= (int) (2*M_PI*radius); c++) {
		for (int r=2; r<radius; r++) {
			x = f.x() + (int) (sinf((float)c / (float) radius) * (float) r);
			y = f.y() + (int) (cosf((float)c / (float) radius) * (float) r);
			if (x<0 || x>=dungeon.map->width()) break;
			if (y<0 || y>=dungeon.map->height()) break;
			if (dungeon.wall[y*dungeon.width+x]) break; // Stop the ray at a black wall
			QColor c = dungeon.map->pixelColor(x, y);
			c.setAlpha(max((int)(sqrt(1-sqr((float)r/(float)radius))*(float)0xFF),c.alpha()));
			dungeon.map->setPixelColor(x, y, c);
		}
	}
	qDebug("Enlighting done");

}

void DungeonViewer::ctrlUpdate()
{
	
}

void DungeonViewer::fileUpdate(int dir)
{
	qDebug("DungeonViewer: File update signal received for dir %d", dir);
//	if (dir != DIR_DUNGEON) return;
	qDebug("DungeonViewer: Valid file update signal received");
	readFiles();
}

void DungeonViewer::readFiles()
{
	qDebug("DungeonViewer: Scanning files in directory");
	// Reloading all files - homework: reload changed files only
	// Free all old images first
	for (int i=0; i<dungeon.size(); i++) delete dungeon.at(i).map;
	dungeon.clear();
	QStringList filters;
	filters << "*.png" << "*.PNG" << "*.gif" << "*.GIF" << "*.pbm" << "*.PBM" << "*.ppm" << "*.PPM" << "*.bmp" << "*.BMP"; // Do not allow for lossy compression formats
	QStringList filesindirectory; // = ctrl->dir[DIR_DUNGEON].entryList(filters, QDir::Files);
	for (int i=0; i<filesindirectory.size(); i++) {
		qDebug() << "DungeonViewer: Found" << filesindirectory.at(i);
		Dungeon d;
		QImage image; //(ctrl->dir[DIR_DUNGEON].filePath(filesindirectory.at(i)));
		if (image.isNull()) qWarning() << "DungeonViewer: Could not open" << filesindirectory.at(i); 
		else {
			d.map = new QImage(image.convertToFormat(QImage::Format_RGBA8888));
			d.width = d.map->width();
			d.height = d.map->height();
			d.wall = new uint8_t[d.width * d.height]; // Cache for speeding calculations up
			// For speeding things up make a wall table
			for (int y=0; y<d.height; y++) {
				for (int x=0; x<d.width; x++) {
					if (d.map->pixelColor(x, y).lightness() == 0x00) d.wall[y*d.width+x] = 0xFF; else d.wall[y*d.width+x] = 0x00;
				}
			}
			d.thumbnail = new QImage(d.map->scaled(THUMBNAILSIZE, THUMBNAILSIZE, Qt::KeepAspectRatio));
			if (ctrl->mode.get() == MODE_MASTER) setAlpha(d.map, 0x40); else setAlpha(d.map, 0x01);
			d.name = filesindirectory.at(i).toUtf8();
			dungeon.append(d);
		}
	}
	actualmap = -1;
	if (ctrl->mode.get() == MODE_MASTER) mode = DV_SELECT; else mode = DV_DISPLAY;
	repaint();
}

void DungeonViewer::paintEvent(QPaintEvent * event)
{
	// All the drawing is done here
	qDebug("DungeonViewer: PaintEvent; size of frame: %4d/%4d", width(), height());
	QPainter painter(this);
	// Start with a black background
	painter.fillRect(0, 0, width(), height(), QColor("black"));
	if (mode == DV_SELECT) {
		int i = 0;
		// FIXME: Allow scrolling of dungeon maps
		for (int y=0; y<height() / (THUMBNAILSIZE+10); y++) {
			for (int x=0; x<width() / (THUMBNAILSIZE+10); x++) {
				if (y>=0 && i<dungeon.size()) {
					painter.drawImage(x * (THUMBNAILSIZE+10) + 5, y * (THUMBNAILSIZE+10) + 5, *dungeon.at(i).thumbnail);
				}
				i++;
			}
		}
	} else {
		if (actualmap == -1) return;
		// Draw the desired rectangle of the actual map on the screen
		int fromx, fromy, sizex, sizey, offsetx, offsety;
		fromx = centerx - width() / 2;
		fromy = centery - height() / 2;
		sizex = width();
		sizey = height();
		if (fromx < 0) {offsetx = -fromx; fromx = 0;} else offsetx = 0;
		if (fromy < 0) {offsety = -fromy; fromy = 0;} else offsety = 0;
		assert(dungeon.size()>=actualmap);
		limitinplace(&sizex, 0, dungeon.at(actualmap).map->width() - fromx);
		limitinplace(&sizey, 0, dungeon.at(actualmap).map->height() - fromy);
		painter.drawImage(offsetx, offsety, *dungeon.at(actualmap).map, fromx, fromy, sizex, sizey);
	}	
	event->accept();
}

void DungeonViewer::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton) {
		if (mode == DV_SELECT) {
			// FIXME: Allow scrolling of dungeon maps
			actualmap = (event->pos().y() / (THUMBNAILSIZE+10)) * (width() / (THUMBNAILSIZE+10)) + event->pos().x() / (THUMBNAILSIZE + 10);
			qDebug() << "DungeonViewer: Selected dungeon" << actualmap;
			if (actualmap >= dungeon.size()) actualmap = -1;
			else {
				mode = DV_DISPLAY;
				centerx = dungeon.at(actualmap).map->width()/2;
				centery = dungeon.at(actualmap).map->height()/2;
			}
			repaint();
		} else {
			char variant;
			if (actualmap == -1) return;
			Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
			if (!modifiers) {
				// No modifiers means "drag"
				qDebug("DungeonViewer: Dragstart");
			} else if (modifiers & Qt::AltModifier && ctrl->mode.get() == MODE_MASTER) {
				// Click with Alt means "fill"
				variant = DV_FILL;
				qDebug("DungeonViewer: Fill");
				// Pressing Ctrl means "exclusively"
				if (modifiers & Qt::ControlModifier) {
					variant |= DV_EXCLUSIVE;
					assert(dungeon.size()>=actualmap);
					setAlpha(dungeon.at(actualmap).map, 0x40);
				}
				QPoint p(event->pos().x()+centerx-width()/2, event->pos().y()+centery-height()/2);
				
				qDebug() << "DungeonViewer: Sending fill & focus";
				QByteArray n(1, (char)FOCUS_DUNGEON);
				n.append(variant); // Variant
				n.append((char) ((uint16_t)p.x() >> 8)); // Coordinates
				n.append((char) ((uint16_t)p.x() & 0xFF));
				n.append((char) ((uint16_t)p.y() >> 8));
				n.append((char) ((uint16_t)p.y() & 0xFF));
				n.append((char) ((uint16_t)radius >> 8));
				n.append((char) ((uint16_t)radius & 0xFF));
				assert(dungeon.size()>=actualmap);

				n.append(dungeon.at(actualmap).name); // Filename
				
				for (int p=0; p<MAXPLAYERLINKS; p++) bulkbay->addToTxBuffer(p, FOCUS, n); // And out to all!
				
				fill(dungeon.at(actualmap), p); // Filling takes a little time so do it after sending the focus event out
				repaint();
			} else if (modifiers & Qt::ShiftModifier && ctrl->mode.get() == MODE_MASTER) {
				// Click with Shift means "enlight"
				variant = DV_ENLIGHT;
				qDebug("DungeonViewer: Enlight");
				// Pressing Ctrl means "exclusively"
				if (modifiers & Qt::ControlModifier) {
					variant |= DV_EXCLUSIVE;
					assert(dungeon.size()>=actualmap);
					setAlpha(dungeon.at(actualmap).map, 0x40);
				}
				QPoint p(event->pos().x()+centerx-width()/2, event->pos().y()+centery-height()/2);

				qDebug() << "DungeonViewer: Sending enlightment & focus";
				QByteArray n(1, (char)FOCUS_DUNGEON);
				n.append(variant); // Variant
				n.append((char) ((uint16_t)p.x() >> 8)); // Coordinates
				n.append((char) ((uint16_t)p.x() & 0xFF));
				n.append((char) ((uint16_t)p.y() >> 8));
				n.append((char) ((uint16_t)p.y() & 0xFF));
				n.append((char) ((uint16_t)radius >> 8));
				n.append((char) ((uint16_t)radius & 0xFF));
				assert(dungeon.size()>=actualmap);
				n.append(dungeon.at(actualmap).name); // Filename
				
				for (int p=0; p<MAXPLAYERLINKS; p++) bulkbay->addToTxBuffer(p, FOCUS, n); // And out to all!
				
				enlight(dungeon.at(actualmap), p, radius);
				repaint();
			}
			dragstartpos = event->pos(); // Memorize clickpoint anyway, so dragging and marking is possible at the same time
		}
	} else if (event->button() == Qt::RightButton) {
		// Right button opens dialog for choosing dungeon - only allowed for gamemaster
		if (mode == DV_DISPLAY && ctrl->mode.get() == MODE_MASTER) mode = DV_SELECT; else mode = DV_DISPLAY;
		repaint();
	}
	event->accept();
}

void DungeonViewer::mouseMoveEvent(QMouseEvent * event)
{
	if (actualmap < 0) return;
	if (QGuiApplication::queryKeyboardModifiers()) return; // No drag when filling or enlighting - looks weird...
	if (event->buttons() & Qt::LeftButton) {
		qDebug() << "DungeonViewer: Mouse drag";
		assert(dungeon.size()>=actualmap);
		centerx = limit(centerx - event->pos().x() + dragstartpos.x(), 0, dungeon.at(actualmap).map->width());
		centery = limit(centery - event->pos().y() + dragstartpos.y(), 0, dungeon.at(actualmap).map->height());
		dragstartpos = event->pos();
		repaint();
	}
	event->accept();
}

void DungeonViewer::wheelEvent(QWheelEvent * event)
{
	qDebug("DungeonViewer: Mouse Wheel");
	Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
	assert(dungeon.size()>=actualmap);

	if (modifiers & Qt::ShiftModifier) centerx = limit(centerx - event->angleDelta().y(), 0, dungeon.at(actualmap).map->width());
	if (!modifiers)      		       centery = limit(centery - event->angleDelta().y(), 0, dungeon.at(actualmap).map->height());
	repaint();
	event->accept();
}

void DungeonViewer::focusEvent(QByteArray data)
{
	if (data.size()<1) {qWarning("DungeonViewer: Received a too short focus event"); return;}
	if (data.at(0) != FOCUS_DUNGEON) return; // Not for us!
	if (data.size()<7) {qWarning("DungeonViewer: Received a too short focus event"); return;}
	parent->setCurrentWidget(this);
	char variant     = data.at(1);
	int newx    = ((uint8_t)data.at(2) << 8) + (uint8_t)data.at(3);
	int newy    = ((uint8_t)data.at(4) << 8) + (uint8_t)data.at(5);
	int radius  = ((uint8_t)data.at(6) << 8) + (uint8_t)data.at(7);
	QString filename = data.mid(8);
	qDebug() << "DungeonViewer: Received focus event of type" << (uint16_t)variant <<"for coordinates" << newx << newy << "with radius" << radius << "and for map" << filename;
	// Search for the correct dungeon map
	int newmap = -1;
	for (int i=0; i<dungeon.size(); i++) if (dungeon.at(i).name == filename) newmap = i;
	if (newmap == -1) qWarning("DungeonViewer: Cannot find map");
	else {
		if (newmap != actualmap) {targetx = centerx = newx; targety = centery = newy; actualmap = newmap;} // When we change the map start with the received coordinates
		else {targetx = newx; targety = newy;}
		if (variant & DV_EXCLUSIVE) setAlpha(dungeon.at(actualmap).map, 0x01); // Exclusive? Clear the alpha map
		if (variant & DV_ENLIGHT) enlight(dungeon.at(actualmap), QPoint(newx, newy), radius);
		if (variant & DV_FILL) fill(dungeon.at(actualmap), QPoint(newx, newy));
		repaint();
		timer->start();
	}
}

void DungeonViewer::timerTick()
{
	qDebug("DungeonViewer: Timer tick");
	int deltax = targetx - centerx;
	int deltay = targety - centery;
	float dist = sqrt(sqr((float)deltax) + sqr((float)deltay));
	if (dist < DV_SCROLLSTEP) {
		// When we're near enough, snap directly to the point
		timer->stop();
		centerx = targetx; 
		centery = targety;
		repaint();
	}
	else {
		centerx += deltax * DV_SCROLLSTEP / dist;
		centery += deltay * DV_SCROLLSTEP / dist;
		repaint();
	}

}