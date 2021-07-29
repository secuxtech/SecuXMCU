#include "bip39.h"
#include "bip39_english.h"
#include <stdlib.h>
#include "nrf_drv_rng.h"
#include "nrf_delay.h"
#include "nrf_crypto.h"
#include "nrf_crypto_hmac.h"
#include "nrf_crypto_hash.h"
#include "pkcs5.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

//-----------Shrink BIP39 --------------------------
extern const char *worddict;
typedef struct
{
	int word_index;
	const char *first_word;
} alpha_word_t;

alpha_word_t word_alpha_table[26] = {{0,NULL}};	// Only 25 entries due to no 'x' word.

void bip39_init(void)
{
//--- Initial word_alpha_table[]
	char lead_ch = 'a';
	const char *p_word = worddict;
	int w_index = 0;
	int table_index = 0;
	memset(word_alpha_table, 0, sizeof(word_alpha_table)); // Clear all pointers to NULL
	do
	{
		word_alpha_table[table_index].first_word = p_word;
		word_alpha_table[table_index].word_index = w_index;

		if (lead_ch == 'z') break;

		//--- Skip to next different leading word.
		while ((*p_word) == lead_ch)
		{
			p_word += (strlen(p_word) + 1);
			++w_index;
		} 
		++table_index;
		lead_ch = *p_word;
	} while(1);
}

const char *find_word_by_index(int index)
{
	const char *p_word = NULL;
	if (index >= 0)
	{
		for(int ii = 0; ii < 25; ++ii)
		{
			if(index<word_alpha_table[ii].word_index)	// Found the block after the hit block ?
			{
				int table_index = ii-1;
				int w_index = word_alpha_table[table_index].word_index;
				p_word = word_alpha_table[table_index].first_word;
				while(w_index<index)
				{
					p_word += (strlen(p_word) + 1);
					++w_index;
				} 
				break;	// Exit for loop.
			}
		}
	}
	return(p_word);
}

int find_index_by_word(const char *the_word, bool partial)
{
	int len = strlen(the_word);
	if ((len <= 0) || (len > 9)) return -1;	// Illegal word.

	char leading_ch = *the_word;
	int tbl_idx = leading_ch - 'a';
	if (leading_ch >= 'x') --tbl_idx;	// Reorder the index for the word behind 'x'

	const char *p_word = word_alpha_table[tbl_idx].first_word;
	int w_index = word_alpha_table[tbl_idx].word_index;
	int found_idx = -1;

	size_t cmp_size = partial ? strlen(the_word) : 10;

	while(*p_word == leading_ch)	// Still in the same leading block ?
	{
		if(strncmp(p_word, the_word, cmp_size) == 0)	// Found ?
		{
			found_idx = w_index;
			break;
		}
		p_word += (strlen(p_word)+1);
		++w_index;
	}
	return(found_idx);
}
//--------------------------------------------------

extern char salt_my[];
extern char my_word[24][10];
extern uint8_t recovery_method  ;

uint8_t random_vector_generate(uint8_t * p_buff, uint8_t size)
{
    ret_code_t ret_code = NRF_SUCCESS;
    ret_code = nrf_crypto_rng_vector_generate(p_buff, size);
    if (ret_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("nrf_crypto_rng_vector_generate failed. (%d)", ret_code);
    }
    return ret_code;
}

void generate_random_bits(uint8_t* random_buff, uint8_t RANDOM_BUFF_SIZE, nrf_crypto_hash_context_t hash_context){
	ret_code_t ret_code = NRF_SUCCESS;
	nrf_crypto_hash_sha256_digest_t     m_digest;
	const size_t m_digest_len = NRF_CRYPTO_HASH_SIZE_SHA256;
	size_t digest_len = m_digest_len;
	
	do
    {
		nrf_delay_ms(10);
		ret_code = random_vector_generate(random_buff, RANDOM_BUFF_SIZE);
	}
    while(ret_code != NRF_SUCCESS);

	//for(int i = 0; i < RANDOM_BUFF_SIZE; i++)		
	//		random_buff[i] = 7 * 16 + 15;

	nrf_crypto_hash_calculate(&hash_context,                     // Context or NULL to allocate internally
                              &g_nrf_crypto_hash_sha256_info,    // Info structure configures hash mode
                              random_buff,                       // Input buffer
                              RANDOM_BUFF_SIZE-1,                // Input size
                              m_digest,                          // Result buffer
                              &digest_len);                      // Result size
	
	random_buff[RANDOM_BUFF_SIZE-1] = m_digest[0];
}

void byte_to_11bit(uint8_t * p_input, uint8_t input_size, uint16_t * p_output, uint8_t output_size)
{		
		uint8_t index = 0, index2 = 0, digit = 0;
		int tmp = 0, mod;
	
		while ((index <= input_size)) {
				tmp <<= 8;
				tmp += p_input[index++];
				digit += 8;
				if (digit >= 11){
						p_output[index2++] = (tmp >> (digit - 11));
						mod = 1;
						for (int i = 0; i < digit - 11; i++) mod <<= 1;
						tmp %= mod;
						digit -= 11;
				}
		}
}


void index_to_recovery_word(uint16_t* index, char(* my_word)[10], size_t size){
	for(int i = 0; i < size; i++){
		const char *p_word = find_word_by_index(index[i]);
		strcpy(my_word[i], p_word);
	}
}

/*
TODO: NOSE mode!!!
void recovery_word_to_mnemonic(size_t size, char* mnemonic){
	for(int i = 0; i < size; i++){
		strcat(mnemonic,my_word[i]);
		NRF_LOG_INFO("%d:%s",i,&my_word[i]);
		if(i != size - 1)
			strcat(mnemonic," ");
	}
}

static nrf_crypto_hmac_context_t m_context;

void generate_seed(unsigned char * seed){
	
	#define RANDOM_BUFF_SIZE    33
	
	char mnemonic[240] = "";
	
	if (recovery_method == 18){		
	char mnemonic[180] = "";
	}
	
	if (recovery_method == 12){		
	char mnemonic[120] = "";
	}

	recovery_word_to_mnemonic(recovery_method, mnemonic);
	
		int  j;
		unsigned int i;
		uint8_t h_hash[64];
		uint8_t m_digest[64] = {0};
        size_t digest_len = sizeof(m_digest);			
		uint8_t counter[4]= {0x00,0x00,0x00,0x01};
		char salt[] = "mnemonic" ;
		
		if( frame == DEVICE_LOADING_FARME){
			//Set hash to ikv
			
			nrf_crypto_hmac_init(&m_context,&g_nrf_crypto_hmac_sha512_info,mnemonic,strlen(mnemonic));
			nrf_crypto_hmac_update(&m_context, "SecuX", 5);
			nrf_crypto_hmac_finalize(&m_context, h_hash, &digest_len);	
			
		}
		if( frame == SECURITY_HIDDEN_ENTER_RECOVERY_FRAME){
			//confirm hash from ikv
			nrf_crypto_hmac_init(&m_context,&g_nrf_crypto_hmac_sha512_info,mnemonic,strlen(mnemonic));
			nrf_crypto_hmac_update(&m_context, "SecuX", 5);
			nrf_crypto_hmac_finalize(&m_context, h_hash, &digest_len);
			
		}
		
		
			strcat(salt, salt_my);
			NRF_LOG_INFO("salt len:%d",strlen(salt));
			NRF_LOG_HEXDUMP_INFO(salt,strlen(salt)+1);
		
			nrf_crypto_hmac_init(&m_context,&g_nrf_crypto_hmac_sha512_info,mnemonic,strlen(mnemonic));
			nrf_crypto_hmac_update(&m_context, salt, strlen(salt));
			nrf_crypto_hmac_update(&m_context, counter, 4);
			nrf_crypto_hmac_finalize(&m_context, seed, &digest_len);			
			memcpy( m_digest, seed, 64 );			
        for( i = 1; i < 2048; i++ )
        {				
							nrf_crypto_hmac_init(&m_context,&g_nrf_crypto_hmac_sha512_info,mnemonic,strlen(mnemonic));					
							nrf_crypto_hmac_update(&m_context, m_digest, sizeof(m_digest));
							nrf_crypto_hmac_finalize(&m_context, m_digest, &digest_len);															            
						  for( j = 0; j < 64; j++ )
              seed[j] ^= m_digest[j];			
        }
}
*/


bool check_if_recovery_word_legal(char(* recovery_word)[10], uint8_t recovery_method)
{
	uint16_t index[24];
	uint8_t random_buff[33];
	for (int i = 0; i < recovery_method; i++)
    {
		int found_idx = find_index_by_word(recovery_word[i], false);

		if (found_idx >= 0)
			index[i] = found_idx;
		else 
			index[i] = 0;
	}
	NRF_LOG_FLUSH();

	uint8_t remain = 11;
	uint16_t pos = 0;
	
	if (recovery_method == 12)
    {	// ENT = 128 bits, 16 bytes, CS = 4 bits
		for (int i = 0; i < 16; i ++)
        {
			if (remain >= 8)
            {
				random_buff[i] = index[pos] >> (remain - 8);
			}
            else
            {
				random_buff[i] = (index[pos] & ((1 << remain) - 1)) << (8 - remain); 
				remain += 11;
				random_buff[i] += index[++pos] >> (remain - 8);
			}	
			remain -= 8;
		}			
	}
    else if (recovery_method == 18)
    {  // ENT = 192 bits, 24 bytes, CS = 6 bits
        for (int i = 0; i < 24; i ++)
        {
			if (remain >= 8)
            {
				random_buff[i] = index[pos] >> (remain - 8);
			}
            else
            {
				random_buff[i] = (index[pos] & ((1 << remain) - 1)) << (8 - remain); 
				remain += 11;
				random_buff[i] += index[++pos] >> (remain - 8);
			}	
			remain -= 8;
		}	
	}
    else if (recovery_method == 24)
    {  // ENT = 256 bits, 32 bytes, CS = 8 bits
		for (int i = 0; i < 32; i ++)
        {
			if (remain >= 8)
            {
				random_buff[i] = index[pos] >> (remain - 8);
			}
            else
            {
				random_buff[i] = (index[pos] & ((1 << remain) - 1)) << (8 - remain); 
				remain += 11;
				random_buff[i] += index[++pos] >> (remain - 8);
			}	
			remain -= 8;
		}	
	}

	nrf_crypto_hash_context_t   hash_context;
	nrf_crypto_hash_sha256_digest_t     m_digest;
	const size_t m_digest_len = NRF_CRYPTO_HASH_SIZE_SHA256;
	size_t digest_len = m_digest_len;
	
	if (recovery_method == 12)
		nrf_crypto_hash_calculate(&hash_context,                   // Context or NULL to allocate internally
								&g_nrf_crypto_hash_sha256_info,    // Info structure configures hash mode
								random_buff,                       // Input buffer
								16,                     		   // Input size
								m_digest,                          // Result buffer
								&digest_len);                      // Result size
	else if (recovery_method == 18)
		nrf_crypto_hash_calculate(&hash_context,                     // Context or NULL to allocate internally
								&g_nrf_crypto_hash_sha256_info,    // Info structure configures hash mode
								random_buff,                       // Input buffer
								24,                     			// Input size
								m_digest,                          // Result buffer
								&digest_len);                      // Result size
	else if (recovery_method == 24)
		nrf_crypto_hash_calculate(&hash_context,                     // Context or NULL to allocate internally
								&g_nrf_crypto_hash_sha256_info,    // Info structure configures hash mode
								random_buff,                       // Input buffer
								32,                     			// Input size
								m_digest,                          // Result buffer
								&digest_len);                      // Result size
	
	if(recovery_method == 12){
		if((index[recovery_method - 1]& 0x0f) == (m_digest[0] >> 4))
			return true;
		else
			return false;
	}
	else if(recovery_method == 18){
		if((index[recovery_method - 1]& 0x3f) == (m_digest[0] >> 2))
			return true;
		else
			return false;		
	}
	else if(recovery_method == 24){
		if((index[recovery_method - 1]& 0xff) == m_digest[0])
			return true;
		else
			return false;		
	}
	return false;
}
