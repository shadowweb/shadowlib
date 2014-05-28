#ifndef SW_CORE_MEMORY_H
#define SW_CORE_MEMORY_H

#define SW_TIME_1K  1000
#define SW_TIME_1M  1000000
#define SW_TIME_1B  1000000000

#define swTimeNSecToSec(n)      (n)/SW_TIME_1B
#define swTimeUSecToSec(n)      (n)/SW_TIME_1M
#define swTimeMSecToSec(n)      (n)/SW_TIME_1K
#define swTimeNSecToMSec(n)     (n)/SW_TIME_1M
#define swTimeUSecToMSec(n)     (n)/SW_TIME_1K
#define swTimeNSecToUSec(n)     (n)/SW_TIME_1K

#define swTimeNSecToSecRem(n)   (n)%SW_TIME_1B
#define swTimeUSecToSecRem(n)   (n)%SW_TIME_1M
#define swTimeMSecToSecRem(n)   (n)%SW_TIME_1K
#define swTimeNSecToMSecRem(n)  (n)%SW_TIME_1M
#define swTimeUSecToMSecRem(n)  (n)%SW_TIME_1K
#define swTimeNSecToUSecRem(n)  (n)%SW_TIME_1K

#define swTimeSecToNSec(n)      (n)*SW_TIME_1B
#define swTimeSecToUSec(n)      (n)*SW_TIME_1M
#define swTimeSecToMSec(n)      (n)*SW_TIME_1K
#define swTimeMSecToNSec(n)     (n)*SW_TIME_1M
#define swTimeMSecToUSec(n)     (n)*SW_TIME_1K
#define swTimeUSecToNSec(n)     (n)*SW_TIME_1K

#endif // SW_CORE_MEMORY_H