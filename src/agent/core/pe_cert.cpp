/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: pe_cert.cpp
**
**/

#include "nxagentd.h"
#include <wincrypt.h>
#include <wintrust.h>

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

/**
 * Get PE certificate information
 */
BOOL GetPeCertificateInfo(LPCWSTR filePath, PE_CERT_INFO *certInfo)
{
    BOOL result = FALSE;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    DWORD dwEncoding, dwContentType, dwFormatType;
    CERT_INFO certSearchParams = {0};
    PCCERT_CONTEXT pCertContext = NULL;
    
    // Get message handle and cert store handle
    if (!CryptQueryObject(
        CERT_QUERY_OBJECT_FILE,
        filePath,
        CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
        CERT_QUERY_FORMAT_FLAG_BINARY,
        0,
        &dwEncoding,
        &dwContentType,
        &dwFormatType,
        &hStore,
        &hMsg,
        NULL)) {
        goto cleanup;
    }

    // Get signer information
    DWORD signerInfoSize = 0;
    if (!CryptMsgGetParam(
        hMsg,
        CMSG_SIGNER_INFO_PARAM,
        0,
        NULL,
        &signerInfoSize)) {
        goto cleanup;
    }

    PCMSG_SIGNER_INFO signerInfo = (PCMSG_SIGNER_INFO)MemAllocZeroed(signerInfoSize);
    if (!signerInfo) {
        goto cleanup;
    }

    if (!CryptMsgGetParam(
        hMsg,
        CMSG_SIGNER_INFO_PARAM,
        0,
        signerInfo,
        &signerInfoSize)) {
        MemFree(signerInfo);
        goto cleanup;
    }

    // Find certificate in store
    certSearchParams.Issuer = signerInfo->Issuer;
    certSearchParams.SerialNumber = signerInfo->SerialNumber;

    pCertContext = CertFindCertificateInStore(
        hStore,
        ENCODING,
        0,
        CERT_FIND_SUBJECT_CERT,
        &certSearchParams,
        NULL);

    if (!pCertContext) {
        MemFree(signerInfo);
        goto cleanup;
    }

    // Get certificate fingerprint (hash)
    DWORD hashSize = 0;
    if (!CertGetCertificateContextProperty(
        pCertContext,
        CERT_SHA1_HASH_PROP_ID,
        NULL,
        &hashSize)) {
        MemFree(signerInfo);
        goto cleanup;
    }

    certInfo->fingerprint = (BYTE*)MemAllocZeroed(hashSize);
    certInfo->fingerprintSize = hashSize;

    if (!CertGetCertificateContextProperty(
        pCertContext,
        CERT_SHA1_HASH_PROP_ID,
        certInfo->fingerprint,
        &hashSize)) {
        MemFree(signerInfo);
        goto cleanup;
    }

    // Get subject and issuer names
    DWORD nameSize = CertGetNameStringW(
        pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        0,
        NULL,
        NULL,
        0);

    if (nameSize > 0) {
        certInfo->subjectName = (LPWSTR)MemAllocZeroed(nameSize * sizeof(WCHAR));
        CertGetNameStringW(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            certInfo->subjectName,
            nameSize);
    }

    nameSize = CertGetNameStringW(
        pCertContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        CERT_NAME_ISSUER_FLAG,
        NULL,
        NULL,
        0);

    if (nameSize > 0) {
        certInfo->issuerName = (LPWSTR)MemAllocZeroed(nameSize * sizeof(WCHAR));
        CertGetNameStringW(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            certInfo->issuerName,
            nameSize);
    }

    MemFree(signerInfo);
    result = TRUE;

cleanup:
    if (pCertContext) CertFreeCertificateContext(pCertContext);
    if (hStore) CertCloseStore(hStore, 0);
    if (hMsg) CryptMsgClose(hMsg);
    return result;
}
