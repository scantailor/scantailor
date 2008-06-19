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

#ifndef PAGELAYOUT_H_
#define PAGELAYOUT_H_

#include "PageId.h"
#include <QLineF>
#include <QString>

class QPolygonF;
class QRectF;
class QTransform;
class QDomElement;
class QDomDocument;

class PageLayout
{
public:
	/**
	 * \brief Construct a null page layout.
	 */
	PageLayout();
	
	/**
	 * \brief Construct a page layout.
	 *
	 * \param split_line The line that splits pages. Endpoints don't matter.
	 *        They only define the line, not a line segment. The split line
	 *        may be null.  In this case \p left_page_valid  and
	 *        \p right_page_valid must be false.
	 * \param left_page_valid True if there is a page to the left of
	 *        \p split_line.
	 * \param right_page_valid True if there is a page to the right of
	 *        \p split_line.
	 */
	PageLayout(QLineF const& split_line,
		bool left_page_valid, bool right_page_valid);
	
	/**
	 * \brief Construct a page layout based on XML data.
	 */
	PageLayout(QDomElement const& layout_el);
	
	bool isNull() const { return m_splitLine.isNull(); }
	
	/**
	 * \brief Get a split line with arbitrary end points.
	 *
	 * The line may be null, which indicates that a split line
	 * was not found.  leftPageValid() and rightPageValid()
	 * will be false in this case.
	 */
	QLineF const& splitLine() const { return m_splitLine; }
	
	bool leftPageValid() const { return m_leftPageValid; }
	
	bool rightPageValid() const { return m_rightPageValid; }
	
	int numSubPages() const;
	
	/**
	 * \brief Get a split line inscribed into a rectangle.
	 * \return A line where both ends touch enges of a rectangle,
	 *         or null line, if it doesn't pass through the rectangle.
	 */
	QLineF inscribedSplitLine(QRectF const& rect) const;
	
	/**
	 * \brief Get the left page outline, even if the left page is invalid.
	 */
	QPolygonF leftPage(QRectF const& rect) const;
	
	/**
	 * \brief Get the right page outline, even if the right page is invalid.
	 */
	QPolygonF rightPage(QRectF const& rect) const;
	
	QPolygonF pageOutline(QRectF const& rect, PageId::SubPage page) const;
	
	PageLayout transformed(QTransform const& xform) const;
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	static QLineF inscribeLine(QLineF const& line, QRectF const& rect);
	
	static QLineF intersectLeftRight(QLineF const& line, double x_left, double x_right);
	
	static QLineF intersectTopBottom(QLineF const& line, double y_top, double y_bottom);
	
	static QLineF clipLeftRight(QLineF const& line, double x_left, double x_right);
	
	static QLineF clipTopBottom(QLineF const& line, double y_top, double y_bottom);
	
	QLineF m_splitLine;
	bool m_leftPageValid;
	bool m_rightPageValid;
};

#endif
