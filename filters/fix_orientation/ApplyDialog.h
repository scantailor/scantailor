/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef FIX_ORIENTATION_APPLYDIALOG_H_
#define FIX_ORIENTATION_APPLYDIALOG_H_

#include "ui_OrientationApplyDialog.h"
#include "PageId.h"
#include "PageRange.h"
#include "PageSequence.h"
#include "IntrusivePtr.h"
#include <QDialog>
#include <vector>
#include <set>

class PageSelectionAccessor;
class QButtonGroup;

namespace fix_orientation
{

class Scope;

class ApplyDialog : public QDialog, private Ui::OrientationApplyDialog
{
	Q_OBJECT
public:
	ApplyDialog(QWidget* parent, PageId const& cur_page,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~ApplyDialog();
signals:
	void appliedTo(std::set<PageId> const& pages);

	void appliedToAllPages(std::set<PageId> const& pages);
private slots:
	void onSubmit();
private:
	PageSequence m_pages;
	std::set<PageId> m_selectedPages;
	std::vector<PageRange> m_selectedRanges;
	PageId m_curPage;
	QButtonGroup* m_pBtnGroup;
};

} // namespace fix_orientation

#endif
