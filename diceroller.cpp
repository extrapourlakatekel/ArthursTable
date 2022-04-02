#include "diceroller.h"

DiceRoller::DiceRoller(QWidget * parent) : QWidget(parent)
{
	this->resize(200,80);
	this->setAutoFillBackground(true);
	frame = new QGroupBox("Dice Roller", this);
	frame->resize(190,70);
	frame->move(5,0);
	ammountBox = new QComboBox(frame);
	ammountBox->resize(50,30);
	ammountBox->move(5, 30); 
	for (int i=1; i<=20; i++) ammountBox->addItem(QString::number(i));
	
	diceTypeBox = new QComboBox(frame);
	diceTypeBox->resize(65,30);
	diceTypeBox->move(65,30);
	QStringList diceTypes;
	diceTypes << "W2" << "W3" << "W4" << "W6" << "W8" << "W12" << "W20" << "W100";
	diceTypeBox->addItems(diceTypes);
	
	rollButton = new QPushButton(frame);
	rollButton->setIcon(QIcon("icons/dice.png"));
	rollButton->resize(30,30);
	rollButton->move(140, 30);
	srand(time(0)); // For roleplay dice this should be enough
	connect(rollButton, SIGNAL(pressed()), this, SLOT(roll()));
	connect(bulkbay, SIGNAL(newDiceRoll(int, QByteArray)), this, SLOT(newDiceRoll(int, QByteArray)));
}

void DiceRoller::roll()
{
	qDebug("DiceRoller: rolling");
	bool secret = ctrl->mode.get() == MODE_MASTER;
	QString resLong = "You rolled " + ammountBox->currentText() + diceTypeBox->currentText() + ": ";
	QString resShort = ammountBox->currentText() + diceTypeBox->currentText();
	if (!secret) resShort += ":";
	int die = diceTypeBox->currentText().mid(1).toInt();
	int sum = 0;
	for (int i=0; i<ammountBox->currentText().toInt(); i++) {
		int r = int64_t(rand()) * die / RAND_MAX + 1;
		if (!secret) resShort += QString::number(r) + " ";
		resLong += QString::number(r) + " ";
		sum += r;
	}
	if (ammountBox->currentText().toInt() > 1) {
		resLong += "Sum " +QString::number(sum);
		if (!secret) resShort += "S" +QString::number(sum);
	}
	for (int p=0; p<MAXPLAYERLINKS; p++) if (ctrl->connectionStatus.get(p))	bulkbay->addToTxBuffer(p, DICEROLL, resShort.toUtf8());
	emit(ctrl->displayText(resLong.toUtf8(), FMT_LOCAL, FMT_BOT));
}

void DiceRoller::newDiceRoll(int p, QByteArray res)
{
	// Convert short form to human readable
	QString resLong = ctrl->remotePlayerName.get(p);
	if (res.indexOf(":") == -1)  resLong += " secretly"; else res.replace(":",": ");
	resLong += " rolled ";
	res.replace("S", "Sum ");
	resLong += res;
	emit(ctrl->displayText(resLong.toUtf8(), p, FMT_BOT));
}

void DiceRoller::chatCommand(QString command)
{
	assert(command.length() > 0);
	if (command.at(0) != 'r') return; // No command for us!
	qDebug("DiceRoller: Command received");
	// FIXME: Parser!

}