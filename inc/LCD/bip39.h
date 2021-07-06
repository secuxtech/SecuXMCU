#ifndef BIP39_H
#define BIP39_H
#include<stdint.h>
#include<string.h>
#include "nrf_crypto_hash.h"

//generate random bit array
uint8_t random_vector_generate(uint8_t * p_buff, uint8_t size);
//generate 256 bit random data + 8 bit hash data
void generate_random_bits(uint8_t* random_buff, uint8_t RANDOM_BUFF_SIZE, nrf_crypto_hash_context_t hash_context);
//change 8*33 bits to 11*24 bits
void byte_to_11bit(uint8_t * p_input, uint8_t input_size, uint16_t * p_output, uint8_t output_size);
//create words
void index_to_recovery_word(uint16_t* index, char(* my_word)[10], size_t size);
//create mnemonic
void recovery_word_to_mnemonic(size_t size, char* mnemonic);
//get words & seed
void generate_seed(unsigned char * seed);

bool check_if_recovery_word_legal(char(* my_word)[10], uint8_t recovery_method);

const char *find_word_by_index(int index);
int find_index_by_word(const char *the_word, bool partial);

#endif
