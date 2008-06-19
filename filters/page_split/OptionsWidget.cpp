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

#include "OptionsWidget.h.moc"
#include "Filter.h"
#include "SplitModeDialog.h"
#include "Settings.h"
#include "Params.h"
#include "PageId.h"
#include "ScopedIncDec.h"
#include <QIcon>
#include <assert.h>

namespace page_split
{

OptionsWidget::OptionsWidget(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings),
	m_ignoreAutoManualToggle(0)
{
	setupUi(this);
	
	connect(singlePageBtn, SIGNAL(clicked()), this, SLOT(singlePageSelected()));
	connect(twoPagesBtn, SIGNAL(clicked()), this, SLOT(twoPagesSelected()));
	connect(changeBtn, SIGNAL(clicked()), this, SLOT(showChangeDialog()));
	connect(autoBtn, SIGNAL(toggled(bool)), this, SLOT(modeChanged(bool)));
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(ImageId const& image_id)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	m_imageId = image_id;
	Rule const rule(m_ptrSettings->getRuleFor(image_id));
	
	singlePageBtn->setChecked(rule.layoutType() == Rule::SINGLE_PAGE);
	twoPagesBtn->setChecked(rule.layoutType() == Rule::TWO_PAGES);
	
	if (rule.layoutType() == Rule::AUTO_DETECT) {
		changeBtn->setEnabled(false);
		autoBtn->setChecked(true);
		scopeLabel->setText("?");
	} else {
		changeBtn->setEnabled(true);
		manualBtn->setChecked(true);
		if (rule.scope() == Rule::THIS_PAGE_ONLY) {
			scopeLabel->setText(tr("This page only"));
		} else {
			scopeLabel->setText(tr("All pages"));
		}
	}
	
	autoBtn->setEnabled(false);
	manualBtn->setEnabled(false);
}

void
OptionsWidget::postUpdateUI(UiData const& ui_data)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	m_uiData = ui_data;

	changeBtn->setEnabled(true);
	autoBtn->setEnabled(true);
	manualBtn->setEnabled(true);
	
	if (ui_data.mode() == MODE_AUTO) {
		autoBtn->setChecked(true);
	} else {
		manualBtn->setChecked(true);
	}
	
	AutoDetectedLayout const auto_layout = ui_data.autoDetectedLayout();
	if (auto_layout != NO_AUTO_DETECTION) {
		singlePageBtn->setChecked(auto_layout == SINGLE_PAGE);
		twoPagesBtn->setChecked(auto_layout == TWO_PAGES);
		scopeLabel->setText(tr("Auto detected"));
	}
}

void
OptionsWidget::manualPageLayoutSet(PageLayout const& page_layout)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	m_uiData.setPageLayout(page_layout);
	m_uiData.setMode(MODE_MANUAL);
	commitCurrentParams();
	
	manualBtn->setChecked(true);
	
	emit invalidateThumbnail(PageId(m_imageId));
}

void
OptionsWidget::singlePageSelected()
{
	m_ptrSettings->applyToPage(m_imageId, Rule::SINGLE_PAGE);
	m_ptrSettings->clearPageParams(m_imageId);
	emit reloadRequested();
}

void
OptionsWidget::twoPagesSelected()
{
	m_ptrSettings->applyToPage(m_imageId, Rule::TWO_PAGES);
	m_ptrSettings->clearPageParams(m_imageId);
	emit reloadRequested();
}

void
OptionsWidget::showChangeDialog()
{
	Rule const rule(m_ptrSettings->getRuleFor(m_imageId));
	SplitModeDialog* dialog = new SplitModeDialog(
		this, rule, m_uiData.autoDetectedLayout()
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(Rule const&)),
		this, SLOT(ruleSet(Rule const&))
	);
	dialog->show();
}

void
OptionsWidget::ruleSet(Rule const& rule)
{
	if (rule.scope() == Rule::THIS_PAGE_ONLY) {
		m_ptrSettings->applyToPage(m_imageId, rule.layoutType());
	} else {
		m_ptrSettings->applyToAllPages(rule.layoutType());
	}
	emit reloadRequested();
}

void
OptionsWidget::modeChanged(bool const auto_mode)
{
	if (m_ignoreAutoManualToggle) {
		return;
	}
	
	if (auto_mode) {
		m_uiData.setMode(MODE_AUTO);
		m_ptrSettings->clearPageParams(m_imageId);
		emit reloadRequested();
	} else {
		m_uiData.setMode(MODE_MANUAL);
		commitCurrentParams();
	}
}

void
OptionsWidget::commitCurrentParams()
{
	Params const params(
		m_uiData.pageLayout(),
		m_uiData.dependencies(), m_uiData.mode()
	);
	m_ptrSettings->setPageParams(m_imageId, params);
}


/*============================= Widget::UiData ==========================*/

OptionsWidget::UiData::UiData()
:	m_mode(MODE_AUTO),
	m_autoDetectedLayout(NO_AUTO_DETECTION)
{
}

OptionsWidget::UiData::~UiData()
{
}

void
OptionsWidget::UiData::setPageLayout(PageLayout const& layout)
{
	m_pageLayout = layout;
}

PageLayout const&
OptionsWidget::UiData::pageLayout() const
{
	return m_pageLayout;
}

void
OptionsWidget::UiData::setDependencies(Dependencies const& deps)
{
	m_deps = deps;
}

Dependencies const&
OptionsWidget::UiData::dependencies() const
{
	return m_deps;
}

void
OptionsWidget::UiData::setMode(AutoManualMode const mode)
{
	m_mode = mode;
}

AutoManualMode
OptionsWidget::UiData::mode() const
{
	return m_mode;
}

void
OptionsWidget::UiData::setAutoDetectedLayout(AutoDetectedLayout const layout)
{
	m_autoDetectedLayout = layout;
}

AutoDetectedLayout
OptionsWidget::UiData::autoDetectedLayout() const
{
	return m_autoDetectedLayout;
}

} // namespace page_split
