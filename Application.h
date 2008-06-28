/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <QApplication>
#include <memory>

class NewOpenProjectDialog;

class Application : public QApplication
{
	Q_OBJECT
public:
	Application(int& argc, char** argv);
	
	void showNewOpenProjectDialog();
private slots:
	void newProject();
	
	void openProject();
	
	void projectContextDestroyed();
private:
	std::auto_ptr<NewOpenProjectDialog> m_ptrNewOpenProjectDialog;
};

#endif
