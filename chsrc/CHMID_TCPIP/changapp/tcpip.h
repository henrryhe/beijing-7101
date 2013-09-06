#ifndef __TCPIP_H
#define __TCPIP_H
#include "sockets.h"
#include "dhcp.h"
#include "smsc_threading.h"
#include "CHMID_TCPIP_api.h"

/*timeout for dhcp response (secs) 120s is idea value, 30s is too little */
#ifndef DHCP_ETHER_TIMEOUT
#define DHCP_ETHER_TIMEOUT 120/*zlm 050822 change it 120 to 10*/
#endif
/*timeout for dhcp response (secs) */

#ifndef IPDRIVER_IP_ADDRESS
#define IPDRIVER_IP_ADDRESS  "0.0.0.0"
#endif  /*!IPDRIVER_IP_ADDRESS*/

#ifndef IPDRIVER_GATEWAY
#define IPDRIVER_GATEWAY     "0.0.0.0"
#endif  /*!IPDRIVER_GATEWAY*/

#ifndef IPDRIVER_NETMASK
#define IPDRIVER_NETMASK     "255.255.255.0"
#endif  /*!IPDRIVER_NETMASK*/

#ifndef IPDRIVER_MAC_ADDRESS 
#define IPDRIVER_MAC_ADDRESS 	"c0:ff:ee:73:05:30" 
#endif  /*!IPDRIVER_MAC_ADDRESS */

#ifndef IPDRIVER_DNS/*wufei add this 071206*/
#define IPDRIVER_DNS "0.0.0.0"
#endif/*!IPDRIVER_DNS*/



static void CH_NETSettingTask( void *pvParam);
static void CH_UpdateNetSetting(void);
static void CH_InitNetSettingSemaphoreAndTask(void);

#endif /* __TCPIP_H */




