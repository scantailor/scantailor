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

#include "Utils.h"
#include "Dpi.h"
#include <QString>
#include <QTransform>
#include <QDir>

namespace output
{

QString
Utils::automaskDir(QString const& out_dir)
{
	return QDir(out_dir).absoluteFilePath("cache/automask");
}

QString
Utils::predespeckleDir(QString const& out_dir)
{
	return QDir(out_dir).absoluteFilePath("cache/predespeckle");
}

QString
Utils::specklesDir(QString const& out_dir)
{
	return QDir(out_dir).absoluteFilePath("cache/speckles");
}

QTransform
Utils::scaleFromToDpi(Dpi const& from, Dpi const& to)
{
	QTransform xform;
	xform.scale(
		(double)to.horizontal() / from.horizontal(),
		(double)to.vertical() / from.vertical()
	);
	return xform;
}

} // namespace output
