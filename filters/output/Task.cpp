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

#include "Task.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "ColorParams.h"
#include "OutputParams.h"
#include "OutputImageParams.h"
#include "OutputFileParams.h"
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
#include "TiffWriter.h"
#include "TiffReader.h"
#include <QImage>
#include <QString>
#include <QObject>
#include <QFile>
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
	QImage m_downscaledImage;
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
	
	ColorParams const color_params(m_ptrSettings->getColorParams(m_pageId));
	Dpi const output_dpi(m_ptrSettings->getDpi(m_pageId));
	QString const out_path(Utils::outFilePath(m_pageId, m_pageNum, m_outDir));
	
	OutputGenerator const generator(
		output_dpi, color_params, data.xform(),
		content_rect_phys, page_rect_phys
	);
	
	OutputImageParams const new_output_image_params(
		generator.outputImageSize(), generator.outputContentRect(),
		data.xform(), output_dpi, color_params
	);
	
	bool regenerate_file = false;
	do { // Just to be able to break from it.
		
		std::auto_ptr<OutputParams> stored_output_params(
			m_ptrSettings->getOutputParams(m_pageId)
		);
		
		if (!stored_output_params.get()) {
			regenerate_file = true;
			break;
		}
		
		if (!stored_output_params->imageParams().matches(new_output_image_params)) {
			regenerate_file = true;
			break;
		}
		
		QFileInfo existing_file_info(out_path);
		if (!existing_file_info.exists()) {
			regenerate_file = true;
			break;
		}
		
		if (!stored_output_params->fileParams().matches(OutputFileParams(existing_file_info))) {
			regenerate_file = true;
			break;
		}
	
	} while (false);
	
	QImage q_img;
	
	if (!regenerate_file) {
		QFile file(out_path);
		if (file.open(QIODevice::ReadOnly)) {
			q_img = TiffReader::readImage(file);
		}
		regenerate_file = q_img.isNull();
	}
	
	if (regenerate_file) {
		q_img = generator.process(data, status, m_ptrDbg.get());
		
		if (!TiffWriter::writeImage(out_path, q_img)) {
			m_ptrSettings->removeOutputParams(m_pageId);
		} else {
			QFileInfo const new_file_info(out_path);
			OutputFileParams const new_file_params(new_file_info);
			m_ptrSettings->setOutputParams(
				m_pageId,
				OutputParams(new_output_image_params, new_file_params)
			);
		}
		
		m_rThumbnailCache.recreateThumbnail(ImageId(out_path), q_img);
	}
	
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
	m_downscaledImage(ImageView::createDownscaledImage(image)),
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
	
	ImageView* view = new ImageView(m_image, m_downscaledImage);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
}

} // namespace output
