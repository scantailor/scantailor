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

#ifndef ZONE_INTERACTION_CONTEXT_H_
#define ZONE_INTERACTION_CONTEXT_H_

#include "EditableSpline.h"
#include "SplineVertex.h"
#include "EditableZoneSet.h"
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#endif

class InteractionHandler;
class InteractionState;
class ImageViewBase;
class EditableZoneSet;

class ZoneInteractionContext
{
public:
	typedef boost::function<
		InteractionHandler* ()
	> DefaultInteractionCreator;

	typedef boost::function<
		InteractionHandler* (InteractionState& interaction)
	> ZoneCreationInteractionCreator;

	typedef boost::function<
		InteractionHandler* (
			InteractionState& interaction,
			EditableSpline::Ptr const& spline, SplineVertex::Ptr const& vertex
		)
	> VertexDragInteractionCreator;

	typedef boost::function<
		InteractionHandler* (InteractionState& interaction)
	> ContextMenuInteractionCreator;

	typedef boost::function<
		void (EditableZoneSet::Zone const& zone)
	> ShowPropertiesCommand;

	ZoneInteractionContext(ImageViewBase& image_view, EditableZoneSet& zones);

	virtual ~ZoneInteractionContext();

	ImageViewBase& imageView() { return m_rImageView; }

	EditableZoneSet& zones() { return m_rZones; }

	virtual InteractionHandler* createDefaultInteraction() {
		return m_defaultInteractionCreator();
	}

	void setDefaultInteractionCreator(DefaultInteractionCreator const& creator) {
		m_defaultInteractionCreator = creator;
	}

	virtual InteractionHandler* createZoneCreationInteraction(InteractionState& interaction) {
		return m_zoneCreationInteractionCreator(interaction);
	}

	void setZoneCreationInteractionCreator(ZoneCreationInteractionCreator const& creator) {
		m_zoneCreationInteractionCreator = creator;
	}

	virtual InteractionHandler* createVertexDragInteraction(
			InteractionState& interaction, EditableSpline::Ptr const& spline,
			SplineVertex::Ptr const& vertex) {
		return m_vertexDragInteractionCreator(interaction, spline, vertex);
	}

	void setVertexDragInteractionCreator(VertexDragInteractionCreator const& creator) {
		m_vertexDragInteractionCreator = creator;
	}

	/**
	 * \note This function may refuse to create a context menu interaction by returning null.
	 */
	virtual InteractionHandler* createContextMenuInteraction(InteractionState& interaction) {
		return m_contextMenuInteractionCreator(interaction);
	}

	void setContextMenuInteractionCreator(ContextMenuInteractionCreator const& creator) {
		m_contextMenuInteractionCreator = creator;
	}

	virtual void showPropertiesCommand(EditableZoneSet::Zone const& zone) {
		m_showPropertiesCommand(zone);
	}

	void setShowPropertiesCommand(ShowPropertiesCommand const& command) {
		m_showPropertiesCommand = command;
	}
private:
	/**
	 * Creates an instance of ZoneDefaultInteraction.
	 */
	InteractionHandler* createStdDefaultInteraction();

	/**
	 * Creates an instance of ZoneCreationInteraction.
	 */
	InteractionHandler* createStdZoneCreationInteraction(InteractionState& interaction);

	/**
	 * Creates an instance of ZoneVertexDragInteraction.
	 */
	InteractionHandler* createStdVertexDragInteraction(
		InteractionState& interaction, EditableSpline::Ptr const& spline,
		SplineVertex::Ptr const& vertex);

	/**
	 * Creates an instance of ZoneContextMenuInteraction.  May return null.
	 */
	InteractionHandler* createStdContextMenuInteraction(InteractionState& interaction);

	static void showPropertiesStub(EditableZoneSet::Zone const&) {}

	ImageViewBase& m_rImageView;
	EditableZoneSet& m_rZones;
	DefaultInteractionCreator m_defaultInteractionCreator;
	ZoneCreationInteractionCreator m_zoneCreationInteractionCreator;
	VertexDragInteractionCreator m_vertexDragInteractionCreator;
	ContextMenuInteractionCreator m_contextMenuInteractionCreator;
	ShowPropertiesCommand m_showPropertiesCommand;
};

#endif
