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

#include "ZoneContextMenuInteraction.h"
#include "ZoneContextMenuInteraction.h.moc"
#include "ZoneInteractionContext.h"
#include "ImageViewBase.h"
#include "EditableZoneSet.h"
#include <QRectF>
#include <QPolygonF>
#include <QMenu>
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QTransform>
#include <QSignalMapper>
#include <QCursor>
#include <QMessageBox>
#include <boost/foreach.hpp>
#include <vector>
#include <assert.h>

class ZoneContextMenuInteraction::OrderByArea
{
public:
	bool operator()(EditableZoneSet::Zone const& lhs, EditableZoneSet::Zone const& rhs) const {
		QRectF const lhs_bbox(lhs.spline()->toPolygon().boundingRect());
		QRectF const rhs_bbox(rhs.spline()->toPolygon().boundingRect());
		qreal const lhs_area = lhs_bbox.width() * lhs_bbox.height();
		qreal const rhs_area = rhs_bbox.width() * rhs_bbox.height();
		return lhs_area < rhs_area;
	}
};

ZoneContextMenuInteraction*
ZoneContextMenuInteraction::create(
	ZoneInteractionContext& context, InteractionState& interaction)
{
	QTransform const from_screen(context.imageView().widgetToImage());
	QPointF const image_mouse_pos(
		from_screen.map(context.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5))
	);

	// Find zones containing the mouse position.
	std::vector<Zone> selectable_zones;
	BOOST_FOREACH(EditableZoneSet::Zone const& zone, context.zones()) {
		QPainterPath path;
		path.setFillRule(Qt::WindingFill);
		path.addPolygon(zone.spline()->toPolygon());
		if (path.contains(image_mouse_pos)) {
			selectable_zones.push_back(Zone(zone));
		}
	}

	if (selectable_zones.empty()) {
		return 0;
	} else {
		return new ZoneContextMenuInteraction(context, interaction, selectable_zones);
	}
}


ZoneContextMenuInteraction::ZoneContextMenuInteraction(
	ZoneInteractionContext& context,
	InteractionState& interaction, std::vector<Zone>& selectable_zones)
:	m_rContext(context),
	m_ptrMenu(new QMenu(&context.imageView())),
	m_highlightedZoneIdx(-1),
	m_switchToDefaultStatePreventer(1)
{
	m_selectableZones.swap(selectable_zones);
	std::sort(m_selectableZones.begin(), m_selectableZones.end(), OrderByArea());

	interaction.capture(m_interaction);

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

	std::vector<Zone>::iterator it(m_selectableZones.begin());
	std::vector<Zone>::iterator const end(m_selectableZones.end());
	for (int i = 0; it != end; ++it, ++i, h = (h + h_step) % 360) {
		color.setHsv(h, s, v, alpha);
		it->color = color.toRgb();

		if (m_selectableZones.size() > 1) {
			pixmap = QPixmap(16, 16);
			color.setAlpha(255);
			pixmap.fill(color);
		}

		QAction* action = m_ptrMenu->addAction(pixmap, tr("Properties"));
		prop_trigger_map->setMapping(action, i);
		hover_map->setMapping(action, i);
		connect(action, SIGNAL(triggered()), prop_trigger_map, SLOT(map()));
		connect(action, SIGNAL(hovered()), hover_map, SLOT(map()));

		action = m_ptrMenu->addAction(pixmap, tr("Delete"));
		del_trigger_map->setMapping(action, i);
		hover_map->setMapping(action, i);
		connect(action, SIGNAL(triggered()), del_trigger_map, SLOT(map()));
		connect(action, SIGNAL(hovered()), hover_map, SLOT(map()));

		m_ptrMenu->addSeparator();
	}

	connect(m_ptrMenu.get(), SIGNAL(aboutToHide()), SLOT(menuAboutToHide()), Qt::QueuedConnection);

	highlightItem(0);
	m_ptrMenu->popup(QCursor::pos());
}

ZoneContextMenuInteraction::~ZoneContextMenuInteraction()
{
}

void
ZoneContextMenuInteraction::onPaint(QPainter& painter, InteractionState const&)
{
	painter.setWorldMatrixEnabled(false);
	painter.setRenderHint(QPainter::Antialiasing);

	if (m_highlightedZoneIdx >= 0) {
		QTransform const to_screen(m_rContext.imageView().imageToWidget());
		Zone const& zone = m_selectableZones[m_highlightedZoneIdx];
		m_visualizer.drawSpline(painter, to_screen, zone.spline());
	}
}

void
ZoneContextMenuInteraction::menuAboutToHide()
{
	maybeSwitchToDefaultState();
}

void
ZoneContextMenuInteraction::propertiesRequest(int const zone_idx)
{
	++m_switchToDefaultStatePreventer;

	m_visualizer.switchToStrokeMode();
	m_rContext.showPropertiesCommand(m_selectableZones[zone_idx]);

	maybeSwitchToDefaultState();
}

void
ZoneContextMenuInteraction::deleteRequest(int const zone_idx)
{
	++m_switchToDefaultStatePreventer;

	QMessageBox::StandardButton const btn = QMessageBox::question(
		&m_rContext.imageView(), tr("Delete confirmation"), tr("Really delete this zone?"),
		QMessageBox::Yes|QMessageBox::No
	);
	if (btn == QMessageBox::Yes) {
		m_rContext.zones().removeZone(m_selectableZones[zone_idx].spline());
		m_rContext.zones().commit();
	}

	maybeSwitchToDefaultState();
}

void
ZoneContextMenuInteraction::highlightItem(int const zone_idx)
{
	if (m_selectableZones.size() > 1) {
		m_visualizer.switchToFillMode(m_selectableZones[zone_idx].color);
	} else {
		m_visualizer.switchToStrokeMode();
	}
	m_highlightedZoneIdx = zone_idx;
	m_rContext.imageView().update();
}

void
ZoneContextMenuInteraction::maybeSwitchToDefaultState()
{
	if (--m_switchToDefaultStatePreventer != 0) {
		assert(m_switchToDefaultStatePreventer > 0);
		return;
	}

	makePeerPreceeder(*m_rContext.createDefaultInteraction());
	m_rContext.imageView().update();
	deleteLater();
	unlink();
}


/*====================== ContextMenuHandler::Visualizer =========================*/

void
ZoneContextMenuInteraction::Visualizer::switchToFillMode(QColor const& color)
{
	m_color = color;
}

void
ZoneContextMenuInteraction::Visualizer::switchToStrokeMode()
{
	m_color = QColor();
}

void
ZoneContextMenuInteraction::Visualizer::prepareForSpline(
	QPainter& painter, EditableSpline::Ptr const& spline)
{
	BasicSplineVisualizer::prepareForSpline(painter, spline);
	if (m_color.isValid()) {
		painter.setBrush(m_color);
	}
}
