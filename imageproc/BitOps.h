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

#ifndef IMAGEPROC_BITOPS_H_
#define IMAGEPROC_BITOPS_H_

namespace imageproc
{

namespace detail
{

extern unsigned char const bitCounts[256];
extern unsigned char const reversedBits[256];

template<typename T, int BytesRemaining>
class NonZeroBits
{
public:
	static unsigned char count(T val) {
		return bitCounts[static_cast<unsigned char>(val)]
			+ NonZeroBits<T, BytesRemaining - 1>::count(val >> 8);
	}
};

template<typename T>
class NonZeroBits<T, 1>
{
public:
	static unsigned char count(T val) {
		return bitCounts[static_cast<unsigned char>(val)];
	}
};


template<typename T, int TotalBytes, int Offset, bool Done = false>
struct ReverseBytes
{
	static T result(T const val) {
		int const left_shift = (TotalBytes - Offset - 1) * 8;
		int const right_shift = Offset * 8;
		
		typedef unsigned char Byte;
		Byte const left_byte = static_cast<Byte>(val >> left_shift);
		Byte const right_byte = static_cast<Byte>(val >> right_shift);
		
		T res(
			ReverseBytes<
				T, TotalBytes, Offset + 1,
				Offset == TotalBytes / 2
			>::result(val)
		);
		res |= T(reversedBits[left_byte]) << right_shift;
		res |= T(reversedBits[right_byte]) << left_shift;
		
		return res;
	}
};

template<typename T, int TotalBytes, int Offset>
struct ReverseBytes<T, TotalBytes, Offset, true>
{
	static T result(T) {
		return T();
	}
};

template<typename T>
struct ReverseBytes<T, 1, 0, false>
{
	static T result(T const val) {
		typedef unsigned char Byte;
		return T(reversedBits[static_cast<Byte>(val)]);
	}
};


template<typename T, int STRIPE_LEN, int BITS_DONE = 0, bool HAVE_MORE_BITS = true>
struct StripedMaskMSB1
{
	static T const value =
		(((T(1) << STRIPE_LEN) - 1) << (BITS_DONE + STRIPE_LEN)) |
		StripedMaskMSB1<
			T, STRIPE_LEN, BITS_DONE + STRIPE_LEN * 2,
			BITS_DONE + STRIPE_LEN * 2 < sizeof(T) * 8
		>::value;
};

template<typename T, int STRIPE_LEN, int BITS_DONE>
struct StripedMaskMSB1<T, STRIPE_LEN, BITS_DONE, false>
{
	static T const value = 0;
};


template<typename T, int STRIPE_LEN, int BITS_DONE = 0, bool HAVE_MORE_BITS = true>
struct StripedMaskLSB1
{
	static T const value =
		(((T(1) << STRIPE_LEN) - 1) << BITS_DONE) |
		StripedMaskLSB1<
			T, STRIPE_LEN, BITS_DONE + STRIPE_LEN * 2,
			BITS_DONE + STRIPE_LEN * 2 < sizeof(T) * 8
		>::value;
};

template<typename T, int STRIPE_LEN, int BITS_DONE>
struct StripedMaskLSB1<T, STRIPE_LEN, BITS_DONE, false>
{
	static T const value = 0;
};


template<typename T, int STRIPE_LEN>
struct MostSignificantZeroes
{
	static int reduce(T val, int count) {
		if (T tmp = val & StripedMaskMSB1<T, STRIPE_LEN>::value) {
			val = tmp;
			count -= STRIPE_LEN;
		}
		return MostSignificantZeroes<T, STRIPE_LEN / 2>::reduce(val, count);
	}
};

template<typename T>
struct MostSignificantZeroes<T, 0>
{
	static int reduce(T val, int count) {
		return count - 1;
	}
};


template<typename T, int STRIPE_LEN>
struct LeastSignificantZeroes
{
	static int reduce(T val, int count) {
		if (T tmp = val & StripedMaskLSB1<T, STRIPE_LEN>::value) {
			val = tmp;
			count -= STRIPE_LEN;
		}
		return LeastSignificantZeroes<T, STRIPE_LEN / 2>::reduce(val, count);
	}
};

template<typename T>
struct LeastSignificantZeroes<T, 0>
{
	static int reduce(T val, int count) {
		return count - 1;
	}
};

} // namespace detail


template<typename T>
int countNonZeroBits(T const val)
{
	return detail::NonZeroBits<T, sizeof(T)>::count(val);
}

template<typename T>
T reverseBits(T const val)
{
	return detail::ReverseBytes<T, sizeof(T), 0>::result(val);
}

template<typename T>
int countMostSignificantZeroes(T const val)
{
	static int const total_bits = sizeof(T) * 8;
	int zeroes = total_bits;
	
	if (val) {
		zeroes = detail::MostSignificantZeroes<T, total_bits / 2>::reduce(
			val, zeroes
		);
	}
	
	return zeroes;
}

template<typename T>
int countLeastSignificantZeroes(T const val)
{
	static int const total_bits = sizeof(T) * 8;
	int zeroes = total_bits;
	
	if (val) {
		zeroes = detail::LeastSignificantZeroes<T, total_bits / 2>::reduce(
			val, zeroes
		);
	}
	
	return zeroes;
}

} // namespace imageproc

#endif
