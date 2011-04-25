/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef OUTPUT_CHANGE_DEWARPING_DIALOG_H_
#define OUTPUT_CHANGE_DEWARPING_DIALOG_H_

#include "ui_OutputChangeDewarpingDialog.h"
#include "DewarpingMode.h"
#include "PageId.h"
#include "PageSequence.h"
#include "IntrusivePtr.h"
#include <QDialog>
#include <QString>
#include <set>

class PageSelectionAccessor;
class QButtonGroup;

namespace output
{

class ChangeDewarpingDialog : public QDialog
{
	Q_OBJECT
public:
	ChangeDewarpingDialog(
		QWidget* parent, PageId const& cur_page, DewarpingMode const& mode,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~ChangeDewarpingDialog();
signals:
	void accepted(std::set<PageId> const& pages, DewarpingMode const& mode);
private slots:
	void onSubmit();
private:
	Ui::OutputChangeDewarpingDialog ui;
	PageSequence m_pages;
	std::set<PageId> m_selectedPages;
	PageId m_curPage;
	DewarpingMode m_mode;
	QButtonGroup* m_pScopeGroup;
};

} // namespace output

#endif
