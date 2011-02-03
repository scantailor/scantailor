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

#include "OutputFileParams.h"
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QDateTime>

namespace output
{

OutputFileParams::OutputFileParams()
:	m_size(-1),
	m_mtime(0)
{
}

OutputFileParams::OutputFileParams(QFileInfo const& file_info)
:	m_size(-1),
	m_mtime(0)
{
	if (file_info.exists()) {
		m_size = file_info.size();
		m_mtime = file_info.lastModified().toTime_t();
	}
}

OutputFileParams::OutputFileParams(QDomElement const& el)
:	m_size(-1),
	m_mtime(0)
{
	if (el.hasAttribute("size")) {
		m_size = (qint64)el.attribute("size").toLongLong();
	}
	if (el.hasAttribute("mtime")) {
		m_mtime = (time_t)el.attribute("mtime").toLongLong();
	}
}

QDomElement
OutputFileParams::toXml(QDomDocument& doc, QString const& name) const
{
	if (isValid()) {
		QDomElement el(doc.createElement(name));
		el.setAttribute("size", QString::number(m_size));
		el.setAttribute("mtime", QString::number(m_mtime));
		return el;
	} else {
		return QDomElement();
	}
}

bool
OutputFileParams::matches(OutputFileParams const& other) const
{
	return isValid() && other.isValid() &&
			m_size == other.m_size/* && m_mtime == other.m_mtime*/;
}

} // namespace output
