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
#include "Dependencies.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "Margins.h"
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
		QSizeF const& aggregated_content_size_mm, bool batch);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
	PageId m_pageId;
	QImage m_image;
	ImageTransformation m_xform;
	QRectF m_contentRect;
	QSizeF m_aggregatedContentSizeMM;
	bool m_batchProcessing;
};


Task::Task(IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	PageId const& page_id,
	QSizeF const& aggregated_content_size_mm,
	bool batch, bool debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_aggregatedContentSizeMM(aggregated_content_size_mm),
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
	
	Dependencies const deps(data.xform().transformBack().map(content_rect));
	QSizeF const content_size_mm(calcContentSizeMM(data.xform(), content_rect));
	m_ptrSettings->setContentSizeMM(m_pageId, content_size_mm, deps);
	
	return FilterResultPtr(
		new UiUpdater(
			m_ptrFilter, m_ptrSettings, m_pageId, data.image(),
			data.xform(), content_rect,
			m_aggregatedContentSizeMM, m_batchProcessing
		)
	);
}

QSizeF
Task::calcContentSizeMM(
	ImageTransformation const& xform, QRectF const& content_rect)
{
	PhysicalTransformation const phys_xform(xform.origDpi());
	QTransform const virt_to_mm(xform.transformBack() * phys_xform.pixelsToMM());
	
	QLineF const hor_line(content_rect.topLeft(), content_rect.topRight());
	QLineF const ver_line(content_rect.topLeft(), content_rect.bottomLeft());
	
	double const width = virt_to_mm.map(hor_line).length();
	double const height = virt_to_mm.map(ver_line).length();
	return QSizeF(width, height);
}


/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	PageId const& page_id,
	QImage const& image, ImageTransformation const& xform,
	QRectF const& content_rect,
	QSizeF const& aggregated_content_size_mm, bool const batch)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_image(image),
	m_xform(xform),
	m_contentRect(content_rect),
	m_aggregatedContentSizeMM(aggregated_content_size_mm),
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
	
	ui->invalidateThumbnail(m_pageId);
	
	if (m_batchProcessing) {
		return;
	}
	
	ImageView* view = new ImageView(
		m_ptrSettings, m_pageId, m_image, m_xform,
		m_contentRect, *opt_widget
	);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
	
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
}

} // namespace page_layout
