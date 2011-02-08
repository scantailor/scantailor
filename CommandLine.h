/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    ConsoleBatch - Batch processing scanned pages from command line.
    Copyright (C) 2011 Petr Kovar <pejuko@gmail.com>

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

#ifndef COMMANDLINE_H_
#define COMMANDLINE_H_

#include <QMap>
#include <QStringList>

#include "Dpi.h"
#include "ProjectPages.h"
#include "ImageFileInfo.h"

// CommandLine is a singleton simulation; if you create anywhere object CommandLine
// you get always access to the same variables
class CommandLine
{
	static QMap<QString, QString> s_options;
	static QString s_project_file;
	static std::vector<QFileInfo> s_files;
	static std::vector<ImageFileInfo> s_images;
	static QString s_output_directory;

	static void parse_cli(QStringList const& argv);
	static void parse_cli(int argc, char **argv);
	static void getDpi(int &xdpi, int &ydpi);

	public:
		CommandLine() {};
		CommandLine(QStringList const& argv) { CommandLine::parse_cli(argv); };
		CommandLine(int argc, char **argv) { CommandLine::parse_cli(argc, argv); };

		QMap<QString, QString> options() { return CommandLine::s_options; };
		QString projectFile() { return CommandLine::s_project_file; };
		std::vector<ImageFileInfo> images() { return CommandLine::s_images; };
		QString outputDirectory() { return CommandLine::s_output_directory; };

		QString & operator[](QString const& key) { return (QString &)CommandLine::s_options[key]; };
		QString const operator[](QString const& key) const { return CommandLine::s_options.value(key); };

		bool help() { return (CommandLine::s_options["help"] == "true"); };
		ProjectPages::LayoutType layout();
		Qt::LayoutDirection layoutDirection();
		void dpi(int &xdpi, int &ydpi) { CommandLine::getDpi(xdpi, ydpi); };

		void printHelp();
};

#endif
