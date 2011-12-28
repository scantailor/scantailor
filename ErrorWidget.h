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

#ifndef ERRORWIDGET_H_
#define ERRORWIDGET_H_

#include "ui_ErrorWidget.h"
#include <QWidget>
#include <Qt>

class QString;

class ErrorWidget : public QWidget, private Ui::ErrorWidget
{
	Q_OBJECT
public:
	ErrorWidget(QString const& text, Qt::TextFormat fmt = Qt::AutoText);
private slots:
	/**
	 * \see QLabel::linkActivated()
	 */
	virtual void linkActivated(QString const& link) {}
};

#endif
