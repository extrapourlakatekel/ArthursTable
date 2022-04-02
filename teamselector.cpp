#include "teamselector.h"

TeamSelector::TeamSelector(QWidget * parent) : QWidget(parent)
{
	this->resize(MASTERPANELWIDTH,105+20*MAXPLAYERS);
	this->setAutoFillBackground(true);
	frame = new QGroupBox("Team Selector", this);
	frame->resize(MASTERPANELWIDTH-10,70+25 * MAXPLAYERS);
	frame->move(5,5);
	
	teamGroup = new QButtonGroup();
	for (int t=0; t<MAXTEAMS; t++) {
		teamsButton[t] = new QPushButton(frame);
		teamsButton[t]->resize(30,25*MAXPLAYERLINKS+25);
		teamsButton[t]->move(30*(t - MAXTEAMS) + MASTERPANELWIDTH - 15, 30);
		teamsButton[t]->setText(QString((char)t+'A'));
		teamsButton[t]->setStyleSheet("text-align: top");
		teamsButton[t]->setCheckable(true);
		teamsButton[t]->setFocusPolicy(Qt::NoFocus);
		teamGroup->addButton(teamsButton[t], t);
	}
	// Set the saved condition
	activeTeam = ctrl->team.get(0);
	assert(activeTeam>=0 && activeTeam <= MAXTEAMS);
	teamsButton[activeTeam]->setChecked(true);
	
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		playerNameLabel[p] = new QLabel("", frame);
		playerNameLabel[p]->resize(MASTERPANELWIDTH - 30*MAXTEAMS - 30, 25);
		playerNameLabel[p]->move(10,50 + p*25);
		buttonGroup[p] = new QButtonGroup(frame);
		//buttonGroup[p]->setExclusive(frame);
		for (int t=0; t<MAXTEAMS; t++) {
			teamSelectBox[p][t] = new QCheckBox(frame);
			teamSelectBox[p][t]->resize(20,20);
			teamSelectBox[p][t]->move( MASTERPANELWIDTH - 10+ (t - MAXTEAMS)*30, p*25+55);
			teamSelectBox[p][t]->hide();
			buttonGroup[p]->addButton(teamSelectBox[p][t], t); 
		}
		int team = ctrl->team.get(p+1);
		assert(team>=0 && team <= MAXTEAMS);
		teamSelectBox[p][team]->setChecked(true);
		connect (buttonGroup[p], SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(playerButtonPressed(QAbstractButton *)));
		remotePlayerNameChange(p);
	}
	effectBox = new QComboBox(frame);;
	effectBox->resize(MASTERPANELWIDTH-20,30);
	effectBox->move(5, MAXPLAYERLINKS*25+60);
	QStringList effects;
	effects << "Normal Communication" << "No Communication" << "Far Away" << "Radio Link" << "Telephone";
	effectBox->addItems(effects);

	// Harnessing
	connect(ctrl, SIGNAL(remotePlayerNameChange(int)), this, SLOT(remotePlayerNameChange(int)));
	connect(teamGroup, SIGNAL(buttonPressed(QAbstractButton *)), this, SLOT(teamButtonPressed(QAbstractButton *)));
	connect(ctrl, SIGNAL(functionKeyPressed(int)), this, SLOT(functionKeyPressed(int)));
	connect(effectBox, SIGNAL(currentIndexChanged(int)), this, SLOT(effectChanged(int)));
	update();
}

void TeamSelector::functionKeyPressed(int key)
// F3 can be used to cycle trough the teams
{
	if (key != Qt::Key_F3) return; // Not for us!
	qDebug("TeamSelector: F3 pressed");
	for (int i=0; i<MAXTEAMS; i++) if (teamsButton[i]->isChecked()) {qDebug("TeamSelector: teamsbutton %d is down", i); teamsButton[(i+1) % MAXTEAMS]->setChecked(true); break;}
	
}

void TeamSelector::remotePlayerNameChange(int p)
{
	assert(p>=0); assert(p<MAXPLAYERLINKS);
	if (ctrl->remotePlayerName.get(p) != "") {
		playerNameLabel[p]->setText(ctrl->remotePlayerName.get(p));
		for (int t=0; t<MAXTEAMS; t++) teamSelectBox[p][t]->show();
	}
	else {
		playerNameLabel[p]->setText("");
		for (int t=0; t<MAXTEAMS; t++) teamSelectBox[p][t]->hide();
	}
}

void TeamSelector::update()
{
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		if (teamSelectBox[p][activeTeam]->isChecked()) {
			playerNameLabel[p]->setStyleSheet("font-weight: bold");
		} else {
			playerNameLabel[p]->setStyleSheet("font-weight: normal");
		}
	}
}

void TeamSelector::teamButtonPressed(QAbstractButton * button)
{
	for (int t=0; t<MAXTEAMS; t++) if (button == qobject_cast<QAbstractButton*>(teamsButton[t])) activeTeam = t;
	qDebug("TeamSelector: Gamemaster changed to team %c", (char)activeTeam+'A');
	ctrl->team.set(activeTeam, 0);
	update();
	emit(ctrl->teamChange());
}

void TeamSelector::playerButtonPressed(QAbstractButton *)
{
	qDebug("TeamSelector: PlayerButton pressed");
	for (int p=0; p<MAXPLAYERLINKS; p++) {
		int team;
		assert (buttonGroup[p]->checkedId() >= 0 && buttonGroup[p]->checkedId() < MAXTEAMS);
		team = (char)buttonGroup[p]->checkedId();
		qDebug() << "Player" << ctrl->remotePlayerName.get(p) << "is now in team" << (char)team + 'A';
		ctrl->team.set(team, p+1);		
	}
	update();
	emit(ctrl->teamChange());
}

void TeamSelector::effectChanged(int effect)
{
	qDebug("TeamSelector: New effect chosen: %d", effect);
	if (ctrl->comEffect.set((ComEffect)effect)) emit(ctrl->commEffectChange()); // When the com effect is really new, tell all the others
}