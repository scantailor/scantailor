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

#ifndef SELECT_CONTENT_PHYS_SIZE_CALC_H_
#define SELECT_CONTENT_PHYS_SIZE_CALC_H_

#include <QTransform>
#include <QSizeF>
#include <QRectF>

class ImageTransformation;

namespace select_content
{

class PhysSizeCalc
{
	// Member-wise copying is OK.
public:
	PhysSizeCalc();

	explicit PhysSizeCalc(ImageTransformation const& xform);

	QSizeF sizeMM(QRectF const& rect_px) const;
private:
	QTransform m_virtToPhys;
};

} // namespace select_content

#endif
