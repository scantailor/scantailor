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

#ifndef ABSTRACTCOMMAND_H_
#define ABSTRACTCOMMAND_H_

#include "RefCountable.h"

template<typename R>
class AbstractCommand0 : public RefCountable
{
public:
	virtual R operator()() = 0;
};

template<typename R, typename A1>
class AbstractCommand1 : public RefCountable
{
public:
	virtual R operator()(A1 arg1) = 0;
};

template<typename R, typename T1, typename T2>
class AbstractCommand2 : public RefCountable
{
public:
	virtual R operator()(T1 arg1, T2 arg2) = 0;
};

#endif
