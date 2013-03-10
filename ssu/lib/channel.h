#pragma once

/**
 * Abstract base class for channel encryption and authentication schemes.
 */
class channel_armor
{
	friend class channel;

protected:
	virtual packet transmit_encode(uint64_t pktseq, packet& pkt) = 0;
	virtual bool receive_decode(uint64_t pktseq, packet& pkt) = 0;
	virtual ~channel_armor();
};

/**
 * Abstract base class representing a channel between a local Socket and a remote endpoint.
 */
class channel : public socket_channel
{
	virtual void start(bool initiate);
	virtual void stop();
};


