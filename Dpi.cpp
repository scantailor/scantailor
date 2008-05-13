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

#include "Dpi.h"
#include "Dpm.h"
#include "imageproc/Constants.h"
#include <QtGlobal>

using namespace imageproc;

Dpi::Dpi(QSize const size)
:	m_xDpi(size.width()),
	m_yDpi(size.height())
{
}

Dpi::Dpi(Dpm const dpm)
:	m_xDpi(qRound(dpm.horizontal() * constants::DPM2DPI)),
	m_yDpi(qRound(dpm.vertical() * constants::DPM2DPI))
{
}

QSize
Dpi::toSize() const
{
	if (isNull()) {
		return QSize();
	} else {
		return QSize(m_xDpi, m_yDpi);
	}
}

bool
Dpi::operator==(Dpi const& other) const
{
	return m_xDpi == other.m_xDpi && m_yDpi == other.m_yDpi;
}
