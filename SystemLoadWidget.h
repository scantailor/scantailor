/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef SYSTEM_LOAD_WIDGET_H_
#define SYSTEM_LOAD_WIDGET_H_

#include "ui_SystemLoadWidget.h"
#include <QWidget>

class SystemLoadWidget : public QWidget
{
	Q_OBJECT
public:
	SystemLoadWidget(QWidget* parent = 0);
private slots:
	void sliderPressed();

	void sliderMoved(int prio);

	void valueChanged(int prio);

	void decreasePriority();

	void increasePriority();
private:
	void showHideToolTip(int prio);

	static QString tooltipText(int prio);

	Ui::SystemLoadWidget ui;
};

#endif
