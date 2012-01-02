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
#include <QDir>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <Qt>
#include <string.h>

#include "CommandLine.h"


//#ifdef Q_WS_WIN
//// Import static plugins
//#Q_IMPORT_PLUGIN(qjpeg)
//#endif

#ifdef ENABLE_CRASH_REPORTER

#include "google-breakpad/client/windows/handler/exception_handler.h"
#include <QDebug>

static wchar_t crash_reporter_path[MAX_PATH];

static bool getCrashReporterPath(wchar_t* buf, DWORD buflen)
{
	DWORD i = GetModuleFileNameW(0, buf, buflen);
	if (i == buflen) {
		return false;
	}
	for (; buf[i] != L'\\'; --i) {
		if (i == 0) {
			return false;
		}
	}
	++i; // Move to the character after the backslash.

	static wchar_t const crash_reporter_exe[] = L"CrashReporter.exe";
	int const to_copy = sizeof(crash_reporter_exe)/sizeof(crash_reporter_exe[0]);
	for (int j = 0; j < to_copy; ++j, ++i) {
		if (i == buflen) {
			return false;
		}
		buf[i] = crash_reporter_exe[j];
	}

	return true;
}

static bool crashCallback(wchar_t const* dump_path,
						  wchar_t const* id,
						  void* context, EXCEPTION_POINTERS* exinfo,
						  MDRawAssertionInfo* assertion,
						  bool succeeded)
{
	if (!succeeded) {
		return false;
	}
	
	static wchar_t command_line[1024] = L"CrashReporter.exe ";
	wchar_t* p = command_line;
	p = lstrcatW(p, L"\"");
	p = lstrcatW(p, dump_path);
	p = lstrcatW(p, L"\" ");
	p = lstrcatW(p, L"\"");
	p = lstrcatW(p, id);
	p = lstrcatW(p, L"\"");
	
	static PROCESS_INFORMATION pinfo;
	static STARTUPINFOW startupinfo = {
		sizeof(STARTUPINFOW), 0, 0, 0,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	if (CreateProcessW(crash_reporter_path, command_line, 0, 0, FALSE,
		CREATE_UNICODE_ENVIRONMENT|CREATE_NEW_CONSOLE, 0, 0,
		&startupinfo, &pinfo)) {
		return true;
	}
	
	// CraeateProcessW() failed.  Maybe crash_reporter_path got corrupted?
	// Let's try to re-create it.
	getCrashReporterPath(
		crash_reporter_path,
		sizeof(crash_reporter_path)/sizeof(crash_reporter_path[0])
	);
	
	if (CreateProcessW(crash_reporter_path, command_line, 0, 0, FALSE,
		CREATE_UNICODE_ENVIRONMENT|CREATE_NEW_CONSOLE, 0, 0,
		&startupinfo, &pinfo)) {
		return true;
	}

	return false;
}

#endif // ENABLE_CRASH_REPORTER


int main(int argc, char** argv)
{
#ifdef ENABLE_CRASH_REPORTER
	getCrashReporterPath(
		crash_reporter_path,
		sizeof(crash_reporter_path)/sizeof(crash_reporter_path[0])
	);

	google_breakpad::ExceptionHandler eh(
		QDir::tempPath().toStdWString().c_str(), 0, &crashCallback,
		0, google_breakpad::ExceptionHandler::HANDLER_ALL
	);
#endif

	Application app(argc, argv);

#ifdef _WIN32
	// Get rid of all references to Qt's installation directory.
	app.setLibraryPaths(QStringList(app.applicationDirPath()));
#endif

	// parse command line arguments
	CommandLine cli(app.arguments());
	CommandLine::set(cli);

	if (cli.hasHelp()) {
		cli.printHelp();
		return 0;
	}
	
	QString const translation("scantailor_"+QLocale::system().name());
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

	if (!cli.projectFile().isEmpty()) {
		main_wnd->openProject(cli.projectFile());
	}

	return app.exec();
}
