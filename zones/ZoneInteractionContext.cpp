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

#include "ZoneInteractionContext.h"
#include "ZoneDefaultInteraction.h"
#include "ZoneCreationInteraction.h"
#include "ZoneVertexDragInteraction.h"
#include "ZoneContextMenuInteraction.h"
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#endif

ZoneInteractionContext::ZoneInteractionContext(
	ImageViewBase& image_view, EditableZoneSet& zones)
:	m_rImageView(image_view),
	m_rZones(zones),
	m_defaultInteractionCreator(
		boost::bind(&ZoneInteractionContext::createStdDefaultInteraction, this)
	),
	m_zoneCreationInteractionCreator(
		boost::bind(&ZoneInteractionContext::createStdZoneCreationInteraction, this, _1)
	),
	m_vertexDragInteractionCreator(
		boost::bind(&ZoneInteractionContext::createStdVertexDragInteraction, this, _1, _2, _3)
	),
	m_contextMenuInteractionCreator(
		boost::bind(&ZoneInteractionContext::createStdContextMenuInteraction, this, _1)
	),
	m_showPropertiesCommand(&ZoneInteractionContext::showPropertiesStub)
{
}

ZoneInteractionContext::~ZoneInteractionContext()
{
}

InteractionHandler*
ZoneInteractionContext::createStdDefaultInteraction()
{
	return new ZoneDefaultInteraction(*this);
}

InteractionHandler*
ZoneInteractionContext::createStdZoneCreationInteraction(InteractionState& interaction)
{
	return new ZoneCreationInteraction(*this, interaction);
}

InteractionHandler*
ZoneInteractionContext::createStdVertexDragInteraction(
	InteractionState& interaction, EditableSpline::Ptr const& spline,
	SplineVertex::Ptr const& vertex)
{
	return new ZoneVertexDragInteraction(*this, interaction, spline, vertex);
}

InteractionHandler*
ZoneInteractionContext::createStdContextMenuInteraction(InteractionState& interaction)
{
	return ZoneContextMenuInteraction::create(*this, interaction);
}
