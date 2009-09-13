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


#ifndef OUTPUT_PICTURE_ZONE_EDITOR_H_
#define OUTPUT_PICTURE_ZONE_EDITOR_H_

#include "ImageViewBase.h"
#include "NonCopyable.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include "PictureZone.h"
#include "imageproc/BinaryImage.h"
#include <QTransform>
#include <QPointF>
#include <QLineF>
#include <QPolygonF>
#include <QColor>
#include <QVector>
#include <QPen>
#include <QBitmap>
#include <boost/intrusive/list.hpp>
#include <memory>
#include <vector>
#include <map>
#include <stddef.h>

class ImageTransformation;
class QPainter;
class QPixmap;
class QMenu;

namespace output
{

class Settings;

class ContextMenuHandlerBase : public QObject
{
	Q_OBJECT
private slots:
	virtual void menuAboutToHide() = 0;
	
	virtual void propertiesRequest(int spline_idx) = 0;

	virtual void deleteRequest(int spline_idx) = 0;
	
	virtual void highlightItem(int spline_idx) = 0;

	virtual void setType(int spline_idx, PictureZone::Type type) = 0;
};


class PictureZoneEditor : public ImageViewBase
{
	Q_OBJECT
public:
	PictureZoneEditor(
		QImage const& image, QImage const& downscaled_image,
		imageproc::BinaryImage const& picture_mask,
		QTransform const& image_to_virt, QPolygonF const& virt_display_area,
		PageId const& page_id, IntrusivePtr<Settings> const& settings);
	
	virtual ~PictureZoneEditor();
protected:
	virtual void paintOverImage(QPainter& painter);
	
	virtual void transformChanged();

	virtual void keyPressEvent(QKeyEvent* event);

	virtual void keyReleaseEvent(QKeyEvent* event);

	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void mousePressEvent(QMouseEvent* event);
	
	virtual void mouseReleaseEvent(QMouseEvent* event);
	
	virtual void mouseMoveEvent(QMouseEvent* event);
	
	virtual void contextMenuEvent(QContextMenuEvent* event);
private slots:
	void advancePictureMaskAnimation();

	void initiateBuildingScreenPictureMask();
private:
	class Edge;
	class Spline;
	class State;
	class MaskTransformTask;
	
	typedef boost::intrusive::list<State, boost::intrusive::constant_time_size<false> > HandlerList;
	
	class Vertex
	{
	public:
		enum Loop { LOOP, NO_LOOP, LOOP_IF_BRIDGED };
		
		typedef IntrusivePtr<Vertex> Ptr;
		
		Vertex(Vertex* prev, Vertex* next);
		
		virtual ~Vertex() {}
		
		virtual void ref() const {}
		
		virtual void unref() const {}
		
		virtual Vertex::Ptr thisOrPrevReal(Loop loop) = 0;
		
		virtual Vertex::Ptr thisOrNextReal(Loop loop) = 0;
		
		virtual QPointF const point() const = 0;
		
		virtual void setPoint(QPointF const& pt) = 0;

		virtual void remove();
		
		bool hasAtLeastSiblings(int num);
		
		Vertex::Ptr prev(Loop loop);
		
		Vertex::Ptr next(Loop loop);
		
		Vertex::Ptr insertBefore(QPointF const& pt);
		
		Vertex::Ptr insertAfter(QPointF const& pt);
	protected:
		Vertex* m_pPrev;
		Vertex::Ptr m_ptrNext;
	};
	
	
	class SentinelVertex : public Vertex
	{
		DECLARE_NON_COPYABLE(SentinelVertex)
	public:
		SentinelVertex();
		
		virtual Vertex::Ptr thisOrPrevReal(Loop loop);
		
		virtual Vertex::Ptr thisOrNextReal(Loop loop);
		
		virtual QPointF const point() const;
		
		virtual void setPoint(QPointF const& pt);
		
		virtual void remove();
		
		Vertex::Ptr firstVertex() const;
		
		Vertex::Ptr lastVertex() const;
		
		bool bridged() const { return m_bridged; }
		
		void setBridged(bool bridged) { m_bridged = bridged; }
	private:
		bool m_bridged;
	};
	
	
	class RealVertex : public Vertex
	{
		DECLARE_NON_COPYABLE(RealVertex)
	public:
		RealVertex(QPointF const& pt, Vertex* prev, Vertex* next);
		
		virtual void ref() const;
		
		virtual void unref() const;
		
		virtual Vertex::Ptr thisOrPrevReal(Loop loop);
		
		virtual Vertex::Ptr thisOrNextReal(Loop loop);
		
		virtual QPointF const point() const;
		
		virtual void setPoint(QPointF const& pt);
	private:
		QPointF m_point;
		mutable int m_refCounter;
	};
	
	class Edge
	{
	public:
		Vertex::Ptr prev;
		Vertex::Ptr next;
		
		Edge() {}
		
		Edge(Vertex::Ptr const& prev, Vertex::Ptr const& next)
		: prev(prev), next(next) {}
		
		Vertex::Ptr splitAt(QPointF const& pt);
		
		bool isValid() const { return prev && next && prev->next(Vertex::LOOP_IF_BRIDGED) == next; }
		
		bool operator==(Edge const& other) const {
			return prev == other.prev && next == other.next;
		}
		
		QLineF toLine() const { return QLineF(prev->point(), next->point()); }
	};
	
	class Spline : public RefCountable
	{
	public:
		typedef IntrusivePtr<Spline> Ptr;
		
		class EdgeIterator
		{
		public:
			EdgeIterator(Spline& spline) : m_ptrNextVertex(spline.firstVertex()) {}
			
			bool hasNext() const;
			
			Edge next();
		private:
			Vertex::Ptr m_ptrNextVertex;
		};
		
		Spline();
		
		void appendVertex(QPointF const& pt);
		
		Vertex::Ptr firstVertex() const { return m_sentinel.firstVertex(); }
		
		Vertex::Ptr lastVertex() const{ return m_sentinel.lastVertex(); }
		
		bool hasAtLeastEdges(int num) const;
		
		void setBridged(bool bridged) { m_sentinel.setBridged(true); }
		
		QPolygonF toPolygon() const;
	private:
		SentinelVertex m_sentinel;
	};
	
	class PictureSpline : public Spline
	{
	public:
		typedef IntrusivePtr<PictureSpline> Ptr;

		PictureSpline(PictureZone::Type type) : m_type(type) {}

		PictureZone::Type type() const { return m_type; }

		void setType(PictureZone::Type type) { m_type = type; }
	private:
		PictureZone::Type m_type;
	};

	class BasicVisualizer
	{
	public:
		BasicVisualizer();
		
		void drawSplines(QPainter& painter, QTransform const& to_screen,
						 std::vector<PictureSpline::Ptr> const& splines);
		
		virtual void drawSpline(QPainter& painter, QTransform const& to_screen,
								PictureSpline::Ptr const& spline);
		
		virtual void prepareForSpline(QPainter& painter,
									  PictureSpline::Ptr const& spline);
	private:
		QPen m_pen;
	};
	
	class State :
		public boost::intrusive::list_base_hook<
			boost::intrusive::link_mode<boost::intrusive::auto_unlink>
		>
	{
	public:
		State(PictureZoneEditor& owner) : m_rOwner(owner) {}
		
		virtual ~State() {}
		
		virtual void activated() {}

		virtual void paint(QPainter& painter) {}
		
		virtual void transformChanged() {}

		virtual void keyPressEvent(QKeyEvent* event) {}

		virtual void keyReleaseEvent(QKeyEvent* event) {}

		virtual void mousePressEvent(QMouseEvent* event) {}
	
		virtual void mouseReleaseEvent(QMouseEvent* event) {}
		
		virtual void mouseMoveEvent(QMouseEvent* event) {}
		
		virtual void contextMenuEvent(QContextMenuEvent* event) {}
	protected:
		HandlerList::iterator handlerPushFront(State* state);
		
		HandlerList::iterator handlerPushBack(State* state);
		
		QTransform toScreen() const { return m_rOwner.toScreen(); }
		
		QTransform fromScreen() const { return m_rOwner.fromScreen(); }
		
		void ensureStatusTip(QString const& tip) { m_rOwner.ensureStatusTip(tip); }

		QPointF screenMousePos() const;
		
		PictureZoneEditor& m_rOwner;
	};
	
	class DefaultState : public State
	{
	public:
		DefaultState(PictureZoneEditor& owner);
		
		virtual void activated();

		virtual void transformChanged() { update(); }

		virtual void paint(QPainter& painter);
		
		virtual void mousePressEvent(QMouseEvent* event);
		
		virtual void mouseReleaseEvent(QMouseEvent* event);
		
		virtual void mouseMoveEvent(QMouseEvent* event);
		
		virtual void contextMenuEvent(QContextMenuEvent* event);
	private:
		void update();

		BasicVisualizer m_visualizer;
		QPointF m_screenMousePos;
		Spline::Ptr m_ptrHighlightedSpline;
		Vertex::Ptr m_ptrHighlightedVertex;
		Edge m_highlightedEdge;
		QPointF m_screenPointOnEdge;
	};
	
	class DragHandler : public State
	{
	public:
		DragHandler(PictureZoneEditor& owner);

		virtual void mousePressEvent(QMouseEvent* event);
		
		virtual void mouseReleaseEvent(QMouseEvent* event);
		
		virtual void mouseMoveEvent(QMouseEvent* event);
	};
	
	class SplineCreationState : public State
	{
	public:
		SplineCreationState(PictureZoneEditor& owner, QPointF const& first_image_point);
		
		virtual void activated();

		virtual void paint(QPainter& painter);
		
		virtual void keyPressEvent(QKeyEvent* event);

		virtual void mouseReleaseEvent(QMouseEvent* event);
		
		virtual void mouseMoveEvent(QMouseEvent* event);
	private:
		void updateStatusTip();

		BasicVisualizer m_visualizer;
		PictureSpline::Ptr m_ptrSpline;
		QPointF m_nextVertexImagePos;
	};
	
	class VertexDragHandler : public State
	{
	public:
		VertexDragHandler(PictureZoneEditor& owner, Spline::Ptr const& spline, Vertex::Ptr const& vertex);
		
		virtual void activated();

		virtual void paint(QPainter& painter);
		
		virtual void mousePressEvent(QMouseEvent* event);
		
		virtual void mouseReleaseEvent(QMouseEvent* event);
		
		virtual void mouseMoveEvent(QMouseEvent* event);
	private:
		void update();
		
		BasicVisualizer m_basicVisualizer;
		Spline::Ptr m_ptrSpline;
		Vertex::Ptr m_ptrVertex;
		QPointF m_dragOffset;
	};
	
	class ContextMenuHandler : public ContextMenuHandlerBase, public State
	{
	public:
		ContextMenuHandler(PictureZoneEditor& owner, std::vector<unsigned> const& spline_indexes,
			QPoint const& mouse_pos);
	private:
		class OrderByArea;
		
		class Visualizer : public BasicVisualizer
		{
		public:
			void switchToFillMode(QColor const& color);
			
			void switchToStrokeMode();

			virtual void prepareForSpline(QPainter& painter, PictureSpline::Ptr const& spline);
		private:
			QColor m_color;
		};
		
		virtual void activated();

		virtual void paint(QPainter& painter);
		
		virtual void menuAboutToHide();
	
		virtual void propertiesRequest(int spline_idx);

		virtual void deleteRequest(int spline_idx);
		
		virtual void highlightItem(int spline_idx);

		virtual void setType(int spline_idx, PictureZone::Type type);
		
		void switchToDefaultState();
		
		Visualizer m_visualizer;
		std::vector<unsigned> m_splineIndexes;
		std::map<int, QColor> m_colorMap;
		std::auto_ptr<QMenu> m_ptrMenu;
		int m_highlightedSplineIdx;
		int m_dontSwitchToDefaultState;
	};

	bool validateScreenPictureMask() const;

	void schedulePictureMaskRebuild();

	void screenPictureMaskBuilt(QPoint const& origin, QImage const& mask);

	void paintOverPictureMask(QPainter& painter);

	void addSpline(PictureSpline::Ptr const& spline);
	
	void commitZones();
	
	QTransform toScreen() const;
	
	QTransform fromScreen() const;
	
	static double sqdist(QPointF const& p1, QPointF const& p2);
	
	static double sqdistToLineSegment(QPointF const& pt, QLineF const& line, QPointF* point_on_segment);
	
	static void visualizeVertex(QPainter& painter, QPointF const& pt, QColor const& color);
	
	static QPointF screenPos(QMouseEvent* event);
	
	imageproc::BinaryImage m_origPictureMask;
	QPixmap m_screenPictureMask;
	QPoint m_screenPictureMaskOrigin;
	QTransform m_screenPictureMaskXform;
	QTransform m_potentialPictureMaskXform;
	QTimer m_pictureMaskRebuildTimer;
	QTimer m_pictureMaskAnimateTimer;
	int m_pictureMaskAnimationPhase; // degrees

	IntrusivePtr<MaskTransformTask> m_ptrMaskTransformTask;
	PageId m_pageId;
	IntrusivePtr<Settings> m_ptrSettings;
	std::vector<PictureSpline::Ptr> m_splines;
	HandlerList m_handlerList;
};

} // namespace output

#endif
