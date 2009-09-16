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
#include "PictureZoneEditor.h"
#include "ImageId.h"
#include "Dpi.h"
#include "Dpm.h"
#include "Utils.h"
#include "ImageTransformation.h"
#include "ThumbnailPixmapCache.h"
#include "DebugImages.h"
#include "OutputGenerator.h"
#include "TiffWriter.h"
#include "ImageLoader.h"
#include "ErrorWidget.h"
#include "imageproc/BinaryImage.h"
#include <QImage>
#include <QString>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTabWidget>
#include <QCoreApplication>
#include <QDebug>

using namespace imageproc;

namespace output
{

class Task::UiUpdater : public FilterResult
{
	Q_DECLARE_TR_FUNCTIONS(output::Task)
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		IntrusivePtr<Settings> const& settings,
		std::auto_ptr<DebugImages> dbg_img,
		QTransform const& image_to_virt,
		QPolygonF const& virt_display_area,
		PageId const& page_id,
		QImage const& orig_image,
		QImage const& output_image,
		BinaryImage const& picture_mask,
		bool batch);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
	std::auto_ptr<DebugImages> m_ptrDbg;
	QTransform m_imageToVirt;
	QPolygonF m_virtDisplayArea;
	PageId m_pageId;
	QImage m_origImage;
	QImage m_downscaledOrigImage;
	QImage m_outputImage;
	QImage m_downscaledOutputImage;
	BinaryImage m_pictureMask;
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
	QString const out_file_path(Utils::outFilePath(m_pageId, m_pageNum, m_outDir));
	QFileInfo const out_file_info(out_file_path);
	QString const automask_dir(Utils::automaskDir(m_outDir));
	QString const automask_file_path(
		QDir(automask_dir).absoluteFilePath(out_file_info.fileName())
	);
	QFileInfo automask_file_info(automask_file_path);
	bool const need_automask = color_params.colorMode() == ColorParams::MIXED
							   && !m_batchProcessing;
	
	OutputGenerator const generator(
		output_dpi, color_params, data.xform(),
		content_rect_phys, page_rect_phys
	);
	
	OutputImageParams const new_output_image_params(
		generator.outputImageSize(), generator.outputContentRect(),
		data.xform(), output_dpi, color_params
	);

	PictureZoneList const new_zones(m_ptrSettings->zonesForPage(m_pageId));
	
	bool need_reprocess = false;
	do { // Just to be able to break from it.
		
		std::auto_ptr<OutputParams> stored_output_params(
			m_ptrSettings->getOutputParams(m_pageId)
		);
		
		if (!stored_output_params.get()) {
			need_reprocess = true;
			break;
		}
		
		if (!stored_output_params->outputImageParams().matches(new_output_image_params)) {
			need_reprocess = true;
			break;
		}

		if (stored_output_params->zones() != new_zones) {
			need_reprocess = true;
			break;
		}
		
		if (!out_file_info.exists()) {
			need_reprocess = true;
			break;
		}
		
		if (!stored_output_params->outputFileParams().matches(OutputFileParams(out_file_info))) {
			need_reprocess = true;
			break;
		}

		if (need_automask) {
			if (!automask_file_info.exists()) {
				need_reprocess = true;
				break;
			}

			if (!stored_output_params->automaskFileParams().matches(OutputFileParams(automask_file_info))) {
				need_reprocess = true;
				break;
			}
		}
	
	} while (false);
	
	QImage out_img;
	BinaryImage automask_img;
	
	if (!need_reprocess) {
		QFile out_file(out_file_path);
		if (out_file.open(QIODevice::ReadOnly)) {
			out_img = ImageLoader::load(out_file, 0);
		}
		need_reprocess = out_img.isNull();

		if (need_automask && !need_reprocess) {
			QFile automask_file(automask_file_path);
			if (automask_file.open(QIODevice::ReadOnly)) {
				automask_img = BinaryImage(ImageLoader::load(automask_file, 0));
			}
			need_reprocess = automask_img.isNull() || automask_img.size() != out_img.size();
		}
	}

	if (need_reprocess) {
		// Even in batch processing mode we should still write automask, because it
		// will be needed when we view the results back in interactive mode.
		bool const write_automask = (color_params.colorMode() == ColorParams::MIXED);

		out_img = generator.process(
			status, data, new_zones, write_automask ? &automask_img : 0, m_ptrDbg.get()
		);
		
		// Note that automask should be generated and written even if need_automask is false.
		// That's because

		if (!TiffWriter::writeImage(out_file_path, out_img) ||
				(write_automask && (!QDir().mkpath(automask_dir) ||
				!TiffWriter::writeImage(automask_file_path, automask_img.toQImage())))) {
			m_ptrSettings->removeOutputParams(m_pageId);
		} else {
			// Note that we can't reuse out_file_info and automask_file_info,
			// as we've just written a new versions of these files.
			OutputParams const out_params(
				new_output_image_params,
				OutputFileParams(QFileInfo(out_file_path)), write_automask ?
				OutputFileParams(QFileInfo(automask_file_path)) : OutputFileParams(),
				new_zones
			);

			m_ptrSettings->setOutputParams(m_pageId, out_params);
		}
		
		m_rThumbnailCache.recreateThumbnail(ImageId(out_file_path), out_img);
	}
	
	return FilterResultPtr(
		new UiUpdater(
			m_ptrFilter, m_ptrSettings, m_ptrDbg, generator.toOutput(),
			QRectF(QPointF(0.0, 0.0), generator.outputImageSize()),
			m_pageId, data.origImage(), out_img, automask_img,
			m_batchProcessing
		)
	);
}


/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	std::auto_ptr<DebugImages> dbg_img,
	QTransform const& image_to_virt,
	QPolygonF const& virt_display_area,
	PageId const& page_id,
	QImage const& orig_image,
	QImage const& output_image,
	BinaryImage const& picture_mask,
	bool const batch)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_ptrDbg(dbg_img),
	m_imageToVirt(image_to_virt),
	m_virtDisplayArea(virt_display_area),
	m_pageId(page_id),
	m_origImage(orig_image),
	m_downscaledOrigImage(ImageView::createDownscaledImage(orig_image)),
	m_outputImage(output_image),
	m_downscaledOutputImage(ImageView::createDownscaledImage(output_image)),
	m_pictureMask(picture_mask),
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

	std::auto_ptr<QWidget> image_view(
		new ImageView(m_outputImage, m_downscaledOutputImage)
	);

	std::auto_ptr<QWidget> zone_editor;
	if (m_pictureMask.isNull()) {
		zone_editor.reset(
			new ErrorWidget(tr("Picture zones are only available in Mixed mode."))
		);
	} else {
		zone_editor.reset(
			new PictureZoneEditor(
				m_origImage, m_downscaledOrigImage, m_pictureMask,
				m_imageToVirt, m_virtDisplayArea,
				m_pageId, m_ptrSettings
			)
		);
	}

	std::auto_ptr<QTabWidget> tab_widget(new QTabWidget);
	tab_widget->setTabPosition(QTabWidget::East);
	tab_widget->addTab(image_view.release(), tr("Output"));
	tab_widget->addTab(zone_editor.release(), tr("Picture Zones"));

	QObject::connect(
		tab_widget.get(), SIGNAL(currentChanged(int)),
		opt_widget, SLOT(reloadIfZonesChanged())
	);

	ui->setImageWidget(tab_widget.release(), ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
}

} // namespace output
