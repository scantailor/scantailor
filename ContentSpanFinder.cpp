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

#include "ContentSpanFinder.h"
#include "imageproc/SlicedHistogram.h"

using namespace imageproc;

void
ContentSpanFinder::findImpl(
	SlicedHistogram const& histogram,
	VirtualFunction1<void, Span>& handler) const
{
	int const hist_size = histogram.size();
	
	int i = 0;
	int content_end = -m_minWhitespaceWidth;
	int content_begin = content_end;
	
	for (;;) {
		// Find the next content position.
		for (; i < hist_size; ++i) {
			if (histogram[i] != 0) {
				break;
			}
		}
		
		if (i - content_end >= m_minWhitespaceWidth) {
			// Whitespace is long enough to break the content block.
			
			// Note that content_end is initialized to
			// -m_minWhitespaceWidth to make this test
			// pass on the first content block, in order to avoid
			// growing a non existing content block.
			
			if (content_end - content_begin >= m_minContentWidth) {
				handler(Span(content_begin, content_end));
			}
			
			content_begin = i;
		}
		
		if (i == hist_size) {
			break;
		}
		
		// Find the next whitespace position.
		for (; i < hist_size; ++i) {
			if (histogram[i] == 0) {
				break;
			}
		}
		content_end = i;
	}
	
	if (content_end - content_begin >= m_minContentWidth) {
		handler(Span(content_begin, content_end));
	}
}
