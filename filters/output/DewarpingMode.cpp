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

#include "DewarpingMode.h"
#include <assert.h>

namespace output
{

DewarpingMode::DewarpingMode(QString const& str)
{
	if (str == "auto") {
		m_mode = AUTO;
	} else if (str == "manual") {
		m_mode = MANUAL;
	} else {
		m_mode = OFF;
	}
}
	
QString
DewarpingMode::toString() const
{
	switch (m_mode) {
		case OFF:
			return "off";
		case AUTO:
			return "auto";
		case MANUAL:
			return "manual";
	}

	assert(!"Unreachable");
	return QString();
}

} // namespace output
