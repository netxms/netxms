/*
$$LIC$$
 */
/*
**     ipfix_print.c - IPFIX message printing funcs
**
**     Copyright Fraunhofer FOKUS
**
**     $Date: 2009-03-19 19:14:44 +0100 (Thu, 19 Mar 2009) $
**
**     $Revision: 1.3 $
**
*/
#include "libipfix.h"
#include "ipfix.h"
#include "ipfix_col.h"

/*----- revision id ------------------------------------------------------*/

static const char cvsid[]="$Id: ipfix_print.c 996 2009-03-19 18:14:44Z csc $";

/*----- globals ----------------------------------------------------------*/

static ipfix_col_info_t *g_colinfo =NULL;
static char             tmpbuf[1000];

static void outf( FILE *fp,
                  char fmt[], ... ) __attribute__ ((format (printf, 2, 3)));

/*----- static funcs -----------------------------------------------------*/

static void outf( FILE *fp, char fmt[], ... )
{
    va_list args;

    va_start( args, fmt );
    (void) vsprintf(tmpbuf, fmt, args );
    va_end( args );

    if ( fp ) {
        fprintf( fp, "%s", tmpbuf );
    }
    else {
        mlogf( 0, "%s", tmpbuf );
    }
}

static int ipfix_print_newsource( ipfixs_node_t *s, void *arg ) 
{
    FILE *fp = (FILE*)arg;

    outf( fp, "#\n# new source: %s/%lu\n#\n",
          ipfix_col_input_get_ident( s->input ), (u_long)s->odid );
    return 0;
}

static int ipfix_print_newmsg( ipfixs_node_t *s, ipfix_hdr_t *hdr, void *arg )
{
    char           timebuf[51];
    FILE           *fp = (FILE*)arg;

    /* print header
     */
    outf( fp, "IPFIX-HDR:\n version=%u,", hdr->version );
    if ( hdr->version == IPFIX_VERSION_NF9 ) 
    {
       time_t t;
        outf( fp, " records=%u\n", hdr->u.nf9.count );
        t = hdr->u.nf9.unixtime;
        strftime( timebuf, 40, "%Y-%m-%d %T %Z", 
                  localtime( &t ));
        outf( fp, " sysuptime=%.3fs, unixtime=%lu (%s)\n", 
              (double)(hdr->u.nf9.sysuptime)/1000.0, 
              (u_long)hdr->u.nf9.unixtime, timebuf );
        outf( fp, " seqno=%lu,", (u_long)hdr->seqno );
        outf( fp, " sourceid=%lu\n", (u_long)hdr->sourceid );
    }
    else {
        outf( fp, " length=%u\n", hdr->u.ipfix.length );
        strftime( timebuf, 40, "%Y-%m-%d %T %Z", 
                  localtime( (const time_t *) &(hdr->u.ipfix.exporttime) ));
        outf( fp, " unixtime=%lu (%s)\n", 
              (u_long)hdr->u.ipfix.exporttime, timebuf );
        outf( fp, " seqno=%lu,", (u_long)hdr->seqno );
        outf( fp, " odid=%lu\n", (u_long)hdr->sourceid );
    }

    return 0;
}

static int ipfix_print_trecord( ipfixs_node_t *s, ipfixt_node_t *t, void *arg )
{
    int   i;
    FILE  *fp = (FILE*)arg;

    outf( fp, "TEMPLATE RECORD:\n" );
    outf( fp, " template id:  %u %s\n", t->ipfixt->tid,
          (t->ipfixt->nscopefields)?"(option template)":"" );
    outf( fp, " nfields:      %u\n", t->ipfixt->nfields );

    for ( i=0; i<t->ipfixt->nfields; i++ ) {
        outf( fp, "  field%02d:%s ie=%u.%u, len=%u (%s)\n", i+1,
              (i<t->ipfixt->nscopefields)?" scope":"",
              t->ipfixt->fields[i].elem->ft->eno, 
              t->ipfixt->fields[i].elem->ft->ftype, 
              t->ipfixt->fields[i].flength,
              t->ipfixt->fields[i].elem->ft->name );
    }

    return 0;
}

static int ipfix_print_drecord( ipfixs_node_t      *s,
                                ipfixt_node_t      *t,
                                ipfix_datarecord_t *data,
                                void               *arg )
{
    char  tmpbuf[2000];
    int   i;
    FILE  *fp = (FILE*)arg;

    if ( !t || !s || !data )
        return -1;

    outf( fp, "DATA RECORD: \n" );
    outf( fp, " template id:  %u %s\n", t->ipfixt->tid,
          (t->ipfixt->nscopefields)?"(option record)":"" );
    outf( fp, " nfields:      %u\n", t->ipfixt->nfields );
    for ( i=0; i<t->ipfixt->nfields; i++ ) {
        outf( fp, " %s: ", t->ipfixt->fields[i].elem->ft->name );

        t->ipfixt->fields[i].elem->snprint( tmpbuf, sizeof(tmpbuf), 
                                            data->addrs[i], data->lens[i] );
        outf( fp, "%s\n", tmpbuf );
    }

    return 0;
}

static void print_cleanup( void *arg )
{
    return;
}

/*----- export funcs -----------------------------------------------------*/

int ipfix_col_start_msglog( FILE *fpout ) 
{
    if ( g_colinfo ) {
        errno = EAGAIN;
        return -1;
    }

    if ( (g_colinfo=calloc( 1, sizeof(ipfix_col_info_t))) ==NULL) {
        return -1;
    }

    g_colinfo->export_newsource = ipfix_print_newsource;
    g_colinfo->export_newmsg    = ipfix_print_newmsg;
    g_colinfo->export_trecord   = ipfix_print_trecord;
    g_colinfo->export_drecord   = ipfix_print_drecord;
    g_colinfo->export_cleanup   = print_cleanup;
    g_colinfo->data = (void*)fpout;

    return ipfix_col_register_export( g_colinfo );
}

void ipfix_col_stop_msglog()
{
    if ( g_colinfo ) {
        (void) ipfix_col_cancel_export( g_colinfo );
        free( g_colinfo );
        g_colinfo = NULL;
    }
}
