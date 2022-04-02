#include "ambientsound.h"

AmbientSound::AmbientSound(QWidget * parent) : QWidget(parent)
{
	this->resize(MASTERPANELWIDTH,100);
	this->setAutoFillBackground(true);
	frame = new QGroupBox("Ambient Sound", this);
	frame->resize(MASTERPANELWIDTH-10, 90);
	frame->move(5,5);
	QStringList filters;
	filters << "*.mp3" << "*.mpeg3" << "*.MP3" << "*.MPEG3";
	playListBox = new QComboBox(frame);
	playListBox->addItems(ctrl->dir[DIR_SFX].entryList(filters, QDir::Files, QDir::Name));
	playListBox->resize(frame->width()-70, 30);
	playListBox->move(5,30);
	connect(playListBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changePlaylist()));

	playOnceIcon = new QIcon("icons/PlayOnce.png");
	playLoopIcon = new QIcon("icons/PlayLoop.png");
	stopIcon = new QIcon("icons/stop.png");

	playOnceButton = new QToolButton(frame);
	playOnceButton->setIcon(*playOnceIcon);
	playOnceButton->resize(30,30);
	playOnceButton->move (frame->width()-65,30);
	connect(playOnceButton, SIGNAL(pressed()), this, SLOT(playOncePressed()));
	
	playLoopButton = new QToolButton(frame);
	playLoopButton->setIcon(*playLoopIcon);
	playLoopButton->resize(30,30);
	playLoopButton->move (frame->width()-35,30);
	connect(playLoopButton, SIGNAL(pressed()), this, SLOT(playLoopPressed()));
	
	ambientVolume = new QSlider(Qt::Horizontal, frame);
	ambientVolume->resize(frame->width()-10,20);
	ambientVolume->setMinimum(0);
	ambientVolume->setMaximum(200);
	ambientVolume->move(5,65);
	connect(ambientVolume, SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));
	ambientVolume->setSliderPosition(ctrl->ambientVolume.get());
}

void AmbientSound::playOncePressed()
{
	
}

void AmbientSound::playLoopPressed()
{
	
}

void AmbientSound::setVolume(int volume)
{
	ctrl->ambientVolume.set(volume);
}

void AmbientSound::heartbeat()
{
/*
	Ctrl::State ambientstate = ctrl->ambientState.get();
	if (ambientstate == Ctrl::RUNNING || ambientstate == Ctrl::STARTUP || ambientstate == Ctrl::FADETORESTART) playButton->setIcon(*stopIcon);
	else playButton->setIcon(*playIcon);

	else {
		if (actualSong != ctrl->musicList.get().at(ctrl->musicPos.get())) {
			actualSong = ctrl->musicList.get().at(ctrl->musicPos.get()); 
			actualSongScrolled = actualSong + "     ";
		}
		posLabel->setText(QString::number(ctrl->musicPos.get()+1) + QString(" / ") + QString::number(ctrl->musicList.get().size()) + " : " + actualSongScrolled);
		if (actualSongScrolled.length() > 20) actualSongScrolled = actualSongScrolled.mid(1) + actualSongScrolled.left(1);
	}
	 * */
}

void AmbientSound::fileUpdate(int dir, QString name)
{
	(void)name;
	if (dir != DIR_SFX) return;
	playListBox->clear();
	playListBox->addItems(ctrl->dir[DIR_MUSIC].entryList(QStringList(), QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name));
}
