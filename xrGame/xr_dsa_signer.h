#pragma once 
#include "..\Components\crypto\crypto.h"

using sha_process_yielder = fastdelegate::FastDelegate1<long>;
using dsa = crypto::xr_dsa;

class xr_dsa_signer
{
public:
	xr_dsa_signer(u8 const p_number[dsa::public_key_length], u8 const q_number[dsa::private_key_length], u8 const g_number[dsa::public_key_length]);
	~xr_dsa_signer();

	shared_str const sign(u8 const* data, u32 data_size);
	shared_str const sign_mt(u8 const* data, u32 data_size, sha_process_yielder yielder);

protected:
	dsa::private_key_t m_private_key;

private:
	xr_dsa_signer() : m_dsa(NULL, NULL, NULL) {};

	dsa m_dsa;
	crypto::xr_sha256 m_sha;

}; //xr_dsa_signer

char const* current_time(string64& dest_time);
