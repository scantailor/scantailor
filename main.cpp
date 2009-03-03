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

#include "config.h"
#include "Application.h"
#include "MainWindow.h"
#include "PngMetadataLoader.h"
#include "TiffMetadataLoader.h"
#include "JpegMetadataLoader.h"
#include <QMetaType>
#include <QtPlugin>
#include <QLocale>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <Qt>

#ifdef Q_WS_WIN
// Import static plugins
Q_IMPORT_PLUGIN(qjpeg)
#endif

int main(int argc, char** argv)
{
	Application app(argc, argv);
	
	QString const locale(QLocale::system().name());
	QTranslator translator;
	
	// Try loading from the current directory.
	if (!translator.load(locale)) {
		// Now try loading from where it's supposed to be.
		QString path(QString::fromUtf8(TRANSLATIONS_DIR_ABS));
		path += QChar('/');
		path += locale;
		if (!translator.load(path)) {
			path = QString::fromUtf8(TRANSLATIONS_DIR_REL);
			path += QChar('/');
			path += locale;
			translator.load(path);
		}
	}
	
	app.installTranslator(&translator);
	
	// This information is used by QSettings.
	app.setApplicationName("Scan Tailor");
	app.setOrganizationName("Scan Tailor");
	app.setOrganizationDomain("scantailor.sourceforge.net");
	QSettings settings;
	
	PngMetadataLoader::registerMyself();
	TiffMetadataLoader::registerMyself();
	JpegMetadataLoader::registerMyself();
	
	MainWindow* main_wnd = new MainWindow();
	main_wnd->setAttribute(Qt::WA_DeleteOnClose);
	if (settings.value("mainWindow/maximized") == false) {
		main_wnd->show();
	} else {
		main_wnd->showMaximized();
	}
	
	// Note that we use app.arguments() rather than argv,
	// because the former is Unicode-safe under Windows.
	QStringList const args(app.arguments());
	if (args.size() > 1) {
		main_wnd->openProject(args[1]);
	}
	
	return app.exec();
}
