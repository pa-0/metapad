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

#ifdef UNICODE
#include <wchar.h>
#endif

#include "include/resource.h"
#include "include/strings.h"
#include "include/macros.h"

extern HWND hwnd;
extern HANDLE globalHeap;

#define NUMBOMS 3
static const BYTE bomLut[NUMBOMS][4] = {{0x3,0xEF,0xBB,0xBF}, {0x2,0xFF,0xFE}, {0x2,0xFE,0xFF}};

static const unsigned short decLut[] = {
	0xfe3f, 0x803f, 0x81ff, 0x817f,  0x80ff, 0x80ff, 0x80ff, 0x80bf,  0x80bf, 0x80bf, 0x80bf, 0x7cbf,  0x80bf, 0x80bf, 0x80bf, 0x7ebf,
	0x6840, 0x6a41, 0x6c42, 0x6e43,  0x7044, 0x7245, 0x7446, 0x7647,  0x7848, 0x7a49, 0x807f, 0x807f,  0x807f, 0xfe7f, 0x807f, 0x807f,
	0x807f, 0x004a, 0x024b, 0x044c,  0x064d, 0x080e, 0x0a0f, 0x0c10,  0x0e11, 0x1012, 0x1213, 0x1414,  0x1615, 0x1816, 0x1a17, 0x1c18,
	0x1e19, 0x201a, 0x221b, 0x241c,  0x261d, 0x281e, 0x2a1f, 0x2c20,  0x2e21, 0x3022, 0x3223, 0x803f,  0x803f, 0x803f, 0x803f, 0x803f,
	0x80ff, 0x344a, 0x360b, 0x388c,  0x3a0d, 0x3c0e, 0x3e0f, 0x40d0,  0x4211, 0x4412, 0x4613, 0x4814,  0x4a15, 0x4c16, 0x4e17, 0x5118,
	0x5219, 0x541a, 0x561b, 0x581c,  0x5a1d, 0x5c1e, 0x5e1f, 0x6020,  0x6221, 0x6422, 0x6623, 0x803f,  0x803f, 0x803f, 0x803f, 0x817f
};
static const unsigned short encLut[] = {
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
 * @param[in] extractMode 0=invalid characters result in an error  1=treat any invalid character as a null - stop decoding and return success  2=silently ignore any invalid characters  3=treat any invalid characters as zeros
 * @param[in] alignMode 0=enforce - input string must be a multiple of log(256)/log([base])  1=append leading zeros  2=append trailing zeros (no effect for base64)
 * @param[in] showError if true, shows an error message when returning an error code
 * @param[in,out] if non-null, returns the position where decoding of [code] finished or was stopped due to error
 * @return Size of decoded array or: -1=invalid characters  -2=alignment error (input length not an expected exact multiple),  -3=invalid base
 */
INT DecodeBase( BYTE base, LPCTSTR code, LPBYTE bin, INT len, BYTE extractMode, BYTE alignMode, BOOL showError, LPCTSTR* end ) {
	DWORD i, ct;
	BYTE j, k = 0, v, w, ls = 0, la = 0x3f, vs = 0, mi = 0, mc = 0;
	unsigned const short* lut = decLut - 0x20;
	if (base < 2 || base > 64 || (w = ((lut[base+0x20] >> 6) & 7) + 1) < 2) return -3;
	if (!SCNUL(code)[0]) return 0;
	if (len < 0) len = 0x7fffffff;
	if (base == 64) { ls = 9; la = 0x7f; }
	if (base <= 32) vs = ((lut[base+0x5f] >> 6) & 7);
	if (!vs && base < 64)
		for(mi = base, i = w-2; i; i--, mi *= base);
	for (i = 0, ct = 0; len > 0 && code[i]; len--, i++, ct++)
		if ((v = (BYTE)code[i]) < 0x20) {
			ct--; continue;
		} else if (v >= 0x80 || ((v = (*(lut+v) >> ls) & la) >= base && v != 0x7f)) {
			switch (extractMode){
				case 0: len = 0; break;
				case 1: i--; len = 1;
				case 2: ct--; break;
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
		} else len = -2;
	}
	if (len < 0) {
		if (showError)
			ERROROUT(len == -1 ? _T("Invalid code string length!") : _T("Invalid code characters!"));
		return len;
	}
	switch (base){
	case 64:
		for (len = 0, k = 0; i && ct; ct--, i--, k=(++k)%4) {
			if ((v = (BYTE)*(code++)) < 0x20) {
				ct++; k--; continue;
			} else if (v >= 0x80 || (v=(*(lut+v) >> 9)) >= 64) {
				if (extractMode < 3 || v == 0x7f) {
					if (v != 0x7f) { ct++; k--; }
					continue;
				}
				v = 0;
			}
			switch(k) {
				case 1: *bin++ = (w << 2) | (v >> 4); break;
				case 2: *bin++ = (w << 4) | (v >> 2); break;
				case 3: *bin++ = (w << 6) | v; break;
			}
			if (k) len++;
			w = v;
		}
		return len;
	default:
		if (!k) bin--;
		for (len = ct; ct; ct--, k=(++k)%w) {
			if (!i) v = 0;
			else {
				i--;
				if ((v = (BYTE)*(code++)) < 0x20) {
					ct++; k--; continue;
				} else if (v >= 0x80 || (v=(*(lut+v) & 0x3f)) >= base) {
					if (extractMode < 3) {
						ct++; k--; continue;
					}
					v = 0;
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
INT EncodeBase( BYTE base, LPBYTE bin, LPTSTR code, INT len, LPBYTE* end ) {
	DWORD i, ct = 0, v;
	BYTE k, w, vs = 0, ma = base-1;
	unsigned const short *lut = decLut - 0x20, *elut = encLut;
	if (base < 2 || base > 64 || (w = ((lut[base+0x20] >> 6) & 7) + 1) < 2) return -3;
	if (!bin) len = 0;
	if (base <= 32) vs = ((lut[base+0x5f] >> 6) & 7);
	if (len < 0)
		for (len = -1; bin[++len]; );
	switch (base) {
	case 64:
		ct = i = (((LONGLONG)len << 2) + 2) / 3;
		for (code--, k = 0, w = 0; i; i--, k=(++k)%4, v <<= 6) {
			if (!k) {
				for (v = 0, k = 3; len && k; k--, len--)
					v |= (*bin++ << (8 * (k-1)));
				k = 0;
			}
			*++code = (TCHAR)(elut[(v >> 18) & 0x3f] >> 8);
		}
		for (; k; k=(++k)%4, ct++)
			*++code = (TCHAR)(elut[64] >> 8);
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



/**
 * Reverse byte pairs.
 *
 * @param[in] buffer Pointer to the start of the data to be reversed.
 * @param[in] len Length of the data to be reversed.
 */
void ReverseBytes(LPBYTE buffer, ULONG len) {
	BYTE temp;
	for (len /= 2; len; len--){
		temp = *buffer;
		*buffer++ = *(buffer+1);
		*buffer++ = temp;
	}
}




/**
 * Check if a buffer starts with a byte order mark.
 * If it does, the buffer is also advanced past the BOM and the given length value is decremented appropriately.
 *
 * @param[in,out] pb Pointer to the starting byte to check.
 * @param[in,out] pbLen Length of input.
 * @return One of ID_ENC_* enums if a BOM is found, else ID_ENC_UNKNOWN
 */
WORD CheckBOM(LPBYTE *pb, DWORD* pbLen) {
	DWORD i, j;
	for (i = 0; i < NUMBOMS; i++){
		if(pb && pbLen && *pb && *pbLen >= (j = bomLut[i][0]) && !memcmp(*pb, bomLut[i]+1, j)){
			*pb += j;
			*pbLen -= j;
			return ID_ENC_UTF8 + (WORD)i;
		}
	}
	return ID_ENC_UNKNOWN;
}

BOOL IsTextUTF8(LPBYTE buf){
	BOOL yes = 0;
	while (*buf) {
		if (*buf < 0x80) buf++;
		else if ((*((PWORD)buf) & 0xc0e0) == 0x80c0){ buf+=2; yes = 1; }
		else if ((*((PDWORD)buf) & 0xc0c0f0) == 0x8080e0){ buf+=3; yes = 1; }
		else if ((*((PDWORD)buf) & 0xc0c0c0f8) == 0x808080f0){ buf+=4; yes = 1; }	//valid UTF-8 outside Unicode range - Win32 cannot handle this - show truncation warning!
		else return 0;
	}
	return yes;
}

WORD GetLineFmt(LPCTSTR sz, DWORD len, DWORD* nCR, DWORD* nLF, DWORD* nStrays, BOOL* binary){
	DWORD i, crlf = 1, cr = 0, lf = 0;
	TCHAR cc, cp = _T('\0');
	if (binary) *binary = FALSE;
	if (nStrays) *nStrays = 0;
	for (i = 0; (len && i++ < len) || (!len && *sz); cp = cc) {
		if ((cc = *sz++) == _T('\r')){
			cr++;
			if ((nStrays || crlf) && *sz != _T('\n')) { crlf = 0; (*nStrays)++; }
		} else if (cc == _T('\n')) {
			lf++;
			if ((nStrays || crlf) && cp != _T('\r')) { crlf = 0; (*nStrays)++; }
		}
		if (cr && lf && !crlf && !nStrays) return ID_LFMT_MIXED;
		if (len && binary && cc == '\0') *binary = TRUE;
	}
	if (nCR) *nCR = cr;
	if (nLF) *nLF = lf;
	if (cr && lf && !crlf) return ID_LFMT_MIXED;
	else if (crlf) return ID_LFMT_DOS;
	else if (cr) return ID_LFMT_MAC;
	else return ID_LFMT_UNIX;
}

void NormalizeBinary(LPTSTR sz, DWORD len){
	while (len--){
		if (*sz == _T('\0'))
#ifdef UNICODE
			*sz = _T('\x2400');
#else
			*sz = _T(' ');
#endif
	}
}
void RestoreBinary(LPTSTR sz, DWORD len){
#ifdef UNICODE
	if (!len) len = lstrlen(sz);
	while (len--){
		if (*sz == _T('\x2400'))
			*sz = _T('\0');
	}
#endif
}

#ifdef USE_RICH_EDIT
void NormalizeLineFmt(LPTSTR* sz, DWORD len, WORD lfmt, DWORD nCR, DWORD nLF, DWORD nStrays){
	LPTSTR odst, dst, osz;
	if (lfmt != ID_LFMT_MIXED || !sz || !*sz) return;
	if (!len) len = lstrlen(*sz);
	osz = *sz;
	odst = dst = (LPTSTR)HeapAlloc(globalHeap, 0, (len+nLF+1) * sizeof(TCHAR));
	for ( ; len--; (*sz)++) {
		if (**sz == _T('\n'))
			*dst++ = _T(' ');
		*dst++ = **sz;
	}
	*dst = _T('\0');
	FREE(osz);
	*sz = odst;
}
void RestoreLineFmt(LPTSTR* sz, DWORD len, WORD lfmt, DWORD lines){
	LPTSTR odst, dst, osz;
	TCHAR cc, cp = _T('\0');
	if (lfmt == ID_LFMT_MAC || !sz || !*sz) return;
	if (!len) len = lstrlen(*sz);
	dst = osz = *sz;
	if (lfmt == ID_LFMT_DOS) odst, dst = (LPTSTR)HeapAlloc(globalHeap, 0, (len+lines+1) * sizeof(TCHAR));
	for ( ; len--; dst++, cp = cc) {
		cc = *dst = *(*sz)++;
		if (cc == _T('\r')) {
			if (lfmt == ID_LFMT_MIXED && cp == _T(' ')) *--dst = _T('\n');
			else if (lfmt == ID_LFMT_UNIX) *dst = _T('\n');
			else if (lfmt == ID_LFMT_DOS) *++dst = _T('\n');
		}
	}
	*dst = _T('\0');
	if (lfmt == ID_LFMT_DOS) {
		FREE(osz);
		*sz = odst;
	}
}
#else
void NormalizeLineFmt(LPTSTR* sz, DWORD len, WORD lfmt, DWORD nCR, DWORD nLF, DWORD nStrays){
	LPTSTR odst, dst, osz;
	TCHAR cc, cp = _T('\0');
	if (lfmt == ID_LFMT_MIXED || !nStrays || !sz || !*sz) return;
	if (!len) len = lstrlen(*sz);
	osz = *sz;
	odst = dst = (LPTSTR)HeapAlloc(globalHeap, 0, (len+nStrays+1) * sizeof(TCHAR));
	for ( ; len--; cp = cc) {
		if ((cc = *(*sz)++) == _T('\r') && **sz != _T('\n')) {
			*dst++ = _T('\r'); *dst++ = _T('\n');
			continue;
		} else if (cc == _T('\n') && cp != _T('\r'))
			*dst++ = _T('\r');
		*dst++ = cc;
	}
	*dst = _T('\0');
	FREE(osz);
	*sz = odst;
}
void RestoreLineFmt(LPTSTR* sz, DWORD len, WORD lfmt, DWORD lines){
	LPTSTR dst = *sz;
	if ((lfmt != ID_LFMT_UNIX && lfmt != ID_LFMT_MAC) || !sz || !*sz) return;
	if (!len) len = lstrlen(*sz);
	for ( ; len--; dst++) {
		*dst = *(*sz)++;
		if (*dst == _T('\r')) {
			if (lfmt == ID_LFMT_UNIX) *dst = _T('\n');
			*sz++;
		}
	}
	*dst = _T('\0');
}
#endif