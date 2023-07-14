/*
 $$LIC$$
 */
/*
 **     ipfix_col.c - IPFIX collector related funcs
 **
 **     Copyright Fraunhofer FOKUS
 **
 **     $Date: 2010-02-23 11:15:04 +0100 (Tue, 23 Feb 2010) $
 **
 **     $Revision: 166 $
 **
 **     todo: 
 **     - check when to delete stored templates
 **     - enhance SCTP support
 **     - TCP - teardown connection after some idle time
 **     - update to current draft
 **     - return errno/errtext
 **     - revise tls alpha code
 **
 */
#include "libipfix.h"
#include "ipfix_col.h"

#ifdef SSLSUPPORT
#include "ipfix_ssl.h"
#endif

/*----- defines ----------------------------------------------------------*/

#define DFLT_TEMPL_LIFETIME     300

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN        100   /* ugh */
#endif

#define READ16(b) ((*(b)<<8)|*((b)+1))
#define READ32(b) ((((((*(b)<<8)|*(b+1))<<8)|(*(b+2)))<<8)|*(b+3))

/*------ structs ---------------------------------------------------------*/

typedef struct tcp_conn_node
{
   ipfixs_node_t *sources;
   ipfix_input_t *details;
} tcp_conn_t;

typedef struct sctp_assoc_node
{
   struct sctp_assoc_node *next;
   ipfixs_node_t *sources;
   uint32_t assoc_id;
} sctp_assoc_node_t;

#ifdef SSLSUPPORT
typedef struct ssl_conn_node
{
   struct ssl_conn_node *next;
   struct ssl_conn_node *prev;

   SSL *ssl;
   BIO *bio;
   ipfixs_node_t *sources;
   ipfix_input_t *details;
} ssl_conn_t;

typedef struct ipfix_col_ssl_node
{
   struct ipfix_col_ssl_node *next;
   ssl_conn_t *scon;

   SSL_CTX *ctx;

} ipfix_col_ssl_node_t;

typedef struct ipfix_col_ssl
{
   ipfix_col_ssl_node_t *nodes;
   int i;
   int nsocks;
   SOCKET *socks;
   SSL_CTX *ctx;
} ipfix_col_ssl_t;

#endif

/*----- revision id ------------------------------------------------------*/

static const char cvsid[] = "$Id: ipfix_col.c 166 2010-02-23 10:15:04Z csc $";

/*----- globals ----------------------------------------------------------*/

static int g_template_lifetime = DFLT_TEMPL_LIFETIME;

ipfixe_node_t *g_exporter = nullptr; /* list of exporters */
ipfixs_node_t *udp_sources = nullptr; /* list of sources   */
mptimer_t g_mt; /* timer */

#ifdef SCTPSUPPORT
sctp_assoc_node_t *sctp_assocs = nullptr;               /* sctp associations */
#endif

/*----- prototypes -------------------------------------------------------*/

static ipfixt_node_t *_get_ipfixt(ipfixt_node_t *tlist, int tid);
static ipfixs_node_t *_get_ipfix_source(ipfixs_node_t **slist, ipfix_input_t *input, uint32_t odid);
static void _delete_ipfixt(ipfixt_node_t **tlist, ipfixt_node_t *node);
void _delete_ipfix_source(ipfixs_node_t **slist, ipfixs_node_t *node);

/*----- static funcs -----------------------------------------------------*/

/*
 * name:        _get_ipfixt
 * remarks:     template management
 */
static ipfixt_node_t *_get_ipfixt(ipfixt_node_t *tlist, int tid)
{
   while (tlist)
   {
      if (tlist->ipfixt->tid == tid)
         return tlist;

      tlist = tlist->next;
   }

   return NULL;
}

static void _delete_ipfixt(ipfixt_node_t **tlist, ipfixt_node_t *node)
{
   ipfixt_node_t *last, *n = *tlist;
   int i;

   if (n == NULL)
   {
      return;
   }
   else if (n == node)
   {
      *tlist = node->next;
      goto freenode;
   }
   else
   {
      last = n;
   }

   while (n)
   {
      if (n == node)
      {
         last->next = node->next;
         goto freenode;
      }

      last = n;
      n = n->next;
   }

   nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 5, _T("[delete_ipfixt] node %hs not found!\n"), node->ident);

   freenode: for (i = 0; i < node->ipfixt->nfields; i++)
   {
      if (node->ipfixt->fields[i].unknown_f)
      {
         ipfix_free_unknown_ftinfo(node->ipfixt->fields[i].elem);
      }
   }
   free(node->ipfixt->fields);
   free(node->ipfixt);
   free(node);
   return;
}

#ifdef INCR_RXTX_BUFSIZE
/* hack, todo: change this func
 */
int _adapt_rcvdbuf( int sock )
{
    int       sockbufsize;
    socklen_t len = sizeof(sockbufsize);

    sockbufsize = 0;
    if ( getsockopt( sock, SOL_SOCKET, SO_RCVBUF,
                     (void*)&sockbufsize, &len )==0 ) {
        mlogf( 4, "[ipfix] default rcvbuf is %dkB\n", sockbufsize/1024 );
    }
    sockbufsize = 131072;
    if ( setsockopt( sock, SOL_SOCKET, SO_RCVBUF,
                     (void*)&sockbufsize, len ) <0 ) {
        mlogf( 0, "[ipfix] setsockopt() failed: %s\n", strerror(errno) );
    }
    else {
        mlogf( 3, "[ipfix] set rcvbuf to %dkB\n", sockbufsize/1024 );
        sockbufsize = 0;
        if ( getsockopt( sock, SOL_SOCKET, SO_RCVBUF,
                         (void*)&sockbufsize, &len )==0 ) {
            mlogf( 4, "[ipfix] rcvbuf is now %dkB\n", sockbufsize/1024 );
        }
    }
    return 0;
}
#endif

/**
 * Convert sockaddr structure to address:port string
 */
static String SockaddrToString(const struct sockaddr *saddr)
{
   InetAddress addr = InetAddress::createFromSockaddr(saddr);
   StringBuffer b = addr.toString();
   if (addr.getFamily() == AF_INET)
   {
      b.append(_T(':'));
      b.append(ntohs(reinterpret_cast<const struct sockaddr_in*>(saddr)->sin_port));
   }
   else if (addr.getFamily() == AF_INET6)
   {
      b.insert(0, _T('['));
      b.append(_T("]:"));
      b.append(ntohs(reinterpret_cast<const struct sockaddr_in*>(saddr)->sin_port));
   }
   return b;
}

/* name: ipfix_col_input_get_ident
 */
String LIBIPFIX_EXPORTABLE ipfix_col_input_get_ident(ipfix_input_t *input)
{
   if (input->type == IPFIX_INPUT_IPCON)
   {
      return SockaddrToString(input->u.ipcon.addr);
   }
   else
   {
      return String(_T("file"));
   }
}

/* name: _new_ipfix_input
 */
ipfix_input_t *_new_ipfix_input(ipfix_input_t *input)
{
   ipfix_input_t *n;

   if (input == nullptr)
   {
      errno = EINVAL;
      return nullptr;
   }

   if ((n = MemAllocStruct<ipfix_input_t>()) == nullptr)
      return nullptr;

   n->type = input->type;

   switch (input->type)
   {
      case IPFIX_INPUT_IPCON:
         n->u.ipcon.addrlen = input->u.ipcon.addrlen;
         if ((n->u.ipcon.addr = static_cast<struct sockaddr*>(MemAllocZeroed(n->u.ipcon.addrlen))) == nullptr)
         {
            MemFree(n);
            return nullptr;
         }
         memcpy(n->u.ipcon.addr, input->u.ipcon.addr, n->u.ipcon.addrlen);
         break;

      case IPFIX_INPUT_FILE:
         if ((n->u.file.name = MemCopyStringA(input->u.file.name)) == nullptr)
         {
            MemFree(n);
            return nullptr;
         }
         break;

      default:
         errno = EINVAL;
         return nullptr;
   }

   return n;
}

/* name: _free_ipfix_input
 */
void _free_ipfix_input(ipfix_input_t *input)
{
   if (input == NULL)
      return;

   if (input->type == IPFIX_INPUT_IPCON)
   {
      free(input->u.ipcon.addr);
   }
   else if (input->type == IPFIX_INPUT_FILE)
   {
      free(input->u.file.name);
   }

   free(input);
}

/* name: _free_ipfix_source
 */
static void _free_ipfix_source(ipfixs_node_t *s)
{
   if (s == NULL)
      return;

   if (s->input)
   {
      _free_ipfix_input(s->input);
   }

   free(s);
}

/* name:        _get_ipfix_source
 * remarks:     source management
 *              a source is a tuple (exporter, odid)
 */
static ipfixs_node_t *_get_ipfix_source(ipfixs_node_t **slist, ipfix_input_t *input, uint32_t odid)
{
   ipfixs_node_t *s = *slist;
   ipfixe_node_t *e;

   while (s)
   {
      if (s->odid == odid)
      {
         if (input == NULL)
            return s;

         if (s->input->type == input->type)
         {
            switch (input->type)
            {
               case IPFIX_INPUT_IPCON:
                  if ((s->input->u.ipcon.addrlen == input->u.ipcon.addrlen)
                           && (memcmp(s->input->u.ipcon.addr, input->u.ipcon.addr, input->u.ipcon.addrlen) == 0))
                  {
                     return s;
                  }
                  break;
               case IPFIX_INPUT_FILE:
                  if (strcmp(s->input->u.file.name, input->u.file.name) == 0)
                  {
                     return s;
                  }
                  break;
               default:
                  errno = EINVAL;
                  return NULL; /* should not happen */
            }
         }
      }

      s = s->next;
   }

   /* new source
    */
   if ((s = MemAllocStruct<ipfixs_node_t>()) == nullptr)
      return nullptr;
   s->odid = odid;

   /* insert input details */
   if ((s->input = _new_ipfix_input(input)) == nullptr)
   {
      MemFree(s);
      return nullptr;
   }

   /** call exporter funcs (todo: error handling)
    */
   for (e = g_exporter; e != nullptr; e = e->next)
   {
      if (e->elem->export_newsource)
         if (e->elem->export_newsource(s, e->elem->data) < 0)
         {
            _free_ipfix_source(s);
            return nullptr;
         }
   }

   /** insert node
    */
   s->next = *slist;
   *slist = s;
   return s;
}

void _delete_ipfix_source(ipfixs_node_t **slist, ipfixs_node_t *node)
{
   ipfixs_node_t *last, *n = *slist;

   if (n == nullptr)
   {
      return;
   }
   else if (n == node)
   {
      *slist = node->next;
      goto freenode;
   }
   else
   {
      last = n;
   }

   while (n != nullptr)
   {
      if (n == node)
      {
         last->next = node->next;
         goto freenode;
      }

      last = n;
      n = n->next;
   }

   nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[delete_ipfixs] node %u not found"), node->odid);

freenode:
   if (node->fp)
   {
      fclose(node->fp);
   }
   while (node->templates)
      _delete_ipfixt(&(node->templates), node->templates);
   _free_ipfix_source(node);
   return;
}

/*
 * name:        readn() 
 * parameters:  >  int   fd      file descriptor
 *              <> char  *ptr    buffer
 *              >  int   nbytes  number of bytes to read
 * purpose:     read 'n' bytes from a file
 * return:      number of bytes read, -1 = error
 */
static int readn(SOCKET fd, char *ptr, int nbytes)
{
   char *p;
   int nleft, nread;

   nleft = nbytes;
   p = ptr;

   while (nleft > 0)
   {
      nread = recv(fd, ptr, nleft, 0);
      if (nread <= 0)
         return (nread); /* error */

      nleft -= nread;
      ptr += nread;
   }
   return (nbytes - nleft);
}

#ifdef SSLSUPPORT
int ssl_readn(SSL *ssl, char *ptr, int nbytes)
{
   char *p;
   int nleft, nread;

   nleft = nbytes;
   p = ptr;

   while (nleft > 0)
   {
      nread = SSL_read(ssl, ptr, nleft);
      if (nread <= 0)
         return (nread); /* error */

      nleft -= nread;
      ptr += nread;
   }
   return (nbytes - nleft);
}
#endif

/*
 * name:        ipfix_parse_hdr()
 * parameters:
 * return:      0/-1
 */
int ipfix_parse_hdr(const uint8_t *buf, size_t buflen, ipfix_hdr_t *hdr)
{
   uint16_t version = READ16(buf);

   switch (version)
   {
      case IPFIX_VERSION_NF9:
         if (buflen < IPFIX_HDR_BYTES_NF9)
            return -1;
         hdr->version = version;
         hdr->u.nf9.count = READ16(buf + 2);
         hdr->u.nf9.sysuptime = READ32(buf + 4);
         hdr->u.nf9.unixtime = READ32(buf + 8);
         hdr->seqno = READ32(buf + 12);
         hdr->sourceid = READ32(buf + 16);
         break;

      case IPFIX_VERSION:
         if (buflen < IPFIX_HDR_BYTES)
            return -1;
         hdr->version = version;
         hdr->u.ipfix.length = READ16(buf + 2);
         hdr->u.ipfix.exporttime = READ32(buf + 4);
         hdr->seqno = READ32(buf + 8);
         hdr->sourceid = READ32(buf + 12);
         break;

      default:
         hdr->version = -1;
         return -1;
   }
   return 0;
}

/*
 * name:        read_templ_field()
 * return:      0/-1
 */
int ipfix_read_templ_field(const uint8_t *buf, size_t buflen, size_t *nread, ipfix_template_field_t *field)
{
   int ftype, eno;

   if (buflen < 4)
   {
      return -1;
   }
   ftype = READ16(buf);
   field->flength = READ16(buf + 2);
   (*nread) += 4;

   if (ftype & IPFIX_EFT_VENDOR_BIT)
   {
      if (buflen < 8)
      {
         return -1;
      }
      ftype &= (~IPFIX_EFT_VENDOR_BIT);
      eno = READ32(buf + 4);
      (*nread) += 4;
   }
   else
   {
      eno = IPFIX_FT_NOENO;
   }

   if ((field->elem = ipfix_get_ftinfo(eno, ftype)) == NULL)
   {
      /** unknown field -> generate node
       */
      if ((field->elem = ipfix_create_unknown_ftinfo(eno, ftype)) == NULL)
      {
         return -1;
      }
      field->unknown_f = 1; /* mark node, so we can drop em later */
   }
   else
   {
      field->unknown_f = 0;
   }

   return 0;
}

/*
 * name:        ipfix_export_hdr()
 * parameters:
 * return:      0/-1
 */
int ipfix_export_hdr(ipfixs_node_t *s, ipfix_hdr_t *hdr)
{
   ipfixe_node_t *e;
   int retval = 0;

   if (!hdr || !s)
      return -1;

   /** call exporter funcs
    */
   for (e = g_exporter; e != NULL; e = e->next)
   {
      if (e->elem->export_newmsg)
         if (e->elem->export_newmsg(s, hdr, e->elem->data) < 0)
            retval = -1;
   }

   return retval;
}

/*
 * name:        ipfix_decode_trecord()
 * func:        create or update template node
 * return:      0/-1
 */
int ipfix_decode_trecord(ipfixs_node_t *s, int setid, const uint8_t *buf, size_t len, int *nread, ipfixt_node_t **templ)
{
   ipfix_template_t *t;
   ipfixt_node_t *n;
   uint16_t templid, nfields, nscopefields, ndatafields;
   int i, newnode = 0;
   size_t offset;
   static const TCHAR *func = _T("ipfix_decode_trecord");

   errno = EIO;

   /** read template header
    */
   switch (setid)
   {
      case IPFIX_SETID_OPTTEMPLATE:
         if (len < 6)
            goto ioerr;
         templid = READ16(buf);
         nfields = READ16(buf + 2);
         nscopefields = READ16(buf + 4);
         offset = 6;
         ndatafields = nfields - nscopefields;
         break;
      case IPFIX_SETID_OPTTEMPLATE_NF9:
      {
         size_t scopelen, optionlen;

         if (len < 6)
            goto ioerr;
         templid = READ16(buf);
         scopelen = READ16(buf + 2);
         optionlen = READ16(buf + 4);
         offset = 6;
         if ((scopelen + optionlen) < len)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] read invalid nf9 template %d"), func, templid);
            return -1;
         }
         nscopefields = (uint16_t)(scopelen / 4);
         ndatafields = (uint16_t)(optionlen / 4);
         nfields = nscopefields + ndatafields;
         break;
      }
      case IPFIX_SETID_TEMPLATE:
      case IPFIX_SETID_TEMPLATE_NF9:
         if (len < 4)
            goto ioerr;
         templid = READ16(buf);
         nfields = READ16(buf + 2);
         offset = 4;
         ndatafields = nfields;
         nscopefields = 0;
         break;
      default:
         return -1;
   }

   if (nfields == 0)
   {
      /** template withdrawal message
       */
      if (templid == setid)
      {
         while (s->templates)
            _delete_ipfixt(&(s->templates), s->templates);
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T( "[%s] %u withdraw all templates"), func, s->odid);
      }
      else if ((n = _get_ipfixt(s->templates, templid)) == NULL)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T( "[%s] %u got withdraw for non-existant template %d"), func, s->odid, templid);
      }
      else
      {
         _delete_ipfixt(&(s->templates), n);
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T( "[%s] %u withdraw template %u"), func, s->odid, templid);
      }

      *nread = (int)offset;
      *templ = NULL;
      errno = 0;
      return 0;
   }

   nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("tid=%d, nfields=%d(%ds/%d)"), templid, nfields, nscopefields, ndatafields);

   /** get template node
    */
   if (((n = _get_ipfixt(s->templates, templid)) == NULL) || (nfields > n->ipfixt->nfields))
   {

      if (n)
         _delete_ipfixt(&(s->templates), n);

      /** alloc mem
       */
      if ((t = MemAllocStruct<ipfix_template_t>()) == nullptr)
         return -1;

      if ((n = MemAllocStruct<ipfixt_node_t>()) == nullptr)
      {
         MemFree(t);
         return -1;
      }

      if ((t->fields = MemAllocArray<ipfix_template_field_t>(nfields)) == NULL)
      {
         MemFree(t);
         MemFree(n);
         return -1;
      }

      newnode = 1;
      n->ipfixt = t;
   }
   else
   {
      newnode = 0;
      t = n->ipfixt;
      /* todo: remove the code below */
      for (i = 0; i < nfields; i++)
      {
         if (t->fields[i].unknown_f)
         {
            ipfix_free_unknown_ftinfo(t->fields[i].elem);
         }
      }
   }

   t->tid = templid;
   t->nfields = nfields;
   t->ndatafields = ndatafields;
   t->nscopefields = nscopefields;

   /** read field definitions
    */
   for (i = 0; i < nfields; i++)
   {
      if ((offset >= len) || (ipfix_read_templ_field(buf + offset, len - offset, &offset, &(t->fields[i])) < 0))
      {
         goto errend;
      }
   }

   /* generate template ident (experimental)
    */
   if (ipfix_get_template_ident(t, n->ident, sizeof(n->ident)) < 0)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] warning: ipfix_get_template_ident() failed"), func);
   }

   /** insert template into tlist 
    */
   if (newnode)
   {
      n->next = s->templates;
      s->templates = n;
   }
   n->expire_time = time(NULL) + g_template_lifetime;
   *nread = (int)offset;
   *templ = n;
   errno = 0;
   return 0;

errend:
   if (newnode)
   {
      free(t);
      free(n);
   }
   else
   {
      _delete_ipfixt(&(s->templates), n);
   }
   return -1;

ioerr:
   nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] invalid message length"), func);
   return -1;
}

/*
 * name:        ipfix_export_trecord()
 * parameters:
 * return:      0/-1
 */
int ipfix_export_trecord(ipfixs_node_t *s, ipfixt_node_t *t)
{
   ipfixe_node_t *e;

   /** call exporter funcs
    */
   for (e = g_exporter; e != NULL; e = e->next)
   {
      if (e->elem->export_trecord)
         (void)e->elem->export_trecord(s, t, e->elem->data);
   }

   return 0;
}

/*
 * name:        ipfix_export_datarecord()
 * parameters:
 * desc:        this func exports an ipfix data record
 * return:      0=ok, -1=error
 */
int ipfix_export_datarecord(ipfixs_node_t *s, ipfixt_node_t *t, ipfix_datarecord_t *data)
{
   ipfixe_node_t *e;

   /** call exporter funcs
    */
   for (e = g_exporter; e != NULL; e = e->next)
   {
      if (e->elem->export_drecord)
         (void)e->elem->export_drecord(s, t, data, e->elem->data);
   }

   return 0;
}

/*
 * name:        ipfix_decode_datarecord()
 * parameters:
 * desc:        this func parses and exports the ipfix data set
 * return:      0=ok, -1=error
 * todo:        parse message before calling this func
 */
int ipfix_decode_datarecord(ipfixt_node_t *n, unsigned char *buf, int buflen, int *nread, ipfix_datarecord_t *data)
{
   uint8_t *p;
   int i, len, bytesleft;
   static const TCHAR *func = _T("ipfix_decode_datarecord");

   /** check data size
    */
   if (n->ipfixt->nfields > data->maxfields)
   {
      if (data->addrs)
         MemFree(data->addrs);
      if (data->lens)
         MemFree(data->lens);
      if ((data->lens = MemAllocArray<uint16_t>(n->ipfixt->nfields)) == nullptr)
      {
         data->maxfields = 0;
         return -1;
      }
      if ((data->addrs = MemAllocArray<void*>(n->ipfixt->nfields)) == nullptr)
      {
         MemFree(data->lens);
         data->lens = nullptr;
         data->maxfields = 0;
         return -1;
      }
      data->maxfields = n->ipfixt->nfields;
   }

   /** parse message
    */
   bytesleft = buflen;
   *nread = 0;
   p = buf;
   for (i = 0; i < n->ipfixt->nfields; i++)
   {

      len = n->ipfixt->fields[i].flength;
      if (len == IPFIX_FT_VARLEN)
      {
         len = *p;
         p++;
         (*nread)++;
         if (len == 255)
         {
            len = READ16(p);
            p += 2;
            (*nread) += 2;
         }
      }

      bytesleft -= len;
      if (bytesleft < 0)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] record%d: msg too short"), func, i + 1);
         errno = EIO;
         return -1;
      }

      n->ipfixt->fields[i].elem->decode(p, p, len);

      data->lens[i] = len;
      data->addrs[i] = p;

      p += len;
      (*nread) += len;
   }

   return 0;
}

void ipfix_free_datarecord(ipfix_datarecord_t *data)
{
   if (data != nullptr)
   {
      if (data->addrs)
         MemFree(data->addrs);
      if (data->lens)
         MemFree(data->lens);
      memset(data, 0, sizeof(ipfix_datarecord_t));
   }
}

int ipfix_parse_msg(ipfix_input_t *input, ipfixs_node_t **sources, const uint8_t *msg, size_t nbytes)
{
   ipfix_hdr_t hdr; /* ipfix packet header */
   ipfixs_node_t *s;
   ipfix_datarecord_t data =
      { NULL, NULL, 0 };
   ipfixe_node_t *e;
   uint8_t *buf; /* ipfix payload */
   uint16_t setid, setlen; /* set id, set lenght */
   int i, nread, offset; /* counter */
   int bytes, bytesleft;
   int err_flag = 0;
   static const TCHAR *func = _T("ipfix_parse_msg");

   ipfix_col_info_t *raw_exporter = 0;

   if (ipfix_parse_hdr((uint8_t*)msg, nbytes, &hdr) < 0)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] read invalid msg header"), func);
      return -1;
   }

   switch (hdr.version)
   {
      case IPFIX_VERSION_NF9:
         buf = (uint8_t*)msg;
         nread = IPFIX_HDR_BYTES_NF9;
         break;
      case IPFIX_VERSION:
         buf = (uint8_t*)msg;
         nread = IPFIX_HDR_BYTES;
         break;
      default:
         return -1;
   }

   /* get source (= src ip + observation domain id)
    */
   if ((s = _get_ipfix_source(sources, input, hdr.sourceid)) == NULL)
   {
      nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] cannot init new source node"), func);
      return -1;
   }

   /** todo: check if there are missing messages
    */
#ifdef DBSUPPORT
    s->last_message_snr = hdr.seqno;
#endif

   /** check for raw exporter
    */
   for (e = g_exporter; e != NULL; e = e->next)
   {
      if (e->elem->export_rawmsg)
      {
         raw_exporter = e->elem;
         (void)raw_exporter->export_rawmsg(s, msg, nbytes, raw_exporter->data);
         goto end;
      }
   }

   if (ipfix_export_hdr(s, &hdr) < 0)
      return -1;

   /** read ipfix sets
    */
   for (i = 0; (nread + 4) < (int)nbytes; i++)
   {

      if ((hdr.version == IPFIX_VERSION_NF9) && (i >= hdr.u.nf9.count))
      {
         break;
      }

      /** read ipfix record header (set id, lenght)
       */
      setid = READ16(buf + nread);
      setlen = READ16(buf + nread + 2);
      nread += 4;
      if (setlen < 4)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 5, _T("[%s] set%d: invalid set length %d"), func, i + 1, setlen);
         continue;
      }
      setlen -= 4;
      if (setlen > (nbytes - nread))
      {
         int ii;

         for (ii = 0; ii < (int)nbytes; ii++)
            fprintf( stderr, "[%02x]", (msg[ii] & 0xFF));
         fprintf( stderr, "\n");

         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 5, _T("[%s] set%d: message too short (%d>%d)"), func, i + 1, setlen + nread, (int)nbytes);
         goto end;
      }

      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("[%s] set%d: sid=%u, setid=%d, setlen=%d"), func, i + 1, hdr.sourceid, setid, setlen + 4);

      /** read rest of ipfix message
       */
      if ((setid == IPFIX_SETID_TEMPLATE_NF9) || (setid == IPFIX_SETID_OPTTEMPLATE_NF9) || (setid == IPFIX_SETID_TEMPLATE)
               || (setid == IPFIX_SETID_OPTTEMPLATE))
      {
         /** [option] template set
          */
         ipfixt_node_t *t;

         for (offset = nread, bytesleft = setlen; bytesleft > 4;)
         {

            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T( "[%s] set%d: decode template, setlen=%d, left=%d"), func, i + 1, setlen, bytesleft);

            if (ipfix_decode_trecord(s, setid, buf + offset, bytesleft, &bytes, &t) < 0)
            {
               nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] record%d: decode template failed: %s"), func, i + 1, _tcserror(errno));
               break;
            }
            else
            {
               if (t && ipfix_export_trecord(s, t) < 0)
               {
                  goto errend;
               }
               bytesleft -= bytes;
               offset += bytes;
               nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T( "[%s] set%d: %d bytes decoded"), func, i + 1, bytes);
            }
         }
         nread += setlen;
      }
      else if (setid > 255)
      {
         /** get template
          */
         ipfixt_node_t *t;

         if ((t = _get_ipfixt(s->templates, setid)) == NULL)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] no template for %d, skip data set"), func, setid);
            nread += setlen;
            err_flag = 1;
         }
         else
         {
            for (e = g_exporter; e != NULL; e = e->next)
            {
               if (e->elem->export_dset)
                  (void)e->elem->export_dset(t, buf + nread, setlen, e->elem->data);
            }

            /** read data records
             */
            for (offset = nread, bytesleft = setlen; bytesleft > 4;)
            {
               if (ipfix_decode_datarecord(t, buf + offset, bytesleft, &bytes, &data) < 0)
               {
                  nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] set%d: decode record failed: %s"), func, i + 1, _tcserror(errno));
                  goto errend;
               }

               (void)ipfix_export_datarecord(s, t, &data);

               bytesleft -= bytes;
               offset += bytes;
            }

            if (bytesleft)
            {
               nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T( "[%s] set%d: skip %d bytes padding"), func, i + 1, bytesleft);
            }
            nread += setlen;
         }
      }
      else
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] set%d: invalid set id %d, set skipped"), func, i + 1, setid);
         nread += setlen;
      }
   } /* for (read sets */

   if (err_flag)
      goto errend;

end:
   ipfix_free_datarecord(&data);
   return nread;

errend:
   ipfix_free_datarecord(&data);
   return -1;
}

int ipfix_parse_raw_msg(ipfixs_node_t *src, ipfixe_node_t *local_exporter, const uint8_t *msg, size_t nbytes)
{
   ipfix_hdr_t hdr; /* ipfix packet header */
   ipfix_datarecord_t data = { NULL, NULL, 0 };
   ipfixe_node_t *e;
   uint8_t *buf; /* ipfix payload */
   uint16_t setid, setlen; /* set id, set lenght */
   int i, nread, offset; /* counter */
   int bytes, bytesleft;
   int err_flag = 0;
   static const TCHAR *func = _T("ipfix_parse_raw_msg");

   if (ipfix_parse_hdr((uint8_t*)msg, nbytes, &hdr) < 0)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] read invalid msg header"), func);
      return -1;
   }

   switch (hdr.version)
   {
      case IPFIX_VERSION_NF9:
         buf = (uint8_t*)msg;
         nread = IPFIX_HDR_BYTES_NF9;
         break;
      case IPFIX_VERSION:
         buf = (uint8_t*)msg;
         nread = IPFIX_HDR_BYTES;
         break;
      default:
         return -1;
   }

   if (ipfix_export_hdr(src, &hdr) < 0)
      return -1;

   /** read ipfix sets
    */
   for (i = 0; (nread + 4) < nbytes; i++)
   {

      if ((hdr.version == IPFIX_VERSION_NF9) && (i >= hdr.u.nf9.count))
      {
         break;
      }

      /** read ipfix record header (set id, lenght)
       */
      setid = READ16(buf + nread);
      setlen = READ16(buf + nread + 2);
      nread += 4;
      if (setlen < 4)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] set%d: invalid set length %d"), func, i + 1, setlen);
         continue;
      }
      setlen -= 4;
      if (setlen > (nbytes - nread))
      {
         int ii;

         for (ii = 0; ii < nbytes; ii++)
            fprintf(stderr, "[%02x]", (msg[ii] & 0xFF));
         fprintf(stderr, "\n");

         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] set%d: message too short (%d>%d)"), func, i + 1, setlen + nread, (int)nbytes);
         goto end;
      }

      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("[%s] set%d: sid=%u, setid=%d, setlen=%d"), func, i + 1, hdr.sourceid, setid, setlen + 4);

      /** read rest of ipfix message
       */
      if ((setid == IPFIX_SETID_TEMPLATE_NF9) || (setid == IPFIX_SETID_OPTTEMPLATE_NF9) || (setid == IPFIX_SETID_TEMPLATE)
               || (setid == IPFIX_SETID_OPTTEMPLATE))
      {
         /** [option] template set
          */
         ipfixt_node_t *t;

         for (offset = nread, bytesleft = setlen; bytesleft > 4;)
         {

            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("[%s] set%d: decode template, setlen=%d, left=%d"), func, i + 1, setlen, bytesleft);

            if (ipfix_decode_trecord(src, setid, buf + offset, bytesleft, &bytes, &t) < 0)
            {
               nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] record%d: decode template failed: %s"), func, i + 1, _tcserror(errno));
               break;
            }
            else
            {
               for (e = local_exporter; e != NULL; e = e->next)
               {
                  if (e->elem->export_trecord)
                     e->elem->export_trecord(src, t, e->elem->data);
               }

               bytesleft -= bytes;
               offset += bytes;
               nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("[%s] set%d: %d bytes decoded"), func, i + 1, bytes);
            }
         }
         nread += setlen;
      }
      else if (setid > 255)
      {
         /** get template
          */
         ipfixt_node_t *t;

         for (e = local_exporter; e != NULL; e = e->next)
         {
            if (e->elem->export_dset)
            {
               (void)e->elem->export_dset(t, buf + nread, setlen, e->elem->data);
            }
         }

         if ((t = _get_ipfixt(src->templates, setid)) == NULL)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] no template for %d, skip data set"), func, setid);
            nread += setlen;
            err_flag = 1;
         }
         else
         {

            /** read data records
             */
            for (offset = nread, bytesleft = setlen; bytesleft > 4;)
            {
               if (ipfix_decode_datarecord(t, buf + offset, bytesleft, &bytes, &data) < 0)
               {
                  nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] set%d: decode record failed: %s"), func, i + 1, _tcserror(errno));
                  goto errend;
               }

               for (e = local_exporter; e != NULL; e = e->next)
               {
                  if (e->elem->export_drecord)
                     (void)e->elem->export_drecord(src, t, &data, e->elem->data);
               }

               //(void) ipfix_export_datarecord( src, t, &data );

               bytesleft -= bytes;
               offset += bytes;
            }

            if (bytesleft)
            {
               nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T( "[%s] set%d: skip %d bytes padding"), func, i + 1, bytesleft);
            }
            nread += setlen;
         }
      }
      else
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] set%d: invalid set id %d, set skipped"), func, i + 1, setid);
         nread += setlen;
      }
   } /* for (read sets */

   if (err_flag)
      goto errend;

   end: ipfix_free_datarecord(&data);
   return nread;

   errend: ipfix_free_datarecord(&data);
   return -1;
}

void process_client_tcp(int fd, int mask, void *data)
{
   ipfix_hdr_t hdr; /* ipfix packet header */
   int i; /* counter */
   int len, space;
   char tmpbuf[65500], *buf;
   tcp_conn_t *tcon = (tcp_conn_t*)data;
   static const TCHAR *func = _T("process_client_tcp");

   nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("[%s] fd %d mask %d called"), func, fd, mask);

   /** read ipfix header 
    */
   if ((i = readn(fd, tmpbuf, IPFIX_HDR_BYTES_NF9)) != IPFIX_HDR_BYTES_NF9)
   {
      if (i < 0)
      {
         TCHAR buffer[1024];
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] read header failed: %s"), func, GetLastSocketErrorText(buffer, 1024));
         goto end;
      }
      if (i == 0)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 5, _T("[%s] read empty header"), func);
      }

      goto end;
   }

   if (ipfix_parse_hdr((uint8_t*)tmpbuf, IPFIX_HDR_BYTES_NF9, &hdr) < 0)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] read invalid header"), func);
      goto end;
   }

   buf = tmpbuf + IPFIX_HDR_BYTES_NF9;
   space = sizeof(tmpbuf) - IPFIX_HDR_BYTES_NF9;

   if (hdr.version == IPFIX_VERSION_NF9)
   {
      /** read ipfix message, we have to parse the records to get all bytes
       */
      for (i = 0; i < hdr.u.nf9.count; i++)
      {
         /** read ipfix record header (set id, lenght)
          */
         if (space < 4)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] record%d: message too long"), func, i + 1);
            goto end;
         }
         if (readn(fd, buf, 4) != 4)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] record%d: hdr read error"), func, i + 1);
            goto end;
         }

         len = (READ16(buf + 2)) - 4;
         buf += 4;
         space -= 4;
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("[%s] record%d: len=%d, space=%d"), func, i + 1, len, space);
         /** read rest of ipfix message
          */
         if (space < len)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] record%d: message too long"), func, i + 1);
            goto end;
         }
         if (readn(fd, buf, len) != len)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] record%d: len=%d, read error"), func, i + 1, len);
            goto end;
         }
         buf += len;

      } /* for (read records */

      if (ipfix_parse_msg(tcon->details, &(tcon->sources), (uint8_t*)tmpbuf, buf - tmpbuf) < 0)
         goto end;
   }
   else if (hdr.version == IPFIX_VERSION)
   {
      /** read ipfix message
       */
      len = hdr.u.ipfix.length - IPFIX_HDR_BYTES_NF9;
      if (space < len)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] ipfix message too long"), func);
         goto end;
      }

      if (readn(fd, buf, len) != len)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] ipfix message read error"), func);
         goto end;
      }

      if (ipfix_parse_msg(tcon->details, &(tcon->sources), (uint8_t*)tmpbuf, hdr.u.ipfix.length) < 0)
         goto end;
   }
   else
      goto end;

   return;

end:
   /** close connection
    */
   nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T( "[%s] fd %d connection closed"), func, fd);
   mpoll_fdrm(fd);
   closesocket(fd);

   /** free connection related data (templates)
    */
   while (tcon->sources)
      _delete_ipfix_source(&(tcon->sources), tcon->sources);
   _free_ipfix_input(tcon->details);
   MemFree(tcon);
   return;
}

void process_client_udp(int fd, int mask, void *data)
{
   ssize_t nbytes;
   char buf[65500];
#ifdef INET6
    struct sockaddr_storage  caddr;
#else
   struct sockaddr_in caddr;
#endif
   socklen_t caddrlen = sizeof(caddr);
   ipfix_input_t input;
   static const TCHAR *func = _T("process_client_udp");

   nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("[%s] %d/%d called"), func, fd, mask);

   /* no support for multi pdu messages
    */
   if ((nbytes = recvfrom(fd, buf, 65000, 0, (struct sockaddr*)&caddr, &caddrlen)) < 0)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] recvfrom() failed: %s"), func, GetLastSocketErrorText(buffer, 1024));
      return;
   }

   input.type = IPFIX_INPUT_IPCON;
   input.u.ipcon.addr = (struct sockaddr*)&caddr;
   input.u.ipcon.addrlen = caddrlen;
   (void)ipfix_parse_msg(&input, &udp_sources, (uint8_t*)buf, nbytes);
}

#ifdef SCTPSUPPORT

/* name:        _sctp_get_assoc
 * remarks:     sctp assoc management
 */
static sctp_assoc_node_t *_sctp_get_assoc( sctp_assoc_node_t *list, 
                                           uint32_t          assoc_id )
{
    sctp_assoc_node_t *a = list;

    while ( a ) {
        if ( a->assoc_id == assoc_id )
            return a;

        a = a->next;
    }

    return NULL;
}

static sctp_assoc_node_t *_sctp_new_assoc( sctp_assoc_node_t **list, 
                                           uint32_t          assoc_id )
{
    sctp_assoc_node_t *a = *list;

    while ( a ) {
        if ( a->assoc_id == assoc_id ) {
            mlogf( 0, "[ipfix] oops ipfix assoc %x already active!\n", 
                   assoc_id );
            /* todo */
            return a;
        }

        a = a->next;
    }

    /** new assoc
     */
    if ( (a=calloc( 1, sizeof(sctp_assoc_node_t))) ==NULL)
        return NULL;
    a->assoc_id = assoc_id;

    /** insert node
     */
    a->next = *list;
    *list  = a;
    return a;
}

static void _sctp_delete_assoc( sctp_assoc_node_t **list, uint32_t assoc_id )
{
    sctp_assoc_node_t *last=*list, *n=*list;

    if( n==NULL ) {
        return;
    }
    else if ( n->assoc_id==assoc_id ) {
        *list = n->next;
        goto freenode;
    }

    while ( n ) {
        if ( n->assoc_id == assoc_id ) {
            last->next = n->next;
            goto freenode;
        }

        last = n;
        n    = n->next;
    }

 freenode:
    if ( n ) {
        while( n->sources ) {
            _delete_ipfix_source( &(n->sources), n->sources );
        }
        free( n );
    }
    return;
}


void process_client_sctp( int fd, int mask, void *data )
{
#ifdef INET6
    struct sockaddr_storage  caddr;
#else
    struct sockaddr_in       caddr;
#endif
    socklen_t               caddrlen = sizeof(caddr);
    ipfix_input_t           input;
    struct sctp_sndrcvinfo  sri;
    int                     msg_flags;
    sctp_assoc_node_t       *assoc;
    ssize_t                 nbytes;
    char                    buf[65500];
    static const TCHAR *func = _T("process_client_sctp");

    mlogf( 4, "[%s] %d/%d called.\n", func, fd, mask );

    /* read message, check notifications
     */
    if ((nbytes=sctp_recvmsg( fd, buf, 65000,
                              (struct sockaddr *)&caddr, &caddrlen,
                              &sri, &msg_flags )) <0 ) {
        mlogf( 0, "[%s] sctp_recvmsg() failed: %s\n", func, strerror(errno) );
        goto end;
    }
    if ( msg_flags & MSG_NOTIFICATION ) {
        union  sctp_notification *snp = (union sctp_notification *)buf;

        if ( snp->sn_header.sn_type == SCTP_ASSOC_CHANGE ) {
            struct sctp_assoc_change *sac = &snp->sn_assoc_change;

            switch( sac->sac_state ) {
              case SCTP_COMM_UP:
                  (void)_sctp_new_assoc( &sctp_assocs, sac->sac_assoc_id );
                  mlogf( 4, "[%s] new assoc 0x%x (state=%d)\n", 
                         func, sac->sac_assoc_id, sac->sac_state );
                  break;
              case SCTP_RESTART:
                  mlogf( 4, "[%s] restart assoc 0x%x (state=%d)\n", 
                         func, sac->sac_assoc_id, sac->sac_state );
                  break;
              default:
                  (void)_sctp_delete_assoc( &sctp_assocs, sac->sac_assoc_id );
                  mlogf( 4, "[%s] rm assoc 0x%x (state=%d)\n", 
                         func, sac->sac_assoc_id, sac->sac_state );
                  break;
            }
        }

        return;
    }

    /* get assoc
     */
    if ( (assoc=_sctp_get_assoc( sctp_assocs, sri.sinfo_assoc_id )) ==NULL ) {
        mlogf( 0, "[%s] unknown assoc_id 0x%x skipped\n", 
               func, sri.sinfo_assoc_id );
        return;
    }

    mlogf( 3, "[%s] got SCTP message of %lu bytes.\n",
           func, nbytes );

    /* parse message
     */
    input.type = IPFIX_INPUT_IPCON;
    input.u.ipcon.addr = (struct sockaddr *)&caddr;
    input.u.ipcon.addrlen = caddrlen;
    (void) ipfix_parse_msg( &input, &(assoc->sources), (uint8_t*)buf, nbytes );
    mlogf( 3, "[%s] SCTP message had %lu bytes.\n",
           func, nbytes );
    return;

 end:
    /* close connection
     * todo: free connection related data (assocs,sources,templates)
     */
    mlogf( 3, "[%s] fd %d connection closed.\n", func, fd );
    mpoll_fdrm( fd );
    close( fd );
}
#endif

void tcp_accept_cb(int fd, int mask, void *data)
{
   /** new exporter connection
    */
#ifdef INET6
    struct sockaddr_storage  caddr;
#else
   struct sockaddr_in caddr;
#endif
   socklen_t caddrlen = sizeof(caddr);
   SOCKET newsockfd;
   tcp_conn_t *tcon;
   ipfix_input_t input;
   static const TCHAR *func = _T("tcp_accept_cb");

   newsockfd = accept(fd, (struct sockaddr*)&caddr, &caddrlen);
   if (newsockfd < 0)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] accept() error: %s"), func, GetLastSocketErrorText(buffer, 1024));
   }
   else
   {
      /* ok we have a new client */
      String conn = SockaddrToString((struct sockaddr*)&caddr);
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 6, _T("[%s] fd %d connection from %s"), func, newsockfd, conn.cstr());

      /* alloc connection data */
      if ((tcon = MemAllocStruct<tcp_conn_t>()) == nullptr)
      {
         nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] out of memory"), func);
         closesocket(newsockfd);
         return;
      }

      /* insert input details */
      input.type = IPFIX_INPUT_IPCON;
      input.u.ipcon.addr = (struct sockaddr*)&caddr;
      input.u.ipcon.addrlen = caddrlen;
      if ((tcon->details = _new_ipfix_input(&input)) == nullptr)
      {
         nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] failed: %s"), func, strerror(errno));
         MemFree(tcon);
         closesocket(newsockfd);
         return;
      }

      /* add client to poll loop */
      if (mpoll_fdadd(newsockfd, MPOLL_IN, process_client_tcp, (void*)tcon) < 0)
      {
         nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] mpoll_fdadd() failed: out of memory"), func);
         closesocket(newsockfd);
         _free_ipfix_input(tcon->details);
         MemFree(tcon);
         return;
      }
   }/*else*/
}

#ifdef SSLSUPPORT
void process_client_ssl(int fd, int mask, void *data)
{
   ipfix_hdr_t hdr; /* ipfix packet header */
   int i; /* counter */
   ssize_t nbytes;
   char tmpbuf[65500], *buf;
   int len, space, err;
   ssl_conn_t *scon = (ssl_conn_t*)data;
   static const TCHAR *func = _T("process_client_ssl");

   nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("[%s] %d/%d called"), func, fd, mask);

   /* read ipfix header 
    */
   if ((i = ssl_readn(scon->ssl, tmpbuf, IPFIX_HDR_BYTES_NF9)) != IPFIX_HDR_BYTES_NF9)
   {
      if (i < 0)
      {
         err = SSL_get_error(scon->ssl, i);
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] read header failed (ssl err=%d): %s"), func, err, _tcserror(errno));
         goto end;
      }
      if (i == 0)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 5, _T("[%s] read empty header"), func);
      }

      goto end;
   }

   if (ipfix_parse_hdr((uint8_t*)tmpbuf, IPFIX_HDR_BYTES_NF9, &hdr) < 0)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] read invalid header"), func);
      goto end;
   }

   buf = tmpbuf + IPFIX_HDR_BYTES_NF9;
   space = sizeof(tmpbuf) - IPFIX_HDR_BYTES_NF9;

   if (hdr.version == IPFIX_VERSION_NF9)
   {
      /* read ipfix message, we have to parse the records to get all bytes
       */
      for (i = 0; i < hdr.u.nf9.count; i++)
      {
         /* read ipfix record header (set id, lenght)
          */
         if (space < 4)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] record%d: message too long"), func, i + 1);
            goto end;
         }
         if (ssl_readn(scon->ssl, buf, 4) != 4)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 3, _T("[%s] record%d: hdr read error"), func, i + 1);
            goto end;
         }

         len = (READ16(buf + 2)) - 4;
         buf += 4;
         space -= 4;
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 9, _T("[%s] record%d: len=%d, space=%d"), func, i + 1, len, space);
         /* read rest of ipfix message
          */
         if (space < len)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] record%d: message too long"), func, i + 1);
            goto end;
         }
         if (ssl_readn(scon->ssl, buf, len) != len)
         {
            nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] record%d: len=%d, read error"), func, i + 1, len);
            goto end;
         }
         buf += len;

      } /* for (read records */

      if (ipfix_parse_msg(scon->details, &(scon->sources), (uint8_t*)tmpbuf, buf - tmpbuf) < 0)
         goto end;
   }
   else if (hdr.version == IPFIX_VERSION)
   {
      /* read ipfix message
       */
      len = hdr.u.ipfix.length - IPFIX_HDR_BYTES_NF9;
      if (space < len)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] ipfix message too long"), func);
         goto end;
      }

      if ((nbytes = ssl_readn(scon->ssl, buf, len)) != len)
      {
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] ipfix message read error (%d != %d)"), func, nbytes, len);
         TCHAR buffer[1024];
         nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] SSL_read() failed (%d/%d): %s"),
                  func, nbytes, SSL_get_error(scon->ssl, static_cast<int>(nbytes)), GetLastSocketErrorText(buffer, 1024));
         goto end;
      }

      if (ipfix_parse_msg(scon->details, &(scon->sources), (uint8_t*)tmpbuf, hdr.u.ipfix.length) < 0)
      {
         goto end;
      }
   }
   else
      goto end;

   return;

end:
   nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 5, _T("[%s] ssl connection closed, fd=%d"), func, fd);
   mpoll_fdrm(fd);
   if (SSL_get_shutdown(scon->ssl) & SSL_RECEIVED_SHUTDOWN)
      SSL_shutdown(scon->ssl);
   else
      SSL_clear(scon->ssl);
   SSL_free(scon->ssl);
   scon->ssl = nullptr;
   while (scon->sources)
      _delete_ipfix_source(&(scon->sources), scon->sources);
   _free_ipfix_input(scon->details);
}

void accept_client_ssl_cb(int fd, int mask, void *data)
{
#ifdef INET6
    struct sockaddr_storage  caddr;
#else
   struct sockaddr_in caddr;
#endif
   int errorCode;
   socklen_t caddrlen = sizeof(caddr);
   ipfix_col_ssl_node_t *col = (ipfix_col_ssl_node_t*)data;
   ssl_conn_t *scon = nullptr;
   ipfix_input_t input;
   BIO *bio = nullptr;
   SSL *ssl = nullptr;
   char hbuf[NI_MAXHOST];
   static const TCHAR *func = _T("accept_client_ssl_cb");

   /* new exporter connection
    */
   SOCKET newfd = accept(fd, (struct sockaddr*)&caddr, &caddrlen);
   if (newfd == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] accept() error: %s"), func, GetLastSocketErrorText(buffer, 1024));
      return;
   }

   if (nxlog_get_debug_level_tag(LIBIPFIX_DEBUG_TAG) >= 6)
   {
      String conn = SockaddrToString((struct sockaddr*)&caddr);
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 5, _T("[%s] fd %d connection from %s"), func, newfd, conn.cstr());
   }

   /* alloc connection data */
   if ((scon = MemAllocStruct<ssl_conn_t>()) == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] out of memory"), func);
      goto err;
   }

   /* insert input details */
   input.type = IPFIX_INPUT_IPCON;
   input.u.ipcon.addr = (struct sockaddr*)&caddr;
   input.u.ipcon.addrlen = caddrlen;
   if ((scon->details = _new_ipfix_input(&input)) == nullptr)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] failed: %s"), func, _tcserror(errno));
      goto err;
   }

   if ((bio = BIO_new_socket(static_cast<int>(newfd), BIO_NOCLOSE)) == NULL)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] BIO_new_socket() failed"), func);
      goto err;
   }

   if ((ssl = SSL_new(col->ctx)) == NULL)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] SSL_new() failed"), func);
      goto err;
   }

   SSL_set_accept_state(ssl);
   SSL_set_bio(ssl, bio, bio);

   if ((errorCode = SSL_accept(ssl)) <= 0)
   {
      errorCode = SSL_get_error(ssl, errorCode);
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] error accepting ssl connection: errorCode=%d"), func, errorCode);
      goto err;
   }

   if (getpeername(newfd, (struct sockaddr*)&caddr, &caddrlen) < 0)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] cannot get peer address: %s"), func, GetLastSocketErrorText(buffer, 1024));
      goto err;
   }

   if (getnameinfo((struct sockaddr*)&caddr, caddrlen, hbuf, sizeof(hbuf), NULL, 0, 0) < 0)
   {
      TCHAR buffer[1024];
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] cannot get peer name: %s"), func, GetLastSocketErrorText(buffer, 1024));
      goto err;
   }

   if ((errorCode = ipfix_ssl_post_connection_check(ssl, hbuf)) != X509_V_OK)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("error: peer certificate: %hs"), X509_verify_cert_error_string(errorCode));
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] error checking ssl object after connection"), func);
      goto err;
   }

   /* add client to poll loop */
   if (mpoll_fdadd(newfd, MPOLL_IN, process_client_ssl, (void*)scon) < 0)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 2, _T("[%s] mpoll_fdadd() failed: out of memory"), func);
      goto err;
   }

   scon->ssl = ssl;
   scon->bio = bio;
   scon->prev = nullptr;
   scon->next = col->scon;
   if (scon->next)
      scon->next->prev = scon;
   col->scon = scon;
   return;

err:
   if (bio)
      BIO_free(bio);
   if (scon)
   {
      if (scon->details)
         _free_ipfix_input(scon->details);
      MemFree(scon);
   }
   closesocket(newfd);

}
#endif

/*
 * name:        cb_maintenance
 * remarks:     
 */
void cb_maintenance(void *user)
{
   /** drop expired templates
    */
   if (udp_sources)
   {
      ipfixs_node_t *s = udp_sources, *sn;
      ipfixt_node_t *t, *tn;
      time_t now = time( NULL);

      while (s)
      {
         for (t = s->templates; t != NULL;)
         {
            tn = t;
            t = t->next;
            if (now > tn->expire_time)
            {
               nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 5, _T("[ipfix_col] drop template %u:%d"), s->odid, tn->ipfixt->tid);
               _delete_ipfixt(&(s->templates), tn);
            }
         }
         sn = s;
         s = s->next;
         if (sn->templates == NULL)
            _delete_ipfix_source(&udp_sources, sn);
      }/*while( sources*/
   }

   /** reschedule
    */
   g_mt = mpoll_timeradd(30, cb_maintenance, NULL);
}

/*----- export funcs -----------------------------------------------------*/

/* name       : ipfix_get_template_ident
 * parameters :   > template           - ipfix template
 *               <  buf
 *                > buflen
 *
 * description: generate a unique string to identify the template
 * returns    : 0 if ok, -1 on error
 * todo       : remove the calloc/free code
 */
int ipfix_get_template_ident(ipfix_template_t *t, char *identbuf, size_t buflen)
{
   int i, j, found;
   int min, last, cur_eno, next_eno, ft, eno;
   char *ident;
   size_t len = t->nfields * 10; /* todo: calc real lenght! */

   /** alloc mem
    */
   if ((ident = MemAllocArray<char>(len + 1)) == nullptr)
   {
      return -1;
   }

   /** build ident string (sorted ftids separated by '_')
    */
   for (cur_eno = 0, next_eno = 0x7FFFFFFF;;)
   {
      last = 0;
      min = 0xFFFF;

      if (cur_eno)
      {
         snprintf(ident + strlen(ident), len - strlen(ident), "v%x_", cur_eno);
      }

      for (j = 0, found = 0; j < t->nfields; j++)
      {
         for (i = 0; i < t->nfields; i++)
         {
            ft = t->fields[i].elem->ft->ftype;
            eno = t->fields[i].elem->ft->eno;

            if ((eno == cur_eno) && (ft < min) && (ft > last))
            {
               min = ft;
               found++;
            }

            if ((eno < next_eno) && (eno > cur_eno))
               next_eno = eno;
         }

         if (min == 0xFFFF)
            break;

         if (j && (found > 1))
            strcat(ident, "_");

         snprintf(ident + strlen(ident), len - strlen(ident), "%x", min);
         last = min;
         min = 0xFFFF;
      }

      if (next_eno == 0x7FFFFFFF)
         break;

      if ((next_eno > cur_eno) && found)
         strcat(ident, "_");

      cur_eno = next_eno;
      next_eno = 0x7FFFFFFF;
   }

   snprintf(identbuf, buflen, "%s", ident);
   len = strlen(ident);
   MemFree(ident);

   if (len >= buflen)
   {
      return -1;
   }

   return 0;
}

/*
 * name:        ipfix_col_cleanup()
 * parameters:  none
 * return:      void
 * remarks:     
 */
void ipfix_col_cleanup(void)
{
   ipfixe_node_t *e;

   while (udp_sources)
   {
      _delete_ipfix_source(&udp_sources, udp_sources);
   }

#ifdef SCTPSUPPORT
    while ( sctp_assocs ) {
        _sctp_delete_assoc( &sctp_assocs, sctp_assocs->assoc_id );
    }
#endif

   mpoll_timerrm(g_mt);

   /** todo: free nodes
    */
   for (e = g_exporter; e != nullptr; e = e->next)
   {
      if (e->elem->export_cleanup)
         e->elem->export_cleanup(e->elem->data);
   }
}

/* name:        ipfix_listen()
 * parameters:  < nfds, fds
 *              > protocol           - IPFIX_PROTO_XXX
 *              > port               - port number
 *              > family             - INET | INET6
 * return:      0 = ok, -1 = error
 * remarks:     
 */
int ipfix_listen(int *nfds, SOCKET **fds, ipfix_proto_t protocol, int port, int family, int maxcon)
{
   SOCKET *socks;
   int nsocks;
   int socktype, sockproto;
   static const TCHAR *func = _T("ipfix_listen");

#ifdef INET6
    struct addrinfo  hints;
    struct addrinfo  *res, *aip;
    char             portstr[30];
    int              *s, error;
#else
   struct sockaddr_in serv_addr;
   SOCKET s;
#endif

   switch (protocol)
   {
      case IPFIX_PROTO_TCP:
         socktype = SOCK_STREAM;
         sockproto = 0;
         if (maxcon < 1)
         {
            errno = EINVAL;
            return -1;
         }
         break;
      case IPFIX_PROTO_UDP:
         socktype = SOCK_DGRAM;
         sockproto = 0;
         break;
#ifdef SCTPSUPPORT
      case IPFIX_PROTO_SCTP:
          socktype = SOCK_SEQPACKET;
          sockproto= IPPROTO_SCTP;
          if ( maxcon < 1 ) {
              errno = EINVAL;
              return -1;
          }
          break;
#endif
      default:
         errno = WSAEOPNOTSUPP;
         return -1;
   }

#ifdef INET6
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = family;
    hints.ai_flags    = AI_PASSIVE;
    hints.ai_socktype = socktype;
    hints.ai_protocol = sockproto;
    sprintf( portstr, "%d", port );
#ifdef SCTPSUPPORT
    if ( protocol==IPFIX_PROTO_SCTP ) {
        hints.ai_socktype = SOCK_STREAM;  /* work around bug in linux libc (1)*/
        hints.ai_protocol = 0;
    }
#endif
    if ( (error=getaddrinfo( NULL, portstr, &hints, &res)) !=0 ) {
        mlogf( 0, "[%s] getaddrinfo() failed: %s\n", 
               func, gai_strerror(error));
        return -1;
    }

    /** get number of nodes, alloc mem
     */
    for ( aip=res, nsocks=0; aip!=NULL; aip=aip->ai_next )
        nsocks++;
    if ( (socks=calloc( nsocks, sizeof(int))) ==NULL) {
        mlogf( 0, "[%s] %s\n", func, strerror(errno) ); 
        return -1;
    }

    /** "res" has a chain of addrinfo structure filled with
     ** 0.0.0.0 (for IPv4), 0:0:0:0:0:0:0:0 (for IPv6) and alike,
     ** with port filled for "portstr".
     */
    for (aip = res, s=socks, nsocks=0;
         aip != NULL;
         aip = aip->ai_next) 
    {
#ifdef SCTPSUPPORT
        if ( protocol==IPFIX_PROTO_SCTP ) {
            aip->ai_socktype = socktype;  /* work around bug in linux libc (2)*/
            aip->ai_protocol = sockproto;
        }
#endif
        if (((*s)=socket( aip->ai_family, aip->ai_socktype, 
                          aip->ai_protocol )) <0) {
            continue;
        }

        SetSocketExclusiveAddrUse(*s);
		  SetSocketReuseFlag(*s);

#ifdef INCR_RXTX_BUFSIZE
        (void)_adapt_rcvdbuf( *s );
#endif
#if defined(IPV6_V6ONLY)
        if ( (aip->ai_family == AF_INET6)
             && setsockopt( *s, IPPROTO_IPV6, IPV6_V6ONLY,
                            (char *)&sock_opt , sizeof(int) ) != 0) {
            mlogf( 0, "[%s] setsockopt() failed: %s\n", func, strerror(errno) );
            goto err;
        }
#endif

        if( bind( *s, aip->ai_addr, aip->ai_addrlen ) < 0 ) {
            mlogf( 0, "[%s] bind(af%d%s,port=%d) failed: %s\n",
                   func, aip->ai_family,
                   (aip->ai_family==AF_INET6)?"(INET6)"
                   :(aip->ai_family==AF_INET)?"(INET)":"",
                   port, strerror(errno) );
            close( *s );
            continue;
        }

        if ( protocol==IPFIX_PROTO_TCP ) {
            if ( listen( *s, maxcon ) <0 ) {
                mlogf( 0, "[%s] listen() failed: %s\n", func, strerror(errno) );
                /* todo */
            }
        }
#ifdef SCTPSUPPORT
        else if ( protocol==IPFIX_PROTO_SCTP ) {
            struct sctp_event_subscribe events;

            /** set sockopts */
            memset( &events, 0, sizeof(events) );
            events.sctp_data_io_event = 1;
            events.sctp_association_event = 1;
            if ( setsockopt( *s, IPPROTO_SCTP, SCTP_EVENTS,
                             &events, sizeof(events) ) <0 ) {
                mlogf( 0, "[%s] setsockopt() failed: %s\n",
                       func, strerror(errno) );
                /* todo */
            }
            if ( listen( *s, maxcon ) <0 ) {
                mlogf( 0, "[%s] listen() failed: %s\n", func, strerror(errno) );
                close( *s );
                continue;
            }

            /** todo: use sctp_bindx to bind to multiple address
             */
            nsocks++;
            s++;
            break;
        }
#endif
        nsocks++;
        s++;
        continue;

    err:
        while( s >= socks ) {
            close(*s);
            s--;
        }
        freeaddrinfo(res); 
        free(socks);
        return -1;
    }
    freeaddrinfo(res); 
#else
   memset((char*)&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(port);

   if ((s = socket( AF_INET, socktype, sockproto)) == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] socket() failed: %s"), func, GetLastSocketErrorText(buffer, 1024));
      return -1;
   }

   SetSocketExclusiveAddrUse(s);
   SetSocketReuseFlag(s);

#ifdef INCR_RXTX_BUFSIZE
    _adapt_rcvdbuf( s );
#endif
   if (bind(s, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] bind(port=%d) failed: %s"), func, port, GetLastSocketErrorText(buffer, 1024));
      closesocket(s);
      return -1;
   }

   if (protocol == IPFIX_PROTO_TCP)
   {
      if (listen(s, maxcon) < 0)
      {
         TCHAR buffer[1024];
         nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] listen() failed: %s"), func, GetLastSocketErrorText(buffer, 1024));
         closesocket(s);
         return -1;
      }
   }
#ifdef SCTPSUPPORT
    else if ( protocol==IPFIX_PROTO_SCTP )
    {
        struct sctp_event_subscribe events;

        /** set sockopts */
        memset( &events, 0, sizeof(events) );
        events.sctp_data_io_event = 1;
        events.sctp_association_event = 1;
        if (setsockopt( s, IPPROTO_SCTP, SCTP_EVENTS, &events, sizeof(events) ) <0 )
        {
           TCHAR buffer[1024];
           nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] setsockopt() failed: %s"), func, GetLastSocketErrorText(buffer, 1024));
            /* todo */
        }
        if ( listen( s, maxcon ) <0 )
        {
           TCHAR buffer[1024];
            nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] listen() failed: %s"), func, GetLastSocketErrorText(buffer, 1024));
            closesocket(s);
            return -1;
        }
    }
#endif

   nsocks = 1;
   if ((socks = MemAllocArray<SOCKET>(nsocks)) == NULL)
   {
      nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] Out of memory"), func);
      closesocket(s);
      return -1;
   }
   socks[0] = s;

#endif

   if (nsocks == 0)
   {
      nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] cannot set up socket"), func);
      MemFree(socks);
      return -1;
   }

   *fds = socks;
   *nfds = nsocks;
   return 0;
}

/* name:        ipfix_col_listen()
 * parameters:  < nfds, fds
 *              > protocol           - IPFIX_PROTO_XXX
 *              > port               - port number
 *              > family             - INET | INET6
 * return:      0 = ok, -1 = error
 * remarks:     
 */
int ipfix_col_listen(int *nfds, SOCKET **fds, ipfix_proto_t protocol, int port, int family, int maxcon)
{
   int i, nsocks;
   SOCKET *socks;
   pcallback_f callback;
   static const TCHAR *func = _T("ipfix_col_listen");

   switch (protocol)
   {
      case IPFIX_PROTO_TCP:
         callback = tcp_accept_cb;
         break;
      case IPFIX_PROTO_UDP:
         callback = process_client_udp;
         break;
#ifdef SCTPSUPPORT
      case IPFIX_PROTO_SCTP:
          callback = process_client_sctp;
          break;
#endif
      default:
         errno = WSAEOPNOTSUPP;
         return -1;
   }

   if (ipfix_listen(&nsocks, &socks, protocol, port, family, maxcon) < 0)
   {
      return -1;
   }

   /* register callbacks
    */
   for (i = 0; i < nsocks; i++)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T( "[%s] fd %d listen on port %d"), func, socks[i], port);

      if (mpoll_fdadd(socks[i], MPOLL_IN, callback, NULL) < 0)
      {
         nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] mpoll_fdadd() failed: out of memory"), func);
         goto errend;
      }
   }/*for*/

   if (!g_mt)
   {
      /* maintenance callback
       */
      g_mt = mpoll_timeradd(30, cb_maintenance, NULL);
   }

   *fds = socks;
   *nfds = nsocks;
   return 0;

errend:
   for (i = 0; i < nsocks; i++)
   {
      mpoll_fdrm(socks[i]);
      closesocket(socks[i]);
   }
   free(socks);
   return -1;
}

/*
 * name:        ipfix_col_close()
 * parameters:  none
 * return:      void
 * remarks:     close fd returned by ipfix_col_listen()
 */
int ipfix_col_close(SOCKET fd)
{
   mpoll_fdrm(fd);
   return closesocket(fd);
}

int ipfix_col_register_export(ipfix_col_info_t *colinfo)
{
   ipfixe_node_t *n, *e, *last;

   if ((n = MemAllocStruct<ipfixe_node_t>()) == nullptr)
      return -1;

   n->elem = colinfo;

   /* append node to list of exporters
    */
   if (g_exporter == nullptr)
   {
      n->next = g_exporter;
      g_exporter = n;
   }
   else
   {
      for (e = last = g_exporter; e != nullptr; e = e->next)
      {
         last = e;
      }
      n->next = nullptr;
      last->next = n;
   }

   return 0;
}

int ipfix_col_cancel_export(ipfix_col_info_t *colinfo)
{
   ipfixe_node_t *e, *last;

   /* remove node from the list of exporters
    * and call exporters cleanup func if exists
    */
   if (g_exporter->elem == colinfo)
   {
      e = g_exporter;
      g_exporter = g_exporter->next;
      goto freenode;
   }
   else
   {
      for (e = last = g_exporter; e != NULL; e = e->next)
      {
         if (e->elem == colinfo)
         {
            last->next = e->next;
            goto freenode;
         }
         last = e;
      }
   }

   errno = EINVAL;
   return -1;

   freenode: if (e->elem->export_cleanup)
   {
      (void)e->elem->export_cleanup(e->elem->data);
   }

   free(e);
   return 0;
}

int ipfix_col_listen_ssl(ipfix_col_t **handle, ipfix_proto_t protocol, int port, int family, int maxcon, ipfix_ssl_opts_t *ssl_details)
{
#ifndef SSLSUPPORT
    errno = ENOTSUP;
    return -1;
#else
   pcallback_f callback;
   int i, nsocks;
   ipfix_col_ssl_t *col;
   ipfix_col_ssl_node_t *node;
   ipfix_col_ssl_node_t *nodes = nullptr;
   static const TCHAR *func = _T("ipfix_col_listen_ssl");

   ipfix_ssl_init();

   if (!handle || !ssl_details)
   {
      errno = EINVAL;
      return -1;
   }

   if (protocol != IPFIX_PROTO_TCP)
   {
      errno = WSAEOPNOTSUPP;
      return -1;
   }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD *method = TLS_method();
#else
   const SSL_METHOD *method = SSLv23_method();
#endif
   if (method == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] cannot obtain SSL method"), func);
      return -1;
   }

   /* setup SSL_CTX object */
   SSL_CTX *ctx = nullptr;
   if (ipfix_ssl_setup_server_ctx(&ctx, method, ssl_details) < 0)
      return -1;

   /* open socket(s) */
   SOCKET *socks;
   if (ipfix_listen(&nsocks, &socks, protocol, port, family, maxcon) < 0)
      goto err;

   if ((col = MemAllocStruct<ipfix_col_ssl_t>()) == NULL)
   {
      nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] out of memory"), func);
      goto err;
   }
   col->ctx = ctx;
   col->socks = socks;
   col->nsocks = nsocks;

   /* register callbacks */
   for (i = 0; i < nsocks; i++)
   {
      nxlog_debug_tag(LIBIPFIX_DEBUG_TAG, 7, _T("[%s] fd %d listen on port %d"), func, socks[i], port);

      if ((node = MemAllocStruct<ipfix_col_ssl_node_t>()) == NULL)
      {
         nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] out of memory"), func);
         goto err;
      }
      node->ctx = ctx;

      switch (protocol)
      {
         case IPFIX_PROTO_TCP:
            callback = accept_client_ssl_cb;
            break;
         default:
            errno = WSAEOPNOTSUPP;
            return -1;
      }

      if (mpoll_fdadd(socks[i], MPOLL_IN, callback, (void*)node) < 0)
      {
         nxlog_write_tag(NXLOG_ERROR, LIBIPFIX_DEBUG_TAG, _T("[%s] mpoll_fdadd() failed: out of memory"), func);
         /* todo: clean up */
         goto err;
      }

      node->next = nodes;
      nodes = node;
   }/*for*/

   col->nodes = nodes;
   *handle = (ipfix_col_t*)col;
   return 0;

err:
   if (ctx != nullptr)
      SSL_CTX_free(ctx);
   MemFree(col);
   /* todo: free nodes */
   if (socks != nullptr)
   {
      for (i = 0; i < nsocks; i++)
      {
         mpoll_fdrm(socks[i]);
         closesocket(socks[i]);
      }
      MemFree(socks);
   }
   return -1;
#endif
}

int ipfix_col_close_ssl(ipfix_col_t *handle)
{
#ifndef SSLSUPPORT
    errno = EINVAL;
    return -1;
#else
   ipfix_col_ssl_t *col = (ipfix_col_ssl_t*)handle;
   ipfix_col_ssl_node_t *n, *node;
   ssl_conn_t *s, *scon;

   if (col != nullptr)
   {
      if (col->ctx != nullptr)
         SSL_CTX_free(col->ctx);
      for (node = col->nodes; node != nullptr;)
      {
         for (scon = node->scon; scon != nullptr;)
         {
            if (scon->ssl)
               SSL_free(scon->ssl);
            s = scon;
            scon = scon->next;
            MemFree(s);
         }
         n = node;
         node = node->next;
         MemFree(n);
      }
      if (col->socks != nullptr)
      {
         int i;
         for (i = 0; i < col->nsocks; i++)
         {
            mpoll_fdrm(col->socks[i]);
            closesocket(col->socks[i]);
         }
         MemFree(col->socks);
      }
      MemFree(col);
   }
   return 0;
#endif
}
