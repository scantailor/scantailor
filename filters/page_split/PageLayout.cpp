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

#include "PageLayout.h"
#include "NumericTraits.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "ToLineProjector.h"
#include "imageproc/PolygonUtils.h"
#include <QPolygonF>
#include <QSizeF>
#include <QPointF>
#include <QTransform>
#include <QDomElement>
#include <QDomDocument>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <algorithm>
#include <math.h>
#include <assert.h>

using namespace imageproc;

namespace page_split
{

PageLayout::PageLayout()
:	m_type(SINGLE_PAGE_UNCUT)
{
}

PageLayout::PageLayout(QRectF const& full_rect)
:	m_uncutOutline(full_rect),
	m_cutter1(full_rect.topLeft(), full_rect.bottomLeft()),
	m_cutter2(full_rect.topRight(), full_rect.bottomRight()),
	m_type(SINGLE_PAGE_UNCUT)
{
}
	
PageLayout::PageLayout(QRectF const& full_rect, QLineF const& cutter1, QLineF const& cutter2)
:	m_uncutOutline(full_rect),
	m_cutter1(cutter1),
	m_cutter2(cutter2),
	m_type(SINGLE_PAGE_CUT)
{
}

PageLayout::PageLayout(QRectF const full_rect, QLineF const& split_line)
:	m_uncutOutline(full_rect),
	m_cutter1(split_line),
	m_type(TWO_PAGES)
{
}

PageLayout::PageLayout(
	QPolygonF const& outline, QLineF const& cutter1,
	QLineF const& cutter2, Type type)
:	m_uncutOutline(outline),
	m_cutter1(cutter1),
	m_cutter2(cutter2),
	m_type(type)
{
}

PageLayout::PageLayout(QDomElement const& layout_el)
:	m_uncutOutline(
		XmlUnmarshaller::polygonF(
			layout_el.namedItem("outline").toElement()
		)
	),
	m_cutter1(
		XmlUnmarshaller::lineF(
			layout_el.namedItem("cutter1").toElement()
		)
	),
	m_cutter2(
		XmlUnmarshaller::lineF(
			layout_el.namedItem("cutter2").toElement()
		)
	)
{
	QString const type(layout_el.attribute("type"));
	QDomElement const split_line_el(layout_el.namedItem("split-line").toElement());
	if (split_line_el.isNull()) {
		m_type = typeFromString(type);
	} else {
		// Backwards compatibility with versions < 0.9.9
		// Note that we fill in m_uncutOutline elsewhere, namely in Task::process().
		QLineF const split_line(XmlUnmarshaller::lineF(split_line_el));

		// Backwards compatibility with versions <= 0.9.1
		bool const left_page = (
			layout_el.attribute("leftPageValid") == "1"
		);
		bool const right_page = (
			layout_el.attribute("rightPageValid") == "1"
		);

		if (type == "two-pages" || (left_page && right_page)) {
			m_type = TWO_PAGES;
			m_cutter1 = split_line;
			m_cutter2 = QLineF();
		} else if (type == "left-page" || left_page) {
			m_type = SINGLE_PAGE_CUT;
			m_cutter1 = QLineF(0, 0, 0, 1); // A bit of a hack, but should work.
			m_cutter2 = split_line;
		} else if (type == "right-page" || right_page) {
			m_type = SINGLE_PAGE_CUT;
			m_cutter1 = split_line;
			m_cutter2 = QLineF();
			// This one is a special case.  We don't know the outline at this point,
			// so we make m_cutter2 null.  We'll assign the correct value to it in
			// setUncutOutline().
		} else {
			m_type = SINGLE_PAGE_UNCUT;
			m_cutter1 = QLineF();
			m_cutter2 = QLineF();
		}
	}
}

void
PageLayout::setType(Type type)
{
	m_type = type;
	if (type == TWO_PAGES) {
		m_cutter2 = m_cutter1;
	}
}

void
PageLayout::setUncutOutline(QRectF const& outline)
{
	m_uncutOutline = outline;
	if (m_uncutOutline.size() < 4) {
		// Empty rect?
		return;
	}

	if (m_type == SINGLE_PAGE_CUT && m_cutter2.isNull()) {
		// Backwards compatibility.  See PageLayout(QDomElement const&);
		m_cutter2.setP1(m_uncutOutline[1]);
		m_cutter2.setP2(m_uncutOutline[2]);
	}
}

QLineF const&
PageLayout::cutterLine(int idx) const
{
	assert(idx >= 0 && idx < numCutters());
	return idx == 0 ? m_cutter1 : m_cutter2;
}

void
PageLayout::setCutterLine(int idx, QLineF const& cutter)
{
	assert(idx >= 0 && idx < numCutters());
	(idx == 0 ? m_cutter1 : m_cutter2) = cutter;
}

LayoutType
PageLayout::toLayoutType() const
{
	switch (m_type) {
		case SINGLE_PAGE_UNCUT:
			return page_split::SINGLE_PAGE_UNCUT;
		case SINGLE_PAGE_CUT:
			return page_split::PAGE_PLUS_OFFCUT;
		case TWO_PAGES:
			return page_split::TWO_PAGES;
	}

	assert(!"Unreachable");
	return page_split::SINGLE_PAGE_UNCUT;
}

int
PageLayout::numCutters() const
{
	switch (m_type) {
		case SINGLE_PAGE_UNCUT:
			return 0;
		case SINGLE_PAGE_CUT:
			return 2;
		case TWO_PAGES:
			return 1;
	}

	assert(!"Unreachable");
	return 0;
}

int
PageLayout::numSubPages() const
{
	return m_type == TWO_PAGES ? 2 : 1;
}

QLineF
PageLayout::inscribedCutterLine(int idx) const
{
	assert(idx >= 0 && idx < numCutters());

	if (m_uncutOutline.size() < 4) {
		return QLineF();
	}

	QLineF const raw_line(cutterLine(idx));
	QPointF const origin(raw_line.p1());
	QPointF const line_vec(raw_line.p2() - origin);
	
	double min_proj = NumericTraits<double>::max();
	double max_proj = NumericTraits<double>::min();
	QPointF min_pt;
	QPointF max_pt;

	QPointF p1, p2;
	double projection;

	for (int i = 0; i < 4; ++i) {
		QLineF const poly_segment(m_uncutOutline[i], m_uncutOutline[(i + 1) & 3]);
		QPointF intersection;
		if (poly_segment.intersect(raw_line, &intersection) == QLineF::NoIntersection) {
			continue;
		}

		// Project the intersection point on poly_segment to check
		// if it's between its endpoints.
		p1 = poly_segment.p2() - poly_segment.p1();
		p2 = intersection - poly_segment.p1();
		projection = p1.x() * p2.x() + p1.y() * p2.y();
		if (projection < 0 || projection > p1.x() * p1.x() + p1.y() * p1.y()) {
			// Intersection point not on the polygon segment.
			continue;
		}

		// Now project it on raw_line and update min/max projections.
		p1 = intersection - origin;
		p2 = line_vec;
		projection = p1.x() * p2.x() + p1.y() * p2.y();
		if (projection <= min_proj) {
			min_proj = projection;
			min_pt = intersection;
		}
		if (projection >= max_proj) {
			max_proj = projection;
			max_pt = intersection;
		}
	}

	QLineF res(min_pt, max_pt);
	ensureSameDirection(raw_line, res);
	return res;
}

QPolygonF
PageLayout::singlePageOutline() const
{
	if (m_uncutOutline.size() < 4) {
		return QPolygonF();
	}

	switch (m_type) {
		case SINGLE_PAGE_UNCUT:
			return m_uncutOutline;
		case SINGLE_PAGE_CUT:
			break;
		case TWO_PAGES:
			return QPolygonF();
	}

	QLineF const line1(extendToCover(m_cutter1, m_uncutOutline));
	QLineF line2(extendToCover(m_cutter2, m_uncutOutline));
	ensureSameDirection(line1, line2);
	QLineF const reverse_line1(line1.p2(), line1.p1());
	QLineF const reverse_line2(line2.p2(), line2.p1());
	
	QPolygonF poly;
	poly << line1.p1();
	maybeAddIntersectionPoint(poly, line1.normalVector(), line2.normalVector());
	poly << line2.p1() << line2.p2();
	maybeAddIntersectionPoint(poly, reverse_line1.normalVector(), reverse_line2.normalVector());
	poly << line1.p2();

	return PolygonUtils::round(m_uncutOutline).intersected(PolygonUtils::round(poly));
}

QPolygonF
PageLayout::leftPageOutline() const
{
	if (m_uncutOutline.size() < 4) {
		return QPolygonF();
	}

	switch (m_type) {
		case SINGLE_PAGE_UNCUT:
		case SINGLE_PAGE_CUT:
			return QPolygonF();
		case TWO_PAGES:
			break;
	}

	QLineF const line1(m_uncutOutline[0], m_uncutOutline[3]);
	QLineF line2(extendToCover(m_cutter1, m_uncutOutline));
	ensureSameDirection(line1, line2);
	QLineF const reverse_line1(line1.p2(), line1.p1());
	QLineF const reverse_line2(line2.p2(), line2.p1());
	
	QPolygonF poly;
	poly << line1.p1();
	maybeAddIntersectionPoint(poly, line1.normalVector(), line2.normalVector());
	poly << line2.p1() << line2.p2();
	maybeAddIntersectionPoint(poly, reverse_line1.normalVector(), reverse_line2.normalVector());
	poly << line1.p2();

	return PolygonUtils::round(m_uncutOutline).intersected(PolygonUtils::round(poly));
}

QPolygonF
PageLayout::rightPageOutline() const
{
	if (m_uncutOutline.size() < 4) {
		return QPolygonF();
	}

	switch (m_type) {
		case SINGLE_PAGE_UNCUT:
		case SINGLE_PAGE_CUT:
			return QPolygonF();
		case TWO_PAGES:
			break;
	}

	QLineF const line1(m_uncutOutline[1], m_uncutOutline[2]);
	QLineF line2(extendToCover(m_cutter1, m_uncutOutline));
	ensureSameDirection(line1, line2);
	QLineF const reverse_line1(line1.p2(), line1.p1());
	QLineF const reverse_line2(line2.p2(), line2.p1());
	
	QPolygonF poly;
	poly << line1.p1();
	maybeAddIntersectionPoint(poly, line1.normalVector(), line2.normalVector());
	poly << line2.p1() << line2.p2();
	maybeAddIntersectionPoint(poly, reverse_line1.normalVector(), reverse_line2.normalVector());
	poly << line1.p2();

	return PolygonUtils::round(m_uncutOutline).intersected(PolygonUtils::round(poly));
}

QPolygonF
PageLayout::pageOutline(PageId::SubPage const page) const
{
	switch (page) {
		case PageId::SINGLE_PAGE:
			return singlePageOutline();
		case PageId::LEFT_PAGE:
			return leftPageOutline();
		case PageId::RIGHT_PAGE:
			return rightPageOutline();
	}
	
	assert(!"Unreachable");
	return QPolygonF();
}

PageLayout
PageLayout::transformed(QTransform const& xform) const
{
	return PageLayout(
		xform.map(m_uncutOutline),
		xform.map(m_cutter1), xform.map(m_cutter2), m_type
	);
}

QDomElement
PageLayout::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.setAttribute("type", typeToString(m_type));
	el.appendChild(marshaller.polygonF(m_uncutOutline, "outline"));
	
	int const num_cutters = numCutters();
	if (num_cutters > 0) {
		el.appendChild(marshaller.lineF(m_cutter1, "cutter1"));
		if (num_cutters > 1) { 
			el.appendChild(marshaller.lineF(m_cutter2, "cutter2"));
		}
	}
	
	return el;
}

PageLayout::Type
PageLayout::typeFromString(QString const& str)
{
	if (str == "two-pages") {
		return TWO_PAGES;
	} else if (str == "single-cut") {
		return SINGLE_PAGE_CUT;
	} else { // "single-uncut"
		return SINGLE_PAGE_UNCUT;
	}
}

QString
PageLayout::typeToString(Type const type)
{
	char const* str = 0;
	switch (type) {
		case SINGLE_PAGE_UNCUT:
			str = "single-uncut";
			break;
		case SINGLE_PAGE_CUT:
			str = "single-cut";
			break;
		case TWO_PAGES:
			str = "two-pages";
			break;
	}
	
	return QString::fromAscii(str);
}

/**
 * Extends or shrinks a line segment in such a way that if you draw perpendicular
 * lines through its endpoints, the given polygon would be squeezed between these
 * two perpendiculars.  This ensures that the resulting line segment intersects
 * all the polygon edges it can possibly intersect.
 */
QLineF
PageLayout::extendToCover(QLineF const& line, QPolygonF const& poly)
{
	if (poly.isEmpty()) {
		return line;
	}

	// Project every vertex of the polygon onto the line and take extremas.

	double min = NumericTraits<double>::max();
	double max = NumericTraits<double>::min();
	ToLineProjector const projector(line);

	BOOST_FOREACH(QPointF const& pt, poly) {
		double const scalar = projector.projectionScalar(pt);
		if (scalar < min) {
			min = scalar;
		}
		if (scalar > max) {
			max = scalar;
		}
	}

	return QLineF(line.pointAt(min), line.pointAt(max));
}

/**
 * Flips \p line2 if that would make the angle between the two lines more acute.
 * The angle between lines is interpreted as an angle between vectors
 * (line1.p2() - line1.p1()) and (line2.p2() - line2.p1()).
 */
void
PageLayout::ensureSameDirection(QLineF const& line1, QLineF& line2)
{
	QPointF const v1(line1.p2() - line1.p1());
	QPointF const v2(line2.p2() - line2.p1());
	double const dot = v1.x() * v2.x() + v1.y() * v2.y();
	if (dot < 0.0) {
		line2 = QLineF(line2.p2(), line2.p1());
	}
}

/**
 * Add the intersection point between \p line1 and \p line2
 * to \p poly, provided they intersect at all and the intersection
 * point is "between" line1.p1() and line2.p1().  We consider a point
 * to be between two other points by projecting it to the line between
 * those two points and checking if the projected point is between them.
 * When finding the intersection point, we treat \p line1 and \p line2
 * as lines, not line segments.
 */
void
PageLayout::maybeAddIntersectionPoint(
	QPolygonF& poly, QLineF const& line1, QLineF const& line2)
{
	QPointF intersection;
	if (line1.intersect(line2, &intersection) == QLineF::NoIntersection) {
		return;
	}

	ToLineProjector const projector(QLineF(line1.p1(), line2.p1()));
	double const p = projector.projectionScalar(intersection);
	if (p > 0.0 && p < 1.0) {
		poly << intersection;
	}
}

} // namespace page_split
