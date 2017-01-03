/*
For general Sqliteman copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Sqliteman
for which a new license (GPL+exception) is in place.

Strategy for editable queries:-
If the table is WITHOUT ROWID, update record with matching primary key
	WITHOUT ROWID primary keys are unique, so there can only be one
If the table has a rowid and an INTEGER PRIMARY KEY column, update record with matching primary key
	INTEGER PRIMARY KEYs are unique, so there can only be one
If the table has a rowid but no INTEGER PRIMARY KEY column, and at least one of rowid, _rowid_, or oid is not used as a column name, update record with matching rowid
	rowids are unique, so there can only be one
Otherwise, we can't do it because we can't identify a unique record to update.
*/
#include <QClipboard>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollBar>
#include <QTreeWidgetItem>

#include "queryeditorwidget.h"
#include "querystringmodel.h"
#include "sqlparser.h"
#include "utils.h"

void QueryEditorWidget::resetModel()
{
	selectModel->clear();
	tabWidget->setCurrentIndex(0);
	termsTable->clear();
	termsTable->setRowCount(0); // clear() doesn't seem to do this
	ordersTable->clear();
	ordersTable->setRowCount(0); // clear() doesn't seem to do this
}

QStringList QueryEditorWidget::getColumns()
{
	QStringList columns;
	SqlParser * parser = Database::parseTable(m_table, m_schema);
	if (parser->m_hasRowid)
	{
		bool rowid = true;
		bool _rowid_ = true;
		bool oid = true;
		foreach (FieldInfo i, parser->m_fields)
		{
			if (i.name.compare("rowid", Qt::CaseInsensitive) == 0)
				{ rowid = false; }
			if (i.name.compare("_rowid_", Qt::CaseInsensitive) == 0)
				{ _rowid_ = false; }
			if (i.name.compare("oid", Qt::CaseInsensitive) == 0)
				{ oid = false; }
		}
		if (rowid)
			{ columns << QString("rowid"); m_rowid = "rowid"; }
		else if (_rowid_)
			{ columns << QString("_rowid_"); m_rowid = "_rowid_"; }
		else if (oid)
			{ columns << QString("oid"); m_rowid = "oid"; }
	}

	foreach (FieldInfo i, parser->m_fields) { columns << i.name; }
	delete parser;
	return columns;
}

void QueryEditorWidget::tableSelected(const QString & table)
{
	resetModel();
	m_table = table;
	m_columnList = getColumns();
	columnModel->setStringList(m_columnList);
	resizeWanted = true;
}

void QueryEditorWidget::resetTableList()
{
	tableList->clear();
	tableList->addItems(Database::getObjects("table", m_schema).keys());
// FIXME need to fix create parser before this will work
//	tableList->addItems(Database::getObjects("view", m_schema).keys());
	tableList->adjustSize();
	tableList->setCurrentIndex(0);
	if (tableList->count() > 0)
	{
		tableSelected(tableList->currentText());
	}
	else
	{
		resetModel();
	}
}

void QueryEditorWidget::schemaSelected(const QString & schema)
{
	m_schema = schema;
	resetTableList();
}

void QueryEditorWidget::resetSchemaList()
{
	schemaList->clear();
	schemaList->addItems(Database::getDatabases().keys());
	schemaList->adjustSize();
	schemaList->setCurrentIndex(0);
	if (schemaList->count() > 0)
	{
		schemaSelected(schemaList->currentText());
	}
	else
	{
		resetTableList();
	}
}

QueryEditorWidget::QueryEditorWidget(QWidget * parent): QWidget(parent)
{
	setupUi(this);

	columnModel = new QueryStringModel(this);
	selectModel = new QueryStringModel(this);
	columnView->setModel(columnModel);
	selectView->setModel(selectModel);

	termsTable->setColumnCount(3);
	termsTable->horizontalHeader()->hide();
	termsTable->verticalHeader()->hide();
	termsTable->setShowGrid(false);
	ordersTable->setColumnCount(3);
	ordersTable->horizontalHeader()->hide();
	ordersTable->verticalHeader()->hide();
	ordersTable->setShowGrid(false);

	connect(schemaList, SIGNAL(activated(const QString &)),
			this, SLOT(schemaSelected(const QString &)));
	connect(tableList, SIGNAL(activated(const QString &)),
			this, SLOT(tableSelected(const QString &)));
	connect(termMoreButton, SIGNAL(clicked()), this, SLOT(moreTerms()));
	connect(termLessButton, SIGNAL(clicked()), this, SLOT(lessTerms()));
	connect(addAllButton, SIGNAL(clicked()), this, SLOT(addAllSelect()));
	connect(addButton, SIGNAL(clicked()), this, SLOT(addSelect()));
	connect(removeAllButton, SIGNAL(clicked()), this, SLOT(removeAllSelect()));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(removeSelect()));
	connect(columnView, SIGNAL(doubleClicked(const QModelIndex &)),
			this, SLOT(addSelect()));
	connect(selectView, SIGNAL(doubleClicked(const QModelIndex &)),
			this, SLOT(removeSelect()));
	connect(orderMoreButton, SIGNAL(clicked()), this, SLOT(moreOrders()));
	connect(orderLessButton, SIGNAL(clicked()), this, SLOT(lessOrders()));
	QPushButton * resetButton = new QPushButton("Reset", this);
	connect(resetButton, SIGNAL(clicked(bool)), this, SLOT(resetClicked()));
	tabWidget->setCornerWidget(resetButton, Qt::TopRightCorner);
	initialised = false;
}

void QueryEditorWidget::setItem(QTreeWidgetItem * item)
{
	if (!initialised)
	{
		resetSchemaList();
		initialised = true;
	}
	if (item)
	{
		// invoked from context menu
		bool found = false;
		for (int i = 0; i < schemaList->count(); ++i)
		{
			if (schemaList->itemText(i) == item->text(1))
			{
				schemaList->setCurrentIndex(i);
				found = true;
				break;
			}
		}
		if (!found)
		{
			schemaList->setCurrentIndex(0);
		}
		if (m_schema != schemaList->currentText())
		{
			schemaSelected(schemaList->currentText());
			// force table select if same name table in different schema
			m_table = QString();
		}
		found = false;
		for (int i = 0; i < tableList->count(); ++i)
		{
			if (tableList->itemText(i) == item->text(0))
			{
				tableList->setCurrentIndex(i);
				found = true;
				break;
			}
		}
		if (!found)
		{
			tableList->setCurrentIndex(0);
		}
		if (m_table != item->text(0))
		{
			tableSelected(tableList->currentText());
		}
		schemaList->setEnabled(false);
		tableList->setEnabled(false);
	}
	else
	{
		// invoked from database menu
		schemaList->setEnabled(true);
		tableList->setEnabled(true);
	}
	resizeWanted = true;
}

QString QueryEditorWidget::statement()
{
	QString logicWord;
	QString sql = "SELECT\n";

	// columns
	if (selectModel->rowCount() == 0)
		sql += "* ";
	else
	{
		sql += Utils::q(selectModel->stringList(), "\"");
	}

	// Add table name
			sql += ("\nFROM " + Utils::q(m_schema) + "." +
					Utils::q(tableList->currentText()));

	// Optionaly add terms
	if (termsTable->rowCount() > 0)
	{
		// But first determine what is the chosen logic word (And/Or)
		(andButton->isChecked()) ? logicWord = " AND " : logicWord = " OR ";

		sql += "\nWHERE ";

		for(int i = 0; i < termsTable->rowCount(); i++)
		{
			QComboBox * fields =
				qobject_cast<QComboBox *>(termsTable->cellWidget(i, 0));
			QComboBox * relations =
				qobject_cast<QComboBox *>(termsTable->cellWidget(i, 1));
			QLineEdit * value =
				qobject_cast<QLineEdit *>(termsTable->cellWidget(i, 2));
			if (fields && relations && value)
			{
				if (i > 0) { sql += logicWord; }
				sql += Utils::q(fields->currentText());

				switch (relations->currentIndex())
				{
					case 0:	// Contains
						sql += (" LIKE " + Utils::like(value->text()));
						break;

					case 1: 	// Doesn't contain
						sql += (" NOT LIKE "
								+ Utils::like(value->text()));
						break;

					case 2:		// Equals
						sql += (" = " + Utils::q(value->text(), "'"));
						break;

					case 3:		// Not equals
						sql += (" <> " + Utils::q(value->text(), "'"));
						break;

					case 4:		// Bigger than
						sql += (" > " + Utils::q(value->text(), "'"));
						break;

					case 5:		// Smaller than
						sql += (" < " + Utils::q(value->text(), "'"));
						break;

					case 6:		// is null
						sql += (" ISNULL");
						break;

					case 7:		// is not null
						sql += (" NOTNULL");
						break;
				}
			}
		}
	}

	// optionally add ORDER BY clauses
	if (ordersTable->rowCount() > 0)
	{
		sql += "\nORDER BY ";
		for (int i = 0; i < ordersTable->rowCount(); i++)
		{
			QComboBox * fields =
				qobject_cast<QComboBox *>(ordersTable->cellWidget(i, 0));
			QComboBox * collators =
				qobject_cast<QComboBox *>(ordersTable->cellWidget(i, 1));
			QComboBox * directions =
				qobject_cast<QComboBox *>(ordersTable->cellWidget(i, 2));
			if (fields && collators && directions)
			{
				if (i > 0) { sql += ", "; }
				sql += Utils::q(fields->currentText());
				sql += " COLLATE ";
				sql += collators->currentText() + " ";
				sql += directions->currentText();
			}
		}
	}

	sql += ";";
	return sql;
}

QString QueryEditorWidget::deleteStatement()
{
	QString logicWord;
	QString sql = "DELETE\n";

	// Add table name
			sql += ("\nFROM " + Utils::q(m_schema) + "." +
					Utils::q(tableList->currentText()));

	// Optionaly add terms
	if (termsTable->rowCount() > 0)
	{
		// But first determine what is the chosen logic word (And/Or)
		(andButton->isChecked()) ? logicWord = " AND " : logicWord = " OR ";

		sql += "\nWHERE ";

		for(int i = 0; i < termsTable->rowCount(); i++)
		{
			QComboBox * fields =
				qobject_cast<QComboBox *>(termsTable->cellWidget(i, 0));
			QComboBox * relations =
				qobject_cast<QComboBox *>(termsTable->cellWidget(i, 1));
			QLineEdit * value =
				qobject_cast<QLineEdit *>(termsTable->cellWidget(i, 2));
			if (fields && relations && value)
			{
				if (i > 0) { sql += logicWord; }
				sql += Utils::q(fields->currentText());

				switch (relations->currentIndex())
				{
					case 0:	// Contains
						sql += (" LIKE " + Utils::like(value->text()));
						break;

					case 1: 	// Doesn't contain
						sql += (" NOT LIKE "
								+ Utils::like(value->text()));
						break;

					case 2:		// Equals
						sql += (" = " + Utils::q(value->text(), "'"));
						break;

					case 3:		// Not equals
						sql += (" <> " + Utils::q(value->text(), "'"));
						break;

					case 4:		// Bigger than
						sql += (" > " + Utils::q(value->text(), "'"));
						break;

					case 5:		// Smaller than
						sql += (" < " + Utils::q(value->text(), "'"));
						break;

					case 6:		// is null
						sql += (" ISNULL");
						break;

					case 7:		// is not null
						sql += (" NOTNULL");
						break;
				}
			}
		}
		sql += ";";
		return sql;
	}
	else
	{
		return NULL;
	}
}

void QueryEditorWidget::addAllSelect()
{
	QStringList list(columnModel->stringList());
	foreach (QString s, list)
	{
		if (s.compare(m_rowid))
		{
			selectModel->append(s);
			list.removeAll(s);
		}
	}
	columnModel->setStringList(list);
}

void QueryEditorWidget::addSelect()
{
	QItemSelectionModel *selections = columnView->selectionModel();
	if (!selections->hasSelection())
		return;
	QStringList list(columnModel->stringList());
	QString val;
	foreach (QModelIndex i, selections->selectedIndexes())
	{
		val = columnModel->data(i, Qt::DisplayRole).toString();
		selectModel->append(val);
		list.removeAll(val);
	}
	columnModel->setStringList(list);
}

void QueryEditorWidget::removeAllSelect()
{
	tableSelected(m_table);
}

void QueryEditorWidget::removeSelect()
{
	QItemSelectionModel *selections = selectView->selectionModel();
	if (!selections->hasSelection()) { return; }
	QStringList list(selectModel->stringList());
	QString val;
	foreach (QModelIndex i, selections->selectedIndexes())
	{
		val = selectModel->data(i, Qt::DisplayRole).toString();
		columnModel->append(val);
		list.removeAll(val);
	}
	selectModel->setStringList(list);
}

void QueryEditorWidget::moreTerms()
{
	int i = termsTable->rowCount();
	termsTable->setRowCount(i + 1);
	QComboBox * fields = new QComboBox();
	fields->addItems(m_columnList);
	termsTable->setCellWidget(i, 0, fields);
	QComboBox * relations = new QComboBox();
	relations->addItems(QStringList() << tr("Contains") << tr("Doesn't contain")
									  << tr("Equals") << tr("Not equals")
									  << tr("Bigger than") << tr("Smaller than")
									  << tr("Is null") << tr("Is not null"));
	termsTable->setCellWidget(i, 1, relations);
	connect(relations, SIGNAL(currentIndexChanged(const QString &)),
			this, SLOT(relationsIndexChanged(const QString &)));
	QLineEdit * value = new QLineEdit();
	termsTable->setCellWidget(i, 2, value);
	termsTable->resizeColumnsToContents();
	termLessButton->setEnabled(true);
	resizeWanted = true;
}

void QueryEditorWidget::lessTerms()
{
	int i = termsTable->rowCount() - 1;
	termsTable->removeRow(i);
	termsTable->resizeColumnsToContents();
	if (i == 0)
		termLessButton->setEnabled(false);
}

void QueryEditorWidget::moreOrders()
{
	int i = ordersTable->rowCount();
	ordersTable->setRowCount(i + 1);
	QComboBox * fields = new QComboBox();
	fields->addItems(m_columnList);
	ordersTable->setCellWidget(i, 0, fields);
	QComboBox * collators = new QComboBox();
	collators->addItem("BINARY");
	collators->addItem("NOCASE");
	collators->addItem("RTRIM");
	collators->addItem("LOCALIZED_CASE");
	collators->addItem("LOCALIZED");
	ordersTable->setCellWidget(i, 1, collators);
	QComboBox * directions = new QComboBox();
	directions->addItem("ASC");
	directions->addItem("DESC");
	ordersTable->setCellWidget(i, 2, directions);
	orderLessButton->setEnabled(true);
	resizeWanted = true;
}

void QueryEditorWidget::lessOrders()
{
	int i = ordersTable->rowCount() - 1;
	ordersTable->removeRow(i);
	if (i == 0)
		orderLessButton->setEnabled(false);
}

void QueryEditorWidget::relationsIndexChanged(const QString &)
{
	QComboBox * relations = qobject_cast<QComboBox *>(sender());
	for (int i = 0; i < termsTable->rowCount(); ++i)
	{
		if (relations == termsTable->cellWidget(i, 1))
		{
			QLineEdit * value =
				qobject_cast<QLineEdit *>(termsTable->cellWidget(i, 2));
			if (value)
			{
				switch (relations->currentIndex())
				{
					case 0: value->show(); // Contains
						return;

					case 1: value->show(); // Doesn't contain
						return;

					case 2: value->show(); // Equals
						return;

					case 3: value->show(); // Not equals
						return;

					case 4: value->show(); // Bigger than
						return;

					case 5: value->show(); // Smaller than
						return;

					case 6: value->hide(); // is null
						return;

					case 7: value->hide(); // is not null
						return;
				}
			}
		}
	}
}

void QueryEditorWidget::resetClicked()
{
	if (schemaList->isEnabled())
	{
		schemaList->setCurrentIndex(0);
		schemaSelected(schemaList->currentText());
	}
	if (tableList->isEnabled()) { tableList->setCurrentIndex(0); }
	tableSelected(tableList->currentText());
}

void QueryEditorWidget::copySql()
{
	QApplication::clipboard()->setText(statement());
}

void QueryEditorWidget::treeChanged()
{
	schemaList->clear();
	schemaList->addItems(Database::getDatabases().keys());
	bool found = false;
	for (int i = 0; i < schemaList->count(); ++i)
	{
		if (schemaList->itemText(i) == m_schema)
		{
			schemaList->setCurrentIndex(i);
			found = true;
			break;
		}
	}
	if (!found)
	{
		schemaList->setEnabled(true);
		schemaList->setCurrentIndex(0);
		if (schemaList->count() > 0)
		{
			schemaSelected(schemaList->currentText());
		}
		else
		{
			resetTableList();
		}
	}
	else
	{
		tableList->clear();
		tableList->addItems(Database::getObjects("table", m_schema).keys());
// FIXME need to fix create parser before this will work
//		tableList->addItems(Database::getObjects("view", m_schema).keys());
		found = false;
		for (int i = 0; i < tableList->count(); ++i)
		{
			if (tableList->itemText(i) == m_table)
			{
				tableList->setCurrentIndex(i);
				found = true;
				break;
			}
		}
		if (!found)
		{
			tableList->setEnabled(true);
			tableList->setCurrentIndex(0);
			if (tableList->count() > 0)
			{
				tableSelected(tableList->currentText());
			}
			else
			{
				resetModel();
			}
		}
		else
		{
			QStringList columns = getColumns();
			if (m_columnList != columns)
			{
				resetModel();
				m_columnList = columns;
				columnModel->setStringList(m_columnList);
				resizeWanted = true;
			}
		}
	}
}

void QueryEditorWidget::tableAltered(QString oldName, QTreeWidgetItem * item)
{
	if (m_schema == item->text(1))
	{
		QString newName(item->text(0));
		for (int i = 0; i < tableList->count(); ++i)
		{
			if (tableList->itemText(i) == oldName)
			{
				tableList->setItemText(i, newName);
				if (m_table == oldName)
				{
					m_table = newName;
					QStringList columns = getColumns();
					if (m_columnList != columns)
					{
						resetModel();
						m_columnList = columns;
						columnModel->setStringList(m_columnList);
						resizeWanted = true;
					}
				}
			}
		}
	}
}

void QueryEditorWidget::fixSchema(const QString & schema)
{
	schemaList->setCurrentIndex(schemaList->findText(schema));
	schemaSelected(schema);
	schemaList->setDisabled(true);
}

void QueryEditorWidget::resizeEvent(QResizeEvent * event)
{
	resizeWanted = true;
	QWidget::resizeEvent(event);
}

void QueryEditorWidget::paintEvent(QPaintEvent * event)
{
	if (resizeWanted)
	{
		Utils::setColumnWidths(termsTable);
		Utils::setColumnWidths(ordersTable);
		resizeWanted = false;
	}
	QWidget::paintEvent(event);
}
