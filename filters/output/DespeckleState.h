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


#ifndef OUTPUT_DESPECKLE_STATE_H_
#define OUTPUT_DESPECKLE_STATE_H_

#include "DespeckleLevel.h"
#include "Dpi.h"
#include "imageproc/BinaryImage.h"
#include <QImage>

class TaskStatus;
class DebugImages;

namespace output
{

class DespeckleVisualization;

/**
 * Holds enough information to build a DespeckleVisualization
 * or to re-despeckle with different DespeckleLevel.
 */
class DespeckleState
{
	// Member-wise copying is OK.
public:
	DespeckleState(QImage const& output,
		imageproc::BinaryImage const& speckles,
		DespeckleLevel level, Dpi const& dpi);

	DespeckleLevel level() const { return m_despeckleLevel; }

	DespeckleVisualization visualize() const;

	DespeckleState redespeckle(DespeckleLevel level,
		TaskStatus const& status, DebugImages* dbg = 0) const;
private:
	static QImage overlaySpeckles(
		QImage const& mixed, imageproc::BinaryImage const& speckles);
	
	static imageproc::BinaryImage extractBW(QImage const& mixed);

	/**
	 * This image is the output image produced by OutputGenerator
	 * with speckles added as black regions.  This image is always in RGB32,
	 * because it only exists for display purposes, namely for being fed to
	 * DespeckleVisualization.
	 */
	QImage m_everythingMixed;

	/**
	 * The B/W part of m_everythingMixed.
	 */
	imageproc::BinaryImage m_everythingBW;
	
	/**
	 * The speckles detected in m_everythingBW.
	 * This image may be null, which is equivalent to having it all white.
	 */
	imageproc::BinaryImage m_speckles;

	/**
	 * The DPI of all 3 above images.
	 */
	Dpi m_dpi;

	/**
	 * Despeckling level at which m_speckles was produced from
	 * m_everythingBW.
	 */
	DespeckleLevel m_despeckleLevel;
};

} // namespace output

#endif
