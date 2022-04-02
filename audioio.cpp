#include "audioio.h"

AudioIo::AudioIo(QWidget * parent) : QWidget(parent)
{
	//setAutoFillBackground(!BORDERLESS);

	talktootherteams = false;
	// Input Control
	
	audioInFrame = new QGroupBox("Local Audio In", this);

	inputLevel = new VuMeter(QIcon("icons/microphone.png"), audioInFrame);
	inputLevel->setMinMax(log10(0.001), log10(1));
	
	mikeIcon = new QIcon("icons/microphone.png");
	muteIcon = new QIcon("icons/mute.png");
	muteButton = new QToolButton(audioInFrame);
	if (ctrl->mute.get()) muteButton->setIcon(*muteIcon);
	else muteButton->setIcon(*mikeIcon);
	connect(muteButton, SIGNAL(pressed()), this, SLOT(toggleMute()));

	inputSensitivity = new QSlider(Qt::Horizontal, audioInFrame);
	inputSensitivity->setMinimum(0);
	inputSensitivity->setMaximum(100);
	connect(inputSensitivity, SIGNAL(valueChanged(int)), this, SLOT(setInputSensitivity(int)));
	inputSensitivity->setSliderPosition(ctrl->inputSensitivity.get());

	voiceEffectBox = new QCheckBox(audioInFrame); 
	connect(voiceEffectBox, SIGNAL(stateChanged(int)), this, SLOT(voiceEffectStateChange(int)));

	voiceEffectButton = new QToolButton(audioInFrame);
	voiceEffectChange(); // Set the proper name
	connect(voiceEffectButton, SIGNAL(pressed()), this, SLOT(voiceEffectButtonPressed()));
	
	commEffectBox = new QCheckBox(audioInFrame);
	connect(commEffectBox, SIGNAL(stateChanged(int)), this, SLOT(commEffectStateChange(int)));
	
	// Ambient Sound & Music Control
	ambientFrame = new QGroupBox("Ambient Volume", this);

	ambientVolume = new QSlider(Qt::Horizontal, ambientFrame);
	ambientVolume->setMinimum(0);
	ambientVolume->setMaximum(100);
	connect(ambientVolume, SIGNAL(valueChanged(int)), this, SLOT(setAmbientVolume(int)));
	ambientVolume->setSliderPosition(ctrl->ambientVolume.get());

	// Output Control
	audioOutFrame = new QGroupBox("Local Audio Out", this);
	
	outputLevelLeft = new VuMeter("L", audioOutFrame);
	outputLevelLeft->setMinMax(log10(0.001), log10(1));

	outputLevelRight = new VuMeter("R", audioOutFrame);
	outputLevelRight->setMinMax(log10(0.001), log10(1));

	teamOrEffectChange();
	layoutChange();
	
	// Do the global harnessing
	connect(ctrl, SIGNAL(functionKeyPressed(int)), this, SLOT(functionKeyPressed(int)));
	connect(ctrl, SIGNAL(functionKeyReleased(int)), this, SLOT(functionKeyReleased(int)));
	connect(ctrl, SIGNAL(teamChange()), this, SLOT(teamOrEffectChange()));
	connect(ctrl, SIGNAL(commEffectChange()), this, SLOT(teamOrEffectChange()));
	connect(ctrl, SIGNAL(gamemasterChange()), this, SLOT(layoutChange()));
	connect(ctrl, SIGNAL(voiceEffectChange(int)), this, SLOT(voiceEffectChange()));
	// Start the timer for the VU animation
	timer = new QTimer;
	timer->setInterval(100);
	timer->setSingleShot(false);
	connect(timer, SIGNAL(timeout()), this, SLOT(updateVu()));
	timer->start();
	
}

void AudioIo::layoutChange()
{
	int ypos = 5;
	audioInFrame->move(5,5);
	inputLevel->repos(20,20,155,20);
	muteButton->resize(30, 30);
	muteButton->move(5,20);
	inputSensitivity->move(40, 40);	
	inputSensitivity->resize(135, 20);
	if (ctrl->mode.get() == MODE_MASTER) {
		audioInFrame->resize(190,115);
		voiceEffectBox->hide(); 
		voiceEffectButton->hide();
		commEffectBox->move(5,65);
	} else {
		audioInFrame->resize(190,140);
		voiceEffectBox->show(); 
		voiceEffectBox->move(5, 60);
		voiceEffectBox->resize(190,40);
		voiceEffectButton->show();
		voiceEffectButton->resize(145, 30);
		voiceEffectButton->move(40, 65);
		commEffectBox->move(5,95);
	}
	ypos += audioInFrame->height() + 5;
	
	// Ambient Sound Frame
	ambientFrame->move(5,ypos);
	ambientFrame->resize(190,50);
	ambientVolume->move(40,25);
	ambientVolume->resize(135,20);
	ypos += ambientFrame->height() + 5;
	
	// Audio Out Frame
	audioOutFrame->move(5,ypos);
	audioOutFrame->resize(190,65);
	outputLevelLeft->repos(15,20,160,20);
	outputLevelRight->repos(15,40,160,20);
	ypos += audioOutFrame->height();
	resize(200,ypos); 
	
}

void AudioIo::updateVu()
{
	//qDebug("Updating VU");
	inputLevel->setValue(log10(ctrl->vuLocalVoice.get()),ctrl->localVoiceActive.get());
	Stereo level;
	level = ctrl->vuOut.get();
	outputLevelLeft->setValue(log10(level.l));
	outputLevelRight->setValue(log10(level.r));
}

void AudioIo::setInputSensitivity(int sensitivity)
{
	ctrl->inputSensitivity.set(sensitivity);
}

void AudioIo::setAmbientVolume(int volume)
{
	ctrl->ambientVolume.set(volume);
}

void AudioIo::toggleMute()
{
	ctrl->mute.set(!ctrl->mute.get());
	if (ctrl->mute.get()) muteButton->setIcon(*muteIcon);
	else muteButton->setIcon(*mikeIcon);
}

void AudioIo::localPlayerNameChange()
{
	layoutChange();
	emit (ctrl->mainLayoutChange());
}

void AudioIo::functionKeyPressed(int key)
{
	if (key == Qt::Key_F1 && talktootherteams) commEffectBox->setCheckState(Qt::Checked);
	else if (key == Qt::Key_F2) voiceEffectBox->setCheckState(Qt::Checked);
}

void AudioIo::functionKeyReleased(int key)
{
	if (key == Qt::Key_F1) commEffectBox->setCheckState(Qt::Unchecked);
	else if (key == Qt::Key_F2) voiceEffectBox->setCheckState(Qt::Unchecked);
}

void AudioIo::voiceEffectStateChange(int state)
{
	qDebug("AudioIo: Voice effect state changed to %d", state);
	ctrl->voiceEffectSelected.set(state == Qt::Checked?0:-1);
}

void AudioIo::commEffectStateChange(int state)
{
	qDebug("AudioIo: Comm effect state changed to %d", state);
	ctrl->pushToTalk.set(state == Qt::Checked);
	if (state == Qt::Checked) commEffectBox->setText("Talking to other team"); else teamOrEffectChange();
	// If Comm Effect is toggled, toggle Voice Effect as well. A player does not contact the gamemaster via radio, does he?
	voiceEffectBox->setCheckState((Qt::CheckState)state);
}

void AudioIo::teamOrEffectChange()
{
	char myteam = ctrl->team.get(0);
	bool otherteams = false;
	for (int p=0; p<MAXPLAYERLINKS; p++) if (ctrl->remotePlayerName.get(p) != "" && ctrl->team.get(p+1) != myteam) otherteams = true;
	if (!otherteams) {commEffectBox->setText("All players are in one\nteam"); talktootherteams = false;}
	else {
		if (ctrl->comEffect.get() == COMEFFECT_NONE) {commEffectBox->setText("Can talk directly to other\nteam(s) at the moment"); talktootherteams = false;}
		else if (ctrl->comEffect.get() == COMEFFECT_MUTE) {commEffectBox->setText("Cannot talk to other\nteam(s) anyway"); talktootherteams = false;}
		else {commEffectBox->setText("Press F1 to talk to other\nteam(s)"); talktootherteams = true;}
	}
	commEffectBox->setEnabled(talktootherteams);
}

void AudioIo::voiceEffectChange()
{
	if (ctrl->voiceEffectName.get(0) == "") voiceEffectButton->setText("New...");	
	else voiceEffectButton->setText(ctrl->voiceEffectName.get(0));	
}

void AudioIo::voiceEffectButtonPressed()
{
	qDebug("AudioIo: Voice effect button pressed");
	emit(ctrl->configureEffect(0));
}
