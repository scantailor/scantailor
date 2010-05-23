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

#ifndef THUMBNAILCOLLECTOR_H_
#define THUMBNAILCOLLECTOR_H_

#include "AbstractFilterDataCollector.h"
#include <memory>

class ThumbnailPixmapCache;
class QGraphicsItem;
class QSizeF;

class ThumbnailCollector : public AbstractFilterDataCollector
{
public:
	virtual void processThumbnail(std::auto_ptr<QGraphicsItem>) = 0;
	
	virtual IntrusivePtr<ThumbnailPixmapCache> thumbnailCache() = 0;
	
	virtual QSizeF maxLogicalThumbSize() const = 0;
};

#endif
