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

#include "OptionsWidget.h"
#include "OptionsWidget.h.moc"
#include "ApplyDialog.h"
#include "Settings.h"
#include "Params.h"
#include "ScopedIncDec.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif

namespace select_content
{

OptionsWidget::OptionsWidget(
	IntrusivePtr<Settings> const& settings,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings),
	m_pageSelectionAccessor(page_selection_accessor),
	m_ignoreAutoManualToggle(0)
{
	setupUi(this);
	
	connect(autoBtn, SIGNAL(toggled(bool)), this, SLOT(modeChanged(bool)));
	connect(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	m_pageId = page_id;
	autoBtn->setChecked(true);
	autoBtn->setEnabled(false);
	manualBtn->setEnabled(false);
}

void
OptionsWidget::postUpdateUI(UiData const& ui_data)
{
	m_uiData = ui_data;
	updateModeIndication(ui_data.mode());
	autoBtn->setEnabled(true);
	manualBtn->setEnabled(true);
}

void
OptionsWidget::manualContentRectSet(QRectF const& content_rect)
{
	m_uiData.setContentRect(content_rect);
	m_uiData.setMode(MODE_MANUAL);
	updateModeIndication(MODE_MANUAL);
	commitCurrentParams();
	
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::modeChanged(bool const auto_mode)
{
	if (m_ignoreAutoManualToggle) {
		return;
	}
	
	if (auto_mode) {
		m_uiData.setMode(MODE_AUTO);
		m_ptrSettings->clearPageParams(m_pageId);
		emit reloadRequested();
	} else {
		m_uiData.setMode(MODE_MANUAL);
		commitCurrentParams();
	}
}

void
OptionsWidget::updateModeIndication(AutoManualMode const mode)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	if (mode == MODE_AUTO) {
		autoBtn->setChecked(true);
	} else {
		manualBtn->setChecked(true);
	}
}

void
OptionsWidget::commitCurrentParams()
{
	Params const params(
		m_uiData.contentRect(), m_uiData.contentSizeMM(),
		m_uiData.dependencies(), m_uiData.mode()
	);
	m_ptrSettings->setPageParams(m_pageId, params);
}

void
OptionsWidget::showApplyToDialog()
{
	ApplyDialog* dialog = new ApplyDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(applySelection(std::set<PageId> const&)),
		this, SLOT(applySelection(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::applySelection(std::set<PageId> const& pages)
{
	if (pages.empty()) {
		return;
	}
	
	Params const params(
		m_uiData.contentRect(), m_uiData.contentSizeMM(),
		m_uiData.dependencies(), m_uiData.mode()
	);

	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setPageParams(page_id, params);
		emit invalidateThumbnail(page_id);
	}
}

/*========================= OptionsWidget::UiData ======================*/

OptionsWidget::UiData::UiData()
:	m_mode(MODE_AUTO)
{
}

OptionsWidget::UiData::~UiData()
{
}

void
OptionsWidget::UiData::setSizeCalc(PhysSizeCalc const& calc)
{
	m_sizeCalc = calc;
}

void
OptionsWidget::UiData::setContentRect(QRectF const& content_rect)
{
	m_contentRect = content_rect;
}

QRectF const&
OptionsWidget::UiData::contentRect() const
{
	return m_contentRect;
}

QSizeF
OptionsWidget::UiData::contentSizeMM() const
{
	return m_sizeCalc.sizeMM(m_contentRect);
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

} // namespace select_content
