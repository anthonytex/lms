/*
 * Copyright (C) 2015 Emeric Poupon
 *
 * This file is part of LMS.
 *
 * LMS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LMS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LMS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Wt/WText>
#include <Wt/WTemplate>
#include <Wt/WPushButton>
#include <Wt/WImage>

#include "LmsApplication.hpp"

#include "ArtistSearch.hpp"

namespace UserInterface {
namespace Mobile {

using namespace Database;

ArtistSearch::ArtistSearch(Wt::WString title, Wt::WContainerWidget *parent)
: Wt::WContainerWidget(parent),
_count(0)
{
	Wt::WTemplate* artistSearch = new Wt::WTemplate(this);
	artistSearch->setTemplateText(Wt::WString::tr("wa-artist-search"));

	Wt::WTemplate *titleTemplate = new Wt::WTemplate(this);
	titleTemplate->setTemplateText(Wt::WString::tr("mobile-search-title"));
	titleTemplate->bindString("text", title);

	artistSearch->bindWidget("title", titleTemplate);

	_contents = new Wt::WContainerWidget();
	artistSearch->bindWidget("release-container", _contents );

	_showMore = new Wt::WTemplate();
	_showMore->setTemplateText(Wt::WString::tr("mobile-search-more"));
	_showMore->bindString("text", "Tap to show more results...");
	_showMore->hide();
	_showMore->clicked().connect(std::bind([=] {
		_sigShowMore.emit();
	}));

	artistSearch->bindWidget("show-more", _showMore);
}

void
ArtistSearch::clear()
{
	_contents->clear();
	_count = 0;
	_showMore->hide();
}

void
ArtistSearch::search(Database::SearchFilter filter, size_t nb)
{
	_filter = filter;

	clear();
	addResults(nb);
}

static
std::vector<Artist::pointer>
getArtists(SearchFilter filter, size_t offset, size_t nb, bool &moreResults)
{
	std::vector<Artist::pointer> artists = Artist::getByFilter(DboSession(), filter, offset, nb + 1);

	if (artists.size() == nb + 1)
	{
		moreResults = true;
		artists.pop_back();
	}
	else
		moreResults = false;

	return artists;
}

void
ArtistSearch::addResults(std::size_t nb)
{
	Wt::Dbo::Transaction transaction(DboSession());

	bool moreResults;
	std::vector<Artist::pointer> artists = getArtists(_filter, _count, nb, moreResults);

	for (Artist::pointer artist : artists)
	{
		Wt::WTemplate* res = new Wt::WTemplate(_contents);
		res->setTemplateText(Wt::WString::tr("wa-artist-res"));

		Wt::WAnchor *coverAnchor = new Wt::WAnchor(Wt::WLink(Wt:: WLink::InternalPath, "/audio/artist/" + std::to_string(artist.id())));
		Wt::WImage *artistImg = new Wt::WImage(coverAnchor);
		artistImg->setStyleClass("center-block"); // TODO move in css?
		artistImg->setStyleClass("release_res_shadow release_img-responsive"); // TODO move in css?

		res->bindWidget("gif", coverAnchor);
		res->bindString("name", Wt::WString::fromUTF8(artist->getName(), Wt::PlainText));
	}

	_count += artists.size();

	if (moreResults)
		_showMore->show();
	else
		_showMore->hide();
}

} // namespace Mobile
} // namespace UserInterface
