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

#ifndef CONTENTSPANFINDER_H_
#define CONTENTSPANFINDER_H_

#include "Span.h"
#include "VirtualFunction.h"

namespace imageproc
{
	class SlicedHistogram;
}

class ContentSpanFinder
{
	// Member-wise copying is OK.
public:
	ContentSpanFinder() : m_minContentWidth(1), m_minWhitespaceWidth(1) {}
	
	void setMinContentWidth(int value) { m_minContentWidth = value; }
	
	void setMinWhitespaceWidth(int value) { m_minWhitespaceWidth = value; }
	
	/**
	 * \brief Find content spans.
	 *
	 * Note that content blocks shorter than min-content-width are still
	 * allowed to merge with other content blocks, if whitespace that
	 * separates them is shorter than min-whitespace-width.
	 */
	template<typename T>
	void find(imageproc::SlicedHistogram const& histogram, T handler) const;
private:
	void findImpl(
		imageproc::SlicedHistogram const& histogram,
		VirtualFunction1<void, Span>& handler) const;
	
	int m_minContentWidth;
	int m_minWhitespaceWidth;
};


template<typename T>
void
ContentSpanFinder::find(imageproc::SlicedHistogram const& histogram, T handler) const
{
	ProxyFunction1<T, void, Span> proxy(handler);
	findImpl(histogram, proxy);
}

#endif
