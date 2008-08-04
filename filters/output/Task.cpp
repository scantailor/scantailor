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
#include "ColorParams.h"
#include "FilterUiInterface.h"
#include "TaskStatus.h"
#include "FilterData.h"
#include "ImageView.h"
#include "ImageId.h"
#include "Dpi.h"
#include "Utils.h"
#include "ImageTransformation.h"
#include "ThumbnailPixmapCache.h"
#include "DebugImages.h"
#include "OutputGenerator.h"
#include <QImage>
#include <QString>
#include <QObject>
#include <QFileInfo>
#include <QDebug>

using namespace imageproc;

namespace output
{

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		std::auto_ptr<DebugImages> dbg_img,
		PageId const& page_id, QImage const& image, bool batch);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	std::auto_ptr<DebugImages> m_ptrDbg;
	PageId m_pageId;
	QImage m_image;
	bool m_batchProcessing;
};


Task::Task(IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	ThumbnailPixmapCache& thumbnail_cache,
	PageId const& page_id, int const page_num,
	QString const& out_dir, bool const batch, bool const debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_rThumbnailCache(thumbnail_cache),
	m_pageId(page_id),
	m_outDir(out_dir),
	m_pageNum(page_num),
	m_batchProcessing(batch)
{
	if (debug) {
		m_ptrDbg.reset(new DebugImages);
	}
}

Task::~Task()
{
}

FilterResultPtr
Task::process(
	TaskStatus const& status, FilterData const& data,
	QPolygonF const& content_rect_phys, QPolygonF const& page_rect_phys)
{
	status.throwIfCancelled();
	
	ColorParams const params(m_ptrSettings->getColorParams(m_pageId));
	Dpi const output_dpi(m_ptrSettings->getDpi(m_pageId));
	
	OutputGenerator const generator(
		output_dpi, params, data.xform(),
		content_rect_phys, page_rect_phys
	);

	QImage const q_img(generator.process(data.image(), status, m_ptrDbg.get()));
	
	QString const out_path(Utils::outFilePath(m_pageId, m_pageNum, m_outDir));
	q_img.save(out_path);
	
	m_rThumbnailCache.recreateThumbnail(ImageId(out_path), q_img);
	
	return FilterResultPtr(
		new UiUpdater(
			m_ptrFilter, m_ptrDbg, m_pageId,
			q_img, m_batchProcessing
		)
	);
}


/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	std::auto_ptr<DebugImages> dbg_img,
	PageId const& page_id, QImage const& image, bool const batch)
:	m_ptrFilter(filter),
	m_ptrDbg(dbg_img),
	m_pageId(page_id),
	m_image(image),
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
	
	ImageView* view = new ImageView(m_image);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
}

} // namespace output
