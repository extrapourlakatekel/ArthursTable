#include "infopanel.h"
#include "assert.h"
#include <QPushButton>

InfoPanel::InfoPanel(QWidget * p) : QWidget(p)
{
	parent = p;
	this->setAutoFillBackground(true);
	tabWidget = new QTabWidget(this);
	
	npcViewer = new NpcViewer(tabWidget);
	tabWidget->addTab(npcViewer, "NPCs");
//	connect(npcViewer, SIGNAL(focusMe()), this, SLOT(focus()));
	
	
	// Add a tab for every image in the maps-directory, can be expanded later...
	QStringList mapList = ctrl->dir[DIR_PICS].entryList(QStringList() << "*.png" << "*.PNG" << "*.jpg" << "*.JPG", QDir::Files);
	qDebug () << "InfoPanel: Pictures in directory:" << mapList;
	for (int i=0; i<mapList.size(); i++) {
		viewer << new Viewer(mapList.at(i));
		connect(viewer[i], SIGNAL(focusMe()), this, SLOT(focus()));
		//connect(viewer[i], SIGNAL(visibilityChange()), this, SLOT(visibilityChange())); Testing for visibility changes don't use slots any more!
		if (viewer[i]->isVisible() || ctrl->mode.get() == MODE_MASTER) tabWidget->addTab(viewer[i], viewer[i]->getName());
	}
	oldteam = ctrl->team.get();
	assert(oldteam < MAXTEAMS);
	if (ctrl->activeTab.get(oldteam) != -1) tabWidget->setCurrentIndex(ctrl->activeTab.get(oldteam));
	
	fullscreenButton = new QToolButton(NULL);
	fullscreenButton->setCheckable(false);
	fullscreenButton->setToolTip("Fullscreen on");
	fullscreenButton->setIcon(QIcon("icons/fullscreen.png"));
	fullscreenButton->setAutoRaise(true);
	
	textInput = new QTextEdit(this);
	textOutput = new QTextEdit(this);
	textOutput->setReadOnly(true);
	textInput->show();
	
	setFocus(Qt::TabFocusReason);
	connect(textInput, SIGNAL(textChanged()), this, SLOT(textEntered()));
	textOutput->show();
	isFullscreen = false;

	connect(fullscreenButton, SIGNAL(pressed()), this, SLOT(toggleFullscreen()));
	connect(ctrl, SIGNAL(displayText(QString, int, bool)), this, SLOT(displayText(QString, int, bool)));
	connect(ctrl, SIGNAL(teamChange()), this, SLOT(visibilityChange())); // A team change may also cause a change in the visibility of the viewers
	connect(bulkbay, SIGNAL(newViewerEvent(int, QByteArray)), this, SLOT(newViewerEvent(int, QByteArray))); // FIXME: Probably move this to ctrl.h too!
	connect(bulkbay, SIGNAL(newChatMessage(int, QByteArray)), this, SLOT(newChatMessage(int, QByteArray)));
	tabWidget->setCornerWidget(fullscreenButton);
	tabWidget->show();
}

InfoPanel::~InfoPanel()
{
	//for (int i=0; i<viewer.size(); i++) {viewer.at(i)->close(); delete viewer.at(i);}
}

void InfoPanel::resizeEvent(QResizeEvent * event)
{
	qDebug("InfoPanel: Resize event");
	event->accept();
	setSize();
}

void InfoPanel::setSize()
{
	qDebug("InfoPanel: Size is: %d x %d", width(), height());
	if (!isFullscreen) {
		tabWidget->resize(width(), height()*2/3-10); 
		tabWidget->move(0, 5); 
		textOutput->resize(width(), height()*1/3-60);
		textOutput->move(0, height()*2/3);
		textInput->resize(width(), 60);
		textInput->move(0, height()-60);
	} else {
		tabWidget->resize(width(), height());
		tabWidget->move(0,0);
		// FIXME: Make this movable
		textOutput->resize(300,100);
		textOutput->move(20,height()-170);
		textInput->resize(300,50);
		textInput->move(20,height()-70);
	}
}

void InfoPanel::toggleFullscreen()
{
	isFullscreen = !isFullscreen;
	if (isFullscreen) {
		qDebug("InfoPanel: Showing fullscreen");
		fullscreenButton->setToolTip("Fullscreen off");
		fullscreenButton->setIcon(QIcon("icons/nofullscreen.png"));
		this->setParent(NULL);
		this->showFullScreen();
		qDebug("InfoPanel: Fullscreen size is: %d x %d", width(), height());
		emit(ctrl->mainLayoutChange());
		//setSize();
	} else {
		qDebug("InfoPanel: Normal view");
		fullscreenButton->setToolTip("Fullscreen on");
		fullscreenButton->setIcon(QIcon("icons/fullscreen.png"));
		this->setParent(parent);
		this->showNormal();
		emit(ctrl->mainLayoutChange());
	}
}

void InfoPanel::textEntered()
{
	QString text = textInput->toPlainText();
	if (text.startsWith("@")) {
		qDebug() << "Text:" << text;
		int matches = 0;
		int player = 0;
		QString sub = text.mid(1);
		QString message("");
		if (sub.indexOf(":") != -1) {sub = sub.mid(0, sub.indexOf(":")); message = text.mid(text.indexOf(":")+1);}
		qDebug() << "Substring" << sub << "Message:" << message;
		for (int p=0; p<MAXPLAYERLINKS; p++) {
			if (ctrl->remotePlayerName.get(p).contains(sub, Qt::CaseInsensitive)) {matches++; player = p;}
		}
		qDebug() << "Matches:" << matches;
		if (matches == 1) {
			if (text.indexOf(":") == -1) {
				textInput->clear();
				textInput->insertPlainText(QString("@") + ctrl->remotePlayerName.get(player) +":");
			}
		}
		if (message.endsWith("\n")) {
			textInput->clear();
			if (matches == 1) {
				message.chop(1);
				//textOutput->setCurrentCharFormat(fmt);
				if (message.length()>0) {
					textOutput->append(ctrl->localPlayerName.get() + " => " + ctrl->remotePlayerName.get(player) + ": " + message); 
					if (ctrl->connectionStatus.get(player)) {
						qDebug("Sending to player %d", player);
						if (bulkbay->addToTxBuffer(player, CHATMESSAGE, "#"+text.toUtf8()) < 0) displayText("Could not send to " + ctrl->remotePlayerName.get(player) + "!", FMT_WARNING, FMT_BOT);
					} else displayText(ctrl->remotePlayerName.get(player) + " is not connected!", FMT_WARNING, FMT_BOT);
				}
			} else displayText("Unknown player!" + sub, FMT_WARNING, FMT_BOT);
		}
	} else {
		if (text.endsWith("\n")) {
			textInput->clear();
			text.chop(1);
			if (text.startsWith("!")) {
				qDebug("SpecialAction!");
				if (text.length() > 1) emit(ctrl->chatCommand(text.mid(1)));
			} else { 
				displayText(ctrl->localPlayerName.get() + ": " + text, FMT_LOCAL, FMT_HUMAN);
				// Send the message
				if (text.length()>0) {
					for (int player=0; player < MAXPLAYERLINKS; player++) {
						if (ctrl->connectionStatus.get(player)) {
							qDebug("Sending to player %d", player);
							if (bulkbay->addToTxBuffer(player, CHATMESSAGE, "*"+text.toUtf8()) < 0) 
								displayText("Could not send to " + ctrl->remotePlayerName.get(player) + "!", FMT_WARNING, FMT_BOT);
						}
					}
				}
			}
		}	
	}
}

void InfoPanel::newChatMessage(int player, QByteArray data)
{
	qDebug("InfoPanel: Got chat message from %d", player);
	if (data.size()<1) return;
	QTextCharFormat fmt;
	if (data.at(0) == '#') displayText(ctrl->remotePlayerName.get(player) + "(privately): " + data.mid(1), player, FMT_HUMAN);
	else displayText(ctrl->remotePlayerName.get(player) + ": " + data.mid(1), player, FMT_HUMAN);
}

void InfoPanel::displayText(QString text, int format, bool bot)
{
	//qDebug("InfoPanel: Displaing text");
	QColor color;
	// FIXME: Simplify!
	switch (format) {
		case FMT_LOCAL:    color = QColor::fromRgb(0,0,0); break;
		case FMT_PLAYER_0: color = QColor::fromRgb(128,128,0); break;
		case FMT_PLAYER_1: color = QColor::fromRgb(0,139,139); break;
		case FMT_PLAYER_2: color = QColor::fromRgb(95,158,160); break;
		case FMT_PLAYER_3: color = QColor::fromRgb(128,0,128); break;
		case FMT_PLAYER_4: color = QColor::fromRgb(160,82,45); break;
		case FMT_PLAYER_5: color = QColor::fromRgb(112,128,144); break;
		case FMT_PLAYER_6: color = QColor::fromRgb(128,128,128); break;
		case FMT_PLAYER_7: color = QColor::fromRgb(188,143,143); break;
		case FMT_WARNING:  color = QColor::fromRgb(255,140,0); break;
		case FMT_ERROR:    color = QColor::fromRgb(255,0,0); break;
		default: qWarning("InfoPanel: Unknown format chosen!");
	}
	textOutput->setTextColor(color);
	textOutput->setFontItalic(bot); // When it is an automatically generated message: print in italic.
	textOutput->append(text.toUtf8()); 
}

bool InfoPanel::isFullScreen()
{
	return isFullscreen;
}

void InfoPanel::forwardKeyPressEvent(QEvent * event)
{
	QCoreApplication::sendEvent(textInput, event); 
}

void InfoPanel::keyPressEvent(QKeyEvent * event)
{
	// When we're fullscreen we have to forward pressed keys manually to the main window.
	// And as soon as we have an own keyPressedEvent-routine we have to do it anyway
	//qDebug("InfoPanel: Key press event");
	QCoreApplication::sendEvent(parent, event);
}

void InfoPanel::keyReleaseEvent(QKeyEvent * event)
{
	// When we're fullscreen we have to forward pressed keys manually to the main window
	QCoreApplication::sendEvent(parent, event);
}

void InfoPanel::escKeyPressed()
{
	if (isFullscreen) toggleFullscreen();

}

void InfoPanel::visibilityChange()
{
	// Also called when the team is changed
	// FIXME: This would be a good chance to sort the tabs alphabetically
	// Remember which tab is now open
	qDebug("InfoPanel: Visibility change");
	ctrl->activeTab.set(tabWidget->currentIndex(), oldteam);
	oldteam = ctrl->team.get();
	assert(oldteam < MAXTEAMS);
	// Remove all tabs first. Do not delete tab #1, that's the NPC viewer!
	for (int i=1; i<tabWidget->count(); i++) tabWidget->removeTab(i);
	for (int i=0; i<viewer.size(); i++) {
		if (viewer[i]->isVisible()) qDebug("    Viewer %d is visible", i);
		if (viewer[i]->isVisible() || ctrl->mode.get() == MODE_MASTER) tabWidget->addTab(viewer[i], viewer[i]->getName());
	}
	if (ctrl->activeTab.get(oldteam) != -1) tabWidget->setCurrentIndex(ctrl->activeTab.get(oldteam));
}

void InfoPanel::newViewerEvent(int player, QByteArray data) 
{
	if (ctrl->mode.get() == MODE_MASTER) {qWarning("InfoPanel: Got viewer message from player %d while beeing gamemaster - ignoring", player); return;}
	// Well let's find out who might be concerned
	if (data.length() < 2) {qWarning("InfoPanel: Got a too short viewer control"); return;}
	QByteArray filename = data.mid(1, data.at(0));
	qDebug() << "InfoPanel: Got message for" << filename;
	data = data.mid(data.at(0) + 1); // Remove the name and the filename's length
	// Ask all the windows whether they are the chosen one
	int i;
	for (i=0; i<viewer.size(); i++) {
		if (viewer[i]->getFilename().toUtf8() == filename) {
			if (viewer[i]->setConfig(&data)) visibilityChange(); // When the command affects the visibility of the tab, process this too
			break;
		}
	}
	if (i == viewer.size()) {
		// We've got a command for a viewer not present yet - construct one!
		viewer << new Viewer(filename);
		connect(viewer.last(), SIGNAL(focusMe()), this, SLOT(focus()));
		connect(viewer.last(), SIGNAL(visibilityChange()), this, SLOT(visibilityChange()));
		viewer.last()->setConfig(&data);
	}
}

void InfoPanel::focus()
// A window within the QTabWidget requires attention
{
	tabWidget->setCurrentWidget(qobject_cast<QWidget*>(sender()));
}

void InfoPanel::fileUpdate(int dir, QString filename)
{
	//FIXME: Do we need this? Viewer takes care of its own
}
