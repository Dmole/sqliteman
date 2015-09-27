/*
For general Sqliteman copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Sqliteman
for which a new license (GPL+exception) is in place.
*/

#ifndef QUERYEDITORDIALOG_H
#define QUERYEDITORDIALOG_H

#include <QStringListModel>

class QTreeWidgetItem;

#include "database.h"
#include "ui_queryeditordialog.h"

/*! \brief Improved QStringListModel for items handling in paralell
layouts. See queryeditordilog.ui in Qt4 designer.
QueryStringModels are used in "swap" items views.
\author Petr Vanek <petr@scribus.info>
*/
class QueryStringModel : public QStringListModel
{
	Q_OBJECT

	public:
		QueryStringModel(QObject * parent = 0);
		//! Make it non-editable
		Qt::ItemFlags flags (const QModelIndex & index) const;
		//! remove all items from model
		void clear();
		//! append new string at the end of the model
		void append(const QString & value);
};


/*!
 * @brief A dialog for creating and editing queries
 * \author Igor Khanin
 * \author Petr Vanek <petr@scribus.info>
 */
class QueryEditorDialog : public QDialog, public Ui::QueryEditorDialog
{
	Q_OBJECT

	public:
		/*!
		 * @brief Creates the query editor.
		 * @param parent The parent widget for the dialog.
		 */
		QueryEditorDialog(QWidget * parent = 0);
		QueryEditorDialog(QTreeWidgetItem * item, QWidget * parent = 0);
		~QueryEditorDialog();
		//! \brief generates a valid SQL statement using the values in the dialog
		QString statement();

	private:
		QString m_schema;
		QueryStringModel * columnModel;
		QueryStringModel * selectModel;
		void CommonSetup();
		QString m_rowid;
		QStringList m_columnList;
		
	private slots:
		void tableSelected(const QString & table);
		// Term tab
		void moreTerms();
		void lessTerms();
		// Fields tab
		void addAllSelect();
		void addSelect();
		void removeAllSelect();
		void removeSelect();
		// Order by tab
		void moreOrders();
		void lessOrders();

		void relationsIndexChanged(const QString &);

	private:
		QString curTable;
};

#endif //QUERYEDITORDIALOG_H
