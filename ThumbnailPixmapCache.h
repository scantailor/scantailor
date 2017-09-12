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

#ifndef THUMBNAILPIXMAPCACHE_H_
#define THUMBNAILPIXMAPCACHE_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "ThumbnailLoadResult.h"
#include "AbstractCommand.h"
#ifndef Q_MOC_RUN
#include <boost/weak_ptr.hpp>
#endif
#include <memory>

class ImageId;
class QImage;
class QPixmap;
class QString;
class QSize;

class ThumbnailPixmapCache : public RefCountable
{
	DECLARE_NON_COPYABLE(ThumbnailPixmapCache)
public:
	enum Status { LOADED, LOAD_FAILED, QUEUED };
	
	typedef AbstractCommand1<
		void, ThumbnailLoadResult const&
	> CompletionHandler;
	
	/**
	 * \brief Constructor.  To be called from the GUI thread only.
	 *
	 * \param thumb_dir The directory to store thumbnails in.  If the
	 *        provided directory doesn't exist, it will be created.
	 * \param max_size The maximum width and height for thumbnails.
	 *        The actual thumbnail size is going to depend on its aspect
	 *        ratio, but it won't exceed the provided maximum.
	 * \param max_cached_pixmaps The maximum number of pixmaps to store
	 *        in memory.
	 * \param expiration_threshold Requests are served from newest to
	 *        oldest ones. If a request is still not served after a certain
	 *        number of newer requests have been served, that request is
	 *        expired.  \p expiration_threshold specifies the exact number
	 *        of requests that cause older requests to expire.
	 *
	 * \see ThumbnailLoadResult::REQUEST_EXPIRED
	 */
	ThumbnailPixmapCache(QString const& thumb_dir, QSize const& max_size,
		int max_cached_pixmaps, int expiration_threshold);
	
	/**
	 * \brief Destructor.  To be called from the GUI thread only.
	 */
	virtual ~ThumbnailPixmapCache();
	
	void setThumbDir(QString const& thumb_dir);

	/**
	 * \brief Take the pixmap from cache, if it's there.
	 *
	 * If it's not, LOAD_FAILED will be returned.
	 *
	 * \note This function is to be called from the GUI thread only.
	 */
	Status loadFromCache(ImageId const& image_id, QPixmap& pixmap);
	
	/**
	 * \brief Take the pixmap from cache or from disk, blocking if necessary.
	 *
	 * \note This function is to be called from the GUI thread only.
	 */
	Status loadNow(ImageId const& image_id, QPixmap& pixmap);
	
	/**
	 * \brief Take the pixmap from cache or schedule a load request.
	 *
	 * If the pixmap is in cache, return it immediately.  Otherwise,
	 * schedule its loading in background.  Once the load request
	 * has been processed, the provided \p call_handler will be called.
	 *
	 * \note This function is to be called from the GUI thread only.
	 *
	 * \param image_id The identifier of the full size image and its thumbnail.
	 * \param[out] pixmap If the pixmap is cached, store it here.
	 * \param completion_handler A functor that will be called on request
	 * completion.  The best way to construct such a functor would be:
	 * \code
	 * class X : public boost::signals::trackable
	 * {
	 * public:
	 * 	void handleCompletion(ThumbnailLoadResult const& result);
	 * };
	 *
	 * X x;
	 * cache->loadRequest(image_id, pixmap, boost::bind(&X::handleCompletion, x, _1));
	 * \endcode
	 * Note that deriving X from boost::signals::trackable (with public inheritance)
	 * allows to safely delete the x object without worrying about callbacks
	 * it may receive in the future.  Keep in mind however, that deleting
	 * x is only safe when done from the GUI thread.  Another thing to
	 * keep in mind is that only boost::bind() can handle trackable binds.
	 * Other methods, for example boost::lambda::bind() can't do that.
	 */
	Status loadRequest(
		ImageId const& image_id, QPixmap& pixmap,
		boost::weak_ptr<CompletionHandler> const& completion_handler);
	
	/**
	 * \brief If no thumbnail exists for this image, create it.
	 *
	 * Using this function is optional.  It just presents an optimization
	 * opportunity.  Suppose you have the full size image already loaded,
	 * and want to avoid a second load when its thumbnail is requested.
	 *
	 * \param image_id The identifier of the full size image and its thumbnail.
	 * \param image The full-size image.
	 *
	 * \note This function may be called from any thread, even concurrently.
	 */
	void ensureThumbnailExists(ImageId const& image_id, QImage const& image);
	
	/**
	 * \brief Re-create and replace the existing thumnail.
	 *
	 * \param image_id The identifier of the full size image and its thumbnail.
	 * \param image The full-size image or a thumbnail.
	 *
	 * \note This function may be called from any thread, even concurrently.
	 */
	void recreateThumbnail(ImageId const& image_id, QImage const& image);
private:
	class Item;
	class Impl;
	
	std::auto_ptr<Impl> m_ptrImpl;
};

#endif
