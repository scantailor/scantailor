/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "OpenGLSupport.h"
#include "config.h"
#include <QSettings>
#ifdef ENABLE_OPENGL
#include <QGLFormat>
#include <QGLWidget>
#endif

bool
OpenGLSupport::supported()
{
#ifndef ENABLE_OPENGL
	return false;
#else
	if (!QGLFormat::hasOpenGL()) {
		return false;
	}

	QGLFormat format;
	format.setSampleBuffers(true);
	format.setStencil(true);
	format.setAlpha(true);

	QGLWidget widget(format);
	format = widget.format();

	if (!format.sampleBuffers()) {
		return false;
	}
	if (!format.stencil()) {
		return false;
	}
	if (!format.alpha()) {
		return false;
	}

	return true;
#endif
}
