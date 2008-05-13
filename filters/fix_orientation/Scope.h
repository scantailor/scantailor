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

#ifndef FIX_ORIENTATION_SCOPE_H_
#define FIX_ORIENTATION_SCOPE_H_

namespace fix_orientation
{

class Scope
{
public:
	Scope(int from, int to, int origin, int step)
	: m_from(from), m_to(to), m_origin(origin), m_step(step) {}
	
	int from() const { return m_from; }
	
	int to() const { return m_to; }
	
	int origin() const { return m_origin; }
	
	int step() const { return m_step; }
private:
	int m_from; // zero based, inclusive
	int m_to; // zero based, exclusive
	int m_origin; // The page that is in.
	int m_step; // 1 or 2
};

} // namespace fix_orientation

#endif
