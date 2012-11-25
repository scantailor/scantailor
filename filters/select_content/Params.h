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

#ifndef SELECT_CONTENT_PARAMS_H_
#define SELECT_CONTENT_PARAMS_H_

#include "Dependencies.h"
#include "AutoManualMode.h"
#include "Margins.h"
#include <QRectF>
#include <QSizeF>
#include <cmath>

class QDomDocument;
class QDomElement;
class QString;

namespace select_content
{

class Params
{
public:
	// Member-wise copying is OK.
	
	Params(QRectF const& rect, QSizeF const& size_mm,
		Dependencies const& deps, AutoManualMode mode);
	
	Params(QRectF const& rect, QSizeF const& size_mm,
		Dependencies const& deps, AutoManualMode mode, bool contentDetect, bool pageDetect, bool fineTuning);
	
	Params(Dependencies const& deps);
	
	Params(QDomElement const& filter_el);
	
	~Params();
	
	QRectF const& contentRect() const { return m_contentRect; }
	QRectF const& pageRect() const { return m_pageRect; }

	QSizeF const& contentSizeMM() const { return m_contentSizeMM; }
	
	Dependencies const& dependencies() const { return m_deps; }
	
	AutoManualMode mode() const { return m_mode; }

	double deviation() const { return m_deviation; }
	void setDeviation(double d) { m_deviation = d; }
	void computeDeviation(double avg) { m_deviation = avg - sqrt(m_contentSizeMM.width() * m_contentSizeMM.height() / 4); }
	bool isDeviant(double std, double max_dev) { return (max_dev*std) < fabs(m_deviation); }

	bool isContentDetectionEnabled() const { return m_contentDetect; };
	bool isPageDetectionEnabled() const { return m_pageDetect; };
	bool isFineTuningEnabled() const { return m_fineTuneCorners; };
    
    Margins pageBorders() const { return m_pageBorders; };
    void setPageBorders(Margins borders) { m_pageBorders = borders; };

	void setMode(AutoManualMode const& mode) { m_mode = mode; };
	void setContentRect(QRectF const& rect) { m_contentRect = rect; };
	void setPageRect(QRectF const& rect) { m_pageRect = rect; };
	void setContentSizeMM(QSizeF const& size) { m_contentSizeMM = size; };
	void setDependencies(Dependencies const& deps) { m_deps = deps; };
	void setContentDetect(bool detect) { m_contentDetect = detect; };
	void setPageDetect(bool detect) { m_pageDetect = detect; };
	void setFineTuneCorners(bool fine_tune) { m_fineTuneCorners = fine_tune; };
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	QRectF m_contentRect;
	QRectF m_pageRect;
    Margins m_pageBorders;
	QSizeF m_contentSizeMM;
	Dependencies m_deps;
	AutoManualMode m_mode;
	bool m_contentDetect;
	bool m_pageDetect;
	bool m_fineTuneCorners;
	double m_deviation;
};

} // namespace select_content

#endif
