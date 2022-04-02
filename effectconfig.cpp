#include "effectconfig.h"

EffectFrame::EffectFrame(int e, QWidget * parent) : QGroupBox(parent)
{
	effectSlot = e;
}

int EffectFrame::getRequiredWidth() {
	return requiredWidth;
}

int EffectFrame::getRequiredHeight() {
	return requiredHeight;
}

EffectNone::EffectNone(int effectSlot, QWidget * parent) : EffectFrame(effectSlot, parent)
{
	requiredWidth = 200;
	requiredHeight = 30;
	this->setTitle("No Effect Engine Selected");
	// Parse the actual setting
	if (ctrl->voiceEffectConfig.get(effectSlot).isEmpty() || ctrl->voiceEffectConfig.get(effectSlot).at(0) != VOICEEFFECT_NONE) {
		// Different effect in config than selected? Reset it!
		ctrl->voiceEffectConfig.set(QByteArray(1, VOICEEFFECT_NONE), effectSlot);
	}
}


EffectPitch::EffectPitch(int e, QWidget * parent) : EffectFrame(e, parent)
{
	requiredWidth = 400;
	requiredHeight = 90;
	this->setTitle("Simple Pitch");
	pitchSlider = new QSlider(Qt::Horizontal, this);
	pitchSlider->resize(380,20);
	pitchSlider->move(10, 30);
	pitchSlider->setTickInterval(1);
	pitchSlider->setTickPosition(QSlider::TicksBothSides);
	
	leftLabel = new QLabel("12 Halftones Down", this);
	leftLabel->resize(130,20);
	leftLabel->setAlignment(Qt::AlignLeft);
	leftLabel->move(10,50);
	centerLabel = new QLabel("Original", this);
	centerLabel->resize(130,20);
	centerLabel->setAlignment(Qt::AlignHCenter);
	centerLabel->move(135,50);
	rightLabel = new QLabel("12 Halftones Up", this);
	rightLabel->resize(130,20);
	rightLabel->setAlignment(Qt::AlignRight);
	rightLabel->move(250,50);

	// Parse the actual setting
	config = ctrl->voiceEffectConfig.get(effectSlot);
	if (config.length() != 2 || config.at(0) != VOICEEFFECT_PITCH) {
		// Different effect in config than selected? Reset it!
		config.clear();
		config.append(VOICEEFFECT_PITCH);
		config.append((char)0);
		ctrl->voiceEffectConfig.set(config, effectSlot);
	}
	pitchSlider->setRange(-12, 12);
	pitchSlider->setValue(config.at(1));
	connect(pitchSlider, SIGNAL(valueChanged(int)), this, SLOT(sliderMoved(int)));
}

void EffectPitch::sliderMoved(int pos)
{
	qDebug("EffectPitch: Slider moved");
	config[1] = pos;
	ctrl->voiceEffectConfig.set(config, effectSlot);
}

EffectSwarm::EffectSwarm(int e, QWidget * parent) : EffectFrame(e, parent)
{
	requiredWidth = 400;
	requiredHeight = 430;
	this->setTitle("Swarm");
	
	voicesTitleLabel = new QLabel("Number of Voices",this);
	voicesTitleLabel->resize(380, 20);
	voicesTitleLabel->setAlignment(Qt::AlignCenter);
	voicesTitleLabel->move(10,30);
	
	voicesSlider = new QSlider(Qt::Horizontal, this);
	voicesSlider->resize(380, 20);
	voicesSlider->move(10,50);
	voicesSlider->setTickInterval(1);
	voicesSlider->setTickPosition(QSlider::TicksBothSides);
	voicesSlider->setRange(2, 50);
	
	voicesLeftLabel = new QLabel("2", this);
	voicesLeftLabel->resize(40,20);
	voicesLeftLabel->setAlignment(Qt::AlignLeft);
	voicesLeftLabel->move(10,70);
	
	voicesCenterLabel = new QLabel("26", this);
	voicesCenterLabel->resize(40,20);
	voicesCenterLabel->setAlignment(Qt::AlignCenter);
	voicesCenterLabel->move(160,70);

	voicesRightLabel = new QLabel("50", this);
	voicesRightLabel->resize(40,20);
	voicesRightLabel->setAlignment(Qt::AlignRight);
	voicesRightLabel->move(360,70);
	
	pitchTitleLabel = new QLabel("General Pitch", this);
	pitchTitleLabel->resize(380,20);
	pitchTitleLabel->setAlignment(Qt::AlignCenter);
	pitchTitleLabel->move(10,100);

	pitchSlider = new QSlider(Qt::Horizontal, this);
	pitchSlider->resize(380, 20);
	pitchSlider->move(10,120);
	pitchSlider->setTickInterval(1);
	pitchSlider->setTickPosition(QSlider::TicksBothSides);
	pitchSlider->setRange(-12, 12);

	pitchLeftLabel = new QLabel("12 Halftones Lower", this);
	pitchLeftLabel->resize(120,20);
	pitchLeftLabel->setAlignment(Qt::AlignLeft);
	pitchLeftLabel->move(10,140);
	
	pitchCenterLabel = new QLabel("0", this);
	pitchCenterLabel->resize(20,20);
	pitchCenterLabel->setAlignment(Qt::AlignCenter);
	pitchCenterLabel->move(190,140);

	pitchRightLabel = new QLabel("12 Halftones Higher", this);
	pitchRightLabel->resize(120,20);
	pitchRightLabel->setAlignment(Qt::AlignRight);
	pitchRightLabel->move(270,140);

	pitchVariationTitleLabel = new QLabel("Pitch Variation", this);
	pitchVariationTitleLabel->resize(380, 20);
	pitchVariationTitleLabel->setAlignment(Qt::AlignCenter);
	pitchVariationTitleLabel->move(10,170);
	
	pitchVariationSlider = new QSlider(Qt::Horizontal, this);
	pitchVariationSlider->resize(380, 20);
	pitchVariationSlider->move(10,190);
	pitchVariationSlider->setTickInterval(2);
	pitchVariationSlider->setTickPosition(QSlider::TicksBothSides);
	pitchVariationSlider->setRange(0, 24);

	pitchVariationLeftLabel = new QLabel("0", this);
	pitchVariationLeftLabel->resize(20,20);
	pitchVariationLeftLabel->setAlignment(Qt::AlignLeft);
	pitchVariationLeftLabel->move(10,210);
	
	pitchVariationCenterLabel = new QLabel(QString(0x00B1) + "6 Halftones", this);
	pitchVariationCenterLabel->resize(120,20);
	pitchVariationCenterLabel->setAlignment(Qt::AlignCenter);
	pitchVariationCenterLabel->move(140,210);

	pitchVariationRightLabel = new QLabel(QString(0x00B1) + "12 Halftones", this);
	pitchVariationRightLabel->resize(120,20);
	pitchVariationRightLabel->setAlignment(Qt::AlignRight);
	pitchVariationRightLabel->move(270,210);

	delayVariationTitleLabel = new QLabel("Delay Variation", this);
	delayVariationTitleLabel->resize(380,20);
	delayVariationTitleLabel->setAlignment(Qt::AlignCenter);
	delayVariationTitleLabel->move(10,240);
	
	delayVariationSlider = new QSlider(Qt::Horizontal, this);
	delayVariationSlider->resize(380, 20);
	delayVariationSlider->move(10,260);
	delayVariationSlider->setTickInterval(5);
	delayVariationSlider->setTickPosition(QSlider::TicksBothSides);
	delayVariationSlider->setRange(0, 100);

	delayVariationLeftLabel = new QLabel("0", this);
	delayVariationLeftLabel->resize(20,20);
	delayVariationLeftLabel->setAlignment(Qt::AlignLeft);
	delayVariationLeftLabel->move(10,280);

	delayVariationCenterLabel = new QLabel("250ms", this);
	delayVariationCenterLabel->resize(120,20);
	delayVariationCenterLabel->setAlignment(Qt::AlignCenter);
	delayVariationCenterLabel->move(140,280);

	delayVariationRightLabel = new QLabel("500ms", this);
	delayVariationRightLabel->resize(120,20);
	delayVariationRightLabel->setAlignment(Qt::AlignRight);
	delayVariationRightLabel->move(270,280);

	volumeVariationTitleLabel = new QLabel("Volume Variation", this);
	volumeVariationTitleLabel->resize(380,20);
	volumeVariationTitleLabel->setAlignment(Qt::AlignCenter);
	volumeVariationTitleLabel->move(10,310);
	
	volumeVariationSlider = new QSlider(Qt::Horizontal, this);
	volumeVariationSlider->resize(380, 20);
	volumeVariationSlider->move(10,330);
	volumeVariationSlider->setTickInterval(5);
	volumeVariationSlider->setTickPosition(QSlider::TicksBothSides);
	volumeVariationSlider->setRange(0, 100);

	volumeVariationLeftLabel = new QLabel(QString(0x00B1) + "0%", this);
	volumeVariationLeftLabel->resize(40,20);
	volumeVariationLeftLabel->setAlignment(Qt::AlignLeft);
	volumeVariationLeftLabel->move(10,350);

	volumeVariationCenterLabel = new QLabel(QString(0x00B1) + "50%", this);
	volumeVariationCenterLabel->resize(120,20);
	volumeVariationCenterLabel->setAlignment(Qt::AlignCenter);
	volumeVariationCenterLabel->move(140,350);

	volumeVariationRightLabel = new QLabel(QString(0x00B1) + "100%", this);
	volumeVariationRightLabel->resize(120,20);
	volumeVariationRightLabel->setAlignment(Qt::AlignRight);
	volumeVariationRightLabel->move(270,350);

	rerollButton = new QPushButton("Reroll", this);
	rerollButton->resize(100,30);
	rerollButton->move(150, 380);
	
	// Parse the actual setting
	config = ctrl->voiceEffectConfig.get(effectSlot);
	if (config.length() != 156 || config.at(0) != VOICEEFFECT_SWARM) {
		// Different effect in config than selected? Reset it!
		config.clear();
		config.append(VOICEEFFECT_SWARM);
		config.append((char)2); // Number of voices
		config.append((char)0); // General pitch
		config.append((char)0); // Pitch variation
		config.append((char)0); // Delay variation
		config.append((char)0); // Volume variation
		config.append(QByteArray(150, (char)0)); // Space for random numbers used for producing the single pitches, volumes & delays
		reroll();
	}
	voicesSlider->setValue(config.at(1));
	pitchSlider->setValue(config.at(2));
	pitchVariationSlider->setValue(config.at(3));
	delayVariationSlider->setValue(config.at(4));
	volumeVariationSlider->setValue(config.at(5));
	connect(voicesSlider, SIGNAL(valueChanged(int)), this, SLOT(settingsChanged()));
	connect(pitchSlider, SIGNAL(valueChanged(int)), this, SLOT(settingsChanged()));
	connect(pitchVariationSlider, SIGNAL(valueChanged(int)), this, SLOT(settingsChanged()));
	connect(delayVariationSlider, SIGNAL(valueChanged(int)), this, SLOT(settingsChanged()));
	connect(volumeVariationSlider, SIGNAL(valueChanged(int)), this, SLOT(settingsChanged()));
	connect(rerollButton, SIGNAL(pressed()), this, SLOT(reroll()));
	
}

void EffectSwarm::settingsChanged()
{
	qDebug("EffectSwarm: Settings Changed");
	config[0] = (char)VOICEEFFECT_SWARM;
	config[1] = (char)voicesSlider->value(); // Number of voices
	config[2] = (char)pitchSlider->value(); // General pitch
	config[3] = (char)pitchVariationSlider->value(); // Pitch variation
	config[4] = (char)delayVariationSlider->value(); // Delay variation
	config[5] = (char)volumeVariationSlider->value(); // Volume variation
	ctrl->voiceEffectConfig.set(config, effectSlot);
}

void EffectSwarm::reroll()
{
	qDebug("EffectSwarm: Reroll");
	// generate an ammount of static randomness enough to power all voices with pitch, delay and volume so you can adjust with the same settings
	for (int i=5; i<156; i++) config[i] = numberBetween(-127, 127);
	ctrl->voiceEffectConfig.set(config, effectSlot);
}

EffectConfig::EffectConfig(QWidget * parent) : QDialog(parent)
{
	setWindowTitle("Effect Configurator");
	this->setModal(true);
	muteHint = new QLabel(this);
	muteHint->setText("You are now muted and local echo is enabled");
	muteHint->move(0,0);
	muteHint->setAlignment(Qt::AlignCenter);

	QStringList effectNames;
	effectNames << "None" << "Simple Pitch" << "Swarm";

	effectEngineBox = new QComboBox(this);
	effectEngineBox->resize(200,30);
	effectEngineBox->move(0,20);
	effectEngineBox->addItems(effectNames);
	
	effectName = new QLineEdit(this);
	effectName->resize(200,30);
	effectName->move(210, 20);
	effectName->setPlaceholderText("Effect Name");
	
	doneButton = new QPushButton("Done", this);
	doneButton->resize(60,30);
	connect(doneButton, SIGNAL(pressed()), this, SLOT(doneButtonPressed()));
	
	actualEngine = -1;
	actualSlot = -1;
	connect(effectName, SIGNAL(textEdited(QString)), this, SLOT(nameChanged(QString)));
	connect(ctrl, SIGNAL(configureEffect(int)), this, SLOT(configureEffect(int)));
	connect(effectEngineBox, SIGNAL(currentIndexChanged(int)), this, SLOT(showEffectEngine(int)));
}

void EffectConfig::configureEffect(int effectSlot)
{
	assert (effectSlot >=0 && effectSlot < VOICEPRESELECTIONS);
	if (actualSlot == effectSlot) return; // Do not restart if the same button is pressed twice
	actualSlot = effectSlot;
	qDebug("EffectConfig: Configuring effect for slot %d", effectSlot);

	int effectEngine;
	if (ctrl->voiceEffectConfig.get().isEmpty()) effectEngine = 0;
	else effectEngine = ctrl->voiceEffectConfig.get().at(0);

	effectEngineBox->setCurrentIndex(effectEngine); // Set the effect of this slot to the box

	// Mute the player and and enable loclal echo
	wasMuted = ctrl->mute.get();
	ctrl->mute.set(true);
	emit(ctrl->muteChange());
	oldSlot = ctrl->voiceEffectSelected.get();
	ctrl->voiceEffectSelected.set(effectSlot); // Activate the selected effect slot so we can hear the modifications
	oldEchoVolume = ctrl->localEchoVolume.get();
	ctrl->localEchoVolume.set(100); // Set the local echo to 100%
	emit(ctrl->localEchoVolumeChange());

	qDebug("EffectConfig: Starting GUI for effect engine %d", effectEngine);
	if (ctrl->voiceEffectName.get(effectSlot) == "new...") effectName->setText(""); else effectName->setText(ctrl->voiceEffectName.get(effectSlot));
	showEffectEngine(effectEngine);
	show();
}

void EffectConfig::nameChanged(QString text)
{
	assert (actualSlot >=0 && actualSlot < VOICEPRESELECTIONS);
	ctrl->voiceEffectName.set(text.simplified(), actualSlot);
	emit(ctrl->voiceEffectChange(actualSlot));

}

void EffectConfig::showEffectEngine(int effectEngine)
{
	qDebug("EffectConfig: Drawing GUI for effect engine %d", effectEngine);
	assert(effectEngine >= 0);
	assert(effectEngine < VOICEEFFECTS);
	if (actualEngine != -1) effectFrame->close();
	actualEngine = effectEngine;
	switch (actualEngine) {
			case VOICEEFFECT_NONE: effectFrame = (EffectFrame *) new EffectNone(actualSlot, this); break;
			case VOICEEFFECT_PITCH: effectFrame = (EffectFrame *) new EffectPitch(actualSlot, this); break;
			case VOICEEFFECT_SWARM: effectFrame = (EffectFrame *) new EffectSwarm(actualSlot, this); break;
			default: qFatal("EffectConfig: Unknown voiceeffect selected");
	}
	effectFrame->setAttribute(Qt::WA_DeleteOnClose);
	effectFrame->move(0,55);
	effectFrame->resize(max(410,effectFrame->getRequiredWidth()), effectFrame->getRequiredHeight()); // FIXME: Why can't the effect do this on its own?
	effectFrame->show();
	setFixedSize(effectFrame->width(), 90+effectFrame->height());
	doneButton->move(0, 60+effectFrame->height());
	muteHint->resize(width(), 20);
}


void EffectConfig::doneButtonPressed()
{
	qDebug("EffectConfig: Done button pressed");
	ctrl->mute.set(wasMuted);
	ctrl->localEchoVolume.set(oldEchoVolume);
	ctrl->voiceEffectSelected.set(oldSlot);
	if (actualEngine != -1) effectFrame->close();
	actualEngine = -1;
	actualSlot = -1;
	hide();
}

void EffectConfig::closeEvent(QCloseEvent *event)
{
	qDebug("EffectConfig: Shall close window");
	doneButtonPressed();
	event->ignore();
}
