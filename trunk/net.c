/*
 * net.c
 *
 * Network implementation
 * All network related functions are grouped here
 *
 * a Net::DNS like library for C
 *
 * (c) NLnet Labs, 2004
 *
 * See the file LICENSE for the license
 */

#include <config.h>

#include <ldns/rdata.h>
#include <ldns/error.h>
#include <ldns/resolver.h>
#include <ldns/buffer.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "util.h"


/* send off an buffer and return any reply packet
 * this is done synchronus. Send using udp
 *
 * sock must be opened, binded etc.
 */
ldns_pkt *
ldns_sendbuf_udp(ldns_buffer *buf, int *sockfd, struct sockaddr *dest)
{
	struct timeval tv_s;
	struct timeval tv_e;
	ldns_pkt * new_pkt;
	int bufsize; /* bogus decl. to make it comile */
	
	assert(buf != NULL);
	assert(*sockfd != 0);

	new_pkt = NULL;

	gettimeofday(&tv_s, NULL);

	if (sendto(*sockfd, buf, bufsize, 0, dest, 
				(socklen_t) sizeof(*dest)) != bufsize) {
		/* ai */
		return NULL;
	}

	/* there are some socket options in drill - do we need them
	 * here */
#if 0
        *reply_size = (size_t) recvfrom(sockfd, *reply, MAX_PACKET, 0, /* flags */
                        (struct sockaddr*) &src, &frmlen);
        close(sockfd);
        
        if (*reply_size == (size_t) -1) {
                return RET_FAIL;
        }
#endif
	
	gettimeofday(&tv_e, NULL);
	
	/* turn the reply into a packet? */
	
	return new_pkt;
}


/* send a buffer using tcp */
ldns_pkt *
ldns_sendbuf_tcp(ldns_buffer *buf, int *sockfd, struct sockaddr *dest)
{
	return NULL;
}

/* axfr is a hack - handle it different */
ldns_pkt *
ldns_sendbuf_axfr(ldns_buffer *buf, int *sockfd, struct sockaddr *dest)
{
	return NULL;
}

/**
 * Send to ptk to the nameserver at ipnumber. Return the data
 * as a ldns_pkt
 * \param[in] resolver to use 
 * \param[in] query to send
 * \return the pkt received from the nameserver
 */
ldns_pkt *
ldns_send(ldns_resolver *r, ldns_pkt *query_pkt)
{
	uint8_t i;
	
	struct sockaddr *ns_ip;
	socklen_t ns_ip_len;
	ldns_rdf **ns_array;
	ldns_buffer *buf;
	ldns_buffer *qb;

	ns_array = ldns_resolver_nameservers(r);
	buf = NULL;
	
	if (ldns_pkt2buffer(qb, *query_pkt) != LDNS_STATUS_OK) {
		return NULL;
	}

	/* loop through all defined nameservers */
	for (i = 0; i < ldns_resolver_nameserver_count(r); i++) {
		ns_ip = ldns_rdf2native_aaaaa(ns_array[i]);
		ns_ip_len = ldns_rdf_size(ns_array[i]);

		/* query */
		buf = ldns_send_udp(qb, ns_ip, ns_ip_len);
		
		if (!buf) {
			break;
		}
	}
	return buf;
}


/**
 */
ldns_buffer *
ldns_send_udp(ldns_buffer *qbin, const struct sockaddr *to, socklen_t tolen)
{
	int sockfd;
	ssize_t bytes;
	ldns_buffer *answer;

	if ((sockfd = socket(to->sa_family, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		return NULL;
	}

	bytes =  sendto(sockfd, ldns_buffer_begin(qbin),
			ldns_buffer_capacity(qbin), 0, to, tolen);

	if (bytes == -1) {
		close(sockfd);
		return NULL;
	}

	if ((size_t) bytes != ldns_buffer_capacity(qbin)) {
		close(sockfd);
		return NULL;
	}
	
	/* wait for an response*/
	answer = ldns_buffer_new(MAX_PACKET_SIZE);
	bytes = recv(sockfd, ldns_buffer_begin(answer),
			MAX_PACKET_SIZE, 0);
	
	close(sockfd);

	if (bytes == -1) {
		ldns_buffer_free(answer);
		return NULL;
	}
	/* resize accordingly */
	ldns_buffer_set_capacity(answer, (size_t) bytes);

	return answer;
}
