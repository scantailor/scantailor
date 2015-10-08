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

#include "ProjectReader.h"
#include "ProjectPages.h"
#include "FileNameDisambiguator.h"
#include "AbstractFilter.h"
#include "XmlUnmarshaller.h"
#include "Dpi.h"
#include <QSize>
#include <QDir>
#include <QDomElement>
#include <QDomNode>
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#endif
#include <set>

ProjectReader::ProjectReader(QDomDocument const& doc)
:	m_doc(doc),
	m_ptrDisambiguator(new FileNameDisambiguator)
{
	QDomElement project_el(m_doc.documentElement());
	m_outDir = project_el.attribute("outputDirectory");
	
	Qt::LayoutDirection layout_direction = Qt::LeftToRight;
	if (project_el.attribute("layoutDirection") == "RTL") {
		layout_direction = Qt::RightToLeft;
	}
	
	QDomElement const dirs_el(project_el.namedItem("directories").toElement());
	if (dirs_el.isNull()) {
		return;
	}
	processDirectories(dirs_el);
	
	QDomElement const files_el(project_el.namedItem("files").toElement());
	if (files_el.isNull()) {
		return;
	}
	processFiles(files_el);
	
	QDomElement const images_el(project_el.namedItem("images").toElement());
	if (images_el.isNull()) {
		return;
	}
	processImages(images_el, layout_direction);
	
	QDomElement const pages_el(project_el.namedItem("pages").toElement());
	if (pages_el.isNull()) {
		return;
	}
	processPages(pages_el);

	// Load naming disambiguator.  This needs to be done after processing pages.
	QDomElement const disambig_el(
		project_el.namedItem("file-name-disambiguation").toElement()
	);
	m_ptrDisambiguator.reset(
		new FileNameDisambiguator(
			disambig_el, boost::bind(&ProjectReader::expandFilePath, this, _1)
		)
	);
}

ProjectReader::~ProjectReader()
{
}

void
ProjectReader::readFilterSettings(std::vector<FilterPtr> const& filters) const
{
	QDomElement project_el(m_doc.documentElement());
	QDomElement filters_el(project_el.namedItem("filters").toElement());
	
	std::vector<FilterPtr>::const_iterator it(filters.begin());
	std::vector<FilterPtr>::const_iterator const end(filters.end());
	for (; it != end; ++it) {
		(*it)->loadSettings(*this, filters_el);
	}
}

void
ProjectReader::processDirectories(QDomElement const& dirs_el)
{
	QString const dir_tag_name("directory");
	
	QDomNode node(dirs_el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != dir_tag_name) {
			continue;
		}
		QDomElement el(node.toElement());
		
		bool ok = true;
		int const id = el.attribute("id").toInt(&ok);
		if (!ok) {
			continue;
		}
		
		QString const path(el.attribute("path"));
		if (path.isEmpty()) {
			continue;
		}
		
		m_dirMap.insert(DirMap::value_type(id, path));
	}
}

void
ProjectReader::processFiles(QDomElement const& files_el)
{
	QString const file_tag_name("file");
	
	QDomNode node(files_el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != file_tag_name) {
			continue;
		}
		QDomElement el(node.toElement());
		
		bool ok = true;
		int const id = el.attribute("id").toInt(&ok);
		if (!ok) {
			continue;
		}
		int const dir_id = el.attribute("dirId").toInt(&ok);
		if (!ok) {
			continue;
		}
		
		QString const name(el.attribute("name"));
		if (name.isEmpty()) {
			continue;
		}
		
		QString const dir_path(getDirPath(dir_id));
		if (dir_path.isEmpty()) {
			continue;
		}
		
		// Backwards compatibility.
		bool const compat_multi_page = (el.attribute("multiPage") == "1");

		QString const file_path(QDir(dir_path).filePath(name));
		FileRecord const rec(file_path, compat_multi_page);
		m_fileMap.insert(FileMap::value_type(id, rec));
	}
}

void
ProjectReader::processImages(
	QDomElement const& images_el,
	Qt::LayoutDirection const layout_direction)
{
	QString const image_tag_name("image");
	
	std::vector<ImageInfo> images;
	
	QDomNode node(images_el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != image_tag_name) {
			continue;
		}
		QDomElement el(node.toElement());
		
		bool ok = true;
		int const id = el.attribute("id").toInt(&ok);
		if (!ok) {
			continue;
		}
		int const sub_pages = el.attribute("subPages").toInt(&ok);
		if (!ok) {
			continue;
		}
		int const file_id = el.attribute("fileId").toInt(&ok);
		if (!ok) {
			continue;
		}
		int const file_image = el.attribute("fileImage").toInt(&ok);
		if (!ok) {
			continue;
		}

		QString const removed(el.attribute("removed"));
		bool const left_half_removed = (removed == "L");
		bool const right_half_removed = (removed == "R");
		
		FileRecord const file_record(getFileRecord(file_id));
		if (file_record.filePath.isEmpty()) {
			continue;
		}
		ImageId const image_id(
			file_record.filePath,
			file_image + int(file_record.compatMultiPage)
		);
		ImageMetadata const metadata(processImageMetadata(el));
		ImageInfo const image_info(
			image_id, metadata, sub_pages,
			left_half_removed, right_half_removed
		);
		
		images.push_back(image_info);
		m_imageMap.insert(ImageMap::value_type(id, image_info));
	}
	
	if (!images.empty()) {
		m_ptrPages.reset(new ProjectPages(images, layout_direction));
	}
}

ImageMetadata
ProjectReader::processImageMetadata(QDomElement const& image_el)
{
	QSize size;
	Dpi dpi;
	
	QDomElement const size_el(image_el.namedItem("size").toElement());
	if (!size_el.isNull()) {
		size = XmlUnmarshaller::size(size_el);
	}
	QDomElement const dpi_el(image_el.namedItem("dpi").toElement());
	if (!dpi_el.isNull()) {
		dpi = XmlUnmarshaller::dpi(dpi_el);
	}
	
	return ImageMetadata(size, dpi);
}

void
ProjectReader::processPages(QDomElement const& pages_el)
{
	QString const page_tag_name("page");
	
	QDomNode node(pages_el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != page_tag_name) {
			continue;
		}
		QDomElement el(node.toElement());
		
		bool ok = true;
		
		int const id = el.attribute("id").toInt(&ok);
		if (!ok) {
			continue;
		}
		
		int const image_id = el.attribute("imageId").toInt(&ok);
		if (!ok) {
			continue;
		}
		
		PageId::SubPage const sub_page = PageId::subPageFromString(
			el.attribute("subPage"), &ok
		);
		if (!ok) {
			continue;
		}
		
		ImageInfo const image(getImageInfo(image_id));
		if (image.id().filePath().isEmpty()) {
			continue;
		}
		
		PageId const page_id(image.id(), sub_page);
		m_pageMap.insert(PageMap::value_type(id, page_id));

		if (el.attribute("selected") == "selected") {
			m_selectedPage.set(page_id, PAGE_VIEW);
		}
	}
}

QString
ProjectReader::getDirPath(int const id) const
{
	DirMap::const_iterator const it(m_dirMap.find(id));
	if (it != m_dirMap.end()) {
		return it->second;
	}
	return QString();
}

ProjectReader::FileRecord
ProjectReader::getFileRecord(int id) const
{
	FileMap::const_iterator const it(m_fileMap.find(id));
	if (it != m_fileMap.end()) {
		return it->second;
	}
	return FileRecord();
}

QString
ProjectReader::expandFilePath(QString const& path_shorthand) const
{
	bool ok = false;
	int const file_id = path_shorthand.toInt(&ok);
	if (!ok) {
		return QString();
	}
	return getFileRecord(file_id).filePath;
}

ImageInfo
ProjectReader::getImageInfo(int id) const
{
	ImageMap::const_iterator it(m_imageMap.find(id));
	if (it != m_imageMap.end()) {
		return it->second;
	}
	return ImageInfo();
}

ImageId
ProjectReader::imageId(int const numeric_id) const
{
	ImageMap::const_iterator it(m_imageMap.find(numeric_id));
	if (it != m_imageMap.end()) {
		return it->second.id();
	}
	return ImageId();
}

PageId
ProjectReader::pageId(int numeric_id) const
{
	PageMap::const_iterator it(m_pageMap.find(numeric_id));
	if (it != m_pageMap.end()) {
		return it->second;
	}
	return PageId();
}
