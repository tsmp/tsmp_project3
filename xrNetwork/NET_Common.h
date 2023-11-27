#pragma once

const u8 NET_TAG_MERGED = 0xE1;
const u8 NET_TAG_NONMERGED = 0xE0;
const u8 NET_TAG_COMPRESSED = 0xC1;
const u8 NET_TAG_NONCOMPRESSED = 0xC0;

#define NET_USE_COMPRESSION 1
#define NET_USE_COMPRESSION_CRC 1

#define NET_LOG_PACKETS 0
#define NET_LOG_COMPRESSION 0
#define NET_DUMP_COMPRESSION 0

#define NET_GUARANTEEDPACKET_DEFAULT 0
#define NET_GUARANTEEDPACKET_IGNORE 1
#define NET_GUARANTEEDPACKET_SEPARATE 2

extern XRNETWORK_API int psNET_GuaranteedPacketMode;

class XRNETWORK_API MultipacketSender
{
public:
    MultipacketSender() = default;
    virtual ~MultipacketSender() = default;

    void SendPacket(const void *packet_data, u32 packet_sz, u32 flags, u32 timeout);
    void FlushSendBuffer(u32 timeout);

protected:
    virtual void _SendTo_LL(const void *data, u32 size, u32 flags, u32 timeout) = 0;

private:
    struct Buffer
    {
        Buffer() : last_flags(0) { buffer.B.count = 0; }

        NET_Packet buffer;
        u32 last_flags;
    };

    void _FlushSendBuffer(u32 timeout, Buffer* buf);

    Buffer _buf;
    Buffer _gbuf;
    xrCriticalSection _buf_cs;
};

class XRNETWORK_API MultipacketReciever
{
public:
    virtual ~MultipacketReciever() = default;
    void RecievePacket(const void *packet_data, u32 packet_sz, u32 param = 0);

protected:
    virtual void _Recieve(const void *data, u32 data_size, u32 param) = 0;
};
