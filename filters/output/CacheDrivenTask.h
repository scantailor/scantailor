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

#ifndef OUTPUT_CACHEDRIVENTASK_H_
#define OUTPUT_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "OutputFileNameGenerator.h"

class QPolygonF;
class PageInfo;
class AbstractFilterDataCollector;
class ImageTransformation;

namespace output
{

class Settings;

class CacheDrivenTask : public RefCountable
{
	DECLARE_NON_COPYABLE(CacheDrivenTask)
public:
	CacheDrivenTask(
		IntrusivePtr<Settings> const& settings,
		OutputFileNameGenerator const& out_file_name_gen);
	
	virtual ~CacheDrivenTask();
	
	void process(
		PageInfo const& page_info, AbstractFilterDataCollector* collector,
		ImageTransformation const& xform, QPolygonF const& content_rect_phys);
private:
	IntrusivePtr<Settings> m_ptrSettings;
	OutputFileNameGenerator m_outFileNameGen;
};

} // namespace output

#endif
