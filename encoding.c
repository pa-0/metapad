/****************************************************************************/
/*                                                                          */
/*   metapad 3.6+                                                           */
/*                                                                          */
/*   Copyright (C) 2021-2024 SoBiT Corp                                     */
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

#include "include/globals.h"
#include "include/resource.h"
#include "include/macros.h"
#include "include/utils.h"

extern HWND hwnd;
extern HANDLE globalHeap;
void ReportLastError(void);
LPCTSTR GetString(WORD uID);
LPCTSTR GetStringEx(WORD uID, WORD total, const LPSTR dict, WORD* dictidx, WORD* dictofs, LPTSTR dictcache, WORD* ofspop, LPCTSTR def);

static const BYTE bomLut[][4] = {{0x3,0xEF,0xBB,0xBF}, {0x2,0xFF,0xFE}, {0x2,0xFE,0xFF}};
static const WORD bomIdx[] = {FC_ENC_UTF8, FC_ENC_UTF16, FC_ENC_UTF16BE};

static const CHAR knownCPT[] = "Local DOS\0MacOS\0EBCDIC US-Canada\0Symbol\0DOS USA / OEM-US\0EBCDIC International\0DOS Arabic ASMO-708\0DOS Arabic\0DOS Greek\0DOS Baltic\0DOS Latin1 / Western European\0DOS Latin2 / Central European \0DOS Cyrillic\0DOS Turkish\0DOS Latin1 Multilingual\0DOS Portuguese\0DOS Icelandic\0DOS Hebrew\0DOS French Canadian\0DOS Arabic\0DOS Nordic\0DOS Russian\0DOS Modern Greek\0EBCDIC Latin2 Multilingual\0Thai\0EBCDIC Greek Modern\0Japanese Shift-JIS\0Simplified Chinese GB2312\0Korean Hangul\0Traditional Chinese Big5\0Traditional Chinese Big5-HKSCS\0EBCDIC Turkish\0EBCDIC Latin1\0EBCDIC US-Canada\0EBCDIC Germany\0EBCDIC Denmark-Norway\0EBCDIC Finland-Sweden\0EBCDIC Italy\0EBCDIC Latin America-Spain\0EBCDIC UK\0EBCDIC France\0EBCDIC International\0EBCDIC Icelandic\0Latin2 / Central European\0Cyrillic\0Latin1 / Western European\0Greek\0Turkish\0Hebrew\0Arabic\0Baltic\0Vietnamese\0Korean Johab\0MAC Roman / Western European\0MAC Japanese\0MAC Traditional Chinese Big5\0MAC Korean\0MAC Arabic\0MAC Hebrew\0MAC Greek\0MAC Cyrillic\0MAC Simplified Chinese GB2312\0MAC Romanian\0MAC Ukrainian\0MAC Thai\0MAC Roman2 / Central European\0MAC Icelandic\0MAC Turkish\0MAC Croatian\0CNS-11643 Taiwan\0TCA Taiwan\0ETEN Taiwan\0IBM5550 Taiwan\0TeleText Taiwan\0Wang Taiwan\0IA5 IRV Western European 7-bit\0IA5 German 7-bit\0IA5 Swedish 7-bit\0IA5 Norwegian 7-bit\0US-ASCII 7-bit\0T.61\0ISO-6937\0EBCDIC Germany\0EBCDIC Denmark-Norway\0EBCDIC Finland-Sweden\0EBCDIC Italy\0EBCDIC Latin America-Spain\0EBCDIC UK\0EBCDIC Japanese Katakana\0EBCDIC France\0EBCDIC Arabic\0EBCDIC Greek\0EBCDIC Hebrew\0EBCDIC Korean\0EBCDIC Thai\0Russian KOI8-R\0EBCDIC Icelandic\0EBCDIC Cyrillic\0EBCDIC Turkish\0EBCDIC Latin1\0Japanese EUC / JIS 0208-1990\0Simplified Chinese GB2312-80\0Korean Wansung\0EBCDIC Serbian-Bulgarian\0EBCDIC Japanese\0Ukrainian KOI8-U\0ISO8859-1 Latin1 / Western European\0ISO8859-2 Latin2 / Central European\0ISO8859-3 Latin3 / South European\0ISO8859-4 Baltic\0ISO8859-5 Cyrillic\0ISO8859-6 Arabic\0ISO8859-7 Greek\0ISO8859-8 Hebrew Visual\0ISO8859-9 Turkish\0ISO8859-11 Thai\0ISO8859-13 Estonian\0ISO8859-14 Celtic\0ISO8859-15 Latin9\0ISO8859-8 Hebrew Logical\0ISO2022 Japanese JIS\0ISO2022 Japanese halfwidth Katakana\0ISO2022 Japanese JIS X 0201-1989\0ISO2022 Korean\0ISO2022 Simplified Chinese\0ISO2022 Traditional Chinese\0EUC Japanese\0EUC Simplified Chinese\0EUC Korean\0EUC Traditional Chinese\0HZ-GB2312 Simplified Chinese \0GB18030 Simplified Chinese\0ISCII Devanagari\0ISCII Bangla\0ISCII Tamil\0ISCII Telugu\0ISCII Assamese\0ISCII Odia\0ISCII Kannada\0ISCII Malayalam\0ISCII Gujarati\0ISCII Punjabi\0UTF-7\0UTF-8";
static WORD knownCPI[] = {1,0,34,4,394,62,207,11,16,37,74,1,2,1,0,1,0,0,0,0,0,0,2,0,3,0,56,3,12,0,0,74,20,92,0,0,0,0,0,0,0,0,0,100,0,0,0,0,0,0,0,0,102,8638,0,0,0,0,0,0,0,0,1,6,3,7,49,1,0,9917,0,0,0,0,0,99,0,0,0,18,133,7,3,3,0,1,3,0,4,6,122,2,0,408,4,27,4,8,24,18,7,3,12,75,1,838,6724,0,0,0,0,0,0,0,0,1,1,0,0,9992,11621,0,0,2,1,1,1702,3,12,0,985,1999,2065,0,0,0,0,0,0,0,0,0,7988,0};
static WORD knownCPnoinvalI[] = {42,50177,0,0,2,1,1,6772,0,0,0,0,0,0,0,0,0,7988};
static BYTE knownCPOrd[] = {43,0,0,0,0,0,0,0,0,43,9,174,1,0,0,0,112,0,112,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,21,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,18,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,116,2,17,1,5,0,0,0,0,0,0,0,0,0,0,0,39,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,3,0};

static unsigned short decLut[] = {
	0xfe3f, 0x81ff, 0x01bf, 0xff7f,  0xff7f, 0xffff, 0xffff, 0xffbf,  0xffff, 0xffff, 0xffff, 0xfbff,  0x03ff, 0xffff, 0xffff, 0xfdff,
	0xe980, 0x0200, 0x0200, 0x0200,  0x0200, 0x0200, 0x0200, 0x0200,  0x0200, 0x0200, 0x0635, 0xffff,  0xffff, 0x7dff, 0x81ff, 0xffff,
	0xffff, 0x7fca, 0x0200, 0x0200,  0x0200, 0x01c0, 0x0200, 0x0200,  0x0200, 0x0200, 0x0200, 0x0200,  0x0200, 0x0200, 0x0200, 0x0200,
	0x0200, 0x0200, 0x0200, 0x0200,  0x0200, 0x0200, 0x0200, 0x0200,  0x0200, 0x0200, 0x0200, 0x4e1b,  0xffff, 0xffff, 0xffff, 0xffff,
	0x00bf, 0xb34a, 0x01c0, 0x0280,  0x0180, 0x0200, 0x0200, 0x02c0,  0x0140, 0x0200, 0x0200, 0x0200,  0x0200, 0x0200, 0x0200, 0x0300,
	0x0100, 0x0200, 0x0200, 0x0200,  0x0200, 0x0200, 0x0200, 0x0200,  0x0200, 0x0200, 0x0200, 0x1a1b,  0xffff, 0xffff, 0xffff, 0x013f,
};
static unsigned short encLut[] = {
	0x4130, 0x0100, 0x0100, 0x0100,  0x0100, 0x0100, 0x0100, 0x0100,  0x0100, 0x0100, 0x0127, 0x0100,  0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,  0x0100, 0x0100, 0x0100, 0x0100,  0x0100, 0x0100, 0x0700, 0x0100,  0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0100,  0x00a5, 0x00ff, 0x00ff, 0x00ff,  0x00ff, 0x00ff, 0x00ff, 0x00ff,  0x00ff, 0x00ff, 0x00ff, 0x00ff,
	0x00ff, 0x00ff, 0x00ff, 0x00ff,  0xb5ff, 0x00ff, 0x00ff, 0x00ff,  0x00ff, 0x00ff, 0x00ff, 0x00ff,  0x00ff, 0x00ff, 0xf1ff, 0x03ff,
	0x0dff,
};



void ExpandDifMap(LPVOID map, WORD width, DWORD len){
	switch(width){
		case 1:
			while(--len)
				*(++(BYTE*)map) = *((BYTE*)map)+((BYTE*)map)[-1]+1;
			break;
		case 2:
			while(--len)
				*(++(WORD*)map) = *((WORD*)map)+((WORD*)map)[-1]+1;
			break;
	}
}

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
	if (encLut[1] == encLut[2]) {
		ExpandDifMap(decLut, sizeof(*decLut), ARRLEN(decLut));
		ExpandDifMap(encLut, sizeof(*encLut), ARRLEN(encLut));
	}
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
			ERROROUT(GetString(len == -1 ? IDS_DECODEBASE_BADLEN : IDS_DECODEBASE_BADCHAR));
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
	if (encLut[1] == encLut[2]) {
		ExpandDifMap(decLut, sizeof(*decLut), ARRLEN(decLut));
		ExpandDifMap(encLut, sizeof(*encLut), ARRLEN(encLut));
	}
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
 * @param[in,out] len In: length of [pb] in bytes; Out: length of detected BOM in bytes
 * @return One of FC_ENC_* enums if a BOM is found, else FC_ENC_UNKNOWN
 */
WORD CheckBOM(LPBYTE pb, DWORD* len) {
	DWORD i, j;
	for (i = 0; i < ARRLEN(bomLut); i++){
		if(pb && len && *len >= (j = bomLut[i][0]) && !memcmp(pb, bomLut[i]+1, j)){
			*len = j;
			return bomIdx[i];
		}
	}
	if (len) *len=0;
	return FC_ENC_UNKNOWN;
}
WORD GetBOM(LPBYTE* bom, WORD enc){
	WORD i;
	for (i = 0; bom && i < ARRLEN(bomIdx); i++)
		if (bomIdx[i] == enc) {
			*bom = (LPBYTE)bomLut[i] + 1;
			return bomLut[i][0];
		}
	return 0;
}


LPCTSTR GetCPName(WORD cp){
	static WORD ofs[ARRLEN(knownCPI)] = {0}, ofspop = 0;
	static TCHAR strcache[sizeof(knownCPT)];
	return GetStringEx(cp, ARRLEN(knownCPI), (LPSTR)knownCPT, knownCPI, ofs, strcache, &ofspop, NULL);
}
void PrintCPName(WORD cp, LPTSTR buf, LPCTSTR format) {
	LPCTSTR asz;
	asz = GetCPName(cp);
	wsprintf(buf, format, cp);
	if (asz) {
		lstrcat(buf, _T(" - "));
		lstrcat(buf, asz);
	}
}
WORD GetNumKnownCPs(){
	return ARRLEN(knownCPI);
}
WORD GetKnownCP(INT idx) {
	if (knownCPOrd[1] < knownCPOrd[0]){
		GetCPName(0);
		ExpandDifMap(knownCPOrd, sizeof(*knownCPOrd), ARRLEN(knownCPOrd));
	}
	return knownCPI[knownCPOrd[idx]];
}
BOOL CanCPHaveInvalidCodes(WORD cp){
	WORD i;
	if (!knownCPnoinvalI[2]) ExpandDifMap(knownCPnoinvalI, sizeof(*knownCPnoinvalI), ARRLEN(knownCPnoinvalI));
	for(i = 0; i < ARRLEN(knownCPnoinvalI); i++){
		if (knownCPnoinvalI[i] == cp)
			return FALSE;
	}
	return TRUE;
}



BOOL IsTextUTF8(LPBYTE buf){
	BOOL yes = 0;
	while (*buf) {
		if (*buf < 0x80) buf++;
		else if ((*((PWORD)buf) & 0xc0e0) == 0x80c0){ buf+=2; yes = 1; }
		else if ((*((PDWORD)buf) & 0xc0c0f0) == 0x8080e0){ buf+=3; yes = 1; }
		else if ((*((PDWORD)buf) & 0xc0c0c0f8) == 0x808080f0){ buf+=4; yes = 1; }
		else return 0;
	}
	return yes;
}

WORD GetLineFmt(LPCTSTR sz, DWORD len, WORD preferred, DWORD* nCR, DWORD* nLF, DWORD* nStrays, DWORD* nSub, BOOL* binary){
	DWORD i, crlf = 1, cr = 0, lf = 0;
	TCHAR cc, cp = _T('\0');
	if (binary) *binary = FALSE;
	if (nStrays) *nStrays = 0;
	if (nSub) *nSub = 0;
	for (i = 0; (len && i++ < len) || (!len && *sz); cp = cc) {
		if ((cc = *sz++) == _T('\r')){
			cr++;
			if ((nStrays || crlf) && *sz != _T('\n')) { crlf = 0; (*nStrays)++; }
		} else if (cc == _T('\n')) {
			lf++;
			if ((nStrays || crlf) && cp != _T('\r')) { crlf = 0; (*nStrays)++; }
		} 
#ifdef USE_RICH_EDIT
		  else if (nSub && cc == '\x14') (*nSub)++;
#endif
		if (cr && lf && !crlf && !nStrays) return FC_LFMT_MIXED;
		if (len && binary && cc == '\0') *binary = TRUE;
	}
	if (nCR) *nCR = cr;
	if (nLF) *nLF = lf;
	if (cr && lf && !crlf) return FC_LFMT_MIXED;
	else if (!cr && !lf && preferred) return preferred;
	else if (crlf) return FC_LFMT_DOS;
	else if (cr) return FC_LFMT_MAC;
	else return FC_LFMT_UNIX;
}

void ImportBinary(LPTSTR sz, DWORD len){
#ifdef UNICODE
	TCHAR c = gbNT ? _T('\x2400') : _T(' ');
#endif
	for ( ; len--; sz++){
		if (*sz == _T('\0'))
#ifdef UNICODE
			*sz = c;
#else
			*sz = _T(' ');
#endif
	}
}
void ExportBinary(LPTSTR sz, DWORD len){
#ifdef UNICODE
	if (!gbNT) return;
	if (!len) len = lstrlen(sz);
	for ( ; len--; sz++){
		if (*sz == _T('\x2400'))
			*sz = _T('\0');
	}
#endif
}

#ifdef USE_RICH_EDIT
void ImportLineFmt(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD nCR, DWORD nLF, DWORD nStrays, DWORD nSub, BOOL* bufDirty){
	LPTSTR odst, dst, osz;
	DWORD len;
	if (lfmt != FC_LFMT_MIXED || !sz || !*sz || !(nLF + nSub)) return;
	len = (chars && *chars) ? *chars : lstrlen(*sz);
	osz = *sz;
	odst = dst = kallocs(len+nLF+nSub+1);
	for ( ; len--; (*sz)++) {
		if (**sz == _T('\n') || **sz == _T('\x14'))
			*dst++ = _T('\x14');
		*dst++ = **sz;
	}
	*dst = _T('\0');
	if (chars) *chars = dst - odst;
	if (bufDirty) {
		if (*bufDirty) kfree(&osz);
		*bufDirty = TRUE;
	}
	*sz = odst;
}
void ExportLineFmt(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD lines, BOOL* bufDirty){
	LPTSTR odst, dst, osz;
	DWORD len, l;
	len = (chars && *chars) ? *chars : lstrlen(*sz);
	if (chars) *chars = len;
	if (lfmt == FC_LFMT_MAC || !sz || !*sz) return;
	odst = dst = osz = *sz;
	if ((LONG)lines == -1) {
		for (l = len, lines = 0; l--; ) {
			if (*osz++ == _T('\r'))
				lines++;
		}
		osz = *sz;
	}
	if (lfmt == FC_LFMT_DOS && lines) odst = dst = kallocs(len+lines+1);
	for ( ; len--; (*sz)++, *dst++) {
		*dst = **sz;
		if (lfmt == FC_LFMT_MIXED && **sz == _T('\x14')) {
			if (*++(*sz) == _T('\r')) *dst = _T('\n');
			len--;
		} else if (**sz == _T('\r')) {
			if (lfmt == FC_LFMT_UNIX) *dst = _T('\n');
			else if (lfmt == FC_LFMT_DOS) *++dst = _T('\n');
		}
	}
	*dst = _T('\0');
	if (chars) *chars = dst - odst;
	if (odst != osz) {
		if (bufDirty) {
			if (*bufDirty) kfree(&osz);
			*bufDirty = TRUE;
		}
		*sz = odst;
	} else *sz = osz;
}
LONG ExportLineFmtDelta(LPCTSTR sz, DWORD* chars, WORD lfmt){
	DWORD len, olen, ct;
	if (!sz) return (lfmt == FC_LFMT_MAC || lfmt == FC_LFMT_UNIX ? 0 : (lfmt == FC_LFMT_DOS ? 1 : -1));
	if (lfmt == FC_LFMT_MAC || lfmt == FC_LFMT_UNIX) return 0;
	ct = len = olen = (chars && *chars) ? *chars : lstrlen(sz);
	for ( ; len--; sz++) {
		switch(lfmt){
			case FC_LFMT_MIXED:
				if (*sz == _T('\x14')) { sz++; ct--; len--; }
				break;
			case FC_LFMT_DOS:
				if (*sz == _T('\r')) ct++;
				break;
		}
	}
	if (chars) *chars = ct;
	return ct - olen;
}
void ExportLineFmtLE(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD lines, BOOL* bufDirty){
#else
#define ExportLineFmtLE	ExportLineFmt
void ImportLineFmt(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD nCR, DWORD nLF, DWORD nStrays, DWORD nSub, BOOL* bufDirty){
	LPTSTR odst, dst, osz;
	TCHAR cc, cp = _T('\0');
	DWORD len;
	if (lfmt == FC_LFMT_MIXED || !sz || !*sz || !nStrays) return;
	len = (chars && *chars) ? *chars : lstrlen(*sz);
	osz = *sz;
	odst = dst = kallocs(len+nStrays+1);
	for ( ; len--; cp = cc) {
		if ((cc = *(*sz)++) == _T('\r') && **sz != _T('\n')) {
			*dst++ = _T('\r'); *dst++ = _T('\n');
			continue;
		} else if (cc == _T('\n') && cp != _T('\r'))
			*dst++ = _T('\r');
		*dst++ = cc;
	}
	*dst = _T('\0');
	if (chars) *chars = dst - odst;
	if (bufDirty) {
		if (*bufDirty) kfree(&osz);
		*bufDirty = TRUE;
	}
	*sz = odst;
}
LONG ExportLineFmtDelta(LPCTSTR sz, DWORD* chars, WORD lfmt){
	DWORD len, olen, ct;
	if (!sz) return (lfmt != FC_LFMT_UNIX && lfmt != FC_LFMT_MAC ? 0 : -1);
	if (lfmt != FC_LFMT_UNIX && lfmt != FC_LFMT_MAC) return 0;
	ct = len = olen = (chars && *chars) ? *chars : lstrlen(sz);
	while ( len-- ) {
		if (*sz++ == _T('\r'))
			ct--;
	}
	if (chars) *chars = ct;
	return ct - olen;
}
void ExportLineFmt(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD lines, BOOL* bufDirty){
#endif
	LPTSTR odst, dst, osz;
	DWORD len;
	len = (chars && *chars) ? *chars : lstrlen(*sz);
	if (chars) *chars = len;
	if ((lfmt != FC_LFMT_UNIX && lfmt != FC_LFMT_MAC) || !sz || !*sz || !lines) return;
	odst = dst = osz = *sz;
	for ( ; len--; dst++) {
		*dst = *(*sz)++;
		if (*dst == _T('\r')) {
			if (lfmt == FC_LFMT_UNIX) *dst = _T('\n');
			(*sz)++;
			len--;
		}
	}
	if (chars) *chars = dst - odst;
	*dst = _T('\0');
	*sz = osz;
}


//Returns number of decoded chars (not including the null terminator)
DWORD DecodeText(LPBYTE* buf, DWORD bytes, DWORD* format, BOOL* bufDirty) {
#ifndef UNICODE
	BOOL bUsedDefault;
#endif
	DWORD chars = bytes;
	LPBYTE newbuf = NULL;
	WORD enc, cp;
	if (!buf || !*buf || !format || !bytes) return 0;
	cp = (*format >> 31 ? (WORD)*format : 0);
	enc = cp ? FC_ENC_CODEPAGE : (WORD)*format;
	if (!cp || enc == FC_ENC_ANSI) cp = CP_ACP;
	if (enc == FC_ENC_UTF16 || enc == FC_ENC_UTF16BE) {
		chars /= 2;
		if (enc == FC_ENC_UTF16BE)
			ReverseBytes(*buf, bytes);
#ifndef UNICODE
		if (sizeof(TCHAR) < 2) {
			bytes = WideCharToMultiByte(cp, 0, (LPCWSTR)*buf, chars, NULL, 0, NULL, NULL);
			if (!(newbuf = (LPBYTE)kalloc(bytes+1)))
				return 0;
			else if (!WideCharToMultiByte(cp, 0, (LPCWSTR)*buf, chars, (LPSTR)newbuf, bytes, NULL, &bUsedDefault)) {
				ReportLastError();
				ERROROUT(GetString(IDS_UNICODE_CONVERT_ERROR));
				bytes = 0;
			}
			if (bUsedDefault)
				ERROROUT(GetString(IDS_UNICODE_CHARS_WARNING));
		}
#else
	} else if (sizeof(TCHAR) > 1) {
		if (enc == FC_ENC_UTF8)
			cp = CP_UTF8;
		do {
			kfree(&newbuf);
			bytes = MultiByteToWideChar(cp, 0, *buf, chars, NULL, 0)*sizeof(TCHAR);
			if (!(newbuf = (LPBYTE)kalloc(bytes+sizeof(TCHAR))))
				return 0;
			else if (!MultiByteToWideChar(cp, CanCPHaveInvalidCodes(cp) ? MB_ERR_INVALID_CHARS : 0, *buf, chars, (LPWSTR)newbuf, bytes)){
				if (!MultiByteToWideChar(cp, 0, *buf, chars, (LPWSTR)newbuf, bytes)){
					if (enc == FC_ENC_CODEPAGE) {
						cp = CP_ACP;
						*format &= 0xfff0000;
						*format |= FC_ENC_ANSI;
						continue;
					}
					ReportLastError();
					ERROROUT(GetString(IDS_UNICODE_LOAD_ERROR));
					bytes = 0;
				} else ERROROUT(GetString(IDS_UNICODE_LOAD_TRUNCATION));
			}
			break;
		} while (1);
#endif
	}
	if (newbuf) {
		if (bufDirty) {
			if (*bufDirty) kfree(buf);
			*bufDirty = TRUE;
		}
		*buf = newbuf;
	}
	((LPTSTR)(*buf))[bytes/=sizeof(TCHAR)] = _T('\0');
	return bytes;
}

//Returns number of encoded bytes (not including the null terminator)
DWORD EncodeText(LPBYTE* buf, DWORD chars, DWORD format, BOOL* bufDirty, BOOL* truncated) {
	DWORD bytes = chars;
	WORD enc, cp;
	LPBYTE newbuf = NULL;
	if (!buf || !*buf || !chars) return 0;
	cp = (format >> 31 ? (WORD)format : 0);
	enc = cp ? FC_ENC_CODEPAGE : (WORD)format;
	if (!cp || enc == FC_ENC_ANSI) cp = CP_ACP;
	if (enc == FC_ENC_UTF16 || enc == FC_ENC_UTF16BE) {
#ifndef UNICODE
		if (sizeof(TCHAR) < 2) {
			bytes = 2 * MultiByteToWideChar(cp, 0, (LPCSTR)*buf, chars, NULL, 0);
			if (!(newbuf = (LPBYTE)kalloc(bytes+1)))
				return 0;
			else if (!MultiByteToWideChar(cp, 0, (LPCSTR)*buf, chars, (LPWSTR)newbuf, bytes)) {
				ReportLastError();
				ERROROUT(GetString(IDS_UNICODE_STRING_ERROR));
				bytes = 0;
			}
		}
#else
		bytes = chars * sizeof(TCHAR);
	} else if (sizeof(TCHAR) > 1) {
		if (enc == FC_ENC_UTF8)
			cp = CP_UTF8;
		do {
			kfree(&newbuf);
			bytes = WideCharToMultiByte(cp, 0, (LPTSTR)*buf, chars, NULL, 0, NULL, NULL);
			if (!(newbuf = (LPBYTE)kalloc(bytes+1)))
				return 0;
			else if (!WideCharToMultiByte(cp, 0, (LPTSTR)*buf, chars, (LPSTR)newbuf, bytes, NULL, (cp != CP_UTF8 && CanCPHaveInvalidCodes(cp) && truncated ? truncated : NULL))) {
				if (enc == FC_ENC_CODEPAGE) {
					cp = CP_ACP;
					ERROROUT(GetString(IDS_ENC_FAILED));
					continue;
				}
				ReportLastError();
				bytes = 0;
			}
			break;
		} while (1);
#endif	
	}
	if (newbuf) {
		if (bufDirty) {
			if (*bufDirty) kfree(buf);
			*bufDirty = TRUE;
		}
		*buf = newbuf;
	}
	if (enc == FC_ENC_UTF16BE)
		ReverseBytes(*buf, bytes);
	(*buf)[bytes] = _T('\0');
	return bytes;
}

void EvaHash(LPBYTE buf, DWORD len, LPBYTE hash) {							//Originally by Bob Jenkins, 1996, Public Domain. Variant: 32-byte multiples only!
	DWORD register a = 0x9e3779b9, b=a, c=a, d=a, e=a, f=a, g=a, h=a;
	DWORD* bb = (DWORD*)buf;
	for (len=(len+31)/32; len--; ){
		a+=*bb++; b+=*bb++; c+=*bb++; d+=*bb++; e+=*bb++; f+=*bb++; g+=*bb++; h+=*bb++;
		a^=b<<11; d+=a; b+=c;	b^=c>>2; e+=b; c+=d;	c^=d<<8; f+=c; d+=e;	d^=e>>16; g+=d; e+=f;	e^=f<<10; h+=e; f+=g;	f^=g>>4; a+=f; g+=h;	g^=h<<8; b+=g; h+=a;	h^=a>>9; c+=h; a+=b;
		a^=b<<11; d+=a; b+=c;	b^=c>>2; e+=b; c+=d;	c^=d<<8; f+=c; d+=e;	d^=e>>16; g+=d; e+=f;	e^=f<<10; h+=e; f+=g;	f^=g>>4; a+=f; g+=h;	g^=h<<8; b+=g; h+=a;	h^=a>>9; c+=h; a+=b;
		a^=b<<11; d+=a; b+=c;	b^=c>>2; e+=b; c+=d;	c^=d<<8; f+=c; d+=e;	d^=e>>16; g+=d; e+=f;	e^=f<<10; h+=e; f+=g;	f^=g>>4; a+=f; g+=h;	g^=h<<8; b+=g; h+=a;	h^=a>>9; c+=h; a+=b;
		a^=b<<11; d+=a; b+=c;	b^=c>>2; e+=b; c+=d;	c^=d<<8; f+=c; d+=e;	d^=e>>16; g+=d; e+=f;	e^=f<<10; h+=e; f+=g;	f^=g>>4; a+=f; g+=h;	g^=h<<8; b+=g; h+=a;	h^=a>>9; c+=h; a+=b;
	}
	bb = (DWORD*)hash;
	*bb++=a; *bb++=b; *bb++=c; *bb++=d; *bb++=e; *bb++=f; *bb++=g; *bb++=h;
}

LPTSTR FormatNumber(LONGLONG num, BOOL group, TCHAR sep, WORD buffer){
	static TCHAR buf[28*2] = {0};
	INT i, j, c;
	LPTSTR bp = buf + buffer*28;
	if (!sep) sep = _T('\'');
	_i64tot(num, bp, 10);
	if (!group || (num >= -9999 && num <= 9999)) return bp;
	for (i=lstrlen(bp), j=26, c=0; i--; ) {
		bp[j--] = bp[i];
		if (!(++c % 3) && i && (i > 1 || bp[i-1] != _T('-')))
			bp[j--] = sep;
	}
	return bp+j+1;
}