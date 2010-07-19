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

#ifndef IMAGEPROC_REVERSE_ARC_LENGTH_MAPPER_H_
#define IMAGEPROC_REVERSE_ARC_LENGTH_MAPPER_H_

#include <vector>

namespace imageproc
{

/**
 * \brief Maps from arc length to function argument.
 *
 * Suppose we have a discrete function where we only know its
 * values for a given set of arguments.  This class provides a way
 * to calculate the function's argument corresponding to a certain
 * arc length.  We consider the arc length between two adjacent samples
 * to be monotonously increasing, that is we consider adjacent samples
 * to be connected by straight lines.
 */
class ReverseArcLengthMapper
{
	// Member-wise copying is OK.
public:
	class Hint
	{
		friend class ReverseArcLengthMapper;
	public:
		Hint();
	private:
		void update(int new_segment);

		int m_lastSegment;
		int m_direction;
	};
	
	ReverseArcLengthMapper();

	/**
	 * \brief Adds an x -> f(x) sample.
	 *
	 * Note that samples must be added in order of increasing arguments.
	 */
	void addSample(double x, double fx);

	/**
	 * \brief Scales arc lengths at every sample so that the
	 *        total arc length becomes equal to the given value.
	 *
	 * Obviously, this should be done after all samples have been added.
	 */
	void normalizeRange(double total_arc_len);

	/**
	 * \brief Maps from arc length to the corresponding function argument.
	 *
	 * This works even for arc length beyond the first or last samples.
	 * When interpolation is impossible, the closest sample is returned.
	 * If no samples are present, zero is returned.
	 */
	double map(double arc_len, Hint& hint);
private:
	struct Sample
	{
		double x;
		double arcLen;

		Sample(double x, double arc_len) : x(x), arcLen(arc_len) {}
	};

	double interpolateBySegment(double arc_len, int segment) const;

	std::vector<Sample> m_samples;
	double m_prevFX;
};

} // namespace imageproc

#endif
