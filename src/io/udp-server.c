#include "udp-server.h"

bool swUDPServerStart(swUDPServer *server, swEdgeLoop *loop, swSocketAddress *address)
{
  bool rtn = false;
  if (server && loop && address)
  {
    swSocket *sock = (swSocket *)server;
    if (swSocketInit(sock, address->storage.ss_family, SOCK_DGRAM))
    {
      swSocketIO *io = (swSocketIO *)server;
      if (swSocketBind(sock, address))
        rtn = swSocketIOStart(io, loop);
      if (!rtn)
      {
        if (io->errorFunc)
          io->errorFunc(io, swSocketIOErrorOtherError);
        swSocketClose(sock);
      }
    }
  }
  return rtn;
}
