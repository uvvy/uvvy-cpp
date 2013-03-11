#pragma once

namespace ssu {

/**
 * User-space interface to the stream.
 */
class stream
{
	abstract_stream* strm;//XXX rename

public:
	typedef uint16_t id_t;      ///< Stream ID within channel.
	typedef uint32_t byteseq_t; ///< Stream byte sequence number.
	typedef uint64_t counter_t; ///< Counter for SID assignment.

	struct unique_stream_id_t
	{
		counter_t counter; ///< Stream counter in channel
		std::vector<char> half_channel_id; ///< Unique channel+direction ID ("half-channel id")
	};
};

} // namespace ssu
