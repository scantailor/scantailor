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

#ifndef MARGINS_H_
#define MARGINS_H_

class Margins
{
public:
	Margins() : m_top(), m_bottom(), m_left(), m_right() {}
	
	Margins(double left, double top, double right, double bottom)
	: m_top(top), m_bottom(bottom), m_left(left), m_right(right) {}
	
	double top() const { return m_top; }
	
	void setTop(double val) { m_top = val; }
	
	double bottom() const { return m_bottom; }
	
	void setBottom(double val) { m_bottom = val; }
	
	double left() const { return m_left; }
	
	void setLeft(double val) { m_left = val; }
	
	double right() const { return m_right; }
	
	void setRight(double val) { m_right = val; }
private:
	double m_top;
	double m_bottom;
	double m_left;
	double m_right;
};

#endif
