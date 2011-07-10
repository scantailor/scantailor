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

#ifndef PAGE_LAYOUT_APPLYDIALOG_H_
#define PAGE_LAYOUT_APPLYDIALOG_H_

#include "ui_PageLayoutApplyDialog.h"
#include "PageId.h"
#include "PageRange.h"
#include "PageSequence.h"
#include "IntrusivePtr.h"
#include <QDialog>
#include <set>

class PageSelectionAccessor;
class QButtonGroup;

namespace page_layout
{

class ApplyDialog : public QDialog, private Ui::PageLayoutApplyDialog
{
	Q_OBJECT
public:
	ApplyDialog(QWidget* parent, PageId const& cur_page,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~ApplyDialog();
signals:
	void accepted(std::set<PageId> const& pages);
private slots:
	void onSubmit();
private:
	PageSequence m_pages;
	std::set<PageId> m_selectedPages;
	std::vector<PageRange> m_selectedRanges;
	PageId m_curPage;
	QButtonGroup* m_pScopeGroup;
};

} // namespace page_layout

#endif
