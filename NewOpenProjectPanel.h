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

#ifndef NEW_OPEN_PROJECT_PANEL_H_
#define NEW_OPEN_PROJECT_PANEL_H_

#include "ui_NewOpenProjectPanel.h"
#include <QWidget>
#include <QRegion>
#include <QSize>

class QString;

class NewOpenProjectPanel : public QWidget, private Ui::NewOpenProjectPanel
{
	Q_OBJECT
public:
	NewOpenProjectPanel(QWidget* parent = 0);
signals:
	void newProject();
	
	void openProject();
	
	void openRecentProject(QString const& project_file);
protected:
	virtual void paintEvent(QPaintEvent* event);
private:
	enum { MARGIN = 7 };
	
	void addRecentProject(QString const& file_path);
	
	QRegion m_mask;
	QSize m_lastPaintSize;
};

#endif
