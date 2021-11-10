/****************************************************************************/
/*                                                                          */
/*   metapad 3.6+                                                           */
/*                                                                          */
/*   Copyright (C) 2021 SoBiT Corp                                          */
/*   Copyright (C) 2013 Mario Rugiero                                       */
/*   Copyright (C) 1999-2011 Alexander Davidson                             */
/*                                                                          */
/*   This program is free software: you can redistribute it and/or modify   */
/*   it under the terms of the GNU General Public License as published by   */
/*   the Free Software Foundation, either version 3 of the License, or      */
/*   (at your option) any later version.                                    */
/*                                                                          */
/*   This program is distributed in the hope that it will be useful,        */
/*   but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          */
/*   GNU General Public License for more details.                           */
/*                                                                          */
/*   You should have received a copy of the GNU General Public License      */
/*   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */
/*                                                                          */
/****************************************************************************/


#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <tchar.h>

#ifdef BUILD_METAPAD_UNICODE
#include <wchar.h>
#endif

#include "include/strings.h"
#include "include/macros.h"

extern HWND hwnd;

/* Converts an array bin of binary data, of size size bytes, into a string hex
 * of hexa characters of size [(2*size) + 1] ANSI or wide characters, depending
 * on definition of UNICODE macro. Null terminates the string.
 *
 * @param[in] bin The array of bytes to encode.
 * @param[in] size Size of the array to convert in bytes.
 * @param[out] hex Pointer to a string big enough to contain the encoded data.
 * @return Nothing.
 */
void BinToHex( const LPBYTE bin, DWORD size, TCHAR* hex )
{
	DWORD i;
	TCHAR HexIndex[] = _T("0123456789ABCDEF");
	for( i = 0; i < size; ++i ){
		hex[2*i] = HexIndex[(bin[i] >> 4) & 0x0F];
		hex[(2*i)+1] = HexIndex[bin[i] & 0x0F];
	}
	hex[2*i] = _T('\0');
} 

/**
 * Decodes a single character (representing only 4 bits) to binary.
 *
 * @param[in] hexchar The character to decode.
 * @return The numeric value hexchar represents. If this is 255, it means
 * hexchar is an invalid hexadecimal character.
 * @note Case insensitive.
 */
static BYTE HexBaseToBin( TCHAR hexchar ) {
	switch( hexchar ){
		case _T('0'): return 0;
		case _T('1'): return 1;
		case _T('2'): return 2;
		case _T('3'): return 3;
		case _T('4'): return 4;
		case _T('5'): return 5;
		case _T('6'): return 6;
		case _T('7'): return 7;
		case _T('8'): return 8;
		case _T('9'): return 9;
		case _T('A'):
		case _T('a'): return 10;
		case _T('B'):
		case _T('b'): return 11;
		case _T('C'):
		case _T('c'): return 12;
		case _T('D'):
		case _T('d'): return 13;
		case _T('E'):
		case _T('e'): return 14;
		case _T('F'):
		case _T('f'): return 15;
		default: return 255;
	}
}

static unsigned short decLut[] = {
	0x803f, 0x803f, 0x81ff, 0x817f,  0x80ff, 0x80ff, 0x80ff, 0x80bf,  0x80bf, 0x80bf, 0x80bf, 0x7cbf,  0x80bf, 0x80bf, 0x80bf, 0x7ebf,
	0x6840, 0x6a41, 0x6c42, 0x6e43,  0x7044, 0x7245, 0x7446, 0x7647,  0x7848, 0x7a49, 0x807f, 0x807f,  0x807f, 0xfe7f, 0x807f, 0x807f,
	0x807f, 0x004a, 0x024b, 0x044c,  0x064d, 0x080e, 0x0a0f, 0x0c10,  0x0e11, 0x1012, 0x1213, 0x1414,  0x1615, 0x1816, 0x1a17, 0x1c18,
	0x1e19, 0x201a, 0x221b, 0x241c,  0x261d, 0x281e, 0x2a1f, 0x2c20,  0x2e21, 0x3022, 0x3223, 0x803f,  0x803f, 0x803f, 0x803f, 0x803f,
	0x80ff, 0x344a, 0x360b, 0x388c,  0x3a0d, 0x3c0e, 0x3e0f, 0x40d0,  0x4211, 0x4412, 0x4613, 0x4814,  0x4a15, 0x4c16, 0x4e17, 0x5118,
	0x5219, 0x541a, 0x561b, 0x581c,  0x5a1d, 0x5c1e, 0x5e1f, 0x6020,  0x6221, 0x6422, 0x6623, 0x803f,  0x803f, 0x803f, 0x803f, 0x817f
};
static unsigned short encLut[] = {
	0x4130, 0x4231, 0x4332, 0x4433,  0x4534, 0x4635, 0x4736, 0x4837,  0x4938, 0x4a39, 0x4b61, 0x4c62,  0x4d63, 0x4e64, 0x4f65, 0x5066,
	0x5167, 0x5268, 0x5369, 0x546a,  0x556b, 0x566c, 0x576d, 0x586e,  0x596f, 0x5a70, 0x6171, 0x6272,  0x6373, 0x6474, 0x6575, 0x6676,
	0x6777, 0x6878, 0x6979, 0x6a7a,  0x6b20, 0x6c20, 0x6d20, 0x6e20,  0x6f20, 0x7020, 0x7120, 0x7220,  0x7320, 0x7420, 0x7520, 0x7620,
	0x7720, 0x7820, 0x7920, 0x7a20,  0x3020, 0x3120, 0x3220, 0x3320,  0x3420, 0x3520, 0x3620, 0x3720,  0x3820, 0x3920, 0x2b20, 0x2f20,
	0x3d20
};

/**
 * Decodes a string of ANSI or wide characters, of given [base], to the binary data it represents.
 * Valid bases: 2 - 36, 64.
 *
 * @param[in] code Null terminated string to decode.
 * @param[out] bin Pointer to the array of bytes where the data will be stored. Not modified if any error occurred.
 * @param[in] base Input base
 * @param[in] len Number of characters of [code] to decode. If negative, decode until null terminator.
 * @param[in] extractMode 0=invalid characters result in an error  1=treat any invalid character as a null - stop decoding and return success  2=silently ignore any invalid characters
 * @param[in] alignMode 0=enforce - input string must be a multiple of log(256)/log([base])  1=append leading zeros  2=append trailing zeros
 * @param[in] showError if true, shows an error message when returning an error code
 * @param[in,out] if non-null, returns the position where decoding of [code] finished or was stopped due to error
 * @return Size of decoded array or: -1=invalid characters  -2=alignment error (input length not an expected exact multiple),  -3=invalid base
 */
DWORD DecodeBase( LPCTSTR code, LPBYTE bin, BYTE base, INT len, BYTE extractMode, BYTE alignMode, BOOL showError, LPCTSTR* end ) {
	DWORD i, ct;
	BYTE j, k = 0, v, w, ls = 0, la = 0x3f, vs = 0, mi = 0, mc = 0;
	unsigned short* lut = decLut - 0x20;
	if (base < 2 || base > 64 || (w = ((lut[base+0x20] >> 6) & 7) + 1) < 2) return -3;
	if (!SCNUL(code)[0]) return 0;
	if (len < 0) len = 0x7fffffff;
	if (base == 64) { ls = 9; la = 0x7f; }
	if (base <= 32) vs = ((lut[base+0x5f] >> 6) & 7);
	if (!vs && base < 64)
		for(mi = base, i = w-2; i; i--, mi *= base);
	for (i = 0, ct = 0; len && code[i]; len--, i++, ct++)
		if ((v = (BYTE)code[i]) < 0x20 || v >= 0x80 || ((v = (*(lut+v) >> ls) & la) >= base && v != 0x7f)) {
			if (extractMode) {
				if (extractMode == 1) break;
				ct--;
			} else {
				len = -2;
				break;
			}
		}
	if (end) *end = code + i;
	if (len >= 0 && (j = ct % w)){
		if (alignMode) {
			j = w - j;
			if (alignMode == 1 && base != 64){ 
				k = j;
				if (!vs)
					for(mc = mi, v = j; v; v--, mc /= base);
			} else {
				ct += j;
				j = 0;
			}
		} else len = -1;
	}
	if (len < 0) {
		if (showError)
			ERROROUT(len == -1 ? _T("Invalid code string length!") : _T("Invalid code characters!"));
		return len;
	}
	switch (base){
	case 64:
		for (bin--, len = ct, k = 0; ct; ct--, k=(++k)%4, w = 0) {
			if (!k) j = 1;
			if (!i) v = 0;
			else {
				i--;
				if ((v = (BYTE)*(code++)) < 0x20 || v >= 0x80 || (v=(*(lut+v) >> 9)) >= 64) {
					if (v != 0x7f) { ct++; k--; }
					continue;
				}
			}
			if (j && k) *++bin = 0;
			j = 0;
			switch(k) {
				case 0: 					*++bin = (v << 2);	break;
				case 1: *bin |= (v >> 4); 	*++bin = (v << 4);	break;
				case 2: *bin |= (v >> 2); 	*++bin = (v << 6);	break;
				case 3: *bin |= v; 							 	break;
			}
		}
		return (len*3)/4;
	default:
		if (!k) bin--;
		for (len = ct; ct; ct--, k=(++k)%w) {
			if (!i) v = 0;
			else {
				i--;
				if ((v = (BYTE)*(code++)) < 0x20 || v >= 0x80 || (v=(*(lut+v) & 0x3f)) >= base) {
					ct++; k--; continue;
				}
			}
			if (j) { j = 0; *bin = 0; }
			if (!k) {
				*(++bin) = 0;
				mc = mi;
			}
			if (vs){
				*bin <<= vs;
				*bin |= v;
			} else {
				*bin += v * mc;
				mc /= base;
			}
		}
		return (len+w-1)/w;
	}
}

/**
 * Encodes binary data to a string of ANSI or wide characters representing an encoding of given [base].
 * Valid bases: 2 - 36, 64.
 *
 * @param[in] bin The array of bytes to encode.
 * @param[out] code Pointer to a string big enough to contain the encoded data (including a null terminator).
 * @param[in] base Output base
 * @param[in] len Number of bytes of [bin] to encode. If negative, encode until a zero byte.
 * @param[in,out] if non-null, returns the position where encoding of [bin] finished
 * @return Size in characters of output string (excluding the null terminator) or: -3=invalid base
 */
DWORD EncodeBase( LPBYTE bin, LPTSTR code, BYTE base, INT len, LPBYTE* end ) {
	DWORD ct = 0, v;
	BYTE k, w, vs = 0, ma = base-1;
	unsigned short *lut = decLut - 0x20, *elut = encLut;
	if (base < 2 || base > 64 || (w = ((lut[base+0x20] >> 6) & 7) + 1) < 2) return -3;
	if (!bin) len = 0;
	if (base <= 32) vs = ((lut[base+0x5f] >> 6) & 7);
	if (len < 0)
		for (len = -1; bin[++len]; );
	switch (base) {
	case 64:
		for (k = 0, w = 0, code-=4; len; k=(++k)%4, len--, v >>= 6) {
			if (!k) {
				for (v = 0, k = 3; k; k--, len--){
					if (len)
						v |= (*bin++ << (8 * (3-k)));
				}
				code+=4; len+=3; ct+=4;
			}
			*code++ = (TCHAR)(elut[v & 0x3f] >> 8); break;
		}
		for (; k; k=(++k)%4)
			*code++ = (TCHAR)(elut[64] >> 8);
		break;
	default:
		for (k = 0, code-=(w+1); len || k; k=(++k)%w) {
			if (!k) { len--; code+=w; ct+=w; v=*bin++; }
			if (vs) {
				*(code+(w-k)) = (TCHAR)(elut[v & ma] & 0x7f);
				v >>= vs;
			} else {
				*(code+(w-k)) = (TCHAR)(elut[v % base] & 0x7f);
				v /= base;
			}
		}
		break;
	}
	*(code+w+1) = _T('\0');
	if (end) *end = bin;
	return ct;
}

DWORD HexToBin( LPCTSTR hex, LPBYTE bin ){
	return DecodeBase(hex, bin, -1, 16, 0, 0, TRUE, NULL);
}

