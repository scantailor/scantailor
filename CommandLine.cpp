/*    Scan Tailor - Interactive post-processing tool for scanned pages.

    CommandLine - Interface for ScanTailor's parameters provided on CL.
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

#include <cstdlib>
#include <assert.h>
#include <iostream>
#include <tiff.h>

#include <QDir>
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
#include "filters/page_layout/Settings.h"
#include "Margins.h"
#include "Despeckle.h"


CommandLine CommandLine::m_globalInstance;


void
CommandLine::set(CommandLine const& cl)
{
	assert(!m_globalInstance.isGlobal());

	m_globalInstance = cl;
	m_globalInstance.setGlobal();
}


bool
CommandLine::parseCli(QStringList const& argv)
{
	QRegExp rx("^--([^=]+)=(.*)$");
	QRegExp rx_switch("^--([^=]+)$");
	QRegExp rx_short("^-([^=]+)=(.*)$");
	QRegExp rx_short_switch("^-([^=]+)$");
	QRegExp rx_project(".*\\.ScanTailor$", Qt::CaseInsensitive);

	QList<QString> opts;
	opts << "help";
	opts << "verbose";
	opts << "layout";
	opts << "layout-direction";
	opts << "orientation";
	opts << "rotate";
	opts << "deskew";
	opts << "skew-deviation";
	opts << "disable-content-detection";
	opts << "enable-page-detection";
	opts << "enable-fine-tuning";
	opts << "content-detection";
	opts << "content-box";
	opts << "content-deviation";
	opts << "enable-auto-margins";
	opts << "margins";
	opts << "margins-left";
	opts << "margins-right";
	opts << "margins-top";
	opts << "margins-bottom";
	opts << "match-layout";
	opts << "match-layout-tolerance";
	opts << "alignment";
	opts << "alignment-vertical";
	opts << "alignment-horizontal";
	opts << "alignment-tolerance";
	opts << "dpi";
	opts << "output-dpi";
	opts << "dpi-x";
	opts << "dpi-y";
	opts << "output-dpi-x";
	opts << "output-dpi-y";
	opts << "color-mode";
	opts << "white-margins";
	opts << "normalize-illumination";
	opts << "threshold";
	opts << "despeckle";
	opts << "dewarping";
	opts << "depth-perception";
	opts << "start-filter";
	opts << "end-filter";
	opts << "output-project";
	opts << "tiff-compression";
	opts << "picture-shape";
 	opts << "language";
	opts << "disable-content-text-mask";
    opts << "window-title";

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
	std::cout << "arg[" << i << "]=" << argv[i].toAscii().constData() << std::endl;
#endif
		if (rx.exactMatch(argv[i])) {
			// option with a value
			QString key = rx.cap(1);
			if (! opts.contains(key)) {
				m_error = true;
				std::cout << "Unknown option '" << key.toStdString() << "'" << std::endl;
				continue;
			}
			m_options[key] = rx.cap(2);
		} else if (rx_switch.exactMatch(argv[i])) {
			// option without value
			QString key = rx_switch.cap(1);
			if (! opts.contains(key)) {
				m_error = true;
				std::cout << "Unknown switch '" << key.toStdString() << "'" << std::endl;
				continue;
			}
			m_options[key] = "true";
		} else if (rx_short.exactMatch(argv[i])) {
			// option with a value
			QString key = shortMap[rx_short.cap(1)];
			if (key == "") {
				std::cout << "Unknown option: '" << rx_short.cap(1).toStdString() << "'" << std::endl;
				m_error = true;
				continue;
			}
			m_options[key] = rx_short.cap(2);
		} else if (rx_short_switch.exactMatch(argv[i])) {
			QString key = shortMap[rx_short_switch.cap(1)];
			if (key == "") {
				std::cout << "Unknown switch: '" << rx_short_switch.cap(1).toStdString() << "'" << std::endl;
				m_error = true;
				continue;
			}
			m_options[key] = "true";
		} else if (rx_project.exactMatch(argv[i])) {
			// project file
			CommandLine::m_projectFile = argv[i];
		} else {
			// handle input images and output directory
			QFileInfo file(argv[i]);
			if (i==(argv.size()-1)) {
				// output directory
				if (file.isDir()) {
					CommandLine::m_outputDirectory = file.filePath();
				} else {
					std::cout << "Error: Last argument must be an existing directory" << std::endl;
					exit(1);
				}
			} else if (file.filePath() == "-") {
				// file names from stdin
				std::string fname;
				while (! std::cin.eof()) {
					std::cin >> fname;
					addImage(fname.c_str());
				}
			} else if (file.isDir()) {
				// add all files from given directory as images
				QDir dir(argv[i]);
				QStringList files = dir.entryList(QDir::Files, QDir::Name);
				for (int f=0; f<files.count(); f++) {
					addImage(dir.filePath(files[f]));
				}
			} else {
				// argument is image
				addImage(file.filePath());
			}
		}
	}

	setup();

#ifdef DEBUG_CLI
	QStringList params = m_options.keys();
	for (int i=0; i<params.size(); i++) { std::cout << params[i].toAscii().constData() << "=" << m_options[params[i]].toAscii().constData() << std::endl; }
	std::cout << "Images: " << CommandLine::m_images.size() << std::endl;
#endif

	return m_error;
}

void
CommandLine::addImage(QString const& path)
{
	QFileInfo file(path);

	// create ImageFileInfo and push to images
	ImageId const image_id(file.filePath());
	ImageMetadata metadata;
	metadata.setDpi(fetchDpi());
	std::vector<ImageMetadata> vMetadata;
	vMetadata.push_back(metadata);
	ImageFileInfo image_info(file, vMetadata);
	m_images.push_back(image_info);
	m_files.push_back(file);
}

void
CommandLine::setup()
{
	m_outputProjectFile = fetchOutputProjectFile();
	m_layoutType = fetchLayoutType();
	m_layoutDirection = fetchLayoutDirection();
	m_colorMode = fetchColorMode();
	m_pictureShape = fetchPictureShape();
	m_dpi = fetchDpi();
	m_outputDpi = fetchDpi("output-dpi");
	m_margins = fetchMargins();
	m_alignment = fetchAlignment();
	m_contentDetection = fetchContentDetection();
	m_contentRect = fetchContentRect();
	m_contentDeviation = fetchContentDeviation();
	m_orientation = fetchOrientation();
	m_threshold = fetchThreshold();
	m_deskewAngle = fetchDeskewAngle();
	m_skewDeviation = fetchSkewDeviation();
	m_startFilterIdx = fetchStartFilterIdx();
	m_endFilterIdx = fetchEndFilterIdx();
	m_matchLayoutTolerance = fetchMatchLayoutTolerance();
	m_dewarpingMode = fetchDewarpingMode();
	m_compression = fetchCompression();
    m_language = fetchLanguage();
    m_windowTitle = fetchWindowTitle();
}


void
CommandLine::printHelp()
{
	std::cout << std::endl;
	std::cout << "Scan Tailor is a post-processing tool for scanned pages." << std::endl;
	std::cout << "Version: " << VERSION << std::endl;
	std::cout << std::endl;
	std::cout << "ScanTailor usage: " << std::endl;
	std::cout << "\t1) scantailor" << std::endl;
	std::cout << "\t2) scantailor <project_file>" << std::endl;
	std::cout << "\t3) scantailor-cli [options] <images|directory|-> <output_directory>" << std::endl;
	std::cout << "\t4) scantailor-cli [options] <project_file> [output_directory]" << std::endl;
	std::cout << std::endl;
	std::cout << "1)" << std::endl;
	std::cout << "\tstart ScanTailor's GUI interface" << std::endl;
	std::cout << "2)" << std::endl;
	std::cout << "\tstart ScanTailor's GUI interface and load project file" << std::endl;
	std::cout << "3)" << std::endl;
	std::cout << "\tbatch processing images from command line; no GUI" << std::endl;
	std::cout << "\tfile names are collected from arguments, input directory or stdin (-)" << std::endl;
	std::cout << "4)" << std::endl;
	std::cout << "\tbatch processing project from command line; no GUI" << std::endl;
	std::cout << "\tif output_directory is specified as last argument, it overwrites the one in project file" << std::endl;
	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "\t--help, -h" << std::endl;
	std::cout << "\t--verbose, -v" << std::endl;
	std::cout << "\t--languge=<cs|de|...>\t-- default: system language" << std::endl;
	std::cout << "\t--layout=, -l=<0|1|1.5|2>\t\t-- default: 0" << std::endl;
	std::cout << "\t\t\t  0: auto detect" << std::endl;
	std::cout << "\t\t\t  1: one page layout" << std::endl;
	std::cout << "\t\t\t1.5: one page layout but cutting is needed" << std::endl;
	std::cout << "\t\t\t  2: two page layout" << std::endl;
	std::cout << "\t--layout-direction=, -ld=<lr|rl>\t-- default: lr" << std::endl;
	std::cout << "\t--orientation=<left|right|upsidedown|none>\n\t\t\t\t\t\t-- default: none" << std::endl;
	std::cout << "\t--rotate=<0.0...360.0>\t\t\t-- it also sets deskew to manual mode" << std::endl;
	std::cout << "\t--deskew=<auto|manual>\t\t\t-- default: auto" << std::endl;
	std::cout << "\t--skew-deviation=<0.0...)\t\t\t-- default: 1.0; pages with bigger skew deviation will be painted in red" << std::endl;
	std::cout << "\t--disable-content-detection\t\t\t-- default: enabled" << std::endl;
	std::cout << "\t--enable-page-detection\t\t\t-- default: disabled" << std::endl;
	std::cout << "\t--enable-fine-tuning\t\t\t-- default: disabled; if page detection enabled it moves edges while corners are in black" << std::endl;
	std::cout << "\t--disable-content-text-mask\n\t\t\t\t\t\t-- disable using text mask to estimate a content box" << std::endl;
	std::cout << "\t--content-detection=<cautious|normal|aggressive>\n\t\t\t\t\t\t-- default: normal" << std::endl;
	std::cout << "\t--content-deviation=<0.0...)\t\t\t-- default: 2.0; pages with bigger content deviation will be painted in red" << std::endl;
	std::cout << "\t--content-box=<<left_offset>x<top_offset>:<width>x<height>>" << std::endl;
	std::cout << "\t\t\t\t\t\t-- if set the content detection is se to manual mode" << std::endl;
	std::cout << "\t\t\t\t\t\t   example: --content-box=100x100:1500x2500" << std::endl;
	std::cout << "\t--enable-auto-margins\t\t\t-- sets the margins to original ones (based on detected page or image size)" << std::endl;
	std::cout << "\t--margins=<number>\t\t\t-- sets left, top, right and bottom margins to same number." << std::endl;
	std::cout << "\t\t--margins-left=<number>" << std::endl;
	std::cout << "\t\t--margins-right=<number>" << std::endl;
	std::cout << "\t\t--margins-top=<number>" << std::endl;
	std::cout << "\t\t--margins-bottom=<number>" << std::endl;
	std::cout << "\t--match-layout=<true|false>\t\t-- default: true" << std::endl;
	std::cout << "\t--match-layout-tolerance=<0.0...)\t-- default: off" << std::endl;
	std::cout << "\t--alignment=<center|original|auto>\t-- sets vertical to original and horizontal to center" << std::endl;
	std::cout << "\t\t--alignment-vertical=<top|center|bottom|original>" << std::endl;
	std::cout << "\t\t--alignment-horizontal=<left|center|right|original>" << std::endl;
	std::cout << "\t--alignment-tolerance=<float>\t\t-- sets tolerance for auto alignment" << std::endl;
	std::cout << "\t--dpi=<number>\t\t\t\t-- sets x and y dpi. default: 600" << std::endl;
	std::cout << "\t\t--dpi-x=<number>" << std::endl;
	std::cout << "\t\t--dpi-y=<number>" << std::endl;
	std::cout << "\t--output-dpi=<number>\t\t\t-- sets x and y output dpi. default: 600" << std::endl;
	std::cout << "\t\t--output-dpi-x=<number>" << std::endl;
	std::cout << "\t\t--output-dpi-y=<number>" << std::endl;
	std::cout << "\t--color-mode=<black_and_white|color_grayscale|mixed>\n\t\t\t\t\t\t-- default: black_and_white" << std::endl;
	std::cout << "\t--picture-shape=<free|rectangular>\n\t\t\t\t\t\t-- default: free" << std::endl;
	std::cout << "\t--white-margins\t\t\t\t-- default: false" << std::endl;
	std::cout << "\t--normalize-illumination\t\t-- default: false" << std::endl;
	std::cout << "\t--threshold=<n>\t\t\t\t-- n<0 thinner, n>0 thicker; default: 0" << std::endl;
	std::cout << "\t--despeckle=<off|cautious|normal|aggressive>\n\t\t\t\t\t\t-- default: normal" << std::endl;
	std::cout << "\t--dewarping=<off|auto>\t\t\t-- default: off" << std::endl;
	std::cout << "\t--depth-perception=<1.0...3.0>\t\t-- default: 2.0" << std::endl;
	std::cout << "\t--start-filter=<1...6>\t\t\t-- default: 4" << std::endl;
	std::cout << "\t--end-filter=<1...6>\t\t\t-- default: 6" << std::endl;
	std::cout << "\t--output-project=, -o=<project_name>" << std::endl;
	std::cout << "\t--tiff-compression=<lzw|deflate|packbits|jpeg|none>\t-- default: lzw" << std::endl;
    std::cout << "\t--window-title=WindowTitle\t\t-- default: project name" << std::endl;
	std::cout << std::endl;
}


page_split::LayoutType
CommandLine::fetchLayoutType()
{
	page_split::LayoutType lt = page_split::AUTO_LAYOUT_TYPE;

	if (!hasLayout())
		return lt;

	if (m_options["layout"] == "1")
		lt = page_split::SINGLE_PAGE_UNCUT;
	else if (m_options["layout"] == "1.5")
		lt = page_split::PAGE_PLUS_OFFCUT;
	else if (m_options["layout"] == "2")
		lt = page_split::TWO_PAGES;

	return lt;
}

Qt::LayoutDirection
CommandLine::fetchLayoutDirection()
{
	Qt::LayoutDirection l = Qt::LeftToRight;
	if (!hasLayoutDirection())
		return l;

	QString ld = m_options["layout-direction"].toLower();
	if (ld == "rl")
		l = Qt::RightToLeft;

	return l;
}

Dpi
CommandLine::fetchDpi(QString oname)
{
	int xdpi=600;
	int ydpi=600;

	if (m_options.contains(oname+"-x")) {
		xdpi = m_options[oname+"-x"].toInt();
	}
	if (m_options.contains(oname+"-y")) {
		ydpi = m_options[oname+"-y"].toInt();
	}
	if (m_options.contains(oname)) {
		xdpi = m_options[oname].toInt();
		ydpi = m_options[oname].toInt();
	}

	return Dpi(xdpi, ydpi);
}

output::ColorParams::ColorMode
CommandLine::fetchColorMode()
{
	QString cm = m_options["color-mode"].toLower();
	
	if (cm == "color_grayscale")
		return output::ColorParams::COLOR_GRAYSCALE;
	else if (cm == "mixed")
		return output::ColorParams::MIXED;

	return output::ColorParams::BLACK_AND_WHITE;
}


output::PictureShape
CommandLine::fetchPictureShape()
{
	QString ps = m_options["picture-shape"].toLower();
	
	if (ps == "rectangular")
		return output::RECTANGULAR_SHAPE;

	return output::FREE_SHAPE;
}


Margins
CommandLine::fetchMargins()
{
	Margins margins(page_layout::Settings::defaultHardMarginsMM());

	if (m_options.contains("margins")) {
		double m = m_options["margins"].toDouble();
		margins.setTop(m);
		margins.setBottom(m);
		margins.setLeft(m);
		margins.setRight(m);
	}

	if (m_options.contains("margins-left"))
		margins.setLeft(m_options["margins-left"].toFloat());
	if (m_options.contains("margins-right"))
		margins.setRight(m_options["margins-right"].toFloat());
	if (m_options.contains("margins-top"))
		margins.setTop(m_options["margins-top"].toFloat());
	if (m_options.contains("margins-bottom"))
		margins.setBottom(m_options["margins-bottom"].toFloat());

	return margins;
}

page_layout::Alignment
CommandLine::fetchAlignment()
{
	page_layout::Alignment alignment(page_layout::Alignment::TOP, page_layout::Alignment::HCENTER);

	if (m_options.contains("match-layout")) {
		if (m_options["match-layout"] == "false") alignment.setNull(true);
		if (m_options["match-layout"] == "true") alignment.setNull(false);
	}

	if (m_options.contains("alignment-tolerance")) {
		alignment.setTolerance(m_options["alignment-tolerance"].toFloat());
	}

	if (m_options.contains("alignment")) {
		if (m_options["alignment"] == "original")
			alignment.setVertical(page_layout::Alignment::VORIGINAL);
		else if (m_options["alignment"] == "auto")
			alignment.setVertical(page_layout::Alignment::VAUTO);
		else
			alignment.setVertical(page_layout::Alignment::VCENTER);
		alignment.setHorizontal(page_layout::Alignment::HCENTER);
	}

	if (m_options.contains("alignment-vertical")) {
		QString a = m_options["alignment-vertical"].toLower();
		if (a == "top") alignment.setVertical(page_layout::Alignment::TOP);
		if (a == "center") alignment.setVertical(page_layout::Alignment::VCENTER);
		if (a == "bottom") alignment.setVertical(page_layout::Alignment::BOTTOM);
		if (a == "original") alignment.setVertical(page_layout::Alignment::VORIGINAL);
		if (a == "auto") alignment.setVertical(page_layout::Alignment::VAUTO);
	}

	if (m_options.contains("alignment-horizontal")) {
		QString a = m_options["alignment-horizontal"].toLower();
		if (a == "left") alignment.setHorizontal(page_layout::Alignment::LEFT);
		if (a == "center") alignment.setHorizontal(page_layout::Alignment::HCENTER);
		if (a == "right") alignment.setHorizontal(page_layout::Alignment::RIGHT);
		if (a == "original") alignment.setHorizontal(page_layout::Alignment::HORIGINAL);
		if (a == "auto") alignment.setHorizontal(page_layout::Alignment::HAUTO);
	}

	alignment.setAutoMargins(isAutoMarginsEnabled());

	return alignment;
}

Despeckle::Level
CommandLine::fetchContentDetection()
{
	Despeckle::Level level = Despeckle::NORMAL;

	if (m_options["content-detection"] != "") {
		QString cm = m_options["content-detection"].toLower();
		if (cm == "cautious")
			level = Despeckle::CAUTIOUS;
		else if (cm == "aggressive")
			level = Despeckle::AGGRESSIVE;
	}

	return level;
}

QRectF
CommandLine::fetchContentRect()
{
	if (!hasContentRect())
		return QRectF();

	QRegExp rx("([\\d\\.]+)x([\\d\\.]+):([\\d\\.]+)x([\\d\\.]+)");

	if (rx.exactMatch(m_options["content-box"])) {
		return QRectF(rx.cap(1).toFloat(), rx.cap(2).toFloat(), rx.cap(3).toFloat(), rx.cap(4).toFloat());
	}

	std::cout << "invalid --content-box=" << m_options["content-box"].toAscii().constData() << std::endl;
	exit(1);
}


double
CommandLine::fetchContentDeviation()
{
	if (!hasContentDeviation())
		return 1.0;

	return m_options["content-deviation"].toDouble();
}

CommandLine::Orientation
CommandLine::fetchOrientation()
{
	if (!hasOrientation())
		return TOP;

	Orientation orient;
	QString cli_orient = m_options["orientation"];

	if (cli_orient == "left") {
		orient = LEFT;
	} else if (cli_orient == "right") {
		orient = RIGHT;
	} else if (cli_orient == "upsidedown") {
		orient = UPSIDEDOWN;
	} else {
		std::cout << "Wrong orientation " << m_options["orientation"].toAscii().constData() << std::endl;
		exit(1);
	}

	return orient;
}


QString
CommandLine::fetchOutputProjectFile()
{
	if (!hasOutputProject())
		return QString();

	return m_options["output-project"];
}

int
CommandLine::fetchThreshold()
{
	if (!hasThreshold())
		return 0;

	return m_options["threshold"].toInt();
}

double
CommandLine::fetchDeskewAngle()
{
	if (!hasDeskewAngle())
		return 0.0;

	return m_options["rotate"].toDouble();
}

double
CommandLine::fetchSkewDeviation()
{
	if (!hasSkewDeviation())
		return 0.0;

	return m_options["skew-deviation"].toDouble();
}

int
CommandLine::fetchStartFilterIdx()
{
	if (!hasStartFilterIdx())
		return 0;

	return m_options["start-filter"].toInt() - 1;
}

int
CommandLine::fetchEndFilterIdx()
{
	if (!hasEndFilterIdx())
		return 5;

	return m_options["end-filter"].toInt() - 1;
}

output::DewarpingMode
CommandLine::fetchDewarpingMode()
{
	if (!hasDewarping())
		return output::DewarpingMode::OFF;

	return output::DewarpingMode(m_options["dewarping"].toLower());
}

output::DespeckleLevel
CommandLine::fetchDespeckleLevel()
{
	if (!hasDespeckle())
		return output::DESPECKLE_NORMAL;

	return output::despeckleLevelFromString(m_options["despeckle"]);
}

output::DepthPerception
CommandLine::fetchDepthPerception()
{
	if (!hasDepthPerception())
		return output::DepthPerception();

	return output::DepthPerception(m_options["depth-perception"]);
}

float
CommandLine::fetchMatchLayoutTolerance()
{
	if (!m_options.contains("match-layout-tolerance"))
		return 0.2;

	return m_options["match-layout-tolerance"].toFloat();
}


bool
CommandLine::hasMargins() const
{
	return(
		m_options.contains("margins") ||
		m_options.contains("margins-left") ||
		m_options.contains("margins-right") ||
		m_options.contains("margins-top") ||
		m_options.contains("margins-bottom")
	);
}

bool
CommandLine::hasAlignment() const
{
	return(
		hasMatchLayoutTolerance() ||
		m_options.contains("alignment") ||
		m_options.contains("alignment-vertical") ||
		m_options.contains("alignment-horizontal") ||
		isAutoMarginsEnabled()
	);
}

bool
CommandLine::hasOutputDpi() const
{
	return(
		m_options.contains("output-dpi") ||
		m_options.contains("output-dpi-x") ||
		m_options.contains("output-dpi-y")
                );
}

bool CommandLine::hasLanguage() const
{
	return m_options.contains("language");
}

int
CommandLine::fetchCompression() const
{
	if (!m_options.contains("tiff-compression"))
	    return COMPRESSION_LZW;
	
	QString c(m_options["tiff-compression"].toLower());
	if (c == "lzw")
	    return COMPRESSION_LZW;
	else if (c == "none")
	    return COMPRESSION_NONE;
	else if (c == "jpeg")
	    return COMPRESSION_JPEG;	
	else if (c == "deflate")
	    return COMPRESSION_DEFLATE;	
	else if (c == "packbits")
	    return COMPRESSION_PACKBITS;
	
	std::cout << "Unknown compression" << std::endl;
    throw("Unknown compression");
}

QString CommandLine::fetchLanguage() const
{
	if (hasLanguage()) {
		return m_options["language"];
	}
    
    return "untranslated";
}

QString CommandLine::fetchWindowTitle() const
{
    if (hasWindowTitle()) {
        return m_options["window-title"];
    }
    
    return "";
}
