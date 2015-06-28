/*
 * Copyright (C) 2013 Emeric Poupon
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


#include <list>
#include <boost/foreach.hpp>

#include "logger/Logger.hpp"

#include "av/InputFormatContext.hpp"
#include "cover/CoverArtGrabber.hpp"

#include "InputMediaFile.hpp"

namespace Transcode
{

bool getStreamType(enum AVMediaType type, Transcode::Stream::Type& streamType)
{
	switch (type) {
		case AVMEDIA_TYPE_VIDEO:	streamType = Transcode::Stream::Video; return true;
		case AVMEDIA_TYPE_AUDIO:	streamType = Transcode::Stream::Audio; return true;
		case AVMEDIA_TYPE_SUBTITLE:	streamType = Transcode::Stream::Subtitle; return true;
		default:
			return false;
	}
}


InputMediaFile::InputMediaFile(const boost::filesystem::path& p)
: _path(p)
{
	Av::InputFormatContext input(_path);
	input.findStreamInfo();

	// Calculate estimated duration
	if (input.getDurationSecs())
		_duration	= boost::posix_time::seconds(input.getDurationSecs() + 1);

	// Get input streams
	std::vector<Av::Stream> avStreams = input.getStreams();

	std::list<enum AVMediaType> avMediaTypes;	// List of encountered streams
	for (std::size_t avStreamId = 0; avStreamId < avStreams.size(); ++avStreamId)
	{
		Av::Stream&	avStream(avStreams[avStreamId]);
		Stream::Type	type;

		if (getStreamType(avStream.getCodecContext().getType(), type))
		{
			// Reject Video stream hat are in fact cover arts
			if (avStream.hasAttachedPic())
				continue;

			avMediaTypes.push_back(avStream.getCodecContext().getType());

			LMS_LOG(MOD_TRANSCODE, SEV_DEBUG) << "Stream idx " << avStreamId << ", type = " << type << ", bitrate = " << avStream.getCodecContext().getBitRate() << ", codec desc = " << avStream.getCodecContext().getCodecDesc();

			_streams.push_back( Stream(avStreamId,
						type,
						avStream.getCodecContext().getBitRate(),
						avStream.getMetadata().get("language"),	// TODO define somewhere else?
						avStream.getCodecContext().getCodecDesc()
						));
		}
	}

	avMediaTypes.unique();
	// Scan for best streams
	BOOST_FOREACH(enum AVMediaType type, avMediaTypes)
	{
		Av::Stream::Idx index;

		if (input.getBestStreamIdx(type, index))
		{
			Stream::Type streamType;
			if (getStreamType(type, streamType))
				_bestStreams.insert(std::make_pair( streamType, index) );
		}
		else
			LMS_LOG(MOD_TRANSCODE, SEV_WARNING) << "Cannot find best stream for type " << type;
	}
}

std::vector<Stream>
InputMediaFile::getStreams(Stream::Type type) const
{
	std::vector<Stream> res;
	BOOST_FOREACH(const Stream& stream, _streams)
	{
		if (stream.getType() == type)
			res.push_back(stream);
	}
	return res;
}

const Stream&
InputMediaFile::getStream(Stream::Id index) const
{
	BOOST_FOREACH(const Stream& stream, _streams)
	{
		if (stream.getId() == index)
			return stream;
	}
	LMS_LOG(MOD_TRANSCODE, SEV_CRIT) << "Cannot find stream index " << index << " in stream map!";
	throw std::runtime_error("InputMediaFile::getStream, cannot find stream idx");
}


} // namespace Transcode
