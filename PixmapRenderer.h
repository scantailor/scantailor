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

#ifndef PIXMAPRENDERER_H_
#define PIXMAPRENDERER_H_

class QPainter;
class QPixmap;

class PixmapRenderer
{
public:
	/**
	 * \brief Workarounds some problems with QPainter::drawPixmap().
	 *
	 * This method is more or less equivalent to:
	 * \code
	 * QPainter::drawPixmap(0, 0, pixmap);
	 * \endcode
	 * However, there are two problems with the above code:\n
	 * 1. On X11, QPainter doesn't (as of Qt 4.4.0) use XRender if
	 *    a transformation is applied.  We call XRender manually, but
	 *    note that not all features of QPainter are implemented.
	 *    In particular, clipping will be done by the bounding box
	 *    of the requested clip region.\n
	 * 2. On Windows, the above code is very slow if a large zoom is
	 *    specified in QPainter.  To fix that we calculate the region of
	 *    the image to be displayed and pass it to the rendering engine.
	 */
	static void drawPixmap(QPainter& painter, QPixmap const& pixmap);
private:
	static void drawPixmapNoXRender(QPainter& painter, QPixmap const& pixmap);
};

#endif
