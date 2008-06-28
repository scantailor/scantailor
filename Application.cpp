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

#include "Application.h.moc"
#include "NewOpenProjectDialog.h"
#include "ProjectCreationContext.h"
#include "ProjectOpeningContext.h"

Application::Application(int& argc, char** argv)
:	QApplication(argc, argv)
{
}

void
Application::showNewOpenProjectDialog()
{
	if (!m_ptrNewOpenProjectDialog.get()) {
		m_ptrNewOpenProjectDialog.reset(new NewOpenProjectDialog);
		connect(
			m_ptrNewOpenProjectDialog.get(), SIGNAL(newProject()),
			this, SLOT(newProject())
		);
		connect(
			m_ptrNewOpenProjectDialog.get(), SIGNAL(openProject()),
			this, SLOT(openProject())
		);
	}
	
	m_ptrNewOpenProjectDialog->show();
}

void
Application::newProject()
{
	m_ptrNewOpenProjectDialog->hide();
	ProjectCreationContext* context = new ProjectCreationContext();
	connect(
		context, SIGNAL(destroyed(QObject*)),
		this, SLOT(projectContextDestroyed())
	);
}

void
Application::openProject()
{
	m_ptrNewOpenProjectDialog->hide();
	ProjectOpeningContext* context = new ProjectOpeningContext();
	connect(
		context, SIGNAL(destroyed(QObject*)),
		this, SLOT(projectContextDestroyed())
	);
	context->openProject();
}

void
Application::projectContextDestroyed()
{
	int num_visible_widgets = 0;
	foreach (QWidget *widget, topLevelWidgets()) {
		// Note that topLevelWidgets() returns all kinds
		// of internal widgets, but they are hidden.
		if (!widget->isHidden()) {
			++num_visible_widgets;
		}
	}
	
	if (num_visible_widgets == 0) {
		showNewOpenProjectDialog();
	}
}
