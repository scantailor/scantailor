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

#ifndef THUMBNAILLOADRESULT_H_
#define THUMBNAILLOADRESULT_H_

#include <QPixmap>

class ThumbnailLoadResult
{
public:
	enum Status {
		/**
		 * \brief Thumbnail loaded successfully.  Pixmap is not null.
		 */
		LOADED,
		
		/**
		 * \brief Thumbnail failed to load.  Pixmap is null.
		 */
		LOAD_FAILED,
		
		/**
		 * \brief Request has expired.  Pixmap is null.
		 *
		 * Consider the following situation: we scroll our thumbnail
		 * list from the beginning all the way to the end.  This will
		 * result in every thumbnail being requested.  If we just
		 * load them in request order, that would be quite slow and
		 * inefficient.  It would be nice if we could cancel the load
		 * requests for items that went out of view.  Unfortunately,
		 * QGraphicsView doesn't provide "went out of view"
		 * notifications.  Instead, we load thumbnails starting from
		 * most recently requested, and expire requests after a certain
		 * number of newer requests are processed.  If the client is
		 * still interested in the thumbnail, it may request it again.
		 */
		REQUEST_EXPIRED
	};
	
	ThumbnailLoadResult(Status status, QPixmap const& pixmap)
	: m_pixmap(pixmap), m_status(status) {}
	
	Status status() const { return m_status; }
	
	QPixmap const& pixmap() const { return m_pixmap; }
private:
	QPixmap m_pixmap;
	Status m_status;
};

#endif
