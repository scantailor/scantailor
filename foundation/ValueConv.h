/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef VALUE_CONV_H_
#define VALUE_CONV_H_

#include "NumericTraits.h"
#include <math.h>

template<typename ToType>
class StaticCastValueConv
{
public:
	template<typename FromType>
	ToType operator()(FromType val) const {
		return static_cast<ToType>(val);
	}
};

template<typename ToType>
class RoundAndClipValueConv
{
public:
	RoundAndClipValueConv(
		ToType min = NumericTraits<ToType>::min(),
		ToType max = NumericTraits<ToType>::max())
	: m_min(min), m_max(max) {}
	
	template<typename FromType>
	ToType operator()(FromType val) const {
		// To avoid possible "comparing signed to unsigned" warnings,
		// we do the comparison with FromType.  It should be fine, as
		// "Round" in the name of the class assumes it's a floating point type,
		// and therefore should be "wider" than ToType.
		if (val < FromType(m_min)) {
			return m_min;
		} else if (val > FromType(m_max)) {
			return m_max;
		} else {
			return static_cast<ToType>(floor(val + 0.5));
		}
	}
private:
	ToType m_min;
	ToType m_max;
};

#endif
