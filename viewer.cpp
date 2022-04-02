#include "viewer.h"

Viewer::Viewer(QString filename, QWidget * parent) : QWidget(parent)
{
	this->filename = filename;
	myname = filename;
	myname.replace(".png",""); myname.replace(".jpg",""); myname.replace(".JPG",""); myname.replace(".PNG","");
	qDebug() << "Viewer: Generating new viewer for filename" << filename;
	//map = new QImage();
	map = QImage();
	overlay = NULL;
	wall = NULL;
	//art = new QImage();
	art = QImage();
	assert (MAXTEAMS <= MAXMARKERS);
	for (int i=0; i<MAXMARKERS; i++) {
		// Generate a buch of markers at once so we do not have to keep track of creation and deletion 
		marker[i] = new Marker(i, this);
		connect(marker[i], SIGNAL(dragged(uint8_t)), this, SLOT(markerDragged(uint8_t)));
		connect(marker[i], SIGNAL(deleted(uint8_t)), this, SLOT(markerDeleted(uint8_t)));
	}
	// Setup actions for context menu
	focusAct = new QAction(QIcon("icons/focus.png"),"Focus Here");
	connect(focusAct, SIGNAL(triggered(bool)), this, SLOT(focusRequested(bool)));
	
	zoomToFitAct= new QAction("Zoom To Fit");
	connect(zoomToFitAct, SIGNAL(triggered(bool)), this, SLOT(zoomToFitRequested(bool)));
	
	allowScrollingAct = new QAction("Allow Scrolling");
	allowScrollingAct->setCheckable(true);
	connect(allowScrollingAct, SIGNAL(changed()), this, SLOT(allowScrollingChanged()));
	
	allowZoomingAct = new QAction("Allow Zooming");
	allowZoomingAct->setCheckable(true);
	connect(allowZoomingAct, SIGNAL(changed()), this, SLOT(allowZoomingChanged()));
	
	showTeamMarkerAct = new QAction("Show Team Marker");
	showTeamMarkerAct->setCheckable(true);
	connect(showTeamMarkerAct, SIGNAL(changed()), this, SLOT(showTeamMarkersChanged()));
	
	showOtherTeamsAct = new QAction("Show Other Teams");
	showOtherTeamsAct->setCheckable(true);
	connect(showOtherTeamsAct, SIGNAL(changed()), this, SLOT(showOtherTeamsChanged()));
	
	removeMarkersAct = new QAction("Remove All Markers");
	connect(removeMarkersAct, SIGNAL(triggered(bool)), this, SLOT(removeMarkers(bool)));
	
	markerSelection = new MarkerSelection(this);
	markerMenu = new QMenu("Add New Marker");
	markerMenu->addAction(markerSelection);
	connect(markerSelection, SIGNAL(newMarker(int, int)), this, SLOT(newMarker(int, int)));

	for (int t=0; t<MAXTEAMS; t++) {
		// Everything for the viewmode
		viewmodeGroup[t] = new QActionGroup(this);
		viewmodeMenu[t] = new QMenu("Set View Mode");
		for (int v=0; v<VIEWMODES; v++) {
			viewmodeAct[t][v] = new QAction(viewmodeName[v]);
			viewmodeAct[t][v]->setCheckable(true);
			viewmodeGroup[t]->addAction(viewmodeAct[t][v]);
			viewmodeMenu[t]->addAction(viewmodeAct[t][v]);
		}
		connect(viewmodeGroup[t], SIGNAL(triggered(QAction *)), this, SLOT(viewmodeSet(QAction *)));
		// Everything for the radius
		radiusMenu[t] = new QMenu("Set View Radius");
		radiusGroup[t] = new QActionGroup(this);
		for (int r=0; r<RADII; r++) {
			radiusAct[t][r] = new QAction(QString::number(radiusSize[r])+"px");
			radiusAct[t][r]->setCheckable(true);
			radiusMenu[t]->addAction(radiusAct[t][r]);
			radiusGroup[t]->addAction(radiusAct[t][r]);
		}
		connect(radiusGroup[t], SIGNAL(triggered(QAction *)), this, SLOT(radiusSet(QAction *)));
		marker[t]->team = t; // The first TEAM markers are reserved as team markes
		marker[t]->active = true;
		marker[t]->setStyle(MARKER_TEAM, t);
	}
	clearConfig();
	loadImage();
	loadConfig();
	adjustMenu();
	QCursor cursor;
	cursor.setShape(Qt::OpenHandCursor);
	setCursor(cursor);
	connect(ctrl, SIGNAL(shutDown()), this, SLOT(shutDown()));
	connect(ctrl, SIGNAL(teamChange()), this, SLOT(teamChange()));
	connect(ctrl, SIGNAL(fileUpdate(int, QString)), this, SLOT(fileUpdate(int, QString)));
	connect(ctrl, SIGNAL(connectionStateChange(int)), this, SLOT(connectionStateChange(int)));
	//qDebug("Viewer: Constructor done!");
	connect(&timer, SIGNAL(timeout()), this, SLOT(transmit()));
}

void Viewer::adjustMenu()
// Make the menu settings correspond to the status variables
{
	qDebug("Viewer: Adjusting menu");
	allowScrollingAct->setChecked(allowScrolling);
	allowZoomingAct->setChecked(allowZooming);
	showTeamMarkerAct->blockSignals(true); // Otherwise activating the team marker checkbox would reset the marker position
	showTeamMarkerAct->setChecked(showTeamMarkers);
	showTeamMarkerAct->blockSignals(false);
	showOtherTeamsAct->setChecked(showOtherTeams);
	for (int t=0; t<MAXTEAMS; t++) {
		assert(radiusStep[t] < RADII);
		assert(radiusStep[t] >=0 );
		assert(viewmode[t] < VIEWMODES);
		assert(viewmode[t] >=0 );
		radiusAct[t][radiusStep[t]]->setChecked(true);
		viewmodeAct[t][viewmode[t]]->setChecked(true);
	}
}
	
QString Viewer::getName()
{
	return myname;
}

QString Viewer::getFilename()
{
	return filename;
}

bool Viewer::isVisible()
{
	return viewmode[ctrl->team.get()] != VIEW_HIDDEN;
}

void Viewer::broadcast(QByteArray data)
{
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		if (ctrl->connectionStatus.get(p) == CON_READY) buffer[p] += data;
	}
}

void Viewer::render()
{
	// FIXME: IN the long run try to seperate redering of the effect and scaling/zooming into two functions,
	// since it is not nesseceary to always do both but take care of a team change race condition!
	int team = ctrl->team.get(); // Local copy of actual team - don't want it to change while painting
	int mode = ctrl->mode.get(); // Same for mode
	qDebug("Viewer: Rendering for team %c", 'A'+team);
	assert(team>=0); 
	assert(team<MAXTEAMS);
	assert(zoom[team] > 0);

	//if (map->isNull()) {*art = QImage(); return;} // Do no perfom any scaling for a null-image
	if (map.isNull()) {art = QImage(); return;} // Do no perfom any scaling for a null-image
	qDebug("Viewer: Zoom was: %2.5f, zoomcenter was: %f/%f", zoom[team], zoomxcenter[team], zoomycenter[team] );

	if (ctrl->mode.get() != MODE_MASTER && !allowScrolling && showTeamMarkers) {
		// When scrolling is forbidden, the position of the team marker indicates the zoom center
		zoomxcenter[team] = marker[team]->getPos().x();
		zoomycenter[team] = marker[team]->getPos().y();
	}
	float minZoom = fmin((float)height()/(float)map.height(), (float) width() / (float)map.width());
	limitinplace(&zoom[team], minZoom, 2.0f);
	limitinplace(&zoomycenter[team], 0.0f, map.height() - 1.0f); 
	limitinplace(&zoomxcenter[team], 0.0f, map.width() - 1.0f); 

	qDebug("Viewer: Zoom is now: %2.5f, zoomcenter is: %f/%f", zoom[team], zoomxcenter[team], zoomycenter[team] );
	
	for (int m=0; m<MAXMARKERS; m++) {
		if (m<MAXTEAMS) marker[m]->active = showTeamMarkers;
		if (marker[m]->active) {
			marker[m]->updateBorder(map.width(), map.height());
			marker[m]->updateZoom(zoom[team], zoomxcenter[team], zoomycenter[team]);
			marker[m]->updatePosition();
		}
	}
	switch(viewmode[team]) {
		case VIEW_HIDDEN:           renderViewHidden(team); break; //Hidden, but not for the gamemaster
		case VIEW_NORMAL:           renderViewNormal(team); break;
		case VIEW_FOG:				renderViewRadius(team, Qt::white, mode); break;
		case VIEW_DARKNESS:			renderViewRadius(team, Qt::black, mode); break;
		case VIEW_ACCESSIBLE:       renderViewAccessible(team, mode); break;
		case VIEW_VISIBLE:			renderViewVisible(team, mode); break;
		case VIEW_VISIBLERADAR:     renderViewVisibleWithRadar(team, mode); break;
		case VIEW_VISIBLEBLUEPRINT: renderViewVisibleWithBlueprint(team, mode); break;
	}
}

void Viewer::renderViewHidden(int team)
{
	// For the players this image is not diplayed, but for the gamemaster we will will identify that it is hidden
	int cutx1 = limit(int(zoomxcenter[team] - width()/2.0/zoom[team]),  0, map.width() - 1);
	int cutx2 = limit(int(zoomxcenter[team] + width()/2.0/zoom[team]),  0, map.width() - 1);
	int cuty1 = limit(int(zoomycenter[team] - height()/2.0/zoom[team]), 0, map.height() - 1);
	int cuty2 = limit(int(zoomycenter[team] + height()/2.0/zoom[team]), 0, map.height() - 1);

	QImage cutout = map.copy(cutx1, cuty1, cutx2 - cutx1, cuty2 - cuty1);
	for (int y = 0; y<cutout.height();  y++) {
		uchar * img = cutout.scanLine(y);
		for (int x = 0; x<cutout.width(); x++) {
			if ((x+y+cutx1+cuty1) % 100 > 50) {
				(*img) = (uchar)(min((*img) + 20, 255)); img++;
				(*img) = (uchar)(min((*img) + 20, 255)); img++;
				(*img) = (uchar)(min((*img) + 20, 255)); img++;
			} else {
				(*img) = (uchar)(max((*img) - 20, 0)); img++;
				(*img) = (uchar)(max((*img) - 20, 0)); img++;
				(*img) = (uchar)(max((*img) - 20, 0)); img++;
				
			}
		}
	}
	art = cutout.scaledToHeight(int(cutout.height() * zoom[team]), Qt::SmoothTransformation);	
	// And adjust the markers
	for (int i=0; i<MAXMARKERS; i++) {
		if (marker[i]->active && (marker[i]->team == team || showOtherTeams)) marker[i]->show();
		else marker[i]->hide();
	}
}

void Viewer::renderViewNormal(int team)
{
	// Now cut out the proper segment of the original image
	int cutx1 = limit(int(zoomxcenter[team] - width()/2.0/zoom[team]),  0, map.width() - 1);
	int cutx2 = limit(int(zoomxcenter[team] + width()/2.0/zoom[team]),  0, map.width() - 1);
	int cuty1 = limit(int(zoomycenter[team] - height()/2.0/zoom[team]), 0, map.height() - 1);
	int cuty2 = limit(int(zoomycenter[team] + height()/2.0/zoom[team]), 0, map.height() - 1);
	QImage cutout = map.copy(cutx1, cuty1, cutx2 - cutx1, cuty2 - cuty1);
	art = cutout.scaledToHeight(int(cutout.height() * zoom[team]), Qt::SmoothTransformation);	
	// And adjust the markers
	for (int i=0; i<MAXMARKERS; i++) {
		if (marker[i]->active && (marker[i]->team == team || showOtherTeams)) {
			marker[i]->updateZoom(zoom[team], zoomxcenter[team], zoomycenter[team]);
			marker[i]->updatePosition();
			marker[i]->show();
		}
		else marker[i]->hide();
	}
}

void Viewer::renderViewRadius(int team, QColor color, int mode) 
{
	qDebug("Viewer: Rendering radius");
	// Now cut out the proper segment of the original image
	int cutx1 = limit(int(zoomxcenter[team] - width()/2.0/zoom[team]),  0, map.width()-1);
	int cutx2 = limit(int(zoomxcenter[team] + width()/2.0/zoom[team]),  0, map.width()-1);
	int cuty1 = limit(int(zoomycenter[team] - height()/2.0/zoom[team]), 0, map.height()-1);
	int cuty2 = limit(int(zoomycenter[team] + height()/2.0/zoom[team]), 0, map.height()-1);
	//if (ctrl->mode.get() == MODE_MASTER) 
	//qDebug("Viewer: Available size of overlay: %d", malloc_usable_size((void*)overlay));
	//qDebug("Viewer: Map contains of %d pixels", map->width() * map->height());
	//return;
	QImage cutout = map.copy(cutx1, cuty1, cutx2 - cutx1, cuty2 - cuty1); // FIXME: Do cutting and multiplying in one step
	// Multiply the overlay with the image
	int xpos = marker[team]->xpos;
	int ypos = marker[team]->ypos;
	int dx, dy;
	int radius = radiusSize[radiusStep[team]];
	int alpha;
	int minalpha; 
	if (mode == MODE_MASTER) minalpha = MINBRIGHT; else minalpha = 0;
	int r = color.red();
	int g = color.green();
	int b = color.blue();
	for (int y = 0; y<cutout.height();  y++) {
		dy = y + cuty1 - ypos;
		uchar * img = cutout.scanLine(y);
		for (int x = 0; x<cutout.width(); x++) {
			dx = x + cutx1 - xpos;
			if ((abs(dx) > radius) || (abs(dy) > radius) || !showTeamMarkers) alpha = minalpha; // To speed things up
			else alpha = (int)(limit(2.0-2.0*sqrt(sqr(dx) + sqr(dy))/(float)radius, minalpha/256.0, 1.0) * 256.0);
			(*img) = (uchar) (((*img) * alpha + r * (256-alpha)) >> 8); img++;
			(*img) = (uchar) (((*img) * alpha + g * (256-alpha)) >> 8); img++;
			(*img) = (uchar) (((*img) * alpha + b * (256-alpha)) >> 8); img++;
		}
	}
	art = cutout.scaledToHeight(int(cutout.height() * zoom[team]), Qt::SmoothTransformation);
	for (int i=0; i<MAXMARKERS; i++) {
		if (marker[i]->active) {
			marker[i]->updateZoom(zoom[team], zoomxcenter[team], zoomycenter[team]);
			marker[i]->updatePosition();
			int x = marker[i]->getPos().x();
			int y = marker[i]->getPos().y();
			assert(x>=0); assert(x<map.width()); assert(y>=0); assert(y<map.height());
			if (sqr(x-xpos) + sqr(y-ypos) < sqr(radius) || mode == MODE_MASTER) { 
				// When we are master or the marker is visible anyway, so show it
				if (marker[i]->active && (marker[i]->team == team || showOtherTeams)) marker[i]->show();
				else marker[i]->hide();
			} else marker[i]->hide();
		} else marker[i]->hide();
	}
}

void Viewer::renderViewAccessible(int team, int mode)
{
	qDebug("Viewer: Rendering whole accessible area");
	// Now cut out the proper segment of the original image
	//if (ctrl->mode.get() == MODE_MASTER) 
	assert(overlay != NULL);
	//qDebug("Viewer: Available size of overlay: %d", malloc_usable_size((void*)overlay));
	//qDebug("Viewer: Map contains of %d pixels", map->width() * map->height());
	//return;
	memset((void*)overlay, 0, map.width() * map.height() * sizeof(uint16_t));
	//else memset((void*)overlay, 64, map->width() * map->height());
	//floodfill(overlay, wall, map->width(), map->height(), marker[team]->getPos(), 1000);
	if (showTeamMarkers) floodfill(overlay, wall, map.width(), map.height(), marker[team]->getPos(), 2000);
	int cutx1 = limit(int(zoomxcenter[team] - width()/2.0/zoom[team]),  0, map.width() - 1);
	int cutx2 = limit(int(zoomxcenter[team] + width()/2.0/zoom[team]),  0, map.width() - 1);
	int cuty1 = limit(int(zoomycenter[team] - height()/2.0/zoom[team]), 0, map.height() - 1);
	int cuty2 = limit(int(zoomycenter[team] + height()/2.0/zoom[team]), 0, map.height() - 1);
	QImage cutout = map.copy(cutx1, cuty1, cutx2 - cutx1, cuty2 - cuty1); // FIXME: Do cutting and multilying in one step
	// Multiply the overlay with the image
	int minbright = 0;
	if (mode == MODE_MASTER) minbright = MINBRIGHT;
	for (int y = 0; y<cutout.height();  y++) {
		uchar * img = cutout.scanLine(y);
		uint16_t * oly = &overlay[(y+cuty1) * map.width() + cutx1];
		for (int x = 0; x<cutout.width(); x++) {
			(*img) = (*img) * max((*oly), minbright) >> 8; img++;
			(*img) = (*img) * max((*oly), minbright) >> 8; img++;
			(*img) = (*img) * max((*oly), minbright) >> 8; img++; oly++;
		}
	}
	// FIXME: Do the overlay here
	art = cutout.scaledToHeight(int(cutout.height() * zoom[team]), Qt::SmoothTransformation);
	// And adjust the markers
	for (int i=0; i<MAXMARKERS; i++) {
		int x = marker[i]->getPos().x();
		int y = marker[i]->getPos().y();
		assert(x>=0); assert(x<map.width()); assert(y>=0); assert(y<map.height());
		if (overlay[y*map.width()+x] > 0 || mode == MODE_MASTER) { 
			// When we are master or the marker is visible anyway show it
			if (marker[i]->active && (marker[i]->team == team || showOtherTeams)) marker[i]->show();
			else marker[i]->hide();
		} else marker[i]->hide();
	}
}

void Viewer::renderViewVisible(int team, int mode)
{
	assert(team < MAXTEAMS);
	qDebug("Viewer: Rendering whole visible area");
	// Now cut out the proper segment of the original image
	int cutx1 = limit(int(zoomxcenter[team] - width()/2.0/zoom[team]),  0, map.width()-1);
	int cutx2 = limit(int(zoomxcenter[team] + width()/2.0/zoom[team]),  0, map.width()-1);
	int cuty1 = limit(int(zoomycenter[team] - height()/2.0/zoom[team]), 0, map.height()-1);
	int cuty2 = limit(int(zoomycenter[team] + height()/2.0/zoom[team]), 0, map.height()-1);
	//if (ctrl->mode.get() == MODE_MASTER) 
	//qDebug("Viewer: Available size of overlay: %d", malloc_usable_size((void*)overlay));
	//qDebug("Viewer: Map contains of %d pixels", map->width() * map->height());
	//return;
	assert(malloc_usable_size((void*)overlay) >= map.width() * map.height() * sizeof(uint16_t));
	memset((void*)overlay, 0, map.width() * map.height() * sizeof(uint16_t));
	if (showTeamMarkers) {
		illuminate(overlay, wall, map.width(), map.height(), marker[team]->getPos(), radiusSize[radiusStep[team]]);
		leeFromImage(overlay, wall, map.width(), map.height(), 20);
	}
	QImage cutout = map.copy(cutx1, cuty1, cutx2 - cutx1, cuty2 - cuty1); // FIXME: Do cutting and multiplying in one step
	qDebug("Viewer: art is at address: %lx, cutout is at: %lx", (long int)(&art), (long int)(&cutout));
	// Multiply the overlay with the image
	int minbright = 0;
	if (mode == MODE_MASTER) minbright = MINBRIGHT;
	uchar * img2 = cutout.scanLine(0);
	img2 += malloc_usable_size((void*)img2);

	qDebug("Viewer: art is at address: %lx, cutout is at: %lx", (long int)(&art), (long int)(&cutout));

	for (int y = 0; y<cutout.height();  y++) {
		uchar * img = cutout.scanLine(y);
//		qDebug("Viewer: y:%d, usable size: %d, widhtbyheight: %d", y, malloc_usable_size((void*)img), cutout.width()*cutout.height()*3);
//		assert(img != NULL);
		uint16_t * oly = &overlay[(y+cuty1) * map.width() + cutx1];
		bool * wl      = &wall   [(y+cuty1) * map.width() + cutx1];
		for (int x = 0; x<cutout.width(); x++) {
			int b = max((*oly), minbright);
			(*img) = (uchar) ((*img) * b >> 8); img++;
			(*img) = (uchar) ((*img) * b >> 8); img++;
			(*img) = (uchar) ((*img) * b >> 8); img++;
			assert(img <= img2);
			oly++;
			wl++;
		}
	}
	qDebug("Viewer: Scaling...");
	qDebug("Viewer: art is at address: %lx, cutout is at: %lx", (long int)(&art), (long int)(&cutout));
	art = cutout.scaledToHeight(int(cutout.height() * zoom[team]), Qt::SmoothTransformation);
	qDebug("Viewer: Overlaying...");
	//assert(art != NULL);
	for (int i=0; i<MAXMARKERS; i++) {
		if (marker[i]->active) {
			int x = marker[i]->getPos().x();
			int y = marker[i]->getPos().y();
			assert(x>=0); assert(x<map.width()); assert(y>=0); assert(y<map.height());
			if (overlay[y*map.width()+x] > 0 || mode == MODE_MASTER) { 
				// When we are master or the marker is visible anyway show it
				if (marker[i]->team == team || showOtherTeams) marker[i]->show();
				else marker[i]->hide();
			} else marker[i]->hide();
		} else marker[i]->hide();
	}
	qDebug("Viewer: Rendering done");
}

void Viewer::renderViewVisibleWithRadar(int team, int mode)
{
	static unsigned int seed = 0;
	int radius = radiusSize[radiusStep[team]];
	qDebug("Viewer: Rendering whole visible area with radar overlay");
	// Now cut out the proper segment of the original image
	int cutx1 = limit(int(zoomxcenter[team] - width()/2.0/zoom[team]),  0, map.width()-1);
	int cutx2 = limit(int(zoomxcenter[team] + width()/2.0/zoom[team]),  0, map.width()-1);
	int cuty1 = limit(int(zoomycenter[team] - height()/2.0/zoom[team]), 0, map.height()-1);
	int cuty2 = limit(int(zoomycenter[team] + height()/2.0/zoom[team]), 0, map.height()-1);
	//if (ctrl->mode.get() == MODE_MASTER) 
	//qDebug("Viewer: Available size of overlay: %d", malloc_usable_size((void*)overlay));
	//qDebug("Viewer: Map contains of %d pixels", map->width() * map->height());
	//return;
	memset((void*)overlay, 0, map.width() * map.height() * sizeof(uint16_t));
	if (showTeamMarkers) {
		illuminate(overlay, wall, map.width(), map.height(), marker[team]->getPos(), 1000);
		leeFromImage(overlay, wall, map.width(), map.height(), 20);
	}
	QImage cutout = map.copy(cutx1, cuty1, cutx2 - cutx1, cuty2 - cuty1); // FIXME: Do cutting and multilying in one step
	// Multiply the overlay with the image
	int minbright = 0;
	if (mode == MODE_MASTER) minbright = MINBRIGHT;
	int xpos = marker[team]->getPos().x() - cutx1;
	int ypos = marker[team]->getPos().y() - cuty1;
	if (showTeamMarkers) {
		for (int y = 0; y<cutout.height();  y++) {
			uchar * img = cutout.scanLine(y);
			uint16_t * oly = &overlay[(y+cuty1) * map.width() + cutx1];
			bool * wl      = &wall   [(y+cuty1) * map.width() + cutx1];
			for (int x = 0; x<cutout.width(); x++) {
				int b = max((*oly), minbright);
				(*img) = (uchar) ((*img) * b >> 8); img++;
				(*img) = (uchar) ((*img) * b >> 8);
				assert(sqr(x - xpos) + sqr(y  - ypos) >=0);
				int r = sqrt(sqr(x - xpos) + sqr(y  - ypos));
				if (r < radius) { 
					int bright = min((radius - r) * 256 / radius, 192);
					assert(bright>=0);
					assert(bright<=192);
					if ((*wl)) (*img) = (uchar)(rand_r(&seed) * (long) bright / RAND_MAX);
					else if (*oly == 0) (*img) = (uchar)max((*img), (rand_r(&seed) * (long)  bright / RAND_MAX) / 2);
				}
				img++;
				(*img) = (uchar) ((*img) * b >> 8); img++;
				oly++;
				wl++;
			}
		}
	}
	qDebug("Viewer: Rendering done");
	xpos = marker[team]->getPos().x();
	ypos = marker[team]->getPos().y();
	art = cutout.scaledToHeight(int(cutout.height() * zoom[team]), Qt::SmoothTransformation);
	for (int i=0; i<MAXMARKERS; i++) {
		if (marker[i]->active) {
			marker[i]->updateZoom(zoom[team], zoomxcenter[team], zoomycenter[team]);
			marker[i]->updatePosition();
			int x = marker[i]->getPos().x();
			int y = marker[i]->getPos().y();
			assert(x>=0); assert(x<map.width()); assert(y>=0); assert(y<map.height());
			if (overlay[y*map.width()+x] > 0 || sqrt(sqr(x - xpos)+sqr(y - ypos)) < radius || mode == MODE_MASTER) { 
				// When we are master or the marker is visible anyway show it
				if (marker[i]->active && (marker[i]->team == team || showOtherTeams)) marker[i]->show();
				else marker[i]->hide();
			} else marker[i]->hide();
		} else marker[i]->hide();
	}
	qDebug("Viewer: Updating Markers done");
}

void Viewer::renderViewVisibleWithBlueprint(int team, int mode)
{
	qDebug("Viewer: Rendering whole accessible area with radar overlay");
	// Now cut out the proper segment of the original image
	int cutx1 = limit(int(zoomxcenter[team] - width()/2.0/zoom[team]),  0, map.width()-1);
	int cutx2 = limit(int(zoomxcenter[team] + width()/2.0/zoom[team]),  0, map.width()-1);
	int cuty1 = limit(int(zoomycenter[team] - height()/2.0/zoom[team]), 0, map.height()-1);
	int cuty2 = limit(int(zoomycenter[team] + height()/2.0/zoom[team]), 0, map.height()-1);
	//if (ctrl->mode.get() == MODE_MASTER) 
	//qDebug("Viewer: Available size of overlay: %d", malloc_usable_size((void*)overlay));
	//qDebug("Viewer: Map contains of %d pixels", map->width() * map->height());
	//return;
	memset((void*)overlay, 0, map.width() * map.height() * sizeof(uint16_t));
	if (showTeamMarkers) {
		illuminate(overlay, wall, map.width(), map.height(), marker[team]->getPos(), 2000);
		leeFromImage(overlay, wall, map.width(), map.height(), 20);
	}
	QImage cutout = map.copy(cutx1, cuty1, cutx2 - cutx1, cuty2 - cuty1); // FIXME: Do cutting and multilying in one step
	// Multiply the overlay with the image
	int minbright = 0;
	if (mode == MODE_MASTER) minbright = MINBRIGHT;

	for (int y = 0; y<cutout.height();  y++) {
		uchar * img = cutout.scanLine(y);
		uint16_t * oly = &overlay[(y+cuty1) * map.width() + cutx1];
		bool * wl      = &wall   [(y+cuty1) * map.width() + cutx1];
		for (int x = 0; x<cutout.width(); x++) {
			int b = max((*oly), minbright);
			int l;
			if ((*wl)) l = 0; else l = 255-b;
			(*img) = (uchar) (((*img) * b >> 8) + l); img++;
			(*img) = (uchar) (((*img) * b >> 8) + l); img++;
			(*img) = (uchar) (((*img) * b >> 8) + l); img++;
			oly++;
			wl++;
		}
	}
	art = cutout.scaledToHeight(int(cutout.height() * zoom[team]), Qt::SmoothTransformation);
	for (int i=0; i<MAXMARKERS; i++) {
		if (marker[i]->active) {
			marker[i]->updateZoom(zoom[team], zoomxcenter[team], zoomycenter[team]);
			marker[i]->updatePosition();
			int x = marker[i]->getPos().x();
			int y = marker[i]->getPos().y();
			assert(x>=0); assert(x<map.width()); assert(y>=0); assert(y<map.height());
			if (overlay[y*map.width()+x] > 0 || mode == MODE_MASTER) { 
				// When we are master or the marker is visible anyway show it
				if (marker[i]->active && (marker[i]->team == team || showOtherTeams)) marker[i]->show();
				else marker[i]->hide();
			} else marker[i]->hide();
		} else marker[i]->hide();
	}
}

void Viewer::paintEvent(QPaintEvent * event)
{
	qDebug() << "Viewer(" << myname << "): Paint event";
	qDebug() << "Viewer(" << myname << "): Window has a size of" << width() << "x" << height();
	QPainter painter(this);
	painter.beginNativePainting();
	painter.setRenderHint(QPainter::Antialiasing);
	QBrush brush(Qt::SolidPattern);
	if (viewmode[ctrl->team.get()] == VIEW_FOG) brush.setColor(Qt::white); else brush.setColor(Qt::black);
	painter.fillRect(0,0, width(), height(), brush);
	int team = ctrl->team.get();
	if (map.isNull()) {
		painter.setPen(Qt::red);
		painter.drawText(0, height()/2-10, width(), 20, Qt::AlignCenter, "Image not yet available!");
		event->accept();
		return;
	} 
	painter.drawImage(limit(width() / 2  - (int)(zoomxcenter[team] * zoom[team]), 0, width()  / 2), 
	                  limit(height() / 2 - (int)(zoomycenter[team] * zoom[team]), 0, height() / 2), art);
	// The markers take care of themselfes
	// FIXME: Draw all the markings
}

void Viewer::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::LeftButton) {
		qDebug("Viewer: Left mouse button pressed");
		dragstartpos = event->pos();
		QCursor cursor;
		cursor.setShape(Qt::ClosedHandCursor);
		setCursor(cursor);
	}
}

void Viewer::mouseReleaseEvent(QMouseEvent * event)
{
	QCursor cursor;
	cursor.setShape(Qt::OpenHandCursor);
	setCursor(cursor);
	event->accept();
}

void Viewer::mouseMoveEvent(QMouseEvent * event)
{
	// Drag to scroll
	// FIXME: Check whether we are in paint mode!
	if (event->buttons() & Qt::LeftButton) {
		int team = ctrl->team.get();
		assert(team>=0); assert(team<MAXTEAMS);
		qDebug() << "Viewer(" + myname + "): Mouse drag";
		zoomxcenter[team] -= float(event->pos().x() - dragstartpos.x())/zoom[team];
		zoomycenter[team] -= float(event->pos().y() - dragstartpos.y())/zoom[team];
		dragstartpos = event->pos();
		render();
		repaint();
		event->accept();	
	}
}

void Viewer::wheelEvent(QWheelEvent * event)
{
	qDebug("Viewer: Mouse Wheel");
	Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
	qDebug("Viewer: Mouse scrolled %d degree in y", event->angleDelta().y());
	qDebug("Viewer: Mouse scrolled %d degree in x", event->angleDelta().x());
	qDebug() << "Viewer: Mouse position while scrolling" << mapFromGlobal(QCursor::pos());
	float deltax = mapFromGlobal(QCursor::pos()).x() - width()/2.0;
	float deltay = mapFromGlobal(QCursor::pos()).y() - height()/2.0;
	int team = ctrl->team.get();
	assert(team>=0); assert(team<MAXTEAMS);

	if (modifiers & Qt::ControlModifier) {
		qDebug("Viewer: Scroll with Ctrl means zoom");
		if (!allowZooming && ctrl->mode.get() != MODE_MASTER) return; // Zooming not allowed!
		float minzoom = fmin(float(width())/float(map.width()), float(height())/float(map.height())); // Zoomlevel that is needed to scale the picture so that it can be displayed completely
		if (event->angleDelta().y() > 0) {
			zoom[team] *= (1.0+event->angleDelta().y()/240.0);
			if (!limitinplace(&zoom[team], minzoom, 2.0f)) {
				zoomycenter[team] += deltay/2 / zoom[team];
				zoomxcenter[team] += deltax/2 / zoom[team];
			}
		}
		else {
			if (zoom[team]>minzoom) {
				zoomycenter[team] -= deltay/2 / zoom[team];
				zoomxcenter[team] -= deltax/2 / zoom[team];
				zoom[team] /= (1.0-event->angleDelta().y()/240.0);
				limitinplace(&zoom[team], minzoom, 2.0f);
			}
		}
		if (!allowZooming) {
			// When the player must not zoom, send the zoom level of the gamemaster
			broadcast(toNBO((uint8_t)SETZOOM) + toNBO((uint8_t)team) + toNBO(zoom[team]));
		}
	} else if (modifiers & Qt::AltModifier) {
		qDebug("Viewer: Scroll with Alt - unused");
	} else if (modifiers & Qt::ShiftModifier) {
		qDebug("Viewer: Scroll with Shift");
		if (!allowScrolling && ctrl->mode.get() != MODE_MASTER) return; // Scrolling not allowed!
		zoomxcenter[team] += float(event->angleDelta().y())/3.0/zoom[team];
	} else {
		qDebug("Viewer: Normal scroll");
		if (!allowScrolling && ctrl->mode.get() != MODE_MASTER) return; // Scrolling not allowed!
		zoomycenter[team] -= float(event->angleDelta().y())/3.0/zoom[team];
		zoomxcenter[team] += float(event->angleDelta().x())/3.0/zoom[team];
	}
	render();
	repaint();
	event->accept();	
}

void Viewer::resizeEvent(QResizeEvent * event)
{
	render();
	event->accept();
}

void Viewer::contextMenuEvent(QContextMenuEvent *event)
{
	qDebug() << "Viewer(" << myname << "): Opening context menu";
	if (ctrl->mode.get() != MODE_MASTER) return; //Sorry no need for a player context menu at this time
	int team = ctrl->team.get();

	QMenu menu;
	menu.addAction(focusAct);
	menu.addAction(zoomToFitAct);
	menu.addSeparator();
	menu.addAction(allowScrollingAct);
	menu.addAction(allowZoomingAct);
	menu.addSeparator();
	menu.addAction(showTeamMarkerAct);
	menu.addAction(showOtherTeamsAct);
	menu.addSeparator();
	menu.addMenu(markerMenu);
	menu.addAction(removeMarkersAct);
	menu.addSeparator();
	menu.addMenu(viewmodeMenu[team]);
	menu.addMenu(radiusMenu[team]);
	menu.addSeparator();
	//menu.addMenu(markingMenu);
	menuatx = event->x();
	menuaty = event->y();
	menu.exec(event->globalPos());
}

void Viewer::loadImage()
{
	qDebug() << "Viewer(" << myname << "): Trying to load my image.";
	if (map.load(ctrl->dir[DIR_PICS].filePath(filename))) {
		map = map.convertToFormat(QImage::Format_RGB888); // Just to be sure!
		assert(map.width() >0 && map.height() > 0);
//		qDebug("Viewer: Loaded map width %d, map height: %d", map->width(), map->height());
		overlay = (uint16_t *)realloc((void *)overlay, map.width() * map.height() * sizeof(uint16_t));
		assert(overlay!=NULL);
		wall = (bool *)realloc((void *)wall, map.width() * map.height() * sizeof(bool));
		assert(wall!=NULL);
		bool * w = wall;
		for (int y=0; y<map.height(); y++) {
			uchar * m = map.scanLine(y);
			for (int x=0; x<map.width(); x++) {
				// When a pixel is pitch black - that means there's a wall
				(*w) = (*m) == 0 && (*(m+1)) == 0 && (*(m+2)) == 0;	w++; m += 3;			
			}
		}
		// Generate default values in case the loading of the settings fails
		for (int t=0; t<MAXTEAMS; t++) {
			zoomxcenter[t] = map.width() / 2.0;
			zoomycenter[t] = map.height() / 2.0;
			zoom[t] = 1e-6;
		}
	} else {
		// Probably the image comes later...
	}
	// Tell all the markers that their playgroungd has changed
	for (int i=0; i<MAXMARKERS; i++) marker[i]->updateBorder(map.width(), map.height());
}

void Viewer::loadConfig()
{
	if (ctrl->mode.get() != MODE_MASTER) return; // Only the gamemaster loads the config file
	// The configuration is stores in the same binyra format as it it used for network communication - not easy to read, but compact
	QFile * file = new QFile(ctrl->dir[DIR_CONFIG].filePath(filename + ".cfg")); // Config files are stored in <imagename with suffix>.cfg
	if (!file->open(QIODevice::ReadOnly)) {
		qDebug("Viewer: Settings do not exist - starting with default values.");
		return;
	}
	file->seek(0); // FIXME: Unnessecairy?
	QByteArray config = file->readAll(); // Suck in all at once
	if (config.size() > 0) setConfig(&config); else qWarning("Viewer: Empty config file");
	file->close();
}

void Viewer::clearConfig()
{
	if (ctrl->mode.get() == MODE_MASTER) {
		timer.setSingleShot(false);
		timer.setInterval(100);
		timer.start();
	} else timer.stop();
	for (int t=0; t<MAXTEAMS; t++) {
		radiusStep[t] = 0; // Initial only - will be overwritten, when config file is read
		viewmode[t] = VIEW_HIDDEN; // Initial only - will be overwritten, when config file is read
	}
	allowZooming = false;
	allowScrolling = false;
	showOtherTeams = false;
	showTeamMarkers = false;
}

void Viewer::shutDown()
{
	if (ctrl->mode.get() == MODE_MASTER) saveConfig();
}

void Viewer::saveConfig()
{
	qDebug() << "Viewer(" << filename << ") saving settings";
	QFile * file = new QFile(ctrl->dir[DIR_CONFIG].filePath(filename + ".cfg")); // Config files are stored in <imagename with suffix>.cfg
	if (!file->open(QIODevice::WriteOnly)) {
		qWarning("Viewer: Cannot open file for config storage!");
		return;
	}
	file->write(getConfig());
	file->close();
}

QByteArray Viewer::getConfig()
// Returns a full configuration of the actual viewer state with all settings including markers for all teams
{
	QByteArray config;
	config += toNBO((uint8_t)CLEARALL); // First: Remember to clear everything
	//qDebug() << config.toHex();
	for (uint8_t t=0; t<MAXTEAMS; t++) {
		config += (toNBO((uint8_t)SETZOOM) + toNBO(t) + toNBO(zoom[t]));
		//qDebug() << config.toHex();
		config += (toNBO((uint8_t)SETCENTER) + toNBO(t) + toNBO(zoomxcenter[t]) + toNBO(zoomycenter[t]));
		//qDebug() << config.toHex();
		config += (toNBO((uint8_t)SETVIEWMODE) + toNBO(t) + toNBO(viewmode[t]) + toNBO(radiusStep[t]));
		//qDebug() << config.toHex();

	}
	// Add the rights
	config += (toNBO((uint8_t)SETRIGHTS) + toNBO(allowScrolling) + toNBO(allowZooming) +
               toNBO(showOtherTeams) + toNBO(showTeamMarkers));
//		qDebug() << config.toHex();
	// Add all markers
	for (uint8_t m=0; m<MAXMARKERS; m++) {
		if (marker[m]->active) {
			config += (toNBO((uint8_t) SETMARKER) + toNBO(m) + marker[m]->getConfig());
			//qDebug() << config.toHex();
		}
	}
	return config;
}

bool Viewer::setConfig(QByteArray * config)
// Sets the configuration by byte array, returns true if update of tab visibility is required - sorry, didn't want to use a signal for that
{
	// Now process all the commands chained in the byte array
	uint8_t c, t, m;
	bool updatevisibility = false; 
	while (config->size() > 0) {
		fromNBO(config, &c);
		switch (c) {
			case Viewer::CLEARALL:
				qDebug("    Parsing: CLEARALL");
				for (int i=0; i<MAXMARKERS; i++)  marker[i]->active = false;
				break;
			case Viewer::SETZOOM:
				qDebug("    Parsing: SETZOOM");
				if (!fromNBO(config, &t)) {qWarning("Viewer: Ran empty while setting zoom"); return false;}
				if (t>=MAXTEAMS) {qWarning("Viewer: Wrong team number for setting config"); return false;}
				assert(t < MAXTEAMS);
				if (!fromNBO(config, &zoom[t])) {
					qWarning("Viewer: Ran empty while setting zoom"); 
					return false;
				}
				break;
			case Viewer::SETCENTER:
				qDebug("    Parsing: SETCENTER");
				if (!fromNBO(config, &t)) {qWarning("Viewer: Ran empty while setting zoomcenter"); return false;}
				if (t>=MAXTEAMS) {qWarning("Viewer: Wrong team number for setting config"); return false;}
				assert(t < MAXTEAMS);
				if (!(fromNBO(config, &zoomxcenter[t]) && fromNBO(config, &zoomycenter[t]))) {
					qWarning("Viewer: Ran empty while setting zoomcenter"); 
					return false;
				}
				break;
			break;
			case Viewer::SETRIGHTS:
				qDebug("    Parsing: SETRIGHTS");
				if (!(fromNBO(config, &allowScrolling) && fromNBO(config, &allowZooming) && 
					  fromNBO(config, &showOtherTeams) && fromNBO(config, &showTeamMarkers))) {
					qWarning("Viewer: Ran empty while setting rights"); 
					return false;
				}
				break;
			case Viewer::SETMARKER:
				qDebug("    Parsing: SETMARKER");
				if (!fromNBO(config, &m)) {qWarning("Viewer: Ran empty while setting markers"); return false;}
				if (m>=MAXMARKERS) {qWarning("Viewer: Wrong marker number for setting config"); return false;}
				qDebug("    Parsing: Configuring marker Nr. %d", m);
				marker[m]->setConfig(config);
				qDebug() << "    Markerpos:" << marker[m]->getPos();
				break;
			case Viewer::SETVIEWMODE:
				qDebug("    Parsing: SETVIEWMODE");
				if (!fromNBO(config, &t)) {
					qWarning("Viewer: Ran empty while setting viewmode"); 
					return false;
				}
				assert (t<MAXTEAMS);
				if (!(fromNBO(config, &viewmode[t]) && fromNBO(config, &radiusStep[t]))) {
					qWarning("Viewer: Ran empty while setting viewmode"); 
					return false;
				}
				updatevisibility = true; // Not sure, but visibility may change by this action
				break;
			case Viewer::FOCUS:
				qDebug("    Parsing: FOCUS");
				// The proper zoom is set in another command, focus the actual team only
				if (!fromNBO(config, &t)) {qWarning("Viewer: Ran empty while setting viewmode"); return false;}
				if (t>=MAXTEAMS) {qWarning("Viewer: Wrong team number for focus"); return false;}
				if (t == ctrl->team.get()) emit(focusMe());
				break;
			case Viewer::SCALETOFIT:
				qDebug("    Parsing: SCALETOFIT");
				qDebug("Viewer: Scale to fit requested");
				if (!fromNBO(config, &t)) {qWarning("Viewer: Ran empty while scaling to fit"); return false;}
				if (t>=MAXTEAMS) {qWarning("Viewer: Wrong team number for scale to fit"); return false;}
				zoomxcenter[t] = map.width() / 2;
				zoomycenter[t] = map.height() / 2;
				zoom[t] = fmin((float)width() / (float) map.width(), (float)height() / (float)map.height());
				break;
		}
	}
	render();
	update(); // FIXME: update or repaint?
	return updatevisibility;
}

void Viewer::transmit()
{
	//qDebug("Viewer: Trying to transmit");
	if (ctrl->mode.get() != MODE_MASTER) return; // Player do not send anything - timer should not be active anyway
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		if (ctrl->connectionStatus.get(p) == CON_READY && buffer[p].size() > 0 && bulkbay->getBufferUsage(p) < BUF_HIGH_PRIO) {
			qDebug("Viewer: Transmitting buffer with %d elements", buffer[p].size());
			// Under these circumstanes we might have a chance to send out our buffer, so prepare it!
			assert(filename.length() < 256);
			QByteArray out = toNBO((uint8_t)filename.length()) + filename.toUtf8() + buffer[p];
			// If buffer filling was sucessful, clear the buffer, if not, try again next time
			if (bulkbay->addToTxBuffer(p, VIEWER, out, false, true) >=0) buffer[p].clear(); else qDebug("Viewer: Couls not send - trying again later!");
		}
	}
}

void Viewer::teamChange()
{
	render();
	update();
}

void Viewer::gamemasterChange()
{
	if (ctrl->mode.get() == MODE_MASTER) {
		// New rules for all!
		loadConfig();
		broadcast(getConfig());
		adjustMenu();
	} else {
		// Have been master are now player
		saveConfig();
		clearConfig();
		adjustMenu(); // Not required because player may not access context menu...		
	}
}

void Viewer::connectionStateChange(int player)
{
	assert(player < MAXPLAYERLINKS);
	assert(player>=0);
	if (ctrl->connectionStatus.get(player) == CON_READY) {
		qDebug("Viewer: Sending config to new player");
		// A new player is ready? Send the whole config.
		buffer[player] = getConfig();
	}
}

void Viewer::markerDragged(uint8_t id)
{
	qDebug("Viewer: Got drag event from marker %d", id);
	broadcast(toNBO((uint8_t) SETMARKER) + toNBO(id) + marker[id]->getConfig());
	render();
	repaint();
}

void Viewer::radiusSet(QAction * action)
{
	int team=-1;
	for (int t=0; t<MAXTEAMS; t++) {
		for (int r=0; r<RADII; r++) {
			if (action == radiusAct[t][r]) {
				radiusStep[t] = r;
				team = t;
				break;
			}
		}
	}
	if (team == -1) {qWarning("Viewer: Cannot find proper radius entry"); return;}
	// Go tell the others
	broadcast(toNBO((uint8_t)SETVIEWMODE) + toNBO((uint8_t)team) + toNBO(viewmode[team]) + toNBO(radiusStep[team]));
	qDebug("Viewer: Radius for team %d set to %d", team, radiusSize[radiusStep[team]]);
	render();
	update();
}

void Viewer::viewmodeSet(QAction * action)
{
	qDebug("Viewer: Setting view modes");
	int team = -1;
	for (int t=0; t<MAXTEAMS; t++) {
		for (int m=0; m<VIEWMODES; m++) {
			if (action == viewmodeAct[t][m]) {
				viewmode[t] = m;
				team = t;
				break;
			}
		}
	}
	if (team == -1) qWarning("Viewer: Signal not found");
	else qDebug() << "Viewer: Viewmode for team" << team << "set to" << viewmodeName[viewmode[team]];
	// Go tell the others
	broadcast(toNBO((uint8_t)SETVIEWMODE) + toNBO((uint8_t)team) + toNBO(viewmode[team]) + toNBO(radiusStep[team]));
	render();
	update();
}

void Viewer::newMarker(int style, int colorindex)
{
	qDebug("Viewer: New marker placed at %d/%d, style %d, color %d", menuatx, menuaty, style, colorindex);
	// Search for an unused marker slot
	uint8_t m;
	uint8_t t = ctrl->team.get();
	for (m=MAXTEAMS; m<MAXMARKERS; m++) {
		if (!marker[m]->active) {
			marker[m]->active = true;
			marker[m]->setStyle((MarkerStyle)style, colorindex);
			marker[m]->updateZoom(zoom[t], zoomxcenter[t], zoomycenter[t]);
			marker[m]->setRelPos(menuatx, menuaty); // The marker needs to know where the menu had been opened
			marker[m]->team = t;
			// Tell the others!
			broadcast(toNBO((uint8_t) SETMARKER) + toNBO(m) + marker[m]->getConfig());
			render();
			update();
			break;
		}
	}
	if (m == MAXMARKERS) qWarning("Viewer: All markers in use");
}

void Viewer::markerDeleted(uint8_t id)
{
	// Tell the others
	qDebug("Viewer: Marker %d deleted", id);
	broadcast(toNBO((uint8_t) SETMARKER) + toNBO(id) + marker[id]->getConfig());
	render();
	update();
}

void Viewer::allowScrollingChanged()
{
	qDebug("Viewer: allow scrolling changed");
	allowScrolling = allowScrollingAct->isChecked();
	sendRights();
}

void Viewer::allowZoomingChanged()
{
	qDebug("Viewer: allow zooming changed");
	allowZooming = allowZoomingAct->isChecked();
	sendRights();
	if (!allowZooming && ctrl->mode.get() == MODE_MASTER) {
		uint8_t team = ctrl->team.get();
		broadcast(toNBO((uint8_t)SETZOOM) + toNBO(team) + toNBO(zoom[team])); // When the user may not zoom, feed him ours
	}
}

void Viewer::showTeamMarkersChanged()
{
	qDebug("Viewer: show team marker changed");
	showTeamMarkers = showTeamMarkerAct->isChecked();
	uint8_t t = ctrl->team.get();
	if (showTeamMarkers) {
		// When a team marker is activated, place it at the menu position
		marker[t]->updateBorder(map.width(), map.height());
		marker[t]->updateZoom(zoom[t], zoomxcenter[t], zoomycenter[t]);
		marker[t]->updatePosition();
		marker[t]->setRelPos(menuatx, menuaty);
	}
	sendRights();
	broadcast(toNBO((uint8_t) SETMARKER) + toNBO(t) + marker[t]->getConfig());
	render();
	update();
}

void Viewer::showOtherTeamsChanged()
{
	qDebug("Viewer: show other teams changed");
	showOtherTeams = showOtherTeamsAct->isChecked();
	sendRights();
	render();
	//Update should not be required, right?
}

void Viewer::focusRequested(bool)
{
	qDebug("Viewer: Requesting focus");
	uint8_t t = ctrl->team.get();
	zoomxcenter[t] = (menuatx - width()/2) / zoom[t] + zoomxcenter[t];
	zoomycenter[t] = (menuaty - height()/2) / zoom[t] + zoomycenter[t];
	broadcast(toNBO((uint8_t)SETZOOM) + toNBO(t) + toNBO(zoom[t]) + 
			  toNBO((uint8_t)SETCENTER) + toNBO(t) + toNBO(zoomxcenter[t]) + toNBO(zoomycenter[t]) +
			  toNBO((uint8_t) Viewer::FOCUS) + toNBO(t)); 
	render();
	update();
}

void Viewer::zoomToFitRequested(bool)
{
	qDebug("Viewer: Sending zoom to fit");
	uint8_t t = ctrl->team.get();
	zoomxcenter[t] = map.width() / 2;
	zoomycenter[t] = map.height() / 2;
	zoom[t] = fmin((float)width() / (float) map.width(), (float)height() / (float)map.height());
	// When the players may not zoom, zoom and fit them!
	if (!allowZooming && !allowScrolling && ctrl->mode.get() == MODE_MASTER) broadcast(toNBO((uint8_t)SCALETOFIT) + toNBO(t));
	render();
	update();
}

void Viewer::removeMarkers(bool)
{
	qDebug("Viewer: Removing all markers");
	for (int i=0; i<MAXMARKERS; i++) marker[i]->active = false;
	render();
	update();	
}

void Viewer::sendRights()
{
	broadcast(toNBO((uint8_t)SETRIGHTS) + toNBO(allowScrolling) + toNBO(allowZooming) +
              toNBO(showOtherTeams) + toNBO(showTeamMarkers));
}

void Viewer::fileUpdate(int dir, QString changedfile)
{
	qDebug("Viewer: Processing file update");
	if (dir != DIR_PICS) return; // Not for us
	if (changedfile != filename) return; // Still not for us
	qDebug("Viewer: File is for us!");
	loadImage();
	render();
	update();
}