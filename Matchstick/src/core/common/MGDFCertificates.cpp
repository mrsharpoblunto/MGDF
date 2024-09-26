#include "stdafx.h"

#include "MGDFCertificates.hpp"

#include <Windows.h>
#include <wincrypt.h>

#include <sstream>

#pragma warning(disable : 4706)

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

const std::string CertificateManager::S_CA_PEM("ca.pem");

void CertificateManager::LoadCerts(
    std::function<void(const std::string &, const std::string &)> insert) {
  const CERT_ENHKEY_USAGE enhkeyUsage{.cUsageIdentifier = 0,
                                      .rgpszUsageIdentifier = NULL};
  const CERT_USAGE_MATCH certUsage{.dwType = USAGE_MATCH_TYPE_AND,
                                   .Usage = enhkeyUsage};
  CERT_CHAIN_PARA chainParams{.cbSize = sizeof(CERT_CHAIN_PARA),
                              .RequestedUsage = certUsage};

  HCERTSTORE store = CertOpenSystemStoreA(NULL, "ROOT");
  if (!store) {
    return;
  }

  PCCERT_CONTEXT cert = nullptr;
  DWORD size = 0;
  std::ostringstream oss;
  // iterate through all certificates in the trusted root
  while (cert = CertEnumCertificatesInStore(store, cert)) {
    PCCERT_CHAIN_CONTEXT chain;
    if (CertGetCertificateChain(NULL, cert, NULL, NULL, &chainParams, 0, NULL,
                                &chain)) {
      const DWORD errorStatus = chain->TrustStatus.dwErrorStatus;
      CertFreeCertificateChain(chain);
      if (!errorStatus) {
        // if the cert has a valid chain, then add it to our PEM to pass
        // into mbedtls
        CryptBinaryToStringA(cert->pbCertEncoded, cert->cbCertEncoded,
                             CRYPT_STRING_BASE64HEADER, nullptr, &size);
        std::string buffer;
        buffer.resize(static_cast<size_t>(size) -
                      1);  // size includes null-terminator which is
                           // added to the string implicitly
        CryptBinaryToStringA(cert->pbCertEncoded, cert->cbCertEncoded,
                             CRYPT_STRING_BASE64HEADER, buffer.data(), &size);
        oss << buffer;
      }
    }
  }
  CertCloseStore(store, 0);
  // store it in our in memory filesystem cache
  insert(S_CA_PEM, oss.str());
}
}  // namespace core

}  // namespace MGDF
