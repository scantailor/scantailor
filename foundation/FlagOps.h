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

#ifndef FLAGOPS_H_
#define FLAGOPS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DEFINE_FLAG_OPS(type) \
inline type operator&(type lhs, type rhs) \
{ return type(unsigned(lhs) & unsigned(rhs)); } \
\
inline type operator|(type lhs, type rhs) \
{ return type(unsigned(lhs) | unsigned(rhs)); } \
\
inline type operator^(type lhs, type rhs) \
{ return type(unsigned(lhs) ^ unsigned(rhs)); } \
\
inline type operator~(type val) \
{ return type(~unsigned(val)); } \
\
inline type& operator&=(type& lhs, type rhs) \
{ lhs = lhs & rhs; return lhs; } \
\
inline type& operator|=(type& lhs, type rhs) \
{ lhs = lhs | rhs; return lhs; } \
\
inline type& operator^=(type& lhs, type rhs) \
{ lhs = lhs ^ rhs; return lhs; }

#endif
