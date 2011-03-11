#include <iostream>

#include <QMap>
#include <QRegExp>
#include <QStringList>

#include "Dpi.h"
#include "ImageId.h"
#include "version.h"
#include "CommandLine.h"
#include "ImageFileInfo.h"
#include "ImageMetadata.h"
#include "filters/page_split/LayoutType.h"
#include "Margins.h"
#include "Despeckle.h"


bool CommandLine::s_gui=true;
QMap<QString, QString> CommandLine::s_options;
QString CommandLine::s_project_file = "";
std::vector<QFileInfo> CommandLine::s_files;
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
	QRegExp rx_short("^-([^=]+)=(.*)$");
	QRegExp rx_short_switch("^-([^=]+)$");
	QRegExp rx_project(".*\\.ScanTailor$", Qt::CaseInsensitive);

	QMap<QString, QString> shortMap;
	shortMap["h"] = "help";
	shortMap["help"] = "help";
	shortMap["v"] = "verbose";
	shortMap["l"] = "layout";
	shortMap["ld"] = "layout-direction";
	shortMap["o"] = "output-project";

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
			// option with a value
			QString key = shortMap[rx_short.cap(1)];
			CommandLine::s_options[key] = rx_short.cap(2);
		} else if (rx_short_switch.exactMatch(argv[i])) {
			QString key = shortMap[rx_short_switch.cap(1)];
			if (key == "") continue;
			CommandLine::s_options[key] = "true";
		} else if (rx_project.exactMatch(argv[i])) {
			// project file
			CommandLine::s_project_file = argv[i];
		} else {
			// image and output directory
			QFileInfo file(argv[i]);
			CommandLine::s_files.push_back(file);
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
				metadata.setDpi(CommandLine::getDpi());
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
	std::cout << "Version: " << VERSION << "\n";
	std::cout << "\n";
	std::cout << "ScanTailor usage: " << "\n";
	std::cout << "\t1) scantailor" << "\n";
	std::cout << "\t2) scantailor <project_file>" << "\n";
	std::cout << "\t3) scantailor-batch [options] <image, image, ...> <output_directory>" << "\n";
	std::cout << "\t4) scantailor-batch [options] <project_file> [output_directory]" << "\n";
	std::cout << "\n";
	std::cout << "1)" << "\n";
	std::cout << "\tstart ScanTailor's GUI interface" << "\n";
	std::cout << "2)" << "\n";
	std::cout << "\tstart ScanTailor's GUI interface and load project file" << "\n";
	std::cout << "3)" << "\n";
	std::cout << "\tbatch processing images from command line; no GUI" << "\n";
	std::cout << "4)" << "\n";
	std::cout << "\tbatch processing project from command line; no GUI" << "\n";
	std::cout << "\tif output_directory is specified as last argument, it overwrites the one in project file" << "\n";
	std::cout << "\n";
	std::cout << "Options:" << "\n";
	std::cout << "\t--help, -h" << "\n";
	std::cout << "\t--verbose, -v" << "\n";
	std::cout << "\t--layout=, -l=<0|1|1.5|2>\t\t-- default: 0" << "\n";
	std::cout << "\t--layout-direction=, -ld=<lr|rl>\t-- default: lr" << "\n";
	std::cout << "\t--orientation=<left|right|upsidedown|none>\n\t\t\t\t\t\t-- default: none" << "\n";
	std::cout << "\t--rotate=<0.0...360.0>\t\t\t-- it also sets deskew to manual mode" << "\n";
	std::cout << "\t--deskew=<auto|manual>\t\t\t-- default: auto" << "\n";
	std::cout << "\t--content-detection=<cautious|normal|aggressive>\n\t\t\t\t\t\t-- default: normal" << "\n";
	std::cout << "\t--content-box=<<left_offset>x<top_offset>:<width>x<height>>" << "\n";
	std::cout << "\t\t\t\t\t\t-- if set the content detection is se to manual mode" << "\n";
	std::cout << "\t\t\t\t\t\t   example: --content-box=100x100:1500x2500" << "\n";
	std::cout << "\t--margins=<number>\t\t\t-- sets left, top, right and bottom margins to same number." << "\n";
	std::cout << "\t\t--margins-left=<number>" << "\n";
	std::cout << "\t\t--margins-right=<number>" << "\n";
	std::cout << "\t\t--margins-top=<number>" << "\n";
	std::cout << "\t\t--margins-bottom=<number>" << "\n";
	std::cout << "\t--match-layout=<true|false>\t\t-- default: true" << "\n";
	std::cout << "\t--match-layout-tolerance=<0.0...)\t-- default: off" << "\n";
	std::cout << "\t--alignment=center\t\t-- sets vertical and horizontal alignment to center" << "\n";
	std::cout << "\t\t--alignment-vertical=<top|center|bottom>" << "\n";
	std::cout << "\t\t--alignment-horizontal=<left|center|right>" << "\n";
	std::cout << "\t--dpi=<number>\t\t\t\t-- sets x and y dpi. default: 600" << "\n";
	std::cout << "\t\t--dpi-x=<number>" << "\n";
	std::cout << "\t\t--dpi-y=<number>" << "\n";
	std::cout << "\t--output-dpi=<number>\t\t\t-- sets x and y output dpi. default: 600" << "\n";
	std::cout << "\t\t--output-dpi-x=<number>" << "\n";
	std::cout << "\t\t--output-dpi-y=<number>" << "\n";
	std::cout << "\t--color-mode=<black_and_white|color_grayscale|mixed>\n\t\t\t\t\t\t-- default: black_and_white" << "\n";
	std::cout << "\t--white-margins\t\t\t\t-- default: false" << "\n";
	std::cout << "\t--normalize-illumination\t\t-- default: false" << "\n";
	std::cout << "\t--threshold=<n>\t\t\t\t-- n<0 thinner, n>0 thicker; default: 0" << "\n";
	std::cout << "\t--despeckle=<off|cautious|normal|aggressive>\n\t\t\t\t\t\t-- default: normal" << "\n";
	std::cout << "\t--dewarping=<off|auto>\t\t\t-- default: off" << "\n";
	std::cout << "\t--depth-perception=<1.0...3.0>\t\t-- default: 2.0" << "\n";
	std::cout << "\t--start-filter=<1...6>\t\t\t-- default: 4" << "\n";
	std::cout << "\t--end-filter=<1...6>\t\t\t-- default: 6" << "\n";
	std::cout << "\t--output-project=, -o=<project_name>" << "\n";
	std::cout << "\n";
}


page_split::LayoutType
CommandLine::layout()
{
	page_split::LayoutType lt = page_split::AUTO_LAYOUT_TYPE;

	if (CommandLine::s_options["layout"] == "1")
		lt = page_split::SINGLE_PAGE_UNCUT;
	else if (CommandLine::s_options["layout"] == "1.5")
		lt = page_split::PAGE_PLUS_OFFCUT;
	else if (CommandLine::s_options["layout"] == "2")
		lt = page_split::TWO_PAGES;

	return lt;
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

Dpi
CommandLine::getDpi(QString oname)
{
	int xdpi=300;
	int ydpi=300;

	if (CommandLine::s_options[oname+"-x"] != "") {
		xdpi = CommandLine::s_options[oname+"-x"].toInt();
	}
	if (CommandLine::s_options[oname+"-y"] != "") {
		ydpi = CommandLine::s_options[oname+"-y"].toInt();
	}
	if (CommandLine::s_options[oname] != "") {
		xdpi = CommandLine::s_options[oname].toInt();
		ydpi = CommandLine::s_options[oname].toInt();
	}

	return Dpi(xdpi, ydpi);
}

output::ColorParams::ColorMode
CommandLine::colorMode()
{
	QString cm = CommandLine::s_options["color-mode"].toLower();
	
	if (cm == "color_grayscale")
		return output::ColorParams::COLOR_GRAYSCALE;
	else if (cm == "mixed")
		return output::ColorParams::MIXED;

	return output::ColorParams::BLACK_AND_WHITE;
}


Margins
CommandLine::getMargins()
{
	Margins margins(10.0, 5.0, 10.0, 5.0);

	if (CommandLine::s_options["margins"] != "") {
		double m = CommandLine::s_options["margins"].toDouble();
		margins.setTop(m);
		margins.setBottom(m);
		margins.setLeft(m);
		margins.setRight(m);
	}

	if (CommandLine::s_options["margins-left"] != "")
		margins.setLeft(CommandLine::s_options["margins-left"].toFloat());
	if (CommandLine::s_options["margins-right"] != "")
		margins.setRight(CommandLine::s_options["margins-right"].toFloat());
	if (CommandLine::s_options["margins-top"] != "")
		margins.setTop(CommandLine::s_options["margins-top"].toFloat());
	if (CommandLine::s_options["margins-bottom"] != "")
		margins.setBottom(CommandLine::s_options["margins-bottom"].toFloat());

	return margins;
}

page_layout::Alignment
CommandLine::getAlignment()
{
	page_layout::Alignment alignment(page_layout::Alignment::TOP, page_layout::Alignment::HCENTER);

	if (CommandLine::s_options["match-layout"] != "") {
		if (CommandLine::s_options["match-layout"] == "false") alignment.setNull(true);
		if (CommandLine::s_options["match-layout"] == "true") alignment.setNull(false);
	}

	if (CommandLine::s_options["alignment"] == "center") {
		alignment.setVertical(page_layout::Alignment::VCENTER);
		alignment.setHorizontal(page_layout::Alignment::HCENTER);
	}

	if (CommandLine::s_options["alignment-vertical"] != "") {
		QString a = CommandLine::s_options["alignment-vertical"].toLower();
		if (a == "top") alignment.setVertical(page_layout::Alignment::TOP);
		if (a == "center") alignment.setVertical(page_layout::Alignment::VCENTER);
		if (a == "bottom") alignment.setVertical(page_layout::Alignment::BOTTOM);
	}

	if (CommandLine::s_options["alignment-horizontal"] != "") {
		QString a = CommandLine::s_options["alignment-horizontal"].toLower();
		if (a == "left") alignment.setHorizontal(page_layout::Alignment::LEFT);
		if (a == "center") alignment.setHorizontal(page_layout::Alignment::HCENTER);
		if (a == "right") alignment.setHorizontal(page_layout::Alignment::RIGHT);
	}

	return alignment;
}

Despeckle::Level
CommandLine::getContentDetection()
{
	Despeckle::Level level = Despeckle::NORMAL;

	if (CommandLine::s_options["content-detection"] != "") {
		QString cm = CommandLine::s_options["content-detection"].toLower();
		if (cm == "cautious")
			level = Despeckle::CAUTIOUS;
		else if (cm == "aggressive")
			level = Despeckle::AGGRESSIVE;
	}

	return level;
}
