/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef OUTPUT_OUTPUT_PARAMS_H_
#define OUTPUT_OUTPUT_PARAMS_H_

#include "OutputImageParams.h"
#include "OutputFileParams.h"

class QDomDocument;
class QDomElement;

namespace output
{

class OutputParams
{
public:
	OutputParams(OutputImageParams const& image_params,
		OutputFileParams const& file_params);
	
	explicit OutputParams(QDomElement const& el);
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
	
	OutputImageParams const& imageParams() const { return m_imageParams; }
	
	OutputFileParams const& fileParams() const { return m_fileParams; }
private:
	OutputImageParams m_imageParams;
	OutputFileParams m_fileParams;
};

} // namespace output

#endif
