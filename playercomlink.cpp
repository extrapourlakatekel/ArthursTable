#include "playercomlink.h"


PlayerComLink::PlayerComLink(QWidget * parent) : QWidget(parent)
{
	complete = false;
	wget = new QProcess(this);
	connect(wget, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(wgetFinished(int)));
	
	this->resize(200,120 + MAXPLAYERLINKS * 85);
	this->setAutoFillBackground(true);
	
	playersFrame = new QGroupBox(this);
	playersFrame->resize(190,MAXPLAYERLINKS * 85 + 110);
	playersFrame->move(5,5);

	localPlayerIcon = new QLabel(playersFrame);
	localPlayerIcon->resize(60,80);
	localPlayerIcon->move(5, 25);

	localPlayerNumber = new QLabel(playersFrame);
	localPlayerNumber->resize(20,20);
	localPlayerNumber->move(5, 85);
	localPlayerNumber->setStyleSheet("font-weight: bold; color: blue; font-size: 20px");
	localPlayerNumber->setToolTip("Your number");

	localPlayerMute = new QLabel(playersFrame);
	localPlayerMute->resize(20,20);
	localPlayerMute->move(25, 85);
	localPlayerMute->setPixmap(QPixmap("icons/mute.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	localPlayerMute->setToolTip("You're mute");

	localPlayerTeam = new QLabel(playersFrame);
	localPlayerTeam->resize(20,20);
	localPlayerTeam->move(50, 85);
	localPlayerTeam->setStyleSheet("font-weight: bold; color: blue; font-size: 20px");
	localPlayerTeam->setToolTip("The team you're in");

	localPlayerName = new QLabel(ctrl->localPlayerName.get(), playersFrame);
	localPlayerName->resize(115, 20);
	localPlayerName->move (70, 25);

	localPlayerRemark = new QLabel(playersFrame);
	localPlayerRemark->resize(115, 20);
	localPlayerRemark->move (70, 40);
	localPlayerRemark->setText("(Local Player)");
	
	localEchoLabel = new QLabel("Local Echo Volume", playersFrame);
	localEchoLabel->move(70, 65);
	localEchoVolume = new QSlider(Qt::Horizontal, playersFrame);
	localEchoVolume->resize(115, 20);
	localEchoVolume->setTickPosition(QSlider::TicksBelow);
	localEchoVolume->setTickInterval(25);
	localEchoVolume->setMinimum(0);
	localEchoVolume->setMaximum(100);
	localEchoVolume->setSliderPosition(ctrl->localEchoVolume.get());
	localEchoVolume->move(70,85);
	connect(localEchoVolume, SIGNAL(valueChanged(int)), this, SLOT(localEchoChanged(int)));
	
	for (int i=0; i<MAXPLAYERLINKS; i++) {
		remotePlayerIcon[i] = new QLabel(playersFrame);
		remotePlayerIcon[i]->resize(60, 80);
		remotePlayerIcon[i]->move(5, 110 + i * 85);

		remotePlayerNumber[i] = new QLabel(playersFrame);
		remotePlayerNumber[i]->resize(20,20);
		remotePlayerNumber[i]->move(5, 170 + i * 85);
		remotePlayerNumber[i]->setStyleSheet("font-weight: bold; color: blue; font-size: 20px");
		remotePlayerNumber[i]->setToolTip("The number of this player");

		remotePlayerMute[i] = new QLabel(playersFrame);
		remotePlayerMute[i]->resize(20,20);
		remotePlayerMute[i]->move(25, 170 + i * 85);
		remotePlayerMute[i]->setPixmap(QPixmap("icons/mute.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		remotePlayerMute[i]->setToolTip("This player is mute");

		remotePlayerTeam[i] = new QLabel(playersFrame);
		remotePlayerTeam[i]->resize(20,20);
		remotePlayerTeam[i]->move(50, 170 + i * 85);
		remotePlayerTeam[i]->setStyleSheet("font-weight: bold; color: blue; font-size: 20px");
		remotePlayerTeam[i]->setToolTip("The team this player is in");

		/*
		remotePlayerModified[i] = new QLabel(playersFrame);
		remotePlayerModified[i]->resize(20,20);
		remotePlayerModified[i]->move(45, 170 + i * 85);
		remotePlayerModified[i]->setPixmap(QPixmap("icons/phone.png").scaled(20,20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		*/
		remotePlayerName[i] = new QLabel(playersFrame);
		remotePlayerName[i]->resize(115, 20);
		remotePlayerName[i]->move (70, 110 + i * 85);

		remotePlayerStatus[i] = new QLabel(playersFrame);
		remotePlayerStatus[i]->resize(115, 20);
		remotePlayerStatus[i]->move (70, 125 + i * 85);

		remotePlayerAddress[i] = new QLabel(playersFrame);
		remotePlayerAddress[i]->resize(115, 20);
		remotePlayerAddress[i]->move (70, 140 + i * 85);

		remotePlayerVolume[i] = new QSlider(Qt::Horizontal, playersFrame);
		remotePlayerVolume[i]->resize(115, 20);
		remotePlayerVolume[i]->move (70, 165 + i * 85);
		remotePlayerVolume[i]->setTickPosition(QSlider::TicksBelow);
		remotePlayerVolume[i]->setTickInterval(25);
		remotePlayerVolume[i]->setMinimum(0);
		remotePlayerVolume[i]->setMaximum(100);
		remotePlayerVolume[i]->setSliderPosition(ctrl->remotePlayerVolume.get(i));
		connect(remotePlayerVolume[i], SIGNAL(valueChanged(int)), this, SLOT(volumeChanged()));
	}
	// Start the timers 
	// FIXME: Do this event based instead of polling!
	updateTimer = new QTimer;
	updateTimer->setInterval(1000);
	updateTimer->setSingleShot(false);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(update()));
	updateTimer->start();
	connect(ctrl, SIGNAL(teamChange()), this, SLOT(update()));
	connect(ctrl, SIGNAL(remotePlayerNameChange(int)), this, SLOT(update()));
	connect(ctrl, SIGNAL(localPlayerNameChange()), this, SLOT(update()));
	
	wgetTimer = new QTimer;
	wgetTimer->setInterval(5000);
	wgetTimer->setSingleShot(false);
	connect(wgetTimer, SIGNAL(timeout()), this, SLOT(startWget()));
	wgetTimer->start();

}

PlayerComLink::~PlayerComLink() 
{
	
}

void PlayerComLink::startWget()
{
	//qDebug("PlayerComLink: Starting wget");
	if (ctrl->useServer.get() && !complete) {
		if (wget->state() != QProcess::NotRunning) qDebug() << "PlayerComLink: Wget did not terminate in time!";
		else {
			// Activate external wget to read the desired data from a http-server
			QString program = "wget";
			QString login = QString("table=")+ctrl->tablename.get()+QString("&name=")+ctrl->localPlayerName.get()+QString("&password=")+
							ctrl->passPhrase.get()+QString("");
			// Remark: Omit the '' normally used with the bash call of wget - no bash in between!
			QStringList arguments;
			arguments << "--post-data" << login.toUtf8() << ctrl->punchingServer.get() << "-O" << ctrl->dir[DIR_WORKING].filePath("remoteplayers.cfg");
			wget->start(program, arguments);
		}		
	}	
}

void PlayerComLink::update()
{
	QString tablename = ctrl->tablename.get();
	if (tablename == "") tablename = "<UNKNOWN TABLE>";
	playersFrame->setTitle(tablename);
	QPixmap unknownIcon("icons/unknown.png");
	QPixmap gamemasterIcon("icons/gamemaster.png");
	QPixmap empty(60,80);
	empty.fill();
	localPlayerName->setText(ctrl->localPlayerName.get());
	QPixmap playerIcon;
	// Define player signs first
	if (ctrl->mode.get() == MODE_MASTER) {
		playerIcon.load("icons/gamemaster.png");
		if(ctrl->playerSign.set('M', 0)) emit(ctrl->playerSignChange(0)); // FIXME: Playersigns are not used at the moment?
		for (int p=0; p<MAXPLAYERLINKS; p++) if(ctrl->playerSign.set('1' + p, p+1)) emit(ctrl->playerSignChange(p+1));
	}
	else {
		playerIcon.load(ctrl->dir[DIR_WORKING].filePath("playericon.png"));
		char playerSign = '1';
		QString lname = ctrl->localPlayerName.get();
		int p;
		for (p=0; p<MAXPLAYERLINKS; p++) {
			QString rname = ctrl->remotePlayerName.get(p);
			if (rname == "GAMEMASTER") {if(ctrl->playerSign.set('M', p+1)) emit(ctrl->playerSignChange(p+1));}
			else if(rname == "" || lname < rname) {
				if(ctrl->playerSign.set(playerSign++, 0)) emit(ctrl->playerSignChange(0));
				// Finish the loop
				for (; p<MAXPLAYERLINKS; p++) if(ctrl->playerSign.set(playerSign++, p+1)) emit(ctrl->playerSignChange(p+1));
				break;
			} 
			if(ctrl->playerSign.set(playerSign++, p+1)) emit(ctrl->playerSignChange(p+1));
			if (p==MAXPLAYERLINKS-1) {if(ctrl->playerSign.set(playerSign++, 0)) emit(ctrl->playerSignChange(0));}
		}
	}
	localPlayerNumber->setText(QString(ctrl->playerSign.get(0)));
	
	if (!playerIcon.isNull()) localPlayerIcon->setPixmap(playerIcon.scaled(60, 80, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	if (ctrl->mute.get()) localPlayerMute->show(); else localPlayerMute->hide();
	localPlayerTeam->setText(QString(ctrl->team.get(0)+'A'));
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		if (ctrl->remotePlayerName.get(p) == "") {
			// The player slot is unsued
			remotePlayerStatus[p]->setText(QString("Unused"));
			remotePlayerIcon[p]->setPixmap(empty);
			remotePlayerMute[p]->hide();
			remotePlayerNumber[p]->hide();
			remotePlayerTeam[p]->hide();
			remotePlayerVolume[p]->hide();
		} else {
			switch (ctrl->connectionStatus.get(p)) {
				case CON_DISCONNECTED:
					// Still at least one registered player unconnected? So we're _not_ complete
					complete = false;
					if (ctrl->remotePlayerAddress.get(p).isNull()) {
						if (ctrl->useServer.get()) remotePlayerStatus[p]->setText(QString("Getting IP")); 
						else remotePlayerStatus[p]->setText(QString("No IP")); 
					}
					else remotePlayerStatus[p]->setText(QString("Connecting"));
					remotePlayerMute[p]->show();
				break;
				case CON_INIT: 
					remotePlayerStatus[p]->setText(QString("Handshaking")); 
					remotePlayerMute[p]->show();
					break;
				case CON_READY: 
					if (ctrl->isSendingVoice.get(p)) remotePlayerMute[p]->hide(); else remotePlayerMute[p]->show();
					remotePlayerStatus[p]->setText(QString("Ready")); 
					break;
			}
			remotePlayerAddress[p]->setText(ctrl->remotePlayerAddress.get(p).toString());
			remotePlayerName[p]->setText(ctrl->remotePlayerName.get(p));
			if (!ctrl->connectionStatus.get(p)) remotePlayerIcon[p]->setPixmap(unknownIcon.scaled(60, 80, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

			remotePlayerVolume[p]->show();
			remotePlayerVolume[p]->setSliderPosition(ctrl->remotePlayerVolume.get(p));

			remotePlayerNumber[p]->show();
			remotePlayerNumber[p]->setText(QString(ctrl->playerSign.get(p+1)));
			
			remotePlayerTeam[p]->show();
			remotePlayerTeam[p]->setText(QString((char)ctrl->team.get(p+1)+'A'));
		}
	}
}

void PlayerComLink::newPlayerIcon(int player, QByteArray data)
{
	assert (player>=0);
	assert (player< MAXPLAYERLINKS);
	QPixmap icon;
	icon.loadFromData(data);
	qDebug("PlayerComLink: Icon is %d pixels wide and %d pixels high", icon.width(), icon.height());
	assert(icon.width() == 60);
	assert(icon.height() == 80);
	remotePlayerIcon[player]->setPixmap(icon);
}

void PlayerComLink::volumeChanged()
{
	// Find out which slider we're from and set the proper entry
	QSlider * slider = qobject_cast<QSlider*>(sender());
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		if (slider == remotePlayerVolume[p]) ctrl->remotePlayerVolume.set(slider->value(), p);
	}
}

void PlayerComLink::wgetFinished(int exitCode)
{
	//qDebug("PlayerComLink: wget finished");
	if (exitCode != 0) {
		qWarning() << "PlayerComLink: wget didn't finish properly and produced output:";
		qWarning() << wget->readAllStandardError();
		return;
	}
	QFile ipaddresses(ctrl->dir[DIR_WORKING].filePath("remoteplayers.cfg"));
	if (!ipaddresses.exists()) {
		qWarning("PlayerComLink: Wget didn't produce an address config file!");
		return;
	}
	if (!ipaddresses.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning("PlayerComLink: Cannot read address config file!");
		return;
	}
	QString line, name;
	QHostAddress addr;
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		line = ipaddresses.readLine(256);
		if (line.length() > 0 && line.indexOf('/') == -1) {qWarning() << "PlayerComLink: wget got only" << line; return;}
		name = line.left(line.indexOf('/')); 
		name = name.trimmed();
		addr = QHostAddress(line.mid(line.indexOf('/')+1).trimmed()); 
		if (name != ctrl->localPlayerName.get() && (name != ctrl->remotePlayerName.get(p) || addr != ctrl->remotePlayerAddress.get(p) ) ) {
			//qDebug() << "Name:" << name;
			//qDebug() << "Address:" << addr;
			if (!name.isEmpty()) {
				if (ctrl->remotePlayerName.set(name, p)) emit(ctrl->remotePlayerNameChange(p)); // Emit signal only when name has changed
				// Don't overwrite a possibly correct ip with garbage from a failed request
				if (!addr.isNull()  && !ctrl->remoteAddressOverride.get(p)) ctrl->remotePlayerAddress.set(addr, p);
			}
		}
	}
	// When we get this far we now should have the player's names
	complete = true;
}


void PlayerComLink::localEchoChanged(int volume)
{
	//qDebug("PlayerComLink: Volume of local echo changed to %d%%", volume);
	ctrl->localEchoVolume.set(volume);
}

