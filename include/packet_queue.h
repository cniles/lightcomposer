#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#ifdef __cplusplus
extern "C"
{
    #include <libavformat/avformat.h>
    #include <SDL.h>
}
#endif

struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
};

int packet_queue_put(PacketQueue *q, AVPacket *pkt);
void packet_queue_init(PacketQueue *q);
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *interrupt);
int packet_queue_empty(PacketQueue *q);

#endif
