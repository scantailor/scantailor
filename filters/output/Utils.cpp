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
#include "PageId.h"
#include "Dpi.h"
#include <QString>
#include <QTransform>
#include <QFileInfo>

namespace output
{

QString
Utils::outFilePath(
	PageId const& page_id, int const page_num, QString const& out_dir)
{
	QString const base_file_name(
		QFileInfo(page_id.imageId().filePath()).completeBaseName()
	);
	
	QString const padded_number(
		QString::fromAscii("%1").arg(
			QString::number(page_num + 1), 4, QChar('0')
		)
	);
	
	QString const out_path(
		QString::fromAscii("%1/%2_%3.png").arg(
			out_dir, padded_number, base_file_name
		)
	);
	
	return out_path;
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
