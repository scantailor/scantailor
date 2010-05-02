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

#include "Utils.h"
#include "ImageId.h"
#include "PageId.h"
#include "PageInfo.h"
#include "Dpi.h"
#include <QString>
#include <QTransform>
#include <QFileInfo>
#include <QDir>

namespace output
{

QString
Utils::outFilePath(PageInfo const& page_info,
				   Qt::LayoutDirection layout_direction, QString const& out_dir)
{
	QString const base_file_name(
		QFileInfo(page_info.imageId().filePath()).completeBaseName()
	);
	
	bool const ltr = (layout_direction == Qt::LeftToRight);
	PageId::SubPage const sub_page = page_info.id().subPage();

	QString path(out_dir);
	path += QLatin1Char('/');
	path += base_file_name;
	if (page_info.isMultiPageFile()) {
		path += QString::fromAscii("_page_");
		path += QString::fromAscii("%1").arg(
			page_info.imageId().page()+1, 4, 10, QLatin1Char('0')
		);
	}
	if (sub_page != PageId::SINGLE_PAGE) {
		path += QLatin1Char('_');
		path += QLatin1Char(ltr == (sub_page == PageId::LEFT_PAGE) ? '1' : '2');
		path += QLatin1Char(sub_page == PageId::LEFT_PAGE ? 'L' : 'R');
	}
	path += QString::fromAscii(".tiff");
	
	return path;
}

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
