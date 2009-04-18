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

#ifndef PAGE_LAYOUT_IMAGEVIEW_H_
#define PAGE_LAYOUT_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "ImageTransformation.h"
#include "PhysicalTransformation.h"
#include "Alignment.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include <QTransform>
#include <QSizeF>
#include <QRectF>
#include <QPointF>
#include <QPoint>

class Margins;

namespace page_layout
{

class OptionsWidget;
class Settings;

class ImageView : public ImageViewBase
{
	Q_OBJECT
public:
	ImageView(
		IntrusivePtr<Settings> const& settings, PageId const& page_id,
		QImage const& image, QImage const& downscaled_image,
		ImageTransformation const& xform,
		QRectF const& adapted_content_rect,
		OptionsWidget const& opt_widget);
	
	virtual ~ImageView();
signals:
	void invalidateThumbnail(PageId const& page_id);
	
	void invalidateAllThumbnails();
	
	void marginsSetLocally(Margins const& margins_mm);
public slots:
	void marginsSetExternally(Margins const& margins_mm);
	
	void leftRightLinkToggled(bool linked);
	
	void topBottomLinkToggled(bool linked);
	
	void alignmentChanged(Alignment const& alignment);
	
	void aggregateHardSizeChanged();
protected:
	virtual void paintOverImage(QPainter& painter);
	
	virtual void wheelEvent(QWheelEvent* event);
	
	virtual void mousePressEvent(QMouseEvent* event);
	
	virtual void mouseReleaseEvent(QMouseEvent* event);
	
	virtual void mouseMoveEvent(QMouseEvent* event);
	
	virtual void hideEvent(QHideEvent* event);
	
	virtual QString defaultStatusTip() const;
private:
	enum { TOP_EDGE = 1, BOTTOM_EDGE = 2, LEFT_EDGE = 4, RIGHT_EDGE = 8 };
	enum FitMode { FIT, DONT_FIT };
	enum AggregateSizeChanged { AGGREGATE_SIZE_UNCHANGED, AGGREGATE_SIZE_CHANGED };
	
	struct StateBeforeResizing
	{
		/**
		 * Transformation from m_origXform coordinates to widget
		 * coordinates.
		 */
		QTransform origToWidget;
		
		/**
		 * Transformation from widget coordinates to m_origXform coordinates
		 */
		QTransform widgetToOrig;
		
		/**
		 * m_middleRect in widget coordinates.
		 */
		QRectF middleWidgetRect;
		
		/**
		 * Mouse pointer position in widget coordinates.
		 */
		QPoint mousePos;
		
		/**
		 * The point in image that is to be centered on the screen
		 * in physical image coordinates.
		 */
		QPointF focalPoint;
	};
	
	void resizeInnerRect(QPoint delta);
	
	void resizeMiddleRect(QPoint delta);
	
	void recalcBoxesAndFit(Margins const& margins_mm);
	
	void updatePresentationTransform(FitMode fit_mode);
	
	int cursorLocationMask(QPoint const& cursor_pos, QRectF const& orig_rect) const;
	
	void forceNonNegativeHardMargins(QRectF& middle_rect) const;
	
	Margins calcHardMarginsMM() const;
	
	void recalcOuterRect();
	
	QSizeF origRectToSizeMM(QRectF const& rect) const;
	
	AggregateSizeChanged commitHardMargins(Margins const& margins_mm);
	
	void invalidateThumbnails(AggregateSizeChanged agg_size_changed);
	
	QString m_defaultStatusTip;
	
	IntrusivePtr<Settings> m_ptrSettings;
	
	PageId const m_pageId;
	
	/**
	 * \brief Image transformation, as provided by the previous filter.
	 *
	 * We pass another transformation to ImageViewBase, which we call
	 * "presentation transformation" in order to be able to display margins
	 * that may be outside the image area.  The presentation transformation
	 * is accessible via physToVirt().
	 */
	ImageTransformation const m_origXform;
	
	/**
	 * Transformation between the original image coordinates and millimeters,
	 * assuming that point (0, 0) in pixel coordinates corresponds to point
	 * (0, 0) in millimeter coordinates.
	 */
	PhysicalTransformation const m_physXform;
	
	/**
	 * Transformation from m_origXform coordinates to millimeter coordinates.
	 */
	QTransform const m_origToMM;
	
	/**
	 * Transformation from millimeter coordinates to m_origXform coordinates.
	 */
	QTransform const m_mmToOrig;
	
	/**
	 * Content box in m_origXform coordinates.
	 */
	QRectF const m_innerRect;
	
	/**
	 * \brief Content box + hard margins in m_origXform coordinates.
	 *
	 * Hard margins are margins that will be there no matter what.
	 * Soft margins are those added to extend the page to match its
	 * size with other pages.
	 */
	QRectF m_middleRect;
	
	/**
	 * \brief Content box + hard + soft margins in m_origXform coordinates.
	 *
	 * Hard margins are margins that will be there no matter what.
	 * Soft margins are those added to extend the page to match its
	 * size with other pages.
	 */
	QRectF m_outerRect;
	
	/**
	 * \brief Aggregate (max width + max height) hard page size.
	 *
	 * This one is for displaying purposes only.  It changes during
	 * dragging, and it may differ from what
	 * m_ptrSettings->getAggregateHardSizeMM() would return.
	 *
	 * \see m_committedAggregateHardSizeMM
	 */
	QSizeF m_aggregateHardSizeMM;
	
	/**
	 * \brief Aggregate (max width + max height) hard page size.
	 *
	 * This one is supposed to be the cached version of what
	 * m_ptrSettings->getAggregateHardSizeMM() would return.
	 *
	 * \see m_aggregateHardSizeMM
	 */
	QSizeF m_committedAggregateHardSizeMM;
	
	Alignment m_alignment;
	
	/**
	 * Some data saved at the beginning of a resizing operation.
	 */
	StateBeforeResizing m_beforeResizing;
	
	/**
	 * A bitwise OR of *_EDGE values.  If non-zero, it means
	 * we are currently dragging one or two edges of the inner rectangle.
	 * \note Both m_innerResizingMask and m_middleResizingMask can't
	 * be non-zero at the same time.
	 */
	int m_innerResizingMask;
	
	/**
	 * A bitwise OR of *_EDGE values.  If non-zero, it means
	 * we are currently dragging one or two edges of the middle rectangle.
	 * \note Both m_innerResizingMask and m_outerResizingMask can't
	 * be non-zero at the same time.
	 */
	int m_middleResizingMask;
	
	bool m_leftRightLinked;
	
	bool m_topBottomLinked;
};

} // namespace page_layout

#endif
