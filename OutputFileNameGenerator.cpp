/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include <QFileInfo>
#include <QDir>
#include <assert.h>

OutputFileNameGenerator::OutputFileNameGenerator()
:	m_ptrDisambiguator(new FileNameDisambiguator),
	m_outDir(),
	m_layoutDirection(Qt::LeftToRight)
{
}

OutputFileNameGenerator::OutputFileNameGenerator(
	IntrusivePtr<FileNameDisambiguator> const& disambiguator,
	QString const& out_dir, Qt::LayoutDirection layout_direction)
:	m_ptrDisambiguator(disambiguator),
	m_outDir(out_dir),
	m_layoutDirection(layout_direction)
{
	assert(m_ptrDisambiguator.get());
}

void
OutputFileNameGenerator::performRelinking(AbstractRelinker const& relinker)
{
	m_ptrDisambiguator->performRelinking(relinker);
	m_outDir = relinker.substitutionPathFor(RelinkablePath(m_outDir, RelinkablePath::Dir));
}

QString
OutputFileNameGenerator::fileNameFor(PageId const& page) const
{
	bool const ltr = (m_layoutDirection == Qt::LeftToRight);
	PageId::SubPage const sub_page = page.subPage();
	int const label = m_ptrDisambiguator->getLabel(page.imageId().filePath());

	QString name(QFileInfo(page.imageId().filePath()).completeBaseName());
	if (label != 0) {
		name += QString::fromAscii("(%1)").arg(label);
	}
	if (page.imageId().isMultiPageFile()) {
		name += QString::fromAscii("_page%1").arg(
			page.imageId().page(), 4, 10, QLatin1Char('0')
		);
	}
	if (sub_page != PageId::SINGLE_PAGE) {
		name += QLatin1Char('_');
		name += QLatin1Char(ltr == (sub_page == PageId::LEFT_PAGE) ? '1' : '2');
		name += QLatin1Char(sub_page == PageId::LEFT_PAGE ? 'L' : 'R');
	}
	name += QString::fromAscii(".tif");
	
	return name;
}

QString
OutputFileNameGenerator::filePathFor(PageId const& page) const
{
	QString const file_name(fileNameFor(page));
	return QDir(m_outDir).absoluteFilePath(file_name);
}
