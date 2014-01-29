/*
 * Header file for the ICE encryption library.
 *
 * Written by Matthew Kwan - July 1996
 * License, as stated on http://www.darkside.com.au/ice/index.html:
 *
 *    The ICE algorithm is public domain, and source code can be
 *    downloaded from this page. There are no patents or copyrights,
 *    so its use is unrestricted. Naturally, if you do decide to
 *    use it, the author would love to hear about it.
 */

#ifndef _ICE_H
#define _ICE_H

typedef struct ice_key_struct	ICE_KEY;

#if __STDC__ || defined(__cplusplus)
#define P_(x) x
#else
#define P_(x) ()
#endif

#ifdef __cplusplus
extern "C" {
#endif

ICE_KEY	*ice_key_create P_((int n));
void	ice_key_destroy P_((ICE_KEY *ik));
void	ice_key_set P_((ICE_KEY *ik, const unsigned char *k));
void	ice_key_encrypt P_((const ICE_KEY *ik,
			const unsigned char *ptxt, unsigned char *ctxt));
void	ice_key_decrypt P_((const ICE_KEY *ik,
			const unsigned char *ctxt, unsigned char *ptxt));
int	ice_key_key_size P_((const ICE_KEY *ik));
int	ice_key_block_size P_((const ICE_KEY *ik));

#ifdef __cplusplus
}
#endif

#undef P_

#endif
