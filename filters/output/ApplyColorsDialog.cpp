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

#include "ApplyColorsDialog.h.moc"
#include "PageSelectionAccessor.h"
#include <QButtonGroup>

namespace output
{

ApplyColorsDialog::ApplyColorsDialog(QWidget* parent,
	IntrusivePtr<PageSequence> const& pages,
	PageSelectionAccessor const& page_selection_accessor)
:	QDialog(parent),
	m_pages(pages->snapshot(PageSequence::PAGE_VIEW)),
	m_selectedPages(page_selection_accessor.selectedPages()),
	m_pScopeGroup(new QButtonGroup(this))
{
	setupUi(this);
	m_pScopeGroup->addButton(thisPageRB);
	m_pScopeGroup->addButton(allPagesRB);
	m_pScopeGroup->addButton(thisPageAndFollowersRB);
	m_pScopeGroup->addButton(selectedPagesRB);
	if (m_selectedPages.size() <= 1) {
		selectedPagesWidget->setEnabled(false);
	}
	
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyColorsDialog::~ApplyColorsDialog()
{
}

void
ApplyColorsDialog::onSubmit()
{
	int const num_pages = m_pages.numPages();
	int const cur_page = m_pages.curPageIdx();
	
	std::set<PageId> pages;
	
	// thisPageRB is intentionally not handled.
	if (allPagesRB->isChecked()) {
		for (int i = 0; i < num_pages; ++i) {
			pages.insert(m_pages.pageAt(i).id());
		}
	} else if (thisPageAndFollowersRB->isChecked()) {
		for (int i = cur_page; i < num_pages; ++i) {
			pages.insert(m_pages.pageAt(i).id());
		}
	} else if (selectedPagesRB->isChecked()) {
		emit accepted(m_selectedPages);
		accept();
		return;
	}
	
	emit accepted(pages);
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

} // namespace output
