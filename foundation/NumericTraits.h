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

#ifndef NUMERIC_TRAITS_H_
#define NUMERIC_TRAITS_H_

#include <limits>

namespace numeric_traits_impl
{

template<typename T, bool IsInteger> struct IntegerSpecific;

} // namespace numeric_traits_impl

/**
 * This class exists mainly because std::numeric_values<>::min() has
 * inconsistent behaviour for integer vs floating point types.
 */
template<typename T>
class NumericTraits
{
public:
	static T max() { return std::numeric_limits<T>::max(); }

	/**
	 * This one behaves as you expect, not as std::numeric_limits<T>::min().
	 * That is, this one will actually give you a negative value both for
	 * integer and floating point types.
	 */
	static T min() {
		return numeric_traits_impl::IntegerSpecific<
			T, std::numeric_limits<T>::is_integer
		>::min();
	}
private:
	
};


namespace numeric_traits_impl
{

template<typename T>
struct IntegerSpecific<T, true>
{
	static T min() { return std::numeric_limits<T>::min(); }
};

template<typename T>
struct IntegerSpecific<T, false>
{
	static T min() { return -std::numeric_limits<T>::max(); }
};

} // namespace numeric_traits_impl

#endif
