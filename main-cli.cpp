/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>
    Copyright (C) 2011  Petr Kovar <pejuko@gmail.com>

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

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <iostream>

#include "CommandLine.h"
#include "ConsoleBatch.h"


int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);

#ifdef _WIN32
	// Get rid of all references to Qt's installation directory.
	app.setLibraryPaths(QStringList(app.applicationDirPath()));
#endif

	// parse command line arguments
	CommandLine cli(app.arguments(), false);
	CommandLine::set(cli);

	if (cli.isError()) {
		cli.printHelp();
		return 1;
	}

	if (cli.hasHelp() || cli.outputDirectory().isEmpty() || (cli.images().size()==0 && cli.projectFile().isEmpty())) {
		cli.printHelp();
		return 0;
	}

	std::auto_ptr<ConsoleBatch> cbatch;

	try {
		if (!cli.projectFile().isEmpty()) {
			cbatch.reset(new ConsoleBatch(cli.projectFile()));
		} else {
			cbatch.reset(new ConsoleBatch(cli.images(), cli.outputDirectory(), cli.getLayoutDirection()));
		}
		cbatch->process();
	} catch(std::exception const& e) {
		std::cerr << e.what() << std::endl;
		exit(1);
	}

	if (cli.hasOutputProject())
		cbatch->saveProject(cli.outputProjectFile());
}
