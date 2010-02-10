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

#include "DespeckleLevel.h"
#include <QString>

namespace output
{

QString despeckleLevelToString(DespeckleLevel const level)
{
	switch (level) {
		case DESPECKLE_OFF:
			return "off";
		case DESPECKLE_CAUTIOUS:
			return "cautious";
		case DESPECKLE_NORMAL:
			return "normal";
		case DESPECKLE_AGGRESSIVE:
			return "aggressive";
	}

	return QString();
}

DespeckleLevel despeckleLevelFromString(QString const& str)
{
	if (str == "off") {
		return DESPECKLE_OFF;
	} else if (str == "cautious") {
		return DESPECKLE_CAUTIOUS;
	} else if (str == "aggressive") {
		return DESPECKLE_AGGRESSIVE;
	} else {
		return DESPECKLE_NORMAL;
	}
}

} // namespace output
