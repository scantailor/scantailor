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

#ifndef ZONE_CONTEXT_MENU_ITEM_H_
#define ZONE_CONTEXT_MENU_ITEM_H_

#include <QString>
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#endif

class InteractionState;
class InteractionHandler;

class ZoneContextMenuItem
{
public:
	/**
	 * A callback may either return the InteractionHandler to connect
	 * in place of ZoneContextMenuInteraction, or null, indicating not
	 * to connect anything when ZoneContextMenuInteraction is disconnected.
	 * The ownership of the returned InteractionHandler will be transferred
	 * to ZoneInteractionContext.  It's still possible for the returned
	 * InteractionHandler to be a member of another object, but in this case
	 * you will need to make sure it's disconnected from ZoneInteractionContext
	 * before ZoneInteractionContext destroys.
	 */
	typedef boost::function<InteractionHandler*(InteractionState&)> Callback;

	ZoneContextMenuItem(QString const& label, Callback const& callback)
		: m_label(label), m_callback(callback) {}

	QString const& label() const { return m_label; }

	Callback const& callback() const { return m_callback; }
private:
	QString m_label;
	Callback m_callback;
};

#endif
