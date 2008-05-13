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

#ifndef IMAGEPROC_GRAYSCALE_H_
#define IMAGEPROC_GRAYSCALE_H_

#include <QVector>
#include <QColor>

class QImage;

namespace imageproc
{

class BinaryImage;

class GrayscaleHistogram
{
public:
	explicit GrayscaleHistogram(QImage const& img);
	
	GrayscaleHistogram(QImage const& img, BinaryImage const& mask);
	
	int& operator[](int idx) { return m_pixels[idx]; }
	
	int operator[](int idx) const { return m_pixels[idx]; }
private:
	void fromMonoImage(QImage const& img);
	
	void fromMonoMSBImage(QImage const& img, BinaryImage const& mask);
	
	void fromGrayscaleImage(QImage const& img);
	
	void fromGrayscaleImage(QImage const& img, BinaryImage const& mask);
	
	void fromAnyImage(QImage const& img);
	
	void fromAnyImage(QImage const& img, BinaryImage const& mask);
	
	int m_pixels[256];
};

QVector<QRgb> createGrayscalePalette();

QImage toGrayscale(QImage const& src);

} // namespace imageproc

#endif
