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

#ifndef IMAGEPROC_DENTFINDER_H_
#define IMAGEPROC_DENTFINDER_H_

#include <stdint.h>

namespace imageproc
{

class BinaryImage;

class DentFinder
{
public:
	/**
	 * The idea is to scan all horizontal, vertical, and diagonal lines
	 * and consider every white pixel between the first and the last black
	 * span to belong to a dent or a hole.
	 */
	static imageproc::BinaryImage findDentsAndHoles(
		imageproc::BinaryImage const& src);
private:
	struct ImgInfo;
	
	static void scanHorizontalLines(ImgInfo info);
	
	static void scanVerticalLines(ImgInfo info);
	
	static void scanSlashDiagonals(ImgInfo info);
	
	static void scanBackslashDiagonals(ImgInfo info);
	
	static uint32_t getPixel(uint32_t const* src_line, int x);
	
	static void transferPixel(uint32_t const* src_line, uint32_t* dst_line, int x);
};

} // namespace imageproc

#endif
