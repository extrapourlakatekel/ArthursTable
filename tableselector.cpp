#include "tableselector.h"

TableSelector::TableSelector(QWidget * parent) : QDialog(parent)
{
	setWindowIcon(QIcon("icons/ArthursTable.png"));
	//qDebug() << ctrl->basedir;
	//qDebug() << ctrl->basedir.entryList(QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name);
	setFixedSize(220,120);
	setModal(true);
	setWindowTitle("Select Table");
	chooseLabel = new QLabel("Choose an existing table...", this);
	chooseLabel->move(0,0);
	existingTables = new QComboBox(this);
	existingTables->move (0,20);
	existingTables->resize(150, 30);
	existingTables->addItems(ctrl->basedir.entryList(QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name));
	okButton = new QPushButton("Ok", this);
	okButton->resize(50,30);
	okButton->move(160, 20);
	newLabel = new QLabel("... or create new one.", this);
	newLabel->move(0,60);
	tableNameEdit = new QLineEdit(this);
	//tableNameEdit->setInputMask(QString("NNNnnnnnnnnnnnnnnnnnnnnn")); // FIXME: Use a validator to allow for a little more complex filenames...
	tableNameEdit->setPlaceholderText("New Tablename");
	tableNameEdit->clear();

	tableNameEdit->move(0,85);
	tableNameEdit->resize(150,20);
	newButton = new QPushButton("New", this);
	newButton->resize(50,30);
	newButton->move(160,80);
	connect(okButton, SIGNAL(pressed()),this, SLOT(open()));
	connect(newButton, SIGNAL(pressed()), this, SLOT(create()));
	connect(tableNameEdit, SIGNAL(editingFinished()), this, SLOT(create())); // Pressing return is valid too - don't have to push the button
	this->show();

}


void TableSelector::open()
{
	qDebug() << "TableSelector: Opening" << existingTables->currentText();
	ctrl->setTableName(existingTables->currentText());
	hide();
	emit(selected());
}

void TableSelector::create()
{
	if (ctrl->tablename.get() != "") return; // Ignore the signal - may be emitted twice and causes total chaos
	if (tableNameEdit->hasAcceptableInput() && tableNameEdit->text().size() > 0) {
		qDebug() << "TableSelector: Creating" << tableNameEdit->text();
		// We do not check whether the file already exists - if so simply open it...
		// FIXME: Check 
		ctrl->setTableName(tableNameEdit->text());
		hide();
		emit(selected());
	}
	else {
		// FIXME: Some kind of error message
	}
	//this->setModal(false);
}

void TableSelector::closeEvent(QCloseEvent * event)
{
	// If the user refuses to setup a table by closing the window - exit!
	//this->setModal(false);
	if (ctrl->dir[DIR_WORKING].path() == ".") emit(quit());
	event->accept();
}
