#include "udp-server.h"

bool swUDPServerStart(swUDPServer *server, swEdgeLoop *loop, swSocketAddress *address)
{
  bool rtn = false;
  if (server && loop && address)
  {
    swSocket *sock = (swSocket *)server;
    if (swSocketInit(sock, address->storage.ss_family, SOCK_DGRAM))
    {
      swTCPConnection *conn = (swTCPConnection *)server;
      if (swSocketBind(sock, address))
        rtn = swTCPConnectionStart(conn, loop);
      if (!rtn)
      {
        if (conn->errorFunc)
          conn->errorFunc(conn, swTCPConnectionErrorOtherError);
        swSocketClose(sock);
      }
    }
  }
  return rtn;
}
