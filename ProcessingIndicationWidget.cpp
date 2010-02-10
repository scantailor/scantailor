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

#include "ProcessingIndicationWidget.h"
#include "imageproc/ColorInterpolation.h"
#include <QTimerEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QRectF>
#include <math.h>

using namespace imageproc;

static double const distinction_increase = 1.0 / 5.0;
static double const distinction_decrease = -1.0 / 3.0;

ProcessingIndicationWidget::ProcessingIndicationWidget(QWidget* parent)
:	QWidget(parent),
	m_animation(10),
	m_distinction(1.0),
	m_distinctionDelta(distinction_increase),
	m_timerId(0)
{
	m_headColor = palette().color(QPalette::Window).darker(200);
	m_tailColor = palette().color(QPalette::Window).darker(130);
}

void
ProcessingIndicationWidget::resetAnimation()
{
	m_distinction = 1.0;
	m_distinctionDelta = distinction_increase;
}

void
ProcessingIndicationWidget::processingRestartedEffect()
{
	m_distinction = 1.0;
	m_distinctionDelta = distinction_decrease;
}

void
ProcessingIndicationWidget::paintEvent(QPaintEvent* event)
{
	QRect animation_rect(animationRect());
	if (!event->rect().contains(animation_rect)) {
		update(animation_rect);
		return;
	}
	
	QColor head_color(colorInterpolation(m_tailColor, m_headColor, m_distinction));
	
	m_distinction += m_distinctionDelta;
	if (m_distinction > 1.0) {
		m_distinction = 1.0;
	} else if (m_distinction <= 0.0) {
		m_distinction = 0.0;
		m_distinctionDelta = distinction_increase;
	}
	
	QPainter painter(this);
	m_animation.nextFrame(head_color, m_tailColor, &painter, animation_rect);
	
	if (m_timerId == 0) {
		m_timerId = startTimer(180);
	}
}

void
ProcessingIndicationWidget::timerEvent(QTimerEvent* event)
{
	killTimer(event->timerId());
	m_timerId = 0;
	update(animationRect());
}

QRect
ProcessingIndicationWidget::animationRect() const
{
	QRect r(0, 0, 80, 80);
	r.moveCenter(rect().center());
	r &= rect();
	return r;
}
