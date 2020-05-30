#ifndef __MY_CONF_H
#define __MY_CONF_H

//#define MY_DEBUG_USB 1

#define DEBUG_USB_IN_OUT 1

#ifdef MY_DEBUG_USB
    #define MY_DEBUGF(message)  printf message
#else
	#define MY_DEBUGF(message)
#endif
#endif
