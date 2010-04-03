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

#ifndef ZONE_CONTEXT_MENU_INTERACTION_H_
#define ZONE_CONTEXT_MENU_INTERACTION_H_

#include "InteractionHandler.h"
#include "InteractionState.h"
#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "PropertySet.h"
#include "IntrusivePtr.h"
#include "BasicSplineVisualizer.h"
#include <QObject>
#include <QColor>
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
	/**
	 * \note This factory method will return null if there are no zones
	 *       under the mouse pointer.
	 */
	static ZoneContextMenuInteraction* create(
		ZoneInteractionContext& context, InteractionState& interaction);

	virtual ~ZoneContextMenuInteraction();
protected:
	class Zone : public EditableZoneSet::Zone
	{
	public:
		QColor color;

		Zone(EditableZoneSet::Zone const& zone) : EditableZoneSet::Zone(zone) {}
	};

	ZoneContextMenuInteraction(ZoneInteractionContext& context,
		InteractionState& interaction, std::vector<Zone>& selectable_zones);

	ZoneInteractionContext& context() { return m_rContext; }
private slots:
	void menuAboutToHide();

	virtual void propertiesRequest(int zone_idx);

	virtual void deleteRequest(int zone_idx);

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

	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	void maybeSwitchToDefaultState();

	ZoneInteractionContext& m_rContext;
	std::vector<Zone> m_selectableZones;
	InteractionState::Captor m_interaction;
	Visualizer m_visualizer;
	std::auto_ptr<QMenu> m_ptrMenu;
	int m_highlightedZoneIdx;
	int m_switchToDefaultStatePreventer;
};

#endif
