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

#include "OutputParams.h"
#include <QDomDocument>
#include <QDomElement>

namespace output
{

OutputParams::OutputParams(
	OutputImageParams const& image_params, OutputFileParams const& file_params)
:	m_imageParams(image_params),
	m_fileParams(file_params)
{
}

OutputParams::OutputParams(QDomElement const& el)
:	m_imageParams(el.namedItem("image").toElement()),
	m_fileParams(el.namedItem("file").toElement())
{
}

QDomElement
OutputParams::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.appendChild(m_imageParams.toXml(doc, "image"));
	el.appendChild(m_fileParams.toXml(doc, "file"));
	return el;
}

} // namespace output
