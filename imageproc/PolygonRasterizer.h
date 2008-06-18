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

#ifndef IMAGEPROC_POLYGONRASTERIZER_H_
#define IMAGEPROC_POLYGONRASTERIZER_H_

#include "BWColor.h"
#include <Qt>
#include <stdint.h>

class QPolygonF;
class QRectF;

namespace imageproc
{

class BinaryImage;

class PolygonRasterizer
{
public:
	static void fill(
		BinaryImage& image, BWColor color,
		QPolygonF const& poly, Qt::FillRule fill_rule);
	
	static void fillExcept(
		BinaryImage& image, BWColor color,
		QPolygonF const& poly, Qt::FillRule fill_rule);
private:
	class Edge;
	class EdgeComponent;
	class EdgeOrderY;
	class EdgeOrderX;
	
	static void clipAndFill(
		BinaryImage& image, BWColor color,
		QPolygonF const& poly, Qt::FillRule fill_rule, bool invert);
	
	static void fillImpl(
		BinaryImage& image, BWColor color,
		QPolygonF const& poly, Qt::FillRule fill_rule,
		QRectF const& bounding_box, bool invert);
	
	static void oddEvenFillLine(
		EdgeComponent const* edges, int num_edges,
		uint32_t* line, uint32_t pattern);
	
	static void windingFillLine(
		EdgeComponent const* edges, int num_edges,
		uint32_t* line, uint32_t pattern, bool invert);
	
	static void fillSegment(
		int x_from, int x_to, uint32_t* line, uint32_t pattern);
};

} // namespace imageproc

#endif
