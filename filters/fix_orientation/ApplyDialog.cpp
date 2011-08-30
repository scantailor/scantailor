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

#include "ApplyDialog.h"
#include "ApplyDialog.h.moc"
#include "PageSelectionAccessor.h"
#include <QButtonGroup>
#include <QDebug>
#include <assert.h>

namespace fix_orientation
{

ApplyDialog::ApplyDialog(
	QWidget* parent,
	PageId const& cur_page, PageSelectionAccessor const& page_selection_accessor)
:	QDialog(parent),
	m_pages(page_selection_accessor.allPages()),
	m_selectedPages(page_selection_accessor.selectedPages()),
	m_selectedRanges(page_selection_accessor.selectedRanges()),
	m_curPage(cur_page),
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
		everyOtherSelectedHint->setText(tr("Can't do: more than one group is selected."));
	}
	
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyDialog::~ApplyDialog()
{
}

void
ApplyDialog::onSubmit()
{
	std::set<PageId> pages;
	
	// thisPageOnlyRB is intentionally not handled.
	if (allPagesRB->isChecked()) {
		m_pages.selectAll().swap(pages);
		emit appliedToAllPages(pages);
		accept();
		return;
	} else if (thisPageAndFollowersRB->isChecked()) {
		m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
	} else if (selectedPagesRB->isChecked()) {
		emit appliedTo(m_selectedPages);
		accept();
		return;
	} else if (everyOtherRB->isChecked()) {
		m_pages.selectEveryOther(m_curPage).swap(pages);
	} else if (everyOtherSelectedRB->isChecked()) {
		assert(m_selectedRanges.size() == 1);
		PageRange const& range = m_selectedRanges.front();
		range.selectEveryOther(m_curPage).swap(pages);
	}
	
	emit appliedTo(pages);
	
	// We assume the default connection from accept() to accepted() was removed.
	accept();
}

} // namespace fix_orientation
