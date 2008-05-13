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

#ifndef SCOPEDINCDEC_H_
#define SCOPEDINCDEC_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

template<class T>
class ScopedIncDec
{
public:
	ScopedIncDec(T& counter) : m_counter(counter) { ++counter; }
	
	~ScopedIncDec() { --m_counter; }
private:
	T& m_counter;
};

#endif
