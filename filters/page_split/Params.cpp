/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "Params.h"
#include <QDomDocument>
#include <QDomElement>

namespace page_split
{

Params::Params(PageLayout const& layout,
	Dependencies const& deps,
	AutoManualMode const split_line_mode)
:	m_layout(layout),
	m_deps(deps),
	m_splitLineMode(split_line_mode)
{
}

Params::Params(QDomElement const& el)
:	m_layout(el.namedItem("pages").toElement()),
	m_deps(el.namedItem("dependencies").toElement()),
	m_splitLineMode(
		el.attribute("mode") == "manual"
		? MODE_MANUAL : MODE_AUTO
	)
{
}

Params::~Params()
{
}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute(
		"mode", m_splitLineMode == MODE_AUTO ? "auto" : "manual"
	);
	el.appendChild(m_layout.toXml(doc, "pages"));
	el.appendChild(m_deps.toXml(doc, "dependencies"));
	return el;
}

} // namespace page_split
