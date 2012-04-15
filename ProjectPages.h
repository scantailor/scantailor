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

#ifndef PROJECT_PAGES_H_
#define PROJECT_PAGES_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "ImageMetadata.h"
#include "ImageId.h"
#include "PageId.h"
#include "PageInfo.h"
#include "PageView.h"
#include "BeforeOrAfter.h"
#include "VirtualFunction.h"
#include <QObject>
#include <QMutex>
#include <QString>
#include <Qt>
#include <set>
#include <vector>
#include <stddef.h>

class ImageFileInfo;
class ImageInfo;
class OrthogonalRotation;
class PageSequence;
class RelinkablePath;
class AbstractRelinker;
class QDomElement;

class ProjectPages : public QObject, public RefCountable
{
	Q_OBJECT
	DECLARE_NON_COPYABLE(ProjectPages)
public:
	enum Pages { ONE_PAGE, TWO_PAGES, AUTO_PAGES };
	enum LayoutType { ONE_PAGE_LAYOUT, TWO_PAGE_LAYOUT };
	
	ProjectPages(Qt::LayoutDirection layout_direction = Qt::LeftToRight);
	
	ProjectPages(std::vector<ImageInfo> const& images,
		Qt::LayoutDirection layout_direction);
	
	ProjectPages(std::vector<ImageFileInfo> const& files,
		Pages pages, Qt::LayoutDirection layout_direction);
	
	virtual ~ProjectPages();
	
	Qt::LayoutDirection layoutDirection() const;
	
	PageSequence toPageSequence(PageView view) const;

	void listRelinkablePaths(VirtualFunction1<void, RelinkablePath const&>& sink) const;

	/**
	 * \note It's up to the caller to make sure different paths aren't collapsed into one.
	 *       Having the same page more the once in a project is not supported by Scan Tailor.
	 */
	void performRelinking(AbstractRelinker const& relinker);
	
	void setLayoutTypeFor(ImageId const& image_id, LayoutType layout);
	
	void setLayoutTypeForAllPages(LayoutType layout);
	
	void autoSetLayoutTypeFor(
		ImageId const& image_id, OrthogonalRotation rotation);
	
	void updateImageMetadata(
		ImageId const& image_id, ImageMetadata const& metadata);
	
	static int adviseNumberOfLogicalPages(
		ImageMetadata const& metadata, OrthogonalRotation rotation);
	
	int numImages() const;
	
	/**
	 * \brief Insert an image before or after the existing one.
	 *
	 * The caller has to make sure he is not inserting an image that already
	 * exists in this ProjectPages.  Requesting to insert a new image
	 * BEFORE the null one is legal and means inserting it at the end.
	 *
	 * \param new_image The image to insert.
	 * \param before_or_after Whether to insert before or after another image.
	 * \param existing The image we are inserting before or after.
	 * \param view This one only affects what is returned.
	 * \return One or two (or zero, if existing image wasn't found) logical
	 *         page descriptors.  If two are returned, they will be returned
	 *         in the order dependent on the layout direction specified
	 *         at construction time.
	 */
	std::vector<PageInfo> insertImage(ImageInfo const& new_image,
		BeforeOrAfter before_or_after, ImageId const& existing, PageView view);

	void removePages(std::set<PageId> const& pages);

	/**
	 * \brief Unremoves half-a-page, if the other half is still present.
	 * 
	 * \param page_id Left or right sub-page to restore.
	 * \return A PageInfo corresponding to the page restored or
	 *         a null PageInfo if restoring failed.
	 */
	PageInfo unremovePage(PageId const& page_id);
	
	/**
	 * \brief Check if all DPIs are OK, in terms of ImageMetadata::isDpiOK()
	 *
	 * \return true if all DPIs are OK, false if not.
	 */
	bool validateDpis() const;
	
	std::vector<ImageFileInfo> toImageFileInfo() const;
	
	void updateMetadataFrom(std::vector<ImageFileInfo> const& files);
signals:
	void modified();
private:
	struct ImageDesc
	{
		ImageId id;
		ImageMetadata metadata;
		int numLogicalPages; // 1 or 2
		bool leftHalfRemoved;  // Both can't be true, and if one is true,
		bool rightHalfRemoved; // then numLogicalPages is 1.
		
		ImageDesc(ImageInfo const& image_info);
		
		ImageDesc(ImageId const& id, ImageMetadata const& metadata, Pages pages);
		
		PageId::SubPage logicalPageToSubPage(int logical_page,
			PageId::SubPage const* sub_pages_in_order) const;
	};
	
	void initSubPagesInOrder(Qt::LayoutDirection layout_direction);
	
	void setLayoutTypeForImpl(
		ImageId const& image_id, LayoutType layout, bool* modified);
	
	void setLayoutTypeForAllPagesImpl(
		LayoutType layout, bool* modified);
	
	void autoSetLayoutTypeForImpl(
		ImageId const& image_id, OrthogonalRotation rotation, bool* modified);
	
	void updateImageMetadataImpl(
		ImageId const& image_id,
		ImageMetadata const& metadata, bool* modified);
	
	std::vector<PageInfo> insertImageImpl(
		ImageInfo const& new_image, BeforeOrAfter before_or_after,
		ImageId const& existing, PageView view, bool& modified);

	void removePagesImpl(std::set<PageId> const& pages, bool& modified);

	PageInfo unremovePageImpl(PageId const& page_id, bool& modified);
	
	mutable QMutex m_mutex;
	std::vector<ImageDesc> m_images;
	PageId::SubPage m_subPagesInOrder[2];
};

#endif
