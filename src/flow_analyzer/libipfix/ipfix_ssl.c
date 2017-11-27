/*
$$LIC$$
 *
 *   ipfix_ssl.c - IPFIX tls/dtls related funcs
 *
 *   Copyright Fraunhofer FOKUS
 *
 *   $Date: 2009-03-27 20:19:27 +0100 (Fr, 27. Mär 2009) $
 *
 *   $Revision: 96 $
 *
 */
#include "libipfix.h"
#include "ipfix.h"
#include "ipfix_col.h"
#include "ipfix_ssl.h"

/*----- defines ----------------------------------------------------------*/

#define READ16(b) ((*(b)<<8)|*((b)+1))
#define READ32(b) ((((((*(b)<<8)|*(b+1))<<8)|(*(b+2)))<<8)|*(b+3))

/*------ structs ---------------------------------------------------------*/

/*----- revision id ------------------------------------------------------*/

static const char cvsid[]="$Id: ipfix_ssl.c 96 2009-03-27 19:19:27Z csc $";

/*----- globals ----------------------------------------------------------*/

int openssl_is_init = 0;
DH *dh512 = NULL;
DH *dh1024 = NULL;

/*----- prototypes -------------------------------------------------------*/

#ifdef _WIN32

static DH *get_dh512()
{
     static unsigned char dh512_p[]={
             0xC1,0xD6,0x58,0xF1,0x8B,0xE4,0x66,0x46,0x73,0x16,0x48,0x99,
             0x5B,0x75,0x4E,0x86,0x71,0x4B,0x1E,0x07,0x07,0x9C,0x9C,0x82,
             0xBD,0xF0,0xAB,0x6C,0x1B,0x28,0xFD,0x61,0x25,0x7D,0x81,0xCF,
             0x6A,0xA5,0x34,0x03,0xA7,0x2E,0x84,0xE1,0x0E,0x4D,0x1E,0xF8,
             0xF9,0xA4,0x83,0xEE,0xDC,0xDA,0x69,0xCC,0x14,0xF5,0x0B,0x27,
             0x36,0xF1,0xE4,0xCB,
             };
     static unsigned char dh512_g[]={
             0x02,
             };
     DH *dh;
     BIGNUM *p, *g;

     p = BN_bin2bn(dh512_p, sizeof(dh512_p), NULL);
     if (p == NULL)
        return NULL;
     g = BN_bin2bn(dh512_g, sizeof(dh512_g), NULL);
     if (g == NULL)
     {
        BN_free(p);
        return NULL;
     }

     if ((dh=DH_new()) == NULL) 
        return(NULL);
     DH_set0_pqg(dh, p, NULL, g);
     return(dh);
}

static DH *get_dh1024()
{
     static unsigned char dh1024_p[]={
             0xB0,0xBA,0xA1,0x78,0x49,0x7A,0x77,0xD1,0xBD,0xA8,0xBD,0x98,
             0x9A,0x45,0xE9,0x22,0xFC,0xCB,0xE2,0x6E,0x44,0x5E,0xE4,0xFA,
             0x01,0xE9,0x30,0x54,0x7D,0xEE,0xEE,0x8F,0x60,0xF2,0x4E,0xE8,
             0x49,0xCF,0xE2,0xAB,0x9E,0xED,0xD6,0xD2,0x9C,0xAE,0x0A,0xD6,
             0xD6,0x32,0x20,0xF9,0x43,0xCC,0x11,0x00,0xAA,0x4D,0xDB,0xD9,
             0x93,0x0B,0x10,0x0A,0xE8,0xCF,0xA8,0x06,0x5A,0x2D,0x51,0x01,
             0xA6,0xCF,0x02,0xF5,0xF5,0x01,0x65,0xAA,0xA5,0x73,0xCF,0xDE,
             0xC8,0xC5,0xC6,0xE7,0x5D,0x0B,0xCA,0x48,0xA0,0xEF,0x16,0xB9,
             0x98,0x58,0x6D,0xB5,0xEE,0xBA,0xCA,0x54,0xB3,0xA9,0xB7,0xA4,
             0x4F,0x37,0xD9,0x28,0x9B,0xBB,0x01,0xD4,0x06,0xFB,0x6C,0x95,
             0xA9,0x84,0xBE,0xA6,0xD1,0x4B,0x80,0xFB,
             };
     static unsigned char dh1024_g[]={
             0x02,
             };
     DH *dh;
     BIGNUM *p, *g;

     p = BN_bin2bn(dh1024_p, sizeof(dh1024_p), NULL);
     if (p == NULL)
        return NULL;
     g = BN_bin2bn(dh1024_g, sizeof(dh1024_g), NULL);
     if (g == NULL)
     {
        BN_free(p);
        return NULL;
     }

     if ((dh=DH_new()) == NULL) return(NULL);

     DH_set0_pqg(dh, p, NULL, g);
     return(dh);
}

#else
extern DH *get_dh512();
extern DH *get_dh1024();
#endif

/*----- funcs ------------------------------------------------------------*/

void ipfix_ssl_opts_free( ipfix_ssl_opts_t *opts )
{
    if ( !opts )
        return;

    if (opts->cafile)   free(opts->cafile);
    if (opts->cadir)    free(opts->cadir);
    if (opts->keyfile)  free(opts->keyfile);
    if (opts->certfile) free(opts->certfile);
    free( opts );
}

int ipfix_ssl_opts_new( ipfix_ssl_opts_t **ssl_opts,
                        ipfix_ssl_opts_t *vals )
{
    ipfix_ssl_opts_t *opts;

    if ( (opts=calloc( 1, sizeof(ipfix_ssl_opts_t))) ==NULL ) {
        return -1;
    }

    if ( ((vals->cafile) && ((opts->cafile=strdup(vals->cafile))==NULL))
         || ((vals->cadir) && ((opts->cadir=strdup(vals->cadir))==NULL))
         || ((vals->keyfile) && ((opts->keyfile=strdup(vals->keyfile))==NULL))
         || ((vals->certfile)
             && ((opts->certfile=strdup(vals->certfile))==NULL)) ) {
        ipfix_ssl_opts_free( opts );
        return -1;
    }

    *ssl_opts = opts;
    return 0;
}

int ipfix_ssl_verify_callback(int ok, X509_STORE_CTX *store)
{
    char data[256];

    if (!ok)
    {
        X509 *cert = X509_STORE_CTX_get_current_cert(store);
        int  depth = X509_STORE_CTX_get_error_depth(store);
        int  err = X509_STORE_CTX_get_error(store);

        mlogf( 1, "[ipfix_ssl_verify_callback] Error with certificate at depth: %i\n", depth);
        X509_NAME_oneline( X509_get_issuer_name(cert), data, 256);
        mlogf( 1, "  issuer   = %s\n", data);
        X509_NAME_oneline( X509_get_subject_name(cert), data, 256);
        mlogf( 1, "  subject  = %s\n", data);
        mlogf( 1, "  err %i:%s\n", err, X509_verify_cert_error_string(err));
    }

    return ok;
}

long ipfix_ssl_post_connection_check(SSL *ssl, char *host)
{
    X509      *cert;
    X509_NAME *subj;
    char      data[256];
    int       extcount;
    int       ok = 0;

    /* Checking the return from SSL_get_peer_certificate here is not strictly
     * necessary.
     */
    if (!(cert = SSL_get_peer_certificate(ssl)) || !host)
        goto err_occured;
    if ((extcount = X509_get_ext_count(cert)) > 0)
    {
        int i;

        for (i = 0;  i < extcount;  i++)
        {
            char              *extstr;
            X509_EXTENSION    *ext;

            ext = X509_get_ext(cert, i);
            extstr = (char*) OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));

            if (!strcmp(extstr, "subjectAltName"))
            {
                int                  j;
                unsigned char        *data;
                STACK_OF(CONF_VALUE) *val;
                CONF_VALUE           *nval;
                const X509V3_EXT_METHOD    *meth;
                void                 *ext_str = NULL;

                if (!(meth = X509V3_EXT_get(ext)))
                    break;

#if OPENSSL_VERSION_NUMBER >= 0x01010000L
// FIXME: OpenSSL 1.1 implementation
#elif (OPENSSL_VERSION_NUMBER > 0x00907000L)
                data = ext->value->data;
                if (meth->it)
                  ext_str = ASN1_item_d2i(NULL, &data, ext->value->length,
                                          ASN1_ITEM_ptr(meth->it));
                else
                  ext_str = meth->d2i(NULL, &data, ext->value->length);
#else
                data = ext->value->data;
                ext_str = meth->d2i(NULL, &data, ext->value->length);
#endif
                val = meth->i2v(meth, ext_str, NULL);
                for (j = 0;  j < sk_CONF_VALUE_num(val);  j++)
                {
                    nval = sk_CONF_VALUE_value(val, j);
                    if (!strcmp(nval->name, "DNS") && !strcmp(nval->value, host))
                    {
                        ok = 1;
                        break;
                    }
                }
            }
            if (ok)
                break;
        }
    }

    if (!ok && (subj = X509_get_subject_name(cert)) &&
        X509_NAME_get_text_by_NID(subj, NID_commonName, data, 256) > 0)
    {
        data[255] = 0;
        if (stricmp(data, host) != 0)
            goto err_occured;
    }

    X509_free(cert);
    return SSL_get_verify_result(ssl);

err_occured:
    if (cert)
        X509_free(cert);
    return X509_V_ERR_APPLICATION_VERIFICATION;
}

int ipfix_ssl_init_con( SSL *con )
{
    extern FILE *mlog_fp;
    int i;
    char *str;
    long verify_error;
    char buf[100];

    if ((i=SSL_accept(con)) <= 0) {
        if (BIO_sock_should_retry(i)) {
            mlogf( 0, "[ipfix_ssl_init] DELAY\n");
            return -1;
        }

        mlogf( 0, "[ipfix_ssl_init] ERROR\n");
        verify_error=SSL_get_verify_result( con );
        if (verify_error != X509_V_OK) {
            mlogf( 0, "[ipfix_ssl_init] verify error: %s\n",
                   X509_verify_cert_error_string(verify_error));
        }
        else
            ERR_print_errors_fp( mlog_fp );

        return -1;
    }

    if ( mlog_vlevel ) 
	 {
        PEM_write_SSL_SESSION( mlog_fp, SSL_get_session(con));

        if ( SSL_get_shared_ciphers(con, buf, sizeof buf) != NULL)
            mlogf( 3, "[ipfix] Shared ciphers:%s\n", buf);
        str=(char*)SSL_CIPHER_get_name( SSL_get_current_cipher(con) );
        mlogf( 3,  "[ipfix] CIPHER is %s\n",(str != NULL)?str:"(NONE)");
        if (SSL_ctrl(con,SSL_CTRL_GET_FLAGS,0,NULL) &
            TLS1_FLAGS_TLS_PADDING_BUG)
            mlogf( 1, "[ipfix] Peer has incorrect TLSv1 block padding\n");
    }

    return 0;
}

static DH *load_dhparam( char *dhfile )
{
    DH *ret=NULL;
    BIO *bio;

    if ((bio=BIO_new_file(dhfile,"r")) == NULL)
        goto err;
    ret = PEM_read_bio_DHparams(bio,NULL,NULL,NULL);

 err:
    if (bio != NULL)
        BIO_free(bio);

    return ret;
}

static void init_dhparams( void )
{
    if ( (dh512==NULL)
         && ((dh512=load_dhparam("dh512.pem")) ==NULL) ) {
        dh512 = get_dh512();
    }

    if ( (dh1024==NULL)
         && ((dh1024=load_dhparam("dh1024.pem")) ==NULL) ) {
        dh1024 = get_dh1024();
    }
}

static DH *ipfix_ssl_tmp_dh_callback(SSL *ssl, int is_export, int keylength)
{
    DH *ret=NULL;

    switch (keylength) {
      case 512:
          ret = dh512;
          break;

      case 1024:
      default: /* generating DH params is too costly to do on the fly */
          ret = dh1024;
          break;
    }
    return ret;
}

static int ipfix_ssl_setup_ctx( SSL_CTX **ssl_ctx,
                                SSL_METHOD *method,
                                ipfix_ssl_opts_t *ssl_details )
{
    SSL_CTX *ctx;
    char buffer[MAX_PATH];

    if ( ((ctx=SSL_CTX_new(method)) ==NULL )
         || (SSL_CTX_load_verify_locations( ctx, ssl_details->cafile,
                                            ssl_details->cadir ) != 1)) {
        mlogf( 0, "[ipfix] error loading ca file and/or directory "
               "(cwd=%s,file=%s): %s\n", _getcwd(buffer, sizeof(buffer)),
               ssl_details->cafile, strerror(errno) );
        goto err;
    }

    if ( SSL_CTX_set_default_verify_paths(ctx) != 1) {
        mlogf( 0, "[ipfix] error loading default CA file and/or dir\n");
        goto err;
    }

    if ( SSL_CTX_use_certificate_chain_file(ctx, ssl_details->certfile) != 1) {
        mlogf( 0, "[ipfix] error loading certificate from file");
        goto err;
    }

    if ( SSL_CTX_use_PrivateKey_file( ctx, ssl_details->keyfile,
                                      SSL_FILETYPE_PEM ) != 1) {
        mlogf( 0, "[ipfix] Error loading private key from file: %s\n",
               ssl_details->keyfile );
        goto err;
    }

    *ssl_ctx = ctx;
    return 0;

 err:
    SSL_CTX_free( ctx );
    return -1;
}

/*----- exporter funcs (client) ------------------------------------------*/

int ipfix_ssl_setup_client_ctx( SSL_CTX **ssl_ctx,
                                SSL_METHOD *method,
                                ipfix_ssl_opts_t *ssl_details )
{
    SSL_CTX *ctx;

    if ( ipfix_ssl_setup_ctx( &ctx, method?method:SSLv23_method(),
                              ssl_details ) <0 )
        return -1;

    SSL_CTX_set_verify( ctx, SSL_VERIFY_PEER,
                        ipfix_ssl_verify_callback);
    SSL_CTX_set_verify_depth( ctx, 4);
    SSL_CTX_set_options( ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2);
    if (SSL_CTX_set_cipher_list( ctx, CIPHER_LIST) != 1) {
        mlogf( 0, "[ipfix] Error setting cipher list (no valid ciphers)");
        goto err;
    }

    *ssl_ctx = ctx;
    return 0;

 err:
    SSL_CTX_free( ctx );
    return -1;
}

/*----- collector funcs (server) -----------------------------------------*/

int ipfix_ssl_setup_server_ctx( SSL_CTX **ssl_ctx,
                                SSL_METHOD *method,
                                ipfix_ssl_opts_t *ssl_details )
{
    SSL_CTX *ctx;

    if ( ipfix_ssl_setup_ctx( &ctx, method?method:SSLv23_method(),
                              ssl_details ) <0 )
        return -1;

    SSL_CTX_set_verify( ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                        ipfix_ssl_verify_callback );
    SSL_CTX_set_verify_depth( ctx, 4 );
    SSL_CTX_set_options( ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 |
                         SSL_OP_SINGLE_DH_USE);
    if (!dh512 || !dh1024)
        init_dhparams();
    SSL_CTX_set_tmp_dh_callback( ctx, ipfix_ssl_tmp_dh_callback );
    if (SSL_CTX_set_cipher_list( ctx, CIPHER_LIST) != 1) {
        mlogf( 0, "[ipfix] error setting cipher list (no valid ciphers)");
        goto err;
    }

    *ssl_ctx = ctx;
    return 0;

 err:
    SSL_CTX_free( ctx );
    return -1;
}
