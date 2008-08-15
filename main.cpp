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

#include "Application.h"
#include "NewOpenProjectDialog.h"
#include "PngMetadataLoader.h"
#include "TiffMetadataLoader.h"
#include "JpegMetadataLoader.h"
#include <QMetaType>
#include <QtPlugin>
#include <QString>

#ifdef Q_WS_WIN
// Import static plugins
Q_IMPORT_PLUGIN(qjpeg)
#endif

int main(int argc, char** argv)
{
	Application app(argc, argv);
	
	// This information is used by QSettings.
	app.setApplicationName("Scan Tailor");
	app.setOrganizationName("Scan Tailor");
	app.setOrganizationDomain("scantailor.sourceforge.net");
	
	PngMetadataLoader::registerMyself();
	TiffMetadataLoader::registerMyself();
	JpegMetadataLoader::registerMyself();
	
	app.showNewOpenProjectDialog();
	
	return app.exec();
}
