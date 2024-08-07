/*
 * Copyright (c) 2020 Belledonne Communications SARL.
 *
 * This file is part of bctoolbox.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <mbedtls/error.h>
#include <mbedtls/version.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <mbedtls/sha512.h>
#include <mbedtls/gcm.h>
#if MBEDTLS_VERSION_NUMBER >= 0x020B0000 // v2.11.0
#include <mbedtls/hkdf.h> // HKDF implemented in version 2.11.0 of mbedtls
#endif



#include "bctoolbox/crypto.hh"
#include "bctoolbox/crypto.h"
#include "bctoolbox/exception.hh"

#include <array>

namespace bctoolbox {

/*****************************************************************************/
/***                      Random Number Generation                         ***/
/*****************************************************************************/

/**
 * @brief Wrapper around mbedtls implementation
 **/
struct RNG::Impl {
	mbedtls_entropy_context entropy; /**< entropy context - store it to be able to free it */
	mbedtls_ctr_drbg_context ctr_drbg; /**< rng context */

	/**
	 * Implementation constructor
	 * Initialise the entropy and RNG context
	 */
	Impl() {
		mbedtls_entropy_init(&entropy);
		mbedtls_ctr_drbg_init(&ctr_drbg);
		if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0) != 0) {
			throw BCTBX_EXCEPTION << "RNG failure at creation: entropy source failure";
		}
	}
	~Impl() {
		mbedtls_ctr_drbg_free(&ctr_drbg);
		mbedtls_entropy_free(&entropy);
	}
};

/**
 * Constructor
 * Just instanciate an implementation
 */
RNG::RNG()
:pImpl(std::unique_ptr<RNG::Impl>(new RNG::Impl())) {};

/**
 * Destructor
 * Implementation is stored in a unique_ptr, nothing to do
 **/
RNG::~RNG()=default;

// Instanciate the class RNG context
std::unique_ptr<RNG::Impl> RNG::pImplClass = std::unique_ptr<RNG::Impl>(new RNG::Impl());


void RNG::randomize(uint8_t *buffer, size_t size) {
	int ret = mbedtls_ctr_drbg_random(&(pImpl->ctr_drbg), buffer, size);
	if ( ret != 0) {
		throw BCTBX_EXCEPTION << ((ret == MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG)?"RNG failure: Request too big":"RNG failure: entropy source failure");
	}
}

std::vector<uint8_t> RNG::randomize(const size_t size) {
	std::vector<uint8_t> buffer(size);
	int ret = mbedtls_ctr_drbg_random(&(pImpl->ctr_drbg), buffer.data(), size);
	if ( ret != 0) {
		throw BCTBX_EXCEPTION << ((ret == MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG)?"RNG failure: Request too big":"RNG failure: entropy source failure");
	}
	return buffer;
}

uint32_t RNG::randomize() {
	uint8_t buffer[4];
	randomize(buffer, 4);
	return (buffer[0]<<24) | (buffer[1]<<16) | (buffer[2]<<8) | buffer[3];
}

/*
 * class randomize functions
 * These use the class RNG context
 */
void RNG::cRandomize(uint8_t *buffer, size_t size) {
	int ret = mbedtls_ctr_drbg_random(&(pImplClass->ctr_drbg), buffer, size);
	if ( ret != 0) {
		throw BCTBX_EXCEPTION << ((ret == MBEDTLS_ERR_CTR_DRBG_REQUEST_TOO_BIG)?"RNG failure: Request too big":"RNG failure: entropy source failure");
	}
}

uint32_t RNG::cRandomize() {
	uint8_t buffer[4];
	cRandomize(buffer, 4);
	return (buffer[0]<<24) | (buffer[1]<<16) | (buffer[2]<<8) | buffer[3];
}

/*****************************************************************************/
/***                      Hash related function                            ***/
/*****************************************************************************/
/* HMAC templates */
/* HMAC must use a specialized template */
template <typename hashAlgo>
std::vector<uint8_t> HMAC(const std::vector<uint8_t> &key, const std::vector<uint8_t> &input) {
	/* if this template is instanciated the static_assert will fail but will give us an error message */
	static_assert(sizeof(hashAlgo) != sizeof(hashAlgo), "You must specialize HMAC function template");
	return std::vector<uint8_t>(0);
}

/* HMAC specialized template for SHA1 */
template <> std::vector<uint8_t> HMAC<SHA1>(const std::vector<uint8_t> &key, const std::vector<uint8_t> &input) {
	std::vector<uint8_t> hmacOutput(SHA1::ssize());
	mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), key.data(), key.size(), input.data(), input.size(), hmacOutput.data());
	return  hmacOutput;
}

/* HMAC specialized template for SHA256 */
template <> std::vector<uint8_t> HMAC<SHA256>(const std::vector<uint8_t> &key, const std::vector<uint8_t> &input) {
	std::vector<uint8_t> hmacOutput(SHA256::ssize());
	mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), key.data(), key.size(), input.data(), input.size(), hmacOutput.data());
	return  hmacOutput;
}

/* HMAC specialized template for SHA384 */
template <> std::vector<uint8_t> HMAC<SHA384>(const std::vector<uint8_t> &key, const std::vector<uint8_t> &input) {
	std::vector<uint8_t> hmacOutput(SHA384::ssize());
	mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA384), key.data(), key.size(), input.data(), input.size(), hmacOutput.data());
	return  hmacOutput;
}

/* HMAC specialized template for SHA512 */
template <> std::vector<uint8_t> HMAC<SHA512>(const std::vector<uint8_t> &key, const std::vector<uint8_t> &input) {
	std::vector<uint8_t> hmacOutput(SHA512::ssize());
	mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), key.data(), key.size(), input.data(), input.size(), hmacOutput.data());
	return  hmacOutput;
}


/* HKDF templates */
/* HKDF must use a specialized template */
template <typename hashAlgo>
std::vector<uint8_t> HKDF(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::vector<uint8_t> &info, size_t okmSize) {
	/* if this template is instanciated the static_assert will fail but will give us an error message */
	static_assert(sizeof(hashAlgo) != sizeof(hashAlgo), "You must specialize HKDF function template");
	return std::vector<uint8_t>(0);
}
template <typename hashAlgo>
std::vector<uint8_t> HKDF(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::string &info, size_t okmSize) {
	/* if this template is instanciated the static_assert will fail but will give us an error message */
	static_assert(sizeof(hashAlgo) != sizeof(hashAlgo), "You must specialize HKDF function template");

	return std::vector<uint8_t>(0);
}

#if MBEDTLS_VERSION_NUMBER >= 0x020B0000 // v2.11.0 - HKDF provided by mbedtls
/* HKDF specialized template for SHA256 */
template <> std::vector<uint8_t> HKDF<SHA256>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::vector<uint8_t> &info, size_t outputSize) {
	std::vector<uint8_t> okm(outputSize);
	if (mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), salt.data(), salt.size(), ikm.data(), ikm.size(), info.data(), info.size(), okm.data(), outputSize) != 0) {
		throw BCTBX_EXCEPTION<<"HKDF-SHA256 error";
	}
	return okm;
};
template <> std::vector<uint8_t> HKDF<SHA256>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::string &info, size_t outputSize) {
	std::vector<uint8_t> okm(outputSize);
	if (mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), salt.data(), salt.size(), ikm.data(), ikm.size(), reinterpret_cast<const unsigned char*>(info.data()), info.size(), okm.data(), outputSize) != 0) {
		throw BCTBX_EXCEPTION<<"HKDF-SHA256 error";
	}
	return okm;
};

/* HKDF specialized template for SHA384 */
template <> std::vector<uint8_t> HKDF<SHA384>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::vector<uint8_t> &info, size_t outputSize) {
	std::vector<uint8_t> okm(outputSize);
	if (mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA384), salt.data(), salt.size(), ikm.data(), ikm.size(), info.data(), info.size(), okm.data(), outputSize) != 0) {
		throw BCTBX_EXCEPTION<<"HKDF-SHA384 error";
	}
	return okm;
};
template <> std::vector<uint8_t> HKDF<SHA384>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::string &info, size_t outputSize) {
	std::vector<uint8_t> okm(outputSize);
	if (mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA384), salt.data(), salt.size(), ikm.data(), ikm.size(), reinterpret_cast<const unsigned char*>(info.data()), info.size(), okm.data(), outputSize) != 0) {
		throw BCTBX_EXCEPTION<<"HKDF-SHA384 error";
	}
	return okm;
};

/* HKDF specialized template for SHA512 */
template <> std::vector<uint8_t> HKDF<SHA512>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::vector<uint8_t> &info, size_t outputSize) {
	std::vector<uint8_t> okm(outputSize);
	if (mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), salt.data(), salt.size(), ikm.data(), ikm.size(), info.data(), info.size(), okm.data(), outputSize) != 0) {
		throw BCTBX_EXCEPTION<<"HKDF-SHA512 error";
	}
	return okm;
};
template <> std::vector<uint8_t> HKDF<SHA512>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::string &info, size_t outputSize) {
	std::vector<uint8_t> okm(outputSize);
	if (mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA512), salt.data(), salt.size(), ikm.data(), ikm.size(), reinterpret_cast<const unsigned char*>(info.data()), info.size(), okm.data(), outputSize) != 0) {
		throw BCTBX_EXCEPTION<<"HKDF-SHA512 error";
	}
	return okm;
};

#else // MBEDTLS_VERSION_NUMBER >= 0x020B0000 - HKDF not provided by mbedtls

/* generic implementation, of HKDF RFC-5869 - use this one if mbedtls does not provide it */
template <typename hashAlgo, typename infoType>
std::vector<uint8_t> HMAC_KDF(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const infoType &info, size_t outputSize) {
	// extraction
	auto prk = HMAC<hashAlgo>(salt, ikm);

	// expansion round 0
	std::vector<uint8_t> T(info.cbegin(), info.cend());
	T.push_back(0x01);
	auto output = HMAC<hashAlgo>(prk, T);
	output.reserve(outputSize);

	// successives expansion rounds
	size_t index = std::min(outputSize, hashAlgo::ssize());
	for(uint8_t i=0x02; index < outputSize; i++) {
		T.assign(output.cbegin()+(i-2)*hashAlgo::ssize(), output.cbegin()+(i-1)*hashAlgo::ssize());
		T.insert(T.end(), info.cbegin(), info.cend());
		T.push_back(i);
		auto round = HMAC<hashAlgo>(prk, T);
		output.insert(output.end(), round.cbegin(), round.cend());
		index += hashAlgo::ssize();
	}
	bctbx_clean(prk.data(), prk.size());
	bctbx_clean(T.data(), T.size());
	output.resize(outputSize); // dump what is not needed (each round compute hashAlgo::ssize() bytes of data, we may need only a fraction of the last round)
	return output;
}

/* HKDF specialized template for SHA256 */
template <> std::vector<uint8_t> HKDF<SHA256>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::vector<uint8_t> &info, size_t outputSize) {
	return HMAC_KDF<SHA256, std::vector<uint8_t>>(salt, ikm, info, outputSize);
};
template <> std::vector<uint8_t> HKDF<SHA256>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::string &info, size_t outputSize) {
	return HMAC_KDF<SHA256, std::string>(salt, ikm, info, outputSize);
};

/* HKDF specialized template for SHA384 */
template <> std::vector<uint8_t> HKDF<SHA384>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::vector<uint8_t> &info, size_t outputSize) {
	return HMAC_KDF<SHA384, std::vector<uint8_t>>(salt, ikm, info, outputSize);
};
template <> std::vector<uint8_t> HKDF<SHA384>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::string &info, size_t outputSize) {
	return HMAC_KDF<SHA384, std::string>(salt, ikm, info, outputSize);
};

/* HKDF specialized template for SHA512 */
template <> std::vector<uint8_t> HKDF<SHA512>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::vector<uint8_t> &info, size_t outputSize) {
	return HMAC_KDF<SHA512, std::vector<uint8_t>>(salt, ikm, info, outputSize);
};
template <> std::vector<uint8_t> HKDF<SHA512>(const std::vector<uint8_t> &salt, const std::vector<uint8_t> &ikm, const std::string &info, size_t outputSize) {
	return HMAC_KDF<SHA512, std::string>(salt, ikm, info, outputSize);
};

#endif // MBEDTLS_VERSION_NUMBER >= 0x020B0000

/*****************************************************************************/
/***                      Authenticated Encryption                         ***/
/*****************************************************************************/
/* AEAD template must be specialized */
template <typename AEADAlgo>
std::vector<uint8_t> AEADEncrypt(const std::vector<uint8_t> &key, const std::vector<uint8_t> IV, const std::vector<uint8_t> &plain, const std::vector<uint8_t> &AD,
		std::vector<uint8_t> &tag) {
	/* if this template is instanciated the static_assert will fail but will give us an error message with faulty type */
	static_assert(sizeof(AEADAlgo) != sizeof(AEADAlgo), "You must specialize AEADEncrypt function template");
	return std::vector<uint8_t>(0);
}

template <typename AEADAlgo>
bool AEADDecrypt(const std::vector<uint8_t> &key, const std::vector<uint8_t> &IV, const std::vector<uint8_t> &cipher, const std::vector<uint8_t> &AD,
		const std::vector<uint8_t> &tag, std::vector<uint8_t> &plain) {
	/* if this template is instanciated the static_assert will fail but will give us an error message with faulty type */
	static_assert(sizeof(AEADAlgo) != sizeof(AEADAlgo), "You must specialize AEADEncrypt function template");
	return false;
}

/* declare AEAD template specialisations : AES256-GCM with 128 bits auth tag*/
template <> std::vector<uint8_t> AEADEncrypt<AES256GCM128>(const std::vector<uint8_t> &key, const std::vector<uint8_t> IV, const std::vector<uint8_t> &plain, const std::vector<uint8_t> &AD,
		std::vector<uint8_t> &tag) {
	// check key size (could have use array but Windows won't compile templates with constexpr functions result as parameter)
	if (key.size() != AES256GCM128::keySize()) {
		throw BCTBX_EXCEPTION<<"AEADEncrypt: Bad input parameter, key is expected to be "<<AES256GCM128::keySize()<<" bytes but "<<key.size()<<" provided";
	}
	tag.resize(AES256GCM128::tagSize());

	mbedtls_gcm_context gcmContext;
	mbedtls_gcm_init(&gcmContext);

	auto ret = mbedtls_gcm_setkey(&gcmContext, MBEDTLS_CIPHER_ID_AES, key.data(), (unsigned int)key.size()*8); // key size in bits
	if (ret != 0) {
		mbedtls_gcm_free(&gcmContext);
		throw BCTBX_EXCEPTION<<"Unable to set key in AES_GCM context : return value "<<ret;
	}

	std::vector<uint8_t> cipher(plain.size()); // cipher size is the same than plain
	ret = mbedtls_gcm_crypt_and_tag(&gcmContext, MBEDTLS_GCM_ENCRYPT, plain.size(), IV.data(), IV.size(), AD.data(), AD.size(), plain.data(), cipher.data(), tag.size(), tag.data());
	mbedtls_gcm_free(&gcmContext);

	if (ret != 0) {
		throw BCTBX_EXCEPTION<<"Error during AES_GCM encryption : return value "<<ret;
	}
	return cipher;
}

template <> bool AEADDecrypt<AES256GCM128>(const std::vector<uint8_t> &key, const std::vector<uint8_t> &IV, const std::vector<uint8_t> &cipher, const std::vector<uint8_t> &AD,
		const std::vector<uint8_t> &tag, std::vector<uint8_t> &plain) {
	// check key and tag size (could have use array but Windows won't compile templates with constexpr functions result as parameter)
	if (key.size() != AES256GCM128::keySize()) {
		throw BCTBX_EXCEPTION<<"AEADDecrypt: Bad input parameter, key is expected to be "<<AES256GCM128::keySize()<<" bytes but "<<key.size()<<" provided";
	}
	if (tag.size() != AES256GCM128::tagSize()) {
		throw BCTBX_EXCEPTION<<"AEADDecrypt: Bad input parameter, tag is expected to be "<<AES256GCM128::tagSize()<<" bytes but "<<tag.size()<<" provided";
	}

	mbedtls_gcm_context gcmContext;
	mbedtls_gcm_init(&gcmContext);
	auto ret = mbedtls_gcm_setkey(&gcmContext, MBEDTLS_CIPHER_ID_AES, key.data(), (unsigned int)key.size()*8); // key size in bits
	if (ret != 0) {
		mbedtls_gcm_free(&gcmContext);
		throw BCTBX_EXCEPTION<<"Unable to set key in AES_GCM context : return value "<<ret;
	}

	plain.resize(cipher.size()); // plain is the same size than cipher
	ret = mbedtls_gcm_auth_decrypt(&gcmContext, cipher.size(), IV.data(), IV.size(), AD.data(), AD.size(), tag.data(), tag.size(), cipher.data(), plain.data());
	mbedtls_gcm_free(&gcmContext);

	if (ret == 0) {
		return true;
	}
	if (ret == MBEDTLS_ERR_GCM_AUTH_FAILED) {
		return false;
	}

	throw BCTBX_EXCEPTION<<"Error during AES_GCM decryption : return value "<<ret;
}


int AES_key_wrap(const std::vector<uint8_t> &plaintext, const std::vector<uint8_t> &key, std::vector<uint8_t> &ciphertext, AesId id){

	uint64_t t;
	size_t m = plaintext.size();	// Size of the plaintext
	size_t r;						// Size of the padded plaintext
	size_t n;						// Number of 64-bit blocks in the padded plaintext
	uint8_t input[16];				// Buffer of the AES input
	uint8_t B[16];					// Buffer of the AES output
	uint8_t *R = (uint8_t *) bctbx_malloc((m+8) * sizeof(uint8_t));	// An array of 8-bit registers

	/* Append padding */
	r = m;
	memcpy(R, plaintext.data(), r);
	while (r%8 != 0) {
		R[r] = 0;
		r++;
	}
	n = r / 8;

	/* Initialise variables */
	// Initialisation of A described in the RFC 5649 Section 3 : https://datatracker.ietf.org/doc/html/rfc5649#section-3
	uint8_t A[8];
	A[0] = 0xA6;
	A[1] = 0x59;
	A[2] = 0x59;
	A[3] = 0xA6;
	for (size_t i = 0 ; i < 4 ; i++) {
		A[4+i] = (m >> (3-i)*8) & 0xFF;
	}

	// Initialise AES context with the key
	mbedtls_aes_context context;
	mbedtls_aes_init(&context);
	switch (id) {
		case AesId::AES128 :
			mbedtls_aes_setkey_enc(&context, key.data(), 128);
			break;
		case AesId::AES192 :
			mbedtls_aes_setkey_enc(&context, key.data(), 192);
			break;
		case AesId::AES256 :
			mbedtls_aes_setkey_enc(&context, key.data(), 256);
			break;
		default :
			return BCTBX_ERROR_INVALID_INPUT_DATA;
	}

	/* Calculate intermediate values */
	if (r == 8) {
		// input = concat(A, R[0])
		memcpy(input, A, 8);
		memcpy(input+8, R, 8);

		// B = AES(key, input)
		mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_ENCRYPT, input, B);

		/* Output the results */
		ciphertext.assign(B, B+16);

		mbedtls_aes_free(&context);
		bctbx_free(R);

		return 0;
	}
	for (int j = 0 ; j <= 5 ; j++) {
		for (size_t i = 0 ; i < n ; i++) {
			// input = concat(A, R[i])
			memcpy(input, A, 8);
			memcpy(input+8, R+(i*8), 8);

			// B = AES(key, input)
			mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_ENCRYPT, input, B);

			// A = MSB(64, B) ^ t where t = (n*j)+i
			t = (n * j) + i+1;
			for (size_t k = 0 ; k < 8 ; k++) {
				A[k] = B[k] ^ ((t >> (7-k)*8) & 0xFF);
			}

			// R[i] = LSB(64, B)
			memcpy(R+(i*8), B+8, 8);
		}
	}

	/* Output the results */
	ciphertext.assign(A, A+8);
	ciphertext.insert(ciphertext.end(), R, R+r);

	mbedtls_aes_free(&context);
	bctbx_free(R);

	return 0;
}

int AES_key_unwrap(const std::vector<uint8_t> &ciphertext, const std::vector<uint8_t> &key, std::vector<uint8_t> &plaintext, AesId id){

	size_t n = (ciphertext.size() - 8) / 8;	// Number of 64-bit blocks of the padded plaintext
	size_t r = ciphertext.size() - 8;		// Size of the padded plaintext
	size_t m = 0;							// Size of the plaintext
	uint8_t input[16];						// Buffer of the AES input
	uint8_t B[16];							// Buffer of the AES output
	uint8_t *R = (uint8_t *) bctbx_malloc(r * sizeof(uint8_t));	// An array of 8-bit registers

	/* Initialise variables */
	uint8_t A[8];
	memcpy(A, ciphertext.data(), 8);
	memcpy(R, ciphertext.data()+8, r);

	// Initialise AES context with the key
	mbedtls_aes_context context;
	mbedtls_aes_init(&context);
	switch (id) {
		case AesId::AES128 :
			mbedtls_aes_setkey_dec(&context, key.data(), 128);
			break;
		case AesId::AES192 :
			mbedtls_aes_setkey_dec(&context, key.data(), 192);
			break;
		case AesId::AES256 :
			mbedtls_aes_setkey_dec(&context, key.data(), 256);
			break;
		default :
			mbedtls_aes_free(&context);
			bctbx_free(R);
			return BCTBX_ERROR_INVALID_INPUT_DATA;
	}

	/* Compute intermediate values */
	if (n == 1) {
		// input = ciphertext
		memcpy(input, ciphertext.data(), 16);

		// B = AES-1(K, input)
		mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_DECRYPT, input, B);

		// A = MSB(64, B)
		memcpy(A, B, 8);

		// R[i] = LSB(64, B)
		memcpy(R, B+8, 8);
	} else {
		for (int j = 5 ; j >= 0 ; j--) {
			for (size_t i = n ; i > 0 ; i--) {
				// input = concat(A ^ t, R[i]) where t = n*j+i
				uint64_t t = (n * j) + i;
				for (size_t k = 0 ; k < 8 ; k++) {
					input[k] = A[k] ^ ((t >> (7-k)*8) & 0xFF);
					input[k+8] = R[(i-1)*8+k];
				}

				// B = AES-1(K, input)
				mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_DECRYPT, input, B);

				// A = MSB(64, B)
				memcpy(A, B, 8);

				// R[i] = LSB(64, B)
				memcpy(R+((i-1)*8), B+8, 8);
			}
		}
	}

	/* Output the results */
	mbedtls_aes_free(&context);
	// AIV verification described in the RFC 5649 Section 3 : https://datatracker.ietf.org/doc/html/rfc5649#section-3
	if (!(A[0] == 0xA6 && A[1] == 0x59 && A[2] == 0x59 && A[3] == 0xA6)){
		bctbx_free(R);
		return BCTBX_ERROR_UNSPECIFIED_ERROR;
	}
	// m = concat(A[4], A[5], A[6], A[7])
	for (size_t i = 0 ; i < 4 ; i++) {
		m |= A[4+i] << ((3-i)*8);
	}
	if ((m <= 8*(n-1)) || (8*n <= m)) {
		bctbx_free(R);
		return BCTBX_ERROR_UNSPECIFIED_ERROR;
	}

	// Remove padding & Return plaintext
	plaintext.assign(R, R+m);

	bctbx_free(R);

	return 0;
}

} // namespace bctoolbox

/*** Random Number Generation: C API ***/
struct bctbx_rng_context_struct {
	std::unique_ptr<bctoolbox::RNG> m_rng; // encapsulate the RNG in a unique_ptr
};

bctbx_rng_context_t *bctbx_rng_context_new(void) {
	bctbx_rng_context_t *context = new bctbx_rng_context_struct();
	context->m_rng = std::unique_ptr<bctoolbox::RNG>(new bctoolbox::RNG());
	return context;
}

int32_t bctbx_rng_get(bctbx_rng_context_t *context, unsigned char*output, size_t output_length) {
	context->m_rng->randomize(output, output_length);
	return 0; // always return 0, in case of problem an exception is raised by randomize
}

void bctbx_rng_context_free(bctbx_rng_context_t *context) {
	context->m_rng=nullptr; // destroy the RNG
	delete(context);
}
