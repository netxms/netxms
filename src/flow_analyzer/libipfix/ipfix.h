/*
$$LIC$$
 */
/*
** ipfix.h - export declarations of libipfix
**
** Copyright Fraunhofer FOKUS
**
** $Id: ipfix.h 96 2009-03-27 19:19:27Z csc $
**
*/
#ifndef IPFIX_H
#define IPFIX_H

#ifdef _WIN32
#ifdef LIBIPFIX_EXPORTS
#define LIBIPFIX_EXPORTABLE __declspec(dllexport)
#else
#define LIBIPFIX_EXPORTABLE __declspec(dllimport)
#endif
#else
#define LIBIPFIX_EXPORTABLE
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ipfix_def.h"

#ifndef ENOTSUP
#define ENOTSUP EOPNOTSUPP
#endif

/*------ structs ---------------------------------------------------------*/

/** netflow9 header format
 **  0                   1                   2                   3
 **    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 **   |       Version Number          |            Count              |
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 **   |                           sysUpTime                           |
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 **   |                           UNIX Secs                           |
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 **   |                       Sequence Number                         |
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 **   |                        Source ID                              |
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */   
/** ipfix header format
 **  0                   1                   2                   3
 **    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 **   |       Version Number          |            Length             |
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 **   |                         Export Time                           |
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 **   |                       Sequence Number                         |
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 **   |                     Observation Domain ID                     |
 **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */   
typedef struct {
    uint16_t   version;     /* version of Flow Record format of this packet */
    union {
        struct {
            uint16_t   count;       /* total number of record in this packet */
            uint32_t   sysuptime;   /* sysuptime in milliseconds */
            uint32_t   unixtime;    /* seconds since 1970 */
        } nf9;
        struct {
            uint16_t   length;      /* total number of record in this packet */
            uint32_t   exporttime;  /* seconds since 1970 */
        } ipfix;
    } u;
    uint32_t   seqno;       /* incremental sequence counter */
    uint32_t   sourceid;    /* sourceid / observation domain id */

} ipfix_hdr_t;

#define IPFIX_VERSION_NF9           0x09
#define IPFIX_HDR_BYTES_NF9         20
#define IPFIX_SETID_TEMPLATE_NF9    0
#define IPFIX_SETID_OPTTEMPLATE_NF9 1

#define IPFIX_VERSION               0x0A
#define IPFIX_HDR_BYTES             16
#define IPFIX_SETID_TEMPLATE        2
#define IPFIX_SETID_OPTTEMPLATE     3
#define IPFIX_FT_VARLEN             65535
#define IPFIX_FT_NOENO              0
#define IPFIX_EFT_VENDOR_BIT        0x8000

#define IPFIX_PORTNO                4739
#define IPFIX_TLS_PORTNO            4740

#define IPFIX_DFLT_TEMPLRESENDINT   30
#define IPFIX_DFLT_TEMPLLIFETIME    300

/** bearer protocol
 */
typedef enum {
    IPFIX_PROTO_SCTP = 132,    /* IPPROTO_SCTP */    
    IPFIX_PROTO_TCP  = 6,      /* IPPROTO_TCP  */    
    IPFIX_PROTO_UDP  = 17      /* IPPROTO_UDP  */    
} ipfix_proto_t;

typedef struct
{
    uint16_t            flength;           /* less or eq. elem->flength  */
    int                 unknown_f;         /* set if unknown elem */
    int                 relay_f;           /* just relay no, encoding (exp.) */
    ipfix_field_t       *elem;
} ipfix_template_field_t;

typedef struct ipfix_datarecord
{
    void              **addrs;
    uint16_t          *lens;
    uint16_t          maxfields;         /* sizeof arrays */
} ipfix_datarecord_t;

typedef enum {
    DATA_TEMPLATE, OPTION_TEMPLATE
} ipfix_templ_type_t;

typedef struct ipfix_template
{
    struct ipfix_template   *next; /* for internal use          */
    ipfix_templ_type_t      type;  /* data or option template   */
    time_t                  tsend; /* time of last transmission */

    uint16_t                tid;
    int                     ndatafields;
    int                     nscopefields;
    int                     nfields;        /* = ndata + nscope */
    ipfix_template_field_t  *fields;
    int                     maxfields;         /* sizeof fields */
} ipfix_template_t;

typedef struct
{
    int              sourceid;    /* domain id of the exporting process */
    int              version;     /* ipfix version to export */
    void             *collectors; /* list of collectors */
    ipfix_template_t *templates;  /* list of templates  */

    char        *buffer;          /* output buffer */
    int         nrecords;         /* no. of records in buffer */
    size_t      offset;           /* output buffer fill level */
    uint32_t    seqno;            /* sequence no. of next message */

    /* experimental */
    int        cs_tid;            /* template id of current dataset */
    int        cs_bytes;          /* size of current set */
    uint8_t    *cs_header;        /* start of current set */

} ipfix_t;

/** exporter funcs
 */
int  ipfix_open( ipfix_t **ifh, int sourceid, int ipfix_version );
int  ipfix_add_collector( ipfix_t *ifh, char *host, int port,
                          ipfix_proto_t protocol );
int  ipfix_new_data_template( ipfix_t *ifh,
                              ipfix_template_t **templ, int nfields );
int  ipfix_new_option_template( ipfix_t *ifh,
                                ipfix_template_t **templ, int nfields );
int  ipfix_add_field( ipfix_t *ifh, ipfix_template_t *templ,
                      uint32_t enterprise_number,
                      uint16_t type, uint16_t length );
int  ipfix_add_scope_field( ipfix_t *ifh, ipfix_template_t *templ,
                            uint32_t enterprise_number,
                            uint16_t type, uint16_t length );
void ipfix_delete_template( ipfix_t *ifh, ipfix_template_t *templ );

int  ipfix_export( ipfix_t *ifh, ipfix_template_t *templ, ... );
int  ipfix_export_array( ipfix_t *ifh, ipfix_template_t *templ,
                         int nfields, void **fields, uint16_t *lengths );
int  ipfix_export_flush( ipfix_t *ifh );
void ipfix_close( ipfix_t *ifh );

/** experimental
 */
typedef struct ipfix_ssl_options {
    char            *cafile;
    char            *cadir;
    char            *keyfile;     /* private key */
    char            *certfile;    /* certificate */
} ipfix_ssl_opts_t;

int  ipfix_add_collector_ssl( ipfix_t *ifh, char *host, int port,
                              ipfix_proto_t protocol,
                              ipfix_ssl_opts_t *ssl_opts );

/** collector funcs
 */
ipfix_field_t *ipfix_get_ftinfo( int eno, int ftid );
int  ipfix_get_eno_ieid( char *field, int *eno, int *ieid );
ipfix_field_t *ipfix_create_unknown_ftinfo( int eno, int ftid );
void ipfix_free_unknown_ftinfo( ipfix_field_t *f );


/** common funcs
 */
int  LIBIPFIX_EXPORTABLE ipfix_init( void );
int  LIBIPFIX_EXPORTABLE ipfix_add_vendor_information_elements( ipfix_field_type_t *fields );
void LIBIPFIX_EXPORTABLE ipfix_cleanup( void );

#ifdef __cplusplus
}
#endif
#endif
