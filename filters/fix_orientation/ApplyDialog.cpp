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

#include "ApplyDialog.h.moc"
#include "PageSelectionAccessor.h"
#include <QButtonGroup>
#include <QDebug>
#include <assert.h>

namespace fix_orientation
{

ApplyDialog::ApplyDialog(
	QWidget* parent, IntrusivePtr<PageSequence> const& pages,
	PageSelectionAccessor const& page_selection_accessor)
:	QDialog(parent),
	m_pages(pages->snapshot(PageSequence::IMAGE_VIEW)),
	m_selectedPages(page_selection_accessor.selectedPages()),
	m_selectedRanges(page_selection_accessor.selectedRanges()),
	m_pBtnGroup(new QButtonGroup(this))
{
	setupUi(this);
	m_pBtnGroup->addButton(thisPageOnlyRB);
	m_pBtnGroup->addButton(allPagesRB);
	m_pBtnGroup->addButton(thisPageAndFollowersRB);
	m_pBtnGroup->addButton(selectedPagesRB);
	m_pBtnGroup->addButton(everyOtherRB);
	m_pBtnGroup->addButton(everyOtherSelectedRB);
	
	if (m_selectedPages.size() <= 1) {
		selectedPagesWidget->setEnabled(false);
		everyOtherSelectedWidget->setEnabled(false);
		everyOtherSelectedHint->setText(selectedPagesHint->text());
	} else if (m_selectedRanges.size() > 1) {
		everyOtherSelectedWidget->setEnabled(false);
		everyOtherSelectedHint->setText(tr("Can't do: more that one group is selected."));
	}
	
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyDialog::~ApplyDialog()
{
}

void
ApplyDialog::onSubmit()
{
	int const cur_page = m_pages.curPageIdx();
	int const num_pages = m_pages.numPages();
	
	std::set<PageId> pages;
	
	// thisPageOnlyRB is intentionally not handled.
	if (allPagesRB->isChecked()) {
		for (int i = 0; i < num_pages; ++i) {
			pages.insert(m_pages.pageAt(i).id());
		}
	} else if (thisPageAndFollowersRB->isChecked()) {
		for (int i = cur_page; i < num_pages; ++i) {
			pages.insert(m_pages.pageAt(i).id());
		}
	} else if (selectedPagesRB->isChecked()) {
		emit appliedTo(m_selectedPages);
		accept();
		return;
	} else if (everyOtherRB->isChecked()) {
		for (int i = cur_page & 1; i < num_pages; i += 2) {
			pages.insert(m_pages.pageAt(i).id());
		}
	} else if (everyOtherSelectedRB->isChecked()) {
		assert(m_selectedRanges.size() == 1);
		PageRange const& range = m_selectedRanges.front();
		int i = (cur_page - range.firstPageIdx) & 1;
		int const limit = range.pages.size();
		for (; i < limit; i += 2) {
			pages.insert(range.pages[i]);
		}
	}
	
	emit appliedTo(pages);
	
	// We assume the default connection from accept() to accepted() was removed.
	accept();
}

} // namespace fix_orientation
