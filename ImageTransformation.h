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

#ifndef IMAGETRANSFORMATION_H_
#define IMAGETRANSFORMATION_H_

#include "OrthogonalRotation.h"
#include "Dpi.h"
#include <QTransform>
#include <QPolygonF>
#include <QRectF>

/**
 * \brief Provides a transformed view of an image.
 *
 * \anchor transformations
 * Suppose we need to make the following transformations
 * to a particular image (in this order):
 * \li Scale it, to achieve the desired optical resolution.
 * \li Rotate it by a multiple of 90 degrees,
 *     swapping its width and height if necessary.
 * \li Crop it, reducing its width and height.
 * \li Apply a rotation around the image center,
 *     increasing its width and height as necessary.
 *
 * Instead of actually modifying the image, we provide
 * a mapping from the original image coordinates to the new ones.
 * Note that all transformation steps are optional.
 */
class ImageTransformation
{
public:
	// Member-wise copying is OK.
	
	ImageTransformation(QRectF const& orig_image_rect, Dpi const& orig_dpi);
	
	~ImageTransformation();
	
	/**
	 * \brief Set the 1st step transformation, recalculating the following ones.
	 *
	 * \see \ref transformations Transformations.
	 */
	void preScaleToDpi(Dpi const& dpi);
	
	/**
	 * \brief Set the 1st step transformation, recalculating the following ones.
	 *
	 * Suppose the original image DPI is 300x600.  This will scale it to
	 * 300x300, the minimum of two.
	 *
	 * \note This transformation is applied automatically on construction.
	 *
	 * \see \ref transformations Transformations.
	 */
	void preScaleToEqualizeDpi();
	
	/**
	 * \brief Get the original image DPI.
	 */
	Dpi const& origDpi() const { return m_origDpi; }
	
	/**
	 * \brief Get the target DPI for pre-scaling.
	 *
	 * Note that if the original DPI was assymetric, pre-scaling to
	 * a symmetric DPI will be applied implicitly.
	 */
	Dpi const& preScaledDpi() const { return m_preScaledDpi; }
	
	/**
	 * \brief Set the 2nd step transformation, resetting the following ones.
	 *
	 * \see \ref transformations Transformations.
	 */
	void setPreRotation(OrthogonalRotation rotation);
	
	/**
	 * \brief Returns the 2nd step rotation.
	 */
	OrthogonalRotation preRotation() const { return m_preRotation; }
	
	/**
	 * \brief Set the 3rd step transformation, resetting the following ones.
	 *
	 * Providing a null polygon has the same effect as providing a polygon
	 * that covers the entire image.  A crop area that exceedes the image
	 * is allowed.
	 *
	 * \see \ref transformations Transformations.
	 */
	void setCropArea(QPolygonF const& area);
	
	/**
	 * \brief Returns the crop area after all transformations.
	 *
	 * If no crop area was set, the whole image is assumed to be
	 * the crop area.
	 */
	QPolygonF const& resultingCropArea() const { return m_resultingCropArea; }
	
	/**
	 * \brief Set the 4th step transformation.
	 *
	 * \see \ref transformations Transformations.
	 */
	void setPostRotation(double degrees);
	
	/**
	 * \brief Returns the 4th step rotation in degrees, as specified.
	 */
	double postRotation() const { return m_postRotation; }
	
	/**
	 * \brief Returns the sine of the 4th step rotation angle.
	 */
	double postRotationSin() const { return m_postRotateXform.m12(); }
	
	/**
	 * \brief Returns the cosine of the 3rd step rotation angle.
	 */
	double postRotationCos() const { return m_postRotateXform.m11(); }
	
	/**
	 * \brief Returns the transformation matrix from the original
	 *        to resulting image coordinates.
	 */
	QTransform const& transform() const { return m_transform; }
	
	/**
	 * \brief Returns the transformation matrix from the resulting
	 *        to original image coordinates.
	 */
	QTransform const& transformBack() const { return m_invTransform; }
	
	/**
	 * \brief Returns the original image rectangle, as specified.
	 */
	QRectF const& origRect() const { return m_origRect; }
	
	/**
	 * \brief Returns the image rectangle after pre-scaling and pre-rotation.
	 */
	QRectF rectBeforeCropping() const;
	
	/**
	 * \brief Returns the resulting image rectangle.
	 *
	 * The top-left corner of the resulting rectangle is expected
	 * to be very close to (0, 0), assuming the original rectangle
	 * had it at (0, 0), but it's not guaranteed to be exactly there.
	 */
	QRectF const& resultingRect() const { return m_resultingRect; }
private:
	QTransform calcCropXform(QPolygonF const& crop_area);
	
	QTransform calcPostRotateXform(double degrees);
	
	void resetCropArea();
	
	void resetPostRotation();
	
	void update();
	
	QTransform m_preScaleXform;
	QTransform m_preRotateXform;
	QTransform m_cropXform;
	QTransform m_postRotateXform;
	QTransform m_transform;
	QTransform m_invTransform;
	double m_postRotation;
	QRectF m_origRect;
	QRectF m_croppedRect;
	QRectF m_resultingRect;
	QPolygonF m_cropArea;
	QPolygonF m_resultingCropArea;
	Dpi m_origDpi;
	Dpi m_preScaledDpi;
	OrthogonalRotation m_preRotation;
};

#endif
