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

#ifndef PAGE_SPLIT_SETTINGS_H_
#define PAGE_SPLIT_SETTINGS_H_

#include "RefCountable.h"
#include "NonCopyable.h"
#include "PageLayout.h"
#include "LayoutType.h"
#include "Params.h"
#include "ImageId.h"
#include "PageId.h"
#include <QMutex>
#include <memory>
#include <map>
#include <set>

class AbstractRelinker;

namespace page_split
{

class Settings : public RefCountable
{
	DECLARE_NON_COPYABLE(Settings)
private:
	class BaseRecord
	{
		// Member-wise copying is OK.
		friend class Settings;
	public:
		BaseRecord();
		
		LayoutType const* layoutType() const {
			return m_layoutTypeValid ? &m_layoutType : 0;
		}
		
		Params const* params() const {
			return m_paramsValid ? &m_params : 0;
		}
		
		/**
		 * \brief A record is considered null of it doesn't carry any
		 *        information.
		 */
		bool isNull() const {
			return !(m_paramsValid || m_layoutTypeValid);
		}
	protected:
		void setParams(Params const& params);
		
		void setLayoutType(LayoutType layout_type);
		
		void clearParams() { m_paramsValid = false; }
		
		void clearLayoutType() { m_layoutTypeValid = false; }
		
		/**
		 * \brief Checks if a particular layout type conflicts with PageLayout
		 *        that is part of Params.
		 */
		bool hasLayoutTypeConflict(LayoutType layout_type) const;
		
		Params m_params;
		LayoutType m_layoutType;
		bool m_paramsValid;
		bool m_layoutTypeValid;
	};
public:
	class UpdateAction;
	
	class Record : public BaseRecord
	{
		// Member-wise copying is OK.
	public:
		Record(LayoutType default_layout_type);
		
		Record(BaseRecord const& base_record,
			LayoutType default_layout_type);
		
		LayoutType combinedLayoutType() const;
		
		void update(UpdateAction const& action);
		
		bool hasLayoutTypeConflict() const;
	private:
		LayoutType m_defaultLayoutType;
	};
	
	
	class UpdateAction
	{
		friend class Settings::Record;
	public:
		UpdateAction() :
		m_params(PageLayout(), Dependencies(), MODE_AUTO),
		m_layoutType(AUTO_LAYOUT_TYPE),
		m_paramsAction(DONT_TOUCH),
		m_layoutTypeAction(DONT_TOUCH) {}
		
		void setLayoutType(LayoutType layout_type);
		
		void clearLayoutType();
		
		void setParams(Params const& params);
		
		void clearParams();
	private:
		enum Action { DONT_TOUCH, SET, CLEAR };
		
		Params m_params;
		LayoutType m_layoutType;
		Action m_paramsAction;
		Action m_layoutTypeAction;
	};
	
	
	Settings();
	
	virtual ~Settings();
	
	/**
	 * \brief Reset all settings to their initial state.
	 */
	void clear();

	void performRelinking(AbstractRelinker const& relinker);
	
	LayoutType defaultLayoutType() const;
	
	/**
	 * Sets layout type for all pages, removing other page
	 * parameters where they conflict with the new layout type.
	 */
	void setLayoutTypeForAllPages(LayoutType layout_type);
	
	/**
	 * Sets layout type for specified pages, removing other page
	 * parameters where they conflict with the new layout type.
	 */
	void setLayoutTypeFor(LayoutType layout_type, std::set<PageId> const& pages);
	
	/**
	 * \brief Returns all data related to a page as a single object.
	 */
	Record getPageRecord(ImageId const& image_id) const;
	
	/**
	 * \brief Performs the requested update on the page.
	 *
	 * If the update would lead to a conflict between the layout
	 * type and page parameters, the page parameters will be
	 * cleared.
	 */
	void updatePage(ImageId const& image_id, UpdateAction const& action);
	
	/**
	 * \brief Performs a conditional update on the page.
	 *
	 * If the update would lead to a conflict between the layout
	 * type and page parameters, the update won't take place.
	 * Whether the update took place or not, the current page record
	 * (updated or not) will be returned.
	 */
	Record conditionalUpdate(
		ImageId const& image_id, UpdateAction const& action,
		bool* conflict = 0);
private:
	typedef std::map<ImageId, BaseRecord> PerPageRecords;
	
	Record getPageRecordLocked(ImageId const& image_id) const;

	void updatePageLocked(ImageId const& image_id, UpdateAction const& action);

	void updatePageLocked(PerPageRecords::iterator it, UpdateAction const& action);
	
	mutable QMutex m_mutex;
	PerPageRecords m_perPageRecords;
	LayoutType m_defaultLayoutType;
};

} // namespace page_split

#endif
