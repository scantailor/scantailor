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

#ifndef PROCESSING_INDICATION_WIDGET_H_
#define PROCESSING_INDICATION_WIDGET_H_

#include "BubbleAnimation.h"
#include <QWidget>
#include <QColor>

class QRect;

/**
 * \brief This widget is displayed in the central area od the main window
 *        when an image is being processed.
 */
class ProcessingIndicationWidget : public QWidget
{
public:
	ProcessingIndicationWidget(QWidget* parent = 0);
	
	/**
	 * \brief Resets animation to the state it had just after
	 *        constructing this object.
	 */
	void resetAnimation();

	/**
	 * \brief Launch the "processing restarted" effect.
	 */
	void processingRestartedEffect();
protected:
	virtual void paintEvent(QPaintEvent* event);
	
	virtual void timerEvent(QTimerEvent* event);
private:
	QRect animationRect() const;
	
	BubbleAnimation m_animation;
	QColor m_headColor;
	QColor m_tailColor;
	double m_distinction;
	double m_distinctionDelta;
	int m_timerId;
};

#endif
