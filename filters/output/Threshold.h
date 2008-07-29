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

#ifndef OUTPUT_THRESHOLDTYPE_H_
#define OUTPUT_THRESHOLDTYPE_H_

namespace output
{

class ThresholdType
{
public:
	enum Method { OTSU, SAUVOLA, WOLF };
	
	ThresholdType() : m_method(OTSU) {}
	
	Method method() const { return m_method; }
	
	void setMethod(Method method) { m_method = method; }
private:
	Method m_method;
};

} // namespace output

#endif
