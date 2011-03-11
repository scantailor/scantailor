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

#ifndef VIRTUALFUNCTION_H_
#define VIRTUALFUNCTION_H_

template<typename R, typename A1>
class VirtualFunction1
{
public:
	virtual ~VirtualFunction1() {}

	virtual R operator()(A1 arg1) = 0;
};

template<typename Delegate, typename R, typename A1>
class ProxyFunction1 : public VirtualFunction1<R, A1>
{
public:
	ProxyFunction1(Delegate delegate) : m_delegate(delegate) {}

	virtual R operator()(A1 arg1) {
		return m_delegate(arg1);
	}
private:
	Delegate m_delegate;
};

template<typename R, typename A1, typename A2>
class VirtualFunction2
{
public:
	virtual ~VirtualFunction2() {}

	virtual R operator()(A1 arg1, A2 arg2) = 0;
};

template<typename Delegate, typename R, typename A1, typename A2>
class ProxyFunction2 : public VirtualFunction2<R, A1, A2>
{
public:
	ProxyFunction2(Delegate delegate) : m_delegate(delegate) {}

	virtual R operator()(A1 arg1, A2 arg2) {
		return m_delegate(arg1, arg2);
	}
private:
	Delegate m_delegate;
};

template<typename R, typename A1, typename A2, typename A3>
class VirtualFunction3
{
public:
	virtual ~VirtualFunction3() {}

	virtual R operator()(A1 arg1, A2 arg2, A3 arg3) = 0;
};

template<typename Delegate, typename R, typename A1, typename A2, typename A3>
class ProxyFunction3 : public VirtualFunction3<R, A1, A2, A3>
{
public:
	ProxyFunction3(Delegate delegate) : m_delegate(delegate) {}

	virtual R operator()(A1 arg1, A2 arg2, A3 arg3) {
		return m_delegate(arg1, arg2, arg3);
	}
private:
	Delegate m_delegate;
};

#endif
