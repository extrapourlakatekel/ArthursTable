#include "voiceeffect.h"

VoiceEffect::VoiceEffect(QWidget * parent) : QWidget(parent)
{
	this->resize(MASTERPANELWIDTH, (VOICEPRESELECTIONS) * 30 + 65);
	this->setAutoFillBackground(true);
	frame = new QGroupBox("Gamemaster Voice Effects", this);
	frame->resize(MASTERPANELWIDTH-10, (VOICEPRESELECTIONS) * 30 + 60);
	frame->move(5,5);
	selectorGroup = new QButtonGroup(frame);
	for (int i=0; i<VOICEPRESELECTIONS+1; i++) {
		masterEffectSelector[i] = new QCheckBox(frame);
		masterEffectSelector[i]->resize(20, 20);
		masterEffectSelector[i]->move (5, i*30 + 30);
		selectorGroup->addButton(masterEffectSelector[i], i);
		if (i==0) {
			masterEffectSelector[i]->setText("none");
			masterEffectSelector[i]->setChecked(true);
			masterEffectSelector[i]->resize(70, 20);
		} else {
			masterEffectSelector[i]->setText(QString("F")+QString::number(i+4));
			masterEffectSelector[i]->resize(50, 20);
		}
	}
	selectorGroup->setExclusive(true);

	for (int i=0; i<VOICEPRESELECTIONS; i++) {
		masterEffectButton[i] = new QPushButton(frame);
		masterEffectButton[i]->resize(MASTERPANELWIDTH - 80, 30);
		masterEffectButton[i]->move (65, i*30 + 55);
		masterEffectButton[i]->setText(ctrl->voiceEffectName.get(i));
		connect(masterEffectButton[i], SIGNAL(pressed()), this, SLOT(effectButtonPressed()));
	}

	connect(selectorGroup, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(gamemasterEffectChanged(QAbstractButton *)));
	connect(ctrl, SIGNAL(functionKeyPressed(int)), this, SLOT(functionKeyPressed(int)));
	connect(ctrl, SIGNAL(functionKeyReleased(int)), this, SLOT(functionKeyReleased(int)));
	connect(ctrl, SIGNAL(voiceEffectChange(int)), this, SLOT(voiceEffectChange(int)));
}

void VoiceEffect::gamemasterEffectChanged(QAbstractButton * button)
{
	qDebug("VoiceEffect: Gamemaster's effect changed");
	QCheckBox * box = qobject_cast<QCheckBox*>(button);
	for (int i=0; i < VOICEPRESELECTIONS + 1; i++) if (box == masterEffectSelector[i]) {
		ctrl->voiceEffectSelected.set(i-1); // -1 means no effect selected
		qDebug("VoiceEffect: Set master's voice effect to %d", i-1);
		if (i>0) {
			oldEchoVolume = ctrl->localEchoVolume.get();
			ctrl->localEchoVolume.set(100); // Set the local echo to 100%
			emit(ctrl->localEchoVolumeChange());
		} else {
			ctrl->localEchoVolume.set(oldEchoVolume2); // Set the old local echo volume
			emit(ctrl->localEchoVolumeChange());
		}
		break;
	}
}

void VoiceEffect::functionKeyPressed(int key)
{
	if (key < Qt::Key_F5 || key > Qt::Key_F12) return; // Not for us - we use F5 to F12 only
	ctrl->voiceEffectSelected.set(key - Qt::Key_F5);
	qDebug("VoiceEffect: Set master's voice effect to %d", key - Qt::Key_F5);
	masterEffectSelector[key - Qt::Key_F5 + 1]->setChecked(true);
	// Enable local echo so that the gamemaster can hear his performance
	oldEchoVolume2 = ctrl->localEchoVolume.get();
	ctrl->localEchoVolume.set(100); // Set the local echo to 100%
	emit(ctrl->localEchoVolumeChange());

}

void VoiceEffect::functionKeyReleased(int key)
{
	if (key < Qt::Key_F5 || key > Qt::Key_F12) return; // Not for us - we use F5 to F12 only
	ctrl->voiceEffectSelected.set(-1);
	qDebug("VoiceEffect: Set master's voice effect to 0");
	masterEffectSelector[0]->setChecked(true);
	// Set the local echo to old volume
	ctrl->localEchoVolume.set(oldEchoVolume2); // Set the old local echo volume
	emit(ctrl->localEchoVolumeChange());
}

void VoiceEffect::voiceEffectChange(int effectNr)
{
	qDebug("VoiceEffect: Received Name change");
	assert(effectNr >= 0); 
	assert(effectNr < VOICEPRESELECTIONS);
	masterEffectButton[effectNr]->setText(ctrl->voiceEffectName.get(effectNr));
}

void VoiceEffect::effectButtonPressed()
{
	// First: Find out which button has been pressed
	QPushButton * button = qobject_cast<QPushButton*>(sender());
	int pressedButton = -1;
	for (int i=0; i < VOICEPRESELECTIONS; i++) if (button == masterEffectButton[i]) {pressedButton = i; break;}
	assert (pressedButton != -1);
	qDebug("VoiceEffect: Effect button nr. %d pressed", pressedButton);
	emit(ctrl->configureEffect(pressedButton));
}