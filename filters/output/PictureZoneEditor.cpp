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

#include "PictureZoneEditor.h.moc"
#include "NonCopyable.h"
#include "Zone.h"
#include "ZoneSet.h"
#include "SerializableSpline.h"
#include "PropertySet.h"
#include "PictureLayerProperty.h"
#include "ZonePropertiesDialog.h"
#include "Settings.h"
#include "ImageTransformation.h"
#include "PixmapRenderer.h"
#include "BackgroundExecutor.h"
#include "AbstractCommand.h"
#include "imageproc/Transform.h"
#include "imageproc/Constants.h"
#include <QPointer>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <Qt>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <assert.h>

namespace output
{

static QRgb const mask_color = 0xff587ff4;

using namespace imageproc;

class PictureZoneEditor::MaskTransformTask :
	public AbstractCommand0<IntrusivePtr<AbstractCommand0<void> > >,
	public QObject
{
	DECLARE_NON_COPYABLE(MaskTransformTask)
public:
	MaskTransformTask(
		PictureZoneEditor* zone_editor,
		BinaryImage const& orig_mask, QTransform const& xform,
		QSize const& target_size);

	void cancel() { m_ptrResult->cancel(); }

	bool const isCancelled() const { return m_ptrResult->isCancelled(); }

	virtual IntrusivePtr<AbstractCommand0<void> > operator()();
private:
	class Result : public AbstractCommand0<void>
	{
	public:
		Result(PictureZoneEditor* zone_editor);

		void setData(QPoint const& origin, QImage const& mask);

		void cancel() { m_cancelFlag.fetchAndStoreRelaxed(1); }

		bool isCancelled() const { return m_cancelFlag.fetchAndAddRelaxed(0) != 0; }

		virtual void operator()();
	private:
		QPointer<PictureZoneEditor> m_ptrZoneEditor;
		QPoint m_origin;
		QImage m_mask;
		mutable QAtomicInt m_cancelFlag;
	};

	IntrusivePtr<Result> m_ptrResult;
	BinaryImage m_origMask;
	QTransform m_xform;
	QSize m_targetSize;
};


PictureZoneEditor::PictureZoneEditor(
	QImage const& image, QImage const& downscaled_image,
	imageproc::BinaryImage const& picture_mask,
	QTransform const& image_to_virt, QPolygonF const& virt_display_area,
	PageId const& page_id, IntrusivePtr<Settings> const& settings)
:	ImageViewBase(image, downscaled_image, image_to_virt, virt_display_area),
	m_context(*this, m_zones),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_origPictureMask(picture_mask),
	m_pictureMaskAnimationPhase(270),
	m_pageId(page_id),
	m_ptrSettings(settings)
{
	setMouseTracking(true);

	m_context.setShowPropertiesCommand(
		boost::bind(&PictureZoneEditor::showPropertiesDialog, this, _1)
	);

	connect(&m_zones, SIGNAL(committed()), SLOT(commitZones()));

	PropertySet props;
	props.locateOrCreate<PictureLayerProperty>()->setLayer(PictureLayerProperty::PAINTER2);
	m_zones.setDefaultProperties(props);

	makeLastFollower(*m_context.createDefaultInteraction());

	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
	rootInteractionHandler().makeLastFollower(*this);

	connect(&m_pictureMaskAnimateTimer, SIGNAL(timeout()), SLOT(advancePictureMaskAnimation()));
	m_pictureMaskAnimateTimer.setSingleShot(true);
	m_pictureMaskAnimateTimer.setInterval(120);

	connect(&m_pictureMaskRebuildTimer, SIGNAL(timeout()), SLOT(initiateBuildingScreenPictureMask()));
	m_pictureMaskRebuildTimer.setSingleShot(true);
	m_pictureMaskRebuildTimer.setInterval(150);

	BOOST_FOREACH(Zone const& zone, m_ptrSettings->zonesForPage(page_id)) {
		EditableSpline::Ptr spline(new EditableSpline(zone.spline()));
		m_zones.addZone(spline, zone.properties());
	}
}

PictureZoneEditor::~PictureZoneEditor()
{
}

void
PictureZoneEditor::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setWorldTransform(QTransform());
	painter.setRenderHint(QPainter::Antialiasing);

	if (!validateScreenPictureMask()) {
		schedulePictureMaskRebuild();
	} else {
		double const sn = sin(constants::DEG2RAD * m_pictureMaskAnimationPhase);
		double const scale = 0.5 * (sn + 1.0); // 0 .. 1
		double const opacity = 0.35 * scale + 0.15;

		QPixmap mask(m_screenPictureMask);

		{
			QPainter mask_painter(&mask);
			mask_painter.translate(-m_screenPictureMaskOrigin);
			paintOverPictureMask(mask_painter);
		}

		painter.setOpacity(opacity);
		painter.drawPixmap(m_screenPictureMaskOrigin, mask);
		painter.setOpacity(1.0);

		if (!m_pictureMaskAnimateTimer.isActive()) {
			m_pictureMaskAnimateTimer.start();
		}
	}
}

void
PictureZoneEditor::advancePictureMaskAnimation()
{
	m_pictureMaskAnimationPhase = (m_pictureMaskAnimationPhase + 40) % 360;
	update();
}

bool
PictureZoneEditor::validateScreenPictureMask() const
{
	return !m_screenPictureMask.isNull() && m_screenPictureMaskXform == virtualToWidget();
}

void
PictureZoneEditor::schedulePictureMaskRebuild()
{
	if (!m_pictureMaskRebuildTimer.isActive() ||
			m_potentialPictureMaskXform != virtualToWidget()) {
		if (m_ptrMaskTransformTask.get()) {
			m_ptrMaskTransformTask->cancel();
			m_ptrMaskTransformTask.reset();
		}
		m_potentialPictureMaskXform = virtualToWidget();
		m_pictureMaskRebuildTimer.start();
	}
}

void
PictureZoneEditor::initiateBuildingScreenPictureMask()
{
	if (validateScreenPictureMask()) {
		return;
	}

	m_screenPictureMask = QPixmap();

	if (m_ptrMaskTransformTask.get()) {
		m_ptrMaskTransformTask->cancel();
		m_ptrMaskTransformTask.reset();
	}

	QTransform const xform(virtualToWidget());
	IntrusivePtr<MaskTransformTask> const task(
		new MaskTransformTask(this, m_origPictureMask, xform, size())
	);

	backgroundExecutor().enqueueTask(task);

	m_screenPictureMask = QPixmap();
	m_ptrMaskTransformTask = task;
	m_screenPictureMaskXform = xform;
}

void
PictureZoneEditor::screenPictureMaskBuilt(QPoint const& origin, QImage const& mask)
{
	m_screenPictureMask = QPixmap::fromImage(mask);
	m_screenPictureMaskOrigin = origin;
	m_pictureMaskAnimationPhase = 270;

	m_ptrMaskTransformTask.reset();
	update();
}

void
PictureZoneEditor::paintOverPictureMask(QPainter& painter)
{
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setTransform(imageToVirtual() * virtualToWidget(), true);
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(mask_color));

#ifndef Q_WS_X11
	// That's how it's supposed to be.
	painter.setCompositionMode(QPainter::CompositionMode_Clear);
#else
	// QPainter::CompositionMode_Clear doesn't work for arbitrarily shaped
	// objects on X11, as well as CompositionMode_Source with a transparent
	// brush.  Fortunately, CompositionMode_DestinationOut with a non-transparent
	// brush does actually work.
	painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
#endif

	typedef PictureLayerProperty PLP;

	// First pass: ERASER1
	BOOST_FOREACH(EditableZoneSet::Zone const& zone, m_zones) {
		if (zone.properties()->locateOrDefault<PLP>()->layer() == PLP::ERASER1) {
			painter.drawPolygon(zone.spline()->toPolygon(), Qt::WindingFill);
		}
	}

	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	// Second pass: PAINTER2
	BOOST_FOREACH (EditableZoneSet::Zone const& zone, m_zones) {
		if (zone.properties()->locateOrDefault<PLP>()->layer() == PLP::PAINTER2) {
			painter.drawPolygon(zone.spline()->toPolygon(), Qt::WindingFill);
		}
	}

#ifndef Q_WS_X11
	// That's how it's supposed to be.
	painter.setCompositionMode(QPainter::CompositionMode_Clear);
#else
	// QPainter::CompositionMode_Clear doesn't work for arbitrarily shaped
	// objects on X11, as well as CompositionMode_Source with a transparent
	// brush.  Fortunately, CompositionMode_DestinationOut with a non-transparent
	// brush does actually work.
	painter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
#endif

	// Third pass: ERASER1
	BOOST_FOREACH (EditableZoneSet::Zone const& zone, m_zones) {
		if (zone.properties()->locateOrDefault<PLP>()->layer() == PLP::ERASER3) {
			painter.drawPolygon(zone.spline()->toPolygon(), Qt::WindingFill);
		}
	}
}

void
PictureZoneEditor::showPropertiesDialog(EditableZoneSet::Zone const& zone)
{
	IntrusivePtr<PropertySet> old_properties(zone.properties()->deepCopy());
	ZonePropertiesDialog dialog(zone.properties(), this);
	connect(&dialog, SIGNAL(updated()), SLOT(update()));
	if (dialog.exec() == QDialog::Accepted) {
		m_zones.commit();
	} else {
		m_zones.setProperties(zone.spline(), old_properties);
	}
}

void
PictureZoneEditor::commitZones()
{
	ZoneSet zones;

	BOOST_FOREACH(EditableZoneSet::Zone const& zone, m_zones) {
		zones.add(Zone(SerializableSpline(zone.spline()->toPolygon()), *zone.properties()));
	}
	
	m_ptrSettings->setZones(m_pageId, zones);
}


/*============================= MaskTransformTask ===============================*/

PictureZoneEditor::MaskTransformTask::MaskTransformTask(
	PictureZoneEditor* zone_editor,
	BinaryImage const& mask, QTransform const& xform,
	QSize const& target_size)
:	m_ptrResult(new Result(zone_editor)),
	m_origMask(mask),
	m_xform(xform),
	m_targetSize(target_size)
{
}

IntrusivePtr<AbstractCommand0<void> >
PictureZoneEditor::MaskTransformTask::operator()()
{
	if (isCancelled()) {
		return IntrusivePtr<AbstractCommand0<void> >();
	}

	QRect const target_rect(
		m_xform.map(
			QRectF(m_origMask.rect())
		).boundingRect().toRect().intersected(
			QRect(QPoint(0, 0), m_targetSize)
		)
	);

	QImage gray_mask(
		transformToGray(
			m_origMask.toQImage(), m_xform, target_rect,
			Qt::black, true, QSizeF(0.0, 0.0)
		)
	);

	QImage mask(gray_mask.size(), QImage::Format_ARGB32_Premultiplied);
	mask.fill(mask_color);
	mask.setAlphaChannel(gray_mask);

	m_ptrResult->setData(target_rect.topLeft(), mask);

	return m_ptrResult;
}


/*===================== MaskTransformTask::Result ===================*/

PictureZoneEditor::MaskTransformTask::Result::Result(
	PictureZoneEditor* zone_editor)
:	m_ptrZoneEditor(zone_editor)
{
}

void
PictureZoneEditor::MaskTransformTask::Result::setData(
	QPoint const& origin, QImage const& mask)
{
	m_mask = mask;
	m_origin = origin;
}

void
PictureZoneEditor::MaskTransformTask::Result::operator()()
{
	if (m_ptrZoneEditor && !isCancelled()) {
		m_ptrZoneEditor->screenPictureMaskBuilt(m_origin, m_mask);
	}
}

} // namespace output
