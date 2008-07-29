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
#include "Params.h"
#include "FilterUiInterface.h"
#include "TaskStatus.h"
#include "FilterData.h"
#include "ImageView.h"
#include "Dpi.h"
#include "Dpm.h"
#include "ImageTransformation.h"
#include "DebugImages.h"
#include "PerformanceTimer.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BWColor.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/Binarize.h"
#include "imageproc/Constants.h"
#include "imageproc/Transform.h"
#include "imageproc/kFill.h"
#include "imageproc/Morphology.h"
#include "imageproc/SeedFill.h"
#include "imageproc/PolygonRasterizer.h"
#include <QRectF>
#include <QPolygonF>
#include <QPoint>
#include <QSize>
#include <QSizeF>
#include <QTransform>
#include <QImage>
#include <QColor>
#include <QObject>
#include <QFileInfo>
#include <QDebug>
#include <vector>
#include <assert.h>

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
	PageId const& page_id, int const page_num,
	QString const& out_dir, bool const batch, bool const debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
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
	
	Params const params(m_ptrSettings->getPageParams(m_pageId));
	
	ImageTransformation xform(data.xform());
	xform.preScaleToDpi(params.dpi());
	
	QRect const new_image_rect(
		xform.transform().map(page_rect_phys).boundingRect().toRect()
	);
	
	QTransform const phys_to_new(
		xform.transform() * QTransform().translate(
			-new_image_rect.left(), -new_image_rect.top()
		)
	);
	
	QColor const white(200, 200, 200); // FIXME: this may hurt binarizeWolf()
	QImage const gray_img(
		transformToGray(
			data.image(), xform.transform(),
			new_image_rect, white
		)
	);
	
	if (DebugImages* dbg = m_ptrDbg.get()) {
		dbg->add(gray_img, "gray_img");
	}
	
	status.throwIfCancelled();
	
	BinaryImage bin_img;
	switch (params.thresholdMode()) {
		case Params::OTSU:
			bin_img = binarizeOtsu(gray_img);
			break;
		case Params::SAUVOLA:
			bin_img = binarizeSauvola(
				gray_img, calcLocalWindowSize(params.dpi())
			);
			break;
		case Params::WOLF:
			bin_img = binarizeWolf(
				gray_img, calcLocalWindowSize(params.dpi())
			);
			break;
	}
	assert(!bin_img.isNull());
	
	BinaryImage seed(openBrick(bin_img, QSize(3, 3)));
	bin_img = seedFill(seed, bin_img, CONN4);
	seed.release();
	
#if 0
	PerformanceTimer kfill_timer;
	bin_img = kFill(bin_img, 3);
	kfill_timer.print("kfill: ");
#endif
	
#if 1
	PerformanceTimer hmt_timer;
	
	// When removing black noice, remove small ones first.
	
	{
		char const pattern[] =
			"XXX"
			" - "
			"   ";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 3);
	}
	
	{
		char const pattern[] =
			"X ?"
			"X  "
			"X- "
			"X- "
			"X  "
			"X ?";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 6);
	}
	
	{
		char const pattern[] =
			"X ?"
			"X ?"
			"X  "
			"X- "
			"X- "
			"X- "
			"X  "
			"X ?"
			"X ?";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 9);
	}
	
	{
		char const pattern[] =
			"XX?"
			"XX?"
			"XX "
			"X+ "
			"X+ "
			"X+ "
			"XX "
			"XX?"
			"XX?";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 9);
	}
	
	{
		char const pattern[] =
			"XX?"
			"XX "
			"X+ "
			"X+ "
			"XX "
			"XX?";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 6);
	}
	
	{
		char const pattern[] =
			"   "
			"X+X"
			"XXX";
		hitMissReplaceAllDirections(bin_img, pattern, 3, 3);
	}
	
	hmt_timer.print("hit-miss operations: ");
#endif
	
#if 0
	PolygonRasterizer::fillExcept(
		bin_img, WHITE, phys_to_new.map(content_rect_phys),
		Qt::WindingFill
	);
#else
	bin_img.fillExcept(
		phys_to_new.map(content_rect_phys).boundingRect().toRect(),
		WHITE
	);
#endif
	
	QString const orig_fname(
		QFileInfo(m_pageId.imageId().filePath()).fileName()
	);
	QString const padded_number(
		QString::fromAscii("%1").arg(
			QString::number(m_pageNum + 1), 4, QChar('0')
		)
	);
	
	// TODO: always output to PNG
	QString const out_path(
		QString::fromAscii("%1/%2_%3").arg(
			m_outDir, padded_number, orig_fname
		)
	);
	
	QImage q_img(bin_img.toQImage());
	Dpm const output_dpm(params.dpi());
	q_img.setDotsPerMeterX(output_dpm.horizontal());
	q_img.setDotsPerMeterY(output_dpm.vertical());
	q_img.save(out_path);
	
	return FilterResultPtr(
		new UiUpdater(
			m_ptrFilter, m_ptrDbg, m_pageId,
			q_img, m_batchProcessing
		)
	);
}

void
Task::hitMissReplaceAllDirections(
	imageproc::BinaryImage& img, char const* const pattern,
	int const pattern_width, int const pattern_height)
{
	hitMissReplaceInPlace(img, WHITE, pattern, pattern_width, pattern_height);
	
	std::vector<char> pattern_data(pattern_width * pattern_height, ' ');
	char* const new_pattern = &pattern_data[0];
	
	// Rotate 90 degrees clockwise.
	char const* p = pattern;
	int new_width = pattern_height;
	int new_height = pattern_width;
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x, ++p) {
			int const new_x = pattern_height - 1 - y;
			int const new_y = x;
			new_pattern[new_y * new_width + new_x] = *p;
		}
	}
	hitMissReplaceInPlace(img, WHITE, new_pattern, new_width, new_height);
	
	// Rotate upside down.
	p = pattern;
	new_width = pattern_width;
	new_height = pattern_height;
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x, ++p) {
			int const new_x = pattern_width - 1 - x;
			int const new_y = pattern_height - 1 - y;
			new_pattern[new_y * new_width + new_x] = *p;
		}
	}
	hitMissReplaceInPlace(img, WHITE, new_pattern, new_width, new_height);
	
	// Rotate 90 degrees counter-clockwise.
	p = pattern;
	new_width = pattern_height;
	new_height = pattern_width;
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x, ++p) {
			int const new_x = y;
			int const new_y = pattern_width - 1 - x;
			new_pattern[new_y * new_width + new_x] = *p;
		}
	}
	hitMissReplaceInPlace(img, WHITE, new_pattern, new_width, new_height);
}

QSize
Task::calcLocalWindowSize(Dpi const& dpi)
{
	QSizeF const size_mm(15, 15);
	QSizeF const size_inch(size_mm * constants::MM2INCH);
	QSizeF const size_pixels_f(
		dpi.horizontal() * size_inch.width(),
		dpi.vertical() * size_inch.height()
	);
	QSize size_pixels(size_pixels_f.toSize());
	
	if (size_pixels.width() < 3) {
		size_pixels.setWidth(3);
	}
	if (size_pixels.height() < 3) {
		size_pixels.setHeight(3);
	}
	
	return size_pixels;
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
	
	// TODO: maybe invalidate thumbnail.
	
	if (m_batchProcessing) {
		return;
	}
	
	ImageView* view = new ImageView(m_image);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
	
	QObject::connect(
		opt_widget, SIGNAL(tonesChanged(QColor const&, QColor const&)),
		view, SLOT(tonesChanged(QColor const&, QColor const&))
	);
}

} // namespace output
