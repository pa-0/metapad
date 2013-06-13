/**
 * @file cencoder.c
 * @brief Base64 encoding functions.
 * @note This is derived from the libb64 project, and has been placed in the public domain.
 * <a href="http://sourceforge.net/projects/libb64"> More info in project's page.</a>
 */

#include "include/cencode.h"

const int CHARS_PER_LINE = 500;
const char FILLER = '=';

/**
 * Initialize a base64_encodestate struct.
 *
 * @param[out] state_in Pointer to the struct to be initialized.
 */
void base64_init_encodestate(base64_encodestate* state_in)
{
	state_in->step = step_A;
	state_in->result = 0;
	state_in->stepcount = 0;
}

/**
 * Encode a byte.
 *
 * @param[in] value_in Value to encode.
 * @return Encoded value.
 */
char base64_encode_value(char value_in)
{
	static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	if (value_in > 63) return FILLER;
	return encoding[(int)value_in];
}

/**
 * Encode a text string.
 *
 * @param[in] plaintext_in Pointer to the string to encode.
 * @param[in] length_in Size of the string to encode.
 * @param[out] code_out Pointer to an array of bytes to contain the encoded bytes.
 * @param[in] state_in Pointer to the input string's base64_encodestate struct.
 * @return Size of the array of encoded bytes.
 */
int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in)
{
	const char* plainchar = plaintext_in;
	const char* const plaintextend = plaintext_in + length_in;
	char* codechar = code_out;
	char result;
	char fragment;

	result = state_in->result;

	switch (state_in->step)
	{
		while (1)
		{
	case step_A:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_A;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result = (fragment & 0x0fc) >> 2;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x003) << 4;
	case step_B:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_B;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0f0) >> 4;
			*codechar++ = base64_encode_value(result);
			result = (fragment & 0x00f) << 2;
	case step_C:
			if (plainchar == plaintextend)
			{
				state_in->result = result;
				state_in->step = step_C;
				return codechar - code_out;
			}
			fragment = *plainchar++;
			result |= (fragment & 0x0c0) >> 6;
			*codechar++ = base64_encode_value(result);
			result  = (fragment & 0x03f) >> 0;
			*codechar++ = base64_encode_value(result);

			++(state_in->stepcount);
			if (state_in->stepcount == CHARS_PER_LINE/4)
			{
				*codechar++ = '\n';
				state_in->stepcount = 0;
			}
		}
	}
	/* control should not reach here */
	return codechar - code_out;
}

/**
 * Encode the end of a block.
 *
 * @param[out] code_out Pointer to the array in which the encoded end will be stored.
 * @param[in] state_in Pointer to the input block's base64_encodestate struct.
 * @return Size of the encoded output.
 */
int base64_encode_blockend(char* code_out, base64_encodestate* state_in)
{
	char* codechar = code_out;

	switch (state_in->step)
	{
	case step_B:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = FILLER;
		*codechar++ = FILLER;
		break;
	case step_C:
		*codechar++ = base64_encode_value(state_in->result);
		*codechar++ = FILLER;
		break;
	case step_A:
		break;
	}
	//*codechar++ = '\n';

	return codechar - code_out;
}
