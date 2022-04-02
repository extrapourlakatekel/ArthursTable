#include "ambient.h"

//#define MUSICBYTESREFILL (48000 * 2 * 2 * 2)

Ambient::Ambient(QWidget * parent) : QWidget(parent)
{
	play = false;
	QStringList filters;

	//qDebug() << ctrl->musicPlayer->workingDirectory();
	
	playIcon = new QIcon("icons/play.png");
	stopIcon = new QIcon("icons/stop.png");
	loopOnIcon = new QIcon("icons/loop.png");
	loopOffIcon = new QIcon("icons/noloop.png");
	this->resize(MASTERPANELWIDTH, 150);
	this->setAutoFillBackground(true);
	frame = new QGroupBox("Ambient Music && Sounds", this);
	frame->resize(MASTERPANELWIDTH-10, 145);
	frame->move(5,5);
	
	musicPlayListBox = new QComboBox(frame);
	musicPlayListBox->addItems(ctrl->dir[DIR_MUSIC].entryList(QStringList(), QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name));
	musicPlayListBox->resize(frame->width()-80, 30);
	musicPlayListBox->move(5,25);
	connect(musicPlayListBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeMusicPlaylist()));
	
	musicPlayButton = new QToolButton(frame);
	musicPlayButton->setIcon(*playIcon);
	musicPlayButton->resize(30,30);
	musicPlayButton->move (frame->width()-75,25);
	connect(musicPlayButton, SIGNAL(pressed()), this, SLOT(musicToggle()));

	musicSkipButton = new QToolButton(frame);
	musicSkipButton->setIcon(QIcon("icons/skip.png"));
	musicSkipButton->resize(20, 30);
	musicSkipButton->move(frame->width()-25, 25);
	connect(musicSkipButton, SIGNAL(pressed()), this, SLOT(nextSong()));

	musicBackButton = new QToolButton(frame);
	musicBackButton->setIcon(QIcon("icons/back.png"));
	musicBackButton->resize(20, 30);
	musicBackButton->move(frame->width()-45, 25);
	connect(musicBackButton, SIGNAL(pressed()), this, SLOT(lastSong()));
	
	posLabel = new QLabel(frame);
	posLabel->resize(frame->width()-20, 20);
	posLabel->move(10, 60);
	QFont font(posLabel->font());
	font.setFixedPitch(true);
	posLabel->setFont(font);

	
	soundPlayListBox = new QComboBox(frame);
	filters << "*.mp3" << "*.mpeg3" << "*.MP3" << "*.MPEG3";
	soundPlayListBox->addItems(ctrl->dir[DIR_SOUND].entryList(filters, QDir::Files, QDir::Name));
	soundPlayListBox->resize(frame->width()-80, 30);
	soundPlayListBox->move(5,85);
	soundPlayListBox->hide();
	connect(soundPlayListBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSound()));
	
	soundButton = new QToolButton(frame);
	soundButton->resize(frame->width()-80, 30);
	soundButton->move(5,85);
	QFileInfo info(ctrl->soundFile.get());
	soundButton->setText(info.fileName());
	if (ctrl->soundFile.get().isEmpty()) soundButton->setText("...");
	connect(soundButton, SIGNAL(pressed()), this, SLOT(soundButtonPressed()));
	

	soundPlayButton = new QToolButton(frame);
	soundPlayButton->setIcon(*playIcon);
	soundPlayButton->resize(30,30);
	soundPlayButton->move (frame->width()-75,85);
	connect(soundPlayButton, SIGNAL(pressed()), this, SLOT(soundToggle()));
	
	soundLoopButton = new QToolButton(frame);
	if (ctrl->soundLoop.get()) soundLoopButton->setIcon(*loopOnIcon); else soundLoopButton->setIcon(*loopOffIcon);
	soundLoopButton->resize(40,30);
	soundLoopButton->move (frame->width()-45,85);
	connect(soundLoopButton, SIGNAL(pressed()), this, SLOT(loopPressed()));
	
	musicLabel = new QLabel(frame);
	musicLabel->setPixmap(QPixmap("icons/note.png").scaledToHeight(20,Qt::SmoothTransformation));
	musicLabel->resize(20,20);
	musicLabel->move(5,120);
	
	fadeSlider = new QSlider(Qt::Horizontal, frame);
	fadeSlider->resize(frame->width()-55,20);
	fadeSlider->setMinimum(0);
	fadeSlider->setMaximum(100);
	fadeSlider->move(25,120);
	connect(fadeSlider, SIGNAL(valueChanged(int)), this, SLOT(changeFading(int)));
	fadeSlider->setSliderPosition(ctrl->ambientFade.get());
	
	soundLabel = new QLabel(frame);
	soundLabel->setPixmap(QPixmap("icons/bird.png").scaledToHeight(20,Qt::SmoothTransformation));
	soundLabel->resize(20,20);
	soundLabel->move(frame->width()-30,120);
	
	
	timer = new QTimer;
	timer->setInterval(200);
	timer->setSingleShot(false);
	connect(timer, SIGNAL(timeout()), this, SLOT(heartbeat()));
	timer->start();
	connect(ctrl, SIGNAL(fileUpdate(int, QString)), this, SLOT(fileUpdate(int, QString)));
	changeMusicPlaylist();
	changeSound();
}

void Ambient::musicToggle()
{
	if (ctrl->musicState.get() == Ctrl::RUNNING || ctrl->musicState.get() == Ctrl::STARTUP) {
		ctrl->musicState.set(Ctrl::FADETOSHUTDOWN);
	}
	else if (ctrl->musicState.get() == Ctrl::STOPPED) {
		ctrl->musicState.set(Ctrl::STARTUP);
	}
	heartbeat();
}

void Ambient::nextSong()
{
	ctrl->musicPos.set((ctrl->musicPos.get()+1) % ctrl->musicList.get().size());
	if (ctrl->musicState.get() == Ctrl::RUNNING || ctrl->musicState.get() == Ctrl::STARTUP) ctrl->musicState.set(Ctrl::FADETORESTART); // Notify the player if required
	heartbeat(); // For update of text
}

void Ambient::lastSong()
{
	ctrl->musicPos.set((ctrl->musicPos.get()+ctrl->musicList.get().size()-1) % ctrl->musicList.get().size());
	if (ctrl->musicState.get() == Ctrl::RUNNING || ctrl->musicState.get() == Ctrl::STARTUP) ctrl->musicState.set(Ctrl::FADETORESTART); // Notify the player if required
	heartbeat(); // For update of text
}

void Ambient::heartbeat()
{
	Ctrl::State musicstate = ctrl->musicState.get();
	if (musicstate == Ctrl::RUNNING || musicstate == Ctrl::STARTUP || musicstate == Ctrl::FADETORESTART) musicPlayButton->setIcon(*stopIcon);
	else musicPlayButton->setIcon(*playIcon);

	Ctrl::State soundstate = ctrl->soundState.get();
	if (soundstate == Ctrl::RUNNING || soundstate == Ctrl::STARTUP || soundstate == Ctrl::FADETORESTART) soundPlayButton->setIcon(*stopIcon);
	else soundPlayButton->setIcon(*playIcon);

	if (ctrl->musicList.get().size() == 0) posLabel->setText("No Music Found In Directory!");
	else {
		if (actualSong != ctrl->musicList.get().at(ctrl->musicPos.get())) {
			actualSong = ctrl->musicList.get().at(ctrl->musicPos.get()); 
			actualSongScrolled = actualSong + "     ";
		}
		posLabel->setText(QString::number(ctrl->musicPos.get()+1) + QString(" / ") + QString::number(ctrl->musicList.get().size()) + " : " + actualSongScrolled);
		if (actualSongScrolled.length() > 20) actualSongScrolled = actualSongScrolled.mid(1) + actualSongScrolled.left(1);
	}
}

void Ambient::changeMusicPlaylist()
{
	QDir playDir(ctrl->dir[DIR_MUSIC].absolutePath() + "/" + musicPlayListBox->currentText());
	//qDebug() << "AmbientMusic: MusicDir is:" << playDir;
	ctrl->musicList.set(playDir.entryList(QStringList() << "*.mp3" << "MP3", QDir::Files));
	ctrl->musicDir.set(playDir);
	ctrl->musicPos.set(0); 
	//qDebug() << "AmbientMusic: Music files in directory:" << playList;
	// FIXME: Shuffe if desired!
	qDebug() << "AmbientMusic: Changing to " << musicPlayListBox->currentText();
	if (ctrl->musicState.get() == Ctrl::RUNNING || ctrl->musicState.get() == Ctrl::STARTUP) ctrl->musicState.set(Ctrl::FADETORESTART); // Notify the player if required
	heartbeat();
}

void Ambient::soundToggle()
{
	int state = ctrl->soundState.get();
	qDebug("Ambient: Toggling sound playback, actual state is %d", state);
	if (state == Ctrl::RUNNING || state == Ctrl::STARTUP) {
		ctrl->soundState.set(Ctrl::FADETOSHUTDOWN);
	}
	else if (state == Ctrl::STOPPED) {
		ctrl->soundState.set(Ctrl::STARTUP);
	}
	heartbeat();
}

void Ambient::changeSound()
{
	//qDebug() << "AmbientMusic: Music files in directory:" << playList;
	qDebug() << "AmbientMusic: Changing sound to " << soundPlayListBox->currentText();
	ctrl->soundFile.set(soundPlayListBox->currentText());
	if (ctrl->soundState.get() == Ctrl::RUNNING || ctrl->soundState.get() == Ctrl::STARTUP) ctrl->soundState.set(Ctrl::FADETORESTART); // Notify the player if required
}

void Ambient::soundButtonPressed()
{
	QString temp = QFileDialog::getOpenFileName(this, tr("Open Sound File"), ctrl->dir[DIR_SOUND].absolutePath(), tr("Sound Files (*.mp3 *.MP3"));
	if (temp.isEmpty()) return;
	ctrl->soundFile.set(temp);
	QFileInfo info(temp);
	soundButton->setText(info.fileName());
}

void Ambient::changeFading(int fade)
{
	//qDebug("Ambient: Set ambient fading to %d", fade);
	ctrl->ambientFade.set(fade);
}

void Ambient::loopPressed()
{
	bool loop = !ctrl->soundLoop.get();
	if (loop) soundLoopButton->setIcon(*loopOnIcon); else soundLoopButton->setIcon(*loopOffIcon);
	ctrl->soundLoop.set(loop);	
}

void Ambient::fileUpdate(int dir, QString name)
{
	(void)name;
	if (dir == DIR_MUSIC) {
		musicPlayListBox->clear();
		musicPlayListBox->addItems(ctrl->dir[DIR_MUSIC].entryList(QStringList(), QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name));
	}
	if (dir == DIR_SOUND) {
		//soundPlayListBox->clear();
		//soundPlayListBox->addItems(ctrl->dir[DIR_SOUND].entryList(QStringList() << "*.mp3" << "*.mpeg3" << "*.MP3" << "*.MPEG3" , QDir::Files, QDir::Name));
	}
}