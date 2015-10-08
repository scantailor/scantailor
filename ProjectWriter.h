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

#ifndef PROJECTWRITER_H_
#define PROJECTWRITER_H_

#include "IntrusivePtr.h"
#include "PageSequence.h"
#include "OutputFileNameGenerator.h"
#include "ImageId.h"
#include "PageId.h"
#include "SelectedPage.h"
#include "VirtualFunction.h"
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#endif
#include <QString>
#include <Qt>
#include <vector>
#include <map>

class AbstractFilter;
class ProjectPages;
class PageInfo;
class QDomDocument;
class QDomElement;

class ProjectWriter
{
	DECLARE_NON_COPYABLE(ProjectWriter)
public:
	typedef IntrusivePtr<AbstractFilter> FilterPtr;
	
	ProjectWriter(
		IntrusivePtr<ProjectPages> const& page_sequence,
		SelectedPage const& selected_page,
		OutputFileNameGenerator const& out_file_name_gen);
	
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
	struct Directory
	{
		QString path;
		int numericId;
		
		Directory(QString const& path, int numeric_id)
		: path(path), numericId(numeric_id) {}
	};
	
	struct File
	{
		QString path;
		int numericId;
		
		File(QString const& path, int numeric_id)
		: path(path), numericId(numeric_id) {}
	};
	
	struct Image
	{
		ImageId id;
		int numericId;
		int numSubPages;
		bool leftHalfRemoved;
		bool rightHalfRemoved;
		
		Image(PageInfo const& page_info, int numeric_id);
	};
	
	struct Page
	{
		PageId id;
		int numericId;
		
		Page(PageId const& id, int numeric_id)
		: id(id), numericId(numeric_id) {}
	};
	
	class Sequenced;
	
	typedef std::map<ImageId, ImageMetadata> MetadataByImage;
	
	typedef boost::multi_index::multi_index_container<
		Directory,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<
				boost::multi_index::member<Directory, QString, &Directory::path>
			>,
			boost::multi_index::sequenced<boost::multi_index::tag<Sequenced> >
		>
	> Directories;
	
	typedef boost::multi_index::multi_index_container<
		File,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<
				boost::multi_index::member<File, QString, &File::path>
			>,
			boost::multi_index::sequenced<boost::multi_index::tag<Sequenced> >
		>
	> Files;
	
	typedef boost::multi_index::multi_index_container<
		Image,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<
				boost::multi_index::member<Image, ImageId, &Image::id>
			>,
			boost::multi_index::sequenced<boost::multi_index::tag<Sequenced> >
		>
	> Images;
	
	typedef boost::multi_index::multi_index_container<
		Page,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<
				boost::multi_index::member<Page, PageId, &Page::id>
			>,
			boost::multi_index::sequenced<boost::multi_index::tag<Sequenced> >
		>
	> Pages;
	
	QDomElement processDirectories(QDomDocument& doc) const;
	
	QDomElement processFiles(QDomDocument& doc) const;
	
	QDomElement processImages(QDomDocument& doc) const;
	
	QDomElement processPages(QDomDocument& doc) const;
	
	void writeImageMetadata(
		QDomDocument& doc, QDomElement& image_el,
		ImageId const& image_id) const;
	
	int dirId(QString const& dir_path) const;
	
	int fileId(QString const& file_path) const;

	QString packFilePath(QString const& file_path) const;
	
	int imageId(ImageId const& image_id) const;
	
	int pageId(PageId const& page_id) const;
	
	void enumImagesImpl(VirtualFunction2<void, ImageId const&, int>& out) const;
	
	void enumPagesImpl(VirtualFunction2<void, PageId const&, int>& out) const;
	
	PageSequence m_pageSequence;
	OutputFileNameGenerator m_outFileNameGen;
	SelectedPage m_selectedPage;
	Directories m_dirs;
	Files m_files;
	Images m_images;
	Pages m_pages;
	MetadataByImage m_metadataByImage;
	Qt::LayoutDirection m_layoutDirection;
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
