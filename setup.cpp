#include "setup.h"

AudioSetup::AudioSetup(int width, QWidget * parent) : QGroupBox(parent)
{
	int ypos = 30;
	this->setTitle("Audio Settings");
	paRelaxSpinBox = new QSpinBox(this);
	paRelaxSpinBox->setRange(0, 4);
	paRelaxSpinBox->setValue(ctrl->paRelaxation.get());
	paRelaxSpinBox->resize(40,20);
	paRelaxSpinBox->move(5, ypos);
	paRelaxLabel = new QLabel("Latency Relaxation (Needs Restart)", this);
	paRelaxLabel->resize(width-10-40, 20);
	paRelaxLabel->move(50, ypos);
	ypos += paRelaxSpinBox->height();
	this->resize(width, ypos+10);
}

void AudioSetup::store()
{
	ctrl->paRelaxation.set(paRelaxSpinBox->value());
}

TrackerSetup::TrackerSetup(int width, QWidget * parent) : QGroupBox(parent)
{
	int ypos = 30;
	this->setTitle("Headtracker Settings");
	trackerLabel = new QLabel("Tracker", this);
	trackerLabel->move(5, ypos);
	deviceLabel = new QLabel("Device", this);
	deviceLabel->move((width-10)/2, ypos);
	ypos += trackerLabel->height();
	trackerBox = new QComboBox(this);
	trackerBox->resize((width-10)/2-5, 30);
	trackerBox->move(5,ypos);
	deviceBox = new QComboBox(this);
	deviceBox->resize((width-10)/2-5, 30);
	deviceBox->move((width-10)/2, ypos);
	QStringList trackerList, deviceList;
	trackerList << "none" << "BNO055"; // List in same order as defined in headtracker.h
	// FIXME: Add other trackers and move this to the tracker module!
	trackerBox->addItems(trackerList);
	trackerBox->setCurrentIndex(ctrl->headTrackerType.get());
	listDevices(ctrl->headTrackerType.get());
	connect(trackerBox, SIGNAL(currentIndexChanged(int)), this, SLOT(listDevices(int)));
	ypos += deviceBox->height() + 5;
	this->resize(width, ypos+10);
}

void TrackerSetup::store()
{
	// Emit init signal only if changes were made
	if (ctrl->headTrackerType.set((TrackerType)trackerBox->currentIndex()) ||
		ctrl->headTrackerDevice.set("/dev/" + deviceBox->currentText()) ) emit(ctrl->initHeadtracker());
}

void TrackerSetup::listDevices(int index)
{
	qDebug("TrackerSetup: Listing devices");
	deviceBox->clear();
	QDir devices("/dev");
	QStringList filter;
	switch (TrackerType(index)) {
		case TRACKER_NONE : break;
		case TRACKER_BNO055 : 
			filter << "ttyUSB*";
			devices.setNameFilters(filter);
			deviceBox->addItems(devices.entryList(QDir::System, QDir::Name)); 
			break;
		default: qWarning("TrackerSetup: Requested unkown tracker!"); break;
	}
}

Setup::Setup(Headtracker * tracker, QWidget * parent) : QDialog(parent)
{
	int ypos = 0;
	int height = 5;
	const int width = 300;
	this->setWindowTitle("Setup");
	this->setModal(true);
	
	// Audio settings
	audioSetup = new AudioSetup(width-10, this);
	audioSetup->move(5, height);
	height += audioSetup->height() + 5;

	// Tracker Settings
	trackerSetup = new TrackerSetup(width-10, this);
	trackerSetup->move(5,height);
	height += trackerSetup->height() + 5;
	connect(trackerSetup, SIGNAL(initTracker()), tracker, SLOT(init()));

	// Network Settings
	comGroupBox = new QGroupBox("Network Settings", this);
	ypos = 0;
	tableName = new QLabel("Tablename: " + ctrl->tablename.get(), comGroupBox);
	tableName->move(5,30);
	ypos += tableName->height() + 30;
	
	playerName = new QLineEdit(ctrl->localPlayerName.get(), comGroupBox);
	playerName->setPlaceholderText("Local charakters name");
	playerName->resize(width-20, 20);
	playerName->move(5, ypos);
	ypos += playerName->height() + 10;

	passphrase = new QLineEdit(ctrl->passPhrase.get(), comGroupBox);
	passphrase->move(5, ypos);
	passphrase->resize(width-20,20);
	passphrase->setPlaceholderText("Passphrase");
	passphrase->setEchoMode(QLineEdit::Password);
	ypos += passphrase->height() + 15;
	
	useServerCheck = new QCheckBox("Use punching server", comGroupBox);
	useServerCheck->move(5, ypos);
	ypos += useServerCheck->height();
	connect(useServerCheck, SIGNAL(toggled(bool)), this, SLOT(useServer(bool)));

	serverName = new QLineEdit(ctrl->punchingServer.get(), comGroupBox);
	serverName->move(5, ypos);
	serverName->resize(width-20,20);
	serverName->setPlaceholderText("Punching server");
	ypos += serverName->height() + 5;
	
	
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		remotePlayersName[p] = new QLineEdit("", comGroupBox);
		remotePlayersName[p]->resize(width-20,20);
		remotePlayersName[p]->move(5,ypos);
		ypos += remotePlayersName[p]->height() + 5;
		
		remotePlayersIP[p] = new QLineEdit("", comGroupBox);
		remotePlayersIP[p]->resize(width-50,20);
		remotePlayersIP[p]->move(5,ypos);
		ypos += remotePlayersIP[p]->height() + 10;
	}

	comGroupBox->resize(width-10, ypos+10);
	comGroupBox->move(5, height + 5);
	height += comGroupBox->height() + 5;
	
	// End of settings
	
	height += 10;
	// Done Button
	doneButton = new QPushButton("Done", this);
	doneButton->move(5,height);
	
	// Cancel Button
	cancelButton = new QPushButton("Cancel", this);
	cancelButton->move(doneButton->width()+10, height);
	height += doneButton->height()+10;
	
	// Frame
	this->setFixedSize(width, height);
	get();
	connect(doneButton, SIGNAL(pressed()), this, SLOT(done()));
	connect(cancelButton, SIGNAL(pressed()), this, SLOT(cancel()));
	this->show();
}

void Setup::get()
{
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		if (ctrl->remotePlayerName.get(p) == "" ) remotePlayersName[p]->setPlaceholderText("Player "+QString::number(p+1)+" Name");
		else remotePlayersName[p]->setText(ctrl->remotePlayerName.get(p));
		if (ctrl->remotePlayerAddress.get(p).isNull()) remotePlayersIP[p]->setPlaceholderText("Player "+QString::number(p+1)+" IP");
		else remotePlayersIP[p]->setText(ctrl->remotePlayerAddress.get(p).toString());
	}
	if (ctrl->useServer.get()) {
		useServerCheck->setCheckState(Qt::Checked); 
		serverName->setEnabled(true);	
		for (int p=0; p<MAXPLAYERLINKS; p++) {
			remotePlayersIP[p]->setEnabled(false);
			remotePlayersName[p]->setEnabled(false);
		}
	}
	else {
		useServerCheck->setCheckState(Qt::Unchecked);
		serverName->setEnabled(false);
		for (int p=0; p<MAXPLAYERLINKS; p++) {
			remotePlayersIP[p]->setEnabled(true);
			remotePlayersName[p]->setEnabled(true);
		}
	}
}

void Setup::done()
{
	audioSetup->store();
	trackerSetup->store();
	if (playerName->text().toUpper() == "GAMEMASTER") {
		if (ctrl->mode.set(MODE_MASTER)) emit(ctrl->gamemasterChange()); 
		if (ctrl->localPlayerName.set("GAMEMASTER")) emit(ctrl->localPlayerNameChange()); 
	} 
	else {
		if (ctrl->mode.set(MODE_PLAYER)) emit (ctrl->gamemasterChange());
		if (ctrl->localPlayerName.set(playerName->text().mid(0,32).simplified())) emit(ctrl->localPlayerNameChange()); // Limit names to 32 characters!
	}
	ctrl->useServer.set(useServerCheck->checkState() == Qt::Checked);
	ctrl->passPhrase.set(passphrase->text().toUtf8());
	ctrl->packetHeader.set(generatePacketHeader(ctrl->passPhrase.get())); // FIXME: IS this a good idea to do this here?
	// Do not overwrite entries if we're using a punching server
	if (!ctrl->useServer.get()) {
		// FIXME: Do alphabetic sorting - simple bubblesort will do (too lazy to code other)
		qDebug("Setup: Do some sorting");
		QString name[MAXPLAYERLINKS];
		QHostAddress address[MAXPLAYERLINKS];
		for (int p=0; p<MAXPLAYERLINKS; p++) {
			name[p] = remotePlayersName[p]->text().mid(0,32).simplified(); // Remove unnessecairy whitespaces and limit names to 32 characters!
			address[p] = QHostAddress(remotePlayersIP[p]->text());
		}
		bool finished;
		do {
			finished = true;
			qDebug() << "Setup: Names:" << name[0] << name[1] << name[2] << name[3] << name[4];
			for (int p=0; p<MAXPLAYERLINKS - 1; p++) {
				if (name[p] == name[p+1] && !name[p].isEmpty()) {	// We have a double entry => remove it! Would cause chaos otherwise!
					name[p+1] = ""; 
					address[p+1] = QHostAddress("");
					finished = false;
				}
				// Sort lexically, gamemaster sorts lexically first - always, Move empty entries to the end
				else if (name[p+1] == "GAMEMASTER" || ( name[p].isEmpty() && !name[p+1].isEmpty() ) || ( name[p] > name[p+1] && name[p] != "GAMEMASTER" && !name[p+1].isEmpty()) ) {
					// Swap the entries
					QString n = name[p]; name[p] = name[p+1]; name[p+1] = n;
					QHostAddress a = address[p]; address[p] = address[p+1]; address[p+1] = a;
					finished = false;
				}
			}
		} while (!finished);

		// Store the sorted values and tell everybody when changes have been done
		for (int p=0; p<MAXPLAYERLINKS; p++) {
			if (ctrl->remotePlayerName.set(name[p], p)) emit(ctrl->remotePlayerNameChange(p)); // Limit names to 32 characters!
			if (ctrl->remotePlayerAddress.set(address[p], p)) emit(ctrl->remotePlayerAddressChange(p));
		}
	} else {
		ctrl->punchingServer.set(serverName->text());
	}
	close();
	deleteLater();
}

void Setup::cancel()
{
	close();
	deleteLater();
}

void Setup::useServer(bool state)
{
	ctrl->useServer.set(state);
	get();

}

