#include <iostream>

#include <QMap>
#include <QRegExp>
#include <QStringList>

#include "Dpi.h"
#include "ImageId.h"
#include "CommandLine.h"
#include "ImageFileInfo.h"
#include "ImageMetadata.h"


QMap<QString, QString> CommandLine::s_options;
QString CommandLine::s_project_file = "";
std::vector<ImageFileInfo> CommandLine::s_images;
QString CommandLine::s_output_directory = ".";


void
CommandLine::parse_cli(int argc, char **argv)
{
	QStringList args;

	for (int i=0; i<argc; i++) {
		args << argv[i];
	}

	CommandLine::parse_cli(args);
}


void
CommandLine::parse_cli(QStringList const& argv)
{
	QRegExp rx("^--([^=]+)=(.*)$");
	QRegExp rx_switch("^--([^=]+)$");
	QRegExp rx_short("^-(.)$");
	QRegExp rx_project(".*\\.ScanTailor$", Qt::CaseInsensitive);

	QMap<QString, QString> shortMap;
	shortMap["h"] = "help";
	shortMap["help"] = "help";
	shortMap["ld"] = "layout-direction";

	// skip first argument (scantailor)
	for (int i=1; i<argv.size(); i++) {
#ifdef DEBUG_CLI
	std::cout << "arg[" << i << "]=" << argv[i].toAscii().constData() << "\n";
#endif
		if (rx.exactMatch(argv[i])) {
			// option with a value
			CommandLine::s_options[rx.cap(1)] = rx.cap(2);
		} else if (rx_switch.exactMatch(argv[i])) {
			// option without value
			CommandLine::s_options[rx_switch.cap(1)] = "true";
		} else if (rx_short.exactMatch(argv[i])) {
			QString key = shortMap[rx_short.cap(1)];
			if (key == "") continue;
			CommandLine::s_options[key] = "true";
		} else if (rx_project.exactMatch(argv[i])) {
			// project file
			CommandLine::s_project_file = argv[i];
		} else {
			// image and output directory
			QFileInfo file(argv[i]);
			if (i==(argv.size()-1)) {
				// output directory
				if (file.isDir()) {
					CommandLine::s_output_directory = file.filePath();
				} else {
					std::cout << "Error: Last argument must be an existing directory" << "\n";
					CommandLine::s_options["help"] = "true";
					break;
				}
			} else {
				// create ImageFileInfo and push to images
				ImageId const image_id(file.filePath());
				ImageMetadata metadata;
				int xdpi, ydpi;
				CommandLine::getDpi(xdpi, ydpi);
				metadata.setDpi(Dpi(xdpi, ydpi));
				std::vector<ImageMetadata> vMetadata;
				vMetadata.push_back(metadata);
				ImageFileInfo image_info(file, vMetadata);
				CommandLine::s_images.push_back(image_info);
			}
		}
	}

#ifdef DEBUG_CLI
	QStringList params = CommandLine::s_options.keys();
	for (int i=0; i<params.size(); i++) { std::cout << params[i].toAscii().constData() << "=" << CommandLine::s_options[params[i]].toAscii().constData() << "\n"; }
	std::cout << "Images: " << CommandLine::s_images.size() << "\n";
#endif
}


void
CommandLine::printHelp()
{
	std::cout << "\n";
	std::cout << "Scan Tailor is a post-processing tool for scanned pages." << "\n";
	std::cout << "\n";
	std::cout << "ScanTailor usage: " << "\n";
	std::cout << "\t1) scantailor [options]" << "\n";
	std::cout << "\t2) scantailor [options] <project_file>" << "\n";
	std::cout << "\t3) scantailor [options] <image, image, ...> <output_directory>" << "\n";
	std::cout << "\n";
	std::cout << "1)" << "\n";
	std::cout << "\tstart ScanTailor's GUI interface" << "\n";
	std::cout << "2)" << "\n";
	std::cout << "\tstart ScanTailor's GUI interface and load project file" << "\n";
	std::cout << "3)" << "\n";
	std::cout << "\tbatch processing images from command line; no GUI" << "\n";
	std::cout << "\n";
	std::cout << "Options:" << "\n";
	std::cout << "\t--help, -h" << "\n";
	std::cout << "\t--layout-direction=, -ld=<lr|rl>\t-- default: lr" << "\n";
	std::cout << "\t--dpi=<number>\t\t\t\t-- sets x and y dpi. default: 300" << "\n";
	std::cout << "\t\t--dpi-x=<number>" << "\n";
	std::cout << "\t\t--dpi-y=<number>" << "\n";
	std::cout << "\n";
}


Qt::LayoutDirection
CommandLine::layoutDirection()
{
	Qt::LayoutDirection l = Qt::LeftToRight;
	QString ld = CommandLine::s_options["layout-direction"].toLower();

	if (ld == "rl")
		l = Qt::RightToLeft;

	return l;
}

void
CommandLine::getDpi(int &xdpi, int &ydpi)
{
	xdpi=300;
	ydpi=300;

	if (CommandLine::s_options["dpi-x"] != "") {
		xdpi = CommandLine::s_options["dpi-x"].toInt();
	}
	if (CommandLine::s_options["dpi-y"] != "") {
		ydpi = CommandLine::s_options["dpi-y"].toInt();
	}
	if (CommandLine::s_options["dpi"] != "") {
		xdpi = CommandLine::s_options["dpi"].toInt();
		ydpi = CommandLine::s_options["dpi"].toInt();
	}
}
