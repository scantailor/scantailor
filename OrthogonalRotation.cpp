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

#include "OrthogonalRotation.h"
#include <QSize>
#include <QSizeF>
#include <QPointF>
#include <QTransform>
#include <assert.h>

void
OrthogonalRotation::nextClockwiseDirection()
{
	m_degrees += 90;
	if (m_degrees == 360) {
		m_degrees = 0;
	}
}

void
OrthogonalRotation::prevClockwiseDirection()
{
	m_degrees -= 90;
	if (m_degrees == -90) {
		m_degrees = 270;
	}
}

QSize
OrthogonalRotation::rotate(QSize const& dimensions) const
{
	if (m_degrees == 90 || m_degrees == 270) {
		return QSize(dimensions.height(), dimensions.width());
	} else {
		return dimensions;
	}
}

QSize
OrthogonalRotation::unrotate(QSize const& dimensions) const
{
	return rotate(dimensions);
}

QSizeF
OrthogonalRotation::rotate(QSizeF const& dimensions) const
{
	if (m_degrees == 90 || m_degrees == 270) {
		return QSizeF(dimensions.height(), dimensions.width());
	} else {
		return dimensions;
	}
}

QSizeF
OrthogonalRotation::unrotate(QSizeF const& dimensions) const
{
	return rotate(dimensions);
}

QPointF
OrthogonalRotation::rotate(
	QPointF const& point, double const xmax, double const ymax) const
{
	QPointF rotated;
	
	switch (m_degrees) {
	case 0:
		rotated = point;
		break;
	case 90:
		rotated.setX(ymax - point.y());
		rotated.setY(point.x());
		break;
	case 180:
		rotated.setX(xmax - point.x());
		rotated.setY(ymax - point.y());
		break;
	case 270:
		rotated.setX(point.y());
		rotated.setY(xmax - point.x());
		break;
	default:
		assert(!"Unreachable");
	}
	
	return rotated;
}

QPointF
OrthogonalRotation::unrotate(
	QPointF const& point, double const xmax, double const ymax) const
{
	QPointF unrotated;
	
	switch (m_degrees) {
	case 0:
		unrotated = point;
		break;
	case 90:
		unrotated.setX(point.y());
		unrotated.setY(xmax - point.x());
		break;
	case 180:
		unrotated.setX(xmax - point.x());
		unrotated.setY(ymax - point.y());
		break;
	case 270:
		unrotated.setX(ymax - point.y());
		unrotated.setY(point.x());
		break;
	default:
		assert(!"Unreachable");
	}
	
	return unrotated;
}

QTransform
OrthogonalRotation::transform(QSizeF const& dimensions) const
{
	QTransform t;
	
	switch (m_degrees) {
	case 0:
		return t;
	case 90:
		t.translate(dimensions.height(), 0);
		break;
	case 180:
		t.translate(dimensions.width(), dimensions.height());
		break;
	case 270:
		t.translate(0, dimensions.width());
		break;
	default:
		assert(!"Unreachable");
	}
	
	t.rotate(m_degrees);
	
	return t;
}
