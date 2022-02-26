/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 * THIS SOFTWARE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED WARRANTY.
 * IN NO EVENT WILL THE AUTHORS BE HELD LIABLE FOR ANY DAMAGES ARISING FROM THE
 * USE OF THIS SOFTWARE.
 * main repo: https://github.com/wangyi-fudan/wyhash
 * author: 王一 Wang Yi <godspeed_china@yeah.net>
 * contributors: Reini Urban, Dietrich Epp, Joshua Haberman, Tommy Ettinger,
 * Daniel Lemire, Otmar Ertl, cocowalla, leo-yuriev, Diego Barrios Romero,
 * paulie-g, dumblob, Yann Collet, ivte-ms, hyb, James Z.M. Gao,
 * easyaspi314 (Devin), TheOneric
 */

/* quick example:
   string s="fjsakfdsjkf";
   uint64_t hash=wyhash(s.c_str(), s.size(), 0, _wyp);
*/

#ifndef wyhash_final_version_3
#define wyhash_final_version_3

#ifndef WYHASH_CONDOM
//protections that produce different results:
//1: normal valid behavior
//2: extra protection against entropy loss (probability=2^-63), aka. "blind multiplication"
#define WYHASH_CONDOM 1
#endif

//includes
#include <stdint.h>
#include <string.h>
#include "hedley.h"
#if defined(_MSC_VER) && defined(_M_X64)
  #include <intrin.h>
  #pragma intrinsic(_umul128)
#endif

#define _likely_(x) HEDLEY_LIKELY(x)
#define _unlikely_(x) HEDLEY_LIKELY(x)

//128bit multiply function
static inline uint64_t _wyrot(uint64_t x)
{
	return (x>>32)|(x<<32);
}
static inline void _wymum(uint64_t *A, uint64_t *B)
{
#if defined(__SIZEOF_INT128__)
	__uint128_t r=*A;
	r*=*B;
# if(WYHASH_CONDOM>1)
	*A^=(uint64_t)r;
	*B^=(uint64_t)(r>>64);
# else
	*A=(uint64_t)r;
	*B=(uint64_t)(r>>64);
# endif
#elif defined(_MSC_VER) && defined(_M_X64)
# if(WYHASH_CONDOM>1)
	uint64_t a, b;
	a=_umul128(*A,*B,&b);
	*A^=a;
	*B^=b;
# else
	*A=_umul128(*A,*B,B);
# endif
#else
	uint64_t ha=*A>>32, hb=*B>>32, la=(uint32_t)*A, lb=(uint32_t)*B, hi, lo;
	uint64_t rh=ha*hb, rm0=ha*lb, rm1=hb*la, rl=la*lb, t=rl+(rm0<<32), c=t<rl;
	lo=t+(rm1<<32);
	c+=lo<t;
	hi=rh+(rm0>>32)+(rm1>>32)+c;
# if(WYHASH_CONDOM>1)
	*A^=lo;
	*B^=hi;
# else
	*A=lo;
	*B=hi;
# endif
#endif
}

//multiply and xor mix function, aka MUM
static inline uint64_t _wymix(uint64_t A, uint64_t B)
{
	_wymum(&A,&B);
	return A^B;
}

//endian macros
#ifndef WYHASH_LITTLE_ENDIAN
  #if defined(_WIN32) || defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define WYHASH_LITTLE_ENDIAN 1
  #elif defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define WYHASH_LITTLE_ENDIAN 0
  #else
    #warning could not determine endianness! Falling back to little endian.
    #define WYHASH_LITTLE_ENDIAN 1
  #endif
#endif

//read functions
#if (WYHASH_LITTLE_ENDIAN)
static inline uint64_t _wyr8(const uint8_t *p) { uint64_t v; memcpy(&v, p, 8); return v;}
static inline uint64_t _wyr4(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v;}
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
static inline uint64_t _wyr8(const uint8_t *p) { uint64_t v; memcpy(&v, p, 8); return __builtin_bswap64(v);}
static inline uint64_t _wyr4(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return __builtin_bswap32(v);}
#elif defined(_MSC_VER)
static inline uint64_t _wyr8(const uint8_t *p) { uint64_t v; memcpy(&v, p, 8); return _byteswap_uint64(v);}
static inline uint64_t _wyr4(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return _byteswap_ulong(v);}
#else
static inline uint64_t _wyr8(const uint8_t *p) {
  uint64_t v; memcpy(&v, p, 8);
  return (((v >> 56) & 0xff) | ((v >> 40) & 0xff00) | ((v >> 24) & 0xff0000) |
		  ((v >>  8) & 0xff000000) |
		  ((v <<  8) & 0xff00000000) |
		  ((v << 24) & 0xff0000000000) |
		  ((v << 40) & 0xff000000000000) |
		  ((v << 56) & 0xff00000000000000));
}
static inline uint64_t _wyr4(const uint8_t *p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return (((v >> 24) & 0xff) |
			((v >>  8) & 0xff00) |
			((v <<  8) & 0xff0000) |
			((v << 24) & 0xff000000));
}
#endif
static inline uint64_t _wyr3(const uint8_t *p, uint64_t k)
{
	return (((uint64_t)p[0])<<16) | (((uint64_t)p[k>>1])<<8) | p[k-1];
}
//wyhash main function
static inline uint64_t wyhash64(const void *key, size_t len, uint64_t seed)
{
	static const uint64_t secret[4] = {
		0xa0761d6478bd642full, 0xe7037ed1a0b428dbull,
		0x8ebc6af09c88c6e3ull, 0x589965cc75374cc3ull
	};
	const uint8_t *p=(const uint8_t *)key;
	uint64_t a, b;
	seed^=*secret;

	if(_likely_(len<=16))
	{
		if(_likely_(len>=4))
		{
			a=(_wyr4(p)<<32)|_wyr4(p+((len>>3)<<2));
			b=(_wyr4(p+len-4)<<32)|_wyr4(p+len-4-((len>>3)<<2));
		}
		else if(_likely_(len>0))
		{
			a=_wyr3(p,len);
			b=0;
		}
		else
			a=b=0;
	}
	else
	{
		uint64_t i=len;
		if(_unlikely_(i>48))
		{
			uint64_t see1=seed, see2=seed;
			do{
				seed=_wymix(_wyr8(p)^secret[1],_wyr8(p+8)^seed);
				see1=_wymix(_wyr8(p+16)^secret[2],_wyr8(p+24)^see1);
				see2=_wymix(_wyr8(p+32)^secret[3],_wyr8(p+40)^see2);
				p+=48; i-=48;
			}while(_likely_(i>48));
			seed^=see1^see2;
		}
		while(_unlikely_(i>16))
		{
			seed=_wymix(_wyr8(p)^secret[1],_wyr8(p+8)^seed);
			i-=16;
			p+=16;
		}
		a=_wyr8(p+i-16);
		b=_wyr8(p+i-8);
	}
	return _wymix(secret[1]^len,_wymix(a^secret[1],b^seed));
}

#ifndef WYHASH32_BIG_ENDIAN
static inline uint32_t _wyr32(const uint8_t* p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return v;
}
#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__)
static inline uint32_t _wyr32(const uint8_t* p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return __builtin_bswap32(v);
}
#elif defined(_MSC_VER)
static inline uint32_t _wyr32(const uint8_t* p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return _byteswap_ulong(v);
}
#endif
static inline uint32_t _wyr24(const uint8_t* p, uint32_t k)
{
	return (((uint32_t)p[0]) << 16) | (((uint32_t)p[k >> 1]) << 8) | p[k - 1];
}
static inline void _wymix32(uint32_t* A, uint32_t* B)
{
	uint64_t  c = *A ^ 0x53c5ca59u;
	c *= *B ^ 0x74743c1bu;
	*A = (uint32_t)c;
	*B = (uint32_t)(c >> 32);
}
// This version is vulnerable when used with a few bad seeds, which should be
// skipped beforehand:
// 0x429dacdd, 0xd637dbf3
static inline uint32_t wyhash32(const void* key, size_t len, uint32_t seed)
{
	const uint8_t* p = (const uint8_t*)key;
	uint64_t i = len;
	uint32_t see1 = (uint32_t)len;

	seed ^= (uint32_t)(len >> 32);
	_wymix32(&seed, &see1);
	for (; i > 8; i -= 8, p += 8) {
		seed ^= _wyr32(p);
		see1 ^= _wyr32(p + 4);
		_wymix32(&seed, &see1);
	}
	if (i >= 4) {
		seed ^= _wyr32(p);
		see1 ^= _wyr32(p + i - 4);
	}
	else if (i) {
		seed ^= _wyr24(p, (uint32_t)i);
	}

	_wymix32(&seed, &see1);
	_wymix32(&seed, &see1);
	return seed ^ see1;
}

#endif
