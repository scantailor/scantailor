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
#include "QtSignalForwarder.h"
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
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#endif
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
	return create(
		context, interaction,
		boost::bind(&ZoneContextMenuInteraction::defaultMenuCustomizer, _1, _2)
	);
}

ZoneContextMenuInteraction*
ZoneContextMenuInteraction::create(
	ZoneInteractionContext& context, InteractionState& interaction,
	MenuCustomizer const& menu_customizer)
{
	std::vector<Zone> selectable_zones(zonesUnderMouse(context));	

	if (selectable_zones.empty()) {
		return 0;
	} else {
		return new ZoneContextMenuInteraction(
			context, interaction, menu_customizer, selectable_zones
		);
	}
}

std::vector<ZoneContextMenuInteraction::Zone>
ZoneContextMenuInteraction::zonesUnderMouse(ZoneInteractionContext& context)
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

	return selectable_zones;
}

ZoneContextMenuInteraction::ZoneContextMenuInteraction(
	ZoneInteractionContext& context, InteractionState& interaction,
	MenuCustomizer const& menu_customizer, std::vector<Zone>& selectable_zones)
:	m_rContext(context),
	m_ptrMenu(new QMenu(&context.imageView())),
	m_highlightedZoneIdx(-1),
	m_menuItemTriggered(false)
{
#ifdef Q_WS_MAC
	m_extraDelaysDone = 0;
#endif

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
	connect(hover_map, SIGNAL(mapped(int)), SLOT(highlightItem(int)));

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

		StandardMenuItems const std_items(
			propertiesMenuItemFor(*it),
			deleteMenuItemFor(*it)
		);

		BOOST_FOREACH(ZoneContextMenuItem const& item, menu_customizer(*it, std_items)) {
			QAction* action = m_ptrMenu->addAction(pixmap, item.label());
			new QtSignalForwarder(
				action, SIGNAL(triggered()),
				boost::bind(
					&ZoneContextMenuInteraction::menuItemTriggered,
					this, boost::ref(interaction), item.callback()
				)
			);
			
			hover_map->setMapping(action, i);
			connect(action, SIGNAL(hovered()), hover_map, SLOT(map()));
		}

		m_ptrMenu->addSeparator();
	}

	// The queued connection is used to ensure it gets called *after*
	// QAction::triggered().
	connect(
		m_ptrMenu.get(), SIGNAL(aboutToHide()),
		SLOT(menuAboutToHide()), Qt::QueuedConnection
	);

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
	if (m_menuItemTriggered) {
		return;
	}

#ifdef Q_WS_MAC
	// On OSX, QAction::triggered() is emitted significantly (like 150ms)
	// later than QMenu::aboutToHide().  This makes it generally not possible
	// to tell whether the menu was just dismissed or a menu item was clicked.
	// The only way to tell is to check back later, which we do here.
	if (m_extraDelaysDone++ < 1) {
		QTimer::singleShot(200, this, SLOT(menuAboutToHide()));
		return;
	}
#endif

	InteractionHandler* next_handler = m_rContext.createDefaultInteraction();
	if (next_handler) {
		makePeerPreceeder(*next_handler);
	}

	unlink();
	m_rContext.imageView().update();
	deleteLater();
}

void
ZoneContextMenuInteraction::menuItemTriggered(
	InteractionState& interaction, ZoneContextMenuItem::Callback const& callback)
{
	m_menuItemTriggered = true;
	m_visualizer.switchToStrokeMode();
	
	InteractionHandler* next_handler = callback(interaction);
	if (next_handler) {
		makePeerPreceeder(*next_handler);
	}

	unlink();
	m_rContext.imageView().update();
	deleteLater();
}

InteractionHandler*
ZoneContextMenuInteraction::propertiesRequest(EditableZoneSet::Zone const& zone)
{
	m_rContext.showPropertiesCommand(zone);
	return m_rContext.createDefaultInteraction();
}

InteractionHandler*
ZoneContextMenuInteraction::deleteRequest(EditableZoneSet::Zone const& zone)
{
	QMessageBox::StandardButton const btn = QMessageBox::question(
		&m_rContext.imageView(), tr("Delete confirmation"), tr("Really delete this zone?"),
		QMessageBox::Yes|QMessageBox::No
	);
	if (btn == QMessageBox::Yes) {
		m_rContext.zones().removeZone(zone.spline());
		m_rContext.zones().commit();
	}

	return m_rContext.createDefaultInteraction();
}

ZoneContextMenuItem
ZoneContextMenuInteraction::deleteMenuItemFor(
	EditableZoneSet::Zone const& zone)
{
	return ZoneContextMenuItem(
		tr("Delete"),
		boost::bind(&ZoneContextMenuInteraction::deleteRequest, this, zone)
	);
}

ZoneContextMenuItem
ZoneContextMenuInteraction::propertiesMenuItemFor(
	EditableZoneSet::Zone const& zone)
{
	return ZoneContextMenuItem(
		tr("Properties"),
		boost::bind(&ZoneContextMenuInteraction::propertiesRequest, this, zone)
	);
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

std::vector<ZoneContextMenuItem>
ZoneContextMenuInteraction::defaultMenuCustomizer(
	EditableZoneSet::Zone const& zone, StandardMenuItems const& std_items)
{
	std::vector<ZoneContextMenuItem> items;
	items.reserve(2);
	items.push_back(std_items.propertiesItem);
	items.push_back(std_items.deleteItem);
	return items;
}


/*========================== StandardMenuItem =========================*/

ZoneContextMenuInteraction::StandardMenuItems::StandardMenuItems(
	ZoneContextMenuItem const& properties_item,
	ZoneContextMenuItem const& delete_item)
:	propertiesItem(properties_item),
	deleteItem(delete_item)
{
}


/*============================= Visualizer ============================*/

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
