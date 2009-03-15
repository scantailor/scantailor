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

#ifndef SKINNEDBUTTON_H_
#define SKINNEDBUTTON_H_

#include <QToolButton>
#include <QPixmap>
#include <QString>

/**
 * \brief A button represented by a set of images.
 *
 * The button won't have any borders or effects other than
 * those represented by set of 3 images:
 * \li The normal state image.
 * \li The hover state image.
 * \li The pressed state image.
 */
class SkinnedButton : public QToolButton
{
public:
	/**
	 * \brief Construct a skinned button from a single image.
	 *
	 * \param file The path to a file or a Qt resource to the
	 *        image representing the normal state of the button.
	 * \param parent An optional parent widget.
	 */
	SkinnedButton(QString const& file, QWidget* parent = 0);
	
	/**
	 * \brief Construct a skinned button from a set of 3 images.
	 *
	 * \param normal_state_file The path to a file or a Qt resource
	 *        to the image representing the normal state of the button.
	 * \param hover_state_file The path to a file or a Qt resource
	 *        to the image representing the hover state of the button.
	 * \param pressed_state_file The path to a file or a Qt resource
	 *        to the image representing the pressed state of the button.
	 * \param parent An optional parent widget.
	 *
	 * Note that the sizes of all 3 images should be the same.
	 */
	SkinnedButton(
		QString const& normal_state_file,
		QString const& hover_state_file,
		QString const& pressed_state_file,
		QWidget* parent = 0);
	
	/**
	 * \brief Set the hover state image.
	 *
	 * \param file The path to a file or a Qt resource to the
	 *        image representing the hover state of the button.
	 *        This image should have the same size as the normal
	 *        state image.
	 */
	void setHoverImage(QString const& file);
	
	/**
	 * \brief Set the pressed state image.
	 *
	 * \param file The path to a file or a Qt resource to the
	 *        image representing the pressed state of the button.
	 *        This image should have the same size as the normal
	 *        state image.
	 */
	void setPressedImage(QString const& file);
	
	/**
	 * \brief Set the mask of the widget based on the alpha channel
	 *        of the normal state image.
	 *
	 * The mask affects things like the mouse-over handling.
	 */
	void setMask();
	
	/**
	 * Bring in the other signatures of setMask().
	 */
	using QToolButton::setMask;
	
	/**
	 * \brief Reimplemented sizeHint().
	 *
	 * \return The size of the normal state image.
	 */
	virtual QSize sizeHint() const;
private:
	void updateStyleSheet();
	
	QPixmap m_normalStatePixmap;
	QString m_normalStateFile;
	QString m_hoverStateFile;
	QString m_pressedStateFile;
};

#endif
