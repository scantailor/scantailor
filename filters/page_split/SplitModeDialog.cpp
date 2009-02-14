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

#include "SplitModeDialog.h.moc"
#include <QPixmap>
#include <assert.h>

namespace page_split
{

SplitModeDialog::SplitModeDialog(
	QWidget* const parent, Rule const& rule,
	PageLayout::Type const auto_detected_layout_type,
	bool const auto_detected_layout_type_valid)
:	QDialog(parent),
	m_layoutType(rule.layoutType()),
	m_autoDetectedLayoutType(auto_detected_layout_type),
	m_autoDetectedLayoutTypeValid(auto_detected_layout_type_valid)
{
	setupUi(this);
	layoutTypeLabel->setPixmap(QPixmap(iconFor(m_layoutType)));
	if (m_layoutType == Rule::AUTO_DETECT) {
		modeAuto->setChecked(true);
	} else {
		modeManual->setChecked(true);
	}
	
	switch (rule.scope()) {
		case Rule::THIS_PAGE_ONLY:
			scopeThisPage->setChecked(true);
			break;
		case Rule::ALL_PAGES:
			scopeAllPages->setChecked(true);
			break;
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
	Rule::LayoutType layout_type = Rule::AUTO_DETECT;
	if (modeManual->isChecked()) {
		layout_type = combinedLayoutType();
	}
	
	Rule::Scope scope = Rule::THIS_PAGE_ONLY;
	if (scopeAllPages->isChecked()) {
		scope = Rule::ALL_PAGES;
	}
	
	emit accepted(Rule(layout_type, scope));
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

Rule::LayoutType
SplitModeDialog::combinedLayoutType() const
{
	if (m_layoutType != Rule::AUTO_DETECT) {
		return m_layoutType;
	}
	
	if (!m_autoDetectedLayoutTypeValid) {
		return Rule::AUTO_DETECT;
	}
	
	switch (m_autoDetectedLayoutType) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			return Rule::SINGLE_PAGE_UNCUT;
		case PageLayout::LEFT_PAGE_PLUS_OFFCUT:
			return Rule::PAGE_PLUS_OFFCUT;
		case PageLayout::RIGHT_PAGE_PLUS_OFFCUT:
			return Rule::PAGE_PLUS_OFFCUT;
		case PageLayout::TWO_PAGES:
			return Rule::TWO_PAGES;
	}
	
	assert(!"Unreachable");
	return Rule::AUTO_DETECT;
}

char const*
SplitModeDialog::iconFor(Rule::LayoutType const layout_type)
{
	char const* resource = "";
	
	switch (layout_type) {
		case Rule::AUTO_DETECT:
			resource = ":/icons/layout_type_auto.png";
			break;
		case Rule::SINGLE_PAGE_UNCUT:
			resource = ":/icons/single_page_uncut_selected.png";
			break;
		case Rule::PAGE_PLUS_OFFCUT:
			resource = ":/icons/right_page_plus_offcut_selected.png";
			break;
		case Rule::TWO_PAGES:
			resource = ":/icons/two_pages_selected.png";
			break;
	}
	
	return resource;
}

} // namespace page_split
