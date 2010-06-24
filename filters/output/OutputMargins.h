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

#ifndef OUTPUT_OUTPUT_MARGINS_H_
#define OUTPUT_OUTPUT_MARGINS_H_

#include "Margins.h"

namespace output
{

/**
 * Having margins on the Output stage is useful when creating zones
 * that are meant to cover a corner or an edge of a page.
 * We use the same margins on all tabs to preserve their geometrical
 * one-to-one relationship.
 */
class OutputMargins : public Margins
{
public:
	OutputMargins() : Margins(10.0, 10.0, 10.0, 10.0) {}
};

} // namespace output

#endif
