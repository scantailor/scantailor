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
#include "PictureZoneList.h"

class QDomDocument;
class QDomElement;

namespace output
{

class OutputParams
{
public:
	OutputParams(OutputImageParams const& output_image_params,
		OutputFileParams const& output_file_params,
		OutputFileParams const& automask_file_params,
		PictureZoneList const& zones);
	
	explicit OutputParams(QDomElement const& el);
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
	
	OutputImageParams const& outputImageParams() const { return m_outputImageParams; }
	
	OutputFileParams const& outputFileParams() const { return m_outputFileParams; }

	OutputFileParams const& automaskFileParams() const { return m_automaskFileParams; }

	PictureZoneList const& zones() const { return m_zones; }
private:
	OutputImageParams m_outputImageParams;
	OutputFileParams m_outputFileParams;
	OutputFileParams m_automaskFileParams;
	PictureZoneList m_zones;
};

} // namespace output

#endif
