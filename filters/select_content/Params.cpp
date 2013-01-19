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

#include "Params.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <QDomDocument>
#include <QDomElement>

#include "CommandLine.h"

namespace select_content
{

Params::Params(
	QRectF const& content_rect, QSizeF const& content_size_mm,
	Dependencies const& deps, AutoManualMode const mode)
:	m_contentRect(content_rect),
	m_pageRect(content_rect),
    m_pageBorders(0,0,0,0),
	m_contentSizeMM(content_size_mm),
	m_deps(deps),
	m_mode(mode),
	m_deviation(0.0)
{
	m_contentDetect = CommandLine::get().isContentDetectionEnabled();
	m_pageDetect = CommandLine::get().isPageDetectionEnabled();
	m_fineTuneCorners = CommandLine::get().isFineTuningEnabled();
}

Params::Params(
	QRectF const& content_rect, QSizeF const& content_size_mm,
	Dependencies const& deps, AutoManualMode const mode, bool contentDetect, bool pageDetect, bool fineTuning)
:	m_contentRect(content_rect),
	m_pageRect(content_rect),
    m_pageBorders(0,0,0,0),
	m_contentSizeMM(content_size_mm),
	m_deps(deps),
	m_mode(mode),
	m_contentDetect(contentDetect),
	m_pageDetect(pageDetect),
	m_fineTuneCorners(fineTuning),
	m_deviation(0.0)
{
}

Params::Params(Dependencies const& deps)
:	m_deps(deps),
	m_mode(MODE_AUTO),
    m_pageBorders(0,0,0,0),
	m_deviation(0.0)
{
	m_contentDetect = CommandLine::get().isContentDetectionEnabled();
	m_pageDetect = CommandLine::get().isPageDetectionEnabled();
	m_fineTuneCorners = CommandLine::get().isFineTuningEnabled();
}

Params::Params(QDomElement const& filter_el)
:	m_contentRect(
		XmlUnmarshaller::rectF(
			filter_el.namedItem("content-rect").toElement()
		)
	),
    m_pageRect(
		XmlUnmarshaller::rectF(
			filter_el.namedItem("page-rect").toElement()
		)		
	),
	m_contentSizeMM(
		XmlUnmarshaller::sizeF(
			filter_el.namedItem("content-size-mm").toElement()
		)
	),
   	m_pageBorders(
		XmlUnmarshaller::margins(
			filter_el.namedItem("page-borders").toElement()
		)
	),
	m_deps(filter_el.namedItem("dependencies").toElement()),
	m_mode(filter_el.attribute("mode") == "manual" ? MODE_MANUAL : MODE_AUTO),
	m_contentDetect(filter_el.attribute("content-detect") == "false" ? false : true),
	m_pageDetect(filter_el.attribute("page-detect") == "true" ? true : false),
	m_fineTuneCorners(filter_el.attribute("fine-tune-corners") == "true" ? true : false),
	m_deviation(filter_el.attribute("deviation").toDouble())
{
	// ! m_contentDetect means content detection is disabled and should be the same as pageRect
	// turn off pagedetection if page detection was enabled and set content detection to manual
	if (m_pageDetect && !m_contentDetect && CommandLine::get().isForcePageDetectionDisabled()) {
		m_pageDetect = false;
		m_contentRect = m_pageRect;
		m_contentDetect = true;
		m_mode = MODE_MANUAL;
	}
}

Params::~Params()
{
}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.setAttribute("mode", m_mode == MODE_AUTO ? "auto" : "manual");
	el.setAttribute("content-detect", m_contentDetect ? "true" : "false");
	el.setAttribute("page-detect", m_pageDetect ? "true" : "false");
	el.setAttribute("fine-tune-corners", m_fineTuneCorners ? "true" : "false");
	el.setAttribute("deviation", m_deviation);
	el.appendChild(marshaller.rectF(m_contentRect, "content-rect"));
	el.appendChild(marshaller.rectF(m_pageRect, "page-rect"));
	el.appendChild(marshaller.sizeF(m_contentSizeMM, "content-size-mm"));
	el.appendChild(marshaller.margins(m_pageBorders, "page-borders"));
	el.appendChild(m_deps.toXml(doc, "dependencies"));
	return el;
}

} // namespace content_rect
