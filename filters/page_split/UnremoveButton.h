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

#ifndef UNREMOVE_BUTTON_H_
#define UNREMOVE_BUTTON_H_

#include "InteractionHandler.h"
#include "InteractionState.h"
#include "Proximity.h"
#include <QPointF>
#include <QPixmap>
#include <QCoreApplication>
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#endif

namespace page_split
{

class UnremoveButton : public InteractionHandler
{
	Q_DECLARE_TR_FUNCTIONS(page_split::UnremoveButton)
public:
	typedef boost::function<QPointF()> PositionGetter;
	typedef boost::function<void()> ClickCallback;

	UnremoveButton(PositionGetter const& position_getter);

	void setClickCallback(ClickCallback const& callback) {
		m_clickCallback = callback;
	}
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	virtual void onProximityUpdate(QPointF const& screen_mouse_pos, InteractionState& interaction);

	virtual void onMousePressEvent(QMouseEvent* event, InteractionState& interaction);
private:
	static void noOp() {}

	PositionGetter m_positionGetter;
	ClickCallback m_clickCallback;
	InteractionState::Captor m_proximityInteraction;
	QPixmap m_defaultPixmap;
	QPixmap m_hoveredPixmap;
	bool m_wasHovered;
};

} // namespace page_split

#endif
