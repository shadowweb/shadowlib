#include "tools/raw-icmpv6/icmpv6-communicator.h"

#include "command-line/option-category.h"
#include "io/socket-io.h"
#include "log/log-manager.h"
#include "protocol/ethernet.h"
#include "protocol/icmp-v6.h"
#include "protocol/protocol-parser.h"
#include "storage/dynamic-buffer.h"

#include <errno.h>
#include <linux/if_ether.h>
#include <netinet/icmp6.h>
#include <netinet/ip.h>

swLoggerDeclareWithLevel(communicatorLogger, "ICMPv6Communicator", swLogLevelInfo);

static swStaticString interfaceName = swStaticStringDefineEmpty;

static int64_t messageSendInterval = 0;
static int64_t messageType         = 0;

swOptionEnumValueName icmpv6MessageEnum[] =
{
  { ND_ROUTER_SOLICIT,    "-rs", "rs" },
  { ND_ROUTER_ADVERT,     "-ra", "ra" },
  { ND_NEIGHBOR_SOLICIT,  "-ns", "ns" },
  { ND_NEIGHBOR_ADVERT,   "-na", "na" },
  { ND_REDIRECT,          "-rd", "rd" },
  { 0,                    NULL,  NULL }
};

swOptionCategoryModuleDeclare(swICMPv6CommunicatorOptions, "ICMPv6 Communicator Options",
  swOptionDeclareScalar("interface|i",        "Interface name",
    "eth0|lo|..",   &interfaceName,         swOptionValueTypeString,  true),
  swOptionDeclareScalar("send-interval",      "Message send interval",
    NULL,           &messageSendInterval,   swOptionValueTypeInt,     true),
  swOptionDeclareScalarEnum("icmpv6-type",     "ICMPv6 message type",
    NULL,           &messageType,           &icmpv6MessageEnum,       true)
);

static void *icmpv6CommunicatorArrayData[1] = {NULL};

static swSocketIO rawSocketIO = { .readTimeout = 0 };
static swDynamicBuffer *readBuffer = NULL;
static swDynamicBuffer *writeBuffer = NULL;
static swProtocolParser parser = { .parsePosition = 0 };
static swProtocolBuilder builder = { .buildPosition = 0 };
static swEdgeTimer writeTimer = { .timerSpec = { .it_interval = {0, 0} } };
static swSocketAddress localIPv4 = {.storage = { .ss_family = AF_INET   } };
static swSocketAddress localIPv6 = {.storage = { .ss_family = AF_INET6  } };
static swSocketAddress localMAC =  {.storage = { .ss_family = AF_PACKET } };

static void writeTimerCallback(swEdgeTimer *timer, uint64_t expiredCount, uint32_t events)
{
  if (timer)
  {
    SW_LOG_DEBUG(&communicatorLogger, "sending message %ld every %ld milliseconds", messageType, messageSendInterval);
    if (swICMPv6BuildRouterSolicit(&builder, &localIPv6, (swEthernetAddress *)&(localMAC.linkLayer.sll_addr), true))
    {
      swProtocolBuilderPrint (&builder);
      swStaticBuffer sendBuffer = swStaticBufferDefineWithLength(builder.buffer.data, builder.buildPosition);
      ssize_t bytesWritten = 0;
      swSocketReturnType ret = swSocketIOWriteTo(&rawSocketIO, &sendBuffer, NULL, &bytesWritten);
      if (ret == swSocketReturnOK)
        SW_LOG_DEBUG(&communicatorLogger, "sent message %ld", messageType);
      else if (ret != swSocketReturnNotReady)
        SW_LOG_ERROR(&communicatorLogger, "failed to send message with error '%s', errno = %d", swSocketReturnTypeTextGet(ret), errno);
      swProtocolBuilderReset(&builder);
    }
    else
      SW_LOG_ERROR(&communicatorLogger, "failed to send message");

  }
}

static bool swICMPv6CommunicatorStartWriteTimer(swEdgeLoop *loop)
{
  bool rtn = false;
  if (swEdgeTimerInit(&writeTimer, writeTimerCallback, true))
  {
    // TODO: find out why swEdgeTimerStart(&writeTimer, loop, 0, messageSendInterval, false)
    // does not work
    if (swEdgeTimerStart(&writeTimer, loop, messageSendInterval, messageSendInterval, false))
      rtn = true;
    else
      swEdgeTimerClose(&writeTimer);
  }
  return rtn;
}

static void swICMPv6CommunicatorStopWriteTimer()
{
  swEdgeTimerStop(&writeTimer);
  swEdgeTimerClose(&writeTimer);
}

static bool bindToInterface(swSocket *sock, swStaticString *interface)
{
  bool rtn = false;
  int index = 0;
  if (swInterfaceInfoGetIndex(interface, &index))
  {
    swSocketAddress address = {0};
    if (swSocketAddressInitLinkLayer(&address, ETH_P_IPV6, index))
    // if (swSocketAddressInitLinkLayer(&address, ETH_P_ALL, index))
    {
      if (swSocketBind(sock, &address))
        rtn = true;
    }
  }
  return rtn;
}

/*
static void dumpPacket(swStaticBuffer *buff)
{
  printf ("Read the following packet\n");
  uint32_t i = 0;
  while (i + 15 < buff->len)
  {
    printf ("%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
      buff->data[i],      buff->data[i + 1],  buff->data[i + 2],  buff->data[i + 3],
      buff->data[i + 4],  buff->data[i + 5],  buff->data[i + 6],  buff->data[i + 7],
      buff->data[i + 8],  buff->data[i + 9],  buff->data[i + 10], buff->data[i + 11],
      buff->data[i + 12], buff->data[i + 13], buff->data[i + 14], buff->data[i + 15]);
    i += 16;
  }
  while (i + 1 < buff->len)
  {
    printf ("%02x%02x ", buff->data[i], buff->data[i + 1]);
    i += 2;
  }
  if (i < buff->len)
    printf ("%02x ", buff->data[i]);
  printf ("\n");
}
*/

static void swICMPv6CommunicatorReadReady(swSocketIO *socketIO)
{
  SW_LOG_DEBUG(&communicatorLogger, "Read ready");
  ssize_t bytesRead = 0;
  swStaticBuffer staticReadBuffer = swStaticBufferDefineWithLength(readBuffer->data, readBuffer->size);
  swSocketReturnType ret = swSocketReturnNone;
  uint16_t iterations = 0;
  while ((ret = swSocketIORead(socketIO, &staticReadBuffer, &bytesRead)))
  {
    if (ret == swSocketReturnOK)
    {
      readBuffer->len = bytesRead;
      // dumpPacket((swStaticBuffer *)readBuffer);
      // TODO: verify that parsing happens correctly, setup timer to send ICMP ND message
      if (swProtocolParserParse(&parser, (swStaticBuffer *)readBuffer))
      {
        printf ("buffer parsed successfully\n");
        swProtocolParserPrint(&parser);
      }
      else
        printf ("error parsing buffer\n");
    }
    else if (ret == swSocketReturnNotReady)
      break;
    else
    {
      SW_LOG_ERROR(&communicatorLogger, "Read returned error '%s'", swSocketReturnTypeTextGet(ret));
      break;
    }
    iterations++;
    if (iterations > 16)
      break;
  }

}

static void swICMPv6CommunicatorWriteReady(swSocketIO *socketIO)
{
  SW_LOG_DEBUG(&communicatorLogger, "Write ready");
}

static bool swICMPv6CommunicatorReadTimeout(swSocketIO *socketIO)
{
  SW_LOG_DEBUG(&communicatorLogger, "Read timeout");
  return true;
}

static bool swICMPv6CommunicatorWriteTimeout(swSocketIO *socketIO)
{
  SW_LOG_DEBUG(&communicatorLogger, "Write timeout");
  return false;
}

static void swICMPv6CommunicatorError(swSocketIO *socketIO, swSocketIOErrorType errorCode)
{
  SW_LOG_DEBUG(&communicatorLogger, "SocketIO error: '%s'", swSocketIOErrorTextGet(errorCode));
}

static void swICMPv6CommunicatorClose(swSocketIO *socketIO)
{
  swEdgeLoop **loopPtr = (swEdgeLoop **)icmpv6CommunicatorArrayData[0];
  swEdgeLoopBreak(*loopPtr);
}

static void swICMPv6CommunicatorStop()
{
  SW_LOG_DEBUG(&communicatorLogger, "Stoppint ICMPv6 communicator");
  swICMPv6CommunicatorStopWriteTimer();
  swSocketIOCleanup(&rawSocketIO);
  swDynamicBufferDelete(readBuffer);
  readBuffer = NULL;
  swProtocolParserRelease(&parser);
  swProtocolBuilderRelease(&builder);
  swDynamicBufferDelete(writeBuffer);
  writeBuffer = NULL;
}

static bool swICMPv6CommunicatorStart()
{
  bool rtn = false;
  swEdgeLoop **loopPtr = (swEdgeLoop **)icmpv6CommunicatorArrayData[0];
  if (loopPtr && *loopPtr)
  {
    if (swInterfaceInfoGetAddress(&interfaceName, &localIPv4) && swInterfaceInfoGetAddress(&interfaceName, &localIPv6) && swInterfaceInfoGetAddress(&interfaceName, &localMAC))
    {
      if ((writeBuffer = swDynamicBufferNew(IP_MAXPACKET)))
      {
        swStaticBuffer staticWriteBuffer = swStaticBufferDefineWithLength(writeBuffer->data, writeBuffer->size);
        if (swProtocolBuilderInit(&builder, &staticWriteBuffer))
        {
          if (swProtocolParserInit(&parser))
          {
            if ((readBuffer = swDynamicBufferNew(IP_MAXPACKET)))
            {
              if (swSocketIOInit(&rawSocketIO))
              {
                // SOCK_DGRAM, htons(ETH_P_ALL), htons(ETH_P_IPV6)
                if (swSocketInitWithProtocol(&(rawSocketIO.sock), AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))
                {
                  if (bindToInterface(&(rawSocketIO.sock), &interfaceName))
                  {
                    swSocketIO *io = &rawSocketIO;
                    swSocketIOReadTimeoutSet(io, 60000);
                    swSocketIOWriteTimeoutSet(io, 1000);
                    swSocketIOReadReadyFuncSet(io, swICMPv6CommunicatorReadReady);
                    swSocketIOWriteReadyFuncSet(io, swICMPv6CommunicatorWriteReady);
                    swSocketIOReadTimeoutFuncSet(io, swICMPv6CommunicatorReadTimeout);
                    swSocketIOWriteTimeoutFuncSet(io, swICMPv6CommunicatorWriteTimeout);
                    swSocketIOErrorFuncSet(io, swICMPv6CommunicatorError);
                    swSocketIOCloseFuncSet(io, swICMPv6CommunicatorClose);
                    if (swSocketIOStart(&rawSocketIO, *loopPtr))
                    {
                      if (swICMPv6CommunicatorStartWriteTimer(*loopPtr))
                        rtn = true;
                    }
                  }
                }
                else
                  SW_LOG_ERROR(&communicatorLogger, "Failed to init raw I/O socket");
              }
            }
          }
        }
        if (!rtn)
          swICMPv6CommunicatorStop();
      }
    }
  }
  return rtn;
}

static swInitData icmpv6CommunicatorData = {.startFunc = swICMPv6CommunicatorStart, .stopFunc = swICMPv6CommunicatorStop, .name = "ICMPv6 Communicator"};

swInitData *swICMPv6CommunicatorDataGet(swEdgeLoop **loopPtr)
{
  icmpv6CommunicatorArrayData[0] = loopPtr;
  return &icmpv6CommunicatorData;
}
