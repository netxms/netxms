/*
$$LIC$$
 *
 *
 *   ipfix.c - IPFIX export protocol
 *
 *   Copyright Fraunhofer FOKUS
 *
 *   $Date: 2009-03-19 19:14:44 +0100 (Thu, 19 Mar 2009) $
 *
 *   $Revision: 1.27 $
 *
 *   remarks: todo - return errno/errtext
 *                 - teardown connection after some idle time (TCP)
 *                 - revise template resending (UDP)
 *                 - update SCTP, add PR-SCTP support
 *                 - support multiple collectors, buffer records per collector
 *                 - add padding bytes
 *                 - group multiple records in one set
 *
 *   netflow 9 - RFC3954
 *   http://www.ietf.org/internet-drafts/draft-ietf-ipfix-protocol-24.txt
 */
#include "libipfix.h"
#include "ipfix.h"
#include "ipfix_fields.h"
#include "ipfix_reverse_fields.h"
#ifdef SSLSUPPORT
#include "ipfix_ssl.h"
#endif

/*----- defines ----------------------------------------------------------*/

#define NODEBUG
#define IPFIX_DEFAULT_BUFLEN  1400

#define INSERTU16(b,l,val) \
        { uint16_t _t=htons((val)); memcpy((b),&_t,2); (l)+=2; }
#define INSERTU32(b,l,val) \
        { uint32_t _t=htonl((val)); memcpy((b),&_t,4); (l)+=4; }


/*----- revision id ------------------------------------------------------*/

static const char cvsid[]="$Id: ipfix.c 996 2009-03-19 18:14:44Z csc $";

/*----- globals ----------------------------------------------------------*/

typedef struct ipfixiobuf
{
    struct ipfixiobuf  *next;
    size_t             buflen;
    char               buffer[IPFIX_DEFAULT_BUFLEN+IPFIX_HDR_BYTES_NF9]; /*!!*/
} iobuf_t;

typedef struct ipfix_message
{
    char        buffer[IPFIX_DEFAULT_BUFLEN];   /* message buffer */
    int         nrecords;                       /* no. of records in buffer */
    size_t      offset;                         /* output buffer fill level */
} ipfix_message_t;

typedef struct ipfix_node
{
    struct ipfix_node   *next;
    ipfix_t             *ifh;

} ipfix_node_t;

typedef struct collector_node
{
    struct collector_node *next;
    int                   usecount;

    char            *chost;       /* collector hostname */
    int             cport;        /* collector port */
    ipfix_proto_t   protocol;     /* used protocol (e.g. tcp) */
    SOCKET          fd;           /* open socket */
    int             ssl_flag;     /* ipfix over tls/ssl */
#ifdef SSLSUPPORT
    ipfix_ssl_opts_t *ssl_opts;
    BIO             *bio;
    SSL_CTX         *ctx;
    SSL             *ssl;
#endif
    struct sockaddr *to;          /* collector address */
    socklen_t       tolen;        /* collector address length */
    time_t          lastaccess;   /* last use of this connection */
    ipfix_message_t message;      /* used only for sctp templates */

} ipfix_collector_t;

static time_t             g_tstart = 0;
static iobuf_t            g_iobuf[2], *g_buflist =NULL;
static ipfix_collector_t  *g_collectors =NULL;
static ipfix_node_t       *g_ipfixlist =NULL;
static uint16_t           g_lasttid;                  /* change this! */
static ipfix_datarecord_t g_data = { NULL, NULL, 0 }; /* ipfix_export */

static ipfix_field_t      *g_ipfix_fields;
#ifndef NOTHREADS
static LIBIPFIX_MUTEX g_mutex;
#define mod_lock()        { \
                            if (!mutex_lock(g_mutex)) \
                                mlogf( 0, "[ipfix] mutex_lock() failed: %s\n", \
                                       strerror( errno ) ); \
                          }
#define mod_unlock()      {  mutex_unlock( g_mutex ); }
#else
#define mod_lock()
#define mod_unlock()
#endif

/*----- prototypes -------------------------------------------------------*/

void _ipfix_drop_collector( ipfix_collector_t **col );
int  _ipfix_write_template( ipfix_t *ifh, ipfix_collector_t *col,
                            ipfix_template_t *templ );
int  _ipfix_send_message( ipfix_t *ifh, ipfix_collector_t *col, int flag,
                          ipfix_message_t *message );
int  _ipfix_write_msghdr( ipfix_t *ifh, ipfix_message_t *msg, iobuf_t *buf );
void _ipfix_disconnect( ipfix_collector_t *col );
int  _ipfix_export_flush( ipfix_t *ifh );


/* name      : do_writeselect
 * parameter : > int  fd     file descr
 *             > int  sec    time
 *             < int  *flag  1 <- write possible, 0 <- otherwise
 * purpose   : check output fd
 * returns   : 0 on success, -1 on error
 */
#ifdef NONBLOCKING_TCP
int do_writeselect( int fd, int sec, int *flag )
{
    fd_set         perm;
    int            fds;
    struct timeval timeout;

    FD_ZERO(&perm);
    FD_SET(fd, &perm);
    fds = fd +1;

    timeout.tv_sec  = sec;
    timeout.tv_usec = 1;

    if ( select(fds, NULL, &perm, NULL, &timeout ) <0 ) {
        *flag = 0;
        return -1;
    }

    *flag = FD_ISSET(fd, &perm);
    return 0;
}
#endif

/* Name      : do_writen
 * Parameter : > collector   collector info e.g. fd and ssl
 *             > *ptr        buffer
 *             > nbytes      number of bytes to write
 * Purpose   : write 'n' bytes to socket
 * Remarks   : replacement of write if fd is a stream socket.
 * Returns   : number of written bytes.
 */
static int do_writen( ipfix_collector_t *col, char *ptr, size_t nbytes )
{
    int     nleft, nwritten;
#ifdef DEBUG
    int     i;

    for ( i=0; i<nbytes; i++ )
        fprintf( stderr, "[%02x]", (ptr[i]&0xFF) );
    fprintf( stderr, "\n" );
#endif

    nleft = (int)nbytes;
    while (nleft > 0) {
        if ( col->ssl_flag == 0 ) {
            nwritten = send( col->fd, ptr, nleft, 0);
            if ( nwritten <= 0 )
                return ( nwritten );               /* error */
        }
#ifdef SSLSUPPORT
        else {
            nwritten = SSL_write( col->ssl, ptr, nleft );
            if ( nwritten <= 0 ) {
                /* todo: check error code */
                int err = SSL_get_error( col->ssl, nwritten );
                switch ( err ) {
                  case SSL_ERROR_SSL:
                      _ipfix_disconnect( col );
                      break;
                  default:
                      mlogf( 0, "[ipfix] ssl_write failed: %d\n", err );
                      _ipfix_disconnect( col );

                }
                return ( nwritten );               /* error */
            }
        }
#endif
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return ((int)nbytes - nleft);
}

#ifdef INCR_RXTX_BUFSIZE
/* hack, todo: change this func
 */
int _adapt_sndbuf( int sock )
{
    int       sockbufsize;
    socklen_t len = sizeof(sockbufsize);

    sockbufsize = 0;
    if ( getsockopt( sock, SOL_SOCKET, SO_SNDBUF,
                     (void*)&sockbufsize, &len )==0 ) {
        mlogf( 4, "[ipfix] default sndbuf is %dkB\n", sockbufsize/1024 );
    }
    sockbufsize = 131072;
    if ( setsockopt( sock, SOL_SOCKET, SO_SNDBUF,
                     (void*)&sockbufsize, len ) <0 ) {
        mlogf( 0, "[ipfix] setsockopt() failed: %s\n", strerror(errno) );
    }
    else {
        mlogf( 3, "[ipfix] set sndbuf to %dkB\n", sockbufsize/1024 );
        sockbufsize = 0;
        if ( getsockopt( sock, SOL_SOCKET, SO_SNDBUF,
                         (void*)&sockbufsize, &len )==0 ) {
            mlogf( 4, "[ipfix] sndbuf is now %dkB\n", sockbufsize/1024 );
        }
    }
    return 0;
}
#endif

static int _connect_nonb( SOCKET sockfd, struct sockaddr *saptr,
                          socklen_t salen, int sec)
{
    int                     n, error;
    fd_set                  rset, wset;
    struct timeval          tval;

	 SetSocketNonBlocking(sockfd);

    error = 0;
    if ( (n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0)
        if (WSAGetLastError() != WSAEINPROGRESS)
            return(-1);

    /* Do whatever we want while the connect is taking place. */

    if (n == 0)
        goto done;      /* connect completed immediately */

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = sec;
    tval.tv_usec = 0;

    if ( (n=select(SELECT_NFDS(sockfd+1), &rset, &wset, NULL, sec ? &tval : NULL)) == 0) {
        return -1;
    }

#ifndef _WIN32
    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))
	 {
		 socklen_t len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            return(-1);                     /* Solaris pending error */
    } else
        mlogf( 0, "[ipfix] select error: sockfd not set");
#endif

 done:
	 SetSocketBlocking(sockfd);

    if (error) {
        errno = error;
        return(-1);
    }
    return(0);
}

iobuf_t *_ipfix_getbuf ( void )
{
    iobuf_t *b = g_buflist;

    if ( b ) {
        g_buflist = b->next;
        b->next = NULL;
    }

    return b;
}

void _ipfix_freebuf( iobuf_t *b )
{
    if ( b ) {
        b->next = g_buflist;
        g_buflist = b;
    }
}

void _free_field_types( ipfix_field_t **flist )
{
    ipfix_field_t *tmp, *n = *flist;

    while( n ) {
        tmp = n->next;
        free( n );
        n = tmp;
    }

    *flist = NULL;
}

/*
 * descr: encode,decode,print funcs
 */
int ipfix_encode_int( void *in, void *out, size_t len )
{
    unsigned char *i = (unsigned char*) in;
    unsigned char *o = (unsigned char*) out;
    uint16_t      tmp16;
    uint32_t      tmp32;
    uint64_t      tmp64;

    switch ( len )
    {
      case 1:
          o[0] = i[0];
          break;
      case 2:
          memcpy( &tmp16, i, len );
          tmp16 = htons( tmp16 );
          memcpy( out, &tmp16, len );
          break;
      case 4:
          memcpy( &tmp32, i, len );
          tmp32 = htonl( tmp32 );
          memcpy( out, &tmp32, len );
          break;
      case 8:
          memcpy( &tmp64, i, len );
          tmp64 = htonq( tmp64 );
          memcpy( out, &tmp64, len );
          break;
      default:
          memset( out, 0xff, len );
          return -1;
    }
    return 0;
}

int ipfix_decode_int( void *in, void *out, size_t len )
{
    unsigned char *i = (unsigned char*) in;
    unsigned char *o = (unsigned char*) out;
    uint16_t      tmp16;
    uint32_t      tmp32;
    uint64_t      tmp64;

    switch ( len )
    {
      case 1:
          o[0] = i[0];
          break;
      case 2:
          memcpy( &tmp16, i, len );
          tmp16 = ntohs( tmp16 );
          memcpy( out, &tmp16, len );
          break;
      case 4:
          memcpy( &tmp32, i, len );
          tmp32 = ntohl( tmp32 );
          memcpy( out, &tmp32, len );
          break;
      case 8:
          memcpy( &tmp64, i, len );
          tmp64 = ntohq( tmp64 );
          memcpy( out, &tmp64, len );
          break;
      default:
          memset( out, 0xff, len );
          return -1;
    }
    return 0;
}

int ipfix_snprint_int( char *str, size_t size, void *data, size_t len )
{
    int8_t       tmp8;
    int16_t      tmp16;
    int32_t      tmp32;
    int64_t      tmp64;

    switch ( len ) {
      case 1:
          memcpy( &tmp8, data, len );
          return snprintf( str, size, "%d", tmp8 );
      case 2:
          memcpy( &tmp16, data, len );
          return snprintf( str, size, "%d", tmp16 );
      case 4:
          memcpy( &tmp32, data, len );
          return snprintf( str, size, "%d", (int)tmp32 );
      case 8:
          memcpy( &tmp64, data, len );
          return snprintf( str, size, INT64_FMT, (INT64)tmp64 );
      default:
          break;
    }
    return snprintf( str, size, "err" );
}

int ipfix_snprint_uint( char *str, size_t size, void *data, size_t len )
{
    uint8_t       tmp8;
    uint16_t      tmp16;
    uint32_t      tmp32;
    uint64_t      tmp64;

    switch ( len ) {
      case 1:
          memcpy( &tmp8, data, len );
          return snprintf( str, size, "%u", tmp8 );
      case 2:
          memcpy( &tmp16, data, len );
          //tmp16 = htons( tmp16 );
          return snprintf( str, size, "%u", tmp16 );
      case 4:
          memcpy( &tmp32, data, len );
          //tmp32 = htonl( tmp32 );
          return snprintf( str, size, "%u", (unsigned int)tmp32 );
      case 8:
          memcpy( &tmp64, data, len );
          //tmp64 = HTONLL( tmp64 );
          return snprintf( str, size, "%llu", (long long unsigned int)tmp64 );
      default:
          break;
    }
    return snprintf( str, size, "err" );
}

int ipfix_encode_bytes( void *in, void *out, size_t len )
{
    if ( in != out )
        memcpy( out, in, len );
    return 0;
}

int ipfix_decode_bytes( void *in, void *out, size_t len )
{
    if ( in != out )
        memcpy( out, in, len );
    return 0;
}

int ipfix_snprint_bytes( char *str, size_t size, void *data, size_t len )
{
    size_t  i, n = 0;
    uint8_t *in = (uint8_t*) data;

    if ( size < 4 )
        return snprintf( str, size, "err" );

    while ( ((len*2) + 2) > size )
        len--;

    sprintf( str, "0x" );
    n = 2;
    for( i=0; i<len; i++ ) {
        sprintf( str+n, "%02x", *in );
        n += 2;
        in++;
    }
    return (int)n;
}

int ipfix_snprint_string( char *str, size_t size, void *data, size_t len )
{
    size_t  i;
    uint8_t *in = (uint8_t*) data;

    for( i=len-1; i>=0; i-- ) {
        if ( in[i] == '\0' ) {
            return snprintf( str, size, "%s", in );
        }
    }

    if ( len < size ) {
        memcpy( str, in, len );
        str[len] = '\0';
        return (int)len;
    }

    return snprintf( str, size, "err" );
}

int ipfix_snprint_ipaddr( char *str, size_t size, void *data, size_t len )
{
    uint8_t *in = (uint8_t*)data;
    char    tmpbuf[100];

    switch ( len ) {
      case 4:
          return snprintf( str, size, "%u.%u.%u.%u",
                           in[0], in[1], in[2], in[3] );
      case 16:
      {
          /** change this!
           */
          uint16_t  i, tmp16;

          for( i=0, *tmpbuf=0; i<16; i+=2 ) {
              memcpy( &tmp16, (char*)data+i, 2 );
              tmp16 = htons( tmp16 );
              sprintf( tmpbuf+strlen(tmpbuf), "%s%x", i?":":"", tmp16 );
          }
          return snprintf( str, size, "%s", tmpbuf );
      }

      default:
          return ipfix_snprint_bytes( str, size, data, len );
    }
}

int ipfix_encode_float( void *in, void *out, size_t len )
{
    uint32_t      tmp32;
    uint64_t      tmp64;

    switch ( len ) {
      case 4:
          memcpy( &tmp32, in, len );
          tmp32 = htonl( tmp32 );
          memcpy( out, &tmp32, len );
          break;
      case 8:
          memcpy( &tmp64, in, len );
          tmp64 = htonq( tmp64 );
          memcpy( out, &tmp64, len );
          break;
      default:
          memset( out, 0xff, len );
          return -1;
    }

    return 0;
}

int ipfix_decode_float( void *in, void *out, size_t len )
{
    return ipfix_encode_float( in, out, len );
}

int ipfix_snprint_float( char *str, size_t size, void *data, size_t len )
{
    //float tmp32;
    //double tmp64;

    switch ( len ) {
      case 4:
          //ipfix_decode_float( data, &tmp32, 4);
          return snprintf( str, size, "%f", *(float*)data );
      case 8:
          //ipfix_decode_float( data, &tmp64, 8);
          return snprintf( str, size, "%lf", *(double*)data);
      default:
          break;
    }

    return snprintf( str, size, "err" );
}

/* name:       ipfix_free_unknown_ftinfo()
 */
void ipfix_free_unknown_ftinfo( ipfix_field_t *f )
{
    if ( f ) {
        if ( f->ft ) {
            if ( f->ft->name )
                free( f->ft->name );
            if ( f->ft->documentation )
                free( f->ft->documentation );
            free( f->ft );
        }
        free( f );
    }
}

/* name:       ipfix_create_unknown_ftinfo()
 * parameters: eno, ftype
 * return:     ftinfo from global list or NULL
 */
ipfix_field_t *ipfix_create_unknown_ftinfo( int eno, int type )
{
    ipfix_field_t      *f;
    ipfix_field_type_t *ft;
    char               tmpbuf[50];

    if ( (f=calloc(1, sizeof(ipfix_field_t))) ==NULL ) {
        return NULL;
    }
    if ( (ft=calloc(1, sizeof(ipfix_field_type_t))) ==NULL ) {
        free( f );
        return NULL;
    }

    sprintf( tmpbuf, "%u_%u", eno, type );
    ft->name = strdup( tmpbuf );
    ft->documentation = strdup( tmpbuf );
    ft->eno    = eno;
    ft->ftype  = type;
    ft->coding = IPFIX_CODING_BYTES;

    f->next    = NULL;
    f->ft      = ft;
    f->encode  = ipfix_encode_bytes;
    f->decode  = ipfix_decode_bytes;
    f->snprint = ipfix_snprint_bytes;

    return f;
}

/* name:       ipfix_get_ftinfo()
 * parameters: eno, ftype
 * return:     ftinfo from global list or NULL
 */
ipfix_field_t *ipfix_get_ftinfo( int eno, int type )
{
    ipfix_field_t *elems = g_ipfix_fields;

    while( elems ) {
        if( (elems->ft->ftype == type) && (elems->ft->eno==eno) )
            return elems;

        elems = elems->next;
    }

    return NULL;
}

int ipfix_get_eno_ieid( char *field, int *eno, int *ieid )
{
    ipfix_field_t *elems = g_ipfix_fields;

    while( elems ) {
        if( stricmp( field, elems->ft->name) ==0) {
            *eno  = elems->ft->eno;
            *ieid = elems->ft->ftype;
            return 0;
        }
        elems = elems->next;
    }

    return -1;
}

/*
 * name:        ipfix_init()
 * parameters:
 * remarks:     init module, read field type info.
 */
int ipfix_init( void )
{
    if ( g_tstart ) {
        ipfix_cleanup();
    }

#ifndef NOTHREADS
	 g_mutex = mutex_create();
#endif
    g_tstart = time(NULL);
#ifndef _WIN32
    signal( SIGPIPE, SIG_IGN );
#endif
    g_lasttid = 255;

    /** alloc iobufs, todo!
     ** for ( i=0; i< niobufs; i++ ) ....
     */
    g_buflist = &(g_iobuf[0]);
    g_iobuf[0].next = &(g_iobuf[1]);
    g_iobuf[1].next = NULL;

    /** init list of field types
     ** - from field_types.h
     ** - in future from config files
     */
	if ( ipfix_add_vendor_information_elements(ipfix_field_types) <0 )
	{
		return -1;
	}

	if ( ipfix_add_vendor_information_elements(ipfix_reverse_field_types) <0 )
	{
		return -1;
	}

	return 0;
}

/*
 * name:        ipfix_add_vendor_information_elements()
 * parameters:  > fields - array of fields of size nfields+1
 *                         the last field has ftype = 0
 * description: add information elements to global list of field types
 * remarks:
 */
int ipfix_add_vendor_information_elements( ipfix_field_type_t *fields )
{
    ipfix_field_type_t *ft;
    ipfix_field_t      *n;

    if ( ! g_tstart ) {          /* hack: change this! */
        if ( ipfix_init() < 0 )
            return -1;
    }

    /** add to list of field types
     */
    ft = fields;
    while ( ft->ftype !=0 ) {
        /** create new node
         */
        if ((n=calloc( 1, sizeof(ipfix_field_t))) ==NULL )
            goto err;
        n->ft = ft;
        if ( ft->coding == IPFIX_CODING_INT ) {
            n->encode = ipfix_encode_int;
            n->decode = ipfix_decode_int;
            n->snprint= ipfix_snprint_int;
        }
        else if ( ft->coding == IPFIX_CODING_UINT ) {
            n->encode = ipfix_encode_int;
            n->decode = ipfix_decode_int;
            n->snprint= ipfix_snprint_uint;
        }
        else if ( ft->coding == IPFIX_CODING_NTP ) {
            n->encode = ipfix_encode_int;
            n->decode = ipfix_decode_int;
            n->snprint= ipfix_snprint_uint;
        }
        else if ( ft->coding == IPFIX_CODING_FLOAT ) {
            n->encode = ipfix_encode_float;
            n->decode = ipfix_decode_float;
            n->snprint= ipfix_snprint_float;
        }
        else if ( ft->coding == IPFIX_CODING_IPADDR ) {
            n->encode = ipfix_encode_bytes;
            n->decode = ipfix_decode_bytes;
            n->snprint= ipfix_snprint_ipaddr;
        }
        else if ( ft->coding == IPFIX_CODING_STRING ) {
            n->encode = ipfix_encode_bytes;
            n->decode = ipfix_decode_bytes;
            n->snprint= ipfix_snprint_string;
        }
        else {
            n->encode = ipfix_encode_bytes;
            n->decode = ipfix_decode_bytes;
            n->snprint= ipfix_snprint_bytes;
        }

        /** insert node
         */
        if ( g_ipfix_fields ) {
            n->next = g_ipfix_fields;
            g_ipfix_fields = n;
        }
        else {
            n->next = NULL;
            g_ipfix_fields = n;
        }
        ft++;
        continue;

    err:
        /** todo: do not free the whole list */
        _free_field_types( &g_ipfix_fields );
        return -1;
    }

    return 0;
}

void ipfix_cleanup ( void )
{
    while ( g_ipfixlist ) {
        ipfix_close( g_ipfixlist->ifh );
    }
    _free_field_types( &g_ipfix_fields );
    g_tstart = 0;
    if ( g_data.lens ) free( g_data.lens );
    if ( g_data.addrs ) free( g_data.addrs );
    g_data.maxfields = 0;
    g_data.lens  = NULL;
    g_data.addrs = NULL;
#ifndef NOTHREADS
    mutex_destroy( g_mutex );
#endif
}

int _ipfix_connect ( ipfix_collector_t *col )
{
    char   *server = col->chost;
    int    port    = col->cport;
    int    socktype, sockproto;
    SOCKET    sock = -1;
#ifdef INET6
    struct addrinfo  *res, *aip;
    struct addrinfo  hints;
    char   portstr[30];
    int    error;
#else
    struct sockaddr_in  serv_addr;
    struct hostent      *h;
#endif

    switch( col->protocol ) {
      case IPFIX_PROTO_TCP:
          socktype = SOCK_STREAM;
          sockproto= 0;
          break;
      case IPFIX_PROTO_UDP:
          socktype = SOCK_DGRAM;
          sockproto= 0;
          break;
#ifdef SCTPSUPPORT
      case IPFIX_PROTO_SCTP:
          socktype = SOCK_SEQPACKET;
          sockproto= IPPROTO_SCTP;
          break;
#endif
      default:
//          errno = ENOTSUP;
          col->fd = -1;
          return -1;
    }

    if ( col->fd >= 0 )
        return 0;

#ifdef INET6
    /** Get host address. Any type of address will do.
     */
    memset( &hints, 0, sizeof (hints));
    hints.ai_socktype = socktype;
    hints.ai_family   = PF_UNSPEC;
    sprintf( portstr, "%d", port );
#ifdef SCTPSUPPORT
    if ( col->protocol==IPFIX_PROTO_SCTP ) {       /* work around bug in linux libc (1) */
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;
    }
#endif
    error = getaddrinfo(server, portstr, &hints, &res);
    if (error != 0) {
        mlogf( 0, "[ipfix] getaddrinfo( %s, %s ) failed: %s\n",
               server, portstr, gai_strerror(error) );
        return -1;
    }

    for (aip = res; aip != NULL; aip = aip->ai_next) {
        /** Open socket. The address type depends on what
         ** getaddrinfo() gave us.
         */
#ifdef SCTPSUPPORT
        if ( col->protocol==IPFIX_PROTO_SCTP ) {   /* work around bug in linux libc (2) */
            aip->ai_socktype = socktype;
            aip->ai_protocol = sockproto;
        }
#endif
        sock = socket(aip->ai_family, aip->ai_socktype, aip->ai_protocol);
        if (sock == INVALID_SOCKET) 
        {
            mlogf( 0, "[ipfix] socket() failed: %s\n", strerror(errno) );
            freeaddrinfo(res);
            return (-1);
        }
#ifdef INCR_RXTX_BUFSIZE
        (void)_adapt_sndbuf( sock );
#endif
        if ( col->protocol==IPFIX_PROTO_TCP ) {
            /* connect to the host */
            if ( _connect_nonb( sock, aip->ai_addr, aip->ai_addrlen, 2) <0) {
                mlogf( 2, "[ipfix] %scannot connect to %s: %s\n",
                       (aip->ai_family==AF_INET6)?"IPv6 "
                       :(aip->ai_family==AF_INET)?"IPv4 ":"",
                       server, strerror(errno) );
                closesocket(sock);
                sock = -1;
                continue;
            }
#ifdef DEBUG
            else {
                mlogf( 2, "[ipfix] %sconnected to %s\n",
                       (aip->ai_family==AF_INET6)?"IPv6 "
                       :(aip->ai_family==AF_INET)?"IPv4 ":"", server );
            }
#endif
        }
        else if ( col->protocol==IPFIX_PROTO_UDP ) {
            /** remember address
             */
            if ( (col->to=calloc( 1, aip->ai_addrlen )) ==NULL) {
                closesocket(sock);
                freeaddrinfo(res);
                return (-1);
            }
            memcpy( col->to, aip->ai_addr, aip->ai_addrlen );
            col->tolen = aip->ai_addrlen;
        }
#ifdef SCTPSUPPORT
        else if ( col->protocol==IPFIX_PROTO_SCTP ) {
            struct sctp_event_subscribe events;

            /** remember address */
            if ( (col->to=calloc( 1, aip->ai_addrlen )) ==NULL) {
                closesocket(sock);
                freeaddrinfo(res);
                return (-1);
            }
            memcpy( col->to, aip->ai_addr, aip->ai_addrlen );
            col->tolen = aip->ai_addrlen;

            /** set sockopts */
            memset( &events, 0, sizeof(events) );
            events.sctp_data_io_event = 1;
            if ( setsockopt( sock, IPPROTO_SCTP, SCTP_EVENTS,
                             &events, sizeof(events) ) <0 ) {
                mlogf( 0, "[ipfix] setsockopt() failed: %s\n",
                       strerror(errno) );
                closesocket(sock);
                freeaddrinfo(res);
                return -1;
            }
        }
#endif
        break;
    }
    freeaddrinfo(res);
#else
    /** get address
     */
    if ( (h=gethostbyname(server)) ==NULL) {
        mlogf( 0, "[ipfix] cannot get address of host '%s': %s\n",
               server, hstrerror(h_errno) );
        errno = EINVAL;
        return -1;
    }
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    memcpy(&(serv_addr.sin_addr), h->h_addr, sizeof(struct in_addr));
    serv_addr.sin_family  = AF_INET;
    serv_addr.sin_port    = htons(port);

    /** open socket
     */
    if ( (sock=socket( AF_INET, socktype, sockproto)) <0 ) {
        mlogf( 0, "[ipfix] socket() failed: %s\n", strerror(errno) );
        return -1;
    }
#ifdef INCR_RXTX_BUFSIZE
    (void)_adapt_sndbuf( sock );
#endif
    if ( col->protocol==IPFIX_PROTO_TCP ) {
        /** Connect to the server.
         */
        if ( _connect_nonb( sock, (struct sockaddr *)&serv_addr,
                            sizeof(serv_addr), 2 /*s*/ ) < 0) {
            closesocket( sock );
            sock = -1;
        }
    }
    else if ( col->protocol==IPFIX_PROTO_UDP ) {
        /** remember address
         */
        if ( (col->to=calloc( 1, sizeof(serv_addr) )) ==NULL) {
            closesocket(sock);
            return -1;
        }
        memcpy( col->to, &serv_addr, sizeof(serv_addr) );
        col->tolen = sizeof(serv_addr);
    }
#ifdef SCTPSUPPORT
    else if ( col->protocol==IPFIX_PROTO_SCTP ) {
        struct sctp_event_subscribe events;

        /** remember address */
        if ( (col->to=calloc( 1, sizeof(serv_addr) )) ==NULL) {
            closesocket(sock);
            return -1;
        }
        memcpy( col->to, &serv_addr, sizeof(serv_addr) );
        col->tolen = sizeof(serv_addr);

        /** set sockopts */
        memset( &events, 0, sizeof(events) );
        events.sctp_data_io_event = 1;
        if ( setsockopt( sock, IPPROTO_SCTP, SCTP_EVENTS,
                         &events, sizeof(events) ) <0 ) {
            mlogf( 0, "[ipfix] setsockopt() failed: %s\n", strerror(errno) );
            closesocket(sock);
            return -1;
        }
    }
#endif
#endif

    if (sock <0 ) {
        mlogf( 1, "[ipfix] cannot connect to %s: %s\n",
               server, strerror(errno) );
        return (-1);
    }
#ifdef SSLSUPPORT
    else if ( col->ssl_flag ) {
        int err;

        if ( col->ctx == NULL ) {
            /* setup SSL_CTX object
             */
            if ( ipfix_ssl_setup_client_ctx( &col->ctx,
                                             col->protocol==IPFIX_PROTO_UDP?
                                             DTLSv1_client_method():
                                             SSLv23_method(),
                                             col->ssl_opts ) <0 ) {
                goto err;
            }
        }

        switch( col->protocol ) {
          case IPFIX_PROTO_TCP:
              if ( (col->bio=BIO_new_socket( sock, BIO_NOCLOSE )) ==NULL ) {
                  mlogf( 0, "[ipfix] BIO_new_socket() failed: %s\n",
                         strerror(errno) );
                  goto err;
              }
              col->ssl = SSL_new( col->ctx );
              SSL_set_bio( col->ssl, col->bio, col->bio );
              if ( SSL_connect( col->ssl ) <= 0 ) {
                  mlogf( 0, "[ipfix] SSL_connect failed: %s\n",
                         strerror(errno) );
                  goto err;
              }
              if ((err=ipfix_ssl_post_connection_check( col->ssl, server ))
                  != X509_V_OK) {
                  mlogf( 0, "[ipfix] error: peer certificate: %s\n",
                         X509_verify_cert_error_string(err));
                  mlogf( 0, "[ipfix] error checking ssl peer" );
              }
              mlogf( 1, "[ipfix] ssl connection opened\n" );
              break;

          case IPFIX_PROTO_UDP:
          case IPFIX_PROTO_SCTP:
          default:
              mlogf( 0, "[ipfix] internal error: protocol not supported\n" );
              closesocket( sock );
              goto err;
        }
    }
#endif

    col->fd = sock;
    col->lastaccess = time(NULL);

    /* check if there are any templates to (re)send
     */
    {
        ipfix_node_t      *node;
        ipfix_collector_t *cnode;
        ipfix_template_t  *tnode;

        for( node=g_ipfixlist; node!=NULL; node=node->next ) {
            for( cnode=(ipfix_collector_t*)node->ifh->collectors;
                 cnode!=NULL; cnode=cnode->next ) {
                if ( col == cnode ) {
                    for( tnode=node->ifh->templates;
                         tnode!=NULL; tnode=tnode->next ) {
                        switch( col->protocol ) {
                          case IPFIX_PROTO_SCTP:
                          case IPFIX_PROTO_TCP:
                              if (_ipfix_write_template( node->ifh, col,
                                                         tnode ) <0 )
                                  return -1;
                              break;
                          case IPFIX_PROTO_UDP:
                              tnode->tsend = 0;
                              break;
                        }
                    }
                }
            }

            if ( col->message.offset ) {
                if ( _ipfix_send_message( node->ifh, col,
                                          (col->protocol==IPFIX_PROTO_SCTP)?1:0,
                                          &col->message ) < 0 )
                    return -1;
            }
        }/*for*/
    }
    return 0;

#ifdef SSLSUPPORT
 err:
    if ( col && col->ctx ) {
        SSL_CTX_free( col->ctx );
        col->ctx = NULL;
        if ( col->ssl )
            SSL_free( col->ssl );
        else if ( col->bio )
            BIO_free(col->bio);
        col->ssl = NULL;
        col->bio = NULL;
    }
    return -1;
#endif
}

void _ipfix_disconnect( ipfix_collector_t *col )
{
#ifdef SSLSUPPORT
    if ( col->ssl_flag ) {
        if ( col->ssl )
            SSL_free(col->ssl);
        else if ( col->bio )
            BIO_free(col->bio);
        col->bio = NULL;
        col->ssl = NULL;
    }
#endif

    if ( ((col->protocol==IPFIX_PROTO_UDP) && col->to )
         || ((col->protocol==IPFIX_PROTO_SCTP) && col->to )) {
        free( col->to );
        col->to=NULL;
    }

    if ( col->fd >= 0 ) {
        closesocket( col->fd );
        col->fd = -1;
    }
}

#ifdef SCTPSUPPORT
int _ipfix_send_msg_sctp( ipfix_t *ifh, ipfix_collector_t *col,
                          iobuf_t *buf, int stream )
{
    if ( sctp_sendmsg( col->fd, buf->buffer, buf->buflen,
                       col->to, col->tolen, 0, 0 /*flags*/,
                       stream, 0, 0 ) != buf->buflen ) {
        mlogf( 0, "[ipfix] sctp_sendmsg() failed: %s\n", strerror(errno) );
        return -1;
    }

    return 0;
}
#endif

int _ipfix_send_message( ipfix_t *ifh, ipfix_collector_t *col, int flag,
                         ipfix_message_t *message )
{
    iobuf_t *buf;

    if ( col->fd <0 )
        return -1;

    if ( (buf=_ipfix_getbuf()) ==NULL )
        return -1;

    _ipfix_write_msghdr( ifh, message, buf );
    memcpy( buf->buffer+buf->buflen, message->buffer, message->offset );
    buf->buflen += message->offset;

    switch( col->protocol ) {
      case IPFIX_PROTO_TCP:
          /* send ipfix message */
          if ( do_writen( col, buf->buffer, buf->buflen ) != (int)buf->buflen ) {
              if ((WSAGetLastError() == WSAECONNRESET)
#ifndef _WIN32
					   || (errno == EPIPE)
#endif
					   ) 
				  {
                  /* todo: check this */
                  _ipfix_disconnect( col );
              }
              goto errend;
          }
          break;

      case IPFIX_PROTO_UDP:
      {
          ssize_t n=0, nleft= (ssize_t)buf->buflen;
          uint8_t *p = (uint8_t*)(buf->buffer);

          while( nleft>0 ) {
              if ( col->ssl_flag ==0 ) {
                  n=sendto( col->fd, p, nleft, 0, col->to, col->tolen );
                  if ( n<=0 )
                      goto errend;
              }
#ifdef SSLSUPPORT
              else {
                  n = do_writen( col, (char*)p, nleft );
                  if ( n<=0 )
                      goto errend;
              }
#endif
              nleft -= n;
              p     += n;
          }
          break;
      }
#ifdef SCTPSUPPORT
      case IPFIX_PROTO_SCTP:
          if ( _ipfix_send_msg_sctp( ifh, col, buf, flag?1:0/*stream*/ ) <0 ) {
              if ( (errno == EPIPE) || (errno==ECONNRESET) ) {
                  /* todo: check this */
                  _ipfix_disconnect( col );
              }
              goto errend;
          }
          break;
#endif
      default:
          goto errend;
    }

    message->nrecords = 0;
    message->offset = 0;
    col->lastaccess = time(NULL);
    _ipfix_freebuf( buf );
    return 0;

 errend:
    _ipfix_freebuf( buf );
    return -1;
}

int _ipfix_send_msg( ipfix_t *ifh, ipfix_collector_t *col, iobuf_t *buf )
{
    int i, retval =-1;

    if ( _ipfix_connect( col ) <0 )
        return -1;

    /* are there any templates to send?
     */
    if ( col->message.offset ) {
        if ( _ipfix_send_message( ifh, col, 0, &col->message ) < 0 ) {
            mlogf( 1, "[ipfix] send tempates failed: %s\n", strerror(errno) );
            return -1;
        }
    }

    switch( col->protocol )
    {
      case IPFIX_PROTO_TCP:
          for( i=0; i<2; i++ ) {
#ifdef NONBLOCKING_TCP
              int flag =0;

              if ( col->ssl_flag == 0 ) {
                  if ( do_writeselect( col->fd, 0, &flag ) <0 ) {
                      goto reconnect;
                  }

                  if ( flag==0 ) {
                      /* write would block */
                      mlogf( 4, "[ipfix] output buf full: cannot send msg!\n");
                      errno = EAGAIN;
                      return -1;
                  }
              }
#endif
              /* send ipfix message */
              if ( do_writen( col, buf->buffer, buf->buflen )
                   != (int)buf->buflen ) {
                  if ( (WSAGetLastError() == WSAECONNRESET) 
#ifndef _WIN32
							 || (errno == EPIPE)
#endif
						   )
                      goto reconnect;
                  else
                      return -1;
              }

              retval =0;
              break;

          reconnect:
              mlogf( 2, "[ipfix] connection lost, reconnect.\n" );
              _ipfix_disconnect( col );
              if ( _ipfix_connect( col ) <0 )
                  return -1;
          }
          break;
      case IPFIX_PROTO_UDP:
      {
          ssize_t n=0, nleft= (ssize_t)buf->buflen;
          uint8_t *p = (uint8_t*)(buf->buffer);

          while( nleft>0 ) {
              if ( col->ssl_flag ==0 ) {
                  n=sendto( col->fd, p, nleft, 0, col->to, col->tolen );
                  if ( n<=0 )
                      return -1;
              }
#ifdef SSLSUPPORT
              else {
                  n=do_writen( col, (char*)p, nleft );
                  if ( n<=0 )
                      return -1;
              }
#endif
              nleft -= n;
              p     += n;
          }
          retval =0;
          break;
      }
#ifdef SCTPSUPPORT
      case IPFIX_PROTO_SCTP:
          retval = _ipfix_send_msg_sctp( ifh, col, buf, 1/*stream*/ );
          break;
#endif
      default:
          return -1;
    }

    col->lastaccess = time(NULL);
    return retval;
}

/*
 * name:        _ipfix_write_hdr()
 * parameters:
 * return:      0/-1
 */
int _ipfix_write_hdr( ipfix_t *ifh, iobuf_t *buf )
{
    time_t      now = time(NULL);

    /** fill ipfix header
     */
    if ( ifh->version == IPFIX_VERSION_NF9 ) {
        buf->buflen = 0;
        ifh->seqno++;
        INSERTU16( buf->buffer+buf->buflen, buf->buflen, ifh->version );
        INSERTU16( buf->buffer+buf->buflen, buf->buflen, ifh->nrecords );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, (uint32_t)((now-g_tstart)*1000));
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, (uint32_t)now );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, ifh->seqno );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, ifh->sourceid );
    }
    else {
        buf->buflen = 0;
        INSERTU16( buf->buffer+buf->buflen, buf->buflen, ifh->version );
        INSERTU16( buf->buffer+buf->buflen, buf->buflen, (uint16_t)(ifh->offset + IPFIX_HDR_BYTES));
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, (uint32_t)now );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, ifh->seqno );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, ifh->sourceid );
    }

    return 0;
}

/* name:        _ipfix_write_msghdr()
 * parameters:
 * return:      0/-1
 */
int _ipfix_write_msghdr( ipfix_t *ifh, ipfix_message_t *msg, iobuf_t *buf )
{
    time_t      now = time(NULL);

    /** fill ipfix header
     */
    if ( ifh->version == IPFIX_VERSION_NF9 ) {
        ifh->seqno++;
        buf->buflen = 0;
        INSERTU16( buf->buffer+buf->buflen, buf->buflen, ifh->version );
        INSERTU16( buf->buffer+buf->buflen, buf->buflen, msg->nrecords );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, (uint32_t)((now-g_tstart)*1000));
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, (uint32_t)now );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, ifh->seqno );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, ifh->sourceid );
    }
    else {
        buf->buflen = 0;
        INSERTU16( buf->buffer+buf->buflen, buf->buflen, ifh->version );
        INSERTU16( buf->buffer+buf->buflen, buf->buflen, (uint16_t)(msg->offset + IPFIX_HDR_BYTES));
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, (uint32_t)now );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, ifh->seqno );
        INSERTU32( buf->buffer+buf->buflen, buf->buflen, ifh->sourceid );
    }

    return 0;
}

/*
 * name:        _ipfix_write_template()
 * parameters:
 * return:      0/-1
 */
int _ipfix_write_template( ipfix_t           *ifh,
                           ipfix_collector_t *col,
                           ipfix_template_t  *templ )
{
    size_t            buflen, tsize=0, ssize=0, osize=0;
    char              *buf;
    uint16_t          tmp16;
    int               i, n;

    /** calc template size
     */
    if ( templ->type == OPTION_TEMPLATE ) {
        for ( i=0, ssize=0; i<templ->nscopefields; i++ ) {
            ssize += 4;
            if (templ->fields[i].elem->ft->eno != IPFIX_FT_NOENO)
                ssize += 4;
        }
        for ( osize=0; i<templ->nfields; i++ ) {
            osize += 4;
            if (templ->fields[i].elem->ft->eno != IPFIX_FT_NOENO)
                osize += 4;
        }
        tsize = 10 + osize + ssize;
    } else {
        for ( tsize=8,i=0; i<templ->nfields; i++ ) {
            tsize += 4;
            if (templ->fields[i].elem->ft->eno != IPFIX_FT_NOENO)
                tsize += 4;
        }
    }

    switch ( col->protocol ) {
      case IPFIX_PROTO_SCTP:
          /* hack */
          buf = col->message.buffer;
          buflen = col->message.offset;
          break;

      case IPFIX_PROTO_TCP:
          for ( ;; ) {
              buf    = col->message.buffer;
              buflen = col->message.offset;
              /* check space */
              if ( buflen + tsize > IPFIX_DEFAULT_BUFLEN ) {
                  if ( _ipfix_send_message( ifh, col, 0, &col->message ) < 0 )
                      return -1;
                  else {
                      col->message.offset =0;
                      col->message.nrecords =0;
                      continue;
                  }
              }
              if ( tsize+ifh->offset > IPFIX_DEFAULT_BUFLEN )
                  return -1;
              break;
          }
          break;

      default:
          /* check space */
          if ( tsize+ifh->offset > IPFIX_DEFAULT_BUFLEN ) {
              if ( _ipfix_export_flush( ifh ) < 0 )
                  return -1;
              if ( tsize+ifh->offset > IPFIX_DEFAULT_BUFLEN )
                  return -1;
          }

          /* write template prior to data */
          if ( ifh->offset > 0 ) {
              memmove( ifh->buffer + tsize, ifh->buffer, ifh->offset );
          	  if ( ifh->cs_tid )
                  ifh->cs_header += tsize;          
          }

          buf = ifh->buffer;
          buflen = 0;
          break;
    }

    /** insert template set into buffer
     */
    if ( ifh->version == IPFIX_VERSION_NF9 ) {
        if ( templ->type == OPTION_TEMPLATE ) {
            INSERTU16( buf+buflen, buflen, IPFIX_SETID_OPTTEMPLATE_NF9);
            INSERTU16( buf+buflen, buflen, (uint16_t)tsize );
            INSERTU16( buf+buflen, buflen, templ->tid );
            INSERTU16( buf+buflen, buflen, (uint16_t)ssize );
            INSERTU16( buf+buflen, buflen, (uint16_t)osize );
        } else {
            INSERTU16( buf+buflen, buflen, IPFIX_SETID_TEMPLATE_NF9);
            INSERTU16( buf+buflen, buflen, (uint16_t)tsize );
            INSERTU16( buf+buflen, buflen, templ->tid );
            INSERTU16( buf+buflen, buflen, templ->nfields );
        }
    } else {
        if ( templ->type == OPTION_TEMPLATE ) {
            INSERTU16( buf+buflen, buflen, IPFIX_SETID_OPTTEMPLATE );
            INSERTU16( buf+buflen, buflen, (uint16_t)tsize );
            INSERTU16( buf+buflen, buflen, templ->tid );
            INSERTU16( buf+buflen, buflen, templ->nfields );
            INSERTU16( buf+buflen, buflen, templ->nscopefields );
        } else {
            INSERTU16( buf+buflen, buflen, IPFIX_SETID_TEMPLATE);
            INSERTU16( buf+buflen, buflen, (uint16_t)tsize );
            INSERTU16( buf+buflen, buflen, templ->tid );
            INSERTU16( buf+buflen, buflen, templ->nfields );
        }
    }

    if ( templ->type == OPTION_TEMPLATE ) {
        n = templ->nfields;
        for ( i=0; i<templ->nscopefields; i++ ) {
            if ( templ->fields[i].elem->ft->eno == IPFIX_FT_NOENO ) {
                INSERTU16( buf+buflen, buflen, templ->fields[i].elem->ft->ftype );
                INSERTU16( buf+buflen, buflen, templ->fields[i].flength );
            } else {
                tmp16 = templ->fields[i].elem->ft->ftype|IPFIX_EFT_VENDOR_BIT;
                INSERTU16( buf+buflen, buflen, tmp16 );
                INSERTU16( buf+buflen, buflen, templ->fields[i].flength );
                INSERTU32( buf+buflen, buflen, templ->fields[i].elem->ft->eno );
            }
        }
    } else {
        i = 0;
        n = templ->nfields;
    }

    for ( ; i<templ->nfields; i++ )
    {
        if ( templ->fields[i].elem->ft->eno == IPFIX_FT_NOENO ) {
            INSERTU16( buf+buflen, buflen, templ->fields[i].elem->ft->ftype );
            INSERTU16( buf+buflen, buflen, templ->fields[i].flength );
        } else {
            tmp16 = templ->fields[i].elem->ft->ftype|IPFIX_EFT_VENDOR_BIT;
            INSERTU16( buf+buflen, buflen, tmp16 );
            INSERTU16( buf+buflen, buflen, templ->fields[i].flength );
            INSERTU32( buf+buflen, buflen, templ->fields[i].elem->ft->eno );
        }
    }
    templ->tsend = time(NULL);

    switch ( col->protocol ) {
#ifdef SCTPSUPPORT
      case IPFIX_PROTO_SCTP:
          col->message.offset = buflen;
          if ( ifh->version == IPFIX_VERSION_NF9 )
              col->message.nrecords++;
          break;
#endif
      case IPFIX_PROTO_TCP:
          col->message.offset = buflen;
          if ( ifh->version == IPFIX_VERSION_NF9 )
              col->message.nrecords++;
          break;
      default:
          ifh->offset += buflen;
          if ( ifh->version == IPFIX_VERSION_NF9 )
              ifh->nrecords++;
          break;
    }

    return 0;
}

/*
 * name:        ipfix_open()
 * parameters:
 * return:      0 = ok, -1 = error
 */
int ipfix_open( ipfix_t **ipfixh, int sourceid, int ipfix_version )
{
    ipfix_t       *i;
    ipfix_node_t  *node;

    if ( ! g_tstart )          /* hack: change this! */
    {
        /** module initialisation
         */
        if ( ipfix_init() < 0 )
            return -1;
    }

    switch( ipfix_version ) {
      case IPFIX_VERSION_NF9:
          break;
      case IPFIX_VERSION:
          break;
      default:
//          errno = ENOTSUP;
          return -1;
    }

    if ( (i=calloc( 1, sizeof(ipfix_t) )) ==NULL )
        return -1;

    if ( (i->buffer=calloc( 1, IPFIX_DEFAULT_BUFLEN )) ==NULL ) {
        free( i );
        return -1;
    }

    i->sourceid  = sourceid;
    i->offset    = 0;
    i->version   = ipfix_version;
    i->seqno     = 0;

    /** store handle in global list
     */
    if ( (node=calloc( 1, sizeof(ipfix_node_t))) ==NULL) {
        free(i->buffer);
        free(i);
        return -1;
    }
    node->ifh   = i;

    mod_lock();
    node->next  = g_ipfixlist;
    g_ipfixlist = node;
    mod_unlock();

    *ipfixh = i;
    return 0;
}

/*
 * name:        ipfix_close()
 * parameters:
 * return:      0 = ok, -1 = error
 */
void ipfix_close( ipfix_t *h )
{
    if ( h )
    {
        ipfix_node_t *l, *n;

        mod_lock();
        _ipfix_export_flush( h );

        while( h->collectors )
            _ipfix_drop_collector( (ipfix_collector_t**)&h->collectors );

        /** remove handle from global list
         */
        for( l=g_ipfixlist, n=l; n!=NULL; n=n->next ) {
            if ( g_ipfixlist->ifh == h ) {
                g_ipfixlist = g_ipfixlist->next;
                break;
            }
            if ( n->ifh == h ) {
                l->next = n->next;
                break;
            }
            l = n;
        }
        if ( n )
            free( n );
#ifdef DEBUG
        else
            fprintf( stderr, "INTERNAL ERROR: ipfix node not found!\n" );
#endif
        free(h->buffer);
        free(h);
        mod_unlock();
    }
}


/*
 * name:        _ipfix_drop_collector()
 * parameters:
 */
void _drop_collector( ipfix_collector_t **list,
                      ipfix_collector_t *node )
{
    if ( !list || !(*list) || !node )
        return;

    if ( *list == node ) {
        *list = (*list)->next;
    }
    else {
        ipfix_collector_t *last = *list;
        ipfix_collector_t *n = (*list)->next;

        while( n ) {
            if ( n == node ) {
                last->next = n->next;
                break;
            }
            last = n;
            n = n->next;
        }
    }
}

void _ipfix_drop_collector( ipfix_collector_t **col )
{
    if ( *col==NULL )
        return;

    (*col)->usecount--;
    if ( (*col)->usecount==0 )
    {
        ipfix_collector_t *node = *col;

        _ipfix_disconnect( *col );
        _drop_collector( &g_collectors, *col );
        *col = NULL;  /* todo! */
#ifdef SSLSUPPORT
        if ( node->ssl_flag ) {
            ipfix_ssl_opts_free( node->ssl_opts );
            if ( node->ctx ) SSL_CTX_free( node->ctx );
            if ( node->ssl ) SSL_free( node->ssl );
        }
#endif
        free( node->chost );
        free( node );
    }
    else {
        *col = NULL;  /* todo! */
    }
    return;
}

/* name:        _ipfix_add_collector()
 * parameters:
 * return:      0 = ok, -1 = error
 */
static int _ipfix_add_collector( ipfix_t *ifh, char *host, int port,
                                 ipfix_proto_t prot, int ssl_flag,
                                 ipfix_ssl_opts_t *ssl_opts )
{
    ipfix_collector_t *col;

    if ( (ifh==NULL) || (host==NULL)  )
        return -1;

#ifndef SSLSUPPORT
    if ( ssl_flag ) {
//        errno = ENOTSUP;
        return -1;
    }
#else
    if ( ssl_flag ) {
        if ( ! openssl_is_init ) {
            (void)SSL_library_init();
            SSL_load_error_strings();
            /* todo: seed prng? */
            openssl_is_init ++;
        }
        if ( (ssl_opts==NULL) || (ssl_opts->keyfile==NULL)
             || (ssl_opts->certfile==NULL) ) {
            errno = EINVAL;
            return -1;
        }
    }
#endif

    /* todo: support only one collector yet
     */
    if ( ifh->collectors ) {
        errno = EAGAIN;
        return -1;
    }

    /* check if collector is already in use
     */
    for( col=g_collectors; col; col=col->next ) {
        if ( (strcmp( col->chost, host ) ==0)
             && (col->cport==port) && (col->protocol==prot) ) {
            /* collector found */
            col->usecount++;
            ifh->collectors = (void*)col;
            return 0;
        }
    }

    if ( (col=calloc( 1, sizeof(ipfix_collector_t))) ==NULL)
        return -1;

    if ( (col->chost=strdup( host )) ==NULL ) {
        free( col );
        return -1;
    }

    col->cport    = port;
    col->ssl_flag = ssl_flag;
    switch ( prot ) {
      case IPFIX_PROTO_TCP:
          col->protocol  = prot;
          break;
      case IPFIX_PROTO_UDP:
      case IPFIX_PROTO_SCTP:
          col->protocol  = prot;
          if ( ssl_flag ) {
              free( col->chost );
              free( col );
//              errno = ENOTSUP;
              return -1;
          }
          break;
      default:
          free( col->chost );
          free( col );
//          errno = ENOTSUP;   /* !! ENOTSUP */
          return -1;
    }

#ifdef SSLSUPPORT
    if ( ssl_flag ) {
        if ( ipfix_ssl_opts_new( &(col->ssl_opts), ssl_opts ) <0 ) {
            free( col->chost );
            free( col );
            return -1;
        }
    }
#endif
    col->fd         = -1;
    col->usecount   = 1;
    col->next       = g_collectors;

    g_collectors    = col;
    ifh->collectors = (void*)col;

    /** connect
     */
    return _ipfix_connect( col );
}

/* name:        ipfix_add_collector()
 * parameters:
 * return:      0 = ok, -1 = error
 */
int ipfix_add_collector( ipfix_t *ifh, char *host, int port,
                         ipfix_proto_t prot )
{
    return _ipfix_add_collector( ifh, host, port, prot, 0, NULL );
}

/* name:        ipfix_add_collector_ssl()
 * parameters:
 * return:      0 = ok, -1 = error
 */
int ipfix_add_collector_ssl( ipfix_t *ifh, char *host, int port,
                             ipfix_proto_t prot, ipfix_ssl_opts_t *opts )
{
    return _ipfix_add_collector( ifh, host, port, prot, 1, opts );
}

/*
 * name:        ipfix_new_template()
 * parameters:
 * return:      0 = ok, -1 = error
 */
int ipfix_new_template( ipfix_t          *ifh,
                        ipfix_template_t **templ,
                        int              nfields )
{
    ipfix_template_t  *t;

    if ( !ifh || !templ || (nfields<1) ) {
        errno = EINVAL;
        return -1;
    }

    /** alloc mem
     */
    if ( (t=calloc( 1, sizeof(ipfix_template_t) )) ==NULL )
        return -1;

    if ( (t->fields=calloc( nfields, sizeof(ipfix_template_field_t) )) ==NULL ) {
        free(t);
        return -1;
    }

    /** generate template id, todo!
     */
    g_lasttid++;
    t->tid       = g_lasttid;
    t->nfields   = 0;
    t->maxfields = nfields;
    *templ       = t;

    /** add template to template list
     */
    t->next = ifh->templates;
    ifh->templates = t;
    return 0;
}

/*
 * name:        ipfix_new_data_template()
 * parameters:
 * return:      0 = ok, -1 = error
 */
int ipfix_new_data_template( ipfix_t          *ifh,
                             ipfix_template_t **templ,
                             int              nfields )
{
    if ( ipfix_new_template( ifh, templ, nfields ) <0 )
        return -1;

    (*templ)->type = DATA_TEMPLATE;
    return 0;
}

/*
 * name:        ipfix_new_option_template()
 * parameters:
 * return:      0 = ok, -1 = error
 */
int ipfix_new_option_template( ipfix_t          *ifh,
                               ipfix_template_t **templ,
                               int              nfields )
{
    if ( ipfix_new_template( ifh, templ, nfields ) <0 )
        return -1;

    (*templ)->type = OPTION_TEMPLATE;
    return 0;
}

/*
 * name:        ipfix_add_field()
 * parameters:
 * return:      0 = ok, -1 = error
 */
int ipfix_add_field( ipfix_t          *ifh,
                     ipfix_template_t *templ,
                     uint32_t         eno,
                     uint16_t         type,
                     uint16_t         length )
{
    int i;

    if ( (templ->nfields < templ->maxfields)
         && (type < IPFIX_EFT_VENDOR_BIT) ) {
        /** set template field
         */
        i = templ->nfields;
        templ->fields[i].flength = length;

        if ((templ->fields[i].elem = ipfix_get_ftinfo( eno, type))==NULL) {
            errno = EINVAL;
            return -1;
        }

        templ->nfields ++;
        templ->ndatafields ++;
    }
    else {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

/*
 * name:        ipfix_add_scope_field()
 * parameters:
 * return:      0 = ok, -1 = error
 */
int ipfix_add_scope_field( ipfix_t          *ifh,
                           ipfix_template_t *templ,
                           uint32_t         eno,
                           uint16_t         type,
                           uint16_t         length )
{
    int i;

    if ( templ->type != OPTION_TEMPLATE ) {
        errno = EINVAL;
        return -1;
    }

    if ( templ->nfields < templ->maxfields ) {

        if ( templ->ndatafields ) {
            /** insert scope prior to options
             */
            memmove( &(templ->fields[templ->nscopefields+1]),
                     &(templ->fields[templ->nscopefields]),
                     templ->ndatafields * sizeof(ipfix_template_field_t) );
        }

        /** set template field
         */
        i = templ->nscopefields;
        templ->fields[i].flength = length;

        if ((templ->fields[i].elem
             = ipfix_get_ftinfo( eno, type))==NULL){
            errno = EINVAL;
            return -1;
        }

        templ->nscopefields ++;
        templ->nfields ++;
    }

    return 0;
}

/*
 * name:        ipfix_get_template()
 * parameters:
 * return:      0 = ok, -1 = error
 */
int ipfix_get_template( ipfix_t          *ifh,
                        ipfix_template_t **templ,
                        int              nfields, ... )
{
    ipfix_template_t  *t;
    int               i, error;
    uint16_t          ftype, flength;
    va_list           args;

    /** create new template
     */
    if ( ipfix_new_template( ifh, &t, nfields ) <0 )
        return -1;

    /** fill template fields
     */
    va_start(args, nfields);
    for ( error=0, i=0; i<nfields; i++ )
    {
        ftype   = va_arg( args, int );
        flength = va_arg( args, int );
        if ( ipfix_add_field( ifh, t, 0, ftype, flength ) <0 )
            error =1;
    }
    va_end(args);

    if (error) {
        ipfix_delete_template( ifh, t );
        return -1;
    }

    *templ = t;
    return 0;
}

/*
 * name:        ipfix_get_template_array()
 * parameters:
 * return:      0 = ok, -1 = error
 * todo: share code with ipfix_get_template()
 */
int ipfix_get_template_array( ipfix_t          *ifh,
                              ipfix_template_t **templ,
                              int              nfields,
                              int              *types,
                              int              *lengths )
{
    ipfix_template_t  *t;
    int               i, error, len=0;

    /** alloc mem
     */
    if ( (t=calloc( 1, sizeof(ipfix_template_t) )) ==NULL )
        return -1;

    if ( (t->fields=calloc( nfields, sizeof(ipfix_template_field_t) )) ==NULL )
    {
        free(t);
        return -1;
    }

    /** fill template fields
     */
    for ( error=0, i=0; i<nfields; i++ )
    {
        t->fields[i].elem->ft->ftype = types[i];
        t->fields[i].flength = lengths[i];

        len += t->fields[i].flength;
        if ((t->fields[i].elem
             = ipfix_get_ftinfo( 0, t->fields[i].elem->ft->ftype )) ==NULL) {
            error =1;
        }
    }

    if (error) {
        errno = EINVAL;
        free(t);
        return -1;
    }

    /** generate template id, todo!
     */
    g_lasttid++;
    t->tid = g_lasttid;
    t->nfields = nfields;
    *templ     = t;

    /** add template to template list
     */
    t->next = ifh->templates;
    ifh->templates = t;
    return 0;
}

/*
 * name:        ipfix_free_template()
 * parameters:
 * return:
 */
void ipfix_free_template( ipfix_template_t *templ )
{
    if ( templ )
    {
        if ( templ->fields )
            free( templ->fields );
        free( templ );
    }
}

/*
 * name:        ipfix_delete_template()
 * parameters:
 * return:
 */
void ipfix_delete_template( ipfix_t *ifh, ipfix_template_t *templ )
{
    ipfix_template_t *l, *n;

    if ( ! templ )
        return;

    /** remove template from list
     */
    for( l=ifh->templates, n=l; n!=NULL; n=n->next ) {
        if ( ifh->templates==templ ) {
            ifh->templates = templ->next;
            break;
        }
        if ( n==templ ) {
            l->next = n->next;
            break;
        }
        l=n;
    }

    /** todo: release template id
16     */
    ipfix_free_template( templ );
}

/*
 * name:        ipfix_release_template()
 * parameters:
 * return:
 */
void ipfix_release_template( ipfix_t *ifh, ipfix_template_t *templ )
{
    ipfix_delete_template( ifh, templ );
}

static void _finish_cs( ipfix_t *ifh )
{
    size_t   buflen;
    uint8_t  *buf;

    /* finish current dataset */
    if ( (buf=ifh->cs_header) ==NULL )
        return;
    buflen = 0;
    INSERTU16( buf+buflen, buflen, ifh->cs_tid );
    INSERTU16( buf+buflen, buflen, ifh->cs_bytes );
    ifh->cs_bytes = 0;
    ifh->cs_header = NULL;
    ifh->cs_tid = 0;
}

int ipfix_export( ipfix_t *ifh, ipfix_template_t *templ, ... )
{
    int       i;
    va_list   args;

    if ( !templ ) {
        errno = EINVAL;
        return -1;
    }

    if ( templ->nfields > g_data.maxfields ) {
        if ( g_data.addrs ) free( g_data.addrs );
        if ( g_data.lens ) free( g_data.lens );
        if ( (g_data.lens=calloc( templ->nfields, sizeof(uint16_t))) ==NULL) {
            g_data.maxfields = 0;
            return -1;
        }
        if ( (g_data.addrs=calloc( templ->nfields, sizeof(void*))) ==NULL) {
            free( g_data.lens );
            g_data.lens = NULL;
            g_data.maxfields = 0;
            return -1;
        }
        g_data.maxfields = templ->nfields;
    }

    /** collect pointers
     */
    va_start(args, templ);
    for ( i=0; i<templ->nfields; i++ )
    {
        g_data.addrs[i] = va_arg(args, char*);          /* todo: change this! */
        if ( templ->fields[i].flength == IPFIX_FT_VARLEN )
            g_data.lens[i] = va_arg(args, int);
        else
            g_data.lens[i] = templ->fields[i].flength;
    }
    va_end(args);

    return ipfix_export_array( ifh, templ, templ->nfields,
                               g_data.addrs, g_data.lens );
}

int _ipfix_export_array( ipfix_t          *ifh,
                        ipfix_template_t *templ,
                        int              nfields,
                        void             **fields,
                        uint16_t         *lengths )
{
    int               i, newset_f=0;
    size_t            buflen, datasetlen;
    uint8_t           *p, *buf;

    /** parameter check
     */
    if ( (templ==NULL) || (nfields!=templ->nfields) ) {
        errno = EINVAL;
        return -1;
    }

    /* insert template set
     * todo: - put multiple templates in one set
     *       - separate template transmission per collector/protocol
     */
    if ( templ->tsend == 0 ) {
        ipfix_collector_t *col = ifh->collectors;

        while ( col ) {
            if ( _ipfix_write_template( ifh, col, templ ) <0 )
                return -1;
            col = col->next;
        }
    }
    else {
        /* move this code into a callback func
         */
        ipfix_collector_t *col = ifh->collectors;
        time_t            now  = time(NULL);

        while ( col ) {
            if ( ( col->protocol==IPFIX_PROTO_UDP )
                 && ((now-templ->tsend)>IPFIX_DFLT_TEMPLRESENDINT) ) {
                if ( _ipfix_write_template( ifh, col, templ ) <0 ) {
                    return -1;
                }
                break;
            }
            col = col->next;
        }
    }

    /** get size of data set, check space
     */
    if ( templ->tid == ifh->cs_tid ) {
        newset_f = 0;
        datasetlen = 0;
    }
    else {
        if ( ifh->cs_tid > 0 ) {
            _finish_cs( ifh );
        }
        newset_f = 1;
        datasetlen = 4;
    }
    
    for ( i=0; i<nfields; i++ ) {
        if ( templ->fields[i].flength == IPFIX_FT_VARLEN ) {
            if ( lengths[i]>254 )
                datasetlen += 3;
            else
                datasetlen += 1;
        } else {
            if ( lengths[i] > templ->fields[i].flength ) {
                errno = EINVAL;
                return -1;
            }
        }
        datasetlen += lengths[i];
    }

    if ( (ifh->offset + datasetlen) > IPFIX_DEFAULT_BUFLEN ) {
        if ( ifh->cs_tid )
            _finish_cs( ifh );
        newset_f = 1;

        if ( _ipfix_export_flush( ifh ) <0 )
            return -1;
    }

    /* fill buffer */
    buf    = (uint8_t*)(ifh->buffer) + ifh->offset;
    buflen = 0;

    if ( newset_f ) {
        /* insert data set
         */
        ifh->cs_bytes = 0;
        ifh->cs_header = buf;
        ifh->cs_tid = templ->tid;
        INSERTU16( buf+buflen, buflen, templ->tid );
        INSERTU16( buf+buflen, buflen, (uint16_t)datasetlen );
    }
    /* csc: to be checked with Lutz whether the usage of "datasetlen" 
     * in the last 30 lines of code is correct */

    /* insert data record
     */
    for ( i=0; i<nfields; i++ ) {
        if ( templ->fields[i].flength == IPFIX_FT_VARLEN ) {
            if ( lengths[i]>254 ) {
                *(buf+buflen) = 0xFF;
                buflen++;
                INSERTU16( buf+buflen, buflen, lengths[i] );
            }
            else {
                *(buf+buflen) = (uint8_t)lengths[i];
                buflen++;
            }
        }
        p = fields[i];
        if ( templ->fields[i].relay_f ) {
            ipfix_encode_bytes( p, buf+buflen, lengths[i] ); /* no encoding */
        }
        else {
            templ->fields[i].elem->encode( p, buf+buflen, lengths[i] );
        }
        buflen += lengths[i];
    }

    ifh->nrecords ++;
    ifh->offset += buflen;
    ifh->cs_bytes += (int)buflen;
    if ( ifh->version == IPFIX_VERSION )
        ifh->seqno ++;
    return 0;
}

/* name:        _ipfix_export_flush()
 * parameters:
 * remarks:     rewrite this func!
 */
int _ipfix_export_flush( ipfix_t *ifh )
{
    iobuf_t           *buf;
    ipfix_collector_t *col;
    int               ret;

    if ( (ifh==NULL) || (ifh->offset==0) )
        return 0;

    if ( ifh->cs_tid > 0 ) {
        /* finish current dataset */
        _finish_cs( ifh );
    }
    
    if ( (buf=_ipfix_getbuf()) ==NULL )
        return -1;

#ifdef DEBUG
    mlogf( 0, "[ipfix_export_flush] msg has %d records, %d bytes\n",
           ifh->nrecords, ifh->offset );
#endif
    _ipfix_write_hdr( ifh, buf );
    memcpy( buf->buffer+buf->buflen, ifh->buffer, ifh->offset );
    buf->buflen += ifh->offset;

    col = ifh->collectors;
    ret = 0;
    while ( col ) {
        if ( _ipfix_send_msg( ifh, col, buf ) <0 ) {
            ret = -1;
        }
        else {
            ifh->offset = 0;    /* ugh! */
            ifh->nrecords = 0;
        }

        col = col->next;
    }

    _ipfix_freebuf( buf );
    return ret;
}

int ipfix_export_array( ipfix_t          *ifh,
                        ipfix_template_t *templ,
                        int              nfields,
                        void             **fields,
                        uint16_t         *lengths )
{
    int ret;

    mod_lock();
    ret = _ipfix_export_array( ifh, templ, nfields, fields, lengths );
    mod_unlock();

    return ret;
}

int ipfix_export_flush( ipfix_t *ifh )
{
    int ret;

    mod_lock();
    ret = _ipfix_export_flush( ifh );
    mod_unlock();

    return ret;
}
