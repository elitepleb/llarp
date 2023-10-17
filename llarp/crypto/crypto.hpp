#pragma once

#include "constants.hpp"
#include "types.hpp"

#include <llarp/util/buffer.hpp>

#include <functional>

#include <cstdint>

namespace llarp
{
  /*
      TODO:
        - make uint8_t pointers const where needed
        -
  */

  struct Crypto
  {
    Crypto();

    ~Crypto() = default;

    /// decrypt cipherText given the key generated from name
    std::optional<AlignedBuffer<32>>
    maybe_decrypt_name(std::string_view ciphertext, SymmNonce nounce, std::string_view name);

    /// xchacha symmetric cipher
    bool
    xchacha20(uint8_t*, size_t size, const SharedSecret&, const TunnelNonce&);
    bool
    xchacha20(uint8_t*, size_t size, const uint8_t*, const uint8_t*);

    /// path dh creator's side
    bool
    dh_client(SharedSecret&, const PubKey&, const SecretKey&, const TunnelNonce&);
    /// path dh relay side
    bool
    dh_server(SharedSecret&, const PubKey&, const SecretKey&, const TunnelNonce&);
    bool
    dh_server(
        uint8_t* shared_secret,
        const uint8_t* other_pk,
        const uint8_t* local_pk,
        const uint8_t* nonce);
    /// transport dh client side
    bool
    transport_dh_client(SharedSecret&, const PubKey&, const SecretKey&, const TunnelNonce&);
    /// transport dh server side
    bool
    transport_dh_server(SharedSecret&, const PubKey&, const SecretKey&, const TunnelNonce&);
    /// blake2b 256 bit
    bool
    shorthash(ShortHash&, uint8_t*, size_t size);
    /// blake2s 256 bit hmac
    bool
    hmac(uint8_t*, uint8_t*, size_t, const SharedSecret&);
    /// ed25519 sign
    bool
    sign(Signature&, const SecretKey&, uint8_t* buf, size_t size);
    /// ed25519 sign, using pointers
    bool
    sign(uint8_t* sig, uint8_t* sk, uint8_t* buf, size_t size);
    bool
    sign(uint8_t* sig, const SecretKey& sk, ustring_view buf);
    /// ed25519 sign (custom with derived keys)
    bool
    sign(Signature&, const PrivateKey&, uint8_t* buf, size_t size);
    /// ed25519 verify
    bool
    verify(const PubKey&, uint8_t*, size_t, const Signature&);
    bool verify(ustring_view, ustring_view, ustring_view);
    bool
    verify(uint8_t*, uint8_t*, size_t, uint8_t*);

    /// derive sub keys for public keys.  hash is really only intended for
    /// testing ands key_n if given.
    bool
    derive_subkey(
        PubKey& derived,
        const PubKey& root,
        uint64_t key_n,
        const AlignedBuffer<32>* hash = nullptr);

    /// derive sub keys for private keys.  hash is really only intended for
    /// testing ands key_n if given.
    bool
    derive_subkey_private(
        PrivateKey& derived,
        const SecretKey& root,
        uint64_t key_n,
        const AlignedBuffer<32>* hash = nullptr);

    /// seed to secretkey
    bool
    seed_to_secretkey(llarp::SecretKey&, const llarp::IdentitySecret&);
    /// randomize buffer
    void
    randomize(uint8_t* buf, size_t len);
    /// randomizer memory
    void
    randbytes(byte_t*, size_t);
    /// generate signing keypair
    void
    identity_keygen(SecretKey&);
    /// generate encryption keypair
    void
    encryption_keygen(SecretKey&);
    /// generate post quantum encrytion key
    void
    pqe_keygen(PQKeyPair&);
    /// post quantum decrypt (buffer, sharedkey_dst, sec)
    bool
    pqe_decrypt(const PQCipherBlock&, SharedSecret&, const byte_t*);
    /// post quantum encrypt (buffer, sharedkey_dst,  pub)
    bool
    pqe_encrypt(PQCipherBlock&, SharedSecret&, const PQPubKey&);

    bool
    check_identity_privkey(const SecretKey&);

    bool
    check_passwd_hash(std::string pwhash, std::string challenge);
  };

  /// return random 64bit unsigned interger
  uint64_t
  randint();

  const byte_t*
  seckey_topublic(const SecretKey& secret);

  const byte_t*
  pq_keypair_to_public(const PQKeyPair& keypair);

  const byte_t*
  pq_keypair_to_secret(const PQKeyPair& keypair);

  struct CryptoManager
  {
   private:
    static Crypto* m_crypto;

    Crypto* m_prevCrypto;

   public:
    explicit CryptoManager(Crypto* crypto) : m_prevCrypto(m_crypto)
    {
      m_crypto = crypto;
    }

    ~CryptoManager()
    {
      m_crypto = m_prevCrypto;
    }

    static Crypto*
    instance()
    {
#ifdef NDEBUG
      return m_crypto;
#else
      if (m_crypto)
        return m_crypto;

      assert(false && "Cryptomanager::instance() was undefined");
      abort();
#endif
    }
  };

  /// rng type that uses llarp::randint(), which is cryptographically secure
  struct CSRNG
  {
    using result_type = uint64_t;

    static constexpr uint64_t
    min()
    {
      return std::numeric_limits<uint64_t>::min();
    };

    static constexpr uint64_t
    max()
    {
      return std::numeric_limits<uint64_t>::max();
    };

    uint64_t
    operator()()
    {
      return llarp::randint();
    };
  };

}  // namespace llarp
