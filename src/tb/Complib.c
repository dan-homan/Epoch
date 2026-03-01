#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC CLK_TCK
#endif

#define const
/* I can read and write anything :-)	*/
#include "complib.h"
#undef const

/* -------------------------------------------------------------------- */
/*                                                                      */
/*                Compress/decompress some chess tables                 */
/*                                                                      */
/*               Copyright (c) 1991--1998 Andrew Kadatch                */
/*                                                                      */
/* The Limited-Reference  variant  of  Lempel-Ziv algorithm implemented */
/* here was first described in  my  B.Sc.  thesis "Efficient algorithms */
/* for image  compression",  Novosibirsk  State  University,  1992, and */
/* cannot be  used in any product distributed in  Russia or CIS without */
/* written permission from the author.                                  */
/*                                                                      */
/* Most of the code listed below is significantly  simplified code from  */
/* the PRS data compression library and therefore it should not be used */
/* in any product (software or hardware, commercial or not, and  so on) */
/* without written permission from the author.                          */
/*                                                                      */
/* -------------------------------------------------------------------- */


/* ---------------------------- Debugging ----------------------------- */
/*                              ---------                               */

#ifndef DEBUG
#define DEBUG	1
#endif

#define VERIFY	1	/* verify decoding while encoding?      */
			/* if 1 then force DEBUG and DECODE     */
			/* NB: makes encoding 10% slower        */

#if DEBUG
#define assert(cond) ((cond) ? (void) 0 : _local_assert (__LINE__))
static void _local_assert (int lineno)
{
  fprintf (stderr, "assertion at line %u failed\n", lineno);
  exit (33);
}
#define debug(x) x
#define dprintf(x) printf x
#else
#define assert(cond) ((void) 0)
#define debug(x)     ((void) 0)
#define dprintf(x)   ((void) 0)
#endif

/* --------------------- Constants, types, etc. ----------------------- */
/*                       ----------------------                         */

#define MIN_BLOCK_BITS	8	/* LOG2 (min size of block to compress)	*/
#define MAX_BLOCK_BITS	16	/* LOG2 (max size of block to compress) */

			/* max. integer we can take LOG2 by table	*/
#define MAX_BITS_HALF	((MAX_BLOCK_BITS + 1) >> 1)
#define MAX_BITS	(MAX_BITS_HALF * 2)

/* assume that integer is at least 32 bits wide */
#ifndef uint
#define uint unsigned
#endif

#ifndef uchar
#define uchar unsigned char
#endif

#if DEBUG || 1
#  define ENCODE 1
#  define DECODE 1
#else
#  define ENCODE COMP_ENCODE
#  define DECODE COMP_DECODE
#endif

#define HEADER_SIZE		80	/* number of reserved bytes	*/
#define STOP_SEARCH_LENGTH	256	/* terminate search if match	*/
					/* length exceeds that value	*/

#define MAX_LENGTH_BITS		5
#define MAX_LENGTH              (1 << MAX_LENGTH_BITS)
#define LONG_BITS               1
#define LONG_LENGTH		(MAX_BLOCK_BITS - LONG_BITS)
#define LONG_QUICK		(MAX_LENGTH - LONG_LENGTH)

#if LONG_LENGTH > (MAX_BLOCK_BITS - LONG_BITS)
#  undef LONG_LENGTH
#  define LONG_LENGTH		(MAX_BLOCK_BITS - LONG_BITS)
#endif

#if LONG_LENGTH >= MAX_LENGTH || LONG_LENGTH <= 0
#error LONG_LENGTH is out of range
#endif

#if LONG_BITS <= 0
#error LONG_BITS must be positive
#endif

#define DELTA	(LONG_BITS + LONG_QUICK - 1)
#if (MAX_LENGTH - 1) - (LONG_LENGTH - LONG_BITS) != DELTA
#error Hmmm
#endif

#define MAX_DISTANCES		24

#define LOG_MAX_DISTANCES	6	/* see check below	*/

#if MAX_DISTANCES > (1 << LOG_MAX_DISTANCES)
#error MAX_DISTANCES should not exceed (1 << LOG_MAX_DISTANCES)
#endif

#define ALPHABET_SIZE		(256 + (MAX_DISTANCES << MAX_LENGTH_BITS))
#define MAX_ALPHABET	ALPHABET_SIZE	/* max. alphabet handled by	*/
					/* Huffman coding routines	*/

#define USE_CRC32		1
/* 0 - use Fletcher's checksum, != 0 - use proper CRC32			*/

#if VERIFY
#  undef  DECODE
#  define DECODE 1
#  undef  DEBUG
#  define DEBUG 1
#endif

static uchar header_title[64] =
"Compressed by DATACOMP v 1.0 (c) 1991--1998 Andrew Kadatch\r\n\0";

#define RET(n) ((n) + __LINE__ * 256)


/* ------------------------- CRC32 routines --------------------------- */
/*                           --------------                             */

#if USE_CRC32

static unsigned CRC32_table[256];
static int CRC32_initialized = 0;

static void CRC32_init (void)
{
  int i, j;
  unsigned k, m = (unsigned) 0xedb88320L;
  if (CRC32_initialized) return;
  for (i = 0; i < 256; ++i)
  {
    k = i; j = 8;
    do
    {
      if ((k&1) != 0)
	k >>= 1;
      else
      {
	k >>= 1;
	k ^= m;
      };
    }
    while (--j);
    CRC32_table[i] = k;
  }
  CRC32_initialized = 1;
}

static unsigned CRC32 (uchar *p, int n, unsigned k)
{
  unsigned *table = CRC32_table;
  uchar *e = p + n;
  while (p + 16 < e)
  {
#   define X(i) k = table[((uchar) k) ^ p[i]] ^ (k >> 8)
    X(0); X(1); X(2); X(3); X(4); X(5); X(6); X(7); X(8);
    X(9); X(10); X(11); X(12); X(13); X(14); X(15);
#   undef X
    p += 16;
  }
  while (p < e)
    k = table[((uchar) k) ^ *p++] ^ (k >> 8);
  return (k);
}

#else

#define CRC32_init()

static unsigned CRC32 (uchar *p, int n, unsigned k1)
{
  unsigned k0 = k1 & 0xffff;
  uchar *e = p + n;
  k1 = (k1 >> 16) & 0xffff;
  while (p + 16 < e)
  {
#   define X(i) k0 += p[i]; k1 += k0;
    X(0); X(1); X(2); X(3); X(4); X(5); X(6); X(7); X(8);
    X(9); X(10); X(11); X(12); X(13); X(14); X(15);
#   undef X
    k0 = (k0 & 0xffff) + (k0 >> 16);
    k1 = (k1 & 0xffff) + (k1 >> 16);
    p += 16;
  }
  while (p < e)
  {
    k0 += *p++;
    k1 += k0;
  }
  k0 = (k0 & 0xffff) + (k0 >> 16);
  k1 = (k1 & 0xffff) + (k1 >> 16);
  k0 = (k0 & 0xffff) + (k0 >> 16);
  k1 = (k1 & 0xffff) + (k1 >> 16);
  assert (((k0 | k1) >> 16) == 0);
  return (k0 + (k1 << 16));
}

#endif   /* USE_CRC32    */



/* ------------------------ Bit IO interface -------------------------- */
/*                          ----------------                            */

#define BITIO_LOCALS	\
  uint   _mask;         \
  int    _bits;         \
  uchar *_ptr

typedef struct
{
  BITIO_LOCALS;
}
bitio_t;

#define BITIO_ENTER(p) do {     \
  _mask = (p)._mask;            \
  _bits = (p)._bits;            \
  _ptr  = (p)._ptr;             \
} while (0)

#define BITIO_LEAVE(p) do {     \
  (p)._mask = _mask;            \
  (p)._bits = _bits;            \
  (p)._ptr  = _ptr;             \
} while (0)

#define BIOWR_START(from) do {	\
  _bits = 32;                   \
  _mask = 0;                    \
  _ptr = (uchar *) (from);      \
} while (0)

/* write 0..32 bits */
#define BIOWR(mask,bits) do {			\
  assert ((((uint) (mask)) >> (bits)) == 0 || (bits) == 8 * sizeof (uint));	\
  if ((_bits -= bits) >= 0)             	\
    _mask = (_mask << (bits)) | (mask);     	\
  else                                  	\
  {                                     	\
    _mask = (_mask << ((bits) + _bits)) | (((uint)(mask)) >> (-_bits));	\
    _ptr[0] = _mask >> 24; _ptr[1] = _mask >> 16;               \
    _ptr[2] = _mask >>  8; _ptr[3] = _mask; _ptr += 4;          \
    _bits += 32;			\
    _mask = mask;                       \
  }                                     \
} while (0)

#define BIOWR_TELL(from) \
  (((int)(_ptr - (uchar *) (from)) << 3) + 32 - _bits)

#define BIOWR_FINISH() do {		\
  if (_bits >= 32) break;		\
  _mask <<= _bits;			\
  do {					\
    _ptr[0] = _mask >> 24;              \
    if (_bits >= 24) break;		\
    _ptr[1] = _mask >> 16;		\
    if (_bits >= 16) break;		\
    _ptr[2] = _mask >>  8;		\
    if (_bits >=  8) break;		\
    _ptr[3] = _mask;			\
  } while (0);				\
  _mask >>= _bits;			\
} while (0)

#define BIORD_START(from) do {		\
  _ptr = (uchar *) (from);              \
  _bits = sizeof (_mask);               \
  _mask = 0;                            \
  do                                    \
    _mask = (_mask << 8) | *_ptr++;     \
  while (--_bits != 0);                 \
  _bits = 16;                           \
} while (0)

/* read [1, 17] bits at once */
#define BIORD(bits)      \
  (_mask >> (8 * sizeof (_mask) - (bits)))

#define BIORD_MORE(bits) do {		\
  _mask <<= (bits);			\
  if ((_bits -= (bits)) <= 0)           \
  {                                     \
    _mask |= ((_ptr[0] << 8) + _ptr[1]) << (-_bits);	\
    _ptr += 2; _bits += 16;		\
  }					\
} while (0)

#define BIORD_TELL1(from) \
  (((int)(_ptr - (uchar *) (from) - sizeof (_mask)) << 3) + 16 - _bits)

/* ------------------------ Find LOG2 number -------------------------- */
/*                          ---------------                             */

#if ENCODE

static uchar LOG2_table[1 << MAX_BITS_HALF];
static int   LOG2_initialized = 0;

#define LOG2(n) \
  ((((uint) (n)) >= (1 << MAX_BITS_HALF)) \
    ? (assert (MAX_BITS >= 8*sizeof (uint) || (((uint) n) >> MAX_BITS) == 0), \
      MAX_BITS_HALF + LOG2_table[(n) >> MAX_BITS_HALF]) \
    : LOG2_table[n])

static void LOG2_init (void)
{
  int i;
  if (LOG2_initialized)
    return;
  LOG2_table[0] = 0xff;
  for (i = 0; i < MAX_BITS_HALF; ++i)
    memset (LOG2_table + (1 << i), i, 1 << i);
  LOG2_initialized = 1;
}

#endif	/* ENCODE */


/* ------------------------ Huffman coding ---------------------------- */
/*                          --------------                              */

#if MAX_ALPHABET <= 0xffff
#  if MAX_ALPHABET <= 1024
/* positive value takes 15 bits => symbol number occupies <= 10 bits	*/
#    define huffman_decode_t	short
#    define huffman_encode_t	unsigned short
#  else
#    define huffman_decode_t	int
#    define huffman_encode_t	unsigned short
#  endif
#else
#  define huffman_decode_t	int
#  define huffman_encode_t	int
#endif


#if DECODE

#define HUFFMAN_DECODE(ch,table,start_bits) do {	\
  (ch) = table[BIORD (start_bits)];                     \
  if (((int) (ch)) >= 0)                                \
  {                                                     \
    BIORD_MORE ((ch) & 31);                             \
    (ch) >>= 5;                                         \
    break;                                              \
  }                                                     \
  BIORD_MORE (start_bits);                              \
  do                                                    \
  {                                                     \
    (ch) = table[BIORD (1) - (ch)];                     \
    BIORD_MORE (1);                                     \
  }                                                     \
  while (((int) (ch)) < 0);                             \
} while (0)

#define HUFFMAN_TABLE_SIZE(n,start_bits) \
  ((1 << (start_bits)) + ((n) << 1))

static int huffman_decode_create
  (huffman_decode_t *table, uchar *length, int n, int start_bits)
{
  int i, j, k, last, freq[32], sum[32];

  /* calculate number of codewords					*/
  memset (freq, 0, sizeof (freq));
  for (i = 0; i < n; ++i)
  {
    if ((k = length[i]) > 31)
      return RET (COMP_ERR_BROKEN);
    ++freq[k];
  }

  /* handle special case(s) -- 0 and 1 symbols in alphabet		*/
  if (freq[0] == n)
  {
    memset (table, 0, sizeof (table[0]) << start_bits);
    return (0);
  }
  if (freq[0] == n-1)
  {
    if (freq[1] != 1)
      return RET (COMP_ERR_BROKEN);
    for (i = 0; length[i] == 0;) ++i;
    i <<= 5;
    for (k = 1 << start_bits; --k >= 0;)
      *table++ = i;
    return (0);
  }

  /* save frequences			*/
  memcpy (sum, freq, sizeof (sum));

  /* check code correctness		*/
  k = 0;
  for (i = 32; --i != 0;)
  {
    if ((k += freq[i]) & 1)
      return RET (COMP_ERR_BROKEN);
    k >>= 1;
  }
  if (k != 1)
    return RET (COMP_ERR_BROKEN);

  /* sort symbols		*/
  k = 0;
  for (i = 1; i < 32; ++i)
    freq[i] = (k += freq[i]);
  last = freq[31];	/* preserve number of symbols in alphabet	*/
  for (i = n; --i >= 0;)
  {
    if ((k = length[i]) != 0)
      table[--freq[k]] = i;
  }

  /* now create decoding table	*/
  k = i = (1<<start_bits) + (n<<1);
  for (n = 32; --n > start_bits;)
  {
    j = i;
    while (k > j)
      table[--i] = -(k -= 2);
    for (k = sum[n]; --k >= 0;)
      table[--i] = table[--last];
    k = j;
  }

  j = i;
  i = 1 << start_bits;
  while (k > j)
    table[--i] = -(k -= 2);

  for (; n > 0; --n)
  {
    for (k = sum[n]; --k >= 0;)
    {
      assert (last <= i && last > 0);
      j = i - (1 << (start_bits - n));
      n |= table[--last] << 5;
      do
	table[--i] = n;
      while (i != j);
      n &= 31;
    }
  }
  assert ((i | last) == 0);

  return (0);
}

#endif /* DECODE */


#if ENCODE

/* create canonical Huffman code		*/
static int x_huffman_create_codes (uint *freq, int n, uint *mask, uchar *length)
{
  struct huff_node_t
  {
    struct huff_node_t *son[2];
    uint freq;
    huffman_encode_t ch;
    unsigned short bits;
  } *buff, *p, *q, *r, *first_sorted, *first_free, *head[256], **link[256];
  int i, k;

  /* allocate memory */
  if ((uint) (n-1) >= MAX_ALPHABET || (buff = malloc (n * (2 * sizeof (*p)))) == 0)
    return (-2);	/* error */
  /* honestly it is easy enough to create Huffman code in-place */
  /* but the use of explicit data structures makes code simpler */

  /* clean everything up		*/
  memset (length, 0, sizeof (length[0]) * n);
  if (mask != 0 && mask != freq)
    memset (mask, 0, sizeof (mask[0]) * n);

  /* store frequencies */
  p = buff;
  for (i = 0; i < n; ++i)
  {
    if ((p->freq = freq[i]) != 0)
    {
      p->son[0] = p+1; p->son[1] = 0;
      p->ch = i;
      ++p;
    }
  }

  /* handle simple case		*/
  if (p <= buff + 1)
  {
    if (p == buff)
      i = 0;		/* no symbols	*/
    else
      i = p[-1].ch + 1;	/* symbol code	*/
    free (buff);
    return (i);
  }
  first_free = p;	/* store location of first unused node	*/

  p[-1].son[0] = 0;	/* terminate the list			*/
  /* radix sort the list by frequency */
  p = buff;		/* head of the list			*/
  /* initialize */
  for (n = 0; n < 256; ++n)
    *(link[n] = head + n) = 0;
  for (i = 0; i < 8 * sizeof (p->freq); i += 8)
  {
    /* link node to the end of respective bucket	*/
    do
    {
      n = (p->freq >> i) & 0xff;
      link[n][0] = p; link[n] = p->son;
    }
    while ((p = p->son[0]) != 0);

    /* merge buckets into single list			*/
    n = 0;
    while (head[n] == 0) ++n;
    p = head[n]; head[k = n] = 0;
    while (++n < 256)
    {
      if (head[n] == 0) continue;
      link[k][0] = head[n]; link[k] = head + k; head[n] = 0;
      k = n;
    }
    link[k][0] = 0; link[k] = head + k;
  }
  first_sorted = p;      /* store head of sorted symbol's list   */

restart:
  assert (p == first_sorted);
  q = first_free;
  r = q - 1;
  while (p != 0 || q != r)
  {
    ++r;

    /* select left subtree	*/
    assert (q <= r && (p != 0 || q != r));
    if (p == 0 || (q != r && p->freq > q->freq))
    {
      r->son[0] = q; r->freq = q->freq; ++q;
    }
    else
    {
      r->son[0] = p; r->freq = p->freq; p = p->son[0];
    }

    /* select right subtree	*/
    assert (q <= r && (p != 0 || q != r));
    if (p == 0 || (q != r && p->freq > q->freq))
    {
      r->son[1] = q; r->freq += q->freq; ++q;
    }
    else
    {
      r->son[1] = p; r->freq += p->freq; p = p->son[0];
    }
  }

  /* evaluate codewords' length		*/
  i = -1; 	/* stack pointer	*/
  n = 0;	/* current tree depth	*/
  p = r;	/* current subtree root	*/
  do
  {
    while (p->son[1] != 0)
    {
      /* put right son into stack and set up its depth   */
      (head[++i] = p->son[1])->bits = ++n;
      (p = p->son[0])->bits = n;
    }
    length[p->ch] = n;
    if (i < 0) break;	/* nothing's in stack			*/
    n = (p = head[i--])->bits;
  }
  while (1);

  p = first_sorted;
#if DEBUG
  for (q = p; (r = q->son[0]) != 0; q = r)
    assert (q->bits >= r->bits);
#endif
  if (p->bits > 15)
  {
    /* I am lazy bastard -- package-merge algorithms is too complicated */
    /* so I'll simply scale frequences down; sooner or later maximal	*/
    /* bit length will not exceed 31					*/
    /* In the production-quality library I used [-1 +3 -2] transform	*/
    /* but as long as 31-bit boundary is unlikely to be reached it is	*/
    /* not necessary to bother about better [but complicated] solutions */
    assert (p == first_sorted);
    q = p;
    do
      q->freq = (q->freq + 1) >> 1;
    while ((q = q->son[0]) != 0);
    goto restart;
  }

  /* now sort symbols in a stable way by increasing codeword length	*/
  /* initialize */
  memset (head, 0, sizeof (head[0]) * 32);
  for (n = 0; n < 32; ++n)
    link[n] = head + n;

  /* link node to the end of respective bucket	*/
  p = buff;
  do
  {
    n = p->bits;
    link[n][0] = p; link[n] = p->son;
  }
  while (++p != first_free);

  /* merge buckets into single list		*/
  n = 0;
  while (head[n] == 0) ++n;
  p = head[n]; k = n;
  while (++n < 32)
  {
    if (head[n] == 0) continue;
    link[k][0] = head[n];
    k = n;
  }
  link[k][0] = 0;

#if DEBUG
  for (q = p; (r = q->son[0]) != 0; q = r)
    assert (r->bits > q->bits || (r->bits == q->bits && r->ch > q->ch));
#endif

  /* set up code masks		*/
  if (mask == 0)
    goto ret;
  if (mask == freq)
    memset (mask, 0, sizeof (mask[0]) * n);

  n = 0;	/* mask		*/
  i = 1;	/* bit length	*/
  do
  {
    mask[p->ch] = (n <<= p->bits - i);
    i = p->bits;
    ++n;
  }
  while ((p = p->son[0]) != 0);

ret:
  free (buff);		/* release memory				*/
  return (-1);		/* 2 or more non-zero frequency symbols		*/
}

#endif /* ENCODE */

#if VERIFY
static int huffman_create_codes (uint *freq, int n, uint *mask, uchar *length)
{
  BITIO_LOCALS;
  int r, ch, ch1, m, start_bits = 14;
  huffman_decode_t *table;
  uchar buff[32];

  r = x_huffman_create_codes (freq, n, mask, length);
  if (r < -1) return (n);

  if ((table = malloc (sizeof (table[0]) * HUFFMAN_TABLE_SIZE (n, start_bits))) == 0)
  {
    fprintf (stderr, "warning: not enough memory to verify huffman code\n");
    return (r);
  }

  for (; start_bits >= 1; --start_bits)
  {
    m = huffman_decode_create (table, length, n, start_bits);
    if (m != 0)
    {
      fprintf (stderr, "cannot create decode table: returned %d\n", m);
      continue;
    }
    m = 0x7fff;
    for (ch = 0; ch < n; ch += m & 1)
    {
      m ^= 0x7fff;
      if (length[ch] == 0)
      {
	assert (mask[ch] == 0);
	continue;
      }
      BIOWR_START (buff);
      BIOWR (mask[ch], length[ch]);
      BIOWR (m, 15);
      BIOWR_FINISH ();
      BIORD_START (buff);
      HUFFMAN_DECODE (ch1, table, start_bits);
      if (ch1 != ch)
	fprintf (stderr, "error: incorrectly decoded symbol %2d for %2d-bit table (got symbol %2d)\n", ch, start_bits, ch1);
      if (BIORD_TELL1 (buff) != length[ch])
	fprintf (stderr, "error: incorrectly decoded symbol %2d for %2d-bit table (read %2d bits instead of %2d)\n", ch, start_bits, BIORD_TELL1 (buff), length[ch]);
    }
  }

  free (table);
  return (r);
}
#else
#  define huffman_create_codes(freq,n,mask,bits) \
	x_huffman_create_codes((freq),(n),(mask),(bits))
#endif /* VERIFY */


/* -------------------- Read/write Huffman code ----------------------- */
/*                      -----------------------                         */

#define MIN_REPT	2

#if MIN_REPT <= 1
#error MIN_REPT must exceed 1
#endif

#if DECODE

#define TEMP_TABLE_BITS 8

static int huffman_read_length (bitio_t *bitio, uchar *length, int n)
{
  BITIO_LOCALS;
  huffman_decode_t table[2][HUFFMAN_TABLE_SIZE (64, TEMP_TABLE_BITS)];
  uchar bits[128];
  int i, j, k;

  BITIO_ENTER (*bitio);
  k = BIORD (1); BIORD_MORE (1);
  if (k != 0)
  {
    memset (length, 0, n);
    goto ret;
  }

  if (n <= 128)
  {
    k = BIORD (5); BIORD_MORE (5);
    for (i = 0; i < n;)
    {
      length[i] = BIORD (k); BIORD_MORE (k);
      if (length[i++] == 0)
      {
	j = i + BIORD (4); BIORD_MORE (4);
	if (j > n)
	  return RET (COMP_ERR_BROKEN);
	while (i != j)
	  length[i++] = 0;
      }
    }
    goto ret;
  }

  BITIO_LEAVE (*bitio);
  i = huffman_read_length (bitio, bits, 128);
  if (i != 0)
    return (i);
  i = huffman_decode_create (table[0], bits,      64, TEMP_TABLE_BITS);
  if (i != 0)
    return (i);
  i = huffman_decode_create (table[1], bits + 64, 64, TEMP_TABLE_BITS);
  if (i != 0)
    return (i);
  BITIO_ENTER (*bitio);

  for (i = 0; i < n;)
  {
    HUFFMAN_DECODE (k, table[0], TEMP_TABLE_BITS);
    if (k <= 31)
    {
      length[i++] = k;
      continue;
    }
    k &= 31;
    HUFFMAN_DECODE (j, table[1], TEMP_TABLE_BITS);
    if (j > 31)
    {
      int jj = j - 32;
      j = 1 << jj;
      if (jj != 0)
      {
	if (jj > 16)
	{
	  j += BIORD (16) << (jj - 16); BIORD_MORE (16);
	}
	j += BIORD (jj); BIORD_MORE (jj);
      }
      j += 31;
    }
    j += MIN_REPT + i;
    if (j > n)
      return RET (COMP_ERR_BROKEN);
    do
      length[i] = k;
    while (++i != j);
  }

ret:
  BITIO_LEAVE (*bitio);
  return (0);
}

#endif /* DECODE */


#if VERIFY
static int x_huffman_write_length (bitio_t *bitio, uchar *length, int n);
static int   huffman_write_length (bitio_t *bitio, uchar *length, int n)
{
  bitio_t Bitio;
  BITIO_LOCALS;
  uchar *bits, *_ptr2;
  int i, k;

  Bitio = *bitio;
  BITIO_ENTER (Bitio);
  k = BIOWR_TELL (_ptr);
  _ptr2 = _ptr + (k >> 3); k &= 7;

  i = x_huffman_write_length (bitio, length, n);
  if (i != 0)
  {
    fprintf (stderr, "huffman_write_length: returned %d\n", i);
    return (i);
  }
  if ((bits = malloc (n)) == 0)
  {
    fprintf (stderr, "huffman_write_length: not enough memory\n");
    return (i);
  }
  BITIO_ENTER (*bitio);
  BIOWR_FINISH ();
  BIORD_START (_ptr2);
  BIORD_MORE (k);
  BITIO_LEAVE (Bitio);
  i = huffman_read_length (&Bitio, bits, n);
  if (i != 0)
    fprintf (stderr, "huffman_read_length: returned %d\n", i);
  if (memcmp (bits, length, n) != 0)
  {
    i = 0; while (bits[i] == length[i]) ++i;
    fprintf (stderr, "huffman_write_length: difference at %2d: %2d != %2d\n", i, length[i], bits[i]);
  }
  free (bits);
  return (0);
}
#else
#  define huffman_write_length(bitio,length,n) \
	x_huffman_write_length((bitio),(length),(n))
#endif /* VERIFY */


#if ENCODE

/* encode huffman table */
static int x_huffman_write_length (bitio_t *bitio, uchar *length, int n)
{
  BITIO_LOCALS;
  uint  freq[128];
  uint  mask[128];
  uchar bits[128];
  int i, k;
  uint m;

  for (i = k = 0; i < n; ++i)
    if (length[i] > k)
      k = length[i];
  assert (n > 0 && k <= n-1 && k <= 31);

  BITIO_ENTER (*bitio);

  if (k == 0)
  {
    BIOWR (1, 1);
    goto ret;
  }
  BIOWR (0, 1);

  if (n <= 128)
  {
    k = LOG2 (k) + 1;
    BIOWR (k, 5);
    for (i = 0; i < n;)
    {
      m = length[i++];
      BIOWR (m, k);
      if (m == 0)
      {
	while (i < n && m < 15 && length[i] == 0) ++i, ++m;
	BIOWR (m, 4);
      }
    }
    goto ret;
  }

  memset (freq, 0, sizeof (freq));
  for (i = 0; i < n;)
  {
    m = length[k = i];
    do ++i; while (i < n && length[i] == m);
    k = i - k;
    if (k < MIN_REPT)
      freq[m] += k;
    else
    {
      ++freq[m + 32];
      if ((k -= MIN_REPT) <= 31)
	++freq[k + 64];
      else
      {
	k -= 31;
	++freq[LOG2 (k) + 96];
      }
    }
  }

  i = huffman_create_codes (freq,      64, mask,      bits);
  k = huffman_create_codes (freq + 64, 64, mask + 64, bits + 64);
  if (i < -1 || k < -1)
    return RET (COMP_ERR_INTERNAL);
  if (i > 0) ++bits[i - 1];		/* singlet */
  if (k > 0) ++bits[k + 64 - 1];
  BITIO_LEAVE (*bitio);
  huffman_write_length (bitio, bits, 128);
  BITIO_ENTER (*bitio);
  if (k > 0) ++bits[k + 64 - 1];
  if (i > 0) ++bits[i - 1];

  for (i = 0; i < n;)
  {
    int c;
    m = length[k = i];
    do ++i; while (i < n && length[i] == m);
    k = i - k;
    if (k < MIN_REPT)
    {
      c = bits[m];
      m = mask[m];
      do BIOWR (m, c); while (--k != 0);
    }
    else
    {
      m += 32;
      c = bits[m];
      m = mask[m];
      BIOWR (m, c);
      if ((k -= MIN_REPT) <= 31)
      {
	k += 64;
	c = bits[k];
	m = mask[k];
      }
      else
      {
	m = (k -= 31);
	c = k = LOG2 (k);
	m -= 1 << k;
	k += 96;
	BIOWR (mask[k], bits[k]);
      }
      BIOWR (m, c);
    }
  }

ret:
  BITIO_LEAVE (*bitio);
  return (0);
}

#endif /* ENCODE */



/* ----------------------- Proper compression ------------------------- */
/*                         ------------------                           */

#if MIN_BLOCK_BITS > MAX_BLOCK_BITS || MAX_BLOCK_BITS > MAX_BITS_HALF*2
#error condition MIN_BLOCK_BITS <= MAX_BLOCK_BITS <= MAX_BITS_HALF*2 failed
#endif

#define DECODE_MAGIC    ((int) 0x5abc947fL)
#define ENCODE_MAGIC    ((int) 0x63ab4d92L)
#define BLOCK_MAGIC     ((int) 0x79a3f29dL)

#if DECODE

#define START_BITS      13

typedef struct
{
  huffman_decode_t table[HUFFMAN_TABLE_SIZE (ALPHABET_SIZE, START_BITS)];
  int distance[MAX_DISTANCES];
  FILE *fd;
  unsigned *crc, *blk;
  int
    block_size_log,     /* block_size is integral power of 2    */
    block_size,         /* 1 << block_size_log                  */
    last_block_size,    /* [original] size of last block        */
    n_blk,              /* total number of blocks               */
    comp_block_size,	/* size of largest compressed block+32	*/
    check_crc;          /* check CRC32?                         */
  volatile int locked, blocks;
  uchar *comp;
  char *name;
  int magic;
}
decode_info;

typedef struct
{
  struct COMP_BLOCK_T b;
  struct
  {
    uchar *first;
    int size;
  } orig, comp;
  struct
  {
    uchar *ptr, *src;
    int rept;
  } emit;
  bitio_t bitio;
  decode_info *parent;
  int n;
  volatile int locked;
  int magic;
}
decode_block;

static void do_decode (decode_info *info, decode_block *block, uchar *e)
{
  BITIO_LOCALS;
  uchar *p, *s;
  int ch;

  if ((p = block->emit.ptr) >= e)
    return;
  if (p == block->orig.first)
  {
    BIORD_START (block->comp.first);
    block->emit.rept = 0;
  }
  else
  {
    BITIO_ENTER (block->bitio);
    if ((ch = block->emit.rept) != 0)
    {
      block->emit.rept = 0;
      s = block->emit.src;
      goto copy;
    }
  }
#define OVER if (p < e) goto over; break
  do
  {
  over:
    HUFFMAN_DECODE (ch, info->table, START_BITS);
    if ((ch -= 256) < 0)
    {
      *p++ = ch;
      OVER;
    }

    s = p + info->distance[ch >> MAX_LENGTH_BITS];

    ch &= MAX_LENGTH - 1;
    if (ch >= LONG_LENGTH)
    {
      ch -= LONG_LENGTH - LONG_BITS;
#if (MAX_BLOCK_BITS - 1) + (LONG_LENGTH - LONG_BITS) >= MAX_LENGTH
      if (ch == DELTA)
      {
	ch = BIORD (5); BIORD_MORE (5);
	ch += DELTA;
      }
#endif
      {
	int n = 1 << ch;
	if (ch > 16)
	{
	  n += BIORD (16) << (ch -= 16); BIORD_MORE (16);
	}
	n += BIORD (ch); BIORD_MORE (ch);
	ch = n;
      }
      ch += LONG_LENGTH - (1 << LONG_BITS);
    }
    else if (ch == 0)
    {
      *p++ = *s; OVER;
    }
    else if (ch == 1)
    {
      p[0] = s[0]; p[1] = s[1]; p += 2; OVER;
    }
    else if (ch == 2)
    {
      p[0] = s[0]; p[1] = s[1]; p[2] = s[2]; p += 3; OVER;
    }
    else if (ch == 3)
    {
      p[0] = s[0]; p[1] = s[1]; p[2] = s[2]; p[3] = s[3]; p += 4; OVER;
    }
    ++ch;
  copy:
    if (ch > 16)
    {
      if (p + ch > e)
      {
	block->emit.rept = ch - (int) (e - p);
	ch = (int) (e - p);
	goto copy;
      }
      do
      {
#       define X(i) p[i] = s[i]
	X(0); X(1); X(2); X(3); X(4); X(5); X(6); X(7); X(8);
	X(9); X(10); X(11); X(12); X(13); X(14); X(15);
#       undef X
	p += 16; s += 16;
      }
      while ((ch -= 16) > 16);
    }
    p += ch; s += ch;
    switch (ch)
    {
#     define X(i) case i: p[-i] = s[-i]
      X(16); X(15); X(14); X(13); X(12); X(11); X(10);
      X(9); X(8); X(7); X(6); X(5); X(4); X(3); X(2);
#     undef X
    }
    p[-1] = s[-1];
  }
  while (p < e);
#undef OVER
  block->emit.ptr = p;
  block->emit.src = s;
  BITIO_LEAVE (block->bitio);
}

/* pretty ugly */
int comp_open_file (COMP_FILE *res, char *name, int check_crc)
{
  BITIO_LOCALS;
  bitio_t Bitio;
  FILE *fd;
  uchar temp[ALPHABET_SIZE >= HEADER_SIZE ? ALPHABET_SIZE : HEADER_SIZE];
  uchar *ptr;
  int header_size, block_size, block_size_log, n_blk, i, n;
  decode_info *info;

  if (res == 0)
    return RET (COMP_ERR_PARAM);

  CRC32_init ();

  *res = 0;

  if ((fd = fopen (name, "rb")) == 0)
    return RET (COMP_ERR_OPEN);

  if (fread (temp, 1, HEADER_SIZE, fd) != HEADER_SIZE)
  {
    fclose (fd);
    return RET (COMP_ERR_READ);
  }

  if (memcmp (temp, header_title, 64) != 0)
  {
    fclose (fd);
    return RET (COMP_ERR_READ);
  }

  ptr = temp;
#define R4(i) \
  ((ptr[i] << 24) + (ptr[(i) + 1] << 16) + (ptr[(i) + 2] << 8) + (ptr[(i) + 3]))

  header_size = R4 (64);
  block_size_log = ptr[70];
  if (block_size_log > MAX_BITS || header_size < 84)
  {
    fclose (fd);
    return RET (COMP_ERR_BROKEN);
  }
  block_size = 1 << block_size_log;
  if (ptr[71] != MAX_DISTANCES)
  {
    fclose (fd);
    return RET (COMP_ERR_BROKEN);
  }
  n_blk = R4 (72);
  if (R4 (76) != (ALPHABET_SIZE << 12) + (LONG_BITS << 8) + (LONG_LENGTH << 4) + MAX_LENGTH_BITS)
  {
    fclose (fd);
    return RET (COMP_ERR_BROKEN);
  }

  if ((ptr = malloc (header_size)) == 0)
  {
    fclose (fd);
    return RET (COMP_ERR_NOMEM);
  }

  if (fread (ptr + HEADER_SIZE, 1, header_size - HEADER_SIZE, fd) != header_size - HEADER_SIZE)
  {
    free (ptr);
    fclose (fd);
    return RET (COMP_ERR_NOMEM);
  }
  memcpy (ptr, temp, HEADER_SIZE);
  header_size -= 4;
  if (CRC32 (ptr, header_size, 0) != R4 (header_size))
  {
    free (ptr);
    fclose (fd);
    return RET (COMP_ERR_BROKEN);
  }
  header_size += 4;

  n = sizeof (info->crc[0]) * (1 + (check_crc ? (3 * n_blk) : n_blk));
  if ((info = malloc (sizeof (*info) + n + strlen (name) + 1)) == 0)
  {
    free (ptr);
    fclose (fd);
    return RET (COMP_ERR_NOMEM);
  }
  info->name = (char *) (info + 1) + n;
  strcpy (info->name, name);

  info->blk = info->crc = (unsigned *) (info + 1);
  if (check_crc) info->blk += 2 * n_blk;

  info->check_crc = check_crc;
  info->block_size_log = block_size_log;
  info->block_size = block_size;
  info->n_blk = n_blk;
  info->fd = fd;

  if (check_crc)
  {
    n_blk <<= 1; i = HEADER_SIZE;
    for (n = 0; n < n_blk; ++n)
    {
      info->crc[n] = R4 (i);
      i += 4;
    }
    n_blk >>= 1;
  }

  i = HEADER_SIZE + (n_blk << 3);
  BIORD_START (ptr + i);

  info->comp_block_size = 0;
  for (n = 0; n <= n_blk; ++n)
  {
    if ((info->blk[n] = BIORD (block_size_log)) == 0)
      info->blk[n] = block_size;
    if (info->comp_block_size < info->blk[n])
      info->comp_block_size = info->blk[n];
    BIORD_MORE (block_size_log);
  }
  info->comp_block_size += 32;

  for (n = 0; n < MAX_DISTANCES; ++n)
  {
    info->distance[n] = - ((int) BIORD (block_size_log));
    BIORD_MORE (block_size_log);
  }

  i += ((n_blk + 1 + MAX_DISTANCES) * block_size_log + 7) >> 3;
  BIORD_START (ptr + i);
  BITIO_LEAVE (Bitio);
  if (huffman_read_length (&Bitio, temp, ALPHABET_SIZE) != 0)
  {
    free (info);
    free (ptr);
    fclose (fd);
    return RET (COMP_ERR_BROKEN);
  }

  if (huffman_decode_create (info->table, temp, ALPHABET_SIZE, START_BITS) != 0)
  {
    free (info);
    free (ptr);
    fclose (fd);
    return RET (COMP_ERR_BROKEN);
  }

  info->last_block_size = info->blk[n_blk];
  info->blk[n_blk] = 0;
  for (n = 0; n <= n_blk; ++n)
  {
    i = info->blk[n];
    info->blk[n] = header_size;
    header_size += i;
  }

  free (ptr);
  info->comp   = 0;
  info->blocks = -1;
  info->locked = -1;
  info->magic = DECODE_MAGIC;
  *res = (COMP_FILE) info;

  return (COMP_ERR_NONE);
}

int comp_log_block_size (COMP_FILE _info)
{
  decode_info *info = (decode_info *)_info;
  if (info == 0 || info->magic != DECODE_MAGIC)
    return (0);
  return (info->block_size_log);
}

int comp_tell_blocks (COMP_FILE _info)
{
  decode_info *info = (decode_info *) _info;
  if (info == 0 || info->magic != DECODE_MAGIC)
    return (-1);
  return (info->n_blk);
}

char *comp_file_name (COMP_FILE _info)
{
  decode_info *info = (decode_info *) _info;
  if (info == 0 || info->magic != DECODE_MAGIC)
    return (0);
  return (info->name);
}

int comp_close_file (COMP_FILE _info)
{
  decode_info *info = (decode_info *) _info;
  FILE *fd;

  if (info == 0 || info->magic != DECODE_MAGIC)
    return RET (COMP_ERR_PARAM);
  if (++info->blocks != 0)
  {
    --info->blocks;
    return RET (COMP_ERR_LOCKED);
  }
  info->magic = 0;
  fd = info->fd;
  if (info->comp != 0) free (info->comp);
  free (info);
  fclose (fd);

  return (COMP_ERR_NONE);
}

int comp_alloc_block (COMP_BLOCK *ret_block, COMP_FILE file)
{
  decode_info *info = (decode_info *) file;
  decode_block *block;
  if (info == 0 || info->magic != DECODE_MAGIC)
    return RET (COMP_ERR_PARAM);
  ++info->blocks;
  *ret_block = 0;
  if (ret_block == 0)
  {
    --info->blocks;
    return RET (COMP_ERR_PARAM);
  }
  if ((block = malloc (sizeof (*block) + info->block_size + 32)) == 0)
  {
    --info->blocks;
    return RET (COMP_ERR_NOMEM);
  }
  block->parent = info;
  block->orig.first = (uchar *)(block + 1);
  block->b.ptr = 0;
  block->b.decoded = -1;
  block->b.total = -1;
  block->b.number = -1;
  block->b.file = (COMP_FILE) info;
  block->comp.first = 0;
  block->n = -1;
  block->locked = -1;
  block->magic = BLOCK_MAGIC;
  *ret_block = &(block->b);
  return (COMP_ERR_NONE);
}

int comp_free_block (COMP_BLOCK _block)
{
  decode_block *block = (decode_block *) _block;
  decode_info *info;
  uchar *comp;
  if (block == 0 || block->magic != BLOCK_MAGIC || (info = block->parent)->magic != DECODE_MAGIC)
    return (COMP_ERR_PARAM);
  if (++block->locked != 0)
  {
    --block->locked;
    return RET (COMP_ERR_LOCKED);
  }
  if ((comp = block->comp.first) != 0)
  {
    if (++info->locked == 0 && info->comp == 0)
    {
      info->comp = comp;
      --info->locked;
    }
    else
    {
      --info->locked;
      free (comp);
    }
  }
  --info->blocks;
  block->magic = 0;
  block->b.ptr = 0;
  block->b.decoded = -1;
  block->b.total = -1;
  block->b.number = -1;
  free (block);
  return (COMP_ERR_NONE);
}

#define RETURN(n) \
  return (--block->locked, \
    ((n) == COMP_ERR_NONE ? COMP_ERR_NONE : RET (n)));

int comp_read_block (COMP_BLOCK _block, int n)
{
  decode_block *block = (decode_block *) _block;
  decode_info *info;
  int comp_size, orig_size;
  uchar *comp, *orig;
  if (block == 0 || block->magic != BLOCK_MAGIC)
    return RET (COMP_ERR_PARAM);
  if (++block->locked != 0)
    RETURN (COMP_ERR_LOCKED);
  info = block->parent;
  assert (info->magic == DECODE_MAGIC);
  if ((unsigned) n >= (unsigned) info->n_blk)
    RETURN (COMP_ERR_PARAM);
  if ((comp = block->comp.first) == 0)
  {
    if (++info->locked == 0 && (comp = info->comp) != 0)
    {
      info->comp = 0;
      --info->locked;
    }
    else
    {
      --info->locked;
      if ((comp = malloc (info->comp_block_size)) == 0)
	RETURN (COMP_ERR_NOMEM);
    }
    block->comp.first = comp;
  }
  block->n = n;
  orig = block->orig.first;
  orig_size = info->block_size;
  if (n == info->n_blk-1) orig_size = info->last_block_size;
  block->orig.size = orig_size; block->orig.first = orig;
  block->comp.size = comp_size = info->blk[n+1] - info->blk[n];
  if (fseek (info->fd, info->blk[n], SEEK_SET) != 0)
    RETURN (COMP_ERR_READ);
  if (fread (comp, 1, comp_size, info->fd) != comp_size)
    RETURN (COMP_ERR_READ);
  if (info->check_crc && info->crc[(n << 1) + 1] != CRC32 (block->comp.first, comp_size, 0))
    RETURN (COMP_ERR_BROKEN);
  block->emit.rept = 0;
  if (comp_size == orig_size)
  {
    memcpy (orig, comp, comp_size);
    block->emit.ptr = orig + comp_size;
    block->b.decoded = comp_size;
  }
  else
  {
    block->emit.ptr = orig;
    block->b.decoded = 0;
  }
  block->b.number = n;
  block->b.ptr = orig;
  block->b.total = orig_size;

  RETURN (COMP_ERR_NONE);
}

static int comp_decode_and_check_crc (COMP_BLOCK _block, int n, int check_crc)
{
  decode_block *block = (decode_block *) _block;
  decode_info *info;
  if (block == 0 || block->magic != BLOCK_MAGIC)
    return RET (COMP_ERR_PARAM);
  if (++block->locked != 0)
    RETURN (COMP_ERR_LOCKED);
  info = block->parent;
  assert (info->magic == DECODE_MAGIC);
  if ((unsigned) (n - 1) > (unsigned) (block->orig.size - 1))
    RETURN (COMP_ERR_PARAM);
  if (check_crc) n = block->orig.size;
  do_decode (info, block, block->orig.first + n);
  if ((block->b.decoded = (int) (block->emit.ptr - block->orig.first)) >= block->orig.size)
  {
    if (++info->locked == 0 && info->comp == 0)
    {
      info->comp = block->comp.first;
      --info->locked;
    }
    else
    {
      --info->locked;
      free (block->comp.first);
    }
    block->comp.first = 0;
  }
  block->b.ptr = block->orig.first;
  block->b.total = block->orig.size;
  if (block->b.decoded >= block->b.total)
  {
    if (block->b.decoded > block->b.total)
      RETURN (COMP_ERR_BROKEN);
    if (block->emit.rept != 0)
      RETURN (COMP_ERR_BROKEN);
  }
  if (check_crc && info->check_crc
    && info->crc[block->n << 1] != CRC32 (block->orig.first, block->orig.size, 0))
    RETURN (COMP_ERR_BROKEN);

  RETURN (COMP_ERR_NONE);
}

int comp_decode_block (COMP_BLOCK _block, int n)
{
  return (comp_decode_and_check_crc (_block, n, 0));
}

int comp_check_crc (COMP_BLOCK _block)
{
  return (comp_decode_and_check_crc (_block, 1, 1));
}

#endif /* DECODE */


#if ENCODE

typedef struct
{
  int distance[MAX_DISTANCES];
  struct
  {
    uchar *first;
    uchar *last;
    int    size;
  } orig, comp;
  bitio_t bitio;
  uint  freq[ALPHABET_SIZE];
  uint  mask[ALPHABET_SIZE];
  uchar bits[ALPHABET_SIZE];
}
encode_info;

static uint longest_match (encode_info *info, uchar *b1)
{
  uchar *p1, *p2, *e = info->orig.last;
  uint n, best_len, best_ofs;
  n = best_len = best_ofs = 0;
  p1 = b1;
  do
  {
    p2 = p1 + info->distance[n];
    if (*p1 == *p2 && p2 >= info->orig.first)
    {
#if 0
      do
      {
	++p1; ++p2;
      }
      while (p1 < e && *p1 == *p2);
#else
      ++p1; ++p2;
      do
      {
#       define X(i) if (p1[i] != p2[i]) {p1 += i; goto done;}
	  X(0); X(1); X(2); X(3); X(4); X(5); X(6); X(7); X(8);
	  X(9); X(10); X(11); X(12); X(13); X(14); X(15);
#       undef X
	p1 += 16; p2 += 16;
      }
      while (p1 < e);
#endif
  done:
      if (p1 > e) p1 = e;
      if ((int) (p1 - b1) > best_len)
      {
	best_len = (int) (p1 - b1);
	best_ofs = n;
	if (best_len >= STOP_SEARCH_LENGTH)
	  break;
      }
      p1 = b1;
    }
  }
  while (++n < MAX_DISTANCES);
  return ((best_len << LOG_MAX_DISTANCES) + best_ofs);
}

static void encode1 (encode_info *info)
{
  uchar *p = info->orig.first, *e = info->orig.last;
  uint n, k;
  while (p < e)
  {
    n = longest_match (info, p);
    if (n == 0)
      ++info->freq[*p++];
    else
    {
      k = ((n & ((1 << LOG_MAX_DISTANCES) - 1)) << MAX_LENGTH_BITS) + 256;
      n >>= LOG_MAX_DISTANCES; p += n; --n;
      if (n >= LONG_LENGTH)
      {
	n -= LONG_LENGTH - (1 << LONG_BITS);
	n = LOG2 (n) + LONG_LENGTH - LONG_BITS;
#if (MAX_BLOCK_BITS - 1) + (LONG_LENGTH - LONG_BITS) >= MAX_LENGTH
	if (n > MAX_LENGTH - 1)
	  n = MAX_LENGTH - 1;
#else
	assert (n <= MAX_LENGTH - 1);
#endif
      }
      ++info->freq[k + n];
    }
  }
}

static void encode2 (encode_info *info)
{
  BITIO_LOCALS;
  uchar *p = info->orig.first, *e = info->orig.last;
  uint n, k, nn, kk;
  BITIO_ENTER (info->bitio);
  while (p < e)
  {
    n = longest_match (info, p);
    if (n == 0)
    {
      n = *p++;
      k = info->bits[n];
      n = info->mask[n];
    }
    else
    {
      k = ((n & ((1 << LOG_MAX_DISTANCES) - 1)) << MAX_LENGTH_BITS) + 256;
      n >>= LOG_MAX_DISTANCES; p += n; --n;
      if (n < LONG_LENGTH)
      {
	n += k;
	k = info->bits[n];
	n = info->mask[n];
      }
      else
      {
	n -= LONG_LENGTH - (1 << LONG_BITS);
	kk = LOG2 (n); n -= 1 << kk;

#if (MAX_BLOCK_BITS - 1) + (LONG_LENGTH - LONG_BITS) >= MAX_LENGTH
	if (kk  >= DELTA)
	  k += MAX_LENGTH - 1;
	else
#else
	  k += kk + LONG_LENGTH - LONG_BITS;
#endif

	nn = info->bits[k];
	k = info->mask[k];
	BIOWR (k, nn);

#if (MAX_BLOCK_BITS - 1) + (LONG_LENGTH - LONG_BITS) >= MAX_LENGTH
	if (((int) (k = kk - DELTA)) >= 0)
	  BIOWR (k, 5);
	k += DELTA;
#else
	k = kk;
#endif
      }
    }
    BIOWR (n, k);
  }
  BITIO_LEAVE (info->bitio);
}

#define E(x) do {		\
  printf ("internal error at line %d\n", __LINE__); \
  printf x;             	\
  i = RET (COMP_ERR_INTERNAL);	\
  goto ret;             	\
} while (0)

typedef struct info_tt
{
  int freq;
  int dist;
  struct info_tt *link;
}
info_t;

#define xrand_value unsigned

#define XRAND1_DEG	127
static struct
{
  xrand_value *r, *f, *e, state [XRAND1_DEG];
} xrand1_table;

static xrand_value xrand1 (void)
{
  if (++(xrand1_table.f) >= xrand1_table.state + XRAND1_DEG)
  {
    xrand1_table.f = xrand1_table.state;
    ++(xrand1_table.r);
  }
  else if (++(xrand1_table.r) >= xrand1_table.state + XRAND1_DEG)
    xrand1_table.r = xrand1_table.state;
  return (*(xrand1_table.f) ^= *(xrand1_table.r));
}

static void xrand1_init (xrand_value x, xrand_value m)
{
  int i;
  for (i = 0; i < XRAND1_DEG; ++i)
  {
    xrand_value y;
    int j;
    for (y = j = 0; j < 8 * sizeof (y); ++j)
    {
      x = x * 27653 + 28181;
      y += ((x >> 15) & 1) << j;
    }
    xrand1_table.state[i] = y & m;
  }

  xrand1_table.r = (xrand1_table.f = xrand1_table.state) + 1;
}

static void evaluate
  (uchar *b, uchar *e, info_t *info, info_t *last, int m)
{
  info_t *q, *i;
  int n, best;
  uchar *p, *p1, *p2, ch;

  p = b + 1;
  if (p >= e) return;
  do
  {
    if (m)
    {
      p += xrand1 () & m;
      if (p >= e) return;
    }
    best = 0; ch = *p;
    i = info;
    do
    {
      p2 = p + i->dist;
      if (p2 < b || *p2 != ch) continue;
      p1 = p + 1;
      ++p2;
      while (p1 + 16 < e)
      {
#       define X(i) if (p1[i] != p2[i]) {p1 += i; goto done;}
	X(0); X(1); X(2); X(3); X(4); X(5); X(6); X(7); X(8);
	X(9); X(10); X(11); X(12); X(13); X(14); X(15);
#       undef X
	p1 += 16; p2 += 16;
      }
      while (p1 < e && *p1 == *p2) ++p1, ++p2;
    done:
      n = (int) (p1 - p);
      if (n >= best)
      {
	if (n > best)
	{
	  best = n;
	  q = 0;
	}
	i->link = q; q = i;
	if (best >= 256) break;
      }
    }
    while (++i != last);
    if (best == 0)
      ++p;
    else
    {
      p += best;
      best += 2;
      do
	q->freq += best;
      while ((q = q->link) != 0);
    }
  }
  while (p < e);
}

#define SORT_BITS	8
#define SORT_SIZE	(1 << SORT_BITS)
#define SORT_MASK	(SORT_SIZE - 1)

/* Standard "qsort" works too slowly on large arrays	*/
/* Simple radixsort is at order of magnitude faster	*/
static void sort_info (info_t *info, int n, int nn)
{
  info_t *head[SORT_SIZE], **link[SORT_SIZE], *p = info;
  int i, m;

  assert (n >= nn);

  for (i = 0; i < SORT_SIZE; ++i)
    *(link[i] = head + i) = 0;
  for (i = 0; i < n; ++i, ++p)
  {
    m = p->freq & SORT_MASK;
    link[m][0] = p; link[m] = &p->link;
  }
  n = 0;

over:
  i = SORT_SIZE; do --i; while ((p = head[i]) == 0);
  head[m = i] = 0;
  while (--i >= 0)
  {
    if (head[i] == 0) continue;
    link[m][0] = head[i]; link[m] = head + m; head[i] = 0;
    m = i;
  }
  link[m][0] = 0; link[m] = head + m;

  if ((n += SORT_BITS) >= 8 * sizeof (info))
    goto done;

  do
  {
    m = (p->freq >> n) & SORT_MASK;
    link[m][0] = p; link[m] = &p->link;
  }
  while ((p = p->link) != 0);

  goto over;

done:
  for (i = 0; i < nn; ++i)
  {
    info[i].freq = p->dist;
    p = p->link;
  }
  for (i = 0; i < nn; ++i)
  {
    info[i].dist = info[i].freq;
    info[i].freq = 0;
  }
}

int comp_analyze_file (COMP_STAT *stat, char *name, int block_size)
{
  FILE *fd;
  info_t *info;
  uchar *p;
  int max_n, n, m, d, pass, *dist;
  unsigned long tm1, tm2;

  if (stat == 0)
    return RET (COMP_ERR_PARAM);
  *stat = 0;

  if ((dist = malloc (sizeof (dist[0]) * (MAX_DISTANCES + 1))) == 0)
    return RET (COMP_ERR_NOMEM);
  dist[MAX_DISTANCES] = 0;

  if ((fd = fopen (name, "rb")) == 0)
  {
    free (dist);
    return RET (COMP_ERR_OPEN);
  }

  LOG2_init ();
  if (block_size <= (1 << MIN_BLOCK_BITS))
    block_size = 1 << MIN_BLOCK_BITS;
  else if (block_size >> MAX_BLOCK_BITS)
    block_size = 1 << MAX_BLOCK_BITS;
  else
    block_size = 1 << LOG2 (block_size);

  max_n = block_size >> 1;

  if ((info = malloc (sizeof (info[0]) * max_n + block_size)) == 0)
  {
    fclose (fd);
    free (dist);
    return RET (COMP_ERR_NOMEM);
  }

  p = (uchar *) (info + max_n);

  for (n = 0; n < max_n; ++n)
  {
    info[n].dist = -(n+1);
    info[n].freq = 0;
  }

  m = (block_size >> 3) - 1;
  d = max_n;
  pass = 1;

  do
  {
    fseek (fd, 0, SEEK_SET);
    tm1 = clock () - CLOCKS_PER_SEC;
    while ((n = fread (p, 1, block_size, fd)) > 0)
    {
      if (m) xrand1_init (27701, m);
      evaluate (p, p + n, info, info + d, m);
      tm2 = clock ();
      if (((tm2 - tm1) << 2) >= CLOCKS_PER_SEC)
      {
	fprintf (stderr, "analyzing \"%s\" pass %d: %10lu\r", name, pass, (unsigned long) ftell (fd));
	fflush (stderr);
	tm1 = tm2;
      }
    }
    fprintf (stderr, "analyzing \"%s\" pass %d: %lu bytes parsed\n", name, pass, (unsigned long) ftell (fd));
    if (n < 0)
    {
      free (info);
      fclose (fd);
      free (dist);
      return RET (COMP_ERR_READ);
    }
    m = 3 * MAX_DISTANCES;
    sort_info (info, d, m);
    if (d == m) break;
    d = m;
    m = 0;
    ++pass;
  }
  while (1);

  for (n = 0; n < MAX_DISTANCES; ++n)
  {
    dist[n] = info[n].dist;
    if (n == 0)
      fprintf (stderr, "%7d", info[n].dist);
    else if ((n & 7) == 0)
      fprintf (stderr, ",\n%7d", info[n].dist);
    else
      fprintf (stderr, ", %7d", info[n].dist);
  }
  fprintf (stderr, "\n"); fflush (stderr);
  dist[MAX_DISTANCES] = ENCODE_MAGIC;

  free (info);
  fclose (fd);

  *stat = (COMP_STAT) dist;
  return (COMP_ERR_NONE);
}

int comp_encode_file (char *in, char *out, int block_size, COMP_STAT stat)
{
  BITIO_LOCALS;
  FILE *fi = 0, *fo = 0;
  encode_info *info;
  int i, n, k, header_size, *dist;
  unsigned n_blk, *blk, *crc;
  unsigned long tm1, tm2;
  uchar *ptr = 0;
#if VERIFY
  decode_info *d_info = 0;
  decode_block  *d_block;
#endif

  if (stat == 0 || (dist = (int *) stat)[MAX_DISTANCES] != ENCODE_MAGIC)
    return RET (COMP_ERR_PARAM);

  LOG2_init ();
  CRC32_init ();

  if (block_size <= (1 << MIN_BLOCK_BITS))
    block_size = 1 << MIN_BLOCK_BITS;
  else if (block_size >> MAX_BLOCK_BITS)
    block_size = 1 << MAX_BLOCK_BITS;
  else
    block_size = 1 << LOG2 (block_size);

  for (n = dist[i = 0]; ++i < MAX_DISTANCES;)
    if (dist[i] < n) n = dist[i];
  n = -n;
  assert (n > 0);

  if ((info = malloc (sizeof (*info) + 3*block_size + n + 32)) == 0)
    E (("not enough memory"));

  memcpy (info->distance, dist, sizeof (info->distance));
  memset (info->freq, 0, sizeof (info->freq));

  info->comp.first = (uchar *) (info + 1);
  info->orig.first = info->comp.first + 2 * block_size + n;
  memset (info->orig.first - n, 0x5a, n);
  memset (info->orig.first + block_size, 0x5a, 32);

#if VERIFY
  if ((d_info = malloc (sizeof (*d_info) + sizeof (*d_block) + block_size + 32)) == 0)
    E (("not enough memory"));
  d_block = (decode_block *) (d_info + 1);
  d_block->orig.first = (uchar *) (d_block + 1);
  d_block->comp.first = info->comp.first;
  d_block->parent = d_info;
  memcpy (d_info->distance, dist, sizeof (d_info->distance));
#endif

  if ((fi = fopen (in, "rb")) == 0)
    E (("cannot open %s\n", in));

  if ((fo = fopen (out, "wb")) == 0)
    E (("cannot open %s\n", out));

  n = 0; n_blk = 0;
  tm1 = clock () - CLOCKS_PER_SEC;
  while ((i = fread (info->orig.first, 1, block_size, fi)) > 0)
  {
    if (n)
      E (("previously read %d out of %d, now %d\n", block_size - n, block_size, i));
    ++n_blk;
    n = block_size - i;
    info->orig.last = info->orig.first + i;
    encode1 (info);
    tm2 = clock ();
    if (((tm2 - tm1) << 2) >= CLOCKS_PER_SEC)
    {
      fprintf (stderr, "pass 1: %10lu\r", (unsigned long) ftell (fi));
      fflush (stderr);
      tm1 = tm2;
    }
  }
  if (i < 0)
    E (("read error file %s\n", in));
  fprintf (stderr, "pass 1: %lu bytes parsed\n", (unsigned long) ftell (fi));
  fflush (stderr);
  fseek (fi, 0, SEEK_SET);

#if 0
  for (i = 256; i < ALPHABET_SIZE; i += MAX_LENGTH)
  {
    n = (i - 256) >> MAX_LENGTH_BITS;
    printf ("table #%2d for offset %d:\n", n, info->distance[n]);
    for (n = 0; n < MAX_LENGTH; ++n)
      printf ("  %3d: %8lu\n", n, (unsigned long) info->freq[n+i]);
  }
#endif

  i = (((LOG2 (block_size >> 1) + 1) * (n_blk + MAX_DISTANCES + 1) + 7) >> 3) + ALPHABET_SIZE + 256 + HEADER_SIZE + 8 * n_blk;
  i = (i + sizeof (crc[0]) - 1) & ~(sizeof (crc[0]) - 1);
  if ((ptr = malloc (i + (3 * sizeof (crc[0]) + 1) * n_blk)) == 0)
    E (("not enough memory\n"));
  crc = (unsigned *) (ptr + i);
  blk = crc + 2 * n_blk;

  n = huffman_create_codes (info->freq, ALPHABET_SIZE, info->mask, info->bits);
  assert (n >= -1);

  i = HEADER_SIZE + (n_blk << 3) + (((LOG2 (block_size >> 1) + 1) * (n_blk + MAX_DISTANCES + 1) + 7) >> 3);
  BIOWR_START (ptr + i);
  BITIO_LEAVE (info->bitio);
  if (n > 0) ++info->bits[n - 1];
  huffman_write_length (&info->bitio, info->bits, ALPHABET_SIZE);
  if (n > 0) --info->bits[n - 1];
  BITIO_ENTER (info->bitio);
  header_size = ((BIOWR_TELL (ptr + i) + 7) >> 3) + i;
  BIOWR_FINISH ();
  header_size += 4;

  fseek (fo, header_size, SEEK_SET);

#if VERIFY
  i = huffman_decode_create (d_info->table, info->bits, ALPHABET_SIZE, START_BITS);
  if (i != 0)
    E (("huffman_decode_create returned %d\n", i));
#endif

  k = 0; n_blk = 0;
  tm1 = clock () - CLOCKS_PER_SEC;
  while ((i = fread (info->orig.first, 1, block_size, fi)) > 0)
  {
#if VERIFY
    int in_size = i;
#endif
    blk[n_blk + 1] = i;
    if (k)
      E (("previously read %d out of %d, now %d\n", block_size - k, block_size, i));
    k = block_size - i;
    info->orig.last = info->orig.first + i;

    BIOWR_START (info->comp.first);
    BITIO_LEAVE (info->bitio);
    encode2 (info);

    BITIO_ENTER (info->bitio);
    n = (BIOWR_TELL (info->comp.first) + 7) >> 3;
    BIOWR_FINISH ();

    crc[(n_blk << 1)] = CRC32 (info->orig.first, i, 0);
    if (n >= i - (i >> 16))
    {
      fprintf (stderr, "warning at %lu: original %d bytes, compressed %d bytes\n",
	((unsigned long) ftell (fi)) - i, i, n);
      memcpy (info->comp.first, info->orig.first, i);
      n = i;
      crc[(n_blk << 1) + 1] = crc[(n_blk << 1)];
    }
    else
      crc[(n_blk << 1) + 1] = CRC32 (info->comp.first, n, 0);
    blk[n_blk++] = n;

    if ((i = fwrite (info->comp.first, 1, n, fo)) != n)
      E (("write error %s: wrote %d, expect %d\n", out, i, n));

#if VERIFY
    if (blk[n_blk - 1] != blk[n_blk])
    {
      d_block->emit.ptr = d_block->orig.first;
      do_decode (d_info, d_block, d_block->orig.first + in_size);
      if (d_block->emit.rept != 0)
	printf ("decode error: %d symbols to copy left\n", d_block->emit.rept);
      if (memcmp (d_block->orig.first, info->orig.first, in_size) != 0)
      {
	for (i = 0; i < in_size; ++i)
	{
	  if (d_block->orig.first[i] != info->orig.first[i])
	  {
	    printf ("difference at %d\n", i);
	    break;
	  }
	}
      }
    }
#endif /* VERIFY */

    tm2 = clock ();
    if (((tm2 - tm1) << 2) >= CLOCKS_PER_SEC)
    {
      fprintf (stderr, "pass 2: %10lu => %10lu\r",
	(unsigned long) ftell (fi),
	(unsigned long) ftell (fo)
      );
      fflush (stderr);
      tm1 = tm2;
    }
  }
  if (i < 0)
    E (("read error file %s\n", in));

  /* write header	*/
  memcpy (ptr, header_title, 64);
#define W4(i,n)				\
  ptr[(i)    ] = (uchar) ((n) >> 24);	\
  ptr[(i) + 1] = (uchar) ((n) >> 16);	\
  ptr[(i) + 2] = (uchar) ((n) >>  8);	\
  ptr[(i) + 3] = (uchar) (n);

  W4 (64, header_size);
  ptr[68] = 0;
  ptr[69] = 0;
  ptr[70] = LOG2 (block_size >> 1) + 1;
  ptr[71] = MAX_DISTANCES;
  W4 (72, n_blk);
  W4 (76, (ALPHABET_SIZE << 12) + (LONG_BITS << 8) + (LONG_LENGTH << 4) + MAX_LENGTH_BITS);

  n_blk <<= 1;
  for (i = HEADER_SIZE, n = 0; n < n_blk; ++n)
  {
    W4 (i, crc[n]);
    i += 4;
  }
  n_blk >>= 1;

  BIOWR_START (ptr + i);
  k = LOG2 (block_size >> 1) + 1;
  for (n = 0; n <= n_blk; ++n)
  {
    if (blk[n] == block_size)
      BIOWR (0, k);
    else
      BIOWR (blk[n], k);
  }
  for (n = 0; n < MAX_DISTANCES; ++n)
  {
    if (-(info->distance[n]) >= block_size)
      BIOWR (0, k);
    else
      BIOWR (-(info->distance[n]), k);
  }
  BIOWR_FINISH ();
  header_size -= 4;
  n = CRC32 (ptr, header_size, 0);
  ptr += header_size;
  W4 (0, n);
  ptr -= header_size;
  header_size += 4;

  fseek (fo, 0, SEEK_SET);
  if (fwrite (ptr, 1, header_size, fo) != header_size)
    E (("write error"));
  fseek (fo, 0, SEEK_END);

  fprintf (stderr, "pass 2: %lu bytes read, %lu bytes written (%.3f : 1)\n",
    (unsigned long) ftell (fi),
    (unsigned long) ftell (fo),
    ((double) ftell (fi)) / ftell (fo)
  );
  fflush (stderr);
  i = 0;

ret:
  if (ptr != 0) free (ptr);
  if (fo != 0) fclose (fo);
  if (fi != 0) fclose (fi);
#if VERIFY
  if (d_info != 0) free (d_info);
#endif
  if (info != 0) free (info);
  return (i);
}

int comp_max_block (void)
{
  return (1 << MAX_BLOCK_BITS);
}

#endif /* ENCODE */


#if 0
int main (void)
{
  int dist[32];
  comp_analyze     ("k", dist, (1 << MAX_BLOCK_BITS));
  comp_encode_file ("k", "k.prs", (1 << MAX_BLOCK_BITS), dist);
//  comp_verify_file ("k.prs", 1);
//  comp_verify_file ("k.prs", 0);
  return (0);
}
#endif
