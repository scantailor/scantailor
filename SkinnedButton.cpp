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

#include "SkinnedButton.h"
#include <QBitmap>

SkinnedButton::SkinnedButton(QString const& file, QWidget* parent)
:	QToolButton(parent),
	m_normalStatePixmap(file),
	m_normalStateFile(file)
{
	updateStyleSheet();
}

SkinnedButton::SkinnedButton(
	QString const& normal_state_file,
	QString const& hover_state_file,
	QString const& pressed_state_file,
	QWidget* parent)
:	QToolButton(parent),
	m_normalStatePixmap(normal_state_file),
	m_normalStateFile(normal_state_file),
	m_hoverStateFile(hover_state_file),
	m_pressedStateFile(pressed_state_file)
{
	updateStyleSheet();
}

void
SkinnedButton::setHoverImage(QString const& file)
{
	m_hoverStateFile = file;
	updateStyleSheet();
}

void
SkinnedButton::setPressedImage(QString const& file)
{
	m_pressedStateFile = file;
	updateStyleSheet();
}

void
SkinnedButton::setMask()
{
	setMask(m_normalStatePixmap.mask());
}

QSize
SkinnedButton::sizeHint() const
{
	if (m_normalStatePixmap.isNull()) {
		return QToolButton::sizeHint();
	} else {
		return m_normalStatePixmap.size();
	}
}

void
SkinnedButton::updateStyleSheet()
{
	QString style = QString(
		"QToolButton {"
			"border: none;"
			"background: transparent;"
			"image: url(%1);"
		"}"
	).arg(m_normalStateFile);
	
	if (!m_hoverStateFile.isEmpty()) {
		style += QString(
			"QToolButton:hover {"
				"image: url(%1);"
			"}"
		).arg(m_hoverStateFile);
	}
	
	if (!m_pressedStateFile.isEmpty()) {
		style += QString(
			"QToolButton:hover:pressed {"
				"image: url(%1);"
			"}"
		).arg(m_pressedStateFile);
	}
	
	setStyleSheet(style);
}
