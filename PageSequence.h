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

#ifndef PAGESEQUENCE_H_
#define PAGESEQUENCE_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "ImageMetadata.h"
#include "ImageId.h"
#include "PageId.h"
#include "PageInfo.h"
#include <QObject>
#include <QMutex>
#include <QString>
#include <vector>
#include <stddef.h>

class ImageFileInfo;
class ImageInfo;
class OrthogonalRotation;
class PageSequenceSnapshot;
class QDomElement;

class PageSequence : public QObject, public RefCountable
{
	Q_OBJECT
	DECLARE_NON_COPYABLE(PageSequence)
public:
	enum View { IMAGE_VIEW, PAGE_VIEW };
	enum Pages { ONE_PAGE, TWO_PAGES, AUTO_PAGES };
	
	PageSequence();
	
	PageSequence(std::vector<ImageInfo> const& images);
	
	PageSequence(std::vector<ImageFileInfo> const& files, Pages pages);
	
	virtual ~PageSequence();
	
	PageSequenceSnapshot snapshot(View view) const;
	
	void setLogicalPagesInImage(ImageId const& image_id, int num_pages);
	
	void setLogicalPagesInAllImages(int num_pages);
	
	void autoSetLogicalPagesInImage(
		ImageId const& image_id, OrthogonalRotation rotation);
	
	void updateImageMetadata(
		ImageId const& image_id, ImageMetadata const& metadata);
	
	static int adviseNumberOfLogicalPages(
		ImageMetadata const& metadata, OrthogonalRotation rotation);
	
	void setCurPage(PageId const& page_id);
	
	int numImages() const;
	
	int curImageIdx() const;
	
	ImageId curImage() const;
	
	PageInfo curPage(View view, int* page_num = 0) const;
	
	PageInfo setFirstPage(View view);
	
	PageInfo setPrevPage(View view, int* page_num = 0);
	
	PageInfo setNextPage(View view, int* page_num = 0);
signals:
	void modified();
private:
	struct ImageDesc
	{
		ImageId id;
		ImageMetadata metadata;
		int numLogicalPages; // 1 or 2
		bool multiPageFile;
		
		ImageDesc(ImageId const& id, ImageMetadata const& metadata,
			bool multi_page_file, int sub_pages);
		
		ImageDesc(ImageId const& id, ImageMetadata const& metadata,
			bool multi_page_file, Pages pages);
		
		PageId::SubPage logicalPageToSubPage(int logical_page) const;
	};
	
	void setLogicalPagesInImageImpl(
		ImageId const& image_id, int num_pages, bool* modified);
	
	void setLogicalPagesInAllImagesImpl(
		int num_pages, bool* modified);
	
	void autoSetLogicalPagesInImageImpl(
		ImageId const& image_id, OrthogonalRotation rotation, bool* modified);
	
	void updateImageMetadataImpl(
		ImageId const& image_id,
		ImageMetadata const& metadata, bool* modified);
	
	void setCurPageImpl(PageId const& page_id, bool* modified);
	
	PageInfo setPrevPageImpl(View view, int* page_num, bool& modified);
	
	PageInfo setNextPageImpl(View view, int* page_num, bool& modified);
	
	PageId::SubPage curSubPageLocked(ImageDesc const& image, View view) const;
	
	mutable QMutex m_mutex;
	std::vector<ImageDesc> m_images;
	int m_totalLogicalPages;
	int m_curImage;
	int m_curLogicalPage; // Not within the current image, but overall.
	int m_curSubPage; // 0 or 1
};


class PageSequenceSnapshot
{
	friend class PageSequence;
public:
	// Member-wise copying is OK.
	
	PageSequenceSnapshot(PageSequence::View view);
	
	~PageSequenceSnapshot();
	
	PageSequence::View view() const { return m_view; }
	
	size_t numPages() const { return m_pages.size(); }
	
	PageInfo const& curPage() const;
	
	size_t curPageIdx() const { return m_curPageIdx; }
	
	PageInfo const& pageAt(size_t idx) const;
private:
	std::vector<PageInfo> m_pages;
	size_t m_curPageIdx;
	PageSequence::View m_view;
};

#endif
