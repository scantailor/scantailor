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

#include "ContentSpanFinder.h"
#include "imageproc/SlicedHistogram.h"
#include "Span.h"
#include <vector>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/test/auto_unit_test.hpp>

namespace Tests
{

using namespace boost::lambda;
using namespace imageproc;

BOOST_AUTO_TEST_SUITE(ContentSpanFinderTestSuite);

BOOST_AUTO_TEST_CASE(test_empty_input)
{
	ContentSpanFinder span_finder;
	
	std::vector<Span> spans;
	void (std::vector<Span>::*push_back) (const Span&) =
		&std::vector<Span>::push_back;
	span_finder.find(
		SlicedHistogram(),
		boost::lambda::bind(push_back, var(spans), _1)
	);
	
	BOOST_CHECK(spans.empty());
}

BOOST_AUTO_TEST_CASE(test_min_content_width)
{
	SlicedHistogram hist;
	hist.setSize(9);
	hist[0] = 0;
	hist[1] = 1;
	hist[2] = 0;
	hist[3] = 1;
	hist[4] = 1;
	hist[5] = 1;
	hist[6] = 0;
	hist[7] = 1;
	hist[8] = 1;
	
	ContentSpanFinder span_finder;
	span_finder.setMinContentWidth(2);
	
	std::vector<Span> spans;
	void (std::vector<Span>::*push_back) (const Span&) =
		&std::vector<Span>::push_back;
	span_finder.find(hist, boost::lambda::bind(push_back, var(spans), _1));
	
	BOOST_REQUIRE(spans.size() == 2);
	BOOST_REQUIRE(spans[0] == Span(3, 3+3));
	BOOST_REQUIRE(spans[1] == Span(7, 7+2));
}

BOOST_AUTO_TEST_CASE(test_min_whitespace_width)
{
	SlicedHistogram hist;
	hist.setSize(9);
	hist[0] = 0;
	hist[1] = 1;
	hist[2] = 0;
	hist[3] = 1;
	hist[4] = 1;
	hist[5] = 0;
	hist[6] = 0;
	hist[7] = 1;
	hist[8] = 1;
	
	ContentSpanFinder span_finder;
	span_finder.setMinWhitespaceWidth(2);
	
	std::vector<Span> spans;
	void (std::vector<Span>::*push_back) (const Span&) =
		&std::vector<Span>::push_back;
	span_finder.find(hist, boost::lambda::bind(push_back, var(spans), _1));
	
	BOOST_REQUIRE(spans.size() == 2);
	BOOST_REQUIRE(spans[0] == Span(1, 1+4));
	BOOST_REQUIRE(spans[1] == Span(7, 7+2));
}

BOOST_AUTO_TEST_CASE(test_min_content_and_whitespace_width)
{
	SlicedHistogram hist;
	hist.setSize(9);
	hist[0] = 0;
	hist[1] = 1;
	hist[2] = 0;
	hist[3] = 1;
	hist[4] = 1;
	hist[5] = 0;
	hist[6] = 0;
	hist[7] = 1;
	hist[8] = 0;
	
	ContentSpanFinder span_finder;
	span_finder.setMinContentWidth(2);
	span_finder.setMinWhitespaceWidth(2);
	
	std::vector<Span> spans;
	void (std::vector<Span>::*push_back) (const Span&) =
		&std::vector<Span>::push_back;
	span_finder.find(hist, boost::lambda::bind(push_back, var(spans), _1));
	
	// Note that although a content block at index 1 is too short,
	// it's still allowed to merge with content at positions 3 and 4
	// because the whitespace between them is too short as well.
	
	BOOST_REQUIRE(spans.size() == 1);
	BOOST_REQUIRE(spans[0] == Span(1, 1+4));
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace Tests
