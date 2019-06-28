/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef IMAGEPROC_SKEWFINDER_H_
#define IMAGEPROC_SKEWFINDER_H_

#include "NonCopyable.h"

namespace imageproc
{

class BinaryImage;

/**
 * \brief The result of the "find skew" operation.
 * \see SkewFinder
 */
class Skew
{
public:
	/**
	 * \brief The threshold separating good and poor confidence values.
	 * \see confidence()
	 */
	static double const GOOD_CONFIDENCE;
	
	Skew() : m_angle(0.0), m_confidence(0.0) {}
	
	Skew(double angle, double confidence)
	: m_angle(angle), m_confidence(confidence) {}
	
	/**
	 * \brief Get the skew angle in degrees.
	 *
	 * Positive values indicate clockwise skews.
	 */
	double angle() const { return m_angle; }
	
	/**
	 * \brief Get the confidence value.
	 *
	 * The worst possible confidence is 0, while everything
	 * above or equal to GOOD_CONFIDENCE indicates high
	 * confidence level.
	 */
	double confidence() const { return m_confidence; }
private:
	double m_angle;
	double m_confidence;
};


class SkewFinder
{
	DECLARE_NON_COPYABLE(SkewFinder)
public:
	static double const DEFAULT_MAX_ANGLE;
	
	static double const DEFAULT_ACCURACY;
	
	static int const DEFAULT_COARSE_REDUCTION;
	
	static int const DEFAULT_FINE_REDUCTION;
	
	SkewFinder();
	
	/**
	 * \brief Set the maximum skew angle, in degrees.
	 *
	 * The range between 0 and max_angle degrees both clockwise
	 * and counter-clockwise will be checked.
	 * \note The angle can't exceed 45 degrees.
	 */
	void setMaxAngle(double max_angle = DEFAULT_MAX_ANGLE);
	
	/**
	 * \brief Set the desired accuracy.
	 *
	 * Accuracy is the allowed deviation from the actual skew
	 * angle, in degrees.
	 */
	void setDesiredAccuracy(double accuracy = DEFAULT_ACCURACY);
	
	/**
	 * \brief Downscale the image before doing a coarse search.
	 *
	 * Downscaling the image before doing a coarse search will speed
	 * things up, but may reduce accuracy.  Specifying a value
	 * that is too high will cause totally wrong results.
	 * \param reduction The number of times to apply a 2x downscaling
	 *                  to the image before doing a coarse search.
	 *                  The default value is recommended for 300 dpi
	 *                  scans of high quality material.
	 */
	void setCoarseReduction(int reduction = DEFAULT_COARSE_REDUCTION);
	
	/**
	 * \brief Downscale the image before doing a fine search.
	 *
	 * Downscaling the image before doing a fine search will speed
	 * things up, but may reduce accuracy.  Comared to a reduction
	 * before a coarse search, it won't give as much of a speed-up,
	 * but it won't cause completely wrong results.
	 * \param reduction The number of times to apply a 2x downscaling
	 *                  to the image before doing a fine search.
	 *                  The default value is recommended for 300 dpi
	 *                  scans of high quality material.
	 */
	void setFineReduction(int reduction = DEFAULT_FINE_REDUCTION);
	
	/**
	 * \brief Set the horizontal to vertical optical resolution ratio.
	 *
	 * If horizontal and vertical optical resolutions (DPI) differ,
	 * it's necessary to provide their ratio.
	 * \param ratio Horizontal optical resolution divided by vertical one.
	 */
	void setResolutionRatio(double ratio);
	
	/**
	 * \brief Process the image and determine its skew.
	 * \note If the image contains text columns at (slightly) different
	 * angles, one of those angles will be found, with a lower confidence.
	 */
	Skew findSkew(BinaryImage const& image) const;
private:
	static double const LOW_SCORE;
	
	double process(BinaryImage const& src, BinaryImage& dst, double angle) const;
	
	static double calcScore(BinaryImage const& image);
	
	double m_maxAngle;
	double m_accuracy;
	double m_resolutionRatio;
	int m_coarseReduction;
	int m_fineReduction;
};

} // namespace imageproc

#endif
