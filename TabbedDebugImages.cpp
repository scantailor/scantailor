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

#include "TabbedDebugImages.h.moc"
#include "DebugImageView.h"

TabbedDebugImages::TabbedDebugImages(QWidget* parent)
:	QTabWidget(parent)
{
	setDocumentMode(true);
	connect(this, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));
}

void
TabbedDebugImages::currentTabChanged(int const idx)
{
	if (DebugImageView* div = dynamic_cast<DebugImageView*>(widget(idx))) {
		div->unlink();
		m_liveViews.push_back(*div);
		removeExcessLiveViews();
		div->setLive(true);
	}
}

void
TabbedDebugImages::removeExcessLiveViews()
{
	int remaining = m_liveViews.size();
	for (; remaining > MAX_LIVE_VIEWS; --remaining) {
		m_liveViews.front().setLive(false);
		m_liveViews.erase(m_liveViews.begin());
	}
}
