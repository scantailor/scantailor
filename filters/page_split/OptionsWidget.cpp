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
	m_ignoreAutoManualToggle(0),
	m_ignoreLayoutTypeToggle(0)
{
	setupUi(this);
	
	connect(
		singlePageUncutBtn, SIGNAL(toggled(bool)),
		this, SLOT(layoutTypeButtonToggled(bool))
	);
	connect(
		leftPagePlusOffcutBtn, SIGNAL(toggled(bool)),
		this, SLOT(layoutTypeButtonToggled(bool))
	);
	connect(
		rightPagePlusOffcutBtn, SIGNAL(toggled(bool)),
		this, SLOT(layoutTypeButtonToggled(bool))
	);
	connect(
		twoPagesBtn, SIGNAL(toggled(bool)),
		this, SLOT(layoutTypeButtonToggled(bool))
	);
	connect(
		changeBtn, SIGNAL(clicked()),
		this, SLOT(showChangeDialog())
	);
	connect(
		autoBtn, SIGNAL(toggled(bool)),
		this, SLOT(splitLineModeChanged(bool))
	);
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(ImageId const& image_id)
{
	ScopedIncDec<int> guard1(m_ignoreAutoManualToggle);
	ScopedIncDec<int> guard2(m_ignoreLayoutTypeToggle);
	
	m_imageId = image_id;
	Rule const rule(m_ptrSettings->getPageRecord(image_id).rule());
	
	switch (rule.layoutType()) {
		case Rule::AUTO_DETECT:
			// Uncheck all buttons.  Can only be done
			// by playing with exclusiveness.
			twoPagesBtn->setChecked(true);
			twoPagesBtn->setAutoExclusive(false);
			twoPagesBtn->setChecked(false);
			twoPagesBtn->setAutoExclusive(true);
			break;
		case Rule::SINGLE_PAGE_UNCUT:
			singlePageUncutBtn->setChecked(true);
			break;
		case Rule::LEFT_PAGE_PLUS_OFFCUT:
			leftPagePlusOffcutBtn->setChecked(true);
			break;
		case Rule::RIGHT_PAGE_PLUS_OFFCUT:
			rightPagePlusOffcutBtn->setChecked(true);
			break;
		case Rule::TWO_PAGES:
			twoPagesBtn->setChecked(true);
			break;
	}
	
	if (rule.layoutType() == Rule::AUTO_DETECT) {
		changeBtn->setEnabled(false);
		scopeLabel->setText("?");
	} else {
		changeBtn->setEnabled(true);
		if (rule.scope() == Rule::THIS_PAGE_ONLY) {
			scopeLabel->setText(tr("This page only"));
		} else {
			scopeLabel->setText(tr("All pages"));
		}
	}
	
	// Uncheck both the Auto and Manual buttons.
	autoBtn->setChecked(true);
	autoBtn->setAutoExclusive(false);
	autoBtn->setChecked(false);
	autoBtn->setAutoExclusive(true);
	
	// And disable both of them.
	autoBtn->setEnabled(false);
	manualBtn->setEnabled(false);
}

void
OptionsWidget::postUpdateUI(UiData const& ui_data)
{
	ScopedIncDec<int> guard1(m_ignoreAutoManualToggle);
	ScopedIncDec<int> guard2(m_ignoreLayoutTypeToggle);
	
	m_uiData = ui_data;

	changeBtn->setEnabled(true);
	autoBtn->setEnabled(true);
	manualBtn->setEnabled(true);
	
	if (ui_data.splitLineMode() == MODE_AUTO) {
		autoBtn->setChecked(true);
	} else {
		manualBtn->setChecked(true);
	}
	
	switch (ui_data.pageLayout().type()) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			singlePageUncutBtn->setChecked(true);
			break;
		case PageLayout::LEFT_PAGE_PLUS_OFFCUT:
			leftPagePlusOffcutBtn->setChecked(true);
			break;
		case PageLayout::RIGHT_PAGE_PLUS_OFFCUT:
			rightPagePlusOffcutBtn->setChecked(true);
			break;
		case PageLayout::TWO_PAGES:
			twoPagesBtn->setChecked(true);
			break;
	}
	
	if (ui_data.layoutTypeAutoDetected()) {
		scopeLabel->setText(tr("Auto detected"));
	}
}

void
OptionsWidget::pageLayoutSetExternally(PageLayout const& page_layout)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	m_uiData.setPageLayout(page_layout);
	m_uiData.setSplitLineMode(MODE_MANUAL);
	commitCurrentParams();
	
	manualBtn->setChecked(true);
	
	emit invalidateThumbnail(PageId(m_imageId));
}

void
OptionsWidget::layoutTypeButtonToggled(bool const checked)
{
	Rule::LayoutType rlt;
	PageLayout::Type plt;
	
	QObject* button = sender();
	if (button == singlePageUncutBtn) {
		rlt = Rule::SINGLE_PAGE_UNCUT;
		plt = PageLayout::SINGLE_PAGE_UNCUT;
	} else if (button == leftPagePlusOffcutBtn) {
		rlt = Rule::LEFT_PAGE_PLUS_OFFCUT;
		plt = PageLayout::LEFT_PAGE_PLUS_OFFCUT;
	} else if (button == rightPagePlusOffcutBtn) {
		rlt = Rule::RIGHT_PAGE_PLUS_OFFCUT;
		plt = PageLayout::RIGHT_PAGE_PLUS_OFFCUT;
	} else {
		assert(button == twoPagesBtn);
		rlt = Rule::TWO_PAGES;
		plt = PageLayout::TWO_PAGES;
	}
	
	Settings::UpdateAction update;
	update.setLayoutType(rlt);
	
	scopeLabel->setText(tr("This page only"));
	
	if (m_uiData.splitLineMode() == MODE_AUTO &&
			rlt != Rule::SINGLE_PAGE_UNCUT) {
		m_ptrSettings->updatePage(m_imageId, update);
		emit reloadRequested();
	} else {
		PageLayout const new_layout(
			plt, m_uiData.pageLayout().splitLine()
		);
		Params const new_params(
			new_layout, m_uiData.dependencies(),
			m_uiData.splitLineMode()
		);
		
		update.setParams(new_params);
		m_ptrSettings->updatePage(m_imageId, update);
		
		m_uiData.setPageLayout(new_layout);
		emit pageLayoutSetLocally(new_layout);
		emit invalidateThumbnail(PageId(m_imageId));
	}
}

void
OptionsWidget::singlePageUncutToggled(bool const checked)
{
	if (!checked || m_ignoreLayoutTypeToggle) {
		return;
	}
	
	PageLayout const new_layout(
		PageLayout::SINGLE_PAGE_UNCUT,
		m_uiData.pageLayout().splitLine()
	);
	Params const new_params(
		new_layout, m_uiData.dependencies(),
		m_uiData.splitLineMode()
	);
	
	Settings::UpdateAction update;
	update.setLayoutType(Rule::SINGLE_PAGE_UNCUT);
	update.setParams(new_params);
	
	m_ptrSettings->updatePage(m_imageId, update);
	scopeLabel->setText(tr("This page only"));
	
	m_uiData.setPageLayout(new_layout);
	emit pageLayoutSetLocally(new_layout);
	emit invalidateThumbnail(PageId(m_imageId));
}

void
OptionsWidget::leftPagePlusOffcutToggled(bool const checked)
{
	if (!checked || m_ignoreLayoutTypeToggle) {
		return;
	}
	
	Settings::UpdateAction update;
	update.setLayoutType(Rule::LEFT_PAGE_PLUS_OFFCUT);
	
	scopeLabel->setText(tr("This page only"));
	
	if (m_uiData.splitLineMode() == MODE_AUTO) {
		m_ptrSettings->updatePage(m_imageId, update);
		emit reloadRequested();
	} else {
		PageLayout const new_layout(
			PageLayout::LEFT_PAGE_PLUS_OFFCUT,
			m_uiData.pageLayout().splitLine()
		);
		Params const new_params(
			new_layout, m_uiData.dependencies(),
			m_uiData.splitLineMode()
		);
		
		update.setParams(new_params);
		m_ptrSettings->updatePage(m_imageId, update);
		
		m_uiData.setPageLayout(new_layout);
		emit pageLayoutSetLocally(new_layout);
		emit invalidateThumbnail(PageId(m_imageId));
	}
}

void
OptionsWidget::rightPagePlusOffcutToggled(bool const checked)
{
	if (!checked || m_ignoreLayoutTypeToggle) {
		return;
	}
	
	Settings::UpdateAction update;
	update.setLayoutType(Rule::RIGHT_PAGE_PLUS_OFFCUT);
	
	scopeLabel->setText(tr("This page only"));
	
	if (m_uiData.splitLineMode() == MODE_AUTO) {
		m_ptrSettings->updatePage(m_imageId, update);
		emit reloadRequested();
	} else {
		PageLayout const new_layout(
			PageLayout::RIGHT_PAGE_PLUS_OFFCUT,
			m_uiData.pageLayout().splitLine()
		);
		Params const new_params(
			new_layout, m_uiData.dependencies(),
			m_uiData.splitLineMode()
		);
		
		update.setParams(new_params);
		m_ptrSettings->updatePage(m_imageId, update);
		
		m_uiData.setPageLayout(new_layout);
		emit pageLayoutSetLocally(new_layout);
		emit invalidateThumbnail(PageId(m_imageId));
	}
}

void
OptionsWidget::twoPagesToggled(bool const checked)
{
	if (!checked || m_ignoreLayoutTypeToggle) {
		return;
	}
	
	Settings::UpdateAction update;
	update.setLayoutType(Rule::TWO_PAGES);
	
	scopeLabel->setText(tr("This page only"));
	
	if (m_uiData.splitLineMode() == MODE_AUTO) {
		m_ptrSettings->updatePage(m_imageId, update);
		emit reloadRequested();
	} else {
		PageLayout const new_layout(
			PageLayout::TWO_PAGES,
			m_uiData.pageLayout().splitLine()
		);
		Params const new_params(
			new_layout, m_uiData.dependencies(),
			m_uiData.splitLineMode()
		);
		
		update.setParams(new_params);
		m_ptrSettings->updatePage(m_imageId, update);
		
		m_uiData.setPageLayout(new_layout);
		emit pageLayoutSetLocally(new_layout);
		emit invalidateThumbnail(PageId(m_imageId));
	}
}

void
OptionsWidget::showChangeDialog()
{
	Settings::Record const record(m_ptrSettings->getPageRecord(m_imageId));
	Params const* params = record.params();
	if (!params) {
		return;
	}
	
	SplitModeDialog* dialog = new SplitModeDialog(
		this, record.rule(), params->pageLayout().type(),
		params->splitLineMode() == MODE_AUTO
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
		Settings::UpdateAction update;
		update.setLayoutType(rule.layoutType());
		m_ptrSettings->updatePage(m_imageId, update);
	} else {
		m_ptrSettings->setLayoutTypeForAllPages(rule.layoutType());
	}
	emit reloadRequested();
}

void
OptionsWidget::splitLineModeChanged(bool const auto_mode)
{
	if (m_ignoreAutoManualToggle) {
		return;
	}
	
	if (auto_mode) {
		Settings::UpdateAction update;
		update.clearParams();
		m_ptrSettings->updatePage(m_imageId, update);
		m_uiData.setSplitLineMode(MODE_AUTO);
		emit reloadRequested();
	} else {
		m_uiData.setSplitLineMode(MODE_MANUAL);
		commitCurrentParams();
	}
}

void
OptionsWidget::commitCurrentParams()
{
	Params const params(
		m_uiData.pageLayout(),
		m_uiData.dependencies(), m_uiData.splitLineMode()
	);
	Settings::UpdateAction update;
	update.setParams(params);
	m_ptrSettings->updatePage(m_imageId, update);
}


/*============================= Widget::UiData ==========================*/

OptionsWidget::UiData::UiData()
:	m_splitLineMode(MODE_AUTO),
	m_layoutTypeAutoDetected(false)
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
OptionsWidget::UiData::setSplitLineMode(AutoManualMode const mode)
{
	m_splitLineMode = mode;
}

AutoManualMode
OptionsWidget::UiData::splitLineMode() const
{
	return m_splitLineMode;
}

bool
OptionsWidget::UiData::layoutTypeAutoDetected() const
{
	return m_layoutTypeAutoDetected;
}

void
OptionsWidget::UiData::setLayoutTypeAutoDetected(bool const val)
{
	m_layoutTypeAutoDetected = val;
}

} // namespace page_split
