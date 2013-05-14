///
/// Copyright (c) 2013, Aldrin D'Souza
/// All rights reserved. 
/// http://opensource.org/licenses/BSD-2-Clause
/// 
/// Redistribution and use in source and binary forms, with or without modification, are permitted
/// provided that the following conditions are met:
/// 
/// Redistributions of source code must retain the above copyright notice, this list of conditions and
/// the following disclaimer.
/// 
/// Redistributions in binary form must reproduce the above copyright notice, this list of conditions
/// and the following disclaimer in the documentation and/or other materials provided with the
/// distribution.
/// 
/// THIS SOFTWARE IS PROVIDED BY {{THE COPYRIGHT HOLDERS AND CONTRIBUTORS}} "AS IS" AND ANY EXPRESS OR
/// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
/// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL {{THE COPYRIGHT HOLDER OR
/// CONTRIBUTORS}} BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
/// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
/// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
/// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
/// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///
/// Taken entirely from https://github.com/aldrin/home/blob/master/code/c++/crypto/crypto.h
/// and munged to fit.
///
/// Requires OpenSSL 1.0.0 or later, install one from brew.
///
#pragma once

#include <stdexcept>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <boost/array.hpp>
#include <boost/utility.hpp>
#include <boost/asio/buffer.hpp>

#if OPENSSL_VERSION_NUMBER < 0x10000000L
#error OpenSSL 1.0.0 or later is required for PKCS5_PBKDF2_HMAC and EVP aes-128-gcm functions.
#endif

// namespace ajd
// {
  namespace crypto
  {
    namespace internal
    {
      /// Check return values from OpenSSL and throw an exception if it failed.
      /// @param what the logical operation being performed
      /// @success the return value from OpenSSL API
      inline void api(const char *what, int ret)
      {
        if (ret == 0)
        {
          std::string message(what);
          message += " failed.";
          throw std::runtime_error(message);
        }
      }
      /// Internal representation of a raw buffer. Uses boost::asio::buffer to translate the passed
      /// container to a raw pointer and length pair.
      template<typename T> struct raw
      {
        T ptr;
        int len;
        template<typename B> raw(const B &b):
          ptr(boost::asio::buffer_cast<T>(b)),
          len(static_cast<int>(boost::asio::buffer_size(b))) {}
      };
    }

    /// Remove sensitive data from the buffer
    template<typename C> void cleanse(C &c)
    {
      internal::raw<void *> r(boost::asio::buffer(c));
      OPENSSL_cleanse(r.ptr, r.len);
    }

    /// A convenience typedef for a 128 bit block.
    typedef boost::array<unsigned char, 16> block;

    /// Checks if the  underlying PRNG is sufficiently seeded. In  the (exceptional) situation where
    /// this check  returns 'false', you  /must/ use the  OpenSSL seed routines  RAND_seed, RAND_add
    /// directly to add entropy to the underlying PRNG.
    inline bool prng_ok() { return RAND_status() == 1; }

    /// Fills the passed container with random bytes.
    /// @param c  (output) container populated with random bits
    template<typename C> void fill_random(C &c)
    {
      internal::raw<unsigned char *> r(boost::asio::buffer(c));
      internal::api("random bytes", RAND_bytes(r.ptr, r.len));
    }

    /// Derives a  key from a  password and salt  using PBKDF2 with  HMAC-SHA256 as the  chosen PRF.
    /// Although the routine can generate arbitrary length  keys, it is best to use crypto::block as
    /// the type for the key  parameter, since it fixes the key length to 128  bit which is what the
    /// other primitives in the wrapper (crypto::hash, crypto::cipher) require.
    /// @param key      (output) container populated with the key bits
    /// @param pass     (input)  container holding the user password
    /// @param salt     (input)  container holding the salt bytes
    /// @param c        (input)  PBKDF2 iteration count (default=10000)
    template <typename C1, typename C2, typename C3>
    void derive_key(C3 &key, const C1 &pass, const C2 &salt, int c = 10000)
    {
      internal::raw<const char *> p(boost::asio::buffer(pass));
      internal::raw<unsigned char *> k(boost::asio::buffer(key));
      internal::raw<const unsigned char *> s(boost::asio::buffer(salt));
      internal::api("key derivation",
                    PKCS5_PBKDF2_HMAC(p.ptr, p.len, s.ptr, s.len, c, EVP_sha256(),
                                      k.len, k.ptr));
    }

    /// Generates a keyed or a plain cryptographic hash.
    class hash : boost::noncopyable
    {
    public:
      enum { size = 32 };
      /// A convenience typedef for a 256 bit SHA-256 value.
      typedef boost::array<unsigned char, 32> value;

      /// The plain hash constructor. Initializes the underlying hash context.
      hash(): keyed_(false), digest_(EVP_sha256()), md_context_(EVP_MD_CTX_create())
      {
        internal::api("digest initialization", EVP_DigestInit_ex(md_context_, digest_, NULL));
      }

      /// The keyed hash constructor. Initializes the underlying hash context.
      /// @param key (input) container holding the secret key
      template<typename C>
      hash(const C &key): keyed_(true), digest_(EVP_sha256())
      {
        HMAC_CTX_init(&hmac_context_);
        internal::raw<const void *> k(boost::asio::buffer(key));
        internal::api("mac initialization", HMAC_Init_ex(&hmac_context_, k.ptr, k.len, digest_, NULL));
      }

      /// Add the  contents of the passed  container to the  underlying context. This method  can be
      /// invoked multiple times to add all the data that needs to be hashed.
      /// @param data (input) container holding the input data
      template <typename C>
      hash &update(const C &data)
      {
        internal::raw<const void *> d(boost::asio::buffer(data));
        internal::api("add data to hash",
                      (keyed_ ?
                       HMAC_Update(&hmac_context_, (const unsigned char *)d.ptr, d.len)
                       : EVP_DigestUpdate(md_context_, d.ptr, d.len)));
        return *this;
      }

      /// Get the  resultant hash value. This  method also reinitializes the  underlying context, so
      /// the same instance can be reused to compute more hashes.
      /// @param sha (output) container populated with the hash/mac bits
      template<typename C>
      void finalize(C &sha)
      {
        internal::raw<unsigned char *> d(boost::asio::buffer(sha));
        internal::api("finalization of hash",
                      (keyed_ ?
                       HMAC_Final(&hmac_context_, d.ptr, 0)
                       : EVP_DigestFinal_ex(md_context_, d.ptr, NULL)));
        internal::api("reinitialization of hash",
                      (keyed_ ?
                       HMAC_Init_ex(&hmac_context_, NULL, 0, NULL, NULL)
                       : EVP_DigestInit_ex(md_context_, digest_, NULL)));
      }

      /// Cleans up the underlying context.
      ~hash()
      {
        keyed_ ? HMAC_CTX_cleanup(&hmac_context_) : EVP_MD_CTX_destroy(md_context_);
      }
    private:
      bool keyed_;
      const EVP_MD *digest_;
      HMAC_CTX hmac_context_;
      EVP_MD_CTX *md_context_;
    };

    /// Provides authenticated encryption (AES-128-GCM)
    class cipher : boost::noncopyable
    {
    public:
      /// Encryption mode  constructor, only takes key  and IV parameters.  Initializes the instance
      /// for encryption. The  key should be 128  bit (since we're with AES-128).  Typically, the IV
      /// should be 128 bit IV too but GCM supports  other IV sizes, so those can be passed to.
      /// @param key (input) container holding 128 key bits (use crypto::block)
      /// @param iv  (input) container holding 128 initialization vector bits (use crypto::block)
      template<typename K, typename I>
      cipher(const K &key, const I &iv): encrypt_(true)
      {
        initialize(boost::asio::buffer(key), boost::asio::buffer(iv));
      }

      /// Decryption  mode constructor,  takes key,  IV and  the authentication  tag  as parameters.
      /// Initializes  the cipher  for  decryption and  sets  the passed  tag  up for  authenticated
      /// decryption. The key  and IV should be the  same that were used to  generate the ciphertext
      /// you're trying to decrypt (obviously). The seal parameter should contain the authentication
      /// tag returned by the 'seal' call after encryption.
      /// @param key  (input) container holding 128 key bits (use crypto::block)
      /// @param iv   (input) container holding 128 initialization vector bits (use crypto::block)
      /// The seal parameter does not have a 'const' with it because of the OpenSSL API.
      /// @param seal (input) container holding 128 authentication tag bits (use crypto::block)
      template<typename K, typename I, typename S>
      cipher(const K &key, const I &iv, S &seal): encrypt_(false)
      {
        initialize(boost::asio::buffer(key), boost::asio::buffer(iv));
        internal::raw<void *> t(boost::asio::buffer(seal));
        internal::api("set tag", EVP_CIPHER_CTX_ctrl(&context_, EVP_CTRL_GCM_SET_TAG, t.len, t.ptr));
      }

      /// The  cipher transformation  routine. This  encrypts or  decrypts the  bytes from  the 'in'
      /// buffer and places them into the 'out'  buffer.  Since GCM does not require any padding the
      /// output buffer size  should be the same  as the input.  If you  have unencrypted associated
      /// data that must be added using 'associate_data' first.
      /// @param input  (input)  plaintext or ciphertext (for encryption or decryption resp.)
      /// @param output (output) inverse of the input
      template<typename I, typename O>
      cipher &transform(const I &input, O &output)
      {
        int outl;
        internal::raw<unsigned char *> out(boost::asio::buffer(output));
        internal::raw<const unsigned char *> in(boost::asio::buffer(input));
        internal::api("transform", EVP_CipherUpdate(&context_, out.ptr, &outl, in.ptr, in.len));
        return *this;
      }

      /// Adds associated authenticated data, i.e. data which is accounted for in the authentication
      /// tag, but  is not encrypted.  Typically, this  is used for  associated meta data  (like the
      /// packet header in a network protocol). This data must be added /before/ any message text is
      /// added to the cipher.
      /// @param aad (input) container with associated data
      template<typename A>
      cipher &associate_data(const A &aad)
      {
        internal::raw<const unsigned char *>a(boost::asio::buffer(aad));
        if (a.len > 0)
        {
          int outl;
          internal::api("associated data", EVP_CipherUpdate(&context_, NULL, &outl, a.ptr, a.len));
        }
        return *this;
      }

      /// The encryption finalization routine. Populates  the authentication tag "seal" that must be
      /// passed  along for  successful decryption.   Any modifications  in the  cipher text  or the
      /// associated data will be detected by the decryptor using this seal.
      /// @param seal (output) container to be populated with the tag bits
      template<typename S>
      void seal(S &seal)
      {
        int outl;
        assert(encrypt_);
        internal::raw<void *> t(boost::asio::buffer(seal));
        internal::api("seal", EVP_CipherFinal_ex(&context_, NULL, &outl));
        internal::api("get tag", EVP_CIPHER_CTX_ctrl(&context_, EVP_CTRL_GCM_GET_TAG, t.len, t.ptr));
      }

      /// The  decryption  finalization  routine. Uses  the  authentication  tag  to verify  if  the
      /// decryption was successful. If the tag verification fails an exception is thrown, if all is
      /// well,  the method silently  returns.  If  an exception  is thrown,  the decrypted  data is
      /// corrupted and /must/ not be used.
      void verify()
      {
        int outl;
        assert(!encrypt_);
        internal::api("verify", EVP_CipherFinal_ex(&context_, NULL, &outl));
      }

      ~cipher()
      {
        EVP_CIPHER_CTX_cleanup(&context_);
      }
    private:
      bool encrypt_;
      EVP_CIPHER_CTX context_;
      void initialize(boost::asio::const_buffer k, boost::asio::const_buffer i)
      {
        EVP_CIPHER_CTX_init(&context_);
        internal::raw<const unsigned char *>iv(i);
        internal::raw<const unsigned char *>key(k);
        internal::api("initialize - i",
                      EVP_CipherInit_ex(&context_, EVP_aes_128_gcm(), NULL, NULL, NULL, encrypt_));
        internal::api("set iv length",
                      EVP_CIPHER_CTX_ctrl(&context_, EVP_CTRL_GCM_SET_IVLEN, iv.len, NULL));
        internal::api("initialize - ii",
                      EVP_CipherInit_ex(&context_, NULL, NULL, key.ptr, iv.ptr, encrypt_));
      }
    };
  }
// }