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
void ReportLastError(void);

#define NUMBOMS 3
static const BYTE bomLut[NUMBOMS][4] = {{0x3,0xEF,0xBB,0xBF}, {0x2,0xFF,0xFE}, {0x2,0xFE,0xFF}};

#define NUMKNOWNCPS 144
static const CHAR knownCPT[] = "Latin2 / Central European\0Cyrillic\0Latin1 / Western European\0Greek\0Turkish\0Hebrew\0Arabic\0Baltic\0Vietnamese\0Russian KOI8-R\0Ukrainian KOI8-U\0Thai\0Japanese Shift-JIS\0Simplified Chinese GB2312\0Korean Hangul\0Traditional Chinese Big5\0UTF-7\0UTF-8\0Local DOS\0MacOS\0Symbol\0DOS USA / OEM-US\0DOS Arabic ASMO-708\0DOS Arabic\0DOS Greek\0DOS Baltic\0DOS Latin1 / Western European\0DOS Latin2 / Central European \0DOS Cyrillic\0DOS Turkish\0DOS Latin1 Multilingual\0DOS Portuguese\0DOS Icelandic\0DOS Hebrew\0DOS French Canadian\0DOS Arabic\0DOS Nordic\0DOS Russian\0DOS Modern Greek\0Traditional Chinese Big5-HKSCS\0Korean Johab\0MAC Roman / Western European\0MAC Japanese\0MAC Traditional Chinese Big5\0MAC Korean\0MAC Arabic\0MAC Hebrew\0MAC Greek\0MAC Cyrillic\0MAC Simplified Chinese GB2312\0MAC Romanian\0MAC Ukrainian\0MAC Thai\0MAC Roman2 / Central European\0MAC Icelandic\0MAC Turkish\0MAC Croatian\0CNS-11643 Taiwan\0TCA Taiwan\0ETEN Taiwan\0IBM5550 Taiwan\0TeleText Taiwan\0Wang Taiwan\0IA5 IRV Western European 7-bit\0IA5 German 7-bit\0IA5 Swedish 7-bit\0IA5 Norwegian 7-bit\0US-ASCII 7-bit\0T.61\0ISO-6937\0Japanese EUC / JIS 0208-1990\0Simplified Chinese GB2312-80\0Korean Wansung\0ISO8859-1 Latin1 / Western European\0ISO8859-2 Latin2 / Central European\0ISO8859-3 Latin3 / South European\0ISO8859-4 Baltic\0ISO8859-5 Cyrillic\0ISO8859-6 Arabic\0ISO8859-7 Greek\0ISO8859-8 Hebrew Visual\0ISO8859-9 Turkish\0ISO8859-11 Thai\0ISO8859-13 Estonian\0ISO8859-14 Celtic\0ISO8859-15 Latin9\0ISO8859-8 Hebrew Logical\0ISO2022 Japanese JIS\0ISO2022 Japanese halfwidth Katakana\0ISO2022 Japanese JIS X 0201-1989\0ISO2022 Korean\0ISO2022 Simplified Chinese\0ISO2022 Traditional Chinese\0EUC Japanese\0EUC Simplified Chinese\0EUC Korean\0EUC Traditional Chinese\0HZ-GB2312 Simplified Chinese \0GB18030 Simplified Chinese\0ISCII Devanagari\0ISCII Bangla\0ISCII Tamil\0ISCII Telugu\0ISCII Assamese\0ISCII Odia\0ISCII Kannada\0ISCII Malayalam\0ISCII Gujarati\0ISCII Punjabi\0EBCDIC US-Canada\0EBCDIC International\0EBCDIC Latin2 Multilingual\0EBCDIC Greek Modern\0EBCDIC Turkish\0EBCDIC Latin1\0EBCDIC US-Canada\0EBCDIC Germany\0EBCDIC Denmark-Norway\0EBCDIC Finland-Sweden\0EBCDIC Italy\0EBCDIC Latin America-Spain\0EBCDIC UK\0EBCDIC France\0EBCDIC International\0EBCDIC Icelandic\0EBCDIC Germany\0EBCDIC Denmark-Norway\0EBCDIC Finland-Sweden\0EBCDIC Italy\0EBCDIC Latin America-Spain\0EBCDIC UK\0EBCDIC Japanese Katakana\0EBCDIC France\0EBCDIC Arabic\0EBCDIC Greek\0EBCDIC Hebrew\0EBCDIC Korean\0EBCDIC Thai\0EBCDIC Icelandic\0EBCDIC Cyrillic\0EBCDIC Turkish\0EBCDIC Latin1\0EBCDIC Serbian-Bulgarian\0EBCDIC Japanese\0";
static const WORD knownCPN[NUMKNOWNCPS] = {1250,1251,1252,1253,1254,1255,1256,1257,1258,20866,21866,874,932,936,949,950,65000,65001,1,2,42,437,708,720,737,775,850,852,855,857,858,860,861,862,863,864,865,866,869,951,1361,10000,10001,10002,10003,10004,10005,10006,10007,10008,10010,10017,10021,10029,10079,10081,10082,20000,20001,20002,20003,20004,20005,20105,20106,20107,20108,20127,20261,20269,20932,20936,20949,28591,28592,28593,28594,28595,28596,28597,28598,28599,28601,28603,28604,28605,38598,50220,50221,50222,50225,50227,50229,51932,51936,51949,51950,52936,54936,57002,57003,57004,57005,57006,57007,57008,57009,57010,57011,37,500,870,875,1026,1047,1140,1141,1142,1143,1144,1145,1146,1147,1148,1149,20273,20277,20278,20280,20284,20285,20290,20297,20420,20423,20424,20833,20838,20871,20880,20905,20924,21025,21027};

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
WORD GetBOM(LPBYTE* bom, WORD enc){
	if (!bom) return 0;
	switch (enc) {
		case ID_ENC_UTF8:
		case ID_ENC_UTF16:
		case ID_ENC_UTF16BE:
			*bom = (LPBYTE)bomLut[enc - ID_ENC_UTF8] + 1;
			return bomLut[enc - ID_ENC_UTF8][0];
	}
	return 0;
}

WORD GetKnownCP(WORD idx) {
	return knownCPN[idx];
}
LPCTSTR GetCPName(WORD cp){
	static WORD ofs[NUMKNOWNCPS] = {0}, ofspop = 0;
	static TCHAR strcache[sizeof(knownCPT)];
	if (cp >= 100 && cp < 300) cp = knownCPN[cp-100];
	return GetStringEx(cp, NUMKNOWNCPS, (LPSTR)knownCPT, knownCPN, ofs, strcache, &ofspop, NULL);
}
void PrintCPName(WORD cp, LPTSTR buf, LPCTSTR format) {
	LPCTSTR asz;
	if (cp >= 100 && cp < 300) cp = knownCPN[cp-100];
	asz = GetCPName(cp);
	wsprintf(buf, format, cp);
	if (asz) {
		lstrcat(buf, _T(" - "));
		lstrcat(buf, asz);
	}
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
		if (cr && lf && !crlf && !nStrays) return ID_LFMT_MIXED;
		if (len && binary && cc == '\0') *binary = TRUE;
	}
	if (nCR) *nCR = cr;
	if (nLF) *nLF = lf;
	if (cr && lf && !crlf) return ID_LFMT_MIXED;
	else if (!cr && !lf && preferred) return preferred;
	else if (crlf) return ID_LFMT_DOS;
	else if (cr) return ID_LFMT_MAC;
	else return ID_LFMT_UNIX;
}

void ImportBinary(LPTSTR sz, DWORD len){
	for ( ; len--; sz++){
		if (*sz == _T('\0'))
#ifdef UNICODE
			*sz = _T('\x2400');
#else
			*sz = _T(' ');
#endif
	}
}
void ExportBinary(LPTSTR sz, DWORD len){
#ifdef UNICODE
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
	if (lfmt != ID_LFMT_MIXED || !sz || !*sz || !(nLF + nSub)) return;
	len = (chars && *chars) ? *chars : lstrlen(*sz);
	osz = *sz;
	odst = dst = (LPTSTR)HeapAlloc(globalHeap, 0, (len+nLF+nSub+1) * sizeof(TCHAR));
	for ( ; len--; (*sz)++) {
		if (**sz == _T('\n') || **sz == _T('\x14'))
			*dst++ = _T('\x14');
		*dst++ = **sz;
	}
	*dst = _T('\0');
	if (chars) *chars = dst - odst;
	if (bufDirty) {
		if (*bufDirty) FREE(osz)
		*bufDirty = TRUE;
	}
	*sz = odst;
}
void ExportLineFmt(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD lines, BOOL* bufDirty){
	LPTSTR odst, dst, osz;
	DWORD len;
	if (lfmt == ID_LFMT_MAC || !sz || !*sz) return;
	len = (chars && *chars) ? *chars : lstrlen(*sz);
	odst = dst = osz = *sz;
	if (lfmt == ID_LFMT_DOS && lines) odst = dst = (LPTSTR)HeapAlloc(globalHeap, 0, (len+lines+1) * sizeof(TCHAR));
	for ( ; len--; (*sz)++, *dst++) {
		*dst = **sz;
		if (lfmt == ID_LFMT_MIXED && **sz == _T('\x14')) {
			if (*++(*sz) == _T('\r')) *dst = _T('\n');
		} else if (**sz == _T('\r')) {
			if (lfmt == ID_LFMT_UNIX) *dst = _T('\n');
			else if (lfmt == ID_LFMT_DOS) *++dst = _T('\n');
		}
	}
	*dst = _T('\0');
	if (chars) *chars = dst - odst;
	if (odst != osz) {
		if (bufDirty) {
			if (*bufDirty) FREE(osz)
			*bufDirty = TRUE;
		}
		*sz = odst;
	}
}
LONG ExportLineFmtDelta(LPCTSTR sz, DWORD* chars, WORD lfmt){
	DWORD len, olen, ct;
	if (!sz) return (lfmt == ID_LFMT_MAC || lfmt == ID_LFMT_UNIX ? 0 : (lfmt == ID_LFMT_DOS ? 1 : -1));
	if (lfmt == ID_LFMT_MAC || lfmt == ID_LFMT_UNIX) return 0;
	ct = len = olen = (chars && *chars) ? *chars : lstrlen(sz);
	for ( ; len--; sz++) {
		switch(lfmt){
			case ID_LFMT_MIXED:
				if (*sz == _T('\x14')) { sz++; ct--; }
				break;
			case ID_LFMT_DOS:
				if (*sz == _T('\r')) ct++;
				break;
		}
	}
	if (chars) *chars = ct;
	return ct - olen;
}
#else
void ImportLineFmt(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD nCR, DWORD nLF, DWORD nStrays, DWORD nSub, BOOL* bufDirty){
	LPTSTR odst, dst, osz;
	TCHAR cc, cp = _T('\0');
	DWORD len;
	if (lfmt == ID_LFMT_MIXED || !sz || !*sz || !nStrays) return;
	len = (chars && *chars) ? *chars : lstrlen(*sz);
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
	if (chars) *chars = dst - odst;
	if (bufDirty) {
		if (*bufDirty) FREE(osz)
		*bufDirty = TRUE;
	}
	*sz = odst;
}
void ExportLineFmt(LPTSTR* sz, DWORD* chars, WORD lfmt, DWORD lines, BOOL* bufDirty){
	LPTSTR odst, dst, osz;
	DWORD len;
	if ((lfmt != ID_LFMT_UNIX && lfmt != ID_LFMT_MAC) || !sz || !*sz || !lines) return;
	len = (chars && *chars) ? *chars : lstrlen(*sz);
	odst = dst = osz = *sz;
	for ( ; len--; dst++) {
		*dst = *(*sz)++;
		if (*dst == _T('\r')) {
			if (lfmt == ID_LFMT_UNIX) *dst = _T('\n');
			*sz++;
		}
	}
	if (chars) *chars = dst - odst;
	*dst = _T('\0');
	*sz = osz;
}
LONG ExportLineFmtDelta(LPCTSTR sz, DWORD* chars, WORD lfmt){
	DWORD len, olen, ct;
	if (!sz) return (lfmt != ID_LFMT_UNIX && lfmt != ID_LFMT_MAC ? 0 : -1);
	if (lfmt != ID_LFMT_UNIX && lfmt != ID_LFMT_MAC) return 0;
	ct = len = olen = (chars && *chars) ? *chars : lstrlen(sz);
	while ( len-- ) {
		if (*sz++ == _T('\r'))
			ct--;
	}
	if (chars) *chars = ct;
	return ct - olen;
}
#endif

//Returns number of decoded chars (not including the null terminator)
DWORD DecodeText(LPBYTE* buf, DWORD bytes, DWORD* format, BOOL* bufDirty, LPBYTE* origBuf) {
#ifndef UNICODE
	BOOL bUsedDefault;
#endif
	DWORD chars = bytes;
	LPBYTE newbuf = NULL;
	WORD enc, cp;
	if (!buf || !*buf || !format || !bytes) return 0;
	cp = (*format >> 31 ? (WORD)*format : 0);
	enc = cp ? ID_ENC_CUSTOM : (WORD)*format;
	if (!cp || enc == ID_ENC_ANSI) cp = CP_ACP;
	if (enc == ID_ENC_UTF16 || enc == ID_ENC_UTF16BE) {
		chars /= 2;
		if (enc == ID_ENC_UTF16BE)
			ReverseBytes(*buf, bytes);
#ifndef UNICODE
		if (sizeof(TCHAR) < 2) {
			bytes = WideCharToMultiByte(cp, 0, (LPCWSTR)*buf, chars, NULL, 0, NULL, NULL);
			if (!(newbuf = (LPBYTE)HeapAlloc(globalHeap, 0, bytes+1))) {
				ReportLastError();
				return 0;
			} else if (!WideCharToMultiByte(cp, 0, (LPCWSTR)*buf, chars, (LPSTR)newbuf, bytes, NULL, &bUsedDefault)) {
				ReportLastError();
				ERROROUT(GetString(IDS_UNICODE_CONVERT_ERROR));
				bytes = 0;
			}
			if (bUsedDefault)
				ERROROUT(GetString(IDS_UNICODE_CHARS_WARNING));
		}
#else
	} else if (sizeof(TCHAR) > 1) {
		if (enc == ID_ENC_UTF8)
			cp = CP_UTF8;
		do {
			FREE(newbuf);
			bytes = MultiByteToWideChar(cp, 0, *buf, chars, NULL, 0)*sizeof(TCHAR);
			if (!(newbuf = (LPBYTE)HeapAlloc(globalHeap, 0, bytes+sizeof(TCHAR)))) {
				ReportLastError();
				return 0;
			} else if (!MultiByteToWideChar(cp, MB_ERR_INVALID_CHARS, *buf, chars, (LPWSTR)newbuf, bytes)){
				if (!MultiByteToWideChar(cp, 0, *buf, chars, (LPWSTR)newbuf, bytes)){
					if (enc == ID_ENC_CUSTOM) {
						cp = CP_ACP;
						*format &= 0xfff0000;
						*format |= ID_ENC_ANSI;
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
			if (*bufDirty) {
				if (origBuf) {	FREE(*origBuf); }
				else		 {	FREE(*buf); }
			}
			*bufDirty = TRUE;
		}
		*buf = newbuf;
		if (origBuf) *origBuf = NULL;
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
	enc = cp ? ID_ENC_CUSTOM : (WORD)format;
	if (!cp || enc == ID_ENC_ANSI) cp = CP_ACP;
	if (enc == ID_ENC_UTF16 || enc == ID_ENC_UTF16BE) {
#ifndef UNICODE
		if (sizeof(TCHAR) < 2) {
			bytes = 2 * MultiByteToWideChar(cp, 0, (LPCSTR)*buf, chars, NULL, 0);
			if (!(newbuf = (LPBYTE)HeapAlloc(globalHeap, 0, bytes+1))) {
				ReportLastError();
				return 0;
			} else if (!MultiByteToWideChar(cp, 0, (LPCSTR)*buf, chars, (LPWSTR)newbuf, bytes)) {
				ReportLastError();
				ERROROUT(GetString(IDS_UNICODE_STRING_ERROR));
				bytes = 0;
			}
		}
#else
		bytes = chars * sizeof(TCHAR);
	} else if (sizeof(TCHAR) > 1) {
		if (enc == ID_ENC_UTF8)
			cp = CP_UTF8;
		do {
			FREE(newbuf)
			bytes = WideCharToMultiByte(cp, 0, (LPTSTR)*buf, chars, NULL, 0, NULL, NULL);
			if (!(newbuf = (LPBYTE)HeapAlloc(globalHeap, 0, bytes+1))) {
				ReportLastError();
				return 0;
			} else if (!WideCharToMultiByte(cp, 0, (LPTSTR)*buf, chars, (LPSTR)newbuf, bytes, NULL, (cp != CP_UTF8 && truncated ? truncated : NULL))) {
				if (enc == ID_ENC_CUSTOM) {
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
		if (*bufDirty) FREE(*buf)
		*buf = newbuf;
		*bufDirty = TRUE;
	}
	if (enc == ID_ENC_UTF16BE)
		ReverseBytes(*buf, bytes);
	(*buf)[bytes] = _T('\0');
	return bytes;
}