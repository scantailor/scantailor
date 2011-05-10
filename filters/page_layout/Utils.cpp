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

#include "Utils.h"
#include "Margins.h"
#include "Alignment.h"
#include "Params.h"
#include "ImageTransformation.h"
#include "PhysicalTransformation.h"
#include <QPolygonF>
#include <QPointF>
#include <QLineF>
#include <QSizeF>
#include <QRectF>
#include <assert.h>
#include <iostream>
#include <cmath>

namespace page_layout
{

QRectF
Utils::adaptContentRect(
	ImageTransformation const& xform, QRectF const& content_rect)
{
	if (!content_rect.isEmpty()) {
		return content_rect;
	}
	
	QPointF const center(xform.resultingRect().center());
	QPointF const delta(0.01, 0.01);
	return QRectF(center - delta, center + delta);
}

QSizeF
Utils::calcRectSizeMM(
	ImageTransformation const& xform, QRectF const& rect)
{
	PhysicalTransformation const phys_xform(xform.origDpi());
	QTransform const virt_to_mm(xform.transformBack() * phys_xform.pixelsToMM());
	
	QLineF const hor_line(rect.topLeft(), rect.topRight());
	QLineF const ver_line(rect.topLeft(), rect.bottomLeft());
	
	double const width = virt_to_mm.map(hor_line).length();
	double const height = virt_to_mm.map(ver_line).length();
	return QSizeF(width, height);
}

void
Utils::extendPolyRectWithMargins(
	QPolygonF& poly_rect, Margins const& margins)
{
	QPointF const down_uv(getDownUnitVector(poly_rect));
	QPointF const right_uv(getRightUnitVector(poly_rect));
	
	// top-left
	poly_rect[0] -= down_uv * margins.top();
	poly_rect[0] -= right_uv * margins.left();
	
	// top-right
	poly_rect[1] -= down_uv * margins.top();
	poly_rect[1] += right_uv * margins.right();
	
	// bottom-right
	poly_rect[2] += down_uv * margins.bottom();
	poly_rect[2] += right_uv * margins.right();
	
	// bottom-left
	poly_rect[3] += down_uv * margins.bottom();
	poly_rect[3] -= right_uv * margins.left();
	
	if (poly_rect.size() > 4) {
		assert(poly_rect.size() == 5);
		// This polygon is closed.
		poly_rect[4] = poly_rect[3];
	}
}

Margins
Utils::calcSoftMarginsMM(
	QSizeF const& hard_size_mm, QSizeF const& aggregate_hard_size_mm,
	Alignment const& alignment, QRectF const& resultRect, QRectF const& boundingRect, QRectF const& agg_content_rect)
{
	if (alignment.isNull()) {
		std::cout << "\tskip soft margins: " <<  "\n";
		// This means we are not aligning this page with others.
		return Margins();
	}

#ifdef DEBUG
	std::cout << "left: " << agg_content_rect.left() << " ";
	std::cout << "top: " << agg_content_rect.top() << " ";
	std::cout << "right: " << agg_content_rect.right() << " ";
	std::cout << "bottom: " << agg_content_rect.bottom() << " ";
	std::cout << "\n";

	std::cout << "boundingLeft: " << boundingRect.left() << " ";
	std::cout << "boundingTop: " << boundingRect.top() << " ";
	std::cout << "boundingRight: " << boundingRect.right() << " ";
	std::cout << "boundingBottom: " << boundingRect.bottom() << " ";
	std::cout << "\n";
#endif

	Alignment myAlign = alignment;
	
	double top = 0.0;
	double bottom = 0.0;
	double left = 0.0;
	double right = 0.0;

	double const delta_width =
			aggregate_hard_size_mm.width() - hard_size_mm.width();
	double const delta_height =
			aggregate_hard_size_mm.height() - hard_size_mm.height();

	// detect original borders in page mirror
	double leftBorder = double(boundingRect.left() - agg_content_rect.left()) / double(agg_content_rect.width());
	double rightBorder = double(agg_content_rect.right() - boundingRect.right()) / double(agg_content_rect.width());
	double topBorder = double(boundingRect.top() - agg_content_rect.top()) / double(agg_content_rect.height());
	double bottomBorder = double(agg_content_rect.bottom() - boundingRect.bottom()) / double(agg_content_rect.height());
	
	// borders in new page layout in mm
	double aggLeftBorder = (delta_width / (leftBorder + rightBorder)) * leftBorder;
	double aggRightBorder = delta_width - aggLeftBorder;
	double aggTopBorder = (delta_height / (topBorder + bottomBorder)) * topBorder;
	double aggBottomBorder = delta_height - aggTopBorder;

#ifdef DEBUG
	std::cout << aggLeftBorder << " " << aggRightBorder << " " << aggTopBorder << " " << aggBottomBorder << ":" << "\n";
	std::cout << leftBorder << " " << rightBorder << " " << topBorder << " " << bottomBorder << ":" << "\n";
#endif

	// if align auto and not empty page
	if (boundingRect.width() > 1.0 && (myAlign.horizontal() == Alignment::HAUTO || myAlign.vertical() == Alignment::VAUTO)) {
		// new borders ratio
		double newLeftBorder = aggLeftBorder / aggregate_hard_size_mm.width();
		double newRightBorder = aggRightBorder / aggregate_hard_size_mm.width();
		double newTopBorder = aggTopBorder / aggregate_hard_size_mm.height();
		double newBottomBorder = aggBottomBorder / aggregate_hard_size_mm.height();

		// content mass index
		double cmi = double(boundingRect.width() * boundingRect.height()) / double(agg_content_rect.width() * agg_content_rect.height());

		double hTolerance = myAlign.tolerance();
		double vTolerance = myAlign.tolerance() * 0.7;
		double hShift = newRightBorder - newLeftBorder;
		double habsShift = std::abs(hShift);
		double vShift = newBottomBorder - newTopBorder;
		double vabsShift = std::abs(vShift);

#ifdef DEBUG 
	std::cout << newLeftBorder << " " << newRightBorder << " " << newTopBorder << " " << newBottomBorder << "\n";
	std::cout << vabsShift << " " << habsShift << std::endl;
	std::cout << cmi << std::endl;
		std::cout << hTolerance << " " << vTolerance << "\n";
#endif
		if (myAlign.horizontal() == Alignment::HAUTO) {
			if ((1.0-cmi) < alignment.tolerance()) {
				myAlign.setHorizontal(Alignment::HCENTER);
			} else if (habsShift < hTolerance){
				myAlign.setHorizontal(Alignment::HCENTER);
			} else {
				if (hShift > 0 && newLeftBorder > 1.4*hTolerance) {
					myAlign.setHorizontal(Alignment::RIGHT);
				} else if (hShift <=0 && newRightBorder > 1.4*hTolerance) {
					myAlign.setHorizontal(Alignment::LEFT);
				} else {
					myAlign.setHorizontal(Alignment::HORIGINAL);
				}
			}
		}

		if (myAlign.vertical() == Alignment::VAUTO) {
			if ((1.0-cmi) < alignment.tolerance()) {
				myAlign.setVertical(Alignment::TOP);
			} else if (vabsShift < vTolerance) {
				myAlign.setVertical(Alignment::VCENTER);
			} else {
				if (vShift > 0 && newBottomBorder > 1.4*vTolerance) {
					myAlign.setVertical(Alignment::TOP);
				} else if (vShift <= 0 && newTopBorder > 1.4*vTolerance) {
					myAlign.setVertical(Alignment::BOTTOM);
				} else {
					myAlign.setVertical(Alignment::VORIGINAL);
				}
			}
		}
	}
#ifdef DEBUG
	std::cout << "\n";
#endif

	if (delta_width > 0.0) {
		switch (myAlign.horizontal()) {
			case Alignment::LEFT:
#ifdef DEBUG
				std::cout << "align:left" << std::endl;
#endif
				right = delta_width;
				break;
			case Alignment::HCENTER:
#ifdef DEBUG
				std::cout << "align:hcenter" << std::endl;
#endif
				left = right = 0.5 * delta_width;
				break;
			case Alignment::RIGHT:
#ifdef DEBUG
				std::cout << "align:right" << std::endl;
#endif
				left = delta_width;
				break;
			default:
#ifdef DEBUG
				std::cout << "align:horiginal" << std::endl;
#endif
				left = aggLeftBorder;
				right = aggRightBorder;
				break;
		}
	}

	if (delta_height > 0.0) {
		switch (myAlign.vertical()) {
			case Alignment::TOP:
#ifdef DEBUG
				std::cout << "align:top" << std::endl;
#endif
				bottom = delta_height;
				break;
			case Alignment::VCENTER:
#ifdef DEBUG
				std::cout << "align:vcenter" << std::endl;
#endif
				top = bottom = 0.5 * delta_height;
				break;
			case Alignment::BOTTOM:
#ifdef DEBUG
				std::cout << "align:bottom" << std::endl;
#endif
				top = delta_height;
				break;
			default:
#ifdef DEBUG
				std::cout << "align:voriginal" << std::endl;
#endif
				top = aggTopBorder;
				bottom = aggBottomBorder;
				break;
		}
	}
	
	return Margins(left, top, right, bottom);
}

QPolygonF
Utils::calcPageRectPhys(
	ImageTransformation const& xform, QPolygonF const& content_rect_phys,
	Params const& params, QSizeF const& aggregate_hard_size_mm, QRectF const& agg_content_rect)
{
	PhysicalTransformation const phys_xform(xform.origDpi());
	
	QPolygonF poly_mm(phys_xform.pixelsToMM().map(content_rect_phys));
	extendPolyRectWithMargins(poly_mm, params.hardMarginsMM());
	
	QSizeF const hard_size_mm(
		QLineF(poly_mm[0], poly_mm[1]).length(),
		QLineF(poly_mm[0], poly_mm[3]).length()
	);
	Margins soft_margins_mm(
		calcSoftMarginsMM(
			hard_size_mm, aggregate_hard_size_mm, params.alignment(), xform.origRect(), content_rect_phys.boundingRect(), agg_content_rect
		)
	);

	extendPolyRectWithMargins(poly_mm, soft_margins_mm);
	return phys_xform.mmToPixels().map(poly_mm);
}

QPointF
Utils::getRightUnitVector(QPolygonF const& poly_rect)
{
	QPointF const top_left(poly_rect[0]);
	QPointF const top_right(poly_rect[1]);
	return QLineF(top_left, top_right).unitVector().p2() - top_left;
}

QPointF
Utils::getDownUnitVector(QPolygonF const& poly_rect)
{
	QPointF const top_left(poly_rect[0]);
	QPointF const bottom_left(poly_rect[3]);
	return QLineF(top_left, bottom_left).unitVector().p2() - top_left;
}

} // namespace page_layout
