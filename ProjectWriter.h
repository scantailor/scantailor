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

#ifndef PROJECTWRITER_H_
#define PROJECTWRITER_H_

#include "IntrusivePtr.h"
#include "PageSequence.h"
#include "ImageId.h"
#include "PageId.h"
#include "VirtualFunction.h"
#include <QString>
#include <vector>
#include <map>

class AbstractFilter;
class QDomDocument;
class QDomElement;

class ProjectWriter
{
	DECLARE_NON_COPYABLE(ProjectWriter)
public:
	typedef IntrusivePtr<AbstractFilter> FilterPtr;
	
	ProjectWriter(QString const& out_dir, IntrusivePtr<PageSequence> const& page_sequence);
	
	~ProjectWriter();
	
	bool write(QString const& file_path, std::vector<FilterPtr> const& filters) const;
	
	/**
	 * \p out will be called like this: out(ImageId, numeric_image_id)
	 */
	template<typename OutFunc>
	void enumImages(OutFunc out) const;
	
	/**
	 * \p out will be called like this: out(LogicalPageId, numeric_page_id)
	 */
	template<typename OutFunc>
	void enumPages(OutFunc out) const;
private:
	struct FileData
	{
		int id;
		bool multiPageFile;
		
		FileData(int id, bool multi_page)
		: id(id), multiPageFile(multi_page) {}
	};
	
	struct ImageData
	{
		int id;
		int numSubPages;
		
		ImageData(int id, int num_sub_pages)
		: id(id), numSubPages(num_sub_pages) {}
	};
	
	typedef std::map<QString, int> DirectoryIds;
	typedef std::map<QString, FileData> FileIds;
	typedef std::map<ImageId, ImageData> ImageIds;
	typedef std::map<PageId, int> PageIds;
	typedef std::map<ImageId, ImageMetadata> MetadataByImage;
	
	QDomElement processDirectories(QDomDocument& doc) const;
	
	QDomElement processFiles(QDomDocument& doc) const;
	
	QDomElement processImages(QDomDocument& doc) const;
	
	QDomElement processPages(QDomDocument& doc) const;
	
	void writeImageMetadata(
		QDomDocument& doc, QDomElement& image_el,
		ImageId const& image_id) const;
	
	int dirId(QString const& dir_path) const;
	
	int fileId(QString const& file_path) const;
	
	int imageId(ImageId const& image_id) const;
	
	int pageId(PageId const& page_id) const;
	
	void enumImagesImpl(VirtualFunction2<void, ImageId const&, int>& out) const;
	
	void enumPagesImpl(VirtualFunction2<void, PageId const&, int>& out) const;
	
	QString m_outDir;
	PageSequenceSnapshot m_pages;
	DirectoryIds m_dirIds; // directory -> numeric id
	FileIds m_fileIds; // file path -> numeric id
	ImageIds m_imageIds; // image id -> numeric id
	PageIds m_pageIds; // page id -> numeric id
	MetadataByImage m_metadataByImage;
};

template<typename OutFunc>
void
ProjectWriter::enumImages(OutFunc out) const
{
	ProxyFunction2<OutFunc, void, ImageId const&, int> proxy(out);
	enumImagesImpl(proxy);
}

template<typename OutFunc>
void
ProjectWriter::enumPages(OutFunc out) const
{
	ProxyFunction2<OutFunc, void, PageId const&, int> proxy(out);
	enumPagesImpl(proxy);
}

#endif
