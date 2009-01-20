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
	enum Type {
		SINGLE_PAGE_UNCUT,
		LEFT_PAGE_PLUS_OFFCUT,
		RIGHT_PAGE_PLUS_OFFCUT,
		TWO_PAGES
	};
	
	/**
	 * \brief Constructs a SINGLE_PAGE_UNCUT layout with a null split line.
	 */
	PageLayout();
	
	/**
	 * \brief Construct a page layout.
	 *
	 * \param type The layout type.
	 * \param split_line The line that splits pages or cuts off garbage.
	 *        Endpoints don't matter - they only define a line, not a line
	 *        segment. The split line may not be null.
	 */
	PageLayout(Type type, QLineF const& split_line);
	
	/**
	 * \brief Construct a page layout based on XML data.
	 */
	PageLayout(QDomElement const& layout_el);
	
	static PageLayout singlePageUncut();
	
	static PageLayout leftPagePlusOffcut(QLineF const& split_line);
	
	static PageLayout rightPagePlusOffcut(QLineF const& split_line);
	
	static PageLayout twoPages(QLineF const& split_line);
	
	Type type() const { return m_type; }
	
	/**
	 * \brief Get a split line with arbitrary end points.
	 *
	 * The line may be null, which indicates that a split line
	 * was not found.  leftPageValid() and rightPageValid()
	 * will be false in this case.
	 */
	QLineF const& splitLine() const { return m_splitLine; }
	
	/**
	 * \brief Get the number of pages (1 or 2) for this layout.
	 */
	int numSubPages() const;
	
	/**
	 * \brief Get a split line inscribed into a rectangle.
	 * \return A line where both ends touch enges of a rectangle,
	 *         or null line, if it doesn't pass through the rectangle.
	 */
	QLineF inscribedSplitLine(QRectF const& rect) const;
	
	/**
	 * \brief For single page layouts, return the outline of that page,
	 *        otherwise return QPolygonF().
	 */
	QPolygonF singlePageOutline(QRectF const& rect) const;
	
	/**
	 * \brief Get the outline of the left page, if it exists.
	 */
	QPolygonF leftPageOutline(QRectF const& rect) const;
	
	/**
	 * \brief Get the outline of the right page, if it exists.
	 */
	QPolygonF rightPageOutline(QRectF const& rect) const;
	
	QPolygonF pageOutline(QRectF const& rect, PageId::SubPage page) const;
	
	PageLayout transformed(QTransform const& xform) const;
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	static QLineF inscribeLine(QLineF const& line, QRectF const& rect);
	
	static QLineF intersectLeftRight(QLineF const& line, double x_left, double x_right);
	
	static QLineF intersectTopBottom(QLineF const& line, double y_top, double y_bottom);
	
	static QLineF clipLeftRight(QLineF const& line, double x_left, double x_right);
	
	static QLineF clipTopBottom(QLineF const& line, double y_top, double y_bottom);
	
	static Type typeFromString(QString const& str);
	
	static QString typeToString(Type type);
	
	QLineF m_splitLine;
	Type m_type;
};

#endif
