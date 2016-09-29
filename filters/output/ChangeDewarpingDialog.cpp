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

#include "ChangeDewarpingDialog.h"
#include "ChangeDewarpingDialog.h.moc"
#include "PageSelectionAccessor.h"
#include "QtSignalForwarder.h"
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#endif

namespace output
{

ChangeDewarpingDialog::ChangeDewarpingDialog(
	QWidget* parent, PageId const& cur_page, DewarpingMode const& mode,
	PageSelectionAccessor const& page_selection_accessor)
:	QDialog(parent),
	m_pages(page_selection_accessor.allPages()),
	m_selectedPages(page_selection_accessor.selectedPages()),
	m_curPage(cur_page),
	m_mode(mode),
	m_pScopeGroup(new QButtonGroup(this))
{
	using namespace boost::lambda;

	ui.setupUi(this);
	m_pScopeGroup->addButton(ui.thisPageRB);
	m_pScopeGroup->addButton(ui.allPagesRB);
	m_pScopeGroup->addButton(ui.thisPageAndFollowersRB);
	m_pScopeGroup->addButton(ui.selectedPagesRB);
	if (m_selectedPages.size() <= 1) {
		ui.selectedPagesWidget->setEnabled(false);
	}

	switch (mode) {
		case DewarpingMode::OFF:
			ui.offRB->setChecked(true);
			break;
		case DewarpingMode::AUTO:
			ui.autoRB->setChecked(true);
			break;
		case DewarpingMode::MANUAL:
			ui.manualRB->setChecked(true);
			break;
	}

	// No, we don't leak memory here.
	new QtSignalForwarder(ui.offRB, SIGNAL(clicked(bool)), var(m_mode) = DewarpingMode::OFF);
	new QtSignalForwarder(ui.autoRB, SIGNAL(clicked(bool)), var(m_mode) = DewarpingMode::AUTO);
	new QtSignalForwarder(ui.manualRB, SIGNAL(clicked(bool)), var(m_mode) = DewarpingMode::MANUAL);
	
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ChangeDewarpingDialog::~ChangeDewarpingDialog()
{
}

void
ChangeDewarpingDialog::onSubmit()
{
	std::set<PageId> pages;
	
	if (ui.thisPageRB->isChecked()) {
		pages.insert(m_curPage);
	} else if (ui.allPagesRB->isChecked()) {
		m_pages.selectAll().swap(pages);
	} else if (ui.thisPageAndFollowersRB->isChecked()) {
		m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
	} else if (ui.selectedPagesRB->isChecked()) {
		emit accepted(m_selectedPages, m_mode);
		accept();
		return;
	}
	
	emit accepted(pages, m_mode);
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

} // namespace output
