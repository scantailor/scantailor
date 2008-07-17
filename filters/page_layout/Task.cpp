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

#include "Task.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "Margins.h"
#include "Utils.h"
#include "FilterUiInterface.h"
#include "TaskStatus.h"
#include "FilterData.h"
#include "ImageView.h"
#include "ImageTransformation.h"
#include "PhysicalTransformation.h"
#include <QSizeF>
#include <QRectF>
#include <QLineF>
#include <QTransform>
#include <QObject>

namespace page_layout
{

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		IntrusivePtr<Settings> const& settings,
		PageId const& page_id,
		QImage const& image,
		ImageTransformation const& xform,
		QRectF const& content_rect,
		Settings::AggregateSizeChanged const agg_size_changed,
		bool batch);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
	PageId m_pageId;
	QImage m_image;
	ImageTransformation m_xform;
	QRectF m_contentRect;
	Settings::AggregateSizeChanged m_aggSizeChanged;
	bool m_batchProcessing;
};


Task::Task(IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	PageId const& page_id, bool batch, bool debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_batchProcessing(batch)
{
}

Task::~Task()
{
}

FilterResultPtr
Task::process(
	TaskStatus const& status, FilterData const& data,
	QRectF const& content_rect)
{
	status.throwIfCancelled();
	
	//Dependencies const deps(data.xform().transformBack().map(content_rect));
	QSizeF const content_size_mm(
		Utils::calcRectSizeMM(data.xform(), content_rect)
	);
	
	Settings::AggregateSizeChanged const agg_size_changed(
		m_ptrSettings->setContentSizeMM(
			m_pageId, content_size_mm
		)
	);
	
	return FilterResultPtr(
		new UiUpdater(
			m_ptrFilter, m_ptrSettings, m_pageId, data.image(),
			data.xform(), content_rect, agg_size_changed,
			m_batchProcessing
		)
	);
}


/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	PageId const& page_id,
	QImage const& image, ImageTransformation const& xform,
	QRectF const& content_rect,
	Settings::AggregateSizeChanged const agg_size_changed,
	bool const batch)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_image(image),
	m_xform(xform),
	m_contentRect(content_rect),
	m_aggSizeChanged(agg_size_changed),
	m_batchProcessing(batch)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.
	
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI();
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);
	
	if (m_aggSizeChanged == Settings::AGGREGATE_SIZE_CHANGED) {
		ui->invalidateAllThumbnails();
	} else {
		ui->invalidateThumbnail(m_pageId);
	}
	
	if (m_batchProcessing) {
		return;
	}
	
	ImageView* view = new ImageView(
		m_ptrSettings, m_pageId, m_image, m_xform,
		m_contentRect, *opt_widget
	);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
	
	QObject::connect(
		view, SIGNAL(invalidateThumbnail(PageId const&)),
		opt_widget, SIGNAL(invalidateThumbnail(PageId const&))
	);
	QObject::connect(
		view, SIGNAL(invalidateAllThumbnails()),
		opt_widget, SIGNAL(invalidateAllThumbnails())
	);
	QObject::connect(
		view, SIGNAL(marginsSetLocally(Margins const&)),
		opt_widget, SLOT(marginsSetExternally(Margins const&))
	);
	QObject::connect(
		opt_widget, SIGNAL(marginsSetLocally(Margins const&)),
		view, SLOT(marginsSetExternally(Margins const&))
	);
	QObject::connect(
		opt_widget, SIGNAL(topBottomLinkToggled(bool)),
		view, SLOT(topBottomLinkToggled(bool))
	);
	QObject::connect(
		opt_widget, SIGNAL(leftRightLinkToggled(bool)),
		view, SLOT(leftRightLinkToggled(bool))
	);
	QObject::connect(
		opt_widget, SIGNAL(alignmentChanged(Alignment const&)),
		view, SLOT(alignmentChanged(Alignment const&))
	);
	QObject::connect(
		opt_widget, SIGNAL(aggregateHardSizeChanged()),
		view, SLOT(aggregateHardSizeChanged())
	);
}

} // namespace page_layout
