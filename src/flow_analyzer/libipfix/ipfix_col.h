/*
$$LIC$$
 */
/*
** ipfix_col.h - export declarations of ipfix collector funcs
**
** Copyright Fraunhofer FOKUS
**
** $Id: ipfix_col.h 96 2009-03-27 19:19:27Z csc $
**
*/
#ifndef IPFIX_COL_H
#define IPFIX_COL_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAXTEMPLIDENT        120

typedef enum {
    IPFIX_INPUT_FILE, IPFIX_INPUT_IPCON
} ipfix_input_type_t;

typedef struct ipfix_input {
    ipfix_input_type_t      type;
    union {
        struct {
            struct sockaddr *addr;           /* exporter addess */
            socklen_t       addrlen;         /* exporter address length */
        } ipcon;
        struct {
            char            *name;           /* filename */
        } file;
    } u;
} ipfix_input_t;

typedef struct ipfixt_node
{
    struct ipfixt_node   *next;
    time_t               expire_time;
    ipfix_template_t     *ipfixt;
    char                 ident[MAXTEMPLIDENT];
#ifdef DBSUPPORT
    char                 tablename[MAXTABLENAMELEN+1]; /* make this dynamic */
    unsigned             template_id;      /* id from template table (hack) */
    unsigned             message_snr;      /* last ipfix message snr (hack) */
#endif
} ipfixt_node_t;

typedef struct ipfixs_node
{
    struct ipfixs_node   *next;
    uint32_t             odid;                        /* observation domain */
    ipfix_input_t        *input;                     /* ipfix input details */
    ipfixt_node_t        *templates;                   /* list of templates */
    char                 fname[MAX_PATH+1];
    FILE                 *fp;
    unsigned int         last_seqno;
	 time_t               boot_time;    /* time of system boot */
	 time_t               export_time;  /* export time for IPFIX */
} ipfixs_node_t;

typedef struct ipfixi_node
{
    struct ipfixe_node      *next;
    ipfix_input_t           *input;          /* ipfix input details */
    struct ipfixs_node      *ods;            /* list of observation domains */
} ipfixi_node_t;

typedef struct ipfix_col_info
{
    int (*export_newsource)(ipfixs_node_t*,void*);
    int (*export_newmsg)(ipfixs_node_t*,ipfix_hdr_t*,void*);
    int (*export_trecord)(ipfixs_node_t*,ipfixt_node_t*,void*);
    int (*export_dset)(ipfixt_node_t*,uint8_t*,size_t,void*);
    int (*export_drecord)(ipfixs_node_t*,ipfixt_node_t*,
                          ipfix_datarecord_t*,void*);
    void (*export_cleanup)(void*);
    void *data;
} ipfix_col_info_t;

typedef void* ipfix_col_t;

void LIBIPFIX_EXPORTABLE ipfix_col_init( void );
int  LIBIPFIX_EXPORTABLE ipfix_col_init_fileexport( char *datadir );
void LIBIPFIX_EXPORTABLE ipfix_col_stop_fileexport( void );
int  LIBIPFIX_EXPORTABLE ipfix_col_register_export( ipfix_col_info_t *colinfo );
int  LIBIPFIX_EXPORTABLE ipfix_col_cancel_export( ipfix_col_info_t *colinfo );
int  LIBIPFIX_EXPORTABLE ipfix_col_listen(int *nfds, SOCKET **fds, ipfix_proto_t protocol, 
                                          int port, int family, int maxcon );
int  LIBIPFIX_EXPORTABLE ipfix_col_start_msglog(FILE *fpout); 
void LIBIPFIX_EXPORTABLE ipfix_col_stop_msglog( void ); 
int  LIBIPFIX_EXPORTABLE ipfix_col_close( SOCKET fd );
void LIBIPFIX_EXPORTABLE ipfix_col_cleanup( void );

/* internal, experimental */
int  ipfix_parse_hdr( uint8_t *buf, size_t buflen, ipfix_hdr_t *hdr );
int  ipfix_parse_msg( ipfix_input_t *input, ipfixs_node_t **sources, 
                      uint8_t *msg, size_t nbytes );
int  ipfix_get_template_ident( ipfix_template_t *t, char *buf, size_t buflen );
int  ipfix_col_listen_ssl( ipfix_col_t **handle, ipfix_proto_t protocol, 
                           int port, int family, int maxcon,
                           ipfix_ssl_opts_t *ssl_opts );
int  ipfix_col_close_ssl( ipfix_col_t *handle );


const char LIBIPFIX_EXPORTABLE *ipfix_col_input_get_ident( ipfix_input_t *input );

#ifdef __cplusplus
}
#endif
#endif
