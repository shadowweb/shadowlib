#include "protocol/ethernet.h"

#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>

#include "collections/hash-map-linear.h"
#include "core/memory.h"

bool swEthernetAddressGetLocal(swSocket *sock, swStaticString *interfaceName, swEthernetAddress *address)
{
  bool rtn = false;
  if (sock && interfaceName && address)
  {
    struct ifreq request = {{{0}}, {{0}}};
    strncpy(request.ifr_name, interfaceName->data, IF_NAMESIZE);
    if (ioctl(sock->fd, SIOCGIFHWADDR, &request) >= 0)
    {
      if (request.ifr_hwaddr.sa_family == ARPHRD_ETHER)
      {
        *address = *((struct ether_addr *)request.ifr_hwaddr.sa_data);
        rtn = true;
      }
    }
  }
  return rtn;
}

swEthernetAddress* swEthernetAddressGetMulticast()
{
  static swEthernetAddress multicastMAC = {{0x33, 0x33, 0x00, 0x00, 0x00, 0x02}};
  return &multicastMAC;
}

// I can also add a mapping here from IP addresses to interfaces (something to think about if I need it)
typedef struct swInterfaceInfoData {
  swHashMapLinear   dataMap;
  swInterfaceInfo  *dataStorage;
  bool              inited;
} swInterfaceInfoData;

static swInterfaceInfoData interfaceData = { .inited = false };

static void swInterfaceInfoReleseStorage(uint32_t count)
{
  if (interfaceData.dataStorage)
  {
    swHashMapLinearClear(&(interfaceData.dataMap));
    swMemoryFree(interfaceData.dataStorage);
    interfaceData.dataStorage = NULL;
  }
}

static uint32_t swInterfaceInfoInitStorage()
{
  uint32_t count = 0;
  struct if_nameindex *nameIndexPair = if_nameindex();
  for (struct if_nameindex *pair = nameIndexPair; pair->if_index != 0 && pair->if_name != NULL; pair++)
    count++;
  if (count && (interfaceData.dataStorage = swMemoryCalloc(count, sizeof(swInterfaceInfo))))
  {
    uint32_t i = 0;
    for (; i < count; i++)
    {
      uint32_t index = nameIndexPair[i].if_index - 1;
      size_t nameLength = strlen(nameIndexPair[i].if_name);
      strncpy(interfaceData.dataStorage[index].name, nameIndexPair[i].if_name, IF_NAMESIZE);
      interfaceData.dataStorage[index].nameString = swStaticStringSetWithLength(interfaceData.dataStorage[index].name, nameLength);
      interfaceData.dataStorage[index].index = index + 1;
      if (swHashMapLinearInsert(&interfaceData.dataMap, &interfaceData.dataStorage[index].nameString, &interfaceData.dataStorage[index]))
        continue;
      break;
    }
    if (i != count)
      swInterfaceInfoReleseStorage(i);
  }
  if_freenameindex(nameIndexPair);
  return count;
}

static bool swInterfaceInfoInitInternal()
{
  bool rtn = false;
  uint32_t interfaceCount = swInterfaceInfoInitStorage();
  if (interfaceCount)
  {
    swInterfaceAddress *interfaceList = NULL;
    if (getifaddrs(&interfaceList) == 0)
    {
      swInterfaceAddress *currentInterface = interfaceList;
      while (currentInterface)
      {
        if (currentInterface->ifa_addr)
        {
          bool success = false;
          swStaticString currentInterfaceName = swStaticStringDefineFromCstr(currentInterface->ifa_name);
          swInterfaceInfo *interfaceInfo = NULL;
          if (swHashMapLinearValueGet(&interfaceData.dataMap, &currentInterfaceName, (void **)&interfaceInfo))
          {
            switch (currentInterface->ifa_addr->sa_family)
            {
              case AF_INET:
                interfaceInfo->ipv4Info.address = swSocketAddressSetInet(*currentInterface->ifa_addr);
                if (currentInterface->ifa_netmask)
                  interfaceInfo->ipv4Info.mask = swSocketAddressSetInet(*currentInterface->ifa_netmask);
                if (currentInterface->ifa_broadaddr)
                  interfaceInfo->ipv4Info.broadcast = swSocketAddressSetInet(*currentInterface->ifa_broadaddr);
                interfaceInfo->ipv4Info.flags = currentInterface->ifa_flags;
                success = true;
                break;
              case AF_INET6:
                interfaceInfo->ipv6Info.address = swSocketAddressSetInet6(*currentInterface->ifa_addr);
                if (currentInterface->ifa_netmask)
                  interfaceInfo->ipv6Info.mask = swSocketAddressSetInet6(*currentInterface->ifa_netmask);
                if (currentInterface->ifa_broadaddr)
                  interfaceInfo->ipv6Info.broadcast = swSocketAddressSetInet6(*currentInterface->ifa_broadaddr);
                interfaceInfo->ipv6Info.flags = currentInterface->ifa_flags;
                success = true;
                break;
              case AF_PACKET:
                interfaceInfo->linkLayerInfo.address = swSocketAddressSetLinkLayer(*currentInterface->ifa_addr);
                if (currentInterface->ifa_data)
                  interfaceInfo->stats = *((swInterfaceAddressStats *)currentInterface->ifa_data);
                if (currentInterface->ifa_netmask)
                  interfaceInfo->linkLayerInfo.mask = swSocketAddressSetLinkLayer(*currentInterface->ifa_netmask);
                if (currentInterface->ifa_broadaddr)
                  interfaceInfo->linkLayerInfo.broadcast = swSocketAddressSetLinkLayer(*currentInterface->ifa_broadaddr);
                success = true;
                break;
              default:
                break;
            }
          }
          if (!success)
            break;
        }
        currentInterface = currentInterface->ifa_next;
      }
      if (!currentInterface)
        rtn = true;
      // call freeifaddr
      freeifaddrs(interfaceList);
    }
  }
  return rtn;
}

bool swInterfaceInfoInit()
{
  bool rtn = false;
  if (!interfaceData.inited)
  {
    // init hash table
    if (swHashMapLinearInit(&interfaceData.dataMap, (swHashKeyHashFunction)swStaticStringHash, (swHashKeyEqualFunction)swStaticStringEqual, NULL, NULL))
    {
      if ((rtn = swInterfaceInfoInitInternal()))
        interfaceData.inited = true;
      else
        swInterfaceInfoRelease();
    }
  }
  return rtn;
}

static void swInterfaceAddressInfoPrint(const char *prefix, swInterfaceAddressInfo *info)
{
  printf ("\t%s -- flags: 0x%08x:\n", prefix, info->flags);
  if (info->address.len)
  {
    swDynamicString *printString = swSocketAddressToString(&info->address);
    printf ("\t\tAdderess: %s\n", printString->data);
    swDynamicStringDelete(printString);
  }
  if (info->mask.len)
  {
    swDynamicString *printString = swSocketAddressToString(&info->mask);
    printf ("\t\tMask: %s\n", printString->data);
    swDynamicStringDelete(printString);
  }
  if (info->broadcast.len)
  {
    swDynamicString *printString = swSocketAddressToString(&info->broadcast);
    printf ("\t\tBroadcast: %s\n", printString->data);
    swDynamicStringDelete(printString);
  }
}

static void swInterfaceIAddressStatsPrint(swInterfaceAddressStats *stats)
{
  printf ("\tStats:\n");
  printf ("\t\tRX: p: %u, b: %u, e: %u (le: %u, ov: %u, cr: %u, fr: %u, fi: %u, mi: %u), d: %u, c: %u\n",
          stats->rx_packets, stats->rx_bytes, stats->rx_errors,
          stats->rx_length_errors, stats->rx_over_errors, stats->rx_crc_errors, stats->rx_frame_errors,
          stats->rx_fifo_errors, stats->rx_missed_errors, stats->rx_dropped, stats->rx_compressed);
  printf ("\t\tTX: p: %u, b: %u, e: %u (ab: %u, ca: %u, fi: %u, ha: %u, wi: %u), d: %u, c: %u\n",
          stats->tx_packets, stats->tx_bytes, stats->tx_errors,
          stats->tx_aborted_errors, stats->tx_carrier_errors, stats->tx_fifo_errors, stats->tx_heartbeat_errors,
          stats->tx_window_errors, stats->tx_dropped, stats->tx_compressed);
  printf ("\t\tMulticast: %u, Collisions: %u\n", stats->multicast, stats->collisions);
}

void swInterfaceInfoPrint()
{
  if (interfaceData.inited)
  {
    uint32_t interfaceCount = swHashMapLinearCount(&(interfaceData.dataMap));
    for (uint32_t i = 0; i < interfaceCount; i++)
    {
      swInterfaceInfo *info = &(interfaceData.dataStorage[i]);
      printf("Interface %u '%s':\n", info->index, info->name);
      swInterfaceAddressInfoPrint("AF_INET", &(info->ipv4Info));
      swInterfaceAddressInfoPrint("AF_INET6", &(info->ipv6Info));
      swInterfaceAddressInfoPrint("AF_PACKET", &(info->linkLayerInfo));
      swInterfaceIAddressStatsPrint(&(info->stats));
    }
  }
}

void swInterfaceInfoRelease()
{
  if (interfaceData.inited)
  {
    swInterfaceInfoReleseStorage(swHashMapLinearCount(&interfaceData.dataMap));
    swHashMapLinearRelease(&interfaceData.dataMap);
    memset(&interfaceData, 0, sizeof(interfaceData));
  }
}

bool swInterfaceInfoRefreshAll()
{
  bool rtn = false;
  if (interfaceData.inited)
  {
    swInterfaceInfoReleseStorage(swHashMapLinearCount(&interfaceData.dataMap));
    rtn = swInterfaceInfoInitInternal();
  }
  return rtn;
}

uint32_t swInterfaceInfoCount()
{
  uint32_t rtn = 0;
  if (interfaceData.inited)
    rtn = swHashMapLinearCount(&interfaceData.dataMap);
  return rtn;
}

bool swInterfaceInfoGetAddress(const swStaticString *name, swSocketAddress *addr)
{
  bool rtn = false;
  if (interfaceData.inited && name && addr)
  {
    swInterfaceInfo *interfaceInfo = NULL;
    if (swHashMapLinearValueGet(&interfaceData.dataMap, (void *)name, (void **)&interfaceInfo))
    {
      switch (addr->addr.sa_family)
      {
        case AF_INET:
          *addr = interfaceInfo->ipv4Info.address;
          rtn = true;
          break;
        case AF_INET6:
          *addr = interfaceInfo->ipv6Info.address;
          rtn = true;
          break;
        case AF_PACKET:
          *addr = interfaceInfo->linkLayerInfo.address;
          rtn = true;
          break;
        default:
          break;
      }
    }
  }
  return rtn;
}

bool swInterfaceInfoGetStats(const swStaticString *name, swInterfaceAddressStats *stats)
{
  bool rtn = false;
  if (interfaceData.inited && name && stats)
  {
    swInterfaceInfo *interfaceInfo = NULL;
    if (swHashMapLinearValueGet(&interfaceData.dataMap, (void *)name, (void **)&interfaceInfo))
    {
      *stats = interfaceInfo->stats;
      rtn = true;
    }
  }
  return rtn;
}

bool swInterfaceInfoGetStatsWithRefresh(const swStaticString *name, swInterfaceAddressStats *stats)
{
  bool rtn = false;
  if (swInterfaceInfoRefreshAll())
    rtn = swInterfaceInfoGetStats(name, stats);
  return rtn;
}

bool swEthernetNameToAddress(const swStaticString *interfaceName, const swEthernetAddress *ethernetAddress, swSocketAddress *address, int domain, int protocol)
{
  bool rtn = false;
  if (interfaceName && address)
  {
    if ((address->linkLayer.sll_ifindex = if_nametoindex(interfaceName->data)) > 0)
    {
      address->linkLayer.sll_family = domain;
      address->linkLayer.sll_protocol = htons (protocol);
      *((swEthernetAddress *)(address->linkLayer.sll_addr)) = *ethernetAddress;
      address->linkLayer.sll_halen = sizeof(swEthernetAddress);
      address->len = sizeof(address->linkLayer);
      rtn = true;
    }
  }
  return rtn;
}
