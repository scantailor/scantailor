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

#include "ProjectWriter.h"
#include "PageSequence.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "ImageMetadata.h"
#include "AbstractFilter.h"
#include <QtXml>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <stddef.h>
#include <assert.h>

ProjectWriter::ProjectWriter(
	QString const& out_dir, IntrusivePtr<PageSequence> const& page_sequence)
:	m_outDir(out_dir),
	m_pages(page_sequence->snapshot(PageSequence::PAGE_VIEW)),
	m_layoutDirection(page_sequence->layoutDirection())
{
	int next_id = 1;
	size_t const num_pages = m_pages.numPages();
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page = m_pages.pageAt(i);
		PageId const& page_id = page.id();
		ImageId const& image_id = page_id.imageId();
		QString const& file_path = image_id.filePath();
		QFileInfo const file_info(file_path);
		QString const dir_path(file_info.absolutePath());
		
		m_metadataByImage[image_id] = page.metadata();
		
		if (m_dirIds.insert(DirectoryIds::value_type(dir_path, next_id)).second) {
			++next_id;
		}
		
		FileData const file_data(next_id, page.isMultiPageFile());
		if (m_fileIds.insert(FileIds::value_type(file_path, file_data)).second) {
			++next_id;
		}
		
		ImageData const img_data(next_id, page.imageSubPages());
		if (m_imageIds.insert(ImageIds::value_type(image_id, img_data)).second) {
			++next_id;
		}
		
		if (m_pageIds.insert(PageIds::value_type(page_id, next_id)).second) {
			++next_id;
		}
	}
}

ProjectWriter::~ProjectWriter()
{
}

bool
ProjectWriter::write(QString const& file_path, std::vector<FilterPtr> const& filters) const
{
	QDomDocument doc;
	QDomElement root_el(doc.createElement("project"));
	doc.appendChild(root_el);
	root_el.setAttribute("outputDirectory", m_outDir);
	root_el.setAttribute(
		"layoutDirection",
		m_layoutDirection == Qt::LeftToRight ? "LTR" : "RTL"
	);
	
	root_el.appendChild(processDirectories(doc));
	root_el.appendChild(processFiles(doc));
	root_el.appendChild(processImages(doc));
	root_el.appendChild(processPages(doc));
	
	QDomElement filters_el(doc.createElement("filters"));
	root_el.appendChild(filters_el);
	std::vector<FilterPtr>::const_iterator it(filters.begin());
	std::vector<FilterPtr>::const_iterator const end(filters.end());
	for (; it != end; ++it) {
		filters_el.appendChild((*it)->saveSettings(*this, doc));
	}
	
	QFile file(file_path);
	if (file.open(QIODevice::WriteOnly)) {
		QTextStream strm(&file);
		doc.save(strm, 2);
		return true;
	}
	
	return false;
}

QDomElement
ProjectWriter::processDirectories(QDomDocument& doc) const
{
	QDomElement dirs_el(doc.createElement("directories"));
	
	DirectoryIds::const_iterator it(m_dirIds.begin());
	DirectoryIds::const_iterator const end(m_dirIds.end());
	for (; it != end; ++it) {
		QDomElement dir_el(doc.createElement("directory"));
		dir_el.setAttribute("id", it->second);
		dir_el.setAttribute("path", it->first);
		dirs_el.appendChild(dir_el);
	}
	
	return dirs_el;
}

QDomElement
ProjectWriter::processFiles(QDomDocument& doc) const
{
	QDomElement files_el(doc.createElement("files"));
	
	FileIds::const_iterator it(m_fileIds.begin());
	FileIds::const_iterator const end(m_fileIds.end());
	for (; it != end; ++it) {
		QString const& file_path = it->first;
		FileData const& file_data = it->second;
		QFileInfo const file_info(file_path);
		QString const& dir_path = file_info.absolutePath();
		QDomElement file_el(doc.createElement("file"));
		file_el.setAttribute("id", file_data.id);
		file_el.setAttribute("dirId", dirId(dir_path));
		file_el.setAttribute("name", file_info.fileName());
		file_el.setAttribute("multiPage", file_data.multiPageFile ? "1" : "0");
		files_el.appendChild(file_el);
	}
	
	return files_el;
}

QDomElement
ProjectWriter::processImages(QDomDocument& doc) const
{
	QDomElement images_el(doc.createElement("images"));
	
	ImageIds::const_iterator it(m_imageIds.begin());
	ImageIds::const_iterator const end(m_imageIds.end());
	for (; it != end; ++it) {
		ImageId const& image_id = it->first;
		QDomElement image_el(doc.createElement("image"));
		image_el.setAttribute("id", it->second.id);
		image_el.setAttribute("subPages", it->second.numSubPages);
		image_el.setAttribute("fileId", fileId(image_id.filePath()));
		image_el.setAttribute("fileImage", image_id.page());
		writeImageMetadata(doc, image_el, image_id);
		images_el.appendChild(image_el);
	}
	
	return images_el;
}

void
ProjectWriter::writeImageMetadata(
	QDomDocument& doc, QDomElement& image_el, ImageId const& image_id) const
{
	MetadataByImage::const_iterator it(m_metadataByImage.find(image_id));
	assert(it != m_metadataByImage.end());
	ImageMetadata const& metadata = it->second;
	
	QDomElement size_el(doc.createElement("size"));
	size_el.setAttribute("width", metadata.size().width());
	size_el.setAttribute("height", metadata.size().height());
	image_el.appendChild(size_el);
	
	QDomElement dpi_el(doc.createElement("dpi"));
	dpi_el.setAttribute("horizontal", metadata.dpi().horizontal());
	dpi_el.setAttribute("vertical", metadata.dpi().vertical());
	image_el.appendChild(dpi_el);
}

QDomElement
ProjectWriter::processPages(QDomDocument& doc) const
{
	QDomElement pages_el(doc.createElement("pages"));
	
	size_t const cur_page = m_pages.curPageIdx();
	size_t const num_pages = m_pages.numPages();
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page = m_pages.pageAt(i);
		PageId const& page_id = page.id();
		QDomElement page_el(doc.createElement("page"));
		page_el.setAttribute("id", pageId(page_id));
		page_el.setAttribute("imageId", imageId(page_id.imageId()));
		page_el.setAttribute("subPage", page_id.subPageAsString());
		if (cur_page == i) {
			page_el.setAttribute("selected", "selected");
		}
		pages_el.appendChild(page_el);
	}
	
	return pages_el;
}

int
ProjectWriter::dirId(QString const& dir_path) const
{
	DirectoryIds::const_iterator it(m_dirIds.find(dir_path));
	assert(it != m_dirIds.end());
	return it->second;
}

int
ProjectWriter::fileId(QString const& file_path) const
{
	FileIds::const_iterator it(m_fileIds.find(file_path));
	assert(it != m_fileIds.end());
	return it->second.id;
}

int
ProjectWriter::imageId(ImageId const& image_id) const
{
	ImageIds::const_iterator it(m_imageIds.find(image_id));
	assert(it != m_imageIds.end());
	return it->second.id;
}

int
ProjectWriter::pageId(PageId const& page_id) const
{
	PageIds::const_iterator it(m_pageIds.find(page_id));
	assert(it != m_pageIds.end());
	return it->second;
}

void
ProjectWriter::enumImagesImpl(VirtualFunction2<void, ImageId const&, int>& out) const
{
	ImageIds::const_iterator it(m_imageIds.begin());
	ImageIds::const_iterator const end(m_imageIds.end());
	for (; it != end; ++it) {
		out(it->first, it->second.id);
	}
}

void
ProjectWriter::enumPagesImpl(VirtualFunction2<void, PageId const&, int>& out) const
{
	PageIds::const_iterator it(m_pageIds.begin());
	PageIds::const_iterator const end(m_pageIds.end());
	for (; it != end; ++it) {
		out(it->first, it->second);
	}
}
