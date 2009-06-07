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

#include "OutputImageParams.h"
#include "ImageTransformation.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "../../Utils.h"
#include <QDomDocument>
#include <QDomElement>
#include <math.h>

namespace output
{

OutputImageParams::OutputImageParams(
	QSize const& out_image_size, QRect const& content_rect,
	ImageTransformation const& xform,
	Dpi const& dpi, ColorParams const& color_params)
:	m_size(out_image_size),
	m_contentRect(content_rect),
	m_partialXform(xform.transform()),
	m_dpi(dpi),
	m_colorParams(color_params)
{
}

OutputImageParams::OutputImageParams(QDomElement const& el)
:	m_size(XmlUnmarshaller::size(el.namedItem("size").toElement())),
	m_contentRect(XmlUnmarshaller::rect(el.namedItem("content-rect").toElement())),
	m_partialXform(el.namedItem("partial-xform").toElement()),
	m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement())),
	m_colorParams(el.namedItem("color-params").toElement())
{
}

QDomElement
OutputImageParams::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.size(m_size, "size"));
	el.appendChild(marshaller.rect(m_contentRect, "content-rect"));
	el.appendChild(m_partialXform.toXml(doc, "partial-xform"));
	el.appendChild(marshaller.dpi(m_dpi, "dpi"));
	el.appendChild(m_colorParams.toXml(doc, "color-params"));
	
	return el;
}

bool
OutputImageParams::matches(OutputImageParams const& other) const
{
	if (m_size != other.m_size) {
		return false;
	}
	
	if (m_contentRect != other.m_contentRect) {
		return false;
	}
	
	if (!m_partialXform.matches(other.m_partialXform)) {
		return false;
	}
	
	if (m_dpi != other.m_dpi) {
		return false;
	}
	
	if (!colorParamsMatch(m_colorParams, other.m_colorParams)) {
		return false;
	}
	
	return true;
}

bool
OutputImageParams::colorParamsMatch(ColorParams const& cp1, ColorParams const& cp2)
{
	if (cp1.colorMode() != cp2.colorMode()) {
		return false;
	}
	
	switch (cp1.colorMode()) {
		case ColorParams::COLOR_GRAYSCALE:
		case ColorParams::MIXED:
			if (cp1.colorGrayscaleOptions() != cp2.colorGrayscaleOptions()) {
				return false;
			}
			break;
		default:;
	}
	
	switch (cp1.colorMode()) {
		case ColorParams::BLACK_AND_WHITE:
		case ColorParams::MIXED:
			if (cp1.blackWhiteOptions() != cp2.blackWhiteOptions()) {
				return false;
			}
			break;
		default:;
	}
	
	return true;
}

/*=============================== PartialXform =============================*/

OutputImageParams::PartialXform::PartialXform(QTransform const& xform)
:	m_11(xform.m11()),
	m_12(xform.m12()),
	m_21(xform.m21()),
	m_22(xform.m22())
{
}

OutputImageParams::PartialXform::PartialXform(QDomElement const& el)
:	m_11(el.namedItem("m11").toElement().text().toDouble()),
	m_12(el.namedItem("m12").toElement().text().toDouble()),
	m_21(el.namedItem("m21").toElement().text().toDouble()),
	m_22(el.namedItem("m22").toElement().text().toDouble())
{
}

QDomElement
OutputImageParams::PartialXform::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.string(Utils::doubleToString(m_11), "m11"));
	el.appendChild(marshaller.string(Utils::doubleToString(m_12), "m12"));
	el.appendChild(marshaller.string(Utils::doubleToString(m_21), "m21"));
	el.appendChild(marshaller.string(Utils::doubleToString(m_22), "m22"));
	
	return el;
}

bool
OutputImageParams::PartialXform::matches(PartialXform const& other) const
{
	return closeEnough(m_11, other.m_11) && closeEnough(m_12, other.m_12)
		&& closeEnough(m_21, other.m_21) && closeEnough(m_22, other.m_22);
}

bool
OutputImageParams::PartialXform::closeEnough(double v1, double v2)
{
	return fabs(v1 - v2) < 0.0001;
}

} // namespace output
