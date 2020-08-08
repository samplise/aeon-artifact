/* 
 * SimulatorTransportCrypto.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of the contributors, nor their associated universities 
 *      or organizations may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ----END-OF-LEGAL-STUFF---- */
#ifndef SIMULATOR_TRANSPORT_CRYPTO_H
#define SIMULATOR_TRANSPORT_CRYPTO_H

#include "TransportCryptoServiceClass.h"
#include "params.h"

namespace SimulatorTransportCrypto_namespace {

  class SimulatorTransportCryptoService : public virtual TransportCryptoServiceClass, public virtual BaseMaceService {
  public:
    void maceInit() { }
    void maceExit() { }
    mace::string getX509CommonName(const MaceKey& peer, registration_uid_t rid) const {
      return params::get("SIM_TRANSPORT_CRYPTO_COMMON_NAME", cn);
    }
    EVP_PKEY* getPublicKey(const MaceKey& peer, registration_uid_t rid) const {
      return (EVP_PKEY*)&key;
    }

  private:
    mace::string cn;
    EVP_PKEY key;
  };

}

#endif // SIMULATOR_TRANSPORT_CRYPTO_H
