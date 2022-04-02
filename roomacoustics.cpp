#include "roomacoustics.h"

RoomAcoustics::RoomAcoustics(QWidget * parent) : QWidget(parent)
{
	resize(200,70);
	setAutoFillBackground(true);
	frame = new QGroupBox("Room Acoustics", this);
	frame->resize(190,65);
	frame->move(5,5);
	effect = new QComboBox(frame);
	effect->resize(180,30);
	effect->move(5, 30); 
}

void RoomAcoustics::teamChange()
{
	
}
