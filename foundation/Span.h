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

#ifndef SPAN_H_
#define SPAN_H_

/**
 * \brief Represents a [from, to) range in one-dimensional space.
 */
class Span
{
public:
	/**
	 * \brief Constructs an empty span.
	 */
	Span() : m_begin(0), m_end(0) {}
	
	/**
	 * \brief Constructs a [begin, end) span.
	 */
	Span(int begin, int end) : m_begin(begin), m_end(end) {}
	
	/**
	 * \brief Constructs a span between a point and another span.
	 */
	Span(int begin, Span const& end)
	: m_begin(begin), m_end(end.begin()) {}
	
	/**
	 * \brief Constructs a span between another span and a point.
	 */
	Span(Span const& begin, int end)
	: m_begin(begin.end()), m_end(end) {}
	
	/**
	 * \brief Constructs a span between two other spans.
	 */
	Span(Span const& begin, Span const& end)
	: m_begin(begin.end()), m_end(end.begin()) {}
	
	int begin() const { return m_begin; }
	
	int end() const { return m_end; }
	
	int width() const { return m_end - m_begin; }
	
	double center() const { return 0.5 * (m_begin + m_end); }
	
	bool operator==(Span const& other) const {
		return m_begin == other.m_begin && m_end == other.m_end;
	}
	
	bool operator!=(Span const& other) const {
		return m_begin != other.m_begin || m_end != other.m_end;
	}
	
	Span& operator+=(int offset) {
		m_begin += offset;
		m_end += offset;
		return *this;
	}
	
	Span& operator-=(int offset) {
		m_begin -= offset;
		m_end -= offset;
		return *this;
	}
	
	Span operator+(int offset) const {
		Span span(*this);
		span += offset;
		return span;
	}
	
	Span operator-(int offset) const {
		Span span(*this);
		span -= offset;
		return span;
	}
private:
	int m_begin;
	int m_end;
};

#endif
