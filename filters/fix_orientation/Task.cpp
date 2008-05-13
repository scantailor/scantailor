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
#include "FilterData.h"
#include "ImageTransformation.h"
#include "filters/page_split/Task.h"
#include "PageId.h"
#include "TaskStatus.h"
#include "ImageView.h"
#include "FilterUiInterface.h"
#include <QImage>

namespace fix_orientation
{

using imageproc::BinaryThreshold;

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		QImage const& image, ImageTransformation const& xform);
	
	virtual void updateUI(FilterUiInterface* wnd);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	QImage m_image;
	ImageTransformation m_xform;
};


Task::Task(
	ImageId const& image_id,
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<page_split::Task> const& next_task)
:	m_ptrFilter(filter),
	m_ptrNextTask(next_task),
	m_ptrSettings(settings),
	m_imageId(image_id)
{
}

Task::~Task()
{
}

FilterResultPtr
Task::process(TaskStatus const& status, FilterData const& data)
{
	// This function is executed from the worker thread.
	
	status.throwIfCancelled();
	
	ImageTransformation xform(data.xform());
	xform.setPreRotation(m_ptrSettings->getRotationFor(m_imageId));
	
	if (m_ptrNextTask) {
		return m_ptrNextTask->process(status, FilterData(data, xform));
	} else {
		return FilterResultPtr(new UiUpdater(m_ptrFilter, data.image(), xform));
	}
}


/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	QImage const& image, ImageTransformation const& xform)
:	m_ptrFilter(filter),
	m_image(image),
	m_xform(xform)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.
	
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI(m_xform.preRotation());
	ui->setOptionsWidget(opt_widget);
	
	ImageView* view = new ImageView(m_image, m_xform);
	ui->setImageWidget(view);
	QObject::connect(
		opt_widget, SIGNAL(rotated(OrthogonalRotation)),
		view, SLOT(setPreRotation(OrthogonalRotation))
	);
}

} // namespace fix_orientation
