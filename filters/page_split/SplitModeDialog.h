/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef PAGE_SPLIT_SPLITMODEDIALOG_H_
#define PAGE_SPLIT_SPLITMODEDIALOG_H_

#include "ui_PageSplitModeDialog.h"
#include "LayoutType.h"
#include "PageLayout.h"
#include "PageId.h"
#include "PageSequence.h"
#include "IntrusivePtr.h"
#include <QDialog>
#include <set>

class PageSelectionAccessor;
class QButtonGroup;

namespace page_split
{

class SplitModeDialog : public QDialog, private Ui::PageSplitModeDialog
{
	Q_OBJECT
public:
	SplitModeDialog(QWidget* parent,
		IntrusivePtr<PageSequence> const& pages,
		PageSelectionAccessor const& page_selection_accessor,
		LayoutType layout_type, PageLayout::Type auto_detected_layout_type,
		bool auto_detected_layout_type_valid);
	
	virtual ~SplitModeDialog();
signals:
	void accepted(std::set<PageId> const& pages, LayoutType layout_type);
private slots:
	void autoDetectionSelected();
	
	void manualModeSelected();
	
	void onSubmit();
private:
	LayoutType combinedLayoutType() const;
	
	static char const* iconFor(LayoutType layout_type);
	
	PageSequenceSnapshot m_pages;
	std::set<PageId> m_selectedPages;
	QButtonGroup* m_pScopeGroup;
	LayoutType m_layoutType;
	PageLayout::Type m_autoDetectedLayoutType;
	bool m_autoDetectedLayoutTypeValid;
};

} // namespace page_split

#endif
