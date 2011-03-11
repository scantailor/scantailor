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
#include <iostream>

#include "CommandLine.h"
#include "ConsoleBatch.h"

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	//QApplication app(argc, argv);

	// parse command line arguments
	CommandLine cli(app.arguments());
	if (cli["help"] == "true") {
		cli.printHelp();
		return 0;
	}
	cli.setGui(false);

	ConsoleBatch* cbatch;

	try {
		if (cli.projectFile() != "") {
			cbatch = new ConsoleBatch(cli.projectFile());
		} else {
			cbatch = new ConsoleBatch(cli.images(), cli.outputDirectory(), cli.layoutDirection());
		}
		cbatch->process();
	} catch(char const *msg) {
		std::cout << msg << "\n";
		exit(1);
	}

	if (cli["output-project"] != "")
		cbatch->saveProject(cli["output-project"]);
}
