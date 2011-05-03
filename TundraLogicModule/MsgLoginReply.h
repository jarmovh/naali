#pragma once

#include "kNet.h"

struct MsgLoginReply
{
	MsgLoginReply()
	{
		InitToDefault();
	}

	MsgLoginReply(const char *data, size_t numBytes)
	{
		InitToDefault();
		kNet::DataDeserializer dd(data, numBytes);
		DeserializeFrom(dd);
	}

	void InitToDefault()
	{
		reliable = true;
		inOrder = true;
		priority = 100;
	}

    enum { messageID = 101 };
	static inline u32 MessageID() { return 101; }
	static inline const char *Name() { return "LoginReply"; }

	bool reliable;
	bool inOrder;
	u32 priority;

	u8 success;
	u8 userID;

	inline size_t Size() const
	{
		return 1 + 1;
	}

	inline void SerializeTo(kNet::DataSerializer &dst) const
	{
		dst.Add<u8>(success);
		dst.Add<u8>(userID);
	}

	inline void DeserializeFrom(kNet::DataDeserializer &src)
	{
		success = src.Read<u8>();
		userID = src.Read<u8>();
	}

};

