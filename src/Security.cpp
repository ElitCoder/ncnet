#include "Security.h"
#include "Log.h"

#include <cryptopp/dh.h>
#include <cryptopp/integer.h>
#include <cryptopp/cmac.h>
#include <cryptopp/modes.h>
#include <cryptopp/gcm.h>

using namespace CryptoPP;
using std::shared_ptr;
using std::make_shared;
using std::string;
using std::vector;
using std::runtime_error;

// http://tools.ietf.org/html/rfc5114#section-2.1
static const Integer p("0xB10B8F96A080E01DDE92DE5EAE5D54EC52C99FBCFB06A3C6"
                       "9A6A9DCA52D23B616073E28675A23D189838EF1E2EE652C0"
                       "13ECB4AEA906112324975C3CD49B83BFACCBDD7D90C4BD70"
                       "98488E9C219A73724EFFD6FAE5644738FAA31A4FF55BCCC0"
                       "A151AF5F0DC8B4BD45BF37DF365C1A65E68CFDA76D4DA708"
                       "DF1FB2BC2E4A4371");

static const Integer g("0xA4D1CBD5C3FD34126765A442EFB99905F8104DD258AC507F"
	                   "D6406CFF14266D31266FEA1E5C41564B777E690F5504F213"
	                   "160217B4B01B886A5E91547F9E2749F4D7FBD7D3B9A92EE1"
	                   "909D0D2263F80A76A6A24C087A091F531DBF0A0169B6A28A"
	                   "D662A4D18E73AFA32D779D5918D08BC8858F4DCEF97C2A24"
	                   "855E6EEB22B3B2E5");

static const Integer q("0xF518AA8781A8DF278ABA4E7D64B7CB9D49462353");

// GCM AES tag size
static constexpr auto TAG_SIZE = 12;

// Default IV size
static constexpr auto IV_SIZE = AES::BLOCKSIZE * 16;

// Default key length (AES-128)
static constexpr auto AES_KEY_LENGTH = 16;

namespace ncnet {
    Security::Security() {
        // Initialize DH and create key-pairs
        dh_ = std::make_shared<DH>();
        dh_->AccessGroupParameters().Initialize(p, q, g);

        dh2_ = make_shared<DH2>(*dh_);
        dh_key_.priv = make_shared<SecByteBlock>(dh2_->StaticPrivateKeyLength());
        dh_key_.pub = make_shared<SecByteBlock>(dh2_->StaticPublicKeyLength());
        sign_key_.priv = make_shared<SecByteBlock>(dh2_->EphemeralPrivateKeyLength());
        sign_key_.pub = make_shared<SecByteBlock>(dh2_->EphemeralPublicKeyLength());

        rnd_ = make_shared<AutoSeededRandomPool>();
        dh2_->GenerateStaticKeyPair(*rnd_, *dh_key_.priv, *dh_key_.pub);
        dh2_->GenerateEphemeralKeyPair(*rnd_, *sign_key_.priv, *sign_key_.pub);
    }

    string Security::get_pub_dh_key() const {
        auto &key = *dh_key_.pub;
        string str_key(reinterpret_cast<const char*>(&key[0]), key.size());
        return str_key;
    }

    string Security::get_pub_sign_key() const {
        auto &key = *sign_key_.pub;
        string str_key(reinterpret_cast<const char*>(&key[0]), key.size());
        return str_key;
    }

    std::string Security::compute_shared_key(const string &client_dh_pub, const string &client_sign_pub) {
        SecByteBlock dh_pub(reinterpret_cast<const byte*>(&client_dh_pub[0]), client_dh_pub.size());
        SecByteBlock sign_pub(reinterpret_cast<const byte*>(&client_sign_pub[0]), client_sign_pub.size());

        shared_key_ = make_shared<SecByteBlock>(dh2_->AgreedValueLength());
        if (!dh2_->Agree(*shared_key_, *dh_key_.priv, *sign_key_.priv, dh_pub, sign_pub)) {
            throw runtime_error("Failed to reach shared secret");
        }

        // Calculate CEK from KEK
        // Take the leftmost 'n' bits for the KEK
        SecByteBlock kek(shared_key_->BytePtr(), AES_KEY_LENGTH);

        // CMAC key follows the 'n' bits used for KEK
        SecByteBlock mack(&shared_key_->BytePtr()[AES_KEY_LENGTH], AES::BLOCKSIZE);
        CMAC<AES> cmac(mack.BytePtr(), mack.SizeInBytes());

        // Generate a random CEK
        cek_ = make_shared<SecByteBlock>(AES_KEY_LENGTH);
        rnd_->GenerateBlock(cek_->BytePtr(), cek_->SizeInBytes());

        // AES in ECB mode is fine - we're encrypting 1 block, so we don't need padding
        ECB_Mode<AES>::Encryption aes;
        aes.SetKey(kek.BytePtr(), kek.SizeInBytes());

        // Will hold the encrypted key and cmac
        SecByteBlock xport(AES::BLOCKSIZE /*ENC(CEK)*/ + AES::BLOCKSIZE /*CMAC*/);
        byte* const ptr = xport.BytePtr();

        // Write the encrypted key in the first 16 bytes, and the CMAC in the second 16 bytes
        // The logical layout of xport:
        //   [    Enc(CEK)    ][  CMAC(Enc(CEK))  ]
        aes.ProcessData(&ptr[0], cek_->BytePtr(), AES::BLOCKSIZE);
        cmac.CalculateTruncatedDigest(&ptr[AES::BLOCKSIZE], AES::BLOCKSIZE, &ptr[0], AES::BLOCKSIZE);

        // Convert to string and return
        string final_packet(reinterpret_cast<const char*>(&xport[0]), xport.size());
        return final_packet;
    }

    void Security::set_encrypted_cek(const string &cek) {
        SecByteBlock encrypted_cek(reinterpret_cast<const byte*>(&cek[0]), AES::BLOCKSIZE);
        SecByteBlock cmac_digest(reinterpret_cast<const byte*>(&cek[AES::BLOCKSIZE]), AES::BLOCKSIZE);
        SecByteBlock out(AES_KEY_LENGTH);

        // Take the leftmost 'n' bits for the KEK
        SecByteBlock kek(shared_key_->BytePtr(), AES_KEY_LENGTH);
        ECB_Mode<AES>::Decryption aes;
        aes.SetKey(kek.BytePtr(), kek.SizeInBytes());
        aes.ProcessData(&out.BytePtr()[0], encrypted_cek.BytePtr(), AES::BLOCKSIZE);

        // Also verify CMAC
        SecByteBlock mack(&shared_key_->BytePtr()[AES_KEY_LENGTH], AES::BLOCKSIZE);
        CMAC<AES> cmac(mack.BytePtr(), mack.SizeInBytes());
        if (!cmac.VerifyTruncatedDigest(&cmac_digest.BytePtr()[0], AES::BLOCKSIZE, &encrypted_cek.BytePtr()[0], AES::BLOCKSIZE)) {
            // No match, fuck this
            throw runtime_error("CMAC validation failed");
        }

        // Store CEK
        cek_ = make_shared<SecByteBlock>(out);
    }

    void Security::encrypt(const shared_ptr<vector<byte>> &plain, size_t start, shared_ptr<vector<byte>> &cipher) {
        byte iv[IV_SIZE];
        rnd_->GenerateBlock(iv, sizeof iv);

        // Encrypted, with Tag
        string cipher2;

        try {
            GCM<AES>::Encryption e;
            e.SetKeyWithIV(*cek_, cek_->size(), iv, sizeof iv);

            // Make room for padding
            cipher->resize(plain->size() + AES::BLOCKSIZE + start);
            ArraySink cs(&(*cipher)[start], cipher->size() - start);

            ArraySource(plain->data() + start, plain->size() - start, true,
                new AuthenticatedEncryptionFilter(e,
                    new Redirector(cs), false, TAG_SIZE
                ) // AuthenticatedEncryptionFilter
            ); // ArraySource

            // Set cipher text length now that its known
            cipher->resize(cs.TotalPutLength() + start);
        } catch (CryptoPP::Exception &e) {
            // Programming error
            throw runtime_error("Encryption failed");
        }

        // Add IV for decrypting
        cipher->insert(cipher->end(), iv, iv + sizeof iv);
    }

    void Security::decrypt(const shared_ptr<vector<byte>> &cipher, size_t start, shared_ptr<vector<byte>> &plain) {
        byte *iv = &cipher->data()[cipher->size() - IV_SIZE];

        try {
            GCM<AES>::Decryption d;
            d.SetKeyWithIV(*cek_, cek_->size(), iv, IV_SIZE);

            // Make room
            plain->resize(cipher->size() - IV_SIZE);
            ArraySink cs(&(*plain)[start], plain->size() - start);

            ArraySource(cipher->data() + start, cipher->size() - start - IV_SIZE, true,
                new AuthenticatedDecryptionFilter(d,
                    new Redirector(cs),
                    AuthenticatedDecryptionFilter::DEFAULT_FLAGS,
                    TAG_SIZE
                )
            );

            // Set cipher text length now that its known
            plain->resize(cs.TotalPutLength() + start);
        } catch (CryptoPP::Exception &e) {
            throw runtime_error("Decryption failed");
        }
    }
}