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

#ifndef PAGE_LAYOUT_PARAMS_H_
#define PAGE_LAYOUT_PARAMS_H_

#include "Margins.h"
#include "Alignment.h"
#include <QRectF>
#include <QSizeF>

namespace page_layout
{

class Params
{
	// Member-wise copying is OK.
public:
	Params(Margins const& hard_margins_mm, QRectF const& content_rect,
		QSizeF const& content_size_mm, Alignment const& alignment);
	
	Margins const& hardMarginsMM() const { return m_hardMarginsMM; }
	
	QRectF const& contentRect() const { return m_contentRect; }
	
	QSizeF const& contentSizeMM() const { return m_contentSizeMM; }
	
	Alignment const& alignment() const { return m_alignment; }
private:
	Margins m_hardMarginsMM;
	QRectF m_contentRect;
	QSizeF m_contentSizeMM;
	Alignment m_alignment;
};

} // namespace page_layout

#endif
