#include "fileview.h"

CFileView::CFileView(QWidget * parent) : QWidget (parent)
{
	m_pathComboBox = NULL;
	m_pathLabel = NULL;
	m_fileviewTable = NULL;
	m_dirButton = NULL;

	initLayout ();
}

CFileView::~CFileView()
{
	if (NULL != m_pathComboBox)
	{
		delete m_pathComboBox;
		m_pathComboBox = NULL;
	}
	if (NULL != m_pathLabel)
	{
		delete m_pathLabel;
		m_pathLabel = NULL;
	}
	if (NULL != m_fileviewTable)
	{
		delete m_fileviewTable;
		m_fileviewTable = NULL;
	}
	if (NULL != m_dirButton)
	{
		delete m_dirButton;
		m_dirButton = NULL;
	}
}

QComboBox * CFileView::createComboBox (const char * slot) const
{
	QComboBox * comboBox = new QComboBox;

	assert (NULL != comboBox);

	comboBox->setEditable(true);
	comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	comboBox->setMinimumContentsLength(1);

	if (NULL != slot)
	{
		connect(comboBox, SIGNAL(activated(QString)), this, slot);
	}

	return comboBox;
}

QLabel * CFileView::createLabel (const QString & strText) const
{
	QLabel * label = new QLabel;

	assert (NULL != label);

	label->setText(strText);

	return label;
}

QPushButton * CFileView::createButton (const QString &strText, const char * slot) const
{
	QPushButton * pushButton = new QPushButton;

	assert (NULL != pushButton);

	pushButton->setFixedSize(30, 30);

	QPixmap icon(strText);
	pushButton->setIcon(icon);
	pushButton->setIconSize(QSize(25, 25));
	pushButton->setStyleSheet("QPushButton{border: none; outline:none;}");
	pushButton->setFocusPolicy(Qt::NoFocus);

	if (NULL != slot)
	{
		connect (pushButton, SIGNAL(clicked()), this, slot);
	}

	return pushButton;
}

void CFileView::slotHeaderSectionClicked(int column)
{
	Qt::SortOrder sortOrder = m_fileviewTable->horizontalHeader()->sortIndicatorOrder();

	emit sortBySectionClicked (column, sortOrder);
}

void CFileView::slotWidgetItemDoubleClicked(int rows /* int columns = unused*/)
{
	assert (true == isRunning());
	assert (NULL != m_fileviewTable);
	if (false == isRunning())
	{
		return;
	}

	QTableWidgetItem * widgetItem = m_fileviewTable->item(rows, 0);
	assert (NULL != widgetItem);

	QString strCurrentPath = m_pathComboBox->currentText();
	int nLength = strCurrentPath.length();

	// consider root path ( / ) case either
	if (1 < nLength &&
		 '/' != strCurrentPath[nLength - 1])
	{
		strCurrentPath += "/";
	}

	strCurrentPath += widgetItem->text();

	QFileInfo fileInfo;
	fileInfo.setFile(strCurrentPath);

	if (true == fileInfo.isDir())
	{
		drawFileEntry (strCurrentPath);
	}
}

QTableWidget * CFileView::createTableWidget () const
{
	QTableWidget * tableWidget = new QTableWidget(0, 3);

	assert (NULL != tableWidget);

	QStringList labels;
	labels << tr("Filename") << tr("Size") << tr("Mod Time");

	tableWidget->setHorizontalHeaderLabels(labels);

	tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	tableWidget->verticalHeader()->hide();
	tableWidget->setSortingEnabled(true);
	tableWidget->sortByColumn(0, Qt::AscendingOrder);
	tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	tableWidget->setShowGrid(false);
	tableWidget->viewport()->setFocusPolicy(Qt::NoFocus);

	connect (tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(slotHeaderSectionClicked(int)));
	connect (tableWidget, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(slotWidgetItemDoubleClicked(int)));

	return tableWidget;
}

QTableWidgetItem * CFileView::createTableWidgetItem (const QString& strItem) const
{
	assert (0 != strItem.length());

	QTableWidgetItem * tableItem = new QTableWidgetItem;
	assert (NULL != tableItem);

	tableItem->setText(strItem);
	tableItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	tableItem->setFlags(Qt::ItemIsEnabled);

	return tableItem;
}

void CFileView::slotPathComboBoxActivated(QString strDirectory)
{
	drawFileEntry (strDirectory);
}

void CFileView::slotDirButtonClicked()
{
	QFileDialog fileDialog;

	fileDialog.setFileMode(QFileDialog::Directory);

	QString strDirectory = fileDialog.getExistingDirectory(this, tr("Find Files"), QDir::currentPath());

	drawFileEntry (strDirectory);
}

/*
 * void QTableModel::clearContents()
 *	{
 *		beginResetModel();
 *		for (int i = 0; i < tableItems.count(); ++i) {
 *			if (tableItems.at(i)) {
 *				tableItems.at(i)->view = 0;
 *				delete tableItems.at(i);
 *				tableItems[i] = 0;
 *			}
 *		}
 *		endResetModel();
 *	}
 *
 * Vector, List 등을 삭제할 때에는 qDeleteAll(begin,end) 함수가 내부의 모든 메모리를 해제하여준다.
 */
void CFileView::clearTableWidget ( void )
{
	if (false == m_fileEntries.isEmpty())
	{
		assert (NULL != m_fileviewTable);

		m_fileviewTable->clearContents();		// clearContents함수가 내부에서 모든 포인터를 해제한다. (바로 위 참조)
		m_fileviewTable->setRowCount(0);
		m_fileEntries.clear();
	}
}

void CFileView::updateItem (const QString& strDirectory)
{
	assert (NULL != m_pathComboBox);

	// findText: item Entry에서 해당 문자열을 찾지 못하면 -1을 반환
	if (-1 == m_pathComboBox->findText(strDirectory))
	{
		m_pathComboBox->addItem(strDirectory);
	}

	m_pathComboBox->setCurrentIndex(m_pathComboBox->findText(strDirectory));
}

void CFileView::drawFileEntry(const QString &strDirectory)
{
	assert (NULL != m_pathComboBox);
	assert (NULL != m_fileviewTable);

	if (true == checkValidationOfDirctory(strDirectory))
	{
		updateItem(strDirectory);
	}
	else	/* false == checkValidationOfDirectory(strDirectory) */
	{
		QMessageBox msgAlert;
		msgAlert.information(this, tr("Cannot Find a Directory"), tr("No Such a Directory: ") + strDirectory);

		m_pathComboBox->removeItem(m_pathComboBox->currentIndex());
		return;
	}

	clearTableWidget ();
	assert (true == m_fileEntries.isEmpty());

	{
		QDir dir;
		dir.setPath(strDirectory);

		m_fileEntries = dir.entryInfoList();
	}

	for (QFileInfoList::iterator iter = m_fileEntries.begin(); iter != m_fileEntries.end(); iter ++)
	{
		// ignore . , .. and symbolic link
		if ("." == iter->fileName() || ".." == iter->fileName() || true ==  iter->isSymLink())
		{
			qDebug() << "Ignore: " << iter->fileName();
			continue;
		}

		int row = m_fileviewTable->rowCount();
		m_fileviewTable->insertRow(row);

		QTableWidgetItem * nameItem = createTableWidgetItem(iter->fileName());
		QTableWidgetItem * sizeItem = createTableWidgetItem(QString::number(iter->size()));
		QTableWidgetItem * timeItem = createTableWidgetItem(iter->lastModified().toString("yyyy-MM-dd AP h:mm:ss"));

		assert (NULL != nameItem);
		assert (NULL != sizeItem);
		assert (NULL != timeItem);

		{
			QFileIconProvider iconProvider;
			nameItem->setIcon(iconProvider.icon (*iter));
		}

		m_fileviewTable->setSortingEnabled(false);
		m_fileviewTable->setItem(row, 0, nameItem);
		m_fileviewTable->setItem(row, 1, sizeItem);
		m_fileviewTable->setItem(row, 2, timeItem);
		m_fileviewTable->setSortingEnabled(true);

		qApp->processEvents(QEventLoop::EventLoopExec);

	}	/* for */

	emit beRun();
}

bool CFileView::checkValidationOfDirctory (const QString &strDirectory) const
{
	int nDirLength = strDirectory.length();

	assert (0 != nDirLength);
	if (0 == nDirLength)
	{
		return false;
	}

	QDir dir;

	dir.setPath(strDirectory);

	if (false == dir.isReadable())
	{
		qDebug() << "No Such a Directory: " << strDirectory;

		return false;
	}

	if (false == dir.isAbsolute())
	{
		qDebug() << "Cannot use relative Path: " << strDirectory;

		return false;
	}

	qDebug() << "Directory: " << strDirectory ;

	return true;
}

void CFileView::initLayout ( void )
{
	m_pathComboBox = createComboBox (SLOT(slotPathComboBoxActivated(QString)));
	m_pathLabel = createLabel(tr("Path"));
	m_fileviewTable = createTableWidget();
	m_dirButton = createButton(":/images/openFolder.png", SLOT(slotDirButtonClicked()));

	assert (NULL != m_pathComboBox);
	assert (NULL != m_pathLabel);
	assert (NULL != m_fileviewTable);
	assert (NULL != m_dirButton);


	QGridLayout * layout = new QGridLayout;
	assert (NULL != layout);

	layout->addWidget(m_pathLabel, 0,0);
	layout->addWidget(m_pathComboBox, 0, 1);
	layout->addWidget(m_dirButton, 0, 2);
	layout->addWidget(m_fileviewTable, 2, 0, 1, 3);

	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(10);

	setLayout(layout);
}

bool CFileView::isRunning ( void ) const
{
	if (0 == m_fileEntries.size())
	{
		return false;
	}
	else	/* 0 < m_fileEntries.size() */
	{
		return true;
	}
}


QString CFileView::getCurrentPath ( void ) const
{
	assert (NULL != m_pathComboBox);

	return m_pathComboBox->currentText();
}

int CFileView::getTableWidgetRows ( void ) const
{
	assert (NULL != m_fileviewTable);

	return m_fileviewTable->rowCount();
}
QString CFileView::getNameStringInTable (int rows) const
{
	assert (NULL != m_fileviewTable);

	int nRows = m_fileviewTable->rowCount();

	assert (rows < nRows);

	return m_fileviewTable->item(rows, 0)->text();
}
QString CFileView::getSizeStringInTable (int rows) const
{
	assert (NULL != m_fileviewTable);

	int nRows = m_fileviewTable->rowCount();

	assert (rows < nRows);

	return m_fileviewTable->item(rows, 1)->text();
}
QString CFileView::getTimeStringInTable (int rows) const
{
	assert (NULL != m_fileviewTable);

	int nRows = m_fileviewTable->rowCount();

	assert (rows < nRows);

	return m_fileviewTable->item(rows, 2)->text();
}

/* return rows. if cannot find, -1  */
int CFileView::findNameInTable (const QString& strName) const
{
	assert (NULL != m_fileviewTable);

	int nRows = m_fileviewTable->rowCount();

	for (int i = 0; i < nRows; i++)
	{
		if (strName == m_fileviewTable->item(i, 0)->text())
		{
			return i;
		}
	}

	return -1;
}


void CFileView::synchronizeColumnOrder (int column, Qt::SortOrder order)
{
	assert (NULL != m_fileviewTable);

	m_fileviewTable->sortByColumn(column, order);
}

void CFileView::updateTableWidgetItemColor (int rows, Qt::GlobalColor color)
{
	assert (NULL != m_fileviewTable);

	int nRows = m_fileviewTable->rowCount();

	assert (rows < nRows);
	if (rows >= nRows)
	{
		return;
	}

	QTableWidgetItem * nameItem = m_fileviewTable->item (rows, 0);
	QTableWidgetItem * sizeItem = m_fileviewTable->item (rows, 1);
	QTableWidgetItem * timeItem = m_fileviewTable->item (rows, 2);

	assert (NULL != nameItem);
	assert (NULL != sizeItem);
	assert (NULL != timeItem);

	nameItem->setTextColor(color);
	sizeItem->setTextColor(color);
	timeItem->setTextColor(color);
}
