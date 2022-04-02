#include <math.h>
#include <QPushButton>
#include <QWindow>
#include "mainwindow.h"

#include "setup.h"


MainWindow::MainWindow(QWidget * parent) : QWidget(parent)
{
	setWindowIcon(QIcon("icons/ArthursTable.png"));
	// Sketchpad
	sketchPad = new SketchPad(this);
	sketchPad->hide();
	
	// Effect Configurator
	effectConfig = new EffectConfig(this);
	effectConfig->hide();
	
	fileWatcher = new FileWatcher();
	
	// Layout
	lastkeyforwarded = 0x00;
	// Left control panel
	headtracker = new Headtracker(this);
	audioIo = new AudioIo(this);
	playerComLink = new PlayerComLink(this);
	dice = new DiceRoller(this);
	
	setupWidget = new QWidget(this);
	setupWidget->resize(CONTROLPANELWIDTH, 35);
	setupWidget->setAutoFillBackground(true);
	setupButton = new QPushButton(setupWidget);
	setupButton->setIcon(QIcon("icons/gear.png"));
	setupButton->resize(30,30);
	setupButton->move(5,0);
	setupButton->setFocusPolicy(Qt::NoFocus);
	setupButton->setCheckable(false);
	connect(setupButton, SIGNAL(pressed()), this, SLOT(startSetup()));

	reloadButton = new QPushButton(setupWidget);
	reloadButton->setIcon(QIcon( "icons/reload.png"));
	reloadButton->resize(30,30);
	reloadButton->move(45,0);
	reloadButton->setFocusPolicy(Qt::NoFocus);
	reloadButton->setCheckable(false);
	connect(reloadButton, SIGNAL(pressed()), this, SLOT(reloadButtonPressed()));
	
	sketchpadButton = new QPushButton(setupWidget);
	sketchpadButton->resize(30,30);
	sketchpadButton->setIcon(QIcon("icons/notepad.png"));
	sketchpadButton->move(85,0);
	sketchpadButton->setFocusPolicy(Qt::NoFocus);
	sketchpadButton->setCheckable(false);
	connect(sketchpadButton, SIGNAL(pressed()), this, SLOT(showSketchpad()));

	filetransferButton = new QPushButton(setupWidget);
	filetransferButton->resize(30,30);
	filetransferButton->setIcon(QIcon("icons/upload.png"));
	filetransferButton->move(125,0);
	filetransferButton->setFocusPolicy(Qt::NoFocus);
	filetransferButton->setCheckable(false);
	connect(filetransferButton, SIGNAL(pressed()), this, SLOT(openFileDialog()));

	visualizerButton = new QPushButton(setupWidget);
	visualizerButton->resize(30,30);
	visualizerButton->setIcon(QIcon("icons/debug.png"));
	visualizerButton->move(165,0);
	visualizerButton->setFocusPolicy(Qt::NoFocus);
	visualizerButton->setCheckable(false);
	connect(visualizerButton, SIGNAL(pressed()), this, SLOT(startVisualizer()));
	
	// Center panel (info & chat)
	infoPanel = new InfoPanel(this);
	
	// Master control panel
	teamSelector = new TeamSelector(this);
	ambient = new Ambient(this);
	//ambientSound = new AmbientSound(this);
	roomAcoustics = new RoomAcoustics(this);
	voiceEffect = new VoiceEffect(this);
	messages = new Messages(this);

	// Show tooltips on all widgets not only on the ones in focus
	setAttribute(Qt::WA_AlwaysShowToolTips);
	
	connect (ctrl, SIGNAL(mainLayoutChange()), this, SLOT(layoutChange()));
	connect(ctrl, SIGNAL(gamemasterChange()), this, SLOT(layoutChange()));
	layoutChange();
}

MainWindow::~MainWindow()
{ 
	delete headtracker;
}

void MainWindow::closeEvent(QCloseEvent * event)
{
	emit(quit());
	event->ignore();
}

void MainWindow::resizeEvent(QResizeEvent * event)
{
	layoutChange();
	event->accept();
}

void MainWindow::layoutChange()
{
	int ypos_l = 0;
	int ypos_r = 0;
	int infopanelwidth, masterpanelwidth;

	int mode = ctrl->mode.get();
	if (mode == MODE_MASTER || mode == MODE_LOOPBACK) masterpanelwidth = MASTERPANELWIDTH; else masterpanelwidth = 0;
	if (this->width() >= (CONTROLPANELWIDTH + MININFOPANELWIDTH + masterpanelwidth)) 
		infopanelwidth = this->width() - CONTROLPANELWIDTH - masterpanelwidth;
	else infopanelwidth = MININFOPANELWIDTH;
	if (infoPanel->isFullScreen()) infopanelwidth = 0; // When the Infopanel has got it's own screen, we do not have to also reserve space for it here

	// Left control panel
	headtracker->move(0,ypos_l);
	ypos_l += headtracker->height();

	audioIo->move(0,ypos_l);
	ypos_l += audioIo->height();

	playerComLink->move(0, ypos_l);
	ypos_l += playerComLink->height();

	dice->move(0, ypos_l);
	ypos_l += dice->height();

	setupWidget->move(0, ypos_l);
	ypos_l += setupWidget->height();
	
	// Master (=right) control panel
	if (mode == MODE_MASTER || mode == MODE_LOOPBACK) {
		teamSelector->show();
		teamSelector->move(CONTROLPANELWIDTH + infopanelwidth, ypos_r);
		ypos_r += teamSelector->height();
		
		ambient->show();
		ambient->move(CONTROLPANELWIDTH + infopanelwidth, ypos_r);
		ypos_r += ambient->height();
		
		/*
		ambientSound->show();
		ambientSound->move(CONTROLPANELWIDTH + infopanelwidth, ypos_r);
		ypos_r += ambientSound->height();
		*/
		roomAcoustics->show();
		roomAcoustics->move(CONTROLPANELWIDTH + infopanelwidth, ypos_r);
		ypos_r += roomAcoustics->height();

		voiceEffect->show();
		voiceEffect->move(CONTROLPANELWIDTH + infopanelwidth, ypos_r);
		ypos_r += voiceEffect->height();
		
		messages->show();
		messages->move(CONTROLPANELWIDTH + infopanelwidth, ypos_r);
		messages->resize(MASTERPANELWIDTH, height()-ypos_r+5);
		
		
	} else {
		teamSelector->hide();
		ambient->hide();
		//ambientSound->hide();
		roomAcoustics->hide();
		voiceEffect->hide();
		messages->hide();
	}
	int height = std::max(ypos_l, ypos_r);
	assert(height>0);
	resize((CONTROLPANELWIDTH + infopanelwidth + masterpanelwidth), this->height());
	move(0, 0);
	
	// Center panel
	if (!infoPanel->isFullScreen()) {
		infoPanel->resize(infopanelwidth, this->height());
		infoPanel->move(CONTROLPANELWIDTH,0);
		// Now we know what the minimum of our window is
		// And we can derive the size of the frame and the inner widget
		setMinimumSize(CONTROLPANELWIDTH + MININFOPANELWIDTH + masterpanelwidth, height);
		setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	} else {
		// Sizes for the control panels are fixed
		setMinimumSize(CONTROLPANELWIDTH + masterpanelwidth, height);
		setFixedWidth(CONTROLPANELWIDTH + masterpanelwidth);
	}
}

void MainWindow::startSetup()
{
	setup = new Setup(headtracker, this);
	//connect(setup, SIGNAL(redraw()), this, SLOT(resize())); // FIXME: Hä?
}

void MainWindow::startVisualizer()
{
	qDebug("MainWindow: Starting visualizer");
	if (visualizerActive) return;
	visualizer = new Visualizer(this);
	visualizer->setAttribute(Qt::WA_DeleteOnClose);
	connect(visualizer, SIGNAL(finished(int)), this, SLOT(visualizerEnded()));
	visualizerActive = true;
}

void MainWindow::visualizerEnded()
{
	qDebug("MainWindow: Visualizer ended");
	visualizerActive = false;
	disconnect(visualizer, SIGNAL(finished(int)), this, SLOT(visualizerEnded()));
}

void MainWindow::showSketchpad()
{
	qDebug("MainWindow: Showing sketchpad");
	sketchPad->show();
}

void MainWindow::openFileDialog()
{
	qDebug("MainWindow: Opening file dialog");
	QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select File(s)"), QDir::homePath());
	qDebug() << "MainWindow: Selected files to send:" << fileNames;
	for (int i=0; i<fileNames.size(); i++) {
		// Check if the file exists
		if (QFile::exists(fileNames.at(i))) {
			QFileInfo info(fileNames.at(i));
			FileInfo file;
			file.name = info.fileName();
			file.dirnr = DIR_SHARE;
			QString target = ctrl->dir[DIR_SHARE].filePath(file.name);
			if (QFile::exists(target)) QFile::remove(target);
			QFile::copy(fileNames.at(i), target);
			facilitymanager->addFileToSend(file);
		} else qWarning("MainWindow: File selected by user does not exist");
	}
}

void MainWindow::keyPressEvent(QKeyEvent * event)
{
	// Forward keypress to the text input window - it's the only window that can process user's text input
	if (event->key() == lastkeyforwarded) return;
	lastkeyforwarded = event->key();  // Avoid windup!
	//qDebug("MainWindow: Key press event");
	if (event->key()>=Qt::Key_F1 && event->key()<=Qt::Key_F12) {
		if (!event->isAutoRepeat()) emit(ctrl->functionKeyPressed(event->key()));
	}
	else if (event->key() == Qt::Key_Escape) {
		if (!event->isAutoRepeat()) emit(ctrl->escKeyPressed());
	}
	else infoPanel->forwardKeyPressEvent(event);
	//grabKeyboard();
	event->accept(); // Not required
}

void MainWindow::keyReleaseEvent(QKeyEvent * event)
{
	lastkeyforwarded = 0x00;

	//qDebug("MainWindow: Key release event");
	if (event->key()>=Qt::Key_F1 && event->key()<=Qt::Key_F12) {
		if (!event->isAutoRepeat()) emit(ctrl->functionKeyReleased(event->key()));
	}
	else if (event->key() == Qt::Key_Escape) {
		if (!event->isAutoRepeat()) emit(ctrl->escKeyReleased());
	}
	//releaseKeyboard();
	event->accept(); // Not required
}

void MainWindow::reloadButtonPressed()
{
	qDebug("MainWindow: ReloadButton pressed");
	fileWatcher->rescan();
}