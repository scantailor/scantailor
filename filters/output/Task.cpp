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

#include "Task.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Params.h"
#include "Settings.h"
#include "ColorParams.h"
#include "OutputParams.h"
#include "OutputImageParams.h"
#include "OutputFileParams.h"
#include "RenderParams.h"
#include "FilterUiInterface.h"
#include "TaskStatus.h"
#include "FilterData.h"
#include "ImageView.h"
#include "ImageViewTab.h"
#include "TabbedImageView.h"
#include "PictureZoneComparator.h"
#include "PictureZoneEditor.h"
#include "FillZoneComparator.h"
#include "FillZoneEditor.h"
#include "DespeckleState.h"
#include "DespeckleView.h"
#include "DespeckleVisualization.h"
#include "DespeckleLevel.h"
#include "DewarpingMode.h"
#include "dewarping/DistortionModel.h"
#include "dewarping/DewarpingPointMapper.h"
#include "DewarpingView.h"
#include "ImageId.h"
#include "PageId.h"
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
#include "imageproc/PolygonUtils.h"
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include <QImage>
#include <QString>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTabWidget>
#include <QCoreApplication>
#include <QDebug>

#include "CommandLine.h"

using namespace imageproc;
using namespace dewarping;

namespace output
{

class Task::UiUpdater : public FilterResult
{
	Q_DECLARE_TR_FUNCTIONS(output::Task::UiUpdater)
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		IntrusivePtr<Settings> const& settings,
		std::auto_ptr<DebugImages> dbg_img,
		Params const& params,
		ImageTransformation const& xform,
		QRect const& virt_content_rect,
		PageId const& page_id,
		QImage const& orig_image,
		QImage const& output_image,
		BinaryImage const& picture_mask,
		DespeckleState const& despeckle_state,
		DespeckleVisualization const& despeckle_visualization,
		bool batch, bool debug);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
	std::auto_ptr<DebugImages> m_ptrDbg;
	Params m_params;
	ImageTransformation m_xform;
	QRect m_virtContentRect;
	PageId m_pageId;
	QImage m_origImage;
	QImage m_downscaledOrigImage;
	QImage m_outputImage;
	QImage m_downscaledOutputImage;
	BinaryImage m_pictureMask;
	DespeckleState m_despeckleState;
	DespeckleVisualization m_despeckleVisualization;
	DespeckleLevel m_despeckleLevel;
	bool m_batchProcessing;
	bool m_debug;
};


Task::Task(IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	PageId const& page_id, OutputFileNameGenerator const& out_file_name_gen,
	ImageViewTab const last_tab, bool const batch, bool const debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_ptrThumbnailCache(thumbnail_cache),
	m_pageId(page_id),
	m_outFileNameGen(out_file_name_gen),
	m_lastTab(last_tab),
	m_batchProcessing(batch),
	m_debug(debug)
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
	QPolygonF const& content_rect_phys)
{
	status.throwIfCancelled();

	Params params(m_ptrSettings->getParams(m_pageId));
	RenderParams const render_params(params.colorParams());
	QString const out_file_path(m_outFileNameGen.filePathFor(m_pageId));
	QFileInfo const out_file_info(out_file_path);

	ImageTransformation new_xform(data.xform());
	new_xform.postScaleToDpi(params.outputDpi());

	QString const automask_dir(Utils::automaskDir(m_outFileNameGen.outDir()));
	QString const automask_file_path(
		QDir(automask_dir).absoluteFilePath(out_file_info.fileName())
	);
	QFileInfo automask_file_info(automask_file_path);

	QString const speckles_dir(Utils::specklesDir(m_outFileNameGen.outDir()));
	QString const speckles_file_path(
		QDir(speckles_dir).absoluteFilePath(out_file_info.fileName())
	);
	QFileInfo speckles_file_info(speckles_file_path);

	bool const need_picture_editor = render_params.mixedOutput() && !m_batchProcessing;
	bool const need_speckles_image = params.despeckleLevel() != DESPECKLE_OFF
		&& params.colorParams().colorMode() != ColorParams::COLOR_GRAYSCALE && !m_batchProcessing;
	
	OutputGenerator const generator(
		params.outputDpi(), params.colorParams(), params.despeckleLevel(),
		new_xform, content_rect_phys
	);
	
	OutputImageParams new_output_image_params(
		generator.outputImageSize(), generator.outputContentRect(),
		new_xform, params.outputDpi(), params.colorParams(),
		params.dewarpingMode(), params.distortionModel(),
		params.depthPerception(), params.despeckleLevel() 
	);

	ZoneSet const new_picture_zones(m_ptrSettings->pictureZonesForPage(m_pageId));
	ZoneSet const new_fill_zones(m_ptrSettings->fillZonesForPage(m_pageId));
	
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

		if (!PictureZoneComparator::equal(stored_output_params->pictureZones(), new_picture_zones)) {
			need_reprocess = true;
			break;
		}

		if (!FillZoneComparator::equal(stored_output_params->fillZones(), new_fill_zones)) {
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

		if (need_picture_editor) {
			if (!automask_file_info.exists()) {
				need_reprocess = true;
				break;
			}

			if (!stored_output_params->automaskFileParams().matches(OutputFileParams(automask_file_info))) {
				need_reprocess = true;
				break;
			}
		}

		if (need_speckles_image) {
			if (!speckles_file_info.exists()) {
				need_reprocess = true;
				break;
			}
			if (!stored_output_params->specklesFileParams().matches(OutputFileParams(speckles_file_info))) {
				need_reprocess = true;
				break;
			}
		}
	
	} while (false);
	
	QImage out_img;
	BinaryImage automask_img;
	BinaryImage speckles_img;
	
	if (!need_reprocess) {
		QFile out_file(out_file_path);
		if (out_file.open(QIODevice::ReadOnly)) {
			out_img = ImageLoader::load(out_file, 0);
		}
		need_reprocess = out_img.isNull();

		if (need_picture_editor && !need_reprocess) {
			QFile automask_file(automask_file_path);
			if (automask_file.open(QIODevice::ReadOnly)) {
				automask_img = BinaryImage(ImageLoader::load(automask_file, 0));
			}
			need_reprocess = automask_img.isNull() || automask_img.size() != out_img.size();
		}

		if (need_speckles_image && !need_reprocess) {
			QFile speckles_file(speckles_file_path);
			if (speckles_file.open(QIODevice::ReadOnly)) {
				speckles_img = BinaryImage(ImageLoader::load(speckles_file, 0));
			}
			need_reprocess = speckles_img.isNull();
		}
	}

	if (need_reprocess) {
		// Even in batch processing mode we should still write automask, because it
		// will be needed when we view the results back in interactive mode.
		// The same applies even more to speckles file, as we need it not only
		// for visualization purposes, but also for re-doing despeckling at
		// different levels without going through the whole output generation process.
		bool const write_automask = render_params.mixedOutput();
		bool const write_speckles_file = params.despeckleLevel() != DESPECKLE_OFF &&
			params.colorParams().colorMode() != ColorParams::COLOR_GRAYSCALE; 

		automask_img = BinaryImage();
		speckles_img = BinaryImage();

		DistortionModel distortion_model;
		if (params.dewarpingMode() == DewarpingMode::MANUAL) {
			distortion_model = params.distortionModel();
		}
		// OutputGenerator will write a new distortion model
		// there, if dewarping mode is AUTO.

		out_img = generator.process(
			status, data, new_picture_zones, new_fill_zones,
			params.dewarpingMode(), distortion_model,
			params.depthPerception(),
			write_automask ? &automask_img : 0,
			write_speckles_file ? &speckles_img : 0,
			m_ptrDbg.get()
		);

		if (params.dewarpingMode() == DewarpingMode::AUTO && distortion_model.isValid()) {
			// A new distortion model was generated.
			// We need to save it to be able to modify it manually.
			params.setDistortionModel(distortion_model);
			m_ptrSettings->setParams(m_pageId, params);
			new_output_image_params.setDistortionModel(distortion_model);
		}

		if (write_speckles_file && speckles_img.isNull()) {
			// Even if despeckling didn't actually take place, we still need
			// to write an empty speckles file.  Making it a special case
			// is simply not worth it.
			BinaryImage(out_img.size(), WHITE).swap(speckles_img);
		}

		bool invalidate_params = false;
		
		if (!TiffWriter::writeImage(out_file_path, out_img)) {
			invalidate_params = true;
		} else {
			deleteMutuallyExclusiveOutputFiles();
		}

		if (write_automask) {
			// Note that QDir::mkdir() will fail if the parent directory,
			// that is $OUT/cache doesn't exist. We want that behaviour,
			// as otherwise when loading a project from a different machine,
			// a whole bunch of bogus directories would be created.
			QDir().mkdir(automask_dir);
			// Also note that QDir::mkdir() will fail if the directory already exists,
			// so we ignore its return value here.

			if (!TiffWriter::writeImage(automask_file_path, automask_img.toQImage())) {
				invalidate_params = true;
			}
		}
		if (write_speckles_file) {
			if (!QDir().mkpath(speckles_dir)) {
				invalidate_params = true;
			} else if (!TiffWriter::writeImage(speckles_file_path, speckles_img.toQImage())) {
				invalidate_params = true;
			}
		}

		if (invalidate_params) {
			m_ptrSettings->removeOutputParams(m_pageId);
		} else {
			// Note that we can't reuse *_file_info objects
			// as we've just overwritten those files.
			OutputParams const out_params(
				new_output_image_params,
				OutputFileParams(QFileInfo(out_file_path)),
				write_automask ? OutputFileParams(QFileInfo(automask_file_path))
				: OutputFileParams(),
				write_speckles_file ? OutputFileParams(QFileInfo(speckles_file_path))
				: OutputFileParams(),
				new_picture_zones, new_fill_zones
			);

			m_ptrSettings->setOutputParams(m_pageId, out_params);
		}
		
		m_ptrThumbnailCache->recreateThumbnail(ImageId(out_file_path), out_img);
	}

	DespeckleState const despeckle_state(
		out_img, speckles_img, params.despeckleLevel(), params.outputDpi()
	);

	DespeckleVisualization despeckle_visualization;
	if (m_lastTab == TAB_DESPECKLING) {
		// Because constructing DespeckleVisualization takes a noticeable
		// amount of time, we only do it if we are sure we'll need it.
		// Otherwise it will get constructed on demand.
		despeckle_visualization = despeckle_state.visualize();
	}

	if (CommandLine::get().isGui()) {
		return FilterResultPtr(
			new UiUpdater(
				m_ptrFilter, m_ptrSettings, m_ptrDbg, params,
				new_xform, generator.outputContentRect(),
				m_pageId, data.origImage(), out_img, automask_img,
				despeckle_state, despeckle_visualization,
				m_batchProcessing, m_debug
			)
		);
	} else {
		return FilterResultPtr(0);
	}
}

/**
 * Delete output files mutually exclusive to m_pageId.
 */
void
Task::deleteMutuallyExclusiveOutputFiles()
{
	switch (m_pageId.subPage()) {
		case PageId::SINGLE_PAGE:
			QFile::remove(
				m_outFileNameGen.filePathFor(
					PageId(m_pageId.imageId(), PageId::LEFT_PAGE)
				)
			);
			QFile::remove(
				m_outFileNameGen.filePathFor(
					PageId(m_pageId.imageId(), PageId::RIGHT_PAGE)
				)
			);
			break;
		case PageId::LEFT_PAGE:
		case PageId::RIGHT_PAGE:
			QFile::remove(
				m_outFileNameGen.filePathFor(
					PageId(m_pageId.imageId(), PageId::SINGLE_PAGE)
				)
			);
			break;
	}
}


/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	std::auto_ptr<DebugImages> dbg_img,
	Params const& params,
	ImageTransformation const& xform,
	QRect const& virt_content_rect,
	PageId const& page_id,
	QImage const& orig_image,
	QImage const& output_image,
	BinaryImage const& picture_mask,
	DespeckleState const& despeckle_state,
	DespeckleVisualization const& despeckle_visualization,
	bool const batch, bool const debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_ptrDbg(dbg_img),
	m_params(params),
	m_xform(xform),
	m_virtContentRect(virt_content_rect),
	m_pageId(page_id),
	m_origImage(orig_image),
	m_downscaledOrigImage(ImageView::createDownscaledImage(orig_image)),
	m_outputImage(output_image),
	m_downscaledOutputImage(ImageView::createDownscaledImage(output_image)),
	m_pictureMask(picture_mask),
	m_despeckleState(despeckle_state),
	m_despeckleVisualization(despeckle_visualization),
	m_batchProcessing(batch),
	m_debug(debug)
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

	std::auto_ptr<ImageViewBase> image_view(
		new ImageView(m_outputImage, m_downscaledOutputImage)
	);
	QPixmap const downscaled_output_pixmap(image_view->downscaledPixmap());

	std::auto_ptr<ImageViewBase> dewarping_view(
		new DewarpingView(
			m_origImage, m_downscaledOrigImage, m_xform.transform(),
			PolygonUtils::convexHull(
				(m_xform.resultingPreCropArea() + m_xform.resultingPostCropArea()).toStdVector()
			),
			m_virtContentRect, m_pageId, m_params.dewarpingMode(),
			m_params.distortionModel(), opt_widget->depthPerception()
		)
	);
	QPixmap const downscaled_orig_pixmap(dewarping_view->downscaledPixmap());
	QObject::connect(
		opt_widget, SIGNAL(depthPerceptionChanged(double)),
		dewarping_view.get(), SLOT(depthPerceptionChanged(double))
	);
	QObject::connect(
		dewarping_view.get(), SIGNAL(distortionModelChanged(dewarping::DistortionModel const&)),
		opt_widget, SLOT(distortionModelChanged(dewarping::DistortionModel const&))
	);

	std::auto_ptr<QWidget> picture_zone_editor;
	if (m_pictureMask.isNull()) {
		picture_zone_editor.reset(
			new ErrorWidget(tr("Picture zones are only available in Mixed mode."))
		);
	} else {
		picture_zone_editor.reset(
			new PictureZoneEditor(
				m_origImage, downscaled_orig_pixmap, m_pictureMask,
				m_xform.transform(), m_xform.resultingPostCropArea(),
				m_pageId, m_ptrSettings
			)
		);
		QObject::connect(
			picture_zone_editor.get(), SIGNAL(invalidateThumbnail(PageId const&)),
			opt_widget, SIGNAL(invalidateThumbnail(PageId const&))
		);
	}

	// We make sure we never need to update the original <-> output
	// mapping at run time, that is without reloading.
	// In OptionsWidget::dewarpingChanged() we make sure to reload
	// if we are on the "Fill Zones" tab, and if not, it will be reloaded
	// anyway when another tab is selected.
	boost::function<QPointF(QPointF const&)> orig_to_output;
	boost::function<QPointF(QPointF const&)> output_to_orig;
	if (m_params.dewarpingMode() != DewarpingMode::OFF && m_params.distortionModel().isValid()) {
		boost::shared_ptr<DewarpingPointMapper> mapper(
			new DewarpingPointMapper(
				m_params.distortionModel(), m_params.depthPerception().value(),
				m_xform.transform(), m_virtContentRect
			)
		);
		orig_to_output = boost::bind(&DewarpingPointMapper::mapToDewarpedSpace, mapper, _1);
		output_to_orig = boost::bind(&DewarpingPointMapper::mapToWarpedSpace, mapper, _1);
	} else {
		typedef QPointF (QTransform::*MapPointFunc)(QPointF const&) const;
		orig_to_output = boost::bind((MapPointFunc)&QTransform::map, m_xform.transform(), _1);
		output_to_orig = boost::bind((MapPointFunc)&QTransform::map, m_xform.transformBack(), _1);
	}

	std::auto_ptr<QWidget> fill_zone_editor(
		new FillZoneEditor(
			m_outputImage, downscaled_output_pixmap,
			orig_to_output, output_to_orig, m_pageId, m_ptrSettings
		)
	);
	QObject::connect(
		fill_zone_editor.get(), SIGNAL(invalidateThumbnail(PageId const&)),
		opt_widget, SIGNAL(invalidateThumbnail(PageId const&))
	);

	std::auto_ptr<QWidget> despeckle_view;
	if (m_params.colorParams().colorMode() == ColorParams::COLOR_GRAYSCALE) {
		despeckle_view.reset(
			new ErrorWidget(tr("Despeckling can't be done in Color / Grayscale mode."))
		);
	} else {
		despeckle_view.reset(
			new DespeckleView(
				m_despeckleState, m_despeckleVisualization, m_debug
			)
		);
		QObject::connect(
			opt_widget, SIGNAL(despeckleLevelChanged(DespeckleLevel, bool*)),
			despeckle_view.get(), SLOT(despeckleLevelChanged(DespeckleLevel, bool*))
		);
	}

	std::auto_ptr<TabbedImageView> tab_widget(new TabbedImageView);
	tab_widget->setDocumentMode(true);
	tab_widget->setTabPosition(QTabWidget::East);
	tab_widget->addTab(image_view.release(), tr("Output"), TAB_OUTPUT);
	tab_widget->addTab(picture_zone_editor.release(), tr("Picture Zones"), TAB_PICTURE_ZONES);
	tab_widget->addTab(fill_zone_editor.release(), tr("Fill Zones"), TAB_FILL_ZONES);
	tab_widget->addTab(dewarping_view.release(), tr("Dewarping"), TAB_DEWARPING);
	tab_widget->addTab(despeckle_view.release(), tr("Despeckling"), TAB_DESPECKLING);
	tab_widget->setCurrentTab(opt_widget->lastTab());

	QObject::connect(
		tab_widget.get(), SIGNAL(tabChanged(ImageViewTab)),
		opt_widget, SLOT(tabChanged(ImageViewTab))
	);

	ui->setImageWidget(tab_widget.release(), ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
}

} // namespace output
