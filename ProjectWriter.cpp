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
#include <boost/foreach.hpp>
#include <stddef.h>
#include <assert.h>

ProjectWriter::ProjectWriter(
	QString const& out_dir, IntrusivePtr<PageSequence> const& page_sequence)
:	m_outDir(out_dir),
	m_pageSequence(page_sequence->snapshot(PageSequence::PAGE_VIEW)),
	m_layoutDirection(page_sequence->layoutDirection())
{
	int next_id = 1;
	size_t const num_pages = m_pageSequence.numPages();
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page = m_pageSequence.pageAt(i);
		PageId const& page_id = page.id();
		ImageId const& image_id = page_id.imageId();
		QString const& file_path = image_id.filePath();
		QFileInfo const file_info(file_path);
		QString const dir_path(file_info.absolutePath());
		
		m_metadataByImage[image_id] = page.metadata();
		
		if (m_dirs.insert(Directory(dir_path, next_id)).second) {
			++next_id;
		}
		
		if (m_files.insert(File(file_path, next_id, page.isMultiPageFile())).second) {
			++next_id;
		}
		
		if (m_images.insert(Image(image_id, next_id, page.imageSubPages())).second) {
			++next_id;
		}
		
		if (m_pages.insert(Page(page_id, next_id)).second) {
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
	
	BOOST_FOREACH(Directory const& dir, m_dirs.get<Sequenced>()) {
		QDomElement dir_el(doc.createElement("directory"));
		dir_el.setAttribute("id", dir.numericId);
		dir_el.setAttribute("path", dir.path);
		dirs_el.appendChild(dir_el);
	}
	
	return dirs_el;
}

QDomElement
ProjectWriter::processFiles(QDomDocument& doc) const
{
	QDomElement files_el(doc.createElement("files"));
	
	BOOST_FOREACH(File const& file, m_files.get<Sequenced>()) {
		QFileInfo const file_info(file.path);
		QString const& dir_path = file_info.absolutePath();
		QDomElement file_el(doc.createElement("file"));
		file_el.setAttribute("id", file.numericId);
		file_el.setAttribute("dirId", dirId(dir_path));
		file_el.setAttribute("name", file_info.fileName());
		file_el.setAttribute("multiPage", file.multiPageFile ? "1" : "0");
		files_el.appendChild(file_el);
	}
	
	return files_el;
}

QDomElement
ProjectWriter::processImages(QDomDocument& doc) const
{
	QDomElement images_el(doc.createElement("images"));
	
	BOOST_FOREACH(Image const& image, m_images.get<Sequenced>()) {
		QDomElement image_el(doc.createElement("image"));
		image_el.setAttribute("id", image.numericId);
		image_el.setAttribute("subPages", image.numSubPages);
		image_el.setAttribute("fileId", fileId(image.id.filePath()));
		image_el.setAttribute("fileImage", image.id.page());
		writeImageMetadata(doc, image_el, image.id);
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
	
	size_t const cur_page = m_pageSequence.curPageIdx();
	size_t const num_pages = m_pageSequence.numPages();
	for (size_t i = 0; i < num_pages; ++i) {
		PageInfo const& page = m_pageSequence.pageAt(i);
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
	Directories::const_iterator const it(m_dirs.find(dir_path));
	assert(it != m_dirs.end());
	return it->numericId;
}

int
ProjectWriter::fileId(QString const& file_path) const
{
	Files::const_iterator const it(m_files.find(file_path));
	assert(it != m_files.end());
	return it->numericId;
}

int
ProjectWriter::imageId(ImageId const& image_id) const
{
	Images::const_iterator const it(m_images.find(image_id));
	assert(it != m_images.end());
	return it->numericId;
}

int
ProjectWriter::pageId(PageId const& page_id) const
{
	Pages::const_iterator const it(m_pages.find(page_id));
	assert(it != m_pages.end());
	return it->numericId;
}

void
ProjectWriter::enumImagesImpl(VirtualFunction2<void, ImageId const&, int>& out) const
{
	BOOST_FOREACH(Image const& image, m_images.get<Sequenced>()) {
		out(image.id, image.numericId);
	}
}

void
ProjectWriter::enumPagesImpl(VirtualFunction2<void, PageId const&, int>& out) const
{
	BOOST_FOREACH(Page const& page, m_pages.get<Sequenced>()) {
		out(page.id, page.numericId);
	}
}
