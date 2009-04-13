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

#include "PageSequence.h.moc"
#include "ImageFileInfo.h"
#include "ImageMetadata.h"
#include "ImageInfo.h"
#include "OrthogonalRotation.h"
#include <boost/foreach.hpp>
#include <QMutexLocker>
#include <QSize>
#include <QDebug>
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
		m_images.push_back(
			ImageDesc(
				image.id(), image.metadata(),
				image.isMultiPageFile(), image.numSubPages()
			)
		);
		m_totalLogicalPages += m_images.back().numLogicalPages;
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
		bool multi_page = num_images > 1;
		for (int i = 0; i < num_images; ++i) {
			ImageMetadata const& metadata = images[i];
			ImageId const id(file_path, i);
			m_images.push_back(ImageDesc(id, metadata, multi_page, pages));
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
						image.multiPageFile,
						image.numLogicalPages
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
					image.multiPageFile,
					image.numLogicalPages
				)
			);
		}
	}
	
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
	
	assert((size_t)m_curImage <= m_images.size());
	ImageDesc const& image = m_images[m_curImage];
	
	if (page_num) {
		if (view == IMAGE_VIEW) {
			*page_num = m_curImage;
		} else {
			*page_num = m_curLogicalPage;
		}
	}
	
	PageId const id(image.id, curSubPageLocked(image, view));
	return PageInfo(
		id, image.metadata,
		image.multiPageFile, image.numLogicalPages
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
			id, image.metadata,
			image.multiPageFile, image.numLogicalPages
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

bool
PageSequence::insertImage(ImageInfo const& new_image,
	BeforeOrAfter before_or_after, ImageId const& existing)
{
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		return insertImageImpl(new_image, before_or_after, existing, was_modified);
	}
	
	if (was_modified) {
		emit modified();
	}
}

void
PageSequence::removeImage(ImageId const& image_id)
{
	bool was_modified = false;
	
	{
		QMutexLocker locker(&m_mutex);
		removeImageImpl(image_id, was_modified);
	}
	
	if (was_modified) {
		emit modified();
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
PageSequence::setLogicalPagesInAllImagesImpl(
	int const num_pages, bool* modified)
{
	assert(num_pages >= 1 && num_pages <= 2);
	
	int logical_pages_seen = 0;
	int const num_images = m_images.size();
	for (int i = 0; i < num_images; ++i) {
		ImageDesc& image = m_images[i];
		int const delta = num_pages - image.numLogicalPages;
		if (delta == 0) {
			continue;
		}
		
		m_totalLogicalPages += delta;
		if (logical_pages_seen < m_curLogicalPage) {
			m_curLogicalPage += delta;
		}
		logical_pages_seen += image.numLogicalPages;
		image.numLogicalPages = num_pages;
		*modified = true;
	}
	
	if (num_pages == 1) {
		m_curSubPage = 0;
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
			int const num_pages = adviseNumberOfLogicalPages(
				image.metadata, rotation
			);
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
		id, image->metadata,
		image->multiPageFile, image->numLogicalPages
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
		id, image->metadata,
		image->multiPageFile, image->numLogicalPages
	);
}

bool
PageSequence::insertImageImpl(ImageInfo const& new_image,
	BeforeOrAfter before_or_after, ImageId const& existing, bool& modified)
{
	assert(new_image.numSubPages() >= 1 && new_image.numSubPages() <= 2);
	
	std::vector<ImageDesc>::iterator it(m_images.begin());
	std::vector<ImageDesc>::iterator const end(m_images.end());
	int logical_pages_seen = 0;
	for (; it != end && it->id != existing; ++it) {
		logical_pages_seen += it->numLogicalPages;
	}
	if (it == end) {
		// Existing image not found.
		return false;
	}
	if (before_or_after == AFTER) {
		++it;
		if (it != end) {
			logical_pages_seen += it->numLogicalPages;
		}
	}
	
	ImageDesc const image_desc(
		new_image.id(), new_image.metadata(),
		new_image.isMultiPageFile(), new_image.numSubPages()
	);
	m_images.insert(it, image_desc);
	
	m_totalLogicalPages += new_image.numSubPages();
	if (logical_pages_seen < m_curLogicalPage) {
		m_curLogicalPage += new_image.numSubPages();
	}
	
	return true;
}

void
PageSequence::removeImageImpl(ImageId const& image_id, bool& modified)
{
	if (m_images.empty()) {
		return;
	}
	
	std::vector<ImageDesc>::iterator it(m_images.begin());
	std::vector<ImageDesc>::iterator const end(m_images.end());
	int logical_pages_seen = 0;
	int idx = 0;
	for (; it != end && it->id != image_id; ++it, ++idx) {
		logical_pages_seen += it->numLogicalPages;
	}
	if (it == end) {
		return;
	}
	
	m_totalLogicalPages -= it->numLogicalPages;
	if (idx == m_curImage) {
		// Removing the current page.
		if (idx < int(m_images.size()) - 1) {
			// Not the last one.
			// Set the first sub-page of the next image as the current page.
			m_curLogicalPage -= m_curSubPage;
			m_curSubPage = 0;
		} else {
			// Last one.
			// Set the last sub-page of the previous image as the current page.
			--m_curImage;
			m_curLogicalPage -= m_curSubPage - 1;
			m_curSubPage = m_images[m_curImage].numLogicalPages - 1;
		}
	} else if (logical_pages_seen < m_curLogicalPage) {
		m_curLogicalPage -= it->numLogicalPages;
	}
	
	m_images.erase(it);
	modified = true;
}

PageId::SubPage
PageSequence::curSubPageLocked(ImageDesc const& image, View const view) const
{
	if (view == IMAGE_VIEW || image.numLogicalPages == 1) {
		return PageId::SINGLE_PAGE;
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

PageSequence::ImageDesc::ImageDesc(
	ImageId const& id, ImageMetadata const& metadata,
	bool const multi_page, int sub_pages)
:	id(id),
	metadata(metadata),
	numLogicalPages(sub_pages),
	multiPageFile(multi_page)
{
}

PageSequence::ImageDesc::ImageDesc(
	ImageId const& id, ImageMetadata const& metadata,
	bool const multi_page, Pages const pages)
:	id(id),
	metadata(metadata),
	multiPageFile(multi_page)
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
		return PageId::SINGLE_PAGE;
	} else {
		return sub_pages_in_order[logical_page];
	}
}
