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

#ifndef ABSTRACTFILTER_H_
#define ABSTRACTFILTER_H_

#include "RefCountable.h"
#include "PageView.h"
#include "PageOrderOption.h"
#include <vector>

class FilterUiInterface;
class PageId;
class ProjectReader;
class ProjectWriter;
class AbstractRelinker;
class QString;
class QDomDocument;
class QDomElement;

/**
 * Filters represent processing stages, like "Deskew", "Margins" and "Output".
 */
class AbstractFilter : public RefCountable
{
public:
	virtual ~AbstractFilter() {}
	
	virtual QString getName() const = 0;
	
	virtual PageView getView() const = 0;
	
	virtual void selected() {}

	virtual int selectedPageOrder() const { return -1; }

	virtual void selectPageOrder(int option) {}

	virtual std::vector<PageOrderOption> pageOrderOptions() const {
		return std::vector<PageOrderOption>();
	}

	virtual void performRelinking(AbstractRelinker const& relinker) = 0;

	virtual void preUpdateUI(FilterUiInterface* ui, PageId const& page_id) = 0;
	
	virtual QDomElement saveSettings(
		ProjectWriter const& writer, QDomDocument& doc) const = 0;
	
	virtual void loadSettings(
		ProjectReader const& reader, QDomElement const& filters_el) = 0;
};

#endif
