#include "sketchpad.h"
#include <math.h>
// Is there a way to do this less globally
QColor sketchColor[SKETCHCOLORS] = {
	QColor::fromRgb(0xFF, 0xFF, 0xFF), QColor::fromRgb(0xC0, 0xC0, 0xC0), QColor::fromRgb(0xA0, 0xA0, 0xA0), QColor::fromRgb(0x80, 0x80, 0x80), QColor::fromRgb(0x00, 0x00, 0x00), 
	QColor::fromRgb(0xFF, 0xA0, 0xA0), QColor::fromRgb(0xFF, 0x40, 0x40), QColor::fromRgb(0xFF, 0x00, 0x00), QColor::fromRgb(0xA0, 0x00, 0x00), QColor::fromRgb(0x80, 0x00, 0x00), 
	QColor::fromRgb(0xFF, 0xC0, 0xA0), QColor::fromRgb(0xFF, 0xA0, 0x40), QColor::fromRgb(0xFF, 0x80, 0x00), QColor::fromRgb(0xA0, 0x50, 0x00), QColor::fromRgb(0x80, 0x40, 0x00),
	QColor::fromRgb(0xFF, 0xFF, 0xA0), QColor::fromRgb(0xFF, 0xFF, 0x40), QColor::fromRgb(0xFF, 0xFF, 0x00), QColor::fromRgb(0xA0, 0xA0, 0x00), QColor::fromRgb(0x80, 0x80, 0x00), 
	QColor::fromRgb(0xA0, 0xFF, 0xA0), QColor::fromRgb(0x40, 0xFF, 0x40), QColor::fromRgb(0x00, 0xFF, 0x00), QColor::fromRgb(0x00, 0xA0, 0x00), QColor::fromRgb(0x00, 0x80, 0x00), 
	QColor::fromRgb(0xA0, 0xFF, 0xFF), QColor::fromRgb(0x40, 0xFF, 0xFF), QColor::fromRgb(0x00, 0xFF, 0xFF), QColor::fromRgb(0x00, 0xA0, 0xA0), QColor::fromRgb(0x00, 0x80, 0x80), 
	QColor::fromRgb(0xA0, 0xA0, 0xFF), QColor::fromRgb(0x40, 0x40, 0xFF), QColor::fromRgb(0x00, 0x00, 0xFF), QColor::fromRgb(0x00, 0x00, 0xA0), QColor::fromRgb(0x00, 0x00, 0x80), 
	QColor::fromRgb(0xFF, 0xA0, 0xFF), QColor::fromRgb(0xFF, 0x40, 0xFF), QColor::fromRgb(0xFF, 0x00, 0xFF), QColor::fromRgb(0xA0, 0x00, 0xA0), QColor::fromRgb(0x80, 0x00, 0x80) 
};

bool isInRadius(int x1, int y1, int x2, int y2, int r)
{
	if ( (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) <= r * r) return true;
	return false;
}

CourtYard::CourtYard()
{
	reset();
}

void CourtYard::reset()
{
	x1 = 0;
	y1 = 0;
	x2 = 0;
	y2 = 0;
}

void CourtYard::expand(int x, int y)
{
	x1 = min(x1, x);
	x2 = max(x2, x);
	y1 = min(y1, y);
	y2 = max(y2, y);
}

SketchTools::SketchTools(QWidget * parent) : QGroupBox(parent)
{
	int ypos = 0;
	resize(160, 400);
	setTitle("Toolbox");
	setAutoFillBackground(false);
	// Color Buttons
	colorButtonGroup = new QButtonGroup(this);
	QPixmap pixmap(20, 20);
	QPainter painter(&pixmap);
	for (int i=0; i<SKETCHCOLORS; i++) {
		colorButton[i] = new QPushButton(this);
		colorButton[i]->resize(30, 30);
		colorButton[i]->move(5 + (i%5) * 30, (i/5) * 30 + 30);
		colorButton[i]->setCheckable(true);
		pixmap.fill(sketchColor[i]);
		if (i==0) {
			painter.setPen(Qt::black);
			painter.drawLine(0, 0, 19, 19);
			painter.drawLine(19, 0, 0, 19);
		}
		colorButton[i]->setIcon(QIcon(pixmap));
		colorButtonGroup->addButton(colorButton[i], i);
	}
	ypos += ((SKETCHCOLORS+4)/5) * 30 + 30;
	colorButton[4]->setChecked(true);
	colorButtonGroup->setExclusive(true);
	// Pen width buttons
	widthButtonGroup = new QButtonGroup(this);
	for (int i=0; i<SKETCHWIDTHS; i++) {
		widthButton[i] = new QPushButton(this);
		widthButton[i]->resize(30, 30);
		widthButton[i]->move(5 + (i%5) * 30, (i/5) * 30 + 30 + ypos);
		widthButton[i]->setCheckable(true);
		pixmap.fill(widthButton[i]->palette().color(QWidget::backgroundRole()));
		painter.drawEllipse(10-i,10-i,i*2+1, i*2+1);
		widthButton[i]->setIcon(QIcon(pixmap));
		widthButtonGroup->addButton(widthButton[i], i);
	}
	widthButton[0]->setChecked(true);
	ypos += ((SKETCHWIDTHS+4)/5) * 30 + 60;
	
	backgroundFileBox = new QComboBox(this);
	backgroundFileBox->resize(150, 30);
	backgroundFileBox->move(5, ypos);
	backgroundFileBox->addItem("No Background");
	backgroundFileBox->addItems(ctrl->dir[DIR_PICS].entryList(QStringList(), QDir::Files, QDir::Name));
	ypos += 30;
	
	backgroundScalingBox = new QComboBox(this);
	backgroundScalingBox->resize(75, 30);
	backgroundScalingBox->move(5, ypos);
	backgroundScalingBox->addItem("x 0.5");
	backgroundScalingBox->addItem("x 1");
	backgroundScalingBox->addItem("x 2");
	backgroundScalingBox->addItem("x 4");
	
	backgroundBrightnessBox = new QComboBox(this);
	backgroundBrightnessBox->resize(75, 30);
	backgroundBrightnessBox->move(80, ypos);
	backgroundBrightnessBox->addItem("0");
	backgroundBrightnessBox->addItem("+");
	backgroundBrightnessBox->addItem("++");
	backgroundBrightnessBox->addItem("+++");
	ypos += 30;

	clearButton = new QToolButton(this);
	clearButton->resize(150,30);
	clearButton->move(5, ypos);
	clearButton->setText("Clear Page");
	ypos += 30;
	
	undoButton = new QToolButton(this);
	undoButton->resize(150, 30);
	undoButton->move(5, ypos);
	undoButton->setText("Undo");
	ypos += 30;
	
	
	gamemasterChange();
	connect(clearButton, SIGNAL(clicked()), this, SLOT(clearButtonPressed()));
	connect(undoButton, SIGNAL(clicked()), this, SLOT(undoButtonPressed()));
	connect(ctrl, SIGNAL(gamemasterChange()), this, SLOT(gamemasterChange()));
	connect(backgroundFileBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBackgroundSettings()));
	connect(backgroundScalingBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBackgroundSettings()));
	connect(backgroundBrightnessBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBackgroundSettings()));
}

int SketchTools::getColor()
{
	//qDebug("SketchTools: Reporting color %d", colorButtonGroup->checkedId());
	int r = colorButtonGroup->checkedId();
	if (r<SKETCHCOLORS) return r;
	return SKETCHCOLORS-1-r;
}

int SketchTools::getWidth()
{
	return 1<<widthButtonGroup->checkedId();	
}


void SketchTools::clearButtonPressed()
{
	QMessageBox::StandardButton reply;
	reply = QMessageBox::question(this, "Confirm", "Clear the entire sketch?",QMessageBox::Yes|QMessageBox::No);
	if (reply == QMessageBox::Yes) {
		emit(clearActualPage());
	}
}

void SketchTools::undoButtonPressed()
{
	qDebug("SketchTools: Undo pressed");
	emit(undo());
}

void SketchTools::gamemasterChange()
{
	if (ctrl->mode.get() == MODE_MASTER) {
		clearButton->show();
		backgroundFileBox->show();
		backgroundScalingBox->show();
		backgroundBrightnessBox->show();
	} else {
		clearButton->hide();
		backgroundFileBox->hide();
		backgroundScalingBox->hide();
		backgroundBrightnessBox->hide();
	}
}

void SketchTools::registerNewTab(QString * filename, uint16_t * scale, uint16_t * brightness)
{
	qDebug() << "SketchTools: Registering new Tab" << (*filename);
	assert (filename != NULL);
	assert(scale != NULL);
	assert(brightness != NULL);
	this->backgroundFile = filename;
	this->backgroundScale = scale;
	this->backgroundBrightness = brightness;
	// Block the signals to process it all at once in the end
	backgroundFileBox->blockSignals(true);
	backgroundScalingBox->blockSignals(true);
	backgroundBrightnessBox->blockSignals(true);
	int i;
	if (!filename->isEmpty()) {
		for (i=1; i< backgroundFileBox->count(); i++) {
			qDebug() << "SketchTools: Comparing" << (*filename) << backgroundFileBox->itemText(i);
			if (backgroundFileBox->itemText(i) == (*filename)) {backgroundFileBox->setCurrentIndex(i); break;}
		}
		if (i == backgroundFileBox->count()) {
			qDebug() << "SketchTools: Background" << (*filename) << "not found on disk.";
			if (ctrl->mode.get() == MODE_MASTER) backgroundFileBox->setCurrentIndex(0);
			else {
				backgroundFileBox->addItem(*filename);
				backgroundFileBox->setCurrentIndex(backgroundFileBox->count()-1); // File not found!
			}
		}
	} else backgroundFileBox->setCurrentIndex(0);
	backgroundScalingBox->setCurrentIndex(*scale);
	backgroundBrightnessBox->setCurrentIndex(*brightness);
	backgroundFileBox->blockSignals(false);
	backgroundScalingBox->blockSignals(false);
	backgroundBrightnessBox->blockSignals(false);

	updateBackgroundSettings(); // Since we had the signals diabled to avoid confusion, do it manually
}

void SketchTools::updateBackgroundSettings()
{
	qDebug("SketchTools: updating background settings");
	if (backgroundFile == NULL || backgroundBrightness == NULL || backgroundScale == NULL) return; // Not ready yet
	if (backgroundFileBox->currentIndex() == 0) (*backgroundFile) = QString(); 
	else (*backgroundFile) = backgroundFileBox->currentText();
	(*backgroundBrightness) = backgroundBrightnessBox->currentIndex();
	(*backgroundScale) = backgroundScalingBox->currentIndex();
	
	emit(updateBackground());
}

void SketchTools::fileUpdate(int dir, QString filename)
{
	&filename;
	if (dir == DIR_PICS) {
		backgroundFileBox->setDuplicatesEnabled(false);
		backgroundFileBox->addItems(ctrl->dir[DIR_PICS].entryList(QStringList(), QDir::Files, QDir::Name));
	}
}


// ********** Sketch Element *******

SketchElement::SketchElement()
{
	clear();
}

QByteArray SketchElement::pack()
{
	QByteArray e = extra.toUtf8();
	uint len = e.size();
	assert (len<65536);
	return toNBO(type) + toNBO(properties) + toNBO(x1) + toNBO(x2) + toNBO(y1) + toNBO(y2) + toNBO(uint16_t(len)) + e;
}

void SketchElement::clear()
{
	type = 0x0000;
	properties = 0;
	x1 = 0;
	x2 = 0;
	y1 = 0;
	y2 = 0;
	extra = QString();
}

bool SketchElement::unpack(QByteArray * in)
{
	if (!fromNBO(in, &type)) return false;
	if (!fromNBO(in, &properties)) return false;
	if (!fromNBO(in, &x1)) return false;
	if (!fromNBO(in, &x2)) return false;
	if (!fromNBO(in, &y1)) return false;
	if (!fromNBO(in, &y2)) return false;
	uint16_t len;
	if (!fromNBO(in, &len)) return false;
	extra = in->left(len);
	return true;	
}


// ********** Sketch Page **********

SketchPage::SketchPage(SketchTools * tools, int nr, QWidget * parent) : QWidget(parent)
{
	courtYard = new CourtYard;
	sketchTools = tools;
	xos = 0;
	yos = 0;
	mynr = nr;
	lastx = 0;
	lasty = 0;
	paintingInProgress = false;
	backgroundFile = QString();
	backgroundScale = 0;
	backgroundBrightness = 0;
	QImage backgroundImage;

	if (ctrl->mode.get() == MODE_MASTER) {load(); unlock();} else {lock();}
	timer.setSingleShot(false);
	timer.setInterval(200);
	connect(&timer, SIGNAL(timeout()), this, SLOT(timerTick()));
	connect(ctrl, SIGNAL(connectionStateChange(int)), this, SLOT(connectionStateChange(int)));
	connect(ctrl, SIGNAL(gamemasterChange()), this, SLOT(gamemasterChange()));
	connect(ctrl, SIGNAL(shutDown()), this, SLOT(shutDown()));
	connect(ctrl, SIGNAL(fileUpdate(int, QString)), this, SLOT(fileUpdate(int, QString)));
	setCursor(cursor);
	timer.start();
}

SketchPage::~SketchPage()
{

}

bool SketchPage::getCutPositions(float &p1, float &p2, int x1, int y1, int x2, int y2, int xc, int yc, int rc)
// Calculates the two positions on a vector (relative to the vector's length) where a circle intersecs it
// Returns false when no or only one solution can be found
{
	// Gain of the vector in x and y direction by length
	//float l = sqrtf((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
	//qDebug("SketchPage: Vector length: %5.2f", l);
	float dx = (float)(x2 - x1);
	float dy = (float)(y2 - y1);
	// Coefficients of square function
	float a = dx*dx + dy*dy;
	float b = 2.0f*(float)x1*dx - 2.0f*(float)xc*dx + 2.0f*(float)y1*dy - 2.0f*(float)yc*dy;
	float c = (float)(x1*x1 - 2*x1*xc + xc*xc + y1*y1 - 2*y1*yc + yc*yc - rc*rc);
	float d = b*b - 4*a*c;
	if (d<=0) return false; // No or only one solution?
	float rd = sqrtf(d);
	p1 = (-b + rd) / (2.0f*a);
	p2 = (-b - rd) / (2.0f*a);
	if (p1 > p2) swp(p1, p2);
	return true;
}

void SketchPage::draw(QPainter *painter, SketchElement *element)
{
	switch (element->type) {
		case 0: break; //Dummy!
		case 1: // Draw a simple line FIXME: Give them names!
			int x1, y1, x2, y2;
			x1 = (int)element->x1 - xos * 250;
			x2 = (int)element->x2 - xos * 250;
			y1 = (int)element->y1 - yos * 250;
			y2 = (int)element->y2 - yos * 250;
			courtYard->expand((int)element->x1, (int)element->y1);
			courtYard->expand((int)element->x2,(int)element->y2);
			// Check whether they might be in view
			//if (x1 < 3*250/2 && x1 > -3*250/2) Later... leave it to qpainter for the first try
			QPen pen;
			int c = element->properties & 0x00FFu;
			//qDebug("SketchPage: Processing color %d", c);
			assert(c < SKETCHCOLORS);
			pen.setColor(sketchColor[c]);
			pen.setWidth(element->properties>>8);
			pen.setCapStyle(Qt::RoundCap);
			painter->setPen(pen);
			if (x1 == x2 && y1 == y2) painter->drawPoint(x1 + width()/2, y1 + height() / 2); // Is only a dot
			else painter->drawLine(x1 + width() /2, y1 + height() / 2, x2 + width() / 2, y2 + height() / 2);
		
		break;
	}
}

void SketchPage::eraser(int x, int y, int eraserradius) 
{
	//qDebug("SketchPage: Elements:");
	for (int i=sketchElement.size()-1; i>=0; i--) {
		SketchElement * element = &sketchElement[i];
		switch (element->type) {
			case 0: break; //Dummy!
			case 1: // We have a line - check if that's within range of the eraser
				//qDebug("    Type %4x parameters: %4x x1: %d y1: %d x2: %d y2: %d", element->type, element->properties, element->x1, element->y1, element->x2, element->y2);
				// FIXME: Only process objects in view
				// Point only
				if (isInRadius(element->x1, element->y1, x, y, eraserradius) && isInRadius(element->x2, element->y2, x, y, eraserradius)) {
					//qDebug("SketchPage: Deleting small element");
					// Object is completely under eraser, that's simple => erase it!
					removeRemote(element);
					sketchElement.remove(i);
					break;
				}	void updateBackground();

				if (element->x1 == element->x2 && element->y1 == element->y2) break; // Do not process points with the following algorithm!
				float p1, p2;
				if (getCutPositions(p1, p2, element->x1, element->y1, element->x2, element->y2, x, y, eraserradius)) {
					//qDebug("SketchPage: Cut positions %5.2f %5.2f", p1, p2);
					if (p1 <= 0.0f && p2 <= 0.0f) break; // Eraser is completely on one side of the vector FIXME: ckeck this before calculating all the stuff!
					if (p1 > 1.0f && p2 > 1.0f) break; // Eraser is completely on the other side of the vector FIXME: ckeck this before calculating all the stuff!
					if ((p1 <= 0.0f && p2 > 1.0f)) {
						// Erasor sourrounding the line completely => erase it completely! 
						// FIXME: Do this without applying that much maths
						removeRemote(element);
						sketchElement.remove(i);
						break;
					} 
					if (p1 <= 0.0f && p2 <= 1.0f) {
						// The erasor is sourrounding the starting point but not the endpoint => shorten the vector from the endpoint
						removeRemote(element); // There is no remote modify (would cause the same network traffic), so delete it...
						int dx = element->x2 - element->x1;
						int dy = element->y2 - element->y1;
						element->x1 = (int16_t)roundf(element->x2 - (float)dx * (1.0 - p2));
						element->y1 = (int16_t)roundf(element->y2 - (float)dy * (1.0 - p2));
						addRemote(element); // ... and add it as new!
						break;
					}
					if (p1 > 0.0f && p2 > 1.0f) {
						removeRemote(element); // There is no remote modify (would cause the same network traffic), so delete it...
						// The erasor is sorrounding the endpoint but not the starting point => shorten the vector from the starting point
						int dx = element->x2 - element->x1;
						int dy = element->y2 - element->y1;
						element->x2 = (int16_t)roundf(element->x1 + (float)dx * p1);
						element->y2 = (int16_t)roundf(element->y1 + (float)dy * p1);
						addRemote(element); // ... and add it as new!
						break;
					}
					if (p1 > 0.0f && p2 <= 1.0f) {
						// Worst case: The erasor is splitting our line in half! Reduce the vector to the first crossection and generate a new vector from the second one
						removeRemote(element); // There is no remote modify (would cause the same network traffic), so delete it...
						SketchElement newElement;
						newElement.type = element->type;
						newElement.properties = element->properties;
						newElement.x2 = element->x2;
						newElement.y2 = element->y2;
						int dx = element->x2 - element->x1;
						int dy = element->y2 - element->y1;
						newElement.x1 = (int16_t)roundf(element->x2 - (float)dx * (1.0 - p2));
						newElement.y1 = (int16_t)roundf(element->y2 - (float)dy * (1.0 - p2));
						element->x2   = (int16_t)roundf(element->x1 + (float)dx * p1);
						element->y2   = (int16_t)roundf(element->y1 + (float)dy * p1);
						sketchElement << newElement;
						addRemote(element); // ... and add it as new!
						addRemote(&newElement); // Also add the second half
						break;
					}
					qDebug("SketchPage: Erasor - this state should not be reached");
				}
			break;
		}	
	}
}

void SketchPage::clear()
{
	SketchElement clearall;
	clearall.type = CLEARALL;
	for (int player=0; player<MAXPLAYERLINKS; player++) sendBuffer[player] += clearall.pack();
	sketchElement.clear();
	//repaint();
	update();
}

void SketchPage::paintEvent(QPaintEvent * event)
{
	//qDebug("SketchPage: Paint event");
	// Paint frame
	QPainter painter(this);
	painter.beginNativePainting();
	painter.setRenderHint(QPainter::Antialiasing);
	QBrush brush(Qt::SolidPattern);
	brush.setColor(QColor::fromRgb(0xFF, 0xFF, 0xFF));
	//painter.fillRect(0,0, 3*250+60, 3*250+60, brush);
	//qDebug("SketchPage: paintevent size x:%d, y:%d", width(), height());
	//assert(width() > 3*250+60);
	//assert(height() > 3*250+60);
	painter.fillRect(0,0, width(), height(), brush);
	if (locked) {
		painter.setPen(Qt::red);
		painter.drawText(width()/2-150, height()/2-10, 300, 20, Qt::AlignCenter, "No Connection to Gamemaster");
		event->accept();
		return;
	}
	courtYard->reset(); // When items are deleted, the courtyard may shrink again so recalculate it every time
	int usablewidth  = width() - 60;
	int usableheight = height() - 60;
	if (!backgroundImage.isNull()) {
		// Paint background image
		int imagex1 =  -usablewidth/2 + xos*250 + backgroundImage.width()/2;
		int imagex2 =   usablewidth/2 + xos*250 + backgroundImage.width()/2;
		int imagey1 = -usableheight/2 + yos*250 + backgroundImage.height()/2;
		int imagey2 =  usableheight/2 + yos*250 + backgroundImage.height()/2;
		courtYard->expand(-backgroundImage.width()/2, -backgroundImage.height()/2);
		courtYard->expand(backgroundImage.width()/2, backgroundImage.height()/2);
		// When the available part of the image is smaller than the screen place it in defined distance from the corner
		int placex = max(0, -imagex1); 
		if (imagex1 <0) imagex1 = 0;
		int placey = max(0, -imagey1);
		if (imagey1 < 0) imagey1 = 0;
		if (imagex2 >= backgroundImage.width()) imagex2 = backgroundImage.width()-1;
		if (imagey2 >= backgroundImage.height()) imagey2 = backgroundImage.height()-1;
		int imagewidth = limit(imagex2 - imagex1 + 1, 0, usablewidth - placex);
		int imageheight = limit(imagey2 - imagey1 + 1, 0, usableheight - placey);
		if (placex < usablewidth && placey < usableheight) painter.drawImage(placex+30, placey+30, backgroundImage, imagex1, imagey1, imagewidth, imageheight);
	} else {
		// Dense raster only when no background image is present
		painter.setPen(Qt::lightGray);
		for (int x = 0; x <= usablewidth / 2; x += 25) {
			painter.drawLine(x + usablewidth/2 + 30, 30, x + usablewidth/2 + 30, usableheight + 30);
			if (x>0) painter.drawLine(-x + usablewidth/2 + 30, 30, -x + usablewidth/2 + 30, usableheight + 30);
		}
		for (int y = 0; y <= usableheight / 2; y += 25) {
			painter.drawLine(30, y + usableheight/2 + 30, usablewidth + 30, y + usableheight/2 + 30);
			if (y>0) painter.drawLine(30, -y + usableheight/2 + 30, usablewidth + 30, -y + usableheight/2 + 30);
		}
	}
	// Coarse raster
	painter.setPen(Qt::gray);
	for (int x = 125; x <= usablewidth / 2; x += 250) {
		painter.drawLine(x + usablewidth/2 + 30, 0, x + usablewidth/2 + 30, usableheight + 60);
		painter.drawLine(-x + usablewidth/2 + 30, 0, -x + usablewidth/2 + 30, usableheight + 60);
	}
	for (int y = 125; y <= usableheight / 2; y += 250) {
		painter.drawLine(0, y + usableheight/2 + 30, usablewidth + 60, y + usableheight/2 + 30);
		painter.drawLine(0, -y + usableheight/2 + 30, usablewidth + 60, -y + usableheight/2 + 30);
	}
	painter.drawLine(0, 30, usablewidth + 60, 30);
	painter.drawLine(0, usableheight + 30, usablewidth + 60, usableheight + 30);
	painter.drawLine(30, 0, 30, usableheight + 60);
	painter.drawLine(usablewidth + 30, 0, usablewidth + 30, usableheight + 60);
	painter.setPen(Qt::gray);
	for (int i = - (usablewidth / 2 - 20) / 250; i <= (usablewidth / 2 - 20) / 250; i++) {
		painter.drawText(i*250+width()/2-30,               5, 60, 20, Qt::AlignCenter, QString::number(i+xos));
		painter.drawText(i*250+width()/2-30, usableheight+35, 60, 20, Qt::AlignCenter, QString::number(i+xos));
	}
	painter.rotate(90.0);
	for (int i = - (usableheight / 2 - 20) / 250; i <= (usableheight / 2 - 20) / 250; i++) {
		painter.drawText(i*250+height()/2-30,             -25, 60, 20, Qt::AlignCenter, QString::number(i+yos));
		painter.drawText(i*250+height()/2-30, -usablewidth-55, 60, 20, Qt::AlignCenter, QString::number(i+yos));
	}
	painter.rotate(-90.0);

	painter.setClipRect(30, 30, usablewidth, usableheight); // Forbid drawing onto the border
	//qDebug("SketchPad: resetting courtyard");
	for (int i=0; i<sketchElement.size(); i++) draw(&painter, &sketchElement[i]);
	if (activeElement != NULL) draw(&painter, activeElement); 
	
	// Draw Courtyard, FIXME: If desired!
	QPen pen;
	pen.setColor(Qt::darkGray); // FIMXE! Stub only! element.properties & 0xFF
	pen.setStyle(Qt::DotLine);
	painter.setPen(pen);
	painter.drawRect(courtYard->x1-xos*250+width()/2, courtYard->y1-yos*250+height()/2, courtYard->x2-courtYard->x1, courtYard->y2-courtYard->y1);
	event->accept();
	//qDebug("SketchPad: Paint event done");
}

void SketchPage::mousePressEvent(QMouseEvent * event)
{
	if (locked) return; // No painting when gamemaster is not present
	paintingInProgress = true;

	//qDebug("SketchPage: Mouse press event");
	// Klicking on the border can be used for scrolling
	scroll = false;
	if (event->x() <= 30)            {scrollLeft();  scroll = true;}
	if (event->x() >= width() - 30)  {scrollRight(); scroll = true;}
	if (event->y() <= 30)            {scrollUp();    scroll = true;}
	if (event->y() >= height() - 30) {scrollDown();  scroll = true;}
	if (scroll) return;
	
	activeElement = new SketchElement;
	undoList.clear();
	
	penColor = sketchTools->getColor();
	penWidth = sketchTools->getWidth();
	
	int x = limit(event->x() - width()/2 + xos*250, -32768, 32767);
	int y = limit(event->y() - height()/2 + yos*250, -32768, 32767);
	if (penColor == 0) eraser(x,y, penWidth * 5);
		// Look trough all drawing elements to see if one is near the eraser
		// FIXME: Limit this to the visible objects only to save CPU!
	else {
		activeElement->type = 1; // Line
		activeElement->properties = penWidth << 8 | penColor; 
		
		Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
		if (modifiers == Qt::NoModifier) {
			drawstraight = false;
			activeElement->x1 = activeElement->x2 = x;
			activeElement->y1 = activeElement->y2 = y;
		} else if (modifiers & Qt::ControlModifier && lastx != 0 && lasty != 0) {
			drawstraight = true;
			activeElement->x1 = lastx;
			activeElement->x2 = x;
			activeElement->y1 = lasty;
			activeElement->y2 = y;
		}
	}
	lastx = x;
	lasty = y;
	//repaint(); // Even if we do not move the mouse we want to see the placed dot, so repaint now
	update();
}


void SketchPage::mouseMoveEvent(QMouseEvent * event)
{
	//qDebug("SketchPage: Mouse move event");	
	// FIXME: Check whether we can interpolate all last points by a straight line
	// FIXME: Add more drawing aids by pressing controls
	if (locked) return; // No painting when gamemaster is not present
	if (scroll) return;
	int x = limit(event->x() - width()/2 + xos*250, -32768, 32767);
	int y = limit(event->y() - height()/2 + yos*250, -32768, 32767);
	if (penColor == 0) eraser(x,y, penWidth * 5);
	else {
		activeElement->x2 = x;
		activeElement->y2 = y;
		if (!drawstraight) {
			addRemote(activeElement); // End of segement, store it
			sketchElement << (*activeElement);
			activeElement->type = 1; // Line
			activeElement->properties = penWidth << 8 | penColor; 
			activeElement->x1 = activeElement->x2 = x;
			activeElement->y1 = activeElement->y2 = y;
		}
	}
	lastx = x;
	lasty = y;
	update();
	//repaint();
}

void SketchPage::mouseReleaseEvent(QMouseEvent * event)
{
	if (locked) return; // No painting when gamemaster is not present
//	qDebug("SketchPage: Mouse release event");
	if (activeElement == NULL) return; // Seems to be the mouse release event of scrolling
	if (penColor != 0) {
		sketchElement << (*activeElement);
		addRemote(activeElement); // When the mouse is released, the last element is finished
		delete activeElement;
		activeElement = NULL;
		paintingInProgress = false;
		//repaint();
		update();
	}
	paintingInProgress = false;

	event->accept(); // Just to avoid the unused variable warning
}

void SketchPage::wheelEvent(QWheelEvent * event)
{
	static int deltax=0, deltay=0;
	Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
	qDebug("Map: Mouse scrolled %d degree in y", event->angleDelta().y());
	qDebug("Map: Mouse scrolled %d degree in x", event->angleDelta().x());
	if (modifiers & Qt::ControlModifier) return; // Cannot zoom
	if (modifiers & Qt::ShiftModifier) {
		deltax+=event->angleDelta().y();
	} else {
		deltay+=event->angleDelta().y();
		deltax+=event->angleDelta().x();
	}
	if (deltax >= 120)  {deltax -= 120; scrollRight();}
	if (deltax <= -120) {deltax += 120;	scrollLeft();}
	if (deltay <= -120) {deltay += 120;	scrollDown();}
	if (deltay >= 120)  {deltay -= 120;	scrollUp();	}
}

void SketchPage::scrollLeft()
{
	if (xos > courtYard->x1/250-1 && xos > -125) xos --;
	repaint();
}

void SketchPage::scrollRight()
{
	if (xos < courtYard->x2/250+1 && xos < 125) xos ++;
	repaint();
}

void SketchPage::scrollUp()
{
	if (yos > courtYard->y1/250-1 && yos > -125) yos --;
	repaint();
}

void SketchPage::scrollDown()
{
	if (yos < courtYard->y2/250+1 && yos < 125) yos ++;
	repaint();	
}

void SketchPage::load()
{
	QFile * file;
	file = new QFile(ctrl->dir[DIR_SKETCHES].filePath("sketch_"+QString::number(mynr)+".sk"));
	if (!file->open(QIODevice::ReadOnly)) {
		qDebug("SketchPad: Cannot open file for sketch nr. %d! Maybere there is none...", mynr);
		return;
	}
	sketchElement.clear();
	file->seek(0);
	newSketch(-1, file->readAll()); // There is no difference whether we load it from disk or get it from the gamemaster
}

QByteArray SketchPage::getSnapshot()
// Returns a snapshot of all sketches including background and offset information
{
	QByteArray buffer;
	SketchElement element;
	// Add a clearall to simplify handling
	element.clear();
	element.type = CLEARALL;
	buffer += element.pack();
	// Store the background information
	element.clear();
	element.type = BACKGROUND;
	element.extra = backgroundFile;
	element.x1 = backgroundBrightness;
	element.x2 = backgroundScale;
	buffer += element.pack();
	// Store the offsets
	element.clear();
	element.type = OFFSET;
	element.extra = QString();
	element.x1 = xos;
	element.x2 = yos;
	buffer += element.pack();

	for (int i=0; i<sketchElement.size(); i++) {
		if (!(sketchElement[i].type & DELETED)) {
			// Do not store/send deleted elements - undo information is lost during save
			buffer += sketchElement[i].pack();
		}
	}
	return buffer;
}

void SketchPage::store()
{
	// Only the master keeps track of all the sketches
	QSaveFile * file;
	file = new QSaveFile(ctrl->dir[DIR_SKETCHES].filePath("sketch_"+QString::number(mynr)+".sk"));
	if (!file->open(QIODevice::WriteOnly)) {qWarning("SketchPad: Cannot write sketch nr. %d to disk!", mynr); return; }
	file->write(qCompress(getSnapshot()));
	file->commit();
	qDebug("SketchPage: Saved sketch nr. %d", mynr);
}

void SketchPage::addRemote(SketchElement * element)
{
	// Store in the undo list as well
	undoList << (*activeElement); // FIXME: Why not _element_???
	// Only the gamemaster broadcasts to all, the players sent to the gamemaster only
	if (ctrl->mode.get() == MODE_MASTER) {
		for (int p=0; p<MAXPLAYERLINKS; p++) sendBuffer[p] += element->pack();
	} else sendBuffer[0] += element->pack();
}

void SketchPage::removeRemote(SketchElement * element)
{
	SketchElement delelement = * element;
	delelement.type |= DELETED;
	undoList << delelement;
	if (ctrl->mode.get() == MODE_MASTER) {
		for (int p=0; p<MAXPLAYERLINKS; p++) sendBuffer[p] += delelement.pack();
	} else sendBuffer[0] += delelement.pack();
}

void SketchPage::addLocal(SketchElement * element)
{
	if (element->type == CLEARALL) {
		sketchElement.clear();
		unlock(); // A clear request from the master indicates the presence of the master
	} else if (element->type & DELETED) {
		// We've got a request to delete one element. Find it!
		element->type &= (~DELETED); // Clear this flag for simpler comparison
		for (int k=0; k<sketchElement.size(); k++) {
			if (sketchElement[k] == (*element)) {
				sketchElement.remove(k); 
				break;
			}
		}
	}
	else sketchElement << *element; // Received a remote sketch element. Add it to the local ones
}

void SketchPage::newSketch(int player, QByteArray data)
{
	// Received a remote sketch
	data = qUncompress(data);
	qDebug("SketchPage %d: Got %d remote sketches", mynr, (int)(data.size()/sizeof(SketchElement)));
	if (player>0 && ctrl->mode.get() == MODE_MASTER) {
		// When we're the gamemaster, relay the sketch to the other players (but not back to sender, of course!)
		// But if this function is used for loading sketches (player = -1) do not send. This will be done when the palyers connect
		for (int p=0; p<MAXPLAYERLINKS; p++) if (p != player && ctrl->connectionStatus.get(p) == CON_READY) {
			sendBuffer[p] += data;
		}
	}
	SketchElement element;
	while (data.size() > 0) {
		if (!element.unpack(&data) ) {qDebug("SketchPage: error converting received sketch"); return;}
		switch(element.type) {
			// Do we have a dummy element that means something completely different than a normal sketch element?
			case CLEARALL:
				sketchElement.clear();
				unlock(); // A clear command from the gamemaster indicates that the connection is up
				break;
			case BACKGROUND:
				backgroundFile = element.extra;
				backgroundBrightness = element.x1;
				backgroundScale = element.x2;
				updateBackground();
				break;
			case OFFSET:
				xos = element.x1;
				yos = element.x2;
				break;
			default:
				addLocal(&element);
				break;
		}
	}
	update();
}

void SketchPage::updateBackground()
{
	qDebug("SketchPage %d: Got background update", mynr);
	if (backgroundFile.isEmpty()) backgroundImage = QImage(); 
	else {
		if (ctrl->dir[DIR_PICS].exists(backgroundFile))	backgroundImage = QImage(ctrl->dir[DIR_PICS].filePath(backgroundFile));
		else backgroundImage = QImage(); //FIXME: Load a temporary substitude
		if (backgroundImage.isNull()) qWarning("SketchPage %d: error loading background image!", mynr);
	}
	// When we're master, send to all the others that the background changed
	if (ctrl->mode.get() == MODE_MASTER) {
		SketchElement element;
		element.type = BACKGROUND;
		element.extra = backgroundFile;
		element.x1 = backgroundBrightness;
		element.x2 = backgroundScale;
		for (int p=0; p<MAXPLAYERLINKS; p++) sendBuffer[p] += element.pack();
	}
	// FIXME: fading and scaling!
	update();
}

void SketchPage::timerTick()
{
	// If we collect for a few 100ms instead of sending it immediately we can compress better
	//qDebug("SketchPage Nr. %d: Timer tick", mynr);
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		//qDebug("SketchPage: %d elements waiting for player %d", remoteElement[p].size(), p);
		if (ctrl->connectionStatus.get(p) == CON_READY && sendBuffer[p].size() > 0 && bulkbay->getBufferUsage(p) < BUF_MID_PRIO) {
			// Player is connected and there is something so send and there seems to be enough space in the send buffer
			int result = bulkbay->addToTxBuffer(p, SKETCH, QByteArray(1, (char)mynr) + qCompress(sendBuffer[p]), REPORT, SERIAL);
			if (result >= 0) {
				emit(ctrl->fileTransferStart(result, "Sketch p."+QString::number(mynr+1), p));
				qDebug("SketchPage %d: Sent sketches to player %d", mynr, p);
				sendBuffer[p].clear(); // When sent successfully, we can clear the buffer of elements to send
			} //else qDebug("SketchPage Nr. %d: Got error %d while trying to send sketches to player %d", mynr, result, p);
		}
	}
}

void SketchPage::connectionStateChange(int player)
{
	if (ctrl->mode.get() == MODE_MASTER) {
		if (ctrl->connectionStatus.get(player) == CON_READY) {
			// A new player joined and we're master - share our art!
			qDebug("SketchPage %d: Sending actual sketch to recently connected player %d", mynr, player);
			sendBuffer[player] = getSnapshot(); // Make player to start from scratch
		}
	} else {
		// The main quesion here is: Do we have lost the connection to the gamemaster? If so stop painting!
		if (ctrl->connectionStatus.get(0) != CON_READY) lock();
	}
}

void SketchPage::fileUpdate(int dir, QString filename)
{
	// FIXME: Will cause a background update message to alle players when the master updates the picture file - not that bad but not nice!
	if (dir == DIR_PICS && filename == backgroundFile) updateBackground();
}

void SketchPage::lock()
{
	cursor.setShape(Qt::WaitCursor);
	setCursor(cursor);
	locked = true;
	repaint();
}

void SketchPage::unlock()
{
	cursor.setShape(Qt::ArrowCursor);
	setCursor(cursor);
	locked = false;
	repaint();
}

void SketchPage::gamemasterChange()
{
	if (ctrl->mode.get() == MODE_MASTER) {
		// We are new gamemaster now?
		load();
		unlock();
		for (int p=0; p<MAXPLAYERLINKS; p++) connectionStateChange(p); // Send our art to every connected player
	} else {
		// We are now player?
		store(); // Save what we've made as master and wait for the art from the new master
		lock();
	}
}

void SketchPage::undo()
{
	qDebug("SketchPage %d: Undo", mynr);
	for (int i=0; i<undoList.size(); i++) {
		undoList[i].type ^= DELETED; // Delete if drawn, draw if deleted - that's undo!
		addLocal(&undoList[i]);
		// And tell the others. FIXME: Try do modify the addremote-function to do this
		if (ctrl->mode.get() == MODE_MASTER) {
			for (int p=0; p<MAXPLAYERLINKS; p++) sendBuffer[p] += undoList[i].pack();
		} else sendBuffer[0] += undoList[i].pack();
	}
	undoList.clear();
	update();
}

void SketchPage::shutDown()
{
	if (ctrl->mode.get() == MODE_MASTER) store();
}

void SketchPage::isActiveTab()
{
	// FIXME: Some instability in this feature - disabling it to keep the software usable
	//sketchTools->disconnect(SIGNAL(updateBackground()));
	//sketchTools->registerNewTab(&backgroundFile, &backgroundScale, &backgroundBrightness);
	//connect(sketchTools, SIGNAL(updateBackground()), this, SLOT(updateBackground()));
}


//****** Sketch Pad **********


SketchPad::SketchPad(QWidget * parent) : QDialog(parent)
{
	setWindowTitle("Sketchpad");
	setModal(false);
	tabWidget = new QTabWidget(this);
	tabWidget->resize(3*250+70, 3*250+60+35);
	tabWidget->move(0,0);
	sketchTools = new SketchTools(this);
	for (int i=0; i<SKETCHPAGES; i++) {
		sketchPage[i] = new SketchPage(sketchTools, i, NULL);
		tabWidget->addTab(sketchPage[i], "Page " + QString::number(i+1));
	}
	sketchTools->resize(sketchTools->width(), tabWidget->height());
	sketchTools->move(tabWidget->width(), 0);
	setMinimumSize(tabWidget->width()+sketchTools->width(), tabWidget->height());
	connect(bulkbay, SIGNAL(newSketch(int, QByteArray)), this, SLOT(newSketch(int, QByteArray)));
	connect(sketchTools, SIGNAL(clearActualPage()), this, SLOT(clearActualPage()));
	connect(sketchTools, SIGNAL(undo()), this, SLOT(undo()));
	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
	int activeTab = ctrl->activeSketchTab.get();
	assert(activeTab >= 0);
	assert(activeTab < SKETCHPAGES);
	tabWidget->setCurrentIndex(activeTab);
	tabChanged(activeTab);
}

SketchPad::~SketchPad()
{
	qDebug("Sketchpad: Destructor called");
}

void SketchPad::closeEvent(QCloseEvent * event)
{
	qDebug("Sketchpad: CloseEvent received");
	hide();
	event->ignore();
}

void SketchPad::newSketch(int player, QByteArray data)
{
	qDebug("SketchPad: Got new sketch from player %d", player);
	int page = data[0];
	assert(page < SKETCHPAGES);
	sketchPage[page]->newSketch(player, data.mid(1));
}

void SketchPad::clearActualPage()
{
	assert(tabWidget->currentIndex() >= 0);
	assert(tabWidget->currentIndex() < 10);
	sketchPage[tabWidget->currentIndex()]->clear();	
}

void SketchPad::undo()
{
	assert(tabWidget->currentIndex() >= 0);
	assert(tabWidget->currentIndex() < 10);
	sketchPage[tabWidget->currentIndex()]->undo();	
}

void SketchPad::resizeEvent(QResizeEvent * event)
{
	tabWidget->resize(width()-sketchTools->width(), height());
	sketchTools->move(width()-sketchTools->width(), 0);
	event->accept();
}

void SketchPad::tabChanged(int tab)
{
	qDebug("SketchPad: Tab changed to %d", tab);
	sketchPage[tab]->isActiveTab();
	ctrl->activeSketchTab.set(tab);
}