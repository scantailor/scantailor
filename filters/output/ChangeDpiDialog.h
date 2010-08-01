/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OUTPUT_CHANGEDPIDIALOG_H_
#define OUTPUT_CHANGEDPIDIALOG_H_

#include "ui_OutputChangeDpiDialog.h"
#include "PageId.h"
#include "PageSequence.h"
#include "IntrusivePtr.h"
#include <QDialog>
#include <QString>
#include <set>

class PageSelectionAccessor;
class QButtonGroup;
class Dpi;

namespace output
{

class ChangeDpiDialog : public QDialog, private Ui::OutputChangeDpiDialog
{
	Q_OBJECT
public:
	ChangeDpiDialog(
		QWidget* parent, Dpi const& dpi, PageId const& cur_page,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~ChangeDpiDialog();
signals:
	void accepted(std::set<PageId> const& pages, Dpi const& dpi);
private slots:
	void dpiSelectionChanged(int index);
	
	void dpiEditTextChanged(QString const& text);
	
	void onSubmit();
private:
	PageSequence m_pages;
	std::set<PageId> m_selectedPages;
	PageId m_curPage;
	QButtonGroup* m_pScopeGroup;
	int m_customItemIdx;
	QString m_customDpiString;
};

} // namespace output

#endif
