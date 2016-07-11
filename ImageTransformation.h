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
 * -#  Scale it, to achieve the desired optical resolution.
 *     It's generally done only in case input had different
 *     horizontal and vertical DPI.  We refer to this operation
 *     as pre-scale.
 * -#  Rotate it by a multiple of 90 degrees,
 *     swapping its width and height if necessary.
 *     That's done on the "Fix Orientation" stage.  We refer to
 *     this operation as pre-rotate.
 * -#  Crop it, reducing its width and height.
 *     That's done on the "Split Pages" stage, where we crop
 *     to the bounding box of a polygon formed by cutters and
 *     remaining image edges.  We refer to this operation as
 *     pre-crop.
 * -#  Apply a rotation around the image center,
 *     increasing its width and height as necessary.
 *     That's done on the "Deskew" stage.  We refer to this
 *     operation as post-rotate.
 * -#  Apply another crop, or maybe extend the image boundaries,
 *     in order to place the required margins around content.
 *     That's done on the "Margins" stage.  We refer to this operation
 *     as post-crop.
 * -#  Scale the image to desired DPI.  That's done on the "Output" stage.
 *     We refer to this operation as post-scale.
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
	 * Note that if the original DPI was asymmetric, pre-scaling to
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
	void setPreCropArea(QPolygonF const& area);

	/**
	 * \brief Get the effective pre-crop area in pre-rotated coordinates.
	 *
	 * If pre-crop area was explicitly set with setPreCropArea(), then
	 * this function returns it as is.  Otherwise, the whole available
	 * area is returned.
	 */
	QPolygonF const& preCropArea() const { return m_preCropArea; }

	/**
	 * \brief Returns the pre-crop area after all transformations.
	 *
	 * If no pre-crop area was set, the whole image is assumed to be
	 * the pre-crop area.
	 */
	QPolygonF const& resultingPreCropArea() const { return m_resultingPreCropArea; }

	/**
	 * \brief Set the 4th step transformation, resetting  the following ones.
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
	 * \brief Set the 5th step transformation, resetting the following ones.
	 */
	void setPostCropArea(QPolygonF const& area);

	/**
	 * \brief Returns the post-crop area after all transformations.
	 *
	 * If no post-crop area was set, the whole image is assumed to be
	 * the post-crop area.
	 */
	QPolygonF const& resultingPostCropArea() const { return m_resultingPostCropArea; }

	/**
	 * \brief Set the 6th step transformation.
	 *
	 * Passing a null (default constructed) Dpi means "don't apply post-scaling".
	 */
	void postScaleToDpi(Dpi const& dpi);

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

	QTransform calcPostScaleXform(Dpi const& target_dpi);

	void resetPreCropArea();

	void resetPostRotation();

	void resetPostCrop();

	void resetPostScale();

	void update();

	QTransform m_preScaleXform;
	QTransform m_preRotateXform;
	QTransform m_preCropXform;
	QTransform m_postRotateXform;
	QTransform m_postCropXform;
	QTransform m_postScaleXform;
	QTransform m_transform;
	QTransform m_invTransform;
	double m_postRotation;
	QRectF m_origRect;
	QRectF m_resultingRect; // Managed by update().
	QPolygonF m_preCropArea;
	QPolygonF m_resultingPreCropArea; // Managed by update().
	QPolygonF m_postCropArea;
	QPolygonF m_resultingPostCropArea; // Managed by update().
	Dpi m_origDpi;
	Dpi m_preScaledDpi; // Always set, as preScaleToEqualizeDpi() is called from the constructor.
	Dpi m_postScaledDpi; // Default constructed object if no post-scaling.
	OrthogonalRotation m_preRotation;
};

#endif
