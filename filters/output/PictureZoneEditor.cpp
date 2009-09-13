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
#include "PictureZoneList.h"
#include "ZonePropertiesDialog.h"
#include "Settings.h"
#include "NonCopyable.h"
#include "ImageTransformation.h"
#include "Zone.h"
#include "ScopedIncDec.h"
#include "PixmapRenderer.h"
#include "BackgroundExecutor.h"
#include "AbstractCommand.h"
#include "imageproc/Transform.h"
#include "imageproc/Constants.h"
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QPixmap>
#include <QBitmap>
#include <QPointer>
#include <QPainter>
#include <QPainterPath>
#include <QCursor>
#include <QPen>
#include <QBrush>
#include <QGradient>
#include <QColor>
#include <QMenu>
#include <QSignalMapper>
#include <QMessageBox>
#include <QDebug>
#include <Qt>
#include <boost/foreach.hpp>
#include <algorithm>
#include <limits>
#include <math.h>
#include <assert.h>

// Proximity thresholds are in screen coordinates.
static double const PROXIMITY_THRESHOLD = 10.0;
static double const SQUARED_PROXIMITY_THRESHOLD = PROXIMITY_THRESHOLD * PROXIMITY_THRESHOLD;

static QRgb const red = 0xcc1420;
static QRgb const yellow = 0xfffe00;
static QRgb const orange = 0xffa90e;
static QRgb const mask_color = 0xff587ff4;

namespace output
{

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
	m_origPictureMask(picture_mask),
	m_pictureMaskAnimationPhase(270),
	m_pageId(page_id),
	m_ptrSettings(settings)
{
	setMouseTracking(true);

	m_handlerList.push_back(*new DefaultState(*this));
	m_handlerList.push_back(*new DragHandler(*this));

	connect(&m_pictureMaskAnimateTimer, SIGNAL(timeout()), SLOT(advancePictureMaskAnimation()));
	m_pictureMaskAnimateTimer.setSingleShot(true);
	m_pictureMaskAnimateTimer.setInterval(120);

	connect(&m_pictureMaskRebuildTimer, SIGNAL(timeout()), SLOT(initiateBuildingScreenPictureMask()));
	m_pictureMaskRebuildTimer.setSingleShot(true);
	m_pictureMaskRebuildTimer.setInterval(150);

	BOOST_FOREACH(PictureZone const& zone, m_ptrSettings->zonesForPage(page_id)) {
		PictureSpline::Ptr spline(new PictureSpline(zone.type()));
		BOOST_FOREACH(QPointF const& pt, zone.shape()) {
			spline->appendVertex(pt);
		}
		if (zone.shape().isClosed()) {
			spline->lastVertex()->remove();
		}
		spline->setBridged(true);
		m_splines.push_back(spline);
	}
}

PictureZoneEditor::~PictureZoneEditor()
{
	while (!m_handlerList.empty()) {
		delete &m_handlerList.front();
	}
}

void
PictureZoneEditor::paintOverImage(QPainter& painter)
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

	BOOST_FOREACH(State& state, m_handlerList) {
		state.paint(painter);
	}
}

void
PictureZoneEditor::transformChanged()
{
	BOOST_FOREACH(State& state, m_handlerList) {
		state.transformChanged();
	}
}

void
PictureZoneEditor::keyPressEvent(QKeyEvent* const event)
{
	event->setAccepted(false);

	HandlerList::iterator it(m_handlerList.begin());
	while (it != m_handlerList.end()) {
		(it++)->keyPressEvent(event);
		if (event->isAccepted()) {
			break;
		}
	}
}

void
PictureZoneEditor::keyReleaseEvent(QKeyEvent* const event)
{
	event->setAccepted(false);

	HandlerList::iterator it(m_handlerList.begin());
	while (it != m_handlerList.end()) {
		(it++)->keyReleaseEvent(event);
		if (event->isAccepted()) {
			break;
		}
	}
}

void
PictureZoneEditor::wheelEvent(QWheelEvent* const event)
{
	handleZooming(event);
}

void
PictureZoneEditor::mousePressEvent(QMouseEvent* const event)
{
	event->setAccepted(false);

	HandlerList::iterator it(m_handlerList.begin());
	while (it != m_handlerList.end()) {
		(it++)->mousePressEvent(event);
		if (event->isAccepted()) {
			break;
		}
	}
}

void
PictureZoneEditor::mouseReleaseEvent(QMouseEvent* const event)
{
	event->setAccepted(false);

	HandlerList::iterator it(m_handlerList.begin());
	while (it != m_handlerList.end()) {
		(it++)->mouseReleaseEvent(event);
		if (event->isAccepted()) {
			break;
		}
	}
}

void
PictureZoneEditor::mouseMoveEvent(QMouseEvent* const event)
{
	event->setAccepted(false);

	HandlerList::iterator it(m_handlerList.begin());
	while (it != m_handlerList.end()) {
		(it++)->mouseMoveEvent(event);
		if (event->isAccepted()) {
			break;
		}
	}
}

void
PictureZoneEditor::contextMenuEvent(QContextMenuEvent* const event)
{
	event->setAccepted(false);

	HandlerList::iterator it(m_handlerList.begin());
	while (it != m_handlerList.end()) {
		(it++)->contextMenuEvent(event);
		if (event->isAccepted()) {
			break;
		}
	}
}

void
PictureZoneEditor::advancePictureMaskAnimation()
{
	m_pictureMaskAnimationPhase = (m_pictureMaskAnimationPhase + 40)	% 360;
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

	// First pass: ERASER1
	BOOST_FOREACH (PictureSpline::Ptr const& spline, m_splines) {
		if (spline->type() == PictureZone::ERASER1) {
			painter.drawPolygon(spline->toPolygon(), Qt::WindingFill);
		}
	}

	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	// Second pass: PAINTER2
	BOOST_FOREACH (PictureSpline::Ptr const& spline, m_splines) {
		if (spline->type() == PictureZone::PAINTER2) {
			painter.drawPolygon(spline->toPolygon(), Qt::WindingFill);
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
	BOOST_FOREACH (PictureSpline::Ptr const& spline, m_splines) {
		if (spline->type() == PictureZone::ERASER3) {
			painter.drawPolygon(spline->toPolygon(), Qt::WindingFill);
		}
	}
}

void
PictureZoneEditor::addSpline(PictureSpline::Ptr const& spline)
{
	m_splines.push_back(spline);
	commitZones();
}

void
PictureZoneEditor::commitZones()
{
	PictureZoneList zones;
	zones.reserve(m_splines.size());
	
	BOOST_FOREACH(PictureSpline::Ptr const& spline, m_splines) {
		zones.push_back(PictureZone(spline->toPolygon(), spline->type()));
	}
	
	m_ptrSettings->setZones(m_pageId, zones);
}

QTransform
PictureZoneEditor::toScreen() const
{
	return imageToVirtual() * virtualToWidget();
}

QTransform
PictureZoneEditor::fromScreen() const
{
	return widgetToVirtual() * virtualToImage();
}

double
PictureZoneEditor::sqdist(QPointF const& p1, QPointF const& p2)
{
	double const dx = p1.x() - p2.x();
	double const dy = p1.y() - p2.y();
	return dx * dx + dy * dy;
}

double
PictureZoneEditor::sqdistToLineSegment(QPointF const& pt, QLineF const& line, QPointF* point_on_segment)
{
	QLineF perpendicular(line.normalVector());
	
	// Make it pass through pt.
	perpendicular.translate(-perpendicular.p1());
	perpendicular.translate(pt);
	
	// Calculate intersection.
	QPointF intersection;
	line.intersect(perpendicular, &intersection);
	
	QRectF bounds(QRectF(line.p1(), line.p2()).normalized());
	bounds.adjust(-0.0001, -0.0001, 0.0001, 0.0001);
	// Extending the bounds helps for (nearly) horizontal or vertical lines.
	
	if (bounds.contains(intersection)) {
		if (point_on_segment) {
			*point_on_segment = intersection;
		}
		return sqdist(intersection, pt);
	}
	
	double sqd[2];
	QPointF pts[2];
	
	sqd[0] = sqdist(line.p1(), pt);
	sqd[1] = sqdist(line.p2(), pt);
	pts[0] = line.p1();
	pts[1] = line.p2();
	
	double const* p_min_sqd = std::min_element(sqd, sqd + 2);
	if (point_on_segment) {
		*point_on_segment = pts[p_min_sqd - sqd];
	}
	return *p_min_sqd;
}

void
PictureZoneEditor::visualizeVertex(QPainter& painter, QPointF const& pt, QColor const& color)
{
	painter.setPen(Qt::NoPen);
	painter.setBrush(color);
	
	QRectF rect(0, 0, 4, 4);
	rect.moveCenter(pt);
	painter.drawEllipse(rect);
}

QPointF
PictureZoneEditor::screenPos(QMouseEvent* event)
{
	return QPointF(0.5, 0.5) + event->pos();
}


/*=================================== Vertex =================================*/

PictureZoneEditor::Vertex::Vertex(Vertex* prev, Vertex* next)
:	m_pPrev(prev),
	m_ptrNext(next)
{
}

void
PictureZoneEditor::Vertex::remove()
{
	m_pPrev->m_ptrNext = m_ptrNext;
	m_ptrNext->m_pPrev = m_pPrev;
	m_pPrev = 0;
	m_ptrNext.reset();
}

bool
PictureZoneEditor::Vertex::hasAtLeastSiblings(int const num)
{
	int todo = num;
	for (Vertex::Ptr node(this); (node = node->next(LOOP)).get() != this; ) {
		if (--todo == 0) {
			return true;
		}
	}
	return false;
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::Vertex::prev(Loop const loop)
{
	return m_pPrev->thisOrPrevReal(loop);
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::Vertex::next(Loop const loop)
{
	return m_ptrNext->thisOrNextReal(loop);
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::Vertex::insertBefore(QPointF const& pt)
{
	Vertex::Ptr new_vertex(new RealVertex(pt, m_pPrev, this));
	m_pPrev->m_ptrNext = new_vertex;
	m_pPrev = new_vertex.get();
	return new_vertex;
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::Vertex::insertAfter(QPointF const& pt)
{
	Vertex::Ptr new_vertex(new RealVertex(pt, this, m_ptrNext.get()));
	m_ptrNext->m_pPrev = new_vertex;
	m_ptrNext = new_vertex;
	return new_vertex;
}


/*=============================== SentinelVertex =============================*/

PictureZoneEditor::SentinelVertex::SentinelVertex()
:	Vertex(this, this),
	m_bridged(false)
{
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::SentinelVertex::thisOrPrevReal(Loop const loop)
{
	if (loop == LOOP || (loop == LOOP_IF_BRIDGED && m_bridged)) {
		return Vertex::Ptr(m_pPrev);
	} else {
		return Vertex::Ptr();
	}
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::SentinelVertex::thisOrNextReal(Loop const loop)
{
	if (loop == LOOP || (loop == LOOP_IF_BRIDGED && m_bridged)) {
		return m_ptrNext;
	} else {
		return Vertex::Ptr();
	}
}

QPointF const
PictureZoneEditor::SentinelVertex::point() const
{
	assert(0);
	return QPointF();
}

void
PictureZoneEditor::SentinelVertex::setPoint(QPointF const& pt)
{
	assert(0);
}

void
PictureZoneEditor::SentinelVertex::remove()
{
	assert(0);
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::SentinelVertex::firstVertex() const
{
	if (m_ptrNext.get() == this) {
		return Vertex::Ptr();
	} else {
		return m_ptrNext;
	}
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::SentinelVertex::lastVertex() const
{
	if (m_pPrev == this) {
		return Vertex::Ptr();
	} else {
		return Vertex::Ptr(m_pPrev);
	}
}


/*================================= RealVertex ===============================*/

PictureZoneEditor::RealVertex::RealVertex(QPointF const& pt, Vertex* prev, Vertex* next)
:	Vertex(prev, next),
	m_point(pt),
	m_refCounter(0)
{
}

void
PictureZoneEditor::RealVertex::ref() const
{
	++m_refCounter;
}

void
PictureZoneEditor::RealVertex::unref() const
{
	if (--m_refCounter == 0) {
		delete this;
	}
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::RealVertex::thisOrPrevReal(Loop)
{
	return Vertex::Ptr(this);
}

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::RealVertex::thisOrNextReal(Loop loop)
{
	return Vertex::Ptr(this);
}

QPointF const
PictureZoneEditor::RealVertex::point() const
{
	return m_point;
}

void
PictureZoneEditor::RealVertex::setPoint(QPointF const& pt)
{
	m_point = pt;
}


/*=================================== Edge ===================================*/

PictureZoneEditor::Vertex::Ptr
PictureZoneEditor::Edge::splitAt(QPointF const& pt)
{
	assert(isValid());
	return prev->insertAfter(pt);
}


/*================================= Spline ===================================*/

PictureZoneEditor::Spline::Spline()
{
}

void
PictureZoneEditor::Spline::appendVertex(QPointF const& pt)
{
	m_sentinel.insertBefore(pt);
}

bool
PictureZoneEditor::Spline::hasAtLeastEdges(int num) const
{
	for (EdgeIterator it((Spline&)*this); num > 0 && it.hasNext(); it.next()) {
		--num;
	}
	
	return num == 0;
}

QPolygonF
PictureZoneEditor::Spline::toPolygon() const
{
	QPolygonF poly;
	
	Vertex::Ptr vertex(firstVertex());
	for (; vertex; vertex = vertex->next(Vertex::NO_LOOP)) {
		poly.push_back(vertex->point());
	}
	
	vertex = lastVertex()->next(Vertex::LOOP_IF_BRIDGED);
	if (vertex) {
		poly.push_back(vertex->point());
	}
	
	return poly;
}


/*======================== Spline::EdgeIterator =======================*/

bool
PictureZoneEditor::Spline::EdgeIterator::hasNext() const
{
	return m_ptrNextVertex && m_ptrNextVertex->next(Vertex::LOOP_IF_BRIDGED);
}

PictureZoneEditor::Edge
PictureZoneEditor::Spline::EdgeIterator::next()
{
	assert(hasNext());
	
	Vertex::Ptr origin(m_ptrNextVertex);
	m_ptrNextVertex = m_ptrNextVertex->next(Vertex::NO_LOOP);
	if (!m_ptrNextVertex) {
		return Edge(origin, origin->next(Vertex::LOOP_IF_BRIDGED));
	} else {
		return Edge(origin, m_ptrNextVertex);
	}
}


/*======================= PictureZoneEditor::BasicVisualizer ======================*/

PictureZoneEditor::BasicVisualizer::BasicVisualizer()
:	m_pen(red)
{
	m_pen.setCosmetic(true);
	m_pen.setWidthF(1.5);
}

void
PictureZoneEditor::BasicVisualizer::drawSplines(
	QPainter& painter, QTransform const& to_screen,
	std::vector<PictureSpline::Ptr> const& splines)
{
	BOOST_FOREACH(PictureSpline::Ptr const& spline, splines) {
		drawSpline(painter, to_screen, spline);
	}
}

void
PictureZoneEditor::BasicVisualizer::drawSpline(
	QPainter& painter, QTransform const& to_screen, PictureSpline::Ptr const& spline)
{
	prepareForSpline(painter, spline);
	painter.drawPolygon(to_screen.map(spline->toPolygon()), Qt::WindingFill);
}

void
PictureZoneEditor::BasicVisualizer::prepareForSpline(
	QPainter& painter, PictureSpline::Ptr const&)
{
	painter.setPen(m_pen);
	painter.setBrush(Qt::NoBrush);
}


/*=================================== State ===============================*/

PictureZoneEditor::HandlerList::iterator
PictureZoneEditor::State::handlerPushFront(State* handler)
{
	handler->unlink();
	HandlerList::iterator const it(
		m_rOwner.m_handlerList.insert(m_rOwner.m_handlerList.begin(), *handler)
	);
	it->activated();
	return it;
}

PictureZoneEditor::HandlerList::iterator
PictureZoneEditor::State::handlerPushBack(State* handler)
{
	handler->unlink();
	HandlerList::iterator const it(
		m_rOwner.m_handlerList.insert(m_rOwner.m_handlerList.end(), *handler)
	);
	it->activated();
	return it;
}

QPointF
PictureZoneEditor::State::screenMousePos() const
{
	return QPointF(0.5, 0.5) + m_rOwner.mapFromGlobal(QCursor::pos());
}

/*================================ DefaultState ===========================*/

PictureZoneEditor::DefaultState::DefaultState(PictureZoneEditor& owner)
:	State(owner),
	m_screenMousePos(screenMousePos())
{
}

void
PictureZoneEditor::DefaultState::activated()
{
	update();
}

void
PictureZoneEditor::DefaultState::paint(QPainter& painter)
{
	QTransform const to_screen(toScreen());
	
	BOOST_FOREACH(PictureSpline::Ptr const& spline, m_rOwner.m_splines) {
		if (spline != m_ptrHighlightedSpline) {
			// Draw the whole spline with one color.
			m_visualizer.drawSpline(painter, to_screen, spline);
			continue;
		}
		
		m_visualizer.prepareForSpline(painter, spline);
		QPolygonF points;
		if (m_ptrHighlightedVertex) {
			Vertex::Ptr vertex(m_ptrHighlightedVertex->next(Vertex::LOOP));
			for (; vertex != m_ptrHighlightedVertex; vertex = vertex->next(Vertex::LOOP)) {
				points.push_back(to_screen.map(vertex->point()));
			}
			
			
		} else {
			assert(m_highlightedEdge.isValid());
			Vertex::Ptr vertex(m_highlightedEdge.prev);
			do {
				vertex = vertex->next(Vertex::LOOP);
				points.push_back(to_screen.map(vertex->point()));
			} while (vertex != m_highlightedEdge.prev);
		}
		
		painter.drawPolyline(points);
	}
	
	if (m_ptrHighlightedVertex) {
		// Draw the two adjacent edges in gradient red-to-orange.
		QLinearGradient gradient; // From inactive to active point.
		gradient.setColorAt(0.0, red);
		gradient.setColorAt(1.0, orange);
		
		QPen pen(painter.pen());
		
		QPointF const prev(to_screen.map(m_ptrHighlightedVertex->prev(Vertex::LOOP)->point()));
		QPointF const pt(to_screen.map(m_ptrHighlightedVertex->point()));
		QPointF const next(to_screen.map(m_ptrHighlightedVertex->next(Vertex::LOOP)->point()));
		
		gradient.setStart(prev);
		gradient.setFinalStop(pt);
		pen.setBrush(gradient);
		painter.setPen(pen);
		painter.drawLine(prev, pt);
		
		gradient.setStart(next);
		pen.setBrush(gradient);
		painter.setPen(pen);
		painter.drawLine(next, pt);
		
		// Visualize the highlighted vertex.
		QPointF const screen_vertex(to_screen.map(m_ptrHighlightedVertex->point()));
		visualizeVertex(painter, screen_vertex, yellow);
	} else if (m_highlightedEdge.isValid()) {
		QLineF const line(to_screen.map(m_highlightedEdge.toLine()));
		
		// Draw the highglighed edge in orange.
		QPen pen(painter.pen());
		pen.setColor(orange);
		painter.setPen(pen);
		painter.drawLine(line);
		
		visualizeVertex(painter, m_screenPointOnEdge, yellow);
	} else {
		visualizeVertex(painter, m_screenMousePos, red);
	}
}

void
PictureZoneEditor::DefaultState::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}
	
	if (m_ptrHighlightedVertex) {
		handlerPushFront(
			new VertexDragHandler(m_rOwner, m_ptrHighlightedSpline, m_ptrHighlightedVertex)
		)->mousePressEvent(event);
		delete this;
	} else if (m_highlightedEdge.isValid()) {
		Vertex::Ptr vertex(m_highlightedEdge.splitAt(fromScreen().map(m_screenPointOnEdge)));
		handlerPushFront(new VertexDragHandler(m_rOwner, m_ptrHighlightedSpline, vertex))->mousePressEvent(event);
		delete this;
	}
}

void
PictureZoneEditor::DefaultState::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) {
		return;
	}
	
	handlerPushFront(new SplineCreationState(m_rOwner, fromScreen().map(screenPos(event))));
	delete this;
}

void
PictureZoneEditor::DefaultState::mouseMoveEvent(QMouseEvent* event)
{
	m_screenMousePos = screenPos(event);
	update();
	m_rOwner.update();
}

void
PictureZoneEditor::DefaultState::contextMenuEvent(QContextMenuEvent* event)
{
	QTransform const to_screen(toScreen());
	QTransform const from_screen(fromScreen());
	
	// Find splines containing this point.
	std::vector<unsigned> splines;
	for (unsigned i = 0; i < m_rOwner.m_splines.size(); ++i) {
		QPainterPath path;
		path.setFillRule(Qt::WindingFill);
		path.addPolygon(m_rOwner.m_splines[i]->toPolygon());
		if (path.contains(from_screen.map(screenMousePos()))) {
			splines.push_back(i);
		}
	}
	
	if (splines.empty()) {
		return;
	}
	
	event->accept();
	
	handlerPushFront(new ContextMenuHandler(m_rOwner, splines, event->globalPos()));
	delete this;
}

void
PictureZoneEditor::DefaultState::update()
{
	QTransform const to_screen(toScreen());
	QPointF const image_mouse_pos(fromScreen().map(screenMousePos()));
	
	m_ptrHighlightedSpline.reset();
	Spline::Ptr highlighted_vertex_spline;
	Spline::Ptr highlighted_edge_spline;
	
	m_ptrHighlightedVertex.reset();
	double highlighted_vertex_sqdist = std::numeric_limits<double>::max();
	
	m_highlightedEdge = Edge();
	double highlighted_edge_sqdist = std::numeric_limits<double>::max();

	bool has_zone_under_mouse = false;

	BOOST_FOREACH(Spline::Ptr const& spline, m_rOwner.m_splines) {
		for (Spline::EdgeIterator it(*spline); it.hasNext(); ) {
			if (!has_zone_under_mouse) {
				QPainterPath path;
				path.setFillRule(Qt::WindingFill);
				path.addPolygon(spline->toPolygon());
				has_zone_under_mouse = path.contains(image_mouse_pos);
			}

			Edge const edge(it.next());
			QLineF const line(to_screen.map(edge.toLine()));
			
			double sqd = sqdist(m_screenMousePos, line.p1());
			if (sqd <= SQUARED_PROXIMITY_THRESHOLD && sqd < highlighted_vertex_sqdist) {
				highlighted_vertex_spline = spline;
				m_ptrHighlightedVertex = edge.prev;
				highlighted_vertex_sqdist = sqd;
			}
			
			sqd = sqdist(m_screenMousePos, line.p2());
			if (sqd <= SQUARED_PROXIMITY_THRESHOLD && sqd < highlighted_vertex_sqdist) {
				highlighted_vertex_spline = spline;
				m_ptrHighlightedVertex = edge.next;
				highlighted_vertex_sqdist = sqd;
			}
			
			if (!m_ptrHighlightedVertex) {
				QPointF point_on_edge;
				sqd = sqdistToLineSegment(m_screenMousePos, line, &point_on_edge);
				if (sqd <= SQUARED_PROXIMITY_THRESHOLD && sqd < highlighted_edge_sqdist) {
					highlighted_edge_spline = spline;
					m_highlightedEdge = edge;
					m_screenPointOnEdge = point_on_edge;
					highlighted_edge_sqdist = sqd;
				}
			}
		}
	}
	
	if (m_ptrHighlightedVertex) {
		// Vertex selection takes preference over edge selection.
		m_highlightedEdge = Edge();
		m_ptrHighlightedSpline = highlighted_vertex_spline;
		ensureStatusTip(tr("Drag the vertex."));
	} else if (highlighted_edge_spline) {
		m_ptrHighlightedSpline = highlighted_edge_spline;
		ensureStatusTip(tr("Click to create a new vertex here."));
	} else if (has_zone_under_mouse) {
		ensureStatusTip(tr("Right click to edit zone properties."));
	} else {
		ensureStatusTip(tr("Click to start creating a new picture zone."));
	}
}


/*================================ DragHandler ===========================*/

PictureZoneEditor::DragHandler::DragHandler(PictureZoneEditor& owner)
:	State(owner)
{
}

void
PictureZoneEditor::DragHandler::mousePressEvent(QMouseEvent* event)
{
	m_rOwner.handleImageDragging(event);
	
	if (event->button() == Qt::LeftButton) {
		event->accept();
	}
}

void
PictureZoneEditor::DragHandler::mouseReleaseEvent(QMouseEvent* event)
{
	m_rOwner.handleImageDragging(event);
	
	if (event->button() == Qt::LeftButton) {
		event->accept();
		handlerPushBack(this);
	}
}

void
PictureZoneEditor::DragHandler::mouseMoveEvent(QMouseEvent* event)
{
	m_rOwner.handleImageDragging(event);
	
	if (event->buttons() & Qt::LeftButton) {
		event->accept();
		handlerPushFront(this);
	}
}


/*============================ SplineCreationState =========================*/

PictureZoneEditor::SplineCreationState::SplineCreationState(
	PictureZoneEditor& owner, QPointF const& first_image_point)
:	State(owner),
	m_ptrSpline(new PictureSpline(PictureZone::PAINTER2)),
	m_nextVertexImagePos(fromScreen().map(screenMousePos()))
{
	m_ptrSpline->appendVertex(first_image_point);
}

void
PictureZoneEditor::SplineCreationState::activated()
{
	updateStatusTip();
	m_rOwner.update();
}

void
PictureZoneEditor::SplineCreationState::paint(QPainter& painter)
{
	QTransform const to_screen(toScreen());
	QTransform const from_screen(fromScreen());
	
	m_visualizer.drawSplines(painter, to_screen, m_rOwner.m_splines);
	
	QPen solid_line_pen(red);
	solid_line_pen.setCosmetic(true);
	solid_line_pen.setWidthF(1.5);
	
	QLinearGradient gradient; // From inactive to active point.
	gradient.setColorAt(0.0, red);
	gradient.setColorAt(1.0, orange);
	
	QPen gradient_pen;
	gradient_pen.setCosmetic(true);
	gradient_pen.setWidthF(1.5);
	
	painter.setPen(solid_line_pen);
	painter.setBrush(Qt::NoBrush);
	
	for (Spline::EdgeIterator it(*m_ptrSpline); it.hasNext(); ) {
		Edge const edge(it.next());
		QLineF const line(to_screen.map(edge.toLine()));
		
		if (edge.prev == m_ptrSpline->firstVertex() &&
				edge.prev->point() == m_nextVertexImagePos) {
			gradient.setStart(line.p2());
			gradient.setFinalStop(line.p1());
			gradient_pen.setBrush(gradient);
			painter.setPen(gradient_pen);
			painter.drawLine(line);
			painter.setPen(solid_line_pen);
		} else {
			painter.drawLine(line);
		}
	}
	
	QLineF const line(to_screen.map(QLineF(m_ptrSpline->lastVertex()->point(), m_nextVertexImagePos)));
	gradient.setStart(line.p1());
	gradient.setFinalStop(line.p2());
	gradient_pen.setBrush(gradient);
	painter.setPen(gradient_pen);
	painter.drawLine(line);
	
	visualizeVertex(painter, to_screen.map(m_nextVertexImagePos), yellow);
}

void
PictureZoneEditor::SplineCreationState::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape) {
		event->accept();
		handlerPushFront(new DefaultState(m_rOwner));
		delete this;
	}
}

void
PictureZoneEditor::SplineCreationState::mouseReleaseEvent(QMouseEvent* event)
{
	QTransform const to_screen(toScreen());
	QTransform const from_screen(fromScreen());
	QPointF const screen_mouse_pos(screenPos(event));
	QPointF const image_mouse_pos(from_screen.map(screen_mouse_pos));
	
	if (m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
		m_ptrSpline->setBridged(true);
		m_rOwner.addSpline(m_ptrSpline);
		
		handlerPushFront(new DefaultState(m_rOwner));
		delete this;
		m_rOwner.update();
	} else if (m_nextVertexImagePos == m_ptrSpline->lastVertex()->point()) {
		m_ptrSpline->lastVertex()->remove();
		if (!m_ptrSpline->firstVertex()) {
			handlerPushFront(new DefaultState(m_rOwner));
			delete this;
			m_rOwner.update();
		}
	} else {
		double sqd = sqdist(screen_mouse_pos, m_ptrSpline->lastVertex()->point());
		if (sqd > SQUARED_PROXIMITY_THRESHOLD) {
			m_ptrSpline->appendVertex(image_mouse_pos);
			updateStatusTip();
		}
	}
}

void
PictureZoneEditor::SplineCreationState::mouseMoveEvent(QMouseEvent* event)
{
	QPointF const screen_mouse_pos(screenPos(event));
	QTransform const to_screen(toScreen());
	QTransform const from_screen(fromScreen());
	
	m_nextVertexImagePos = from_screen.map(screen_mouse_pos);
	
	QPointF const last(to_screen.map(m_ptrSpline->lastVertex()->point()));
	if (sqdist(last, screen_mouse_pos) <= SQUARED_PROXIMITY_THRESHOLD) {
		m_nextVertexImagePos = m_ptrSpline->lastVertex()->point();
	} else if (m_ptrSpline->hasAtLeastEdges(2)) {
		QPointF const first(to_screen.map(m_ptrSpline->firstVertex()->point()));
		if (sqdist(first, screen_mouse_pos) <= SQUARED_PROXIMITY_THRESHOLD) {
			m_nextVertexImagePos = m_ptrSpline->firstVertex()->point();
			updateStatusTip();
		}
	}
	
	m_rOwner.update();
}

void
PictureZoneEditor::SplineCreationState::updateStatusTip()
{
	if (m_ptrSpline->hasAtLeastEdges(2)) {
		ensureStatusTip(tr("Finish the zone by connecting its start and end points."));
	} else {
		ensureStatusTip(tr("At least 3 edges are required. ESC to discard this zone."));
	}
}


/*============================= VertexDragHandler ===========================*/

PictureZoneEditor::VertexDragHandler::VertexDragHandler(
	PictureZoneEditor& owner, Spline::Ptr const& spline, Vertex::Ptr const& vertex)
:	State(owner),
	m_ptrSpline(spline),
	m_ptrVertex(vertex),
	m_dragOffset(toScreen().map(vertex->point()) - screenMousePos())
{
	update();
}

void
PictureZoneEditor::VertexDragHandler::activated()
{
	update();
	m_rOwner.update();
}

void
PictureZoneEditor::VertexDragHandler::paint(QPainter& painter)
{
	QTransform const to_screen(toScreen());
	
	BOOST_FOREACH(PictureSpline::Ptr const& spline, m_rOwner.m_splines) {
		if (spline != m_ptrSpline) {
			// Draw the whole spline in solid color.
			m_basicVisualizer.drawSpline(painter, to_screen, spline);
			continue;
		}
		
		// Draw the solid part of the spline.
		QPolygonF points;
		Vertex::Ptr vertex(m_ptrVertex->next(Vertex::LOOP));
		for (; vertex != m_ptrVertex; vertex = vertex->next(Vertex::LOOP)) {
			points.push_back(to_screen.map(vertex->point()));
		}
		
		m_basicVisualizer.prepareForSpline(painter, spline);
		painter.drawPolyline(points);
	}
	
	QLinearGradient gradient; // From remote to selected point.
	gradient.setColorAt(0.0, red);
	gradient.setColorAt(1.0, orange);
	
	QPen gradient_pen;
	gradient_pen.setCosmetic(true);
	gradient_pen.setWidthF(1.5);
	
	painter.setBrush(Qt::NoBrush);
	
	QPointF const pt(to_screen.map(m_ptrVertex->point()));
	QPointF const prev(to_screen.map(m_ptrVertex->prev(Vertex::LOOP)->point()));
	QPointF const next(to_screen.map(m_ptrVertex->next(Vertex::LOOP)->point()));
	
	gradient.setStart(prev);
	gradient.setFinalStop(pt);
	gradient_pen.setBrush(gradient);
	painter.setPen(gradient_pen);
	painter.drawLine(prev, pt);
	
	gradient.setStart(next);
	gradient_pen.setBrush(gradient);
	painter.setPen(gradient_pen);
	painter.drawLine(next, pt);
	
	visualizeVertex(painter, to_screen.map(m_ptrVertex->point()), yellow);
}

void
PictureZoneEditor::VertexDragHandler::mousePressEvent(QMouseEvent* event)
{
	event->accept(); // Prevent it reaching the DragHandler.
}

void
PictureZoneEditor::VertexDragHandler::mouseReleaseEvent(QMouseEvent* event)
{
	event->accept(); // Prevent it reaching the DragHandler.
	
	if (event->button() == Qt::LeftButton) {
		if (m_ptrVertex->point() == m_ptrVertex->next(Vertex::LOOP)->point() ||
				m_ptrVertex->point() == m_ptrVertex->prev(Vertex::LOOP)->point()) {
			if (m_ptrVertex->hasAtLeastSiblings(3)) {
				m_ptrVertex->remove();
			}
		}

		m_rOwner.commitZones();

		handlerPushFront(new DefaultState(m_rOwner));
		delete this;
	}
}

void
PictureZoneEditor::VertexDragHandler::mouseMoveEvent(QMouseEvent* event)
{
	event->accept(); // Prevent it reaching the DragHandler.
	m_ptrVertex->setPoint(fromScreen().map(screenPos(event) + m_dragOffset));
	
	update();
	m_rOwner.update();
}

void
PictureZoneEditor::VertexDragHandler::update()
{
	bool can_merge = false;

	if (m_ptrVertex->hasAtLeastSiblings(3)) {
		QTransform const to_screen(toScreen());
		QPointF const origin(to_screen.map(m_ptrVertex->point()));

		QPointF const prev(m_ptrVertex->prev(Vertex::LOOP)->point());
		double const sqdist_to_prev = sqdist(origin, to_screen.map(prev));

		QPointF const next(m_ptrVertex->next(Vertex::LOOP)->point());
		double const sqdist_to_next= sqdist(origin, to_screen.map(next));


		if (sqdist_to_prev <= SQUARED_PROXIMITY_THRESHOLD && sqdist_to_prev < sqdist_to_next) {
			m_ptrVertex->setPoint(prev);
			can_merge = true;
		} else if (sqdist_to_next <= SQUARED_PROXIMITY_THRESHOLD) {
			m_ptrVertex->setPoint(next);
			can_merge = true;
		}
	}

	if (can_merge) {
		ensureStatusTip(tr("Merge these two vertices."));
	} else {
		ensureStatusTip(tr("Move the vertex to one of its neighbors to merge them."));
	}
}


/*=========================== ContextMenuHandler ===============================*/

class PictureZoneEditor::ContextMenuHandler::OrderByArea
{
public:
	OrderByArea(std::vector<PictureSpline::Ptr> const& splines, QTransform const& to_screen) {
		m_areas.reserve(splines.size());
		BOOST_FOREACH(PictureSpline::Ptr const& spline, splines) {
			QRectF const bbox(to_screen.map(spline->toPolygon()).boundingRect());
			m_areas.push_back(bbox.width() * bbox.height());
		}
	}
	
	bool operator()(unsigned lhs, unsigned rhs) const {
		return m_areas[lhs] > m_areas[rhs];
	}
private:
	std::vector<double> m_areas;
};


PictureZoneEditor::ContextMenuHandler::ContextMenuHandler(
	PictureZoneEditor& owner, std::vector<unsigned> const& spline_indexes,
	QPoint const& mouse_pos)
:	State(owner),
	m_splineIndexes(spline_indexes),
	m_ptrMenu(new QMenu(&m_rOwner)),
	m_highlightedSplineIdx(-1),
	m_dontSwitchToDefaultState(0)
{
	std::vector<unsigned> reordered(m_splineIndexes);
	std::sort(reordered.begin(), reordered.end(), OrderByArea(m_rOwner.m_splines, toScreen()));
	
	std::vector<PictureSpline::Ptr> splines(m_rOwner.m_splines);
	for (unsigned i = 0; i < m_splineIndexes.size(); ++i) {
		m_rOwner.m_splines[reordered[i]] = splines[m_splineIndexes[i]];
	}
	
	int h = 20;
	int const h_step = 65;
	int const s = 255 * 64 / 100;
	int const v = 255 * 96 / 100;
	int const alpha = 150;
	QColor color;
	
	QSignalMapper* hover_map = new QSignalMapper(this);
	QSignalMapper* prop_trigger_map = new QSignalMapper(this);
	QSignalMapper* del_trigger_map = new QSignalMapper(this);
	connect(hover_map, SIGNAL(mapped(int)), SLOT(highlightItem(int)));
	connect(prop_trigger_map, SIGNAL(mapped(int)), SLOT(propertiesRequest(int)));
	connect(del_trigger_map, SIGNAL(mapped(int)), SLOT(deleteRequest(int)));

	QPixmap pixmap;

	std::vector<unsigned>::reverse_iterator it(m_splineIndexes.rbegin());
	std::vector<unsigned>::reverse_iterator const end(m_splineIndexes.rend());
	for (; it != end; ++it, h = (h + h_step) % 360) {
		color.setHsv(h, s, v, alpha);
		color = color.toRgb();
		m_colorMap[*it] = color;
				
		if (m_splineIndexes.size() > 1) {
			pixmap = QPixmap(16, 16);
			color.setAlpha(255);
			pixmap.fill(color);
		}

		QAction* action = m_ptrMenu->addAction(pixmap, tr("Properties"));
		prop_trigger_map->setMapping(action, *it);
		hover_map->setMapping(action, *it);
		connect(action, SIGNAL(triggered()), prop_trigger_map, SLOT(map()));
		connect(action, SIGNAL(hovered()), hover_map, SLOT(map()));

		action = m_ptrMenu->addAction(pixmap, tr("Delete"));
		del_trigger_map->setMapping(action, *it);
		hover_map->setMapping(action, *it);
		connect(action, SIGNAL(triggered()), del_trigger_map, SLOT(map()));
		connect(action, SIGNAL(hovered()), hover_map, SLOT(map()));

		m_ptrMenu->addSeparator();
	}
	
	connect(m_ptrMenu.get(), SIGNAL(aboutToHide()), SLOT(menuAboutToHide()), Qt::QueuedConnection);
	
	highlightItem(m_splineIndexes.back());
	m_ptrMenu->popup(mouse_pos);
}

void
PictureZoneEditor::ContextMenuHandler::activated()
{
	m_rOwner.update();
	ensureStatusTip(QString());
}

void
PictureZoneEditor::ContextMenuHandler::paint(QPainter& painter)
{
	if (m_highlightedSplineIdx >= 0) {
		m_visualizer.drawSpline(painter, toScreen(), m_rOwner.m_splines[m_highlightedSplineIdx]);
	}
}

void
PictureZoneEditor::ContextMenuHandler::menuAboutToHide()
{
	switchToDefaultState();
}

void
PictureZoneEditor::ContextMenuHandler::setType(int spline_idx, PictureZone::Type type)
{
	m_rOwner.m_splines[spline_idx]->setType(type);
}

void
PictureZoneEditor::ContextMenuHandler::propertiesRequest(int spline_idx)
{
	{
		ScopedIncDec<int> guard(m_dontSwitchToDefaultState);
		
		m_visualizer.switchToStrokeMode();

		PictureZone::Type const old_type = m_rOwner.m_splines[spline_idx]->type();
		ZonePropertiesDialog dialog(spline_idx, old_type, &m_rOwner);
		connect(
			&dialog, SIGNAL(typeChanged(int, PictureZone::Type)),
			SLOT(setType(int, PictureZone::Type))
		);
		if (dialog.exec() == QDialog::Accepted) {
			m_rOwner.commitZones();
		} else {
			m_rOwner.m_splines[spline_idx]->setType(old_type);
		}
	}
	
	switchToDefaultState();
}

void
PictureZoneEditor::ContextMenuHandler::deleteRequest(int spline_idx)
{
	{
		ScopedIncDec<int> guard(m_dontSwitchToDefaultState);

		QMessageBox::StandardButton const btn = QMessageBox::question(
			&m_rOwner, tr("Delete confirmation"), tr("Really delete this zone?"),
			QMessageBox::Yes|QMessageBox::No
		);
		if (btn == QMessageBox::Yes) {
			m_rOwner.m_splines.erase(m_rOwner.m_splines.begin() + spline_idx);
			m_rOwner.commitZones();
		}
	}

	switchToDefaultState();
}

void
PictureZoneEditor::ContextMenuHandler::highlightItem(int spline_idx)
{
	if (m_splineIndexes.size() > 1) {
		m_visualizer.switchToFillMode(m_colorMap[spline_idx]);
	} else {
		m_visualizer.switchToStrokeMode();
	}
	m_highlightedSplineIdx = spline_idx;
	m_rOwner.update();
}

void
PictureZoneEditor::ContextMenuHandler::switchToDefaultState()
{
	if (m_dontSwitchToDefaultState == 0 && is_linked()) {
		handlerPushFront(new DefaultState(m_rOwner));
		unlink();
		deleteLater();
		m_rOwner.update();
	}
}


/*====================== ContextMenuHandler::Visualizer =========================*/

void
PictureZoneEditor::ContextMenuHandler::Visualizer::switchToFillMode(QColor const& color)
{
	m_color = color;
}

void
PictureZoneEditor::ContextMenuHandler::Visualizer::switchToStrokeMode()
{
	m_color = QColor();
}

void
PictureZoneEditor::ContextMenuHandler::Visualizer::prepareForSpline(
	QPainter& painter, PictureSpline::Ptr const& spline)
{
	BasicVisualizer::prepareForSpline(painter, spline);
	if (m_color.isValid()) {
		painter.setBrush(m_color);
	}
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
