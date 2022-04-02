#include "ambientmusic.h"

#define MUSICBYTESREFILL (48000 * 2 * 2 * 2)

AmbientMusic::AmbientMusic(QWidget * parent) : QWidget(parent)
{
	play = false;

	//qDebug() << ctrl->musicPlayer->workingDirectory();
	
	playIcon = new QIcon("icons/play.png");
	stopIcon = new QIcon("icons/stop.png");
	this->resize(MASTERPANELWIDTH,95);
	if (!BORDERLESS) this->setAutoFillBackground(true);
	frame = new QGroupBox("Ambient Music", this);
	frame->resize(MASTERPANELWIDTH-10, 90);
	frame->move(5,5);
	
	playListBox = new QComboBox(frame);
	playListBox->addItems(ctrl->dir[DIR_MUSIC].entryList(QStringList(), QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name));
	playListBox->resize(frame->width()-80, 30);
	playListBox->move(5,30);
	connect(playListBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changePlaylist()));
	
	playButton = new QToolButton(frame);
	playButton->setIcon(*playIcon);
	playButton->resize(30,30);
	playButton->move (frame->width()-75,30);
	connect(playButton, SIGNAL(pressed()), this, SLOT(toggle()));

	skipButton = new QToolButton(frame);
	skipButton->setIcon(QIcon("icons/skip.png"));
	skipButton->resize(20, 30);
	skipButton->move(frame->width()-25, 30);
	connect(skipButton, SIGNAL(pressed()), this, SLOT(nextSong()));

	backButton = new QToolButton(frame);
	backButton->setIcon(QIcon("icons/back.png"));
	backButton->resize(20, 30);
	backButton->move(frame->width()-45, 30);
	connect(backButton, SIGNAL(pressed()), this, SLOT(lastSong()));
	
	posLabel = new QLabel(frame);
	posLabel->resize(frame->width()-20, 20);
	posLabel->move(10, 65);
	QFont font(posLabel->font());
	font.setFixedPitch(true);
	posLabel->setFont(font);

	timer = new QTimer;
	timer->setInterval(200);
	timer->setSingleShot(false);
	connect(timer, SIGNAL(timeout()), this, SLOT(heartbeat()));
	timer->start();
	connect(ctrl, SIGNAL(fileUpdate(int, QString)), this, SLOT(fileUpdate(int, QString)));
	changePlaylist();
}

void AmbientMusic::toggle()
{
	if (ctrl->musicState.get() == Ctrl::RUNNING || ctrl->musicState.get() == Ctrl::STARTUP) {
		ctrl->musicState.set(Ctrl::FADETOSHUTDOWN);
	}
	else if (ctrl->musicState.get() == Ctrl::STOPPED) {
		ctrl->musicState.set(Ctrl::STARTUP);
	}
	heartbeat();
}

void AmbientMusic::nextSong()
{
	ctrl->musicPos.set((ctrl->musicPos.get()+1) % ctrl->musicList.get().size());
	if (ctrl->musicState.get() == Ctrl::RUNNING || ctrl->musicState.get() == Ctrl::STARTUP) ctrl->musicState.set(Ctrl::FADETORESTART); // Notify the player if required
	heartbeat(); // For update of text
}

void AmbientMusic::lastSong()
{
	ctrl->musicPos.set((ctrl->musicPos.get()+ctrl->musicList.get().size()-1) % ctrl->musicList.get().size());
	if (ctrl->musicState.get() == Ctrl::RUNNING || ctrl->musicState.get() == Ctrl::STARTUP) ctrl->musicState.set(Ctrl::FADETORESTART); // Notify the player if required
	heartbeat(); // For update of text
}

void AmbientMusic::heartbeat()
{
	Ctrl::State musicstate = ctrl->musicState.get();
	if (musicstate == Ctrl::RUNNING || musicstate == Ctrl::STARTUP || musicstate == Ctrl::FADETORESTART) playButton->setIcon(*stopIcon);
	else playButton->setIcon(*playIcon);

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

void AmbientMusic::changePlaylist()
{
	QDir playDir(ctrl->dir[DIR_MUSIC].absolutePath() + "/" + playListBox->currentText());
	//qDebug() << "AmbientMusic: MusicDir is:" << playDir;
	ctrl->musicList.set(playDir.entryList(QStringList() << "*.mp3" << "MP3", QDir::Files));
	ctrl->musicDir.set(playDir);
	ctrl->musicPos.set(0); 
	//qDebug() << "AmbientMusic: Music files in directory:" << playList;
	// FIXME: Shuffe if desired!
	qDebug() << "AmbientMusic: Changing to " << playListBox->currentText();
	if (ctrl->musicState.get() == Ctrl::RUNNING || ctrl->musicState.get() == Ctrl::STARTUP) ctrl->musicState.set(Ctrl::FADETORESTART); // Notify the player if required
	heartbeat();
}

void AmbientMusic::fileUpdate(int dir, QString name)
{
	(void)name;
	if (dir != DIR_MUSIC) return;
	playListBox->clear();
	playListBox->addItems(ctrl->dir[DIR_MUSIC].entryList(QStringList(), QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name));
}