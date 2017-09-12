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

#ifndef ZONE_CONTEXT_MENU_INTERACTION_H_
#define ZONE_CONTEXT_MENU_INTERACTION_H_

#include "ZoneContextMenuItem.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "PropertySet.h"
#include "IntrusivePtr.h"
#include "BasicSplineVisualizer.h"
#include <QObject>
#include <QColor>
#include <QtGlobal>
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#endif
#include <map>
#include <memory>
#include <vector>

class ZoneInteractionContext;
class QPainter;
class QMenu;

class ZoneContextMenuInteraction : public QObject, public InteractionHandler
{
	Q_OBJECT
public:
	struct StandardMenuItems
	{
		ZoneContextMenuItem propertiesItem;
		ZoneContextMenuItem deleteItem;

		StandardMenuItems(
			ZoneContextMenuItem const& properties_item,
			ZoneContextMenuItem const& delete_item);
	};

	typedef boost::function<
		std::vector<ZoneContextMenuItem>(
			EditableZoneSet::Zone const&, StandardMenuItems const&
		)
	> MenuCustomizer;

	/**
	 * \note This factory method will return null if there are no zones
	 *       under the mouse pointer.
	 */
	static ZoneContextMenuInteraction* create(
		ZoneInteractionContext& context, InteractionState& interaction);

	/**
	 * Same as above, plus a menu customization callback.
	 */
	static ZoneContextMenuInteraction* create(
		ZoneInteractionContext& context, InteractionState& interaction,
		MenuCustomizer const& menu_customizer);

	virtual ~ZoneContextMenuInteraction();
protected:
	class Zone : public EditableZoneSet::Zone
	{
	public:
		QColor color;

		Zone(EditableZoneSet::Zone const& zone) : EditableZoneSet::Zone(zone) {}
	};

	static std::vector<Zone> zonesUnderMouse(ZoneInteractionContext& context);

	ZoneContextMenuInteraction(
		ZoneInteractionContext& context, InteractionState& interaction,
		MenuCustomizer const& menu_customizer, std::vector<Zone>& selectable_zones);

	ZoneInteractionContext& context() { return m_rContext; }
private slots:
	void menuAboutToHide();

	void highlightItem(int zone_idx);
private:
	class OrderByArea;

	class Visualizer : public BasicSplineVisualizer
	{
	public:
		void switchToFillMode(QColor const& color);

		void switchToStrokeMode();

		virtual void prepareForSpline(QPainter& painter, EditableSpline::Ptr const& spline);
	private:
		QColor m_color;
	};

	static std::vector<ZoneContextMenuItem> defaultMenuCustomizer(
		EditableZoneSet::Zone const& zone, StandardMenuItems const& std_items);

	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	void menuItemTriggered(InteractionState& interaction,
		ZoneContextMenuItem::Callback const& callback);

	InteractionHandler* deleteRequest(EditableZoneSet::Zone const& zone);

	InteractionHandler* propertiesRequest(EditableZoneSet::Zone const& zone);

	ZoneContextMenuItem propertiesMenuItemFor(EditableZoneSet::Zone const& zone);

	ZoneContextMenuItem deleteMenuItemFor(EditableZoneSet::Zone const& zone);

	ZoneInteractionContext& m_rContext;
	std::vector<Zone> m_selectableZones;
	InteractionState::Captor m_interaction;
	Visualizer m_visualizer;
	std::auto_ptr<QMenu> m_ptrMenu;
	int m_highlightedZoneIdx;
	bool m_menuItemTriggered;
#ifdef Q_WS_MAC
	int m_extraDelaysDone;
#endif
};

#endif
