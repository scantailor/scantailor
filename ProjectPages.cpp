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

#include "ProjectPages.h"
#include "ProjectPages.h.moc"
#include "ImageFileInfo.h"
#include "ImageMetadata.h"
#include "ImageInfo.h"
#include "OrthogonalRotation.h"
#include "PageSequence.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#endif
#include <QMutexLocker>
#include <QFileInfo>
#include <QSize>
#include <QDebug>
#include <map>
#include <algorithm>
#include <stddef.h>
#include <assert.h>

ProjectPages::ProjectPages(Qt::LayoutDirection const layout_direction)
{
	initSubPagesInOrder(layout_direction);
}

ProjectPages::ProjectPages(
	std::vector<ImageInfo> const& info,
	Qt::LayoutDirection const layout_direction)
{
	initSubPagesInOrder(layout_direction);
	
	BOOST_FOREACH(ImageInfo const& image, info) {
		ImageDesc image_desc(image);
		
		// Enforce some rules.
		if (image_desc.numLogicalPages == 2) {
			image_desc.leftHalfRemoved = false;
			image_desc.rightHalfRemoved = false;
		} else if (image_desc.numLogicalPages != 1) {
			continue;
		} else if (image_desc.leftHalfRemoved && image_desc.rightHalfRemoved) {
			image_desc.leftHalfRemoved = false;
			image_desc.rightHalfRemoved = false;
		}

		m_images.push_back(image_desc);
	}
}

ProjectPages::ProjectPages(
	std::vector<ImageFileInfo> const& files,
	Pages const pages, Qt::LayoutDirection const layout_direction)
{
	initSubPagesInOrder(layout_direction);
	
	BOOST_FOREACH(ImageFileInfo const& file, files) {
		QString const& file_path = file.fileInfo().absoluteFilePath();
		std::vector<ImageMetadata> const& images = file.imageInfo();
		int const num_images = images.size();
		int const multi_page_base = num_images > 1 ? 1 : 0;
		for (int i = 0; i < num_images; ++i) {
			ImageMetadata const& metadata = images[i];
			ImageId const id(file_path, multi_page_base + i);
			m_images.push_back(ImageDesc(id, metadata, pages));
		}
	}
}

ProjectPages::~ProjectPages()
{
}

Qt::LayoutDirection
ProjectPages::layoutDirection() const
{
	if (m_subPagesInOrder[0] == PageId::LEFT_PAGE) {
		return Qt::LeftToRight;
	} else {
		assert(m_subPagesInOrder[0] == PageId::RIGHT_PAGE);
		return Qt::RightToLeft;
	}
}

void
ProjectPages::initSubPagesInOrder(Qt::LayoutDirection const layout_direction)
{
	if (layout_direction == Qt::LeftToRight) {
		m_subPagesInOrder[0] = PageId::LEFT_PAGE;
		m_subPagesInOrder[1] = PageId::RIGHT_PAGE;
	} else {
		m_subPagesInOrder[0] = PageId::RIGHT_PAGE;
		m_subPagesInOrder[1] = PageId::LEFT_PAGE;
	}
}

PageSequence
ProjectPages::toPageSequence(PageView const view) const
{
	PageSequence pages;
	
	if (view == PAGE_VIEW) {
		QMutexLocker locker(&m_mutex);
		
		int const num_images = m_images.size();
		for (int i = 0; i < num_images; ++i) {
			ImageDesc const& image = m_images[i];
			assert(image.numLogicalPages >= 1 && image.numLogicalPages <= 2);
			for (int j = 0; j < image.numLogicalPages; ++j) {
				PageId const id(
					image.id,
					image.logicalPageToSubPage(
						j, m_subPagesInOrder
					)
				);
				pages.append(
					PageInfo(
						id, image.metadata,
						image.numLogicalPages,
						image.leftHalfRemoved,
						image.rightHalfRemoved
					)
				);
			}
		}
	} else {
		assert(view == IMAGE_VIEW);
		
		QMutexLocker locker(&m_mutex);
		
		int const num_images = m_images.size();
		for (int i = 0; i < num_images; ++i) {
			ImageDesc const& image = m_images[i];
			PageId const id(image.id, PageId::SINGLE_PAGE);
			pages.append(
				PageInfo(
					id, image.metadata,
					image.numLogicalPages,
					image.leftHalfRemoved,
					image.rightHalfRemoved
				)
			);
		}
	}
	
	return pages;
}

void
ProjectPages::listRelinkablePaths(VirtualFunction1<void, RelinkablePath const&>& sink) const
{
	// It's generally a bad idea to do callbacks while holding an internal mutex,
	// so we accumulate results into this vector first.
	std::vector<QString> files;
	
	{
		QMutexLocker locker(&m_mutex);
		
		files.reserve(m_images.size());
		BOOST_FOREACH(ImageDesc const& image, m_images) {
			files.push_back(image.id.filePath());
		}
	}

	BOOST_FOREACH(QString const& file, files) {
		sink(RelinkablePath(file, RelinkablePath::File));
	}
}

void
ProjectPages::performRelinking(AbstractRelinker const& relinker)
{
	QMutexLocker locker(&m_mutex);

	BOOST_FOREACH(ImageDesc& image, m_images) {
		RelinkablePath const old_path(image.id.filePath(), RelinkablePath::File);
		QString const new_path(relinker.substitutionPathFor(old_path));
		image.id.setFilePath(new_path);
	}
}

void
ProjectPages::setLayoutTypeFor(ImageId const& image_id, LayoutType const layout)
{
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		setLayoutTypeForImpl(image_id, layout, &was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
}

void
ProjectPages::setLayoutTypeForAllPages(LayoutType const layout)
{
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		setLayoutTypeForAllPagesImpl(layout, &was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
}

void
ProjectPages::autoSetLayoutTypeFor(
	ImageId const& image_id, OrthogonalRotation const rotation)
{
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		autoSetLayoutTypeForImpl(image_id, rotation, &was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
}

void
ProjectPages::updateImageMetadata(
	ImageId const& image_id, ImageMetadata const& metadata)
{
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		updateImageMetadataImpl(image_id, metadata, &was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
}

int
ProjectPages::adviseNumberOfLogicalPages(
	ImageMetadata const& metadata, OrthogonalRotation const rotation)
{
	QSize const size(rotation.rotate(metadata.size()));
	QSize const dpi(rotation.rotate(metadata.dpi().toSize()));
	
	if (size.width() * dpi.height() > size.height() * dpi.width()) {
		return 2;
	} else {
		return 1;
	}
}

int
ProjectPages::numImages() const
{
	QMutexLocker locker(&m_mutex);
	return m_images.size();
}

std::vector<PageInfo>
ProjectPages::insertImage(
	ImageInfo const& new_image, BeforeOrAfter before_or_after,
	ImageId const& existing, PageView const view)
{
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		return insertImageImpl(
			new_image, before_or_after, existing, view, was_modified
		);
	}
	
	if (was_modified) {
		emit modified();
	}
}

void
ProjectPages::removePages(std::set<PageId> const& pages)
{
	bool was_modified = false;

	{
		QMutexLocker locker(&m_mutex);
		removePagesImpl(pages, was_modified);
	}

	if (was_modified) {
		emit modified();
	}
}

PageInfo
ProjectPages::unremovePage(PageId const& page_id)
{
	bool was_modified = false;

	PageInfo page_info;

	{
		QMutexLocker locker(&m_mutex);
		page_info = unremovePageImpl(page_id, was_modified);
	}

	if (was_modified) {
		emit modified();
	}

	return page_info;
}

bool
ProjectPages::validateDpis() const
{
	QMutexLocker locker(&m_mutex);
	
	BOOST_FOREACH(ImageDesc const& image, m_images) {
		if (!image.metadata.isDpiOK()) {
			return false;
		}
	}
	
	return true;
}

namespace
{

struct File
{
	QString fileName;
	mutable std::vector<ImageMetadata> metadata;
	
	File(QString const& fname) : fileName(fname) {}
	
	operator ImageFileInfo() const { return ImageFileInfo(fileName, metadata); }
};

} // anonymous namespace

std::vector<ImageFileInfo>
ProjectPages::toImageFileInfo() const
{
	using namespace boost::multi_index;
	
	multi_index_container<
		File,
		indexed_by<
			ordered_unique<member<File, QString, &File::fileName> >,
			sequenced<>
		>
	> files;
	
	{
		QMutexLocker locker(&m_mutex);
		
		BOOST_FOREACH(ImageDesc const& image, m_images) {
			File const file(image.id.filePath());
			files.insert(file).first->metadata.push_back(image.metadata);
		}
	}
	
	return std::vector<ImageFileInfo>(files.get<1>().begin(), files.get<1>().end());
}

void
ProjectPages::updateMetadataFrom(std::vector<ImageFileInfo> const& files)
{
	typedef std::map<ImageId, ImageMetadata> MetadataMap;
	MetadataMap metadata_map;
	
	BOOST_FOREACH(ImageFileInfo const& file, files) {
		QString const file_path(file.fileInfo().absoluteFilePath());
		int page = 0;
		BOOST_FOREACH(ImageMetadata const& metadata, file.imageInfo()) {
			metadata_map[ImageId(file_path, page)] = metadata;
			++page;
		}
	}
	
	QMutexLocker locker(&m_mutex);
	
	BOOST_FOREACH(ImageDesc& image, m_images) {
		MetadataMap::const_iterator const it(metadata_map.find(image.id));
		if (it != metadata_map.end()) {
			image.metadata = it->second;
		}
	}
}

void
ProjectPages::setLayoutTypeForImpl(
	ImageId const& image_id, LayoutType const layout, bool* modified)
{
	int const num_pages = (layout == TWO_PAGE_LAYOUT ? 2 : 1);
	int const num_images = m_images.size();
	for (int i = 0; i < num_images; ++i) {
		ImageDesc& image = m_images[i];
		if (image.id == image_id) {
			int adjusted_num_pages = num_pages;
			if (num_pages == 2 && image.leftHalfRemoved != image.rightHalfRemoved) {
				// Both can't be removed, but we handle that case anyway
				// by treating it like none are removed.
				--adjusted_num_pages;
			}

			int const delta = adjusted_num_pages - image.numLogicalPages;
			if (delta == 0) {
				break;
			}
			
			image.numLogicalPages = adjusted_num_pages;
			
			*modified = true;
			break;
		}
	}
}

void
ProjectPages::setLayoutTypeForAllPagesImpl(
	LayoutType const layout, bool* modified)
{
	int const num_pages = (layout == TWO_PAGE_LAYOUT ? 2 : 1);	
	int const num_images = m_images.size();
	for (int i = 0; i < num_images; ++i) {
		ImageDesc& image = m_images[i];

		int adjusted_num_pages = num_pages;
		if (num_pages == 2 && image.leftHalfRemoved != image.rightHalfRemoved) {
			// Both can't be removed, but we handle that case anyway
			// by treating it like none are removed.
			--adjusted_num_pages;
		}

		int const delta = adjusted_num_pages - image.numLogicalPages;
		if (delta == 0) {
			continue;
		}

		image.numLogicalPages = adjusted_num_pages;
		*modified = true;
	}
}

void
ProjectPages::autoSetLayoutTypeForImpl(
	ImageId const& image_id, OrthogonalRotation const rotation, bool* modified)
{
	int const num_images = m_images.size();
	for (int i = 0; i < num_images; ++i) {
		ImageDesc& image = m_images[i];
		if (image.id == image_id) {
			int num_pages = adviseNumberOfLogicalPages(image.metadata, rotation);
			if (num_pages == 2 && image.leftHalfRemoved != image.rightHalfRemoved) {
				// Both can't be removed, but we handle that case anyway
				// by treating it like none are removed.
				--num_pages;
			}

			int const delta = num_pages - image.numLogicalPages;
			if (delta == 0) {
				break;
			}
			
			image.numLogicalPages = num_pages;
			
			*modified = true;
			break;
		}
	}
}

void
ProjectPages::updateImageMetadataImpl(
	ImageId const& image_id,
	ImageMetadata const& metadata, bool* modified)
{
	int const num_images = m_images.size();
	for (int i = 0; i < num_images; ++i) {
		ImageDesc& image = m_images[i];
		if (image.id == image_id) {
			if (image.metadata != metadata) {
				image.metadata = metadata;
				*modified = true;
			}
			break;
		}
	}
}

std::vector<PageInfo>
ProjectPages::insertImageImpl(
	ImageInfo const& new_image, BeforeOrAfter before_or_after,
	ImageId const& existing, PageView const view, bool& modified)
{	
	std::vector<PageInfo> logical_pages;

	std::vector<ImageDesc>::iterator it(m_images.begin());
	std::vector<ImageDesc>::iterator const end(m_images.end());
	for (; it != end && it->id != existing; ++it) {
		// Continue until we find the existing image.
	}
	if (it == end) {
		// Existing image not found.
		if (!(before_or_after == BEFORE && existing.isNull())) { 
			return logical_pages;
		} // Otherwise we can still handle that case.
	}
	if (before_or_after == AFTER) {
		++it;
	}
	
	ImageDesc image_desc(new_image);

	// Enforce some rules.
	if (image_desc.numLogicalPages == 2) {
		image_desc.leftHalfRemoved = false;
		image_desc.rightHalfRemoved = false;
	} else if (image_desc.numLogicalPages != 1) {
		return logical_pages;
	} else if (image_desc.leftHalfRemoved && image_desc.rightHalfRemoved) {
		image_desc.leftHalfRemoved = false;
		image_desc.rightHalfRemoved = false;
	}

	m_images.insert(it, image_desc);
	
	PageInfo page_info_templ(
		PageId(new_image.id(), PageId::SINGLE_PAGE),
		image_desc.metadata, image_desc.numLogicalPages,
		image_desc.leftHalfRemoved, image_desc.rightHalfRemoved
	);

	if (view == IMAGE_VIEW || (image_desc.numLogicalPages == 1 &&
			image_desc.leftHalfRemoved == image_desc.rightHalfRemoved)) {
		logical_pages.push_back(page_info_templ);
	} else {
		if (image_desc.numLogicalPages == 2 ||
				(image_desc.numLogicalPages == 1 && image_desc.rightHalfRemoved)) {
			page_info_templ.setId(PageId(new_image.id(), m_subPagesInOrder[0]));
			logical_pages.push_back(page_info_templ);
		}
		if (image_desc.numLogicalPages == 2 ||
				(image_desc.numLogicalPages == 1 && image_desc.leftHalfRemoved)) {
			page_info_templ.setId(PageId(new_image.id(), m_subPagesInOrder[1]));
			logical_pages.push_back(page_info_templ);
		}
	}

	return logical_pages;
}

void
ProjectPages::removePagesImpl(std::set<PageId> const& to_remove, bool& modified)
{
	std::set<PageId>::const_iterator const to_remove_end(to_remove.end());

	std::vector<ImageDesc> new_images;
	new_images.reserve(m_images.size());
	int new_total_logical_pages = 0;

	int const num_old_images = m_images.size();
	for (int i = 0; i < num_old_images; ++i) {
		ImageDesc image(m_images[i]);

		if (to_remove.find(PageId(image.id, PageId::SINGLE_PAGE)) != to_remove_end) {
			image.numLogicalPages = 0;
			modified = true;
		} else {
			if (to_remove.find(PageId(image.id, PageId::LEFT_PAGE)) != to_remove_end) {
				image.leftHalfRemoved = true;
				--image.numLogicalPages;
				modified = true;
			}
			if (to_remove.find(PageId(image.id, PageId::RIGHT_PAGE)) != to_remove_end) {
				image.rightHalfRemoved = true;
				--image.numLogicalPages;
				modified = true;
			}
		}

		if (image.numLogicalPages > 0) {
			new_images.push_back(image);
			new_total_logical_pages += new_images.back().numLogicalPages;
		}
	}

	new_images.swap(m_images);
}

PageInfo
ProjectPages::unremovePageImpl(PageId const& page_id, bool& modified)
{
	if (page_id.subPage() == PageId::SINGLE_PAGE) {
		// These can't be unremoved.
		return PageInfo();
	}

	std::vector<ImageDesc>::iterator it(m_images.begin());
	std::vector<ImageDesc>::iterator const end(m_images.end());
	for (; it != end && it->id != page_id.imageId(); ++it) {
		// Continue until we find the corresponding image.
	}
	if (it == end) {
		// The corresponding image wasn't found.
		return PageInfo();
	}
	
	ImageDesc& image = *it;

	if (image.numLogicalPages != 1) {
		return PageInfo();
	}

	if (page_id.subPage() == PageId::LEFT_PAGE && image.leftHalfRemoved) {
		image.leftHalfRemoved = false;
	} else if (page_id.subPage() == PageId::RIGHT_PAGE && image.rightHalfRemoved) {
		image.rightHalfRemoved = false;
	} else {
		return PageInfo();
	}
	
	image.numLogicalPages = 2;

	return PageInfo(
		page_id, image.metadata, image.numLogicalPages,
		image.leftHalfRemoved, image.rightHalfRemoved
	);
}


/*========================= ProjectPages::ImageDesc ======================*/

ProjectPages::ImageDesc::ImageDesc(ImageInfo const& image_info)
:	id(image_info.id()),
	metadata(image_info.metadata()),
	numLogicalPages(image_info.numSubPages()),
	leftHalfRemoved(image_info.leftHalfRemoved()),
	rightHalfRemoved(image_info.rightHalfRemoved())
{
}

ProjectPages::ImageDesc::ImageDesc(
	ImageId const& id, ImageMetadata const& metadata, Pages const pages)
:	id(id),
	metadata(metadata),
	leftHalfRemoved(false),
	rightHalfRemoved(false)
{
	switch (pages) {
		case ONE_PAGE:
			numLogicalPages = 1;
			break;
		case TWO_PAGES:
			numLogicalPages = 2;
			break;
		case AUTO_PAGES:
			numLogicalPages = adviseNumberOfLogicalPages(
				metadata, OrthogonalRotation()
			);
			break;
	}
}

PageId::SubPage
ProjectPages::ImageDesc::logicalPageToSubPage(
	int const logical_page, PageId::SubPage const* sub_pages_in_order) const
{
	assert(numLogicalPages >= 1 && numLogicalPages <= 2);
	assert(logical_page >= 0 && logical_page < numLogicalPages);
	
	if (numLogicalPages == 1) {
		if (leftHalfRemoved && !rightHalfRemoved) {
			return PageId::RIGHT_PAGE;
		} else if (rightHalfRemoved && !leftHalfRemoved) {
			return PageId::LEFT_PAGE;
		} else {
			return PageId::SINGLE_PAGE;
		}
	} else {
		return sub_pages_in_order[logical_page];
	}
}
