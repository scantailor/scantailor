/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef DRAG_HANDLER_H_
#define DRAG_HANDLER_H_

#include "InteractionHandler.h"
#include "InteractionState.h"
#include <QPoint>
#include <QCoreApplication>

class ImageViewBase;

class DragHandler : public InteractionHandler
{
	Q_DECLARE_TR_FUNCTIONS(DragHandler)
public:
	DragHandler(ImageViewBase& image_view);

	bool isActive() const;
protected:
	virtual void onMousePressEvent(QMouseEvent* event, InteractionState& interaction);

	virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

	virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction);
private:
	ImageViewBase& m_rImageView;
	InteractionState::Captor m_interaction;
	QPoint m_lastMousePos;
};

#endif
