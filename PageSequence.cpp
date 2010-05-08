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

#include "PageSequence.h.moc"
#include "ImageFileInfo.h"
#include "ImageMetadata.h"
#include "ImageInfo.h"
#include "OrthogonalRotation.h"
#include <boost/foreach.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <QMutexLocker>
#include <QFileInfo>
#include <QSize>
#include <QDebug>
#include <map>
#include <algorithm>
#include <stddef.h>
#include <assert.h>

PageSequence::PageSequence(Qt::LayoutDirection const layout_direction)
:	m_totalLogicalPages(0),
	m_curImage(0),
	m_curLogicalPage(0),
	m_curSubPage(0)
{
	initSubPagesInOrder(layout_direction);
}

PageSequence::PageSequence(
	std::vector<ImageInfo> const& info,
	Qt::LayoutDirection const layout_direction)
:	m_totalLogicalPages(0),
	m_curImage(0),
	m_curLogicalPage(0),
	m_curSubPage(0)
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
		m_totalLogicalPages += image_desc.numLogicalPages;
	}
}

PageSequence::PageSequence(
	std::vector<ImageFileInfo> const& files,
	Pages const pages, Qt::LayoutDirection const layout_direction)
:	m_totalLogicalPages(0),
	m_curImage(0),
	m_curLogicalPage(0),
	m_curSubPage(0)
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
			m_totalLogicalPages += m_images.back().numLogicalPages;
		}
	}
}

PageSequence::~PageSequence()
{
}

Qt::LayoutDirection
PageSequence::layoutDirection() const
{
	if (m_subPagesInOrder[0] == PageId::LEFT_PAGE) {
		return Qt::LeftToRight;
	} else {
		assert(m_subPagesInOrder[0] == PageId::RIGHT_PAGE);
		return Qt::RightToLeft;
	}
}

void
PageSequence::initSubPagesInOrder(Qt::LayoutDirection const layout_direction)
{
	if (layout_direction == Qt::LeftToRight) {
		m_subPagesInOrder[0] = PageId::LEFT_PAGE;
		m_subPagesInOrder[1] = PageId::RIGHT_PAGE;
	} else {
		m_subPagesInOrder[0] = PageId::RIGHT_PAGE;
		m_subPagesInOrder[1] = PageId::LEFT_PAGE;
	}
}

PageSequenceSnapshot
PageSequence::snapshot(View const view) const
{
	std::vector<PageInfo> pages;
	int cur_page;
	
	if (view == PAGE_VIEW) {
		QMutexLocker locker(&m_mutex);
		
		cur_page = m_curLogicalPage;
		pages.reserve(m_totalLogicalPages);
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
				pages.push_back(
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
		
		cur_page = m_curImage;
		int const num_images = m_images.size();
		pages.reserve(num_images);
		for (int i = 0; i < num_images; ++i) {
			ImageDesc const& image = m_images[i];
			PageId const id(image.id, PageId::SINGLE_PAGE);
			pages.push_back(
				PageInfo(
					id, image.metadata,
					image.numLogicalPages,
					image.leftHalfRemoved,
					image.rightHalfRemoved
				)
			);
		}
	}
	
	assert((pages.empty() && cur_page == 0) || cur_page < (int)pages.size());
	
	PageSequenceSnapshot snapshot(view);
	pages.swap(snapshot.m_pages);
	snapshot.m_curPageIdx = cur_page;
	
	return snapshot;
}

void
PageSequence::setLogicalPagesInImage(ImageId const& image_id, int const num_pages)
{
	assert(num_pages >= 1 && num_pages <= 2);
	
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		setLogicalPagesInImageImpl(image_id, num_pages, &was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
}

void
PageSequence::setLogicalPagesInAllImages(int const num_pages)
{
	assert(num_pages >= 1 && num_pages <= 2);
	
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		setLogicalPagesInAllImagesImpl(num_pages, &was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
}

void
PageSequence::autoSetLogicalPagesInImage(
	ImageId const& image_id, OrthogonalRotation const rotation)
{
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		autoSetLogicalPagesInImageImpl(image_id, rotation, &was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
}

void
PageSequence::updateImageMetadata(
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
PageSequence::adviseNumberOfLogicalPages(
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

void
PageSequence::setCurPage(PageId const& page_id)
{
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		setCurPageImpl(page_id, &was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
}

int
PageSequence::numImages() const
{
	QMutexLocker locker(&m_mutex);
	return m_images.size();
}

int
PageSequence::curImageIdx() const
{
	QMutexLocker locker(&m_mutex);
	return m_curImage;
}

ImageId
PageSequence::curImage() const
{
	QMutexLocker locker(&m_mutex);
	assert((size_t)m_curImage <= m_images.size());
	ImageDesc const& image = m_images[m_curImage];
	return image.id;
}

PageInfo
PageSequence::curPage(View const view, int* page_num) const
{
	QMutexLocker locker(&m_mutex);
	
	if (page_num) {
		if (view == IMAGE_VIEW) {
			*page_num = m_curImage;
		} else {
			*page_num = m_curLogicalPage;
		}
	}

	if ((size_t)m_curImage >= m_images.size()) {
		assert(m_curImage == 0 && m_images.size() == 0);
		return PageInfo();
	}

	ImageDesc const& image = m_images[m_curImage];
	PageId const id(image.id, curSubPageLocked(image, view));
	return PageInfo(
		id, image.metadata, image.numLogicalPages,
		image.leftHalfRemoved, image.rightHalfRemoved
	);
}

PageInfo
PageSequence::setFirstPage(View const view)
{
	PageInfo info;
	bool was_modified = false;
	
	do {
		QMutexLocker const locker(&m_mutex);
		
		if (m_images.empty()) {
			break;
		}
		
		if (m_curImage != 0 || m_curLogicalPage != 0 || m_curSubPage != 0) {
			m_curImage = 0;
			m_curLogicalPage = 0;
			m_curSubPage = 0;
			was_modified = true;
		}
		
		ImageDesc const& image = m_images[0];
		
		PageId const id(image.id, curSubPageLocked(image, view));
		info = PageInfo(
			id, image.metadata, image.numLogicalPages,
			image.leftHalfRemoved, image.rightHalfRemoved
		);
	} while (false);
	
	if (was_modified) {
		emit modified();
	}
	
	return info;
}

PageInfo
PageSequence::setPrevPage(View const view, int* page_num)
{
	PageInfo info;
	
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		info = setPrevPageImpl(view, page_num, was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
	
	return info;
}

PageInfo
PageSequence::setNextPage(View const view, int* page_num)
{
	PageInfo info;
	
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		info = setNextPageImpl(view, page_num, was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
	
	return info;
}

std::vector<PageInfo>
PageSequence::insertImage(
	ImageInfo const& new_image, BeforeOrAfter before_or_after,
	ImageId const& existing, View const view)
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
PageSequence::removePages(std::set<PageId> const& pages)
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
PageSequence::unremovePage(PageId const& page_id)
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
PageSequence::validateDpis() const
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
PageSequence::toImageFileInfo() const
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
PageSequence::updateMetadataFrom(std::vector<ImageFileInfo> const& files)
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
PageSequence::setLogicalPagesInImageImpl(
	ImageId const& image_id, int const num_pages, bool* modified)
{
	assert(num_pages >= 1 && num_pages <= 2);

	int logical_pages_seen = 0;
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
			m_totalLogicalPages += delta;
			if (logical_pages_seen < m_curLogicalPage) {
				m_curLogicalPage += delta;
			}
			m_curSubPage = 0;
			
			*modified = true;
			break;
		}
		logical_pages_seen += image.numLogicalPages;
	}
}

void
PageSequence::setLogicalPagesInAllImagesImpl(
	int const num_pages, bool* modified)
{
	assert(num_pages >= 1 && num_pages <= 2);
	
	int logical_pages_seen = 0;
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
		
		m_totalLogicalPages += delta;
		if (logical_pages_seen < m_curLogicalPage) {
			m_curLogicalPage += delta;
		}

		if (m_curImage == i && adjusted_num_pages == 1) {
			m_curSubPage = 0;
		}

		logical_pages_seen += image.numLogicalPages;
		image.numLogicalPages = adjusted_num_pages;
		*modified = true;
	}
}

void
PageSequence::autoSetLogicalPagesInImageImpl(
	ImageId const& image_id, OrthogonalRotation const rotation, bool* modified)
{
	int logical_pages_seen = 0;
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
			m_totalLogicalPages += delta;
			if (logical_pages_seen < m_curLogicalPage) {
				m_curLogicalPage += delta;
			}
			m_curSubPage = 0;
			
			*modified = true;
			break;
		}
		logical_pages_seen += image.numLogicalPages;
	}
}

void
PageSequence::updateImageMetadataImpl(
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

void
PageSequence::setCurPageImpl(PageId const& page_id, bool* modified)
{
	int logical_pages_seen = 0;
	int const num_images = m_images.size();
	for (int i = 0; i < num_images; ++i) {
		ImageDesc const& image = m_images[i];
		if (image.id == page_id.imageId()) {
			int sub_page = 0;
			if (page_id.subPage() != PageId::SINGLE_PAGE) {
				for (; sub_page < 2; ++sub_page) {
					if (m_subPagesInOrder[sub_page] == page_id.subPage()) {
						break;
					}
				}
			}
			assert(sub_page <= 1);
			
			if (sub_page >= image.numLogicalPages) {
				sub_page = image.numLogicalPages - 1;
			}
			
			if (m_curImage != i || sub_page != m_curSubPage) {
				*modified = true;
			}
			
			m_curImage = i;
			m_curSubPage = sub_page;
			m_curLogicalPage = logical_pages_seen + sub_page;
			break;
		}
		logical_pages_seen += image.numLogicalPages;
	}
}

PageInfo
PageSequence::setPrevPageImpl(View const view, int* page_num, bool& modified)
{
	if (m_images.empty()) {
		return PageInfo();
	}
	
	assert((size_t)m_curImage < m_images.size());
	
	ImageDesc const* image = &m_images[m_curImage];
	if (view == PAGE_VIEW && m_curSubPage == 1) {
		// Move to the previous sub-page within the same image.
		assert(image->numLogicalPages > 1);
		--m_curLogicalPage;
		--m_curSubPage;
		modified = true;
	} else if (m_curImage > 0) {
		// Move to the last sub-page of the previous image.
		m_curLogicalPage -= m_curSubPage; // Move to sub-page 0
		--m_curLogicalPage; // Previous image, last page.
		--m_curImage;
		--image;
		m_curSubPage = image->numLogicalPages - 1;
		modified = true;
	}
	
	if (page_num) {
		if (view == IMAGE_VIEW) {
			*page_num = m_curImage;
		} else {
			*page_num = m_curLogicalPage;
		}
	}
	
	PageId const id(image->id, curSubPageLocked(*image, view));
	return PageInfo(
		id, image->metadata, image->numLogicalPages,
		image->leftHalfRemoved, image->rightHalfRemoved
	);
}

PageInfo
PageSequence::setNextPageImpl(View const view, int* page_num, bool& modified)
{
	if (m_images.empty()) {
		return PageInfo();
	}
	
	assert((size_t)m_curImage < m_images.size());
	
	ImageDesc const* image = &m_images[m_curImage];
	if (view == PAGE_VIEW && m_curSubPage == 0 && image->numLogicalPages > 1) {
		// Move to the next sub-page within the same image.
		++m_curLogicalPage;
		++m_curSubPage;
		modified = true;
	} else if (m_curImage < (int)m_images.size() - 1) {
		// Move to the first sub-page of the next image.
		m_curLogicalPage -= m_curSubPage; // Move to sub-page 0.
		m_curLogicalPage += image->numLogicalPages; // Next image, first page.
		m_curSubPage = 0;
		++m_curImage;
		++image;
		modified = true;
	}
	
	if (page_num) {
		if (view == IMAGE_VIEW) {
			*page_num = m_curImage;
		} else {
			*page_num = m_curLogicalPage;
		}
	}
	
	PageId const id(image->id, curSubPageLocked(*image, view));
	return PageInfo(
		id, image->metadata, image->numLogicalPages,
		image->leftHalfRemoved, image->rightHalfRemoved
	);
}

std::vector<PageInfo>
PageSequence::insertImageImpl(
	ImageInfo const& new_image, BeforeOrAfter before_or_after,
	ImageId const& existing, View const view, bool& modified)
{	
	std::vector<PageInfo> logical_pages;

	std::vector<ImageDesc>::iterator it(m_images.begin());
	std::vector<ImageDesc>::iterator const end(m_images.end());
	int logical_pages_seen = 0;
	for (; it != end && it->id != existing; ++it) {
		logical_pages_seen += it->numLogicalPages;
	}
	if (it == end) {
		// Existing image not found.
		if (!(before_or_after == BEFORE && existing.isNull())) { 
			return logical_pages;
		} // Otherwise we can still handle that case.
	}
	if (before_or_after == AFTER) {
		++it;
		if (it != end) {
			logical_pages_seen += it->numLogicalPages;
		}
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
	
	m_totalLogicalPages += new_image.numSubPages();
	if (logical_pages_seen < m_curLogicalPage) {
		m_curLogicalPage += new_image.numSubPages();
	}
	
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
				image_desc.numLogicalPages == 1 && image_desc.rightHalfRemoved) {
			page_info_templ.setId(PageId(new_image.id(), m_subPagesInOrder[0]));
			logical_pages.push_back(page_info_templ);
		}
		if (image_desc.numLogicalPages == 2 ||
				image_desc.numLogicalPages == 1 && image_desc.leftHalfRemoved) {
			page_info_templ.setId(PageId(new_image.id(), m_subPagesInOrder[1]));
			logical_pages.push_back(page_info_templ);
		}
	}

	return logical_pages;
}

void
PageSequence::removePagesImpl(std::set<PageId> const& to_remove, bool& modified)
{
	std::set<PageId>::const_iterator const to_remove_end(to_remove.end());

	std::vector<ImageDesc> new_images;
	new_images.reserve(m_images.size());
	int new_total_logical_pages = 0;
	int new_cur_image_lower_bound = 0;
	int new_cur_logical_page_lower_bound = 0;
	int cur_image_intact = true;

	int const num_old_images = m_images.size();
	for (int i = 0; i < num_old_images; ++i) {
		ImageDesc image(m_images[i]);
		if (m_curImage == i) {
			new_cur_image_lower_bound = new_images.size();
			new_cur_logical_page_lower_bound = new_total_logical_pages;
		}

		if (image.numLogicalPages == 1) {
			if (to_remove.find(PageId(image.id, PageId::SINGLE_PAGE)) == to_remove_end) {
				new_images.push_back(image);
				new_total_logical_pages += image.numLogicalPages;
			} else {
				modified = true;
				if (m_curImage == i) {
					cur_image_intact = false;
				}
			}
		} else {
			assert(image.numLogicalPages == 2);

			int subpages_to_remove = 0;
			if (to_remove.find(PageId(image.id, PageId::SINGLE_PAGE)) != to_remove_end) {
				subpages_to_remove = 2;
			} else {
				if (to_remove.find(PageId(image.id, PageId::LEFT_PAGE)) != to_remove_end) {
					image.leftHalfRemoved = true;
					--image.numLogicalPages;
					++subpages_to_remove;
				}
				if (to_remove.find(PageId(image.id, PageId::RIGHT_PAGE)) != to_remove_end) {
					image.rightHalfRemoved = true;
					--image.numLogicalPages;
					++subpages_to_remove;
				}
			}

			if (subpages_to_remove < 2) {
				new_images.push_back(image);
				new_total_logical_pages += new_images.back().numLogicalPages;
			}

			if (subpages_to_remove > 0) {
				modified = true;
				if (m_curImage == i) {
					cur_image_intact = false;
				}
			}
		}
	}

	if (new_images.empty()) {
		m_curImage = 0;
		m_curLogicalPage = 0;
		m_curSubPage = 0;
	} else if (new_cur_image_lower_bound >= (int)new_images.size()) {
		// The last sub-page in the sequence becomes the current page.
		m_curImage = new_images.size() - 1;
		m_curLogicalPage = new_total_logical_pages - 1;
		m_curSubPage = new_images.back().numLogicalPages - 1;
	} else {
		m_curImage = new_cur_image_lower_bound;
		m_curLogicalPage = new_cur_logical_page_lower_bound;
		if (!cur_image_intact) {
			m_curSubPage = 0;
		}
	}

	new_images.swap(m_images);
}

PageInfo
PageSequence::unremovePageImpl(PageId const& page_id, bool& modified)
{
	if (page_id.subPage() == PageId::SINGLE_PAGE) {
		// These can't be unremoved.
		return PageInfo();
	}

	std::vector<ImageDesc>::iterator it(m_images.begin());
	std::vector<ImageDesc>::iterator const end(m_images.end());
	int logical_pages_seen = 0;
	for (; it != end && it->id != page_id.imageId(); ++it) {
		logical_pages_seen += it->numLogicalPages;
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
		if (logical_pages_seen == m_curLogicalPage) {
			assert(m_curSubPage == 0);
			m_curSubPage = 1;
		}
		image.leftHalfRemoved = false;
	} else if (page_id.subPage() == PageId::RIGHT_PAGE && image.rightHalfRemoved) {
		++logical_pages_seen;
		image.rightHalfRemoved = false;
	} else {
		return PageInfo();
	}
	
	image.numLogicalPages = 2;
	++m_totalLogicalPages;
	if (logical_pages_seen <= m_curLogicalPage) {
		++m_curLogicalPage;
	}

	return PageInfo(
		page_id, image.metadata, image.numLogicalPages,
		image.leftHalfRemoved, image.rightHalfRemoved
	);
}

PageId::SubPage
PageSequence::curSubPageLocked(ImageDesc const& image, View const view) const
{
	if (view == IMAGE_VIEW) {
		return PageId::SINGLE_PAGE;
	}

	if (image.numLogicalPages == 1) {
		if (image.leftHalfRemoved && !image.rightHalfRemoved) {
			return PageId::RIGHT_PAGE;
		} else if (image.rightHalfRemoved && !image.leftHalfRemoved) {
			return PageId::LEFT_PAGE;
		} else {
			return PageId::SINGLE_PAGE;
		}
	}
	
	return m_subPagesInOrder[m_curSubPage];
}


/*========================= PageSequenceSnapshot ========================*/

PageSequenceSnapshot::PageSequenceSnapshot(PageSequence::View const view)
:	m_curPageIdx(0),
	m_view(view)
{
}

PageSequenceSnapshot::~PageSequenceSnapshot()
{
}

PageInfo const&
PageSequenceSnapshot::curPage() const
{
	return m_pages.at(m_curPageIdx); // may throw
}

PageInfo const&
PageSequenceSnapshot::pageAt(size_t const idx) const
{
	return m_pages.at(idx); // may throw
}


/*========================= PageSequence::ImageDesc ======================*/

PageSequence::ImageDesc::ImageDesc(ImageInfo const& image_info)
:	id(image_info.id()),
	metadata(image_info.metadata()),
	numLogicalPages(image_info.numSubPages()),
	leftHalfRemoved(image_info.leftHalfRemoved()),
	rightHalfRemoved(image_info.rightHalfRemoved())
{
}

PageSequence::ImageDesc::ImageDesc(
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
PageSequence::ImageDesc::logicalPageToSubPage(
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
