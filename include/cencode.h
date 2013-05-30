/**
 * @file cencoder.h
 * @brief Base64 encoding protos.
 * @note This is derived from the libb64 project, and has been placed in the public domain.
 * <a href="http://sourceforge.net/projects/libb64"> More info in project's page.</a> 
 */

#ifndef BASE64_CENCODE_H
#define BASE64_CENCODE_H

/**
 * @enum base64_encodestep
 * @brief Specifies the possible encoding steps.
 */
typedef enum
{
	step_A, step_B, step_C
} base64_encodestep;

/**
 * @struct base64_encodestate
 * @brief Keeps track of a given byte encoding state.
 * @var result Current value of the byte.
 * @var step Next step of encoding to be performed on the byte.
 * @var stepcount Number of encoding steps performed at the moment.
 */
typedef struct
{
	base64_encodestep step;
	char result;
	int stepcount;
} base64_encodestate;

void base64_init_encodestate(base64_encodestate* state_in);

char base64_encode_value(char value_in);

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in);

int base64_encode_blockend(char* code_out, base64_encodestate* state_in);

#endif /* BASE64_CENCODE_H */
