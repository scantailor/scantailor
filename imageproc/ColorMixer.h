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

#ifndef IMAGEPROC_COLOR_MIXER_H_
#define IMAGEPROC_COLOR_MIXER_H_

#include <limits>
#include <stdint.h>

namespace imageproc
{

template<typename AccumType>
class GrayColorMixer
{
public:
	GrayColorMixer() : m_accum() {}

	void add(uint8_t gray_level, AccumType weight) {
		m_accum += AccumType(gray_level) * weight;
	}

	uint8_t mix(AccumType total_weight) const {
		return mixImpl<std::numeric_limits<AccumType>::is_integer>(total_weight);
	}
private:
	template<bool IntegerVersion>
	uint8_t mixImpl(AccumType total_weight) const {
		return static_cast<uint8_t>(m_accum / total_weight + AccumType(0.5));
	}

	template<>
	uint8_t mixImpl<true>(AccumType total_weight) const {
		AccumType const half_weight = total_weight >> 1;
		AccumType const mixed = (m_accum + half_weight) / total_weight;
		return static_cast<uint8_t>(mixed);
	}

	AccumType m_accum;
};


template<typename AccumType>
class RgbColorMixer
{
public:
	RgbColorMixer() : m_redAccum(), m_greenAccum(), m_blueAccum() {}
	
	void add(uint32_t rgb, AccumType weight) {
		m_redAccum += AccumType((rgb >> 16) & 0xFF) * weight;
		m_greenAccum += AccumType((rgb >> 8) & 0xFF) * weight;
		m_blueAccum += AccumType(rgb & 0xFF) * weight;
	}
	
	uint32_t mix(AccumType total_weight) const {
		return mixImpl<std::numeric_limits<AccumType>::is_integer>(total_weight);
	}
private:
	template<bool IntegerVersion>
	uint32_t mixImpl(AccumType total_weight) const {
		AccumType const scale = 1 / total_weight;
		uint32_t const r = uint32_t(AccumType(0.5) + m_redAccum * scale);
		uint32_t const g = uint32_t(AccumType(0.5) + m_greenAccum * scale);
		uint32_t const b = uint32_t(AccumType(0.5) + m_blueAccum * scale);
		return (r << 16) | (g << 8) | b;
	};

	template<>
	uint32_t mixImpl<true>(AccumType total_weight) const {
		AccumType const half_weight = total_weight >> 1;
		uint32_t const r = uint32_t((m_redAccum + half_weight) / total_weight);
		uint32_t const g = uint32_t((m_greenAccum + half_weight) / total_weight);
		uint32_t const b = uint32_t((m_blueAccum + half_weight) / total_weight);
		return (r << 16) | (g << 8) | b;
	};

	AccumType m_redAccum;
	AccumType m_greenAccum;
	AccumType m_blueAccum;
};


template<typename AccumType>
class ArgbColorMixer
{
public:
	ArgbColorMixer() : m_alphaAccum(), m_redAccum(), m_greenAccum(), m_blueAccum() {}
	
	void add(uint32_t argb, AccumType weight) {
		m_alphaAccum += AccumType((argb >> 24) & 0xFF) * weight;
		m_redAccum += AccumType((argb >> 16) & 0xFF) * weight;
		m_greenAccum += AccumType((argb >> 8) & 0xFF) * weight;
		m_blueAccum += AccumType(argb & 0xFF) * weight;
	}
	
	uint32_t mix(AccumType total_weight) const {
		return mixImpl<std::numeric_limits<AccumType>::is_integer>(total_weight);
	}
private:
	template<bool IntegerVersion>
	uint32_t mixImpl(AccumType total_weight) const {
		AccumType const scale = 1 / total_weight;
		uint32_t const a = uint32_t(AccumType(0.5) + m_alphaAccum * scale);
		uint32_t const r = uint32_t(AccumType(0.5) + m_redAccum * scale);
		uint32_t const g = uint32_t(AccumType(0.5) + m_greenAccum * scale);
		uint32_t const b = uint32_t(AccumType(0.5) + m_blueAccum * scale);
		return (a << 24) | (r << 16) | (g << 8) | b;
	};

	template<>
	uint32_t mixImpl<true>(AccumType total_weight) const {
		AccumType const half_weight = total_weight >> 1;
		uint32_t const a = uint32_t((m_alphaAccum + half_weight) / total_weight);
		uint32_t const r = uint32_t((m_redAccum + half_weight) / total_weight);
		uint32_t const g = uint32_t((m_greenAccum + half_weight) / total_weight);
		uint32_t const b = uint32_t((m_blueAccum + half_weight) / total_weight);
		return (a << 24) | (r << 16) | (g << 8) | b;
	};

	AccumType m_alphaAccum;
	AccumType m_redAccum;
	AccumType m_greenAccum;
	AccumType m_blueAccum;
};

} // namespace imageproc

#endif
