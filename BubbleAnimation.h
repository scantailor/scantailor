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

#ifndef BUBBLEANIMATION_H_
#define BUBBLEANIMATION_H_

#include <QRectF>

class QColor;
class QPaintDevice;
class QPainter;

/**
 * \brief Renders a sequence of frames with a circle of bubles of
 *        varying colors.
 */
class BubbleAnimation
{
public:
	BubbleAnimation(int num_bubbles);
	
	/**
	 * \brief Renders the next frame of the animation.
	 *
	 * \param head_color The color of the head of the string of bubbles.
	 * \param tail_color The color of the tail of the string of bubbles.
	 * \param pd The device to paint to.
	 * \param rect The rectangle in device coordinates to render to.
	 *        A null rectangle indicates the whole device area
	 *        is to be used.
	 * \return Whether more frames follow.  After returning false,
	 *         the next call will render the first frame again.
	 */
	bool nextFrame(
		QColor const& head_color, QColor const& tail_color,
		QPaintDevice* pd, QRectF rect = QRectF());
	
	/**
	 * \brief Renders the next frame of the animation.
	 *
	 * \param head_color The color of the head of the string of bubbles.
	 * \param tail_color The color of the tail of the string of bubbles.
	 * \param painter The painter to use for drawing.
	 *        Saving and restoring its state is the responsibility
	 *        of the caller. 
	 * \param rect The rectangle in painter coordinates to render to.
	 * \return Whether more frames follow.  After returning false,
	 *         the next call will render the first frame again.
	 */
	bool nextFrame(
		QColor const& head_color, QColor const& tail_color,
		QPainter* painter, QRectF rect);
private:
	int m_numBubbles;
	int m_curFrame;
};

#endif
