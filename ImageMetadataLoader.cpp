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

#include "ImageMetadataLoader.h"
#include "ImageMetadata.h"
#include <QString>
#include <QIODevice>
#include <QFile>

ImageMetadataLoader::LoaderList ImageMetadataLoader::m_sLoaders;

void
ImageMetadataLoader::registerLoader(
	IntrusivePtr<ImageMetadataLoader> const& loader)
{
	m_sLoaders.push_back(loader);
}

ImageMetadataLoader::Status
ImageMetadataLoader::loadImpl(
	QIODevice& io_device,
	VirtualFunction1<void, ImageMetadata const&>& out)
{
	LoaderList::iterator it(m_sLoaders.begin());
	LoaderList::iterator const end(m_sLoaders.end());
	for (; it != end; ++it) {
		Status const status = (*it)->loadMetadata(io_device, out);
		if (status != FORMAT_NOT_RECOGNIZED) {
			return status;
		}
	}
	return FORMAT_NOT_RECOGNIZED;
}

ImageMetadataLoader::Status
ImageMetadataLoader::loadImpl(
	QString const& file_path,
	VirtualFunction1<void, ImageMetadata const&>& out)
{
	QFile file(file_path);
	if (!file.open(QIODevice::ReadOnly)) {
		return GENERIC_ERROR;
	}
	return loadImpl(file, out);
}

