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

#include "OutputParams.h"
#include "PictureZonePropFactory.h"
#include "FillZonePropFactory.h"
#include <QDomDocument>
#include <QDomElement>

namespace output
{

OutputParams::OutputParams(
	OutputImageParams const& output_image_params,
	OutputFileParams const& output_file_params,
	OutputFileParams const& automask_file_params,
	OutputFileParams const& speckles_file_params,
	ZoneSet const& picture_zones,
	ZoneSet const& fill_zones)
:	m_outputImageParams(output_image_params),
	m_outputFileParams(output_file_params),
	m_automaskFileParams(automask_file_params),
	m_specklesFileParams(speckles_file_params),
	m_pictureZones(picture_zones),
	m_fillZones(fill_zones)
{
}

OutputParams::OutputParams(QDomElement const& el)
:	m_outputImageParams(el.namedItem("image").toElement()),
	m_outputFileParams(el.namedItem("file").toElement()),
	m_automaskFileParams(el.namedItem("automask").toElement()),
	m_specklesFileParams(el.namedItem("speckles").toElement()),
	m_pictureZones(el.namedItem("zones").toElement(), PictureZonePropFactory()),
	m_fillZones(el.namedItem("fill-zones").toElement(), FillZonePropFactory())
{
}

QDomElement
OutputParams::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.appendChild(m_outputImageParams.toXml(doc, "image"));
	el.appendChild(m_outputFileParams.toXml(doc, "file"));
	el.appendChild(m_automaskFileParams.toXml(doc, "automask"));
	el.appendChild(m_specklesFileParams.toXml(doc, "speckles"));
	el.appendChild(m_pictureZones.toXml(doc, "zones"));
	el.appendChild(m_fillZones.toXml(doc, "fill-zones"));
	return el;
}

} // namespace output
