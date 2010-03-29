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

#include "PerformanceTimer.h"
#include <QDebug>

void
PerformanceTimer::print(char const* prefix)
{
	clock_t const now = clock();
	double const sec = double(now - m_start) / CLOCKS_PER_SEC;
	if (sec > 10.0) {
		qDebug() << prefix << (long)sec << " sec";
	} else if (sec > 0.01) {
		qDebug() << prefix << (long)(sec * 1000) << " msec";
	} else {
		qDebug() << prefix << (long)(sec * 1000000) << " usec";
	}
}
