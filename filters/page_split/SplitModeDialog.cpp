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

#include "SplitModeDialog.h.moc"
#include "PageSelectionAccessor.h"
#include <QPixmap>
#include <QButtonGroup>
#include <assert.h>

namespace page_split
{

SplitModeDialog::SplitModeDialog(
	QWidget* const parent, PageId const& cur_page,
	PageSelectionAccessor const& page_selection_accessor,
	LayoutType const layout_type,
	PageLayout::Type const auto_detected_layout_type,
	bool const auto_detected_layout_type_valid)
:	QDialog(parent),
	m_pages(page_selection_accessor.allPages()),
	m_selectedPages(page_selection_accessor.selectedPages()),
	m_curPage(cur_page),
	m_pScopeGroup(new QButtonGroup(this)),
	m_layoutType(layout_type),
	m_autoDetectedLayoutType(auto_detected_layout_type),
	m_autoDetectedLayoutTypeValid(auto_detected_layout_type_valid)
{
	setupUi(this);
	m_pScopeGroup->addButton(thisPageRB);
	m_pScopeGroup->addButton(allPagesRB);
	m_pScopeGroup->addButton(thisPageAndFollowersRB);
	m_pScopeGroup->addButton(selectedPagesRB);
	if (m_selectedPages.size() <= 1) {
		selectedPagesWidget->setEnabled(false);
	}
	
	layoutTypeLabel->setPixmap(QPixmap(iconFor(m_layoutType)));
	if (m_layoutType == AUTO_LAYOUT_TYPE) {
		modeAuto->setChecked(true);
	} else {
		modeManual->setChecked(true);
	}
	
	connect(modeAuto, SIGNAL(pressed()), this, SLOT(autoDetectionSelected()));
	connect(modeManual, SIGNAL(pressed()), this, SLOT(manualModeSelected()));
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

SplitModeDialog::~SplitModeDialog()
{
}

void
SplitModeDialog::autoDetectionSelected()
{
	layoutTypeLabel->setPixmap(QPixmap(":/icons/layout_type_auto.png"));
}

void
SplitModeDialog::manualModeSelected()
{
	char const* resource = iconFor(combinedLayoutType());
	layoutTypeLabel->setPixmap(QPixmap(resource));
}

void
SplitModeDialog::onSubmit()
{
	LayoutType layout_type = AUTO_LAYOUT_TYPE;
	if (modeManual->isChecked()) {
		layout_type = combinedLayoutType();
	}
	
	std::set<PageId> pages;
	
	if (thisPageRB->isChecked()) {
		pages.insert(m_curPage);
	} else if (allPagesRB->isChecked()) {
		m_pages.selectAll().swap(pages);
		emit accepted(m_selectedPages, true, layout_type);
		accept();
	} else if (thisPageAndFollowersRB->isChecked()) {
		m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
	} else if (selectedPagesRB->isChecked()) {
		emit accepted(m_selectedPages, false, layout_type);
		accept();
		return;
	}
	
	emit accepted(pages, false, layout_type);
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

LayoutType
SplitModeDialog::combinedLayoutType() const
{
	if (m_layoutType != AUTO_LAYOUT_TYPE) {
		return m_layoutType;
	}
	
	if (!m_autoDetectedLayoutTypeValid) {
		return AUTO_LAYOUT_TYPE;
	}
	
	switch (m_autoDetectedLayoutType) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			return SINGLE_PAGE_UNCUT;
		case PageLayout::SINGLE_PAGE_CUT:
			return PAGE_PLUS_OFFCUT;
		case PageLayout::TWO_PAGES:
			return TWO_PAGES;
	}
	
	assert(!"Unreachable");
	return AUTO_LAYOUT_TYPE;
}

char const*
SplitModeDialog::iconFor(LayoutType const layout_type)
{
	char const* resource = "";
	
	switch (layout_type) {
		case AUTO_LAYOUT_TYPE:
			resource = ":/icons/layout_type_auto.png";
			break;
		case SINGLE_PAGE_UNCUT:
			resource = ":/icons/single_page_uncut_selected.png";
			break;
		case PAGE_PLUS_OFFCUT:
			resource = ":/icons/right_page_plus_offcut_selected.png";
			break;
		case TWO_PAGES:
			resource = ":/icons/two_pages_selected.png";
			break;
	}
	
	return resource;
}

} // namespace page_split
