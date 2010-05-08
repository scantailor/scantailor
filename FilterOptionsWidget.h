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

#ifndef FILTEROPTIONSWIDGET_H_
#define FILTEROPTIONSWIDGET_H_

#include "PageId.h"
#include "PageInfo.h"
#include <QWidget>

class FilterOptionsWidget : public QWidget
{
	Q_OBJECT
signals:
	/**
	 * \brief To be emitted by subclasses when they want to reload the page.
	 */
	void reloadRequested();
	
	void invalidateThumbnail(PageId const& page_id);

	/**
	 * This signature differs from invalidateThumbnail(PageId) in that
	 * it will cause PageInfo stored by ThumbnailSequence to be updated.
	 */
	void invalidateThumbnail(PageInfo const& page_info);
	
	void invalidateAllThumbnails();
	
	/**
	 * After we've got rid of "Widest Page" / "Tallest Page" links,
	 * there is no one using this signal.  It's a candidate for removal.
	 */
	void goToPage(PageId const& page_id);
};

#endif
