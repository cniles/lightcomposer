#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

#include "blocking_queue.h"
#include "netlights.h"

extern "C" {
  #include <SDL.h>
  #include <SDL_thread.h>
}

void get_multicast_address(const char* in_string, int port, sockaddr_in *in) {
  memset(in, 0, sizeof(sockaddr_in));

  in->sin_port = htons(port);
  in->sin_family = AF_INET;
  in->sin_addr.s_addr = inet_addr(in_string);
}

int BroadcastThread(void *userdata) {
  netlights_ctx *ctx = (netlights_ctx*)userdata;

  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  sockaddr_in addr;
  get_multicast_address("224.1.1.1", 9999, &addr);

  if (connect(fd, (sockaddr*)&addr, sizeof(sockaddr_in))) {
    std::cout << "Error occurred connecting socket " << std::endl;
    return 1; 
  }

  uint32_t message[100];
  uint32_t *val;
  int count = 0;

  message[0] = ctx->n;
  
  while (ctx->lightsq->pop(&val) >= 0) {
    message[count + 1] = *val;
    delete val;
    count++;
    if (count >= ctx->n) {
      // publish
      int w = write(fd, message, sizeof(uint32_t) * (1 + ctx->n));

      if (w <= 0) {
        std::cout << "Write error occurred" << std::endl;
      } else {
        std::cout << "Wrote " << w << " bytes" << std::endl;
      }
      count = 0;
    }
  }

  std::cout << "Closing broadcast thread" << std::endl;

  close(fd);

  return 0;
}

void netlights_init(netlights_ctx *ctx) {
  SDL_DetachThread(SDL_CreateThread(BroadcastThread, "BroadcastThread", (void*)ctx));
}
