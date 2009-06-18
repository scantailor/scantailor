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
#include "CrashReportDialog.h"
#include <QApplication>
#include <QTranslator>
#include <QStringList>
#include <QLocale>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	
	QString const translation("crashreporter_"+QLocale::system().name());
	QTranslator translator;
	
	// Try loading from the current directory.
	if (!translator.load(translation)) {
		// Now try loading from where it's supposed to be.
		QString path(QString::fromUtf8(TRANSLATIONS_DIR_ABS));
		path += QChar('/');
		path += translation;
		if (!translator.load(path)) {
			path = QString::fromUtf8(TRANSLATIONS_DIR_REL);
			path += QChar('/');
			path += translation;
			translator.load(path);
		}
	}
	
	app.installTranslator(&translator);
	
	// Note that we use app.arguments() rather than argv,
	// because the former is Unicode-safe under Windows.
	QStringList const args(app.arguments());
	if (args.size() < 3) {
		return 1;
	}
	
	CrashReportDialog* dialog = new CrashReportDialog(args[1], args[2]);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->show();

	return app.exec();
}
