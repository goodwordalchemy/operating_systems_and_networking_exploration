#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <errno.h>

#include "ip_address.h"

int get_local_ip_address(char *buf, int buflen) {
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    struct sockaddr_in *s4;
    struct sockaddr_in6 *s6;

    if(getifaddrs(&myaddrs) != 0) {
        perror("getifaddrs");
        exit(1);
    }

    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;

        switch (ifa->ifa_addr->sa_family) {
            case AF_INET:
                s4 = (struct sockaddr_in *)ifa->ifa_addr;
                in_addr = &s4->sin_addr;
                break;

            case AF_INET6:
                s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                in_addr = &s6->sin6_addr;
                break;

            default:
                continue;
        }

        if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, buflen))
            fprintf(stderr, "%s: inet_ntop failed!\n", ifa->ifa_name);
        else {
            freeifaddrs(myaddrs);
            return 0;
        }
    }

    freeifaddrs(myaddrs);
    return 1;
}
