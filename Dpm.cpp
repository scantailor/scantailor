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

#include "Dpm.h"
#include "Dpi.h"
#include "imageproc/Constants.h"
#include <QImage>
#include <QtGlobal>

using namespace imageproc;

Dpm::Dpm(QSize const size)
:	m_xDpm(size.width()),
	m_yDpm(size.height())
{
}

Dpm::Dpm(Dpi const dpi)
:	m_xDpm(qRound(dpi.horizontal() * constants::DPI2DPM)),
	m_yDpm(qRound(dpi.vertical() * constants::DPI2DPM))
{
}

Dpm::Dpm(QImage const& image)
:	m_xDpm(image.dotsPerMeterX()),
	m_yDpm(image.dotsPerMeterY())
{
}

bool
Dpm::isNull() const
{
	return Dpi(*this).isNull();
}

QSize
Dpm::toSize() const
{
	if (isNull()) {
		return QSize();
	} else {
		return QSize(m_xDpm, m_yDpm);
	}
}

bool
Dpm::operator==(Dpm const& other) const
{
	return m_xDpm == other.m_xDpm && m_yDpm == other.m_yDpm;
}
