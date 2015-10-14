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

#include "ThumbnailPixmapCache.h"
#include "ImageId.h"
#include "ImageLoader.h"
#include "AtomicFileOverwriter.h"
#include "RelinkablePath.h"
#include "OutOfMemoryHandler.h"
#include "imageproc/Scale.h"
#include "imageproc/GrayImage.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QString>
#include <QChar>
#include <QImage>
#include <QPixmap>
#include <QEvent>
#include <QSize>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/foreach.hpp>
#endif
#include <algorithm>
#include <vector>
#include <new>

using namespace ::boost;
using namespace ::boost::multi_index;
using namespace imageproc;

class ThumbnailPixmapCache::Item
{
public:
	enum Status {
		/**
		 * The background threaed hasn't touched it yet.
		 */
		QUEUED,
		
		/**
		 * The image is currently being loaded by a background
		 * thread, or it has been loaded, but the main thread
		 * hasn't yet received the loaded image, or it's currently
		 * converting it to a pixmap.
		 */
		IN_PROGRESS,
		
		/**
		 * The image was loaded and then converted to a pixmap
		 * by the main thread.
		 */
		LOADED,
		
		/**
		 * The image could not be loaded.
		 */
		LOAD_FAILED
	};
	
	ImageId imageId;
	
	mutable QPixmap pixmap; /**< Guaranteed to be set if status is LOADED */
	
	mutable std::vector<
		boost::weak_ptr<CompletionHandler>
	> completionHandlers;
	
	/**
	 * The total image loading attempts (of any images) by
	 * ThumbnailPixmapCache at the time of the creation of this item.
	 * This information is used for request expiration.
	 * \see ThumbnailLoadResult::REQUEST_EXPIRED
	 */
	int precedingLoadAttempts;
	
	mutable Status status;
	
	Item(ImageId const& image_id, int preceding_load_attepmts, Status status);
	
	Item(Item const& other);
private:
	Item& operator=(Item const& other); // Assignment is forbidden.
};


class ThumbnailPixmapCache::Impl : public QThread
{
public:
	Impl(QString const& thumb_dir, QSize const& max_thumb_size,
		int max_cached_pixmaps, int expiration_threshold);
	
	~Impl();

	void setThumbDir(QString const& thumb_dir);
	
	Status request(
		ImageId const& image_id, QPixmap& pixmap, bool load_now = false,
		boost::weak_ptr<CompletionHandler> const* completion_handler = 0);
	
	void ensureThumbnailExists(ImageId const& image_id, QImage const& image);
	
	void recreateThumbnail(ImageId const& image_id, QImage const& image);
protected:
	virtual void run();
	
	virtual void customEvent(QEvent* e);
private:
	class LoadResultEvent;
	class ItemsByKeyTag;
	class LoadQueueTag;
	class RemoveQueueTag;
	
	typedef multi_index_container<
		Item,
		indexed_by<
			ordered_unique<tag<ItemsByKeyTag>, member<Item, ImageId, &Item::imageId> >,
			sequenced<tag<LoadQueueTag> >,
			sequenced<tag<RemoveQueueTag> >
		>
	> Container;
	
	typedef Container::index<ItemsByKeyTag>::type ItemsByKey;
	typedef Container::index<LoadQueueTag>::type LoadQueue;
	typedef Container::index<RemoveQueueTag>::type RemoveQueue;
	
	class BackgroundLoader : public QObject
	{
	public:
		BackgroundLoader(Impl& owner);
	protected:
		virtual void customEvent(QEvent* e);
	private:
		Impl& m_rOwner;
	};
	
	void backgroundProcessing();
	
	static QImage loadSaveThumbnail(
		ImageId const& image_id, QString const& thumb_dir,
		QSize const& max_thumb_size);
	
	static QString getThumbFilePath(
		ImageId const& image_id, QString const& thumb_dir);
	
	static QImage makeThumbnail(
		QImage const& image, QSize const& max_thumb_size);
	
	void queuedToInProgress(LoadQueue::iterator const& lq_it);
	
	void postLoadResult(
		LoadQueue::iterator const& lq_it, QImage const& image,
		ThumbnailLoadResult::Status status);
	
	void processLoadResult(LoadResultEvent* result);
	
	void removeExcessLocked();
	
	void removeItemLocked(RemoveQueue::iterator const& it);
	
	void cachePixmapUnlocked(ImageId const& image_id, QPixmap const& pixmap);
	
	void cachePixmapLocked(ImageId const& image_id, QPixmap const& pixmap);
	
	mutable QMutex m_mutex;
	BackgroundLoader m_backgroundLoader;
	Container m_items;
	ItemsByKey& m_itemsByKey; /**< ImageId => Item mapping */
	
	/**
	 * An "std::list"-like view of QUEUED items in the order they are
	 * going to be loaded.  Actually the list contains all kinds of items,
	 * but all QUEUED ones precede any others.  New QUEUED items are added
	 * to the front of this list for purposes of request expiration.
	 * \see ThumbnailLoadResult::REQUEST_EXPIRED
	 */
	LoadQueue& m_loadQueue;
	
	/**
	 * An "std::list"-like view of LOADED items in the order they are
	 * going to be removed. Actually the list contains all kinds of items,
	 * but all LOADED ones precede any others.  Note that we don't bother
	 * removing items without a pixmap, which would be all except LOADED
	 * items.  New LOADED items are added after the last LOADED item
	 * already present in the list.
	 */
	RemoveQueue& m_removeQueue;
	
	/**
	 * An iterator of m_removeQueue that marks the end of LOADED items.
	 */
	RemoveQueue::iterator m_endOfLoadedItems;
	
	QString m_thumbDir;
	QSize m_maxThumbSize;
	int m_maxCachedPixmaps;
	
	/**
	 * \see ThumbnailPixmapCache::ThumbnailPixmapCache()
	 */
	int m_expirationThreshold;
	
	int m_numQueuedItems;
	int m_numLoadedItems;
	
	/**
	 * Total image loading attempts so far.  Used for request expiration.
	 * \see ThumbnailLoadResult::REQUEST_EXPIRED
	 */
	int m_totalLoadAttempts;
	
	bool m_threadStarted;
	bool m_shuttingDown;
};


class ThumbnailPixmapCache::Impl::LoadResultEvent : public QEvent
{
public:
	LoadResultEvent(Impl::LoadQueue::iterator const& lq_t,
		QImage const& image, ThumbnailLoadResult::Status status);
	
	virtual ~LoadResultEvent();
	
	Impl::LoadQueue::iterator lqIter() const { return m_lqIter; }
	
	QImage const& image() const { return m_image; }
	
	void releaseImage() { m_image = QImage(); }
	
	ThumbnailLoadResult::Status status() const { return m_status; }
private:
	Impl::LoadQueue::iterator m_lqIter;
	QImage m_image;
	ThumbnailLoadResult::Status m_status;
};


/*========================== ThumbnailPixmapCache ===========================*/

ThumbnailPixmapCache::ThumbnailPixmapCache(
	QString const& thumb_dir, QSize const& max_thumb_size,
	int const max_cached_pixmaps, int const expiration_threshold)
:	m_ptrImpl(
		new Impl(
			RelinkablePath::normalize(thumb_dir), max_thumb_size,
			max_cached_pixmaps, expiration_threshold
		)
	)
{
}

ThumbnailPixmapCache::~ThumbnailPixmapCache()
{
}

void
ThumbnailPixmapCache::setThumbDir(QString const& thumb_dir)
{
	m_ptrImpl->setThumbDir(RelinkablePath::normalize(thumb_dir));
}

ThumbnailPixmapCache::Status
ThumbnailPixmapCache::loadFromCache(ImageId const& image_id, QPixmap& pixmap)
{
	return m_ptrImpl->request(image_id, pixmap);
}

ThumbnailPixmapCache::Status
ThumbnailPixmapCache::loadNow(ImageId const& image_id, QPixmap& pixmap)
{
	return m_ptrImpl->request(image_id, pixmap, true);
}

ThumbnailPixmapCache::Status
ThumbnailPixmapCache::loadRequest(
	ImageId const& image_id, QPixmap& pixmap,
	boost::weak_ptr<CompletionHandler> const& completion_handler)
{
	return m_ptrImpl->request(image_id, pixmap, false, &completion_handler);
}

void
ThumbnailPixmapCache::ensureThumbnailExists(
	ImageId const& image_id, QImage const& image)
{
	m_ptrImpl->ensureThumbnailExists(image_id, image);
}

void
ThumbnailPixmapCache::recreateThumbnail(
	ImageId const& image_id, QImage const& image)
{
	m_ptrImpl->recreateThumbnail(image_id, image);
}


/*======================= ThumbnailPixmapCache::Impl ========================*/

ThumbnailPixmapCache::Impl::Impl(
	QString const& thumb_dir, QSize const& max_thumb_size,
	int const max_cached_pixmaps, int const expiration_threshold)
:	m_backgroundLoader(*this),
	m_items(),
	m_itemsByKey(m_items.get<ItemsByKeyTag>()),
	m_loadQueue(m_items.get<LoadQueueTag>()),
	m_removeQueue(m_items.get<RemoveQueueTag>()),
	m_endOfLoadedItems(m_removeQueue.end()),
	m_thumbDir(thumb_dir),
	m_maxThumbSize(max_thumb_size),
	m_maxCachedPixmaps(max_cached_pixmaps),
	m_expirationThreshold(expiration_threshold),
	m_numQueuedItems(0),
	m_numLoadedItems(0),
	m_totalLoadAttempts(0),
	m_threadStarted(false),
	m_shuttingDown(false)
{
	// Note that QDir::mkdir() will fail if the parent directory,
	// that is $OUT/cache doesn't exist. We want that behaviour,
	// as otherwise when loading a project from a different machine,
	// a whole bunch of bogus directories would be created.
	QDir().mkdir(m_thumbDir);

	m_backgroundLoader.moveToThread(this);
}

ThumbnailPixmapCache::Impl::~Impl()
{
	{
		QMutexLocker const locker(&m_mutex);
		
		if (!m_threadStarted) {
			return;
		}
		
		m_shuttingDown = true;
	}
	
	quit();
	wait();
}

void
ThumbnailPixmapCache::Impl::setThumbDir(QString const& thumb_dir)
{
	QMutexLocker locker(&m_mutex);

	if (thumb_dir == m_thumbDir) {
		return;
	}

	m_thumbDir = thumb_dir;

	BOOST_FOREACH(Item const& item, m_loadQueue) {
		// This trick will make all queued tasks to expire.
		m_totalLoadAttempts = std::max(
			m_totalLoadAttempts,
			item.precedingLoadAttempts + m_expirationThreshold + 1
		);
	}
}

ThumbnailPixmapCache::Status
ThumbnailPixmapCache::Impl::request(
	ImageId const& image_id, QPixmap& pixmap, bool const load_now,
	boost::weak_ptr<CompletionHandler> const* completion_handler)
{
	assert(QCoreApplication::instance()->thread() == QThread::currentThread());
	
	QMutexLocker locker(&m_mutex);
	
	if (m_shuttingDown) {
		return LOAD_FAILED;
	}
	
	ItemsByKey::iterator const k_it(m_itemsByKey.find(image_id));
	if (k_it != m_itemsByKey.end()) {
		if (k_it->status == Item::LOADED) {
			pixmap = k_it->pixmap;
			
			// Move it after all other candidates for removal.
			RemoveQueue::iterator const rq_it(
				m_items.project<RemoveQueueTag>(k_it)
			);
			m_removeQueue.relocate(m_endOfLoadedItems, rq_it);
			
			return LOADED;
		} else if (k_it->status == Item::LOAD_FAILED) {
			pixmap = k_it->pixmap;
			return LOAD_FAILED;
		}
	}
	
	if (load_now) {
		QString const thumb_dir(m_thumbDir);
		QSize const max_thumb_size(m_maxThumbSize);
		
		locker.unlock();
		
		pixmap = QPixmap::fromImage(
			loadSaveThumbnail(image_id, thumb_dir, max_thumb_size)
		);
		if (pixmap.isNull()) {
			return LOAD_FAILED;
		}
		
		cachePixmapUnlocked(image_id, pixmap);
		return LOADED;
	}
	
	if (!completion_handler) {
		return LOAD_FAILED;
	}
	
	if (k_it != m_itemsByKey.end()) {
		assert(k_it->status == Item::QUEUED || k_it->status == Item::IN_PROGRESS);
		k_it->completionHandlers.push_back(*completion_handler);
		
		if (k_it->status == Item::QUEUED) {
			// Because we've got a new request for this item,
			// we move it to the beginning of the load queue.
			// Note that we don't do it for IN_PROGRESS items,
			// because all QUEUED items must precede any other
			// items in the load queue.
			LoadQueue::iterator const lq_it(
				m_items.project<LoadQueueTag>(k_it)
			);
			m_loadQueue.relocate(m_loadQueue.begin(), lq_it);
		}
		
		return QUEUED;
	}
	
	// Create a new item.
	LoadQueue::iterator const lq_it(
		m_loadQueue.push_front(
			Item(image_id, m_totalLoadAttempts, Item::QUEUED)
		).first
	);
	// Now our new item is at the beginning of the load queue and at the
	// end of the remove queue.
	
	assert(lq_it->status == Item::QUEUED);
	assert(lq_it->completionHandlers.empty());
	
	if (m_endOfLoadedItems == m_removeQueue.end()) {
		m_endOfLoadedItems = m_items.project<RemoveQueueTag>(lq_it);
	}
	lq_it->completionHandlers.push_back(*completion_handler);
	
	if (m_numQueuedItems++ == 0) {
		if (m_threadStarted) {
			// Wake the background thread up.
			QCoreApplication::postEvent(
				&m_backgroundLoader, new QEvent(QEvent::User)
			);
		} else {
			// Start the background thread.
			start();
			m_threadStarted = true;
		}
	}
	
	return QUEUED;
}

void
ThumbnailPixmapCache::Impl::ensureThumbnailExists(
	ImageId const& image_id, QImage const& image)
{
	if (m_shuttingDown) {
		return;
	}
	
	if (image.isNull()) {
		return;
	}
	
	QMutexLocker locker(&m_mutex);
	QString const thumb_dir(m_thumbDir);
	QSize const max_thumb_size(m_maxThumbSize);
	locker.unlock();
	
	QString const thumb_file_path(getThumbFilePath(image_id, thumb_dir));
	if (QFile::exists(thumb_file_path)) {
		return;
	}
	
	QImage const thumbnail(makeThumbnail(image, max_thumb_size));
	
	AtomicFileOverwriter overwriter;
	QIODevice* iodev = overwriter.startWriting(thumb_file_path);
	if (iodev && thumbnail.save(iodev, "PNG")) {
		overwriter.commit();
	}
}

void
ThumbnailPixmapCache::Impl::recreateThumbnail(
	ImageId const& image_id, QImage const& image)
{
	if (m_shuttingDown) {
		return;
	}
	
	if (image.isNull()) {
		return;
	}
	
	QMutexLocker locker(&m_mutex);
	QString const thumb_dir(m_thumbDir);
	QSize const max_thumb_size(m_maxThumbSize);
	locker.unlock();
	
	QString const thumb_file_path(getThumbFilePath(image_id, thumb_dir));
	QImage const thumbnail(makeThumbnail(image, max_thumb_size));
	bool thumb_written = false;
	
	// Note that we may be called from multiple threads at the same time.
	AtomicFileOverwriter overwriter;
	QIODevice* iodev = overwriter.startWriting(thumb_file_path);
	if (iodev && thumbnail.save(iodev, "PNG")) {
		thumb_written = overwriter.commit();
	} else {
		overwriter.abort();
	}
	
	if (!thumb_written) {
		return;
	}
	
	QMutexLocker const locker2(&m_mutex);
	
	ItemsByKey::iterator const k_it(m_itemsByKey.find(image_id));
	if (k_it == m_itemsByKey.end()) {
		return;
	}
	
	switch (k_it->status) {
		case Item::LOADED:
		case Item::LOAD_FAILED:
			removeItemLocked(m_items.project<RemoveQueueTag>(k_it));
			break;
		case Item::QUEUED:
			break;
		case Item::IN_PROGRESS:
			// We have a small race condition in this case.
			// We don't know if the other thread has already loaded
			// the thumbnail or not.  In case it did, again we
			// don't know if it loaded the old or new version.
			// Well, let's just pretend the thumnail was loaded
			// (or failed to load) before we wrote the new version.
			break;
	}
}

void
ThumbnailPixmapCache::Impl::run()
{
	backgroundProcessing();
	exec(); // Wait for further processing requests (via custom events).
}

void
ThumbnailPixmapCache::Impl::customEvent(QEvent* e)
{
	processLoadResult(dynamic_cast<LoadResultEvent*>(e));
}

void
ThumbnailPixmapCache::Impl::backgroundProcessing()
{
	// This method is called from a background thread.
	assert(QCoreApplication::instance()->thread() != QThread::currentThread());
	
	for (;;) {
		try {
			// We are going to initialize these while holding the mutex.
			LoadQueue::iterator lq_it;
			ImageId image_id;
			QString thumb_dir;
			QSize max_thumb_size;

			{
				QMutexLocker const locker(&m_mutex);

				if (m_shuttingDown || m_items.empty()) {
					break;
				}

				lq_it = m_loadQueue.begin();
				image_id = lq_it->imageId;

				if (lq_it->status != Item::QUEUED) {
					// All QUEUED items precede any other items
					// in the load queue, so it means there are no
					// QUEUED items at all.
					assert(m_numQueuedItems == 0);
					break;
				}

				// By marking the item as IN_PROGRESS, we prevent it
				// from being processed again before the GUI thread
				// receives our LoadResultEvent.
				queuedToInProgress(lq_it);

				if (m_totalLoadAttempts - lq_it->precedingLoadAttempts
						> m_expirationThreshold) {

					// Expire this request.  The reasoning behind
					// request expiration is described in
					// ThumbnailLoadResult::REQUEST_EXPIRED
					// documentation.

					postLoadResult(
						lq_it, QImage(),
						ThumbnailLoadResult::REQUEST_EXPIRED
					);
					continue;
				}

				// Expired requests don't count as load attempts.
				++m_totalLoadAttempts;

				// Copy those while holding the mutex.
				thumb_dir = m_thumbDir;
				max_thumb_size = m_maxThumbSize;
			} // mutex scope

			QImage const image(
				loadSaveThumbnail(image_id, thumb_dir, max_thumb_size)
			);

			ThumbnailLoadResult::Status const status = image.isNull()
				? ThumbnailLoadResult::LOAD_FAILED
				: ThumbnailLoadResult::LOADED;
			postLoadResult(lq_it, image, status);
		} catch (std::bad_alloc const&) {
			OutOfMemoryHandler::instance().handleOutOfMemorySituation();
		}
	}
}

QImage
ThumbnailPixmapCache::Impl::loadSaveThumbnail(
	ImageId const& image_id, QString const& thumb_dir,
	QSize const& max_thumb_size)
{
	QString const thumb_file_path(getThumbFilePath(image_id, thumb_dir));
	
	QImage image(ImageLoader::load(thumb_file_path, 0));
	if (!image.isNull()) {
		return image;
	}
	
	image = ImageLoader::load(image_id);
	if (image.isNull()) {
		return QImage();
	}
	
	QImage const thumbnail(makeThumbnail(image, max_thumb_size));
	thumbnail.save(thumb_file_path, "PNG");
	
	return thumbnail;
}

QString
ThumbnailPixmapCache::Impl::getThumbFilePath(
	ImageId const& image_id, QString const& thumb_dir)
{
	// Because a project may have several files with the same name (from
	// different directories), we add a hash of the original image path
	// to the thumbnail file name.
	
	QByteArray const orig_path_hash(
		QCryptographicHash::hash(
			image_id.filePath().toUtf8(), QCryptographicHash::Md5
		).toHex()
	);
	QString const orig_path_hash_str(
		QString::fromAscii(orig_path_hash.data(), orig_path_hash.size())
	);
	
	QFileInfo const orig_img_path(image_id.filePath());
	QString thumb_file_path(thumb_dir);
	thumb_file_path += QChar('/');
	thumb_file_path += orig_img_path.baseName();
	thumb_file_path += QChar('_');
	thumb_file_path += QString::number(image_id.zeroBasedPage());
	thumb_file_path += QChar('_');
	thumb_file_path += orig_path_hash_str;
	thumb_file_path += QString::fromAscii(".png");
	
	return thumb_file_path;
}

QImage
ThumbnailPixmapCache::Impl::makeThumbnail(
	QImage const& image, QSize const& max_thumb_size)
{
	if (image.width() < max_thumb_size.width() &&
	    image.height() < max_thumb_size.height()) {
		return image;
	}
	
	QSize to_size(image.size());
	to_size.scale(max_thumb_size, Qt::KeepAspectRatio);
	
	if (image.format() == QImage::Format_Indexed8 && image.isGrayscale()) {
		// This will be faster than QImage::scale().
		return scaleToGray(GrayImage(image), to_size);
	}
	
	return image.scaled(
		to_size,
		Qt::KeepAspectRatio, Qt::SmoothTransformation
	);
}

void
ThumbnailPixmapCache::Impl::queuedToInProgress(LoadQueue::iterator const& lq_it)
{
	assert(lq_it->status == Item::QUEUED);
	lq_it->status = Item::IN_PROGRESS;
	
	assert(m_numQueuedItems > 0);
	--m_numQueuedItems;
	
	// Move it item to the end of load queue.
	// The point is to keep QUEUED items before any others.
	m_loadQueue.relocate(m_loadQueue.end(), lq_it);
	
	// Going from QUEUED to IN_PROGRESS doesn't require
	// moving it in the remove queue, as we only remove
	// LOADED items.
}

void
ThumbnailPixmapCache::Impl::postLoadResult(
	LoadQueue::iterator const& lq_it, QImage const& image,
	ThumbnailLoadResult::Status const status)
{
	LoadResultEvent* e = new LoadResultEvent(lq_it, image, status);
	QCoreApplication::postEvent(this, e);
}

void
ThumbnailPixmapCache::Impl::processLoadResult(LoadResultEvent* result)
{
	assert(QCoreApplication::instance()->thread() == QThread::currentThread());
	
	QPixmap pixmap(QPixmap::fromImage(result->image()));
	result->releaseImage();
	
	std::vector<boost::weak_ptr<CompletionHandler> > completion_handlers;
	
	{
		QMutexLocker const locker(&m_mutex);
		
		if (m_shuttingDown) {
			return;
		}
		
		LoadQueue::iterator const lq_it(result->lqIter());
		RemoveQueue::iterator const rq_it(
			m_items.project<RemoveQueueTag>(lq_it)
		);
		
		Item const& item = *lq_it;
		
		if (result->status() == ThumbnailLoadResult::LOADED
				&& pixmap.isNull()) {
			// That's a special case caused by cachePixmapLocked().
			assert(!item.pixmap.isNull());
		} else {
			item.pixmap = pixmap;
		}
		item.completionHandlers.swap(completion_handlers);
		
		if (result->status() == ThumbnailLoadResult::LOADED) {
			// Maybe remove an older item.
			removeExcessLocked();
			
			item.status = Item::LOADED;
			++m_numLoadedItems;
			
			// Move this item after all other LOADED items in
			// the remove queue.
			m_removeQueue.relocate(m_endOfLoadedItems, rq_it);
			
			// Move to the end of load queue.
			m_loadQueue.relocate(m_loadQueue.end(), lq_it);
		} else if (result->status() == ThumbnailLoadResult::LOAD_FAILED) {
			// We keep items that failed to load, as they are cheap
			// to keep and helps us avoid trying to load them
			// again and again.
			
			item.status = Item::LOAD_FAILED;
			
			// Move to the end of load queue.
			m_loadQueue.relocate(m_loadQueue.end(), lq_it);
		} else {
			assert(result->status() == ThumbnailLoadResult::REQUEST_EXPIRED);
			
			// Just remove it.
			removeItemLocked(rq_it);
		}
	} // mutex scope
	
	// Notify listeners.
	ThumbnailLoadResult const load_result(result->status(), pixmap);
	typedef boost::weak_ptr<CompletionHandler> WeakHandler;
	BOOST_FOREACH (WeakHandler const& wh, completion_handlers) {
		boost::shared_ptr<CompletionHandler> const sh(wh.lock());
		if (sh.get()) {
			(*sh)(load_result);
		}
	}
}

void
ThumbnailPixmapCache::Impl::removeExcessLocked()
{
	if (m_numLoadedItems >= m_maxCachedPixmaps) {
		assert(m_numLoadedItems > 0);
		assert(!m_removeQueue.empty());
		assert(m_removeQueue.front().status == Item::LOADED);
		removeItemLocked(m_removeQueue.begin());
	}
}

void
ThumbnailPixmapCache::Impl::removeItemLocked(
	RemoveQueue::iterator const& it)
{
	switch (it->status) {
		case Item::QUEUED:
			assert(m_numQueuedItems > 0);
			--m_numQueuedItems;
			break;
		case Item::LOADED:
			assert(m_numLoadedItems > 0);
			--m_numLoadedItems;
			break;
		default:;
	}
	
	if (m_endOfLoadedItems == it) {
		++m_endOfLoadedItems;
	}
	
	m_removeQueue.erase(it);
}

void
ThumbnailPixmapCache::Impl::cachePixmapUnlocked(
	ImageId const& image_id, QPixmap const& pixmap)
{
	QMutexLocker const locker(&m_mutex);
	cachePixmapLocked(image_id, pixmap);
}

void
ThumbnailPixmapCache::Impl::cachePixmapLocked(
	ImageId const& image_id, QPixmap const& pixmap)
{
	if (m_shuttingDown) {
		return;
	}
	
	Item::Status const new_status = pixmap.isNull()
			? Item::LOAD_FAILED : Item::LOADED;
	
	// Check if such item already exists.
	ItemsByKey::iterator const k_it(m_itemsByKey.find(image_id));
	if (k_it == m_itemsByKey.end()) {
		// Existing item not found.
		
		// Maybe remove an older item.
		removeExcessLocked();
		
		// Insert our new item.
		RemoveQueue::iterator const rq_it(
			m_removeQueue.insert(
				m_endOfLoadedItems,
				Item(image_id, m_totalLoadAttempts, new_status)
			).first
		);
		// Our new item is now after all LOADED items in the
		// remove queue and at the end of the load queue.
		
		if (new_status == Item::LOAD_FAILED) {
			--m_endOfLoadedItems;
		}
		
		rq_it->pixmap = pixmap;
		
		assert(rq_it->completionHandlers.empty());
		return;
	}
	
	switch (k_it->status) {
		case Item::LOADED:
			// There is no point in replacing LOADED items.
		case Item::IN_PROGRESS:
			// It's unsafe to touch IN_PROGRESS items.
			return;
		default:
			break;
	}
	
	if (new_status == Item::LOADED && k_it->status == Item::QUEUED) {
		// Not so fast.  We can't go from QUEUED to LOADED directly.
		// Well, maybe we can, but we'd have to invoke the completion
		// handlers right now.  We'd rather do it asynchronously,
		// so let's transition it to IN_PROGRESS and send
		// a LoadResultEvent asynchronously.
		
		assert(!k_it->completionHandlers.empty());
		
		LoadQueue::iterator const lq_it(
			m_items.project<LoadQueueTag>(k_it)
		);
		
		lq_it->pixmap = pixmap;
		queuedToInProgress(lq_it);
		postLoadResult(lq_it, QImage(), ThumbnailLoadResult::LOADED);
		return;
	}
	
	assert(k_it->status == Item::LOAD_FAILED);
	
	k_it->status = new_status;
	k_it->pixmap = pixmap;
	
	if (new_status == Item::LOADED) {
		RemoveQueue::iterator const rq_it(
			m_items.project<RemoveQueueTag>(k_it)
		);
		m_removeQueue.relocate(m_endOfLoadedItems, rq_it);
		++m_numLoadedItems;
	}
}


/*====================== ThumbnailPixmapCache::Item =========================*/

ThumbnailPixmapCache::Item::Item(ImageId const& image_id,
	int const preceding_load_attempts, Status const st)
:	imageId(image_id),
	precedingLoadAttempts(preceding_load_attempts),
	status(st)
{
}

ThumbnailPixmapCache::Item::Item(Item const& other)
:	imageId(other.imageId),
	pixmap(other.pixmap),
	completionHandlers(other.completionHandlers),
	precedingLoadAttempts(other.precedingLoadAttempts),
	status(other.status)
{
}


/*=============== ThumbnailPixmapCache::Impl::LoadResultEvent ===============*/

ThumbnailPixmapCache::Impl::LoadResultEvent::LoadResultEvent(
	Impl::LoadQueue::iterator const& lq_it, QImage const& image,
	ThumbnailLoadResult::Status const status)
:	QEvent(QEvent::User),
	m_lqIter(lq_it),
	m_image(image),
	m_status(status)
{
}

ThumbnailPixmapCache::Impl::LoadResultEvent::~LoadResultEvent()
{
}


/*================== ThumbnailPixmapCache::BackgroundLoader =================*/

ThumbnailPixmapCache::Impl::BackgroundLoader::BackgroundLoader(Impl& owner)
:	m_rOwner(owner)
{
}

void
ThumbnailPixmapCache::Impl::BackgroundLoader::customEvent(QEvent*)
{
	m_rOwner.backgroundProcessing();
}
