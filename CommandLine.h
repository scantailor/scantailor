/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

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

#ifndef COMMANDLINE_H_
#define COMMANDLINE_H_

#include <QMap>
#include <QRectF>
#include <QStringList>

#include "Dpi.h"
#include "filters/page_split/LayoutType.h"
#include "filters/output/ColorParams.h"
#include "filters/output/DespeckleLevel.h"
#include "filters/output/DewarpingMode.h"
#include "filters/output/DepthPerception.h"
#include "filters/page_layout/Alignment.h"
#include "ImageFileInfo.h"
#include "Margins.h"
#include "Despeckle.h"

/**
 * CommandLine is a singleton simulation.
 * use CommandLine::get() to get access to global class
 * use CommandLine::set(CommandLine const&) to set the global class
 */
class CommandLine
{
	// Member-wise copying is OK.
public:
	enum Orientation {TOP, LEFT, RIGHT, UPSIDEDOWN};

	static CommandLine const& get() { return m_globalInstance; }
	static void set(CommandLine const& cl);

	CommandLine(QStringList const& argv, bool g=true) : m_gui(g), m_global(false) { CommandLine::parseCli(argv); }

	bool isGui() const { return m_gui; }
	bool isVerbose() const { return contains("verbose"); }

	std::vector<ImageFileInfo> const& images() const { return m_images; }
	QString const& outputDirectory() const { return m_outputDirectory; }
	QString const& projectFile() const { return m_projectFile; }
	QString const& outputProjectFile() const { return m_outputProjectFile; }

	bool hasMargins() const;
	bool hasAlignment() const;
	bool hasOutputDpi() const;

	bool hasHelp() const { return contains("help"); }
	bool hasOutputProject() const { return contains("output-project"); }
	bool hasLayout() const { return contains("layout"); }
	bool hasLayoutDirection() const { return contains("layout-direction"); }
	bool hasStartFilterIdx() const { return contains("start-filter"); }
	bool hasEndFilterIdx() const { return contains("end-filter"); }
	bool hasOrientation() const { return contains("orientation"); }
	bool hasDeskewAngle() const { return contains("rotate"); }
	bool hasDeskew() const { return contains("deskew"); }
	bool hasContentRect() const { return contains("content-box"); }
	bool hasColorMode() const { return contains("color-mode"); }
	bool hasWhiteMargins() const { return contains("white-margins"); }
	bool hasNormalizeIllumination() const { return contains("normalize-illumination"); }
	bool hasThreshold() const { return contains("threshold"); }
	bool hasDespeckle() const { return contains("despeckle"); }
	bool hasDewarping() const { return contains("dewarping"); }
	bool hasDepthPerception() const { return contains("dewarping"); }

	page_split::LayoutType getLayout() const { return m_layoutType; }
	Qt::LayoutDirection getLayoutDirection() const { return m_layoutDirection; }
	output::ColorParams::ColorMode getColorMode() const { return m_colorMode; }
	Dpi getInputDpi() const { return m_dpi; }
	Dpi getOutputDpi() const { return m_outputDpi; }
	Margins getMargins() const { return m_margins; }
	page_layout::Alignment getAlignment() const { return m_alignment; }
	Despeckle::Level getContentDetection() const { return m_contentDetection; }
	QRectF getContentRect() const { return m_contentRect; }
	Orientation getOrientation() const { return m_orientation; }
	int getThreshold() const { return m_threshold; }
	double getDeskewAngle() const { return m_deskewAngle; }
	int getStartFilterIdx() const { return m_startFilterIdx; }
	int getEndFilterIdx() const { return m_endFilterIdx; }
	output::DewarpingMode getDewarpingMode() const { return m_dewarpingMode; }
	output::DespeckleLevel getDespeckleLevel() const { return m_despeckleLevel; }
	output::DepthPerception getDepthPerception() const { return m_depthPerception; }

	bool help() { return m_options.contains("help"); }
	void printHelp();

private:
	CommandLine() : m_gui(true), m_global(false) {}

	static CommandLine m_globalInstance;

	bool m_gui;
	bool m_global;

	bool isGlobal() { return m_global; }
	void setGlobal() { m_global = true; }

	bool contains(QString const& key) const { return m_options.contains(key); }

	QMap<QString, QString> m_options;
	QString m_projectFile;
	QString m_outputProjectFile;
	std::vector<QFileInfo> m_files;
	std::vector<ImageFileInfo> m_images;
	QString m_outputDirectory;

	page_split::LayoutType m_layoutType;
	Qt::LayoutDirection m_layoutDirection;
	output::ColorParams::ColorMode m_colorMode;
	Dpi m_dpi;
	Dpi m_outputDpi;
	Margins m_margins;
	page_layout::Alignment m_alignment;
	Despeckle::Level m_contentDetection;
	QRectF m_contentRect;
	Orientation m_orientation;
	int m_threshold;
	double m_deskewAngle;
	int m_startFilterIdx;
	int m_endFilterIdx;
	output::DewarpingMode m_dewarpingMode;
	output::DespeckleLevel m_despeckleLevel;
	output::DepthPerception m_depthPerception;

	void parseCli(QStringList const& argv);
	void addImage(QString const& path);
	void setup();
	page_split::LayoutType fetchLayoutType();
	output::ColorParams::ColorMode fetchColorMode();
	Qt::LayoutDirection fetchLayoutDirection();
	Dpi fetchDpi(QString oname="dpi");
	Margins fetchMargins();
	page_layout::Alignment fetchAlignment();
	Despeckle::Level fetchContentDetection();
	QRectF fetchContentRect();
	Orientation fetchOrientation();
	QString fetchOutputProjectFile();
	int fetchThreshold();
	double fetchDeskewAngle();
	int fetchStartFilterIdx();
	int fetchEndFilterIdx();
	output::DewarpingMode fetchDewarpingMode();
	output::DespeckleLevel fetchDespeckleLevel();
	output::DepthPerception fetchDepthPerception();
};

#endif
