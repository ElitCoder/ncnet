#pragma once

#include <cryptopp/secblock.h>
#include <cryptopp/dh.h>
#include <cryptopp/dh2.h>

namespace ncnet {
    struct KeyPair {
        std::shared_ptr<CryptoPP::SecByteBlock> priv;
        std::shared_ptr<CryptoPP::SecByteBlock> pub;
    };

    class Security {
    public:
        explicit Security();
        std::string get_pub_dh_key() const;
        std::string get_pub_sign_key() const;
        std::string compute_shared_key(const std::string &client_dh_pub, const std::string &client_sign_pub);
        void set_encrypted_cek(const std::string &cek);
        const CryptoPP::SecByteBlock &get_cek() const;

    private:
        std::shared_ptr<CryptoPP::DH> dh_;
        std::shared_ptr<CryptoPP::DH2> dh2_;
        KeyPair dh_key_;
        KeyPair sign_key_;
        std::shared_ptr<CryptoPP::SecByteBlock> shared_key_;
        std::shared_ptr<CryptoPP::SecByteBlock> cek_;
    };
}