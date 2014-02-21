#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <strings.h>
#include <netdb.h>


#include "coap.h"

#define PORT 5683

int main(int argc, char **argv)
{
    int fd, rc;
    struct sockaddr_in servaddr, cliaddr;

    uint8_t buf[4096];
    uint8_t scratch_raw[4096];
    coap_rw_buffer_t scratch_buf = {scratch_raw, sizeof(scratch_raw)};
    fd = socket(AF_INET,SOCK_DGRAM,0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);

    if (inet_aton("129.132.15.80", &servaddr.sin_addr)==0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    size_t sndlen = sizeof(buf);
    coap_packet_t sndpkt;
    coap_make_req_observe(&scratch_buf, &sndpkt);

    if (0 != (rc = coap_build(buf, &sndlen, &sndpkt)))
            printf("coap_build failed rc=%d\n", rc);
    else
    {
#ifdef DEBUG
        printf("Sending: ");
        coap_dump (buf, sndlen, true);
        printf("\n");
#endif
#ifdef DEBUG
        coap_dumpPacket(&sndpkt);
#endif
        sendto(fd, buf, sndlen, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    }

    while(1)
    {
        int n, rc;
        socklen_t len = sizeof(cliaddr);
        coap_packet_t pkt;

        n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &len);
#ifdef DEBUG
        printf("Received: ");
        coap_dump(buf, n, true);
        printf("\n");
#endif

        if (0 != (rc = coap_parse(&pkt, buf, n)))
            printf("Bad packet rc=%d\n", rc);
        else
        {
            size_t rsplen = sizeof(buf);
            coap_packet_t rsppkt;
#ifdef DEBUG
            coap_dumpPacket(&pkt);
#endif
            //coap_handle_req(&scratch_buf, &pkt, &rsppkt);
            coap_make_response(&scratch_buf, &rsppkt, NULL, 0, 0, 0, 0, COAP_CONTENTTYPE_NONE);

            if (0 != (rc = coap_build(buf, &rsplen, &rsppkt)))
                printf("coap_build failed rc=%d\n", rc);
            else
            {
#ifdef DEBUG
                printf("Sending: ");
                coap_dump(buf, rsplen, true);
                printf("\n");
#endif
#ifdef DEBUG
                coap_dumpPacket(&rsppkt);
#endif

                sendto(fd, buf, rsplen, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
            }
        }
    }
}
