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

#include "SeedFillGeneric.h"

namespace imageproc
{

namespace detail
{

namespace seed_fill_generic
{

void initHorTransitions(std::vector<HTransition>& transitions, int const width)
{
	transitions.reserve(width);
	
	if (width == 1) {
		// No transitions allowed.
		transitions.push_back(HTransition(0, 0));
		return;
	}
	
	// Only east transition is allowed.
	transitions.push_back(HTransition(0, 1));
	
	for (int i = 1; i < width - 1; ++i) {
		// Both transitions are allowed.
		transitions.push_back(HTransition(-1, 1));
	}
	
	// Only west transition is allowed.
	transitions.push_back(HTransition(-1, 0));
}

void initVertTransitions(std::vector<VTransition>& transitions, int const height)
{
	transitions.reserve(height);
	
	if (height == 1) {
		// No transitions allowed.
		transitions.push_back(VTransition(0, 0));
		return;
	}
	
	// Only south transition is allowed.
	transitions.push_back(VTransition(0, ~0));
	
	for (int i = 1; i < height - 1; ++i) {
		// Both transitions are allowed.
		transitions.push_back(VTransition(~0, ~0));
	}
	
	// Only north transition is allowed.
	transitions.push_back(VTransition(~0, 0));
}

} // namespace seed_fill_generic

} // namespace detail

} // namespace imageproc
