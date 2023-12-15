#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

sockaddr_in create_listen_addr_in(const char *in_string, int port) {
  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  if(inet_aton(in_string, &(sin.sin_addr)) < 0) {
    std::cout << "Failed to read multicast address " << errno << std::endl;
  }

  sin.sin_port = htons(port);
  sin.sin_family = AF_INET;

  return sin;
}

void bind_and_listen(SDL_cond *ready_cond, sockaddr_in *sin) {
  int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (fd < 0) {
    std::cout << "Failed to open socket" << std::endl;
    return;
  }

  ip_mreq opt_val;
  opt_val.imr_multiaddr = sin->sin_addr;
  opt_val.imr_interface.s_addr = htonl(INADDR_ANY);

  if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &opt_val, sizeof(ip_mreqn)) < 0) {
    std::cout << "setsockopt failed: IP_ADD_MEMBERSHIP " << errno << std::endl;
    return;
  }

  if (bind(fd, (sockaddr*)sin, sizeof(sockaddr_in))) {
    std::cout << "Bind failed" << std::endl;
    return;
  }

  // unblock sender now that we're listening
  SDL_CondSignal(ready_cond);

  std::cout << "Listening" << std ::endl;

  uint32_t message[100];

  int r;

  if ((r = read(fd, message, sizeof(message))) < 0) {
    std::cout << "Failed to read message off socket" << std::endl;
    return;
  }

  for (int i = 0; i <= message[0]; ++i) {
    std::cout << message[i] << ", ";
  }

  std::cout << std::endl;
  close(fd);
}

int bind_and_listen_thread(void *data) {
  sockaddr_in in = create_listen_addr_in("224.1.1.1", 9999);
  bind_and_listen((SDL_cond*)data, &in);
  return 0;
}

SDL_Thread *listen_for_traffic(SDL_cond *readyCond) { 
  SDL_Thread *thread = SDL_CreateThread(bind_and_listen_thread, "bind_and_listen", readyCond);
  
  return thread;
}
