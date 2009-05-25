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

#include "OptionsWidget.h.moc"
#include "Filter.h"
#include "SplitModeDialog.h"
#include "Settings.h"
#include "Params.h"
#include "LayoutType.h"
#include "PageId.h"
#include "PageSequence.h"
#include "ScopedIncDec.h"
#include <QPixmap>
#include <boost/foreach.hpp>
#include <assert.h>

namespace page_split
{

OptionsWidget::OptionsWidget(
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<PageSequence> const& page_sequence,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings),
	m_ptrPages(page_sequence),
	m_pageSelectionAccessor(page_selection_accessor),
	m_ignoreAutoManualToggle(0),
	m_ignoreLayoutTypeToggle(0)
{
	setupUi(this);
	flipSidesFrame->setVisible(false);
	
	m_flipLeftToRightIcon.addPixmap(QPixmap(":/icons/big-right-arrow.png"));
	m_flipRightToLeftIcon.addPixmap(QPixmap(":/icons/big-left-arrow.png"));
	
	connect(
		singlePageUncutBtn, SIGNAL(toggled(bool)),
		this, SLOT(layoutTypeButtonToggled(bool))
	);
	connect(
		pagePlusOffcutBtn, SIGNAL(toggled(bool)),
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
	connect(
		flipSidesBtn, SIGNAL(clicked()),
		this, SLOT(flipSidesButtonClicked())
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
	Settings::Record const record(m_ptrSettings->getPageRecord(image_id));
	LayoutType const layout_type(record.combinedLayoutType());
	
	switch (layout_type) {
		case AUTO_LAYOUT_TYPE:
			// Uncheck all buttons.  Can only be done
			// by playing with exclusiveness.
			twoPagesBtn->setChecked(true);
			twoPagesBtn->setAutoExclusive(false);
			twoPagesBtn->setChecked(false);
			twoPagesBtn->setAutoExclusive(true);
			break;
		case SINGLE_PAGE_UNCUT:
			singlePageUncutBtn->setChecked(true);
			break;
		case PAGE_PLUS_OFFCUT:
			pagePlusOffcutBtn->setChecked(true);
			break;
		case TWO_PAGES:
			twoPagesBtn->setChecked(true);
			break;
	}
	
	if (layout_type == AUTO_LAYOUT_TYPE) {
		changeBtn->setEnabled(false);
		scopeLabel->setText("?");
	} else {
		changeBtn->setEnabled(true);
		scopeLabel->setText(tr("Set manually"));
	}
	
	// Uncheck both the Auto and Manual buttons.
	autoBtn->setChecked(true);
	autoBtn->setAutoExclusive(false);
	autoBtn->setChecked(false);
	autoBtn->setAutoExclusive(true);
	
	// And disable both of them.
	autoBtn->setEnabled(false);
	manualBtn->setEnabled(false);
	
	// Hide the flip-sides panel, because we don't yet know
	// where the split line is.
	flipSidesFrame->setVisible(false);
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
	
	QIcon const* flip_sides_icon = 0;
	switch (ui_data.pageLayout().type()) {
		case PageLayout::SINGLE_PAGE_UNCUT:
			singlePageUncutBtn->setChecked(true);
			break;
		case PageLayout::LEFT_PAGE_PLUS_OFFCUT:
			flip_sides_icon = &m_flipLeftToRightIcon;
			break;
		case PageLayout::RIGHT_PAGE_PLUS_OFFCUT:
			flip_sides_icon = &m_flipRightToLeftIcon;
			break;
		case PageLayout::TWO_PAGES:
			twoPagesBtn->setChecked(true);
			break;
	}
	
	if (flip_sides_icon) {
		pagePlusOffcutBtn->setChecked(true);
		flipSidesBtn->setIcon(*flip_sides_icon);
		flipSidesFrame->setVisible(true);
	} else {
		flipSidesFrame->setVisible(false);
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
	if (!checked || m_ignoreLayoutTypeToggle) {
		return;
	}
	
	LayoutType lt;
	int logical_pages = 1;
	
	QObject* button = sender();
	if (button == singlePageUncutBtn) {
		lt = SINGLE_PAGE_UNCUT;
	} else if (button == pagePlusOffcutBtn) {
		lt = PAGE_PLUS_OFFCUT;
	} else {
		assert(button == twoPagesBtn);
		lt = TWO_PAGES;
		logical_pages = 2;
	}
	
	Settings::UpdateAction update;
	update.setLayoutType(lt);
	
	scopeLabel->setText(tr("Set manually"));
	
	m_ptrPages->setLogicalPagesInImage(m_imageId, logical_pages);
	
	if (lt == PAGE_PLUS_OFFCUT ||
			(lt != SINGLE_PAGE_UNCUT &&
			m_uiData.splitLineMode() == MODE_AUTO)) {
		m_ptrSettings->updatePage(m_imageId, update);
		emit reloadRequested();
	} else {
		PageLayout::Type plt;
		if (lt == SINGLE_PAGE_UNCUT) {
			plt = PageLayout::SINGLE_PAGE_UNCUT;
		} else {
			assert(lt == TWO_PAGES);
			plt = PageLayout::TWO_PAGES;
		}
		
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
OptionsWidget::showChangeDialog()
{
	Settings::Record const record(m_ptrSettings->getPageRecord(m_imageId));
	Params const* params = record.params();
	if (!params) {
		return;
	}
	
	SplitModeDialog* dialog = new SplitModeDialog(
		this, m_ptrPages, m_pageSelectionAccessor, record.combinedLayoutType(),
		params->pageLayout().type(), params->splitLineMode() == MODE_AUTO
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&, LayoutType)),
		this, SLOT(layoutTypeSet(std::set<PageId> const&, LayoutType))
	);
	dialog->show();
}

void
OptionsWidget::layoutTypeSet(std::set<PageId> const& pages, LayoutType const layout_type)
{
	if (pages.empty()) {
		return;
	}
	
	int const logical_pages = (layout_type == TWO_PAGES) ? 2 : 1;

	if (int(pages.size()) == m_ptrPages->numImages()) {
		m_ptrSettings->setLayoutTypeForAllPages(layout_type);
		if (layout_type != AUTO_LAYOUT_TYPE) {
			m_ptrPages->setLogicalPagesInAllImages(logical_pages);
		}
	} else {
		m_ptrSettings->setLayoutTypeFor(layout_type, pages);
		if (layout_type != AUTO_LAYOUT_TYPE) {
			BOOST_FOREACH(PageId const& page_id, pages) {
				m_ptrPages->setLogicalPagesInImage(
					page_id.imageId(), logical_pages
				);
			}
		}
	}
	
	if (int(pages.size()) > m_ptrPages->numImages() / 2) {
		emit invalidateAllThumbnails();
	} else {
		BOOST_FOREACH(PageId const& page_id, pages) {
			emit invalidateThumbnail(page_id);
		}
	}
	
	if (layout_type == AUTO_LAYOUT_TYPE) {
		scopeLabel->setText(tr("Auto detected"));
		emit reloadRequested();
	} else {
		scopeLabel->setText(tr("Set manually"));
	}
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
OptionsWidget::flipSidesButtonClicked()
{
	ScopedIncDec<int> const guard(m_ignoreAutoManualToggle);
	
	PageLayout::Type new_plt;
	QIcon const* new_flip_sides_icon = 0;
	switch (m_uiData.pageLayout().type()) {
		case PageLayout::LEFT_PAGE_PLUS_OFFCUT:
			new_plt = PageLayout::RIGHT_PAGE_PLUS_OFFCUT;
			new_flip_sides_icon = &m_flipRightToLeftIcon;
			break;
		case PageLayout::RIGHT_PAGE_PLUS_OFFCUT:
			new_plt = PageLayout::LEFT_PAGE_PLUS_OFFCUT;
			new_flip_sides_icon = &m_flipLeftToRightIcon;
			break;
		default:
			return;
	}
	
	PageLayout const new_layout(
		new_plt, m_uiData.pageLayout().splitLine()
	);
	Params const new_params(
		new_layout, m_uiData.dependencies(),
		m_uiData.splitLineMode()
	);
	
	Settings::UpdateAction update;
	update.setParams(new_params);
	m_ptrSettings->updatePage(m_imageId, update);
	
	m_uiData.setPageLayout(new_layout);
	m_uiData.setSplitLineMode(MODE_MANUAL);
	flipSidesBtn->setIcon(*new_flip_sides_icon);
	manualBtn->setChecked(true);
	
	emit pageLayoutSetLocally(new_layout);
	emit invalidateThumbnail(PageId(m_imageId));
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
