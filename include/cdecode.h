/**
 * @file cdecoder.h
 * @brief Base64 decoding protos.
 * @note This is derived from the libb64 project, and has been placed in the public domain.
 * <a href="http://sourceforge.net/projects/libb64"> More info in project's page.</a> 
 */

#ifndef BASE64_CDECODE_H
#define BASE64_CDECODE_H

/**
 * @enum base64_decodestep
 * @brief Specifies the possible decoding steps.
 */
typedef enum
{
	step_a, step_b, step_c, step_d
} base64_decodestep;

/**
 * @struct base64_decodestate
 * @brief Keeps track of the decoding step of a given byte.
 * @var plainchar Current byte value.
 * @var step Next decoding step to be performed on the byte.
 */
typedef struct
{
	base64_decodestep step;
	char plainchar;
} base64_decodestate;

void base64_init_decodestate(base64_decodestate* state_in);

int base64_decode_value(char value_in);

int base64_decode_block(const char* code_in, const int length_in, char* plaintext_out, base64_decodestate* state_in);

#endif /* BASE64_CDECODE_H */
