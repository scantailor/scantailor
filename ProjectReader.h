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

#ifndef PROJECTREADER_H_
#define PROJECTREADER_H_

#include "ImageId.h"
#include "PageId.h"
#include "ImageInfo.h"
#include "ImageMetadata.h"
#include "SelectedPage.h"
#include "IntrusivePtr.h"
#include <QString>
#include <QDomDocument>
#include <Qt>
#include <vector>
#include <map>

class QDomElement;
class ProjectData;
class ProjectPages;
class FileNameDisambiguator;
class AbstractFilter;

class ProjectReader
{
public:
	typedef IntrusivePtr<AbstractFilter> FilterPtr;
	
	ProjectReader(QDomDocument const& doc);
	
	~ProjectReader();
	
	void readFilterSettings(std::vector<FilterPtr> const& filters) const;
	
	bool success() const { return m_ptrPages.get() != 0; }
	
	QString const& outputDirectory() const { return m_outDir; }
	
	IntrusivePtr<ProjectPages> const& pages() const { return m_ptrPages; }

	SelectedPage const& selectedPage() const { return m_selectedPage; }

	IntrusivePtr<FileNameDisambiguator> const& namingDisambiguator() const {
		return m_ptrDisambiguator;
	}
	
	ImageId imageId(int numeric_id) const;
	
	PageId pageId(int numeric_id) const;
private:
	struct FileRecord
	{
		QString filePath;
		bool compatMultiPage; // Backwards compatibility.

		FileRecord() : compatMultiPage(false) {}

		FileRecord(QString const& file_path, bool compat_multi_page)
			: filePath(file_path), compatMultiPage(compat_multi_page) {}
	};

	typedef std::map<int, QString> DirMap;
	typedef std::map<int, FileRecord> FileMap;
	typedef std::map<int, ImageInfo> ImageMap;
	typedef std::map<int, PageId> PageMap;
	
	void processDirectories(QDomElement const& dirs_el);
	
	void processFiles(QDomElement const& files_el);
	
	void processImages(QDomElement const& images_el,
		Qt::LayoutDirection layout_direction);
	
	ImageMetadata processImageMetadata(QDomElement const& image_el);
	
	void processPages(QDomElement const& pages_el);
	
	QString getDirPath(int id) const;
	
	FileRecord getFileRecord(int id) const;

	QString expandFilePath(QString const& path_shorthand) const;
	
	ImageInfo getImageInfo(int id) const;
	
	QDomDocument m_doc;
	QString m_outDir;
	DirMap m_dirMap;
	FileMap m_fileMap;
	ImageMap m_imageMap;
	PageMap m_pageMap;
	SelectedPage m_selectedPage;
	IntrusivePtr<ProjectPages> m_ptrPages;
	IntrusivePtr<FileNameDisambiguator> m_ptrDisambiguator;
};

#endif
