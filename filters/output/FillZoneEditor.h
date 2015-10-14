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


#ifndef OUTPUT_FILL_ZONE_EDITOR_H_
#define OUTPUT_FILL_ZONE_EDITOR_H_

#include "ImageViewBase.h"
#include "ImagePixmapUnion.h"
#include "NonCopyable.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include "ZoneInteractionContext.h"
#include "ColorPickupInteraction.h"
#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "ZoomHandler.h"
#include "DragHandler.h"
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#endif
#include <QPoint>
#include <QPointF>
#include <QColor>

class InteractionState;
class QPainter;

namespace output
{

class Settings;


class FillZoneEditor : public ImageViewBase, private InteractionHandler
{
	Q_OBJECT
public:
	FillZoneEditor(
		QImage const& image, ImagePixmapUnion const& downscaled_version,
		boost::function<QPointF(QPointF const&)> const& orig_to_image,
		boost::function<QPointF(QPointF const&)> const& image_to_orig,
		PageId const& page_id, IntrusivePtr<Settings> const& settings);
	
	virtual ~FillZoneEditor();
signals:
	void invalidateThumbnail(PageId const& page_id);
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);
private slots:
	void commitZones();

	void updateRequested();
private:
	class MenuCustomizer;

	typedef QColor (*ColorAdapter)(QColor const&);

	InteractionHandler* createContextMenuInteraction(InteractionState& interaction);

	InteractionHandler* createColorPickupInteraction(
		EditableZoneSet::Zone const& zone, InteractionState& interaction);

	static QColor toOpaque(QColor const& color);

	static QColor toGrayscale(QColor const& color);

	static QColor toBlackWhite(QColor const& color);

	static ColorAdapter colorAdapterFor(QImage const& image);

	ColorAdapter m_colorAdapter;
	EditableZoneSet m_zones;

	// Must go after m_zones.
	ZoneInteractionContext m_context;

	// Must go after m_context.
	ColorPickupInteraction m_colorPickupInteraction;
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;	

	boost::function<QPointF(QPointF const&)> m_origToImage;
	boost::function<QPointF(QPointF const&)> m_imageToOrig;
	PageId m_pageId;
	IntrusivePtr<Settings> m_ptrSettings;
};

} // namespace output

#endif
