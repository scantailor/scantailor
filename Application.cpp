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

#include "Application.h"
#include "Application.h.moc"
#include "OutOfMemoryHandler.h"
#include <new>

Application::Application(int& argc, char** argv)
:	QApplication(argc, argv)
{
}

bool
Application::notify(QObject* receiver, QEvent* e)
{
	try {
		return QApplication::notify(receiver, e);
	} catch (std::bad_alloc const&) {
		OutOfMemoryHandler::instance().handleOutOfMemorySituation();
        return false;
    }
}
