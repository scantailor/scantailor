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

#include "OutputImageParams.h"
#include "ImageTransformation.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "../../Utils.h"
#include <QPolygonF>
#include <QDomDocument>
#include <QDomElement>
#include <math.h>

namespace output
{

OutputImageParams::OutputImageParams(
	QSize const& out_image_size, QRect const& content_rect,
	ImageTransformation xform,
	Dpi const& dpi, ColorParams const& color_params,
	DewarpingMode const& dewarping_mode,
	dewarping::DistortionModel const& distortion_model,
	DepthPerception const& depth_perception,
	DespeckleLevel const despeckle_level)
:	m_size(out_image_size),
	m_contentRect(content_rect),
	m_dpi(dpi),
	m_colorParams(color_params),
	m_distortionModel(distortion_model),
	m_depthPerception(depth_perception),
	m_dewarpingMode(dewarping_mode),
	m_despeckleLevel(despeckle_level)
{
	// For historical reasons, we disregard post-cropping and post-scaling here.
	xform.setPostCropArea(QPolygonF()); // Resets post-scale as well.
	m_partialXform = xform.transform();
}

OutputImageParams::OutputImageParams(QDomElement const& el)
:	m_size(XmlUnmarshaller::size(el.namedItem("size").toElement())),
	m_contentRect(XmlUnmarshaller::rect(el.namedItem("content-rect").toElement())),
	m_partialXform(el.namedItem("partial-xform").toElement()),
	m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement())),
	m_colorParams(el.namedItem("color-params").toElement()),
	m_distortionModel(el.namedItem("distortion-model").toElement()),
	m_depthPerception(el.attribute("depthPerception")),
	m_dewarpingMode(el.attribute("dewarpingMode")),
	m_despeckleLevel(despeckleLevelFromString(el.attribute("despeckleLevel")))
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
	el.appendChild(m_distortionModel.toXml(doc, "distortion-model"));
	el.setAttribute("depthPerception", m_depthPerception.toString());
	el.setAttribute("dewarpingMode", m_dewarpingMode.toString());
	el.setAttribute("despeckleLevel", despeckleLevelToString(m_despeckleLevel));
	
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
	
	if (!colorParamsMatch(m_colorParams, m_despeckleLevel,
			other.m_colorParams, other.m_despeckleLevel)) {
		return false;
	}

	if (m_dewarpingMode != other.m_dewarpingMode) {
		return false;
	} else if (m_dewarpingMode != DewarpingMode::OFF) {
		if (!m_distortionModel.matches(other.m_distortionModel)) {
			return false;
		}
		if (m_depthPerception.value() != other.m_depthPerception.value()) {
			return false;
		}
	}
	
	return true;
}

bool
OutputImageParams::colorParamsMatch(
	ColorParams const& cp1, DespeckleLevel const dl1,
	ColorParams const& cp2, DespeckleLevel const dl2)
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
			if (dl1 != dl2) {
				return false;
			}
			break;
		default:;
	}
	
	return true;
}

/*=============================== PartialXform =============================*/

OutputImageParams::PartialXform::PartialXform()
:	m_11(),
	m_12(),
	m_21(),
	m_22()
{
}

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
