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

#ifndef OUTPUT_FILE_NAME_GENERATOR_H_
#define OUTPUT_FILE_NAME_GENERATOR_H_

#include "FileNameDisambiguator.h"
#include "IntrusivePtr.h"
#include <QString>
#include <Qt>

class PageId;
class AbstractRelinker;

class OutputFileNameGenerator
{
	// Member-wise copying is OK.
public:
	OutputFileNameGenerator();

	OutputFileNameGenerator(
		IntrusivePtr<FileNameDisambiguator> const& disambiguator,
		QString const& out_dir, Qt::LayoutDirection layout_direction);

	void performRelinking(AbstractRelinker const& relinker);

	Qt::LayoutDirection layoutDirection() const { return m_layoutDirection; }

	QString const& outDir() const { return m_outDir; }

	FileNameDisambiguator* disambiguator() { return m_ptrDisambiguator.get(); }

	FileNameDisambiguator const* disambiguator() const { return m_ptrDisambiguator.get(); }

	QString fileNameFor(PageId const& page) const;

	QString filePathFor(PageId const& page) const;
private:
	IntrusivePtr<FileNameDisambiguator> m_ptrDisambiguator;
	QString m_outDir;
	Qt::LayoutDirection m_layoutDirection;
};

#endif
