/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef TABBED_DEBUG_IMAGES_H_
#define TABBED_DEBUG_IMAGES_H_

#include "DebugImageView.h"
#include <QTabWidget>
#ifndef Q_MOC_RUN
#include <boost/intrusive/list.hpp>
#endif

class TabbedDebugImages : public QTabWidget
{
	Q_OBJECT
public:
	TabbedDebugImages(QWidget* parent = 0);
private slots:
	void currentTabChanged(int idx);
private:
	typedef boost::intrusive::list<
		DebugImageView, boost::intrusive::constant_time_size<false>
	> DebugViewList;

	enum { MAX_LIVE_VIEWS = 3 };

	void removeExcessLiveViews();

	/**
	 * We don't want to keep all the debug images in memory.  We normally keep
	 * only a few of them.  This list holds references to them in the order
	 * they become live.
	 */
	DebugViewList m_liveViews;
};

#endif
