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

#ifndef FILTERUIINTERFACE_H_
#define FILTERUIINTERFACE_H_

#include "PageId.h"
#include "AbstractCommand.h"
#include "IntrusivePtr.h"

class DebugImages;
class FilterOptionsWidget;
class QWidget;

/**
 * \brief A reduced interface to MainWindow to allow filters to manupulate the UI.
 */
class FilterUiInterface
{
public:
	enum Ownership { KEEP_OWNERSHIP, TRANSFER_OWNERSHIP };
	
	virtual ~FilterUiInterface() {}
	
	virtual void setOptionsWidget(
		FilterOptionsWidget* widget, Ownership ownership) = 0;
	
	virtual void setImageWidget(
		QWidget* widget, Ownership ownership,
		DebugImages* debug_images = 0) = 0;
	
	virtual void invalidateThumbnail(PageId const& page_id) = 0;
	
	virtual void invalidateAllThumbnails() = 0;

	/**
	 * Returns a callable object that when called will open a relinking dialog.
	 */
	virtual IntrusivePtr<AbstractCommand0<void> > relinkingDialogRequester() = 0;
};

#endif
