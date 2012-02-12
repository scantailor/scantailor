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
#include <boost/foreach.hpp>

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
	
//	connect(autoBtn, SIGNAL(pressed(bool)), this, SLOT(modeChanged(bool)));
	connect(disableBtn, SIGNAL(pressed()), this, SLOT(contentDetectionDisabled()));
	connect(pageDetectAutoBtn, SIGNAL(pressed()), this, SLOT(pageDetectionEnabled()));
	connect(pageDetectDisableBtn, SIGNAL(pressed()), this, SLOT(pageDetectionDisabled()));
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
	disableBtn->setEnabled(false);
	pageDetectAutoBtn->setEnabled(false);
	pageDetectDisableBtn->setEnabled(false);
}

void
OptionsWidget::postUpdateUI(UiData const& ui_data)
{
	m_uiData = ui_data;
	updateModeIndication(ui_data.mode());
	autoBtn->setEnabled(true);
	manualBtn->setEnabled(true);
	disableBtn->setEnabled(true);
	pageDetectAutoBtn->setEnabled(true);
	pageDetectDisableBtn->setEnabled(true);
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
	
	m_uiData.setContentDetection(true);
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
OptionsWidget::contentDetectionDisabled(void)
{
	m_uiData.setContentDetection(false);
	autoBtn->setChecked(false);
	manualBtn->setChecked(false);
	disableBtn->setChecked(true);
	commitCurrentParams();
	emit reloadRequested();
}

void
OptionsWidget::pageDetectionDisabled(void)
{
	m_uiData.setPageDetection(false);
	pageDetectAutoBtn->setChecked(false);
	pageDetectDisableBtn->setChecked(true);
	commitCurrentParams();
	emit reloadRequested();
}

void
OptionsWidget::pageDetectionEnabled(void)
{
	m_uiData.setPageDetection(true);
	pageDetectAutoBtn->setChecked(true);
	pageDetectDisableBtn->setChecked(false);
	commitCurrentParams();
	emit reloadRequested();
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
	disableBtn->setChecked(false);
}

void
OptionsWidget::commitCurrentParams()
{
	Params const params(
		m_uiData.contentRect(), m_uiData.contentSizeMM(),
		m_uiData.dependencies(), m_uiData.mode(), m_uiData.contentDetection(), m_uiData.pageDetection()
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
		m_uiData.dependencies(), m_uiData.mode(), m_uiData.contentDetection(), m_uiData.pageDetection()
	);

	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setPageParams(page_id, params);
		emit invalidateThumbnail(page_id);
	}
}

/*========================= OptionsWidget::UiData ======================*/

OptionsWidget::UiData::UiData()
:	m_mode(MODE_AUTO),
	m_contentDetection(true),
	m_pageDetection(true)
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

void
OptionsWidget::UiData::setContentDetection(bool detect)
{
	m_contentDetection = detect;
}

void
OptionsWidget::UiData::setPageDetection(bool detect)
{
	m_pageDetection = detect;
}

} // namespace select_content
