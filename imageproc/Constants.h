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

#ifndef IMAGEPROC_CONSTANTS_H_
#define IMAGEPROC_CONSTANTS_H_

namespace imageproc
{

namespace constants
{

extern double const PI;

extern double const SQRT_2;

/**
 * angle_rad = angle_deg * RED2RAD
 */
extern double const DEG2RAD;

/**
 * angle_deg = angle_rad * RAD2DEG
 */
extern double const RAD2DEG;

/**
 * mm = inch * INCH2MM
 */
extern double const INCH2MM;

/**
 * inch = mm * MM2INCH
 */
extern double const MM2INCH;

/**
 * dots_per_meter = dots_per_inch * DPI2DPM
 */
extern double const DPI2DPM;

/**
 * dots_per_inch = dots_per_meter * DPM2DPI
 */
extern double const DPM2DPI;

} // namespace constants

} // namespace imageproc

#endif
