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

#ifndef ORTHOGONALROTATION_H_
#define ORTHOGONALROTATION_H_

class QSize;
class QSizeF;
class QPointF;
class QTransform;

class OrthogonalRotation
{
public:
	OrthogonalRotation() : m_degrees(0) {}
	
	int toDegrees() const { return m_degrees; }

	void nextClockwiseDirection();
	
	void prevClockwiseDirection();
	
	QSize rotate(QSize const& dimensions) const;
	
	QSize unrotate(QSize const& dimensions) const;
	
	QSizeF rotate(QSizeF const& dimensions) const;
	
	QSizeF unrotate(QSizeF const& dimensions) const;
	
	QPointF rotate(QPointF const& point, double xmax, double ymax) const;
	
	QPointF unrotate(QPointF const& point, double xmax, double ymax) const;
	
	QTransform transform(QSizeF const& dimensions) const;
private:
	int m_degrees;
};

inline bool operator==(OrthogonalRotation const& lhs, OrthogonalRotation const& rhs)
{
	return lhs.toDegrees() == rhs.toDegrees();
}

inline bool operator!=(OrthogonalRotation const& lhs, OrthogonalRotation const& rhs)
{
	return lhs.toDegrees() != rhs.toDegrees();
}

#endif
