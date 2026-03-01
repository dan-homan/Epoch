// UNDONE: on error delete files
// UNDONE: better use symmetry in VIterate

#define	NEW
//#define	KPPKP_16BIT

#if !defined (DEBUG) && !defined(EBUG)
#define	NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#if defined (_WIN32)
#include <windows.h>
#endif

// Constants and types

#if defined (_MSC_VER)

#undef	TB_CDECL
#define	TB_CDECL		__cdecl
#define	TB_FASTCALL	__fastcall
#if _MSC_VER >= 1200
#define	INLINE		__forceinline
#endif

#else

#define	TB_CDECL
#define	TB_FASTCALL

#endif

#if !defined (INLINE)
#define	INLINE	inline
#endif

typedef	int	square;
typedef	unsigned char	BYTE;
typedef unsigned long	ULONG;
typedef unsigned long	INDEX;	// Index in a table

#define	MAX_PIECES	4		/* Maximum # of white or black pieces */
#define	MAX_MOVES	256		/* Maximum # of pseudo legal moves in a position */

#define COLOR_DECLARED

typedef	int	color;

#define	x_colorWhite	0
#define	x_colorBlack	1
#define	x_colorNeutral	2

#define PIECES_DECLARED

typedef	int	piece;

#define	x_pieceNone		0
#define	x_piecePawn		1
#define	x_pieceKnight	2
#define	x_pieceBishop	3
#define	x_pieceRook		4
#define	x_pieceQueen	5
#define	x_pieceKing		6

enum
	{
	A1 = 0x00, B1, C1, D1, E1, F1, G1, H1,
	A2 = 0x10, B2, C2, D2, E2, F2, G2, H2,
	A3 = 0x20, B3, C3, D3, E3, F3, G3, H3,
	A4 = 0x30, B4, C4, D4, E4, F4, G4, H4,
	A5 = 0x40, B5, C5, D5, E5, F5, G5, H5,
	A6 = 0x50, B6, C6, D6, E6, F6, G6, H6,
	A7 = 0x60, B7, C7, D7, E7, F7, G7, H7,
	A8 = 0x70, B8, C8, D8, E8, F8, G8, H8,
	XX = 0x7F
	};

static const int	rgiRowEnPassant[] = { 3, 4 };
static const int	rgiStepEnPassant[] = { -0x10, 0x10 };
static const int	rgiPawnStep[] = { 0x10, -0x10 };
static const int	rgiPawnRow[] = { 1, 6 };	
static const int	rgiPromoteRow[] = { 7, 0 };	

static const bool	rgfSweep[] = { false, false, false, true, true, true, false };

static const square rgsqNextSquare[] =
	{
	B1, C1, D1, E1, F1, G1, H1, A2, XX, XX, XX, XX, XX, XX, XX, XX, 
	B2, C2, D2, E2, F2, G2, H2, A3, XX, XX, XX, XX, XX, XX, XX, XX, 
	B3, C3, D3, E3, F3, G3, H3, A4, XX, XX, XX, XX, XX, XX, XX, XX, 
	B4, C4, D4, E4, F4, G4, H4, A5, XX, XX, XX, XX, XX, XX, XX, XX, 
	B5, C5, D5, E5, F5, G5, H5, A6, XX, XX, XX, XX, XX, XX, XX, XX, 
	B6, C6, D6, E6, F6, G6, H6, A7, XX, XX, XX, XX, XX, XX, XX, XX, 
	B7, C7, D7, E7, F7, G7, H7, A8, XX, XX, XX, XX, XX, XX, XX, XX,
	B8, C8, D8, E8, F8, G8, H8, XX, XX, XX, XX, XX, XX, XX, XX, XX
	};

static const square rgsqNextSquarePawn[] =
	{
	XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B2, C2, D2, E2, F2, G2, H2, A3, XX, XX, XX, XX, XX, XX, XX, XX, 
	B3, C3, D3, E3, F3, G3, H3, A4, XX, XX, XX, XX, XX, XX, XX, XX, 
	B4, C4, D4, E4, F4, G4, H4, A5, XX, XX, XX, XX, XX, XX, XX, XX, 
	B5, C5, D5, E5, F5, G5, H5, A6, XX, XX, XX, XX, XX, XX, XX, XX, 
	B6, C6, D6, E6, F6, G6, H6, A7, XX, XX, XX, XX, XX, XX, XX, XX, 
	B7, C7, D7, E7, F7, G7, H7, XX, XX, XX, XX, XX, XX, XX, XX, XX
	};

static const square rgsqNextSquareTriangle[] =
	{
	B1, C1, D1, B2, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, C2, D2, C3, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, XX, D3, D4, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	};

#if defined (NEW)
static const square rgsqNextSquareLargeTriangle[] =
	{
	B1, C1, D1, E1, F1, G1, H1, B2, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, C2, D2, E2, F2, G2, H2, C3, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, XX, D3, E3, F3, G3, H3, D4, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, XX, XX, E4, F4, G4, H4, E5, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, XX, XX, XX, F5, G5, H5, F6, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, XX, XX, XX, XX, G6, H6, G7, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, XX, XX, XX, XX, XX, H7, H8, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	};
#endif

static const square rgsqNextSquareHalf[] =
	{
	B1, C1, D1, A2, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B2, C2, D2, A3, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B3, C3, D3, A4, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B4, C4, D4, A5, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B5, C5, D5, A6, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B6, C6, D6, A7, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B7, C7, D7, A8, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B8, C8, D8, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	};

static const square rgsqNextSquareHalfPawn[] =
	{
	XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B2, C2, D2, A3, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B3, C3, D3, A4, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B4, C4, D4, A5, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B5, C5, D5, A6, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B6, C6, D6, A7, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	B7, C7, D7, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, 
	};

static const square rgsqFirstSquare[]	= { XX, A2, A1, A1, A1, A1, A1 };

static const piece rgpiPiecesLinear [] =
	{
	x_piecePawn,
	x_pieceKnight,
	x_pieceBishop,
	x_pieceRook,
	x_pieceQueen
	};

enum
	{
	x_mvCapture = 1,
	x_mvEnPassant = 2,
	x_mvPromoteKnight = 4,
	x_mvPromoteBishop = 8,
	x_mvPromoteRook = 16,
	x_mvPromoteQueen = 32
	};

const int x_mvPromotion = x_mvPromoteKnight |
						  x_mvPromoteBishop |
						  x_mvPromoteRook |
						  x_mvPromoteQueen;

typedef struct
	{
	square	m_sqFrom;
	square	m_sqTo;
	ULONG	m_ulFlags;
	int		m_iIndexFrom;
	int		m_iIndexTo;
	square	m_sqSaveEnPassant;
	piece	m_piCaptured;
	}
	CMove;

// Pointers to functions

typedef bool (* PfnBool) (color side, color xside);

// Statistics

static unsigned long *prgulStatistics;

// Useful macros

#define	FValidSquare(sq)	(!((sq) & 0x88))
#define	OtherSide(side)		(color) ((side) ^ 1)
#define	Row(sq)				((sq) >> 4)
#define	Column(sq)			((sq) & 7)
#define	SquareMake(row,col)	(((row) << 4) + (col))
#define	UlMvFlags(sq)		(x_colorNeutral == rgColor[sq] ? 0 : x_mvCapture)
#define	Linear(sq)			(rgindLinear [sq]) /* ((INDEX) (((sq) & 7) | (((sq) >> 1) & 0x38))) */
#define	PchColor(side)		(x_colorWhite == side ? "white" : "black")
#define	PchTm(side)			(x_colorWhite == side ? "wtm" : "btm")

// Board representation

color	rgColor [120];
piece	rgBoard [120];
static	square	rgsqPieceList[2][MAX_PIECES];
static	int		rgPieceCount[2];
static	square	sqEnPassant;
static	CMove	rgmv [MAX_MOVES];

static	const square *rgpsqNext[2][MAX_PIECES];

//-----------------------------------------------------------------------------
// Calculate index.
// Functions are in separate file, include it here.

static const INDEX rgindLinear[] =
	{
	 0,  1,  2,  3,  4,  5,  6,  7, XX, XX, XX, XX, XX, XX, XX, XX,
	 8,  9, 10, 11, 12, 13, 14, 15, XX, XX, XX, XX, XX, XX, XX, XX,
	16, 17, 18, 19, 20, 21, 22, 23, XX, XX, XX, XX, XX, XX, XX, XX,
	24, 25, 26, 27, 28, 29, 30, 31, XX, XX, XX, XX, XX, XX, XX, XX,
	32, 33, 34, 35, 36, 37, 38, 39, XX, XX, XX, XX, XX, XX, XX, XX,
	40, 41, 42, 43, 44, 45, 46, 47, XX, XX, XX, XX, XX, XX, XX, XX,
	48, 49, 50, 51, 52, 53, 54, 55, XX, XX, XX, XX, XX, XX, XX, XX,
	56, 57, 58, 59, 60, 61, 62, 63, XX, XX, XX, XX, XX, XX, XX, XX
	};

// Find piece in a list
// UNDONE:	Make a specialized version for generator;
//			It can use the fact that pieces in a list are sorted.

// NOTE: Code assuming that we'll produce MAX 5-man tables,
// so shall try only 3 elements max

#if defined (T41_INCLUDE)

INLINE square SqFindFirst
	(
	square	*psq,
	piece	pi
	)
	{
	square sq;

	sq = psq[1];
	if (rgBoard[sq] != pi)
		{
		sq = psq[2];
		if (rgBoard[sq] != pi)
			{
			sq = psq[3];
			assert (rgBoard[sq] == pi);
			}
		}
	assert (FValidSquare (sq));
	return Linear (sq);
	}

INLINE square SqFindSecond
	(
	square	*psq,
	piece	pi
	)
	{
	while (rgBoard[psq[1]] != pi)
		psq ++;
	while (rgBoard[psq[2]] != pi)
		psq ++;
	assert (FValidSquare (psq[2]));
	return Linear (psq[2]);
	}

#else

INLINE square SqFindFirst
	(
	square	*psq,
	piece	pi
	)
	{
	square sq;

	sq = psq[1];
	if (rgBoard[sq] != pi)
		{
		sq = psq[2];
		assert (rgBoard[sq] == pi);
		}
	assert (FValidSquare (sq));
	return Linear (sq);
	}

#define	SqFindSecond(psq, pi)	Linear (psq[2])

#endif

#define	SqFindKing(psq)			Linear (psq[0])
#define	SqFindOne(psq, pi)		Linear (psq[1])
#define	SqFindThird(psq, pi)	Linear (psq[3])
#define	SqFind2(psq,pi1,sq1,pi2,sq2) if(pi1==rgBoard[psq[1]]){sq1=psq[1];sq2=psq[2];}else{sq2=psq[1];sq1=psq[2];};sq1=Linear(sq1);sq2=Linear(sq2)

#define	ILLEGAL_POSSIBLE

#include "tbindex.cpp"

// Global data

static	piece	rgPieces[2][MAX_PIECES];
static	bool	rgfHasPawns[2];
static	bool	fBothSidesHavePawns;
static	int		rgiCount[10];

static	INDEX	cbCalc;
static	INDEX	rgCb[2];
static	FILE	*rgFp[2];
static	BYTE	*rgpbFile[2];

static	ULONG	cbMemoryUsed;
static	ULONG	cbMemoryCache;
static	BYTE	*pbMemory;
static	BYTE	*pbCache;

static bool	fMapFiles = false;

static	PfnCalcIndex pfnCalcIndexSide;
static	PfnCalcIndex pfnCalcIndexXside;

// Globals of main pass

static	bool	fChanged;

static	tb_t	tbtLo;
static	tb_t	tbtHi;
static	int		L_tbtLo;
static	int		L_tbtHi;

// Replace "result in N" by "Result in N+1"

#define	TbtReplaceResult(tbt)\
	(tb_t) ((bev_mimin == tbt || bev_miminx == tbt) ? bev_limaxx :\
			(bev_limaxx == tbt) ? bev_miminx :\
			(((tbt) > 0) - (tbt)))

#define	L_TbtReplaceResult(tbt)	(((tbt) > 0) - (tbt))

/* (((tbt) <= 0) ? (-tbt) : (1-tbt)) */

// Does new score better than current best?

#define	FBetter(best, score)\
	(bev_miminx == best ? (score > 0) :\
	 bev_limaxx == best ? (score >= 0 || bev_miminx == score) :\
	 bev_miminx == score ? (best <= 0) :\
	 bev_limaxx == score ? (best < 0 && bev_miminx != score) :\
	 (best < score))

#define	L_FBetter(best, score)	(best < score)

// Get 16-bit score in a machine-independent way

#define	L_Get(pb)	((((signed char *) (pb)) [1] << 8) + ((BYTE *) (pb)) [0])


// Is square atacked?

static ULONG	rgulAtacks [(H8 - A1) * 2 + 1];
static int		rgiStep [(H8 - A1) * 2 + 1];

INLINE bool	FAtacked
	(
	square	sqToCheck,	// IN | Square to check (typically king's square)
	color	side		// IN | Side to atack
	)
	{
	int i;

	i = rgPieceCount[side]-1;
	do
		{
		square	sq;
		piece	pi;

		sq = rgsqPieceList[side][i];
		pi = rgBoard[sq];
		if ((x_piecePawn == pi) && (x_colorBlack == side))
			pi = x_pieceNone;
		if (rgulAtacks [H8 + sq - sqToCheck] & (1 << pi))
			{
			if (rgfSweep[pi])
				{
				int iStep = rgiStep [H8 + sq - sqToCheck];

				for (square sqTemp = sq + iStep;; sqTemp += iStep)
					{
					if (x_pieceNone != rgBoard[sqTemp])
						{
						if (sqTemp != sqToCheck)
							goto next;
						break;
						}
					}
				}
			return true;
			}
next:	i --;
		}
	while (i >= 0);
	return false;
	}

// Initialize one direction in atacks detection tables

static void VInitializeDirection
	(
	int		iStep,
	piece	pi,
	int		count
	)
	{
	for (int iOffset = iStep; count != 0; iOffset += iStep, count --)
		{
		rgulAtacks[H8 - iOffset] |= (1 << pi);
		rgiStep[H8 - iOffset] = iStep;
		}
	}
	
// Initialize atacks detection tables

static void VInitializeAttacks (void)
	{
	// White pawns
	VInitializeDirection (0x0F, x_piecePawn, 1);
	VInitializeDirection (0x11, x_piecePawn, 1);

	// Black pawns
	VInitializeDirection (-0x0F, x_pieceNone, 1);
	VInitializeDirection (-0x11, x_pieceNone, 1);

	// Knights
	VInitializeDirection (0x12, x_pieceKnight, 1);
	VInitializeDirection (0x21, x_pieceKnight, 1);
	VInitializeDirection (0x0E, x_pieceKnight, 1);
	VInitializeDirection (0x1F, x_pieceKnight, 1);
	VInitializeDirection (-0x12, x_pieceKnight, 1);
	VInitializeDirection (-0x21, x_pieceKnight, 1);
	VInitializeDirection (-0x0E, x_pieceKnight, 1);
	VInitializeDirection (-0x1F, x_pieceKnight, 1);

	// Bishops
	VInitializeDirection (0x11, x_pieceBishop, 7);
	VInitializeDirection (0x0F, x_pieceBishop, 7);
	VInitializeDirection (-0x11, x_pieceBishop, 7);
	VInitializeDirection (-0x0F, x_pieceBishop, 7);

	// Rooks
	VInitializeDirection (0x01, x_pieceRook, 7);
	VInitializeDirection (0x10, x_pieceRook, 7);
	VInitializeDirection (-0x01, x_pieceRook, 7);
	VInitializeDirection (-0x10, x_pieceRook, 7);

	// Queens
	VInitializeDirection (0x01, x_pieceQueen, 7);
	VInitializeDirection (0x10, x_pieceQueen, 7);
	VInitializeDirection (0x11, x_pieceQueen, 7);
	VInitializeDirection (0x0F, x_pieceQueen, 7);
	VInitializeDirection (-0x01, x_pieceQueen, 7);
	VInitializeDirection (-0x10, x_pieceQueen, 7);
	VInitializeDirection (-0x11, x_pieceQueen, 7);
	VInitializeDirection (-0x0F, x_pieceQueen, 7);

	// Kings
	VInitializeDirection (0x01, x_pieceKing, 1);
	VInitializeDirection (0x10, x_pieceKing, 1);
	VInitializeDirection (0x11, x_pieceKing, 1);
	VInitializeDirection (0x0F, x_pieceKing, 1);
	VInitializeDirection (-0x01, x_pieceKing, 1);
	VInitializeDirection (-0x10, x_pieceKing, 1);
	VInitializeDirection (-0x11, x_pieceKing, 1);
	VInitializeDirection (-0x0F, x_pieceKing, 1);
	}

// Update PieceList when capture happened

INLINE void VUpdateOnCapture
	(
	CMove	*pmv,
	square	sqTo,
	color	xside,
	piece	pi
	)
	{
	int	i;

	rgiCount[5*xside+pi-x_piecePawn] --;
	for (i = 1; rgsqPieceList[xside][i] != sqTo; i ++)
		;
	pmv->m_iIndexTo = i;
	for (i ++; i < rgPieceCount[xside]; i ++)
		rgsqPieceList[xside][i-1] = rgsqPieceList[xside][i];
	rgPieceCount[xside] --;
	}

// Set en passant square

INLINE void VSetEnPassant
	(
	square	sqFrom,
	square	sqTo,
	color	side
	)
	{
	sqEnPassant = XX;
	if (x_piecePawn == rgBoard[sqTo])
		{
		if (0!=Column(sqTo) && x_piecePawn==rgBoard[sqTo-1] && side!=rgColor[sqTo-1] ||
			7!=Column(sqTo) && x_piecePawn==rgBoard[sqTo+1] && side!=rgColor[sqTo+1])
			{
			if (1 == Row(sqFrom) && 3 == Row(sqTo))
				sqEnPassant = sqFrom + 0x10;
			else if (6 == Row(sqFrom) && 4 == Row(sqTo))
				sqEnPassant = sqFrom - 0x10;
			}
		}
	}

// Make one move

static void VMakeMove
	(
	CMove	*pmv
	)
	{
	square	sqFrom = pmv->m_sqFrom;
	square	sqTo = pmv->m_sqTo;
	color	side = rgColor[sqFrom];

	assert (rgColor[sqFrom] == side);
	assert (x_pieceNone != rgBoard[sqFrom] && x_pieceKing != rgBoard[sqTo]);
	assert (rgColor[sqTo] != side);

	// Save info for UnamkeMove
	pmv->m_sqSaveEnPassant = sqEnPassant;
	pmv->m_piCaptured = rgBoard[sqTo];
	rgsqPieceList[side][pmv->m_iIndexFrom] = sqTo;

	// Modify board structures
	if (pmv->m_ulFlags & x_mvCapture)
		VUpdateOnCapture (pmv, sqTo, OtherSide (side), rgBoard[sqTo]);
	else if (pmv->m_ulFlags & x_mvEnPassant)
		{
		square	sqEnP = SquareMake (Row (sqFrom), Column (sqTo));

		VUpdateOnCapture (pmv, sqEnP, OtherSide (side), x_piecePawn);
		rgColor[sqEnP] = x_colorNeutral;
		rgBoard[sqEnP] = x_pieceNone;
		}
	rgColor[sqTo] = side;
	rgBoard[sqTo] = rgBoard[sqFrom];
	rgColor[sqFrom] = x_colorNeutral;
	rgBoard[sqFrom] = x_pieceNone;

	// Fix necessary data if promotion
	if (pmv->m_ulFlags & x_mvPromotion)
		{
		rgBoard[sqTo] = pmv->m_ulFlags & x_mvPromoteKnight ? x_pieceKnight :
						pmv->m_ulFlags & x_mvPromoteBishop ? x_pieceBishop :
						pmv->m_ulFlags & x_mvPromoteRook ? x_pieceRook : x_pieceQueen;
		rgiCount[5*side+rgBoard[sqTo]-x_piecePawn] ++;
		rgiCount[5*side+x_piecePawn-x_piecePawn] --;
		}

	// Set en passant
	VSetEnPassant (sqFrom, sqTo, side);
	}

// Restore PieceList, Color and Board after capture

INLINE void VRestoreOnCapture
	(
	CMove	*pmv,
	square	sqTo,
	color	xside,
	piece	pi
	)
	{
	int		i;

	for (i = (rgPieceCount[xside] ++); i > pmv->m_iIndexTo; i --)
		rgsqPieceList[xside][i] = rgsqPieceList[xside][i-1];
	rgsqPieceList[xside][i] = sqTo;
	rgColor[sqTo] = xside;
	rgBoard[sqTo] = pi;
	rgiCount[5*xside+pi-x_piecePawn] ++;
	}

// Unmake one move

static void VUnmakeMove
	(
	CMove	*pmv
	)
	{
	square	sqFrom = pmv->m_sqFrom;
	square	sqTo = pmv->m_sqTo;
	color	side = rgColor[sqTo];

	sqEnPassant = pmv->m_sqSaveEnPassant;

	// Restore 'from' square
	rgColor[sqFrom] = side;
	rgBoard[sqFrom] = rgBoard[sqTo];
	rgsqPieceList[side][pmv->m_iIndexFrom] = sqFrom;

	// Now restore 'to' and possible en passant square
	if (pmv->m_ulFlags & x_mvCapture)
		VRestoreOnCapture (pmv, sqTo, OtherSide (side), pmv->m_piCaptured);
	else
		{
		if (pmv->m_ulFlags & x_mvEnPassant)
			VRestoreOnCapture (pmv, SquareMake (Row (sqFrom), Column (sqTo)), OtherSide (side), x_piecePawn);
		rgColor[sqTo] = x_colorNeutral;
		rgBoard[sqTo] = x_pieceNone;
		}

	// Fix board data, if promotion
	if (pmv->m_ulFlags & x_mvPromotion)
		{
		rgiCount[5*side+rgBoard[sqFrom]-x_piecePawn] --;
		rgiCount[5*side+x_piecePawn-x_piecePawn] ++;
		rgBoard[sqFrom] = x_piecePawn;
		}

	assert (rgColor[sqFrom] == side);
	assert (x_pieceNone != rgBoard[sqFrom]);
	assert (rgColor[sqTo] != side);
	}

// Add pseudo legal move to the list

#define	VAddMove(pmv,sqFrom,sqTo,iIndexFrom,ulFlags) (pmv)->m_sqFrom=(sqFrom),(pmv)->m_sqTo=(sqTo),(pmv)->m_ulFlags=(ulFlags),(pmv)->m_iIndexFrom=(iIndexFrom)

// Generate all PseudoLegal moves in one direction

INLINE CMove* PmvGenPseudoLegalDirection
	(
	CMove	*pmv,
	color	side,
	square	sqFrom,
	int		iIndexFrom,
	int		iStep
	)
	{
	for (square sq = sqFrom + iStep; FValidSquare(sq); sq += iStep)
		{
		color col = rgColor[sq];

		if (x_colorNeutral == col)
			{
			VAddMove (pmv, sqFrom, sq, iIndexFrom, 0);	// Non-capture
			pmv ++;
			}
		else
			{
			if (side != col)
				{
				VAddMove (pmv, sqFrom, sq, iIndexFrom, x_mvCapture);
				pmv ++;
				}
			break;
			}
		}
	return pmv;
	}

// Add pawn move to the list

INLINE CMove* PmvAddPawnMove
	(
	CMove	*pmv,
	color	side,
	square	sqFrom,
	square	sqTo,
	int		iIndexFrom,
	ULONG	ulFlags
	)
	{
	if (Row(sqTo) == rgiPromoteRow[side])
		{
		VAddMove (pmv, sqFrom, sqTo, iIndexFrom, ulFlags | x_mvPromoteKnight);
		VAddMove (pmv + 1, sqFrom, sqTo, iIndexFrom, ulFlags | x_mvPromoteBishop);
		VAddMove (pmv + 2, sqFrom, sqTo, iIndexFrom, ulFlags | x_mvPromoteRook);
		ulFlags |= x_mvPromoteQueen;
		pmv += 3;
		}
	VAddMove (pmv, sqFrom, sqTo, iIndexFrom, ulFlags);
	return pmv + 1;
	}

// Generate all pseudo legal moves

static CMove* PmvGenPseudoLegalMoves
	(
	CMove	*pmv,
	color	side
	)
	{
	int		cPieces = rgPieceCount[side];
	color	xside = OtherSide (side);
	int		iPawnStep = rgiPawnStep[side];

	for (int iIndexFrom = 0; iIndexFrom < cPieces; iIndexFrom++)
		{
		square	sq = rgsqPieceList[side][iIndexFrom];
		piece	pi = rgBoard[sq];

		switch (pi)
			{
		case x_piecePawn:
			if (x_colorNeutral == rgColor[sq + iPawnStep])
				{
				pmv = PmvAddPawnMove (pmv, side, sq, sq + iPawnStep, iIndexFrom, 0);
				if (Row (sq) == rgiPawnRow[side] && x_colorNeutral == rgColor[sq + 2*iPawnStep])
					{
					VAddMove (pmv, sq, sq + 2*iPawnStep, iIndexFrom, 0);
					pmv ++;
					}
				}
			if (FValidSquare (sq + iPawnStep + 1))
				{
				if (rgColor[sq + iPawnStep + 1] == xside)
					pmv = PmvAddPawnMove (pmv, side, sq, sq + iPawnStep + 1, iIndexFrom, x_mvCapture);
				else if (sq + iPawnStep + 1 == sqEnPassant)
					{
					VAddMove (pmv, sq, sq + iPawnStep + 1, iIndexFrom, x_mvEnPassant);
					pmv ++;
					}
				}
			if (FValidSquare (sq + iPawnStep - 1))
				{
				if (rgColor[sq + iPawnStep - 1] == xside)
					pmv = PmvAddPawnMove (pmv, side, sq, sq + iPawnStep - 1, iIndexFrom, x_mvCapture);
				else if (sq + iPawnStep - 1 == sqEnPassant)
					{
					VAddMove (pmv, sq, sq + iPawnStep - 1, iIndexFrom, x_mvEnPassant);
					pmv ++;
					}
				}
			break;
		case x_pieceKnight:
			if (FValidSquare (sq + 0x12) && rgColor[sq + 0x12] != side)
				{
				VAddMove (pmv, sq, sq + 0x12, iIndexFrom, UlMvFlags (sq + 0x12));
				pmv ++;
				}
			if (FValidSquare (sq + 0x21) && rgColor[sq + 0x21] != side)
				{
				VAddMove (pmv, sq, sq + 0x21, iIndexFrom, UlMvFlags (sq + 0x21));
				pmv ++;
				}
			if (FValidSquare (sq + 0x0E) && rgColor[sq + 0x0E] != side)
				{
				VAddMove (pmv, sq, sq + 0x0E, iIndexFrom, UlMvFlags (sq + 0x0E));
				pmv ++;
				}
			if (FValidSquare (sq + 0x1F) && rgColor[sq + 0x1F] != side)
				{
				VAddMove (pmv, sq, sq + 0x1F, iIndexFrom, UlMvFlags (sq + 0x1F));
				pmv ++;
				}
			if (FValidSquare (sq - 0x12) && rgColor[sq - 0x12] != side)
				{
				VAddMove (pmv, sq, sq - 0x12, iIndexFrom, UlMvFlags (sq - 0x12));
				pmv ++;
				}
			if (FValidSquare (sq - 0x21) && rgColor[sq - 0x21] != side)
				{
				VAddMove (pmv, sq, sq - 0x21, iIndexFrom, UlMvFlags (sq - 0x21));
				pmv ++;
				}
			if (FValidSquare (sq - 0x0E) && rgColor[sq - 0x0E] != side)
				{
				VAddMove (pmv, sq, sq - 0x0E, iIndexFrom, UlMvFlags (sq - 0x0E));
				pmv ++;
				}
			if (FValidSquare (sq - 0x1F) && rgColor[sq - 0x1F] != side)
				{
				VAddMove (pmv, sq, sq - 0x1F, iIndexFrom, UlMvFlags (sq - 0x1F));
				pmv ++;
				}
			break;
		case x_pieceBishop:
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, 0x11);
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, 0x0F);
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, -0x11);
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, -0x0F);
			break;
		case x_pieceQueen:
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, 0x11);
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, 0x0F);
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, -0x11);
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, -0x0F);
			// Fall through
		case x_pieceRook:
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, 0x01);
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, 0x10);
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, -0x01);
			pmv = PmvGenPseudoLegalDirection (pmv, side, sq, iIndexFrom, -0x10);
			break;
		case x_pieceKing:
			if (FValidSquare (sq + 0x01) && rgColor[sq + 0x01] != side)
				{
				VAddMove (pmv, sq, sq + 0x01, iIndexFrom, UlMvFlags (sq + 0x01));
				pmv ++;
				}
			if (FValidSquare (sq + 0x10) && rgColor[sq + 0x10] != side)
				{
				VAddMove (pmv, sq, sq + 0x10, iIndexFrom, UlMvFlags (sq + 0x10));
				pmv ++;
				}
			if (FValidSquare (sq + 0x11) && rgColor[sq + 0x11] != side)
				{
				VAddMove (pmv, sq, sq + 0x11, iIndexFrom, UlMvFlags (sq + 0x11));
				pmv ++;
				}
			if (FValidSquare (sq + 0x0F) && rgColor[sq + 0x0F] != side)
				{
				VAddMove (pmv, sq, sq + 0x0F, iIndexFrom, UlMvFlags (sq + 0x0F));
				pmv ++;
				}
			if (FValidSquare (sq - 0x01) && rgColor[sq - 0x01] != side)
				{
				VAddMove (pmv, sq, sq - 0x01, iIndexFrom, UlMvFlags (sq - 0x01));
				pmv ++;
				}
			if (FValidSquare (sq - 0x10) && rgColor[sq - 0x10] != side)
				{
				VAddMove (pmv, sq, sq - 0x10, iIndexFrom, UlMvFlags (sq - 0x10));
				pmv ++;
				}
			if (FValidSquare (sq - 0x11) && rgColor[sq - 0x11] != side)
				{
				VAddMove (pmv, sq, sq - 0x11, iIndexFrom, UlMvFlags (sq - 0x11));
				pmv ++;
				}
			if (FValidSquare (sq - 0x0F) && rgColor[sq - 0x0F] != side)
				{
				VAddMove (pmv, sq, sq - 0x0F, iIndexFrom, UlMvFlags (sq - 0x0F));
				pmv ++;
				}
			break;
		default:
			assert (0);
			}
		}
	return pmv;
	}

// Put piece to the board

INLINE void VPutPiece
	(
	color	side,
	square	sq,
	piece	pi,
	int		index
	)
	{
	rgColor[sq] = side;
	rgBoard[sq] = pi;
	rgsqPieceList[side][index] = sq;
	}
	
// Iterate through all the positions

static void VIterateThroughPositions
	(
	color	side,	// IN | side to move
	PfnBool	pfn		// IN | function to be called for each position
	)
	{
	square	sq;
	piece	pi;
	color	xside = OtherSide (side);

	// Clear the board
	sq = A1;
	for (sq = A1; sq != XX; sq = rgsqNextSquare [sq])
		{
		rgColor[sq] = x_colorNeutral;
		rgBoard[sq] = x_pieceNone;
		}
	sqEnPassant = XX;

	// Main loop
	for (int iWhite = 0;;)
		{
		// Set white pieces to the board
		while (iWhite < rgPieceCount[x_colorWhite])
			{
			pi = rgPieces[x_colorWhite][iWhite];
			// Find empty square and put piece on it
			for (sq = rgsqFirstSquare[pi];
				 rgBoard[sq] != x_pieceNone;
				 sq = rgpsqNext[x_colorWhite][iWhite][sq])
				;
			assert (FValidSquare (sq));
			VPutPiece (x_colorWhite, sq, pi, iWhite);
			iWhite ++;
			}

		const square *prgSq;	// Possible squares of black king

#if defined (NEW)
		int iLinearWhiteKing = Linear (rgsqPieceList[x_colorWhite][0]);

		// In new indexing schema can use extra symmetryry
		prgSq = rgfNotDiagonal[iLinearWhiteKing] || rgfHasPawns[x_colorWhite] || rgfHasPawns[x_colorBlack]
				? rgsqNextSquare : rgsqNextSquareLargeTriangle;
		
#else
		prgSq = rgsqNextSquare;
#endif
		for (square sqBlackKing = A1; XX != sqBlackKing; sqBlackKing = prgSq[sqBlackKing])
			{
			if (x_pieceNone != rgBoard[sqBlackKing]
#if defined (NEW)	// Obviously illegal position
				|| INF == IndHalfKings(iLinearWhiteKing, Linear (sqBlackKing))
#endif
				)
				continue;
			assert (FValidSquare (sqBlackKing));
			VPutPiece (x_colorBlack, sqBlackKing, x_pieceKing, 0);

			// Loop through remaining black pieces
			for (int iBlack = 1;;)
				{
				// Set black pieces to the board
				while (iBlack < rgPieceCount[x_colorBlack])
					{
					pi = rgPieces[x_colorBlack][iBlack];
					// Find empty square and put piece on it
					for (sq = rgsqFirstSquare[pi];
						 rgBoard[sq] != x_pieceNone; 
						 sq = rgpsqNext[x_colorBlack][iBlack][sq])
						;
					assert (FValidSquare (sq));
					VPutPiece (x_colorBlack, sq, pi, iBlack);
					iBlack ++;
					}

				// Call our function (all loops were written because of that call)
				if ((*pfn) (side, xside))	// Legal position?
					{
#if defined (NEW)	// 'Standard' TBGEN does not handle EP
					// En passant
					if (fBothSidesHavePawns)
						{
						// UNDONE: speedup that check (bitboard?)
						for (int i = 1; i < rgPieceCount[xside]; i ++)
							{
							sq = rgsqPieceList[xside][i];
							if ((rgiRowEnPassant[xside] == Row(sq)) &&
								(x_piecePawn == rgBoard[sq]) &&
								(x_pieceNone == rgBoard[sq-rgiPawnStep[xside]]) &&
								(x_pieceNone == rgBoard[sq-rgiPawnStep[xside]*2]) &&
								((0 != Column(sq) && x_piecePawn == rgBoard[sq-1] && side == rgColor[sq-1]) ||
								 (7 != Column(sq) && x_piecePawn == rgBoard[sq+1] && side == rgColor[sq+1])))
								{
								sqEnPassant = sq-rgiPawnStep[xside];
								(*pfn) (side, xside);
								sqEnPassant = XX;
								}
							}
						}
#endif
					}

				// Move black piece to the next position
				if (1 == iBlack)
					break;
				else
					{
nextB:
					sq = rgsqPieceList[x_colorBlack][iBlack-1];
					pi = rgPieces[x_colorBlack][iBlack-1];
					// Remove piece from board
					rgColor[sq] = x_colorNeutral;
					rgBoard[sq] = x_pieceNone;
					// Find next free square and put piece on it
					do
						{
						sq = rgpsqNext[x_colorBlack][iBlack-1][sq];
						if (XX == sq)
							{
							iBlack --;
							if (1 != iBlack)
								goto nextB;
							goto nextBK;
							}
						}
					while (x_pieceNone != rgBoard[sq]);
					assert (FValidSquare (sq));
					VPutPiece (x_colorBlack, sq, pi, iBlack-1);
					}
				}
nextBK:
			rgColor[sqBlackKing] = x_colorNeutral;
			rgBoard[sqBlackKing] = x_pieceNone;
			}

		// Move white piece to the next position
nextW:
		sq = rgsqPieceList[x_colorWhite][iWhite-1];
		pi = rgPieces[x_colorWhite][iWhite-1];
		// Remove piece from board
		rgColor[sq] = x_colorNeutral;
		rgBoard[sq] = x_pieceNone;
		// Find next free square and put piece on it
		do
			{
			sq = rgpsqNext[x_colorWhite][iWhite-1][sq];
			if (sq == XX)
				{
				iWhite --;
				if (0 != iWhite)
					goto nextW;
				return;
				}
			}
		while (x_pieceNone != rgBoard[sq]);
		assert (FValidSquare (sq));
		VPutPiece (x_colorWhite, sq, pi, iWhite-1);
		}
	}

//-----------------------------------------------------------------------------

static void VPrintPosition (void)
	{
	int	i;

	printf ("White pieces: %02X", rgsqPieceList[x_colorWhite][0]);
	for (i = rgPieceCount[x_colorWhite]-1; i > 0; i --)
		printf (" %02X", rgsqPieceList[x_colorWhite][i]);
	printf ("\nBlack pieces: %02X", rgsqPieceList[x_colorBlack][0]);
	for (i = rgPieceCount[x_colorBlack]-1; i > 0; i --)
		printf (" %02X", rgsqPieceList[x_colorBlack][i]);
	printf ("\nEn passant %02X\n", sqEnPassant);
	}

//-----------------------------------------------------------------------------
//
//	Calculate # of positions

static bool	FCalcMaxIndex
	(
	color	side,
	color	xside
	)
	{
	INDEX	ind;

	// Valid position?
	if (FAtacked (rgsqPieceList[xside][0], side))
		return false;

	ind = pfnCalcIndexSide (rgsqPieceList[x_colorWhite], rgsqPieceList[x_colorBlack],
							Linear(sqEnPassant), false);
	if (ind > rgCb[side])
		rgCb[side] = ind;

	// In theory, can print error message immediatelly,
	// but it's better to calculate how many memory necessary
	if (ind >= cbCalc)
		return true;

	// Mark position as valid
#if 0
	// Check not valid when there are 2 identical pieces
	if (bev_broken != rgpbFile[side][ind])
		{
		printf ("*** Index repeated: %d\n", ind);
		VPrintPosition ();
		exit (1);
		}
#endif
	rgpbFile[side][ind] = (BYTE) bev_draw;
	if (!FAtacked (rgsqPieceList[side][0], xside))
		return true;

	// Generate all pseudolegal moves
	// UNDONE: first generate only king moves, as usually king can escape
	CMove	*pmv;
	CMove	*pmvEnd;

	pmvEnd = PmvGenPseudoLegalMoves (rgmv, side);

	// Now iterate through all of them
	for (pmv = rgmv; pmv < pmvEnd; pmv ++)
		{
		VMakeMove (pmv);
		// Illegal move?
		if (! FAtacked (rgsqPieceList[side][0], xside))
			{
			VUnmakeMove (pmv);
			return true;
			}
		VUnmakeMove (pmv);
		}

	// No legal moves
	rgpbFile[side][ind] = (BYTE) bev_li0;
	return true;
	}

//-----------------------------------------------------------------------------
//
//	Calculate # of positions

static bool	L_FCalcMaxIndex
	(
	color	side,
	color	xside
	)
	{
	INDEX	ind;

	// Valid position?
	if (FAtacked (rgsqPieceList[xside][0], side))
		return false;

	ind = pfnCalcIndexSide (rgsqPieceList[x_colorWhite], rgsqPieceList[x_colorBlack],
							Linear(sqEnPassant), false);
	ind *= 2;
	if (ind > rgCb[side])
		rgCb[side] = ind;

	// In theory, can print error message immediatelly,
	// but it's better to calculate how many memory necessary
	if (ind >= cbCalc)
		return true;

	// Mark position as valid
	rgpbFile[side][ind] = rgpbFile[side][ind+1] = 0;
	if (!FAtacked (rgsqPieceList[side][0], xside))
		return true;

	// Generate all pseudolegal moves
	// UNDONE: first generate only king moves, as usually king can escape
	CMove	*pmv;
	CMove	*pmvEnd;

	pmvEnd = PmvGenPseudoLegalMoves (rgmv, side);

	// Now iterate through all of them
	for (pmv = rgmv; pmv < pmvEnd; pmv ++)
		{
		VMakeMove (pmv);
		// Illegal move?
		if (! FAtacked (rgsqPieceList[side][0], xside))
			{
			VUnmakeMove (pmv);
			return true;
			}
		VUnmakeMove (pmv);
		}

	// No legal moves
	rgpbFile[side][ind]   = (BYTE) L_bev_li0;
	rgpbFile[side][ind+1] = (BYTE) (L_bev_li0 >> 8);
	return true;
	}

//-----------------------------------------------------------------------------
//
//	Write entire file to disk

static void VWriteFile
	(
	color	side,
	BYTE	*pb,
	int		iTb
	)
	{
	FILE *fp;
	ULONG cbWritten;

	fp = rgFp[side];
	if (fseek (fp, 0, SEEK_SET))
		{
		printf ("*** Seek (writing file) failed\n");
		exit (1);
		return;
		}
	// Shrink kppkp
#if !defined (KPPKP_16BIT)
	if (tbid_kppkp == iTb)
		{
		int	cbMax;

		cbMax = rgCb[side]/2;
		for (int cb = 0; cb < cbMax; cb ++)
			{
			int	tbValue = L_Get (pb + 2*cb);

			if (0 == tbValue)
				pb [cb] = (BYTE) 0;
			else if (L_bev_broken == tbValue)
				pb [cb] = (BYTE) bev_broken;
			else if (-32640 == tbValue)
				pb [cb] = (BYTE) bev_limaxx;
			else if (32640 == tbValue)
				pb [cb] = (BYTE) bev_miminx;
			else if (tbValue > 32640 && tbValue <= L_bev_mi1)
				pb [cb] = (BYTE) (tbValue - 32640);
			else if (tbValue < -32640 && tbValue >= L_bev_li0)
				pb [cb] = (BYTE) (tbValue + 32640);
			else
				{
				printf ("Value out of range: side=%d index=%08X value=%d\n", side, cb, tbValue);
				pb [cb] = (BYTE) bev_broken;
				}
			}
		rgCb[side] /= 2;
		}
#endif
	// Now write the file
	cbWritten = fwrite (pb, 1, rgCb[side], fp);
	if (cbWritten != rgCb[side])
		{
		printf ("*** Write (writing file) failed\n");
		exit (1);
		return;
		}
	}

//-----------------------------------------------------------------------------
//
//	Main loop - make move and probe table with opponent to move

// Print position if something is wrong

static void VInvalid
	(
	char	*pchText,
	color	side,
	INDEX	ind,
	int		tbtValue,
	int		tbtTable
	)
	{
	printf ("\n*** %s: index = %08X value = %02X table value = %02X side = %d\n",
			pchText, ind, tbtValue, tbtTable, side);
	VPrintPosition ();
	exit (1);
	}

//	Routine to make *minimal* quiet move,
//	do not bother with en promotion/capture.
//	Modify ONLY board structures that are necessary for index calculating.

INLINE void VMinimalMakeQuietMove
	(
	CMove	*pmv,
	color	side
	)
	{
	square	sqFrom = pmv->m_sqFrom;
	square	sqTo = pmv->m_sqTo;

	pmv->m_sqSaveEnPassant = sqEnPassant;
	rgsqPieceList[side][pmv->m_iIndexFrom] = sqTo;
	// rgColor[sqTo] = side;
	rgBoard[sqTo] = rgBoard[sqFrom];
	// rgColor[sqFrom] = x_colorNeutral;
	// rgBoard[sqFrom] = x_pieceNone;

	// Set en passant
	VSetEnPassant (sqFrom, sqTo, side);
	}

INLINE void VUnmakeMinimalQuietMove
	(
	CMove	*pmv,
	color	side
	)
	{
	square	sqFrom = pmv->m_sqFrom;
	square	sqTo = pmv->m_sqTo;

	rgsqPieceList[side][pmv->m_iIndexFrom] = sqFrom;
	// rgColor[sqFrom] = side;
	// rgBoard[sqFrom] = rgBoard[sqTo];
	// rgColor[sqTo] = x_colorNeutral;
	rgBoard[sqTo] = x_pieceNone;
	sqEnPassant = pmv->m_sqSaveEnPassant;
	}

//-----------------------------------------------------------------------------
//
// Promote information from opposite table when everything is in memory

static bool FProbeOpposite
	(
	color	side,
	color	xside
	)
	{
	CMove	*pmv;
	CMove	*pmvEnd;
	tb_t	*ptbt;
	int		tbtBest;
	BYTE	*pbOpposite;
	INDEX	ind;
	INDEX	indMax;
	PfnCalcIndex pfnci;

	// Calculate index of current position
	ind = pfnCalcIndexSide (rgsqPieceList[x_colorWhite], rgsqPieceList[x_colorBlack],
							Linear(sqEnPassant), false);

	// Possible legal position?
	if (ind >= rgCb [side])
		{
		assert (FAtacked (rgsqPieceList[xside][0], side));
		return false;
		}
	ptbt = (tb_t*) rgpbFile[side] + ind;
	tbtBest = *ptbt;

	// Legal position?
	if (bev_broken == tbtBest)
		{
		assert (FAtacked (rgsqPieceList[xside][0], side));
		return false;
		}
	assert (!FAtacked (rgsqPieceList[xside][0], side));

	// Is it necessary to iterate?
#if 1
	if (bev_limaxx != tbtBest && bev_miminx != tbtBest && (tbtBest <= tbtLo || tbtBest > tbtHi))
		return true;
#endif

	// Generate all pseudolegal moves
	pmvEnd = PmvGenPseudoLegalMoves (rgmv, side);

	// Now iterate through all of them
	pfnci = pfnCalcIndexXside;
	pbOpposite = rgpbFile[xside];
	indMax = rgCb [xside];
	tbtBest = -65536;
	for (pmv = rgmv; pmv < pmvEnd; pmv ++)
		{
		// If capture/promotion, have to probe minor;
		// Otherwise, probe opposite
		if (0 == pmv->m_ulFlags)
			{
			int	tbtScore;

			VMinimalMakeQuietMove (pmv, side);

			// Probe opposite table
			ind = pfnci (rgsqPieceList[x_colorWhite],
						 rgsqPieceList[x_colorBlack],
						 Linear(sqEnPassant), false);
			
			// Possible legal move?
			if (ind < indMax)
				{
				tbtScore = (tb_t) pbOpposite [ind];
				// Legal move?
				if (bev_broken != tbtScore)
					{
					tbtScore = TbtReplaceResult (tbtScore);
					if (FBetter (tbtBest, tbtScore))
						tbtBest = tbtScore;
					}
				}
			VUnmakeMinimalQuietMove (pmv, side);
			}
		else
			{
			int	iTb;

			VMakeMove (pmv);
			// Illegal move?
			if (FAtacked (rgsqPieceList[side][0], xside))
				{
				VUnmakeMove (pmv);
				continue;
				}
			// Probe minor table
			iTb = IDescFindFromCounters	(rgiCount);
			if (0 != iTb)
				{
				int	tbtScore;

				if (iTb < 0)
					{
					INDEX indexChild;

					iTb = -iTb;
					indexChild = PfnIndCalc(iTb, side) (
										rgsqPieceList[x_colorBlack],
										rgsqPieceList[x_colorWhite],
										XX, true);
					tbtScore = TbtProbeTable (iTb, side, indexChild);
					}
				else
					{
					INDEX indexChild;

					indexChild = PfnIndCalc(iTb, xside) (
										rgsqPieceList[x_colorWhite],
										rgsqPieceList[x_colorBlack],
										XX, false);
					tbtScore = TbtProbeTable (iTb, xside, indexChild);
					}
				if (bev_broken == tbtScore)
					{
					printf ("*** Invalid minor value -- something wrong\n");
					VPrintPosition ();
					exit (1);
					}
				VUnmakeMove (pmv);
				tbtScore = TbtReplaceResult (tbtScore);
				if (FBetter (tbtBest, tbtScore))
					tbtBest = tbtScore;
				}
			else
				{
				if (2 != rgPieceCount[x_colorWhite]+rgPieceCount[x_colorBlack])
					{
					printf ("*** Unable to probe minor -- something wrong\n");
					VPrintPosition ();
					exit (1);
					}
				// KK, captured last enemy piece
				VUnmakeMove (pmv);
				*ptbt = bev_draw;
				return true;
				}
			}
		}
	if (-65536 == tbtBest)	// No legal moves
		*ptbt = (tb_t) (FAtacked (rgsqPieceList[side][0], xside) ? bev_li0 : bev_draw);
	else if (tbtBest != *ptbt)
		{
		*ptbt = (tb_t) tbtBest;
		fChanged = true;
		}
	return true;
	}

//-----------------------------------------------------------------------------
//
// Promote information from opposite table when everything is in memory

static bool L_FProbeOpposite
	(
	color	side,
	color	xside
	)
	{
	CMove	*pmv;
	CMove	*pmvEnd;
	BYTE	*ptbt;
	int		tbtBest;
	BYTE	*pbOpposite;
	INDEX	ind;
	INDEX	indMax;
	PfnCalcIndex pfnci;

	// Calculate index of current position
	ind = pfnCalcIndexSide (rgsqPieceList[x_colorWhite], rgsqPieceList[x_colorBlack],
							Linear(sqEnPassant), false);
	ind *= 2;

	// Possible legal position?
	if (ind >= rgCb [side])
		{
		assert (FAtacked (rgsqPieceList[xside][0], side));
		return false;
		}
	ptbt = rgpbFile[side] + ind;
	tbtBest = L_Get (ptbt);

	// Legal position?
	if (L_bev_broken == tbtBest)
		{
		assert (FAtacked (rgsqPieceList[xside][0], side));
		return false;
		}
	assert (!FAtacked (rgsqPieceList[xside][0], side));

	// Is it necessary to iterate?
#if 1
	if (tbtBest <= L_tbtLo || tbtBest > L_tbtHi)
		return true;
#endif

	// Generate all pseudolegal moves
	pmvEnd = PmvGenPseudoLegalMoves (rgmv, side);

	// Now iterate through all of them
	pfnci = pfnCalcIndexXside;
	pbOpposite = rgpbFile[xside];
	indMax = rgCb [xside];
	tbtBest = -65536;
	for (pmv = rgmv; pmv < pmvEnd; pmv ++)
		{
		// If no capture/promotion, probe opposite
		// Otherwise, probe minor
		if (0 == pmv->m_ulFlags)
			{
			int	tbtScore;

			VMinimalMakeQuietMove (pmv, side);

			// Probe opposite table
			ind = pfnci (rgsqPieceList[x_colorWhite],
						 rgsqPieceList[x_colorBlack],
						 Linear(sqEnPassant), false);
			ind *= 2;
			
			// Possible legal move?
			if (ind < indMax)
				{
				tbtScore = L_Get (pbOpposite + ind);
				// Legal move?
				if (L_bev_broken != tbtScore)
					{
					tbtScore = L_TbtReplaceResult (tbtScore);
					if (L_FBetter (tbtBest, tbtScore))
						tbtBest = tbtScore;
					}
				}
			VUnmakeMinimalQuietMove (pmv, side);
			}
		else
			{
			int	iTb;

			VMakeMove (pmv);
			// Illegal move?
			if (FAtacked (rgsqPieceList[side][0], xside))
				{
				VUnmakeMove (pmv);
				continue;
				}
			// Probe minor table
			iTb = IDescFindFromCounters	(rgiCount);
			if (0 != iTb)
				{
				int	tbtScore;

				if (iTb < 0)
					{
					INDEX indexChild;

					iTb = -iTb;
					indexChild = PfnIndCalc(iTb, side) (
										rgsqPieceList[x_colorBlack],
										rgsqPieceList[x_colorWhite],
										XX, true);
					tbtScore = TbtProbeTable (iTb, side, indexChild);
					}
				else
					{
					INDEX indexChild;

					indexChild = PfnIndCalc(iTb, xside) (
										rgsqPieceList[x_colorWhite],
										rgsqPieceList[x_colorBlack],
										XX, false);
					tbtScore = TbtProbeTable (iTb, xside, indexChild);
					}
				if (bev_broken == tbtScore)
					{
					printf ("*** Invalid minor value -- something wrong\n");
					VPrintPosition ();
					exit (1);
					}
				VUnmakeMove (pmv);
				tbtScore = S_to_L (tbtScore);
				tbtScore = L_TbtReplaceResult (tbtScore);
				if (L_FBetter (tbtBest, tbtScore))
					tbtBest = tbtScore;
				}
			else
				{
				if (2 != rgPieceCount[x_colorWhite]+rgPieceCount[x_colorBlack])
					{
					printf ("*** Unable to probe minor -- something wrong\n");
					VPrintPosition ();
					exit (1);
					}
				// KK, captured last enemy piece
				VUnmakeMove (pmv);
				ptbt[0] = ptbt[1] = 0;
				return true;
				}
			}
		}
	if (-65536 == tbtBest)	// No legal moves
		{
		tbtBest = FAtacked (rgsqPieceList[side][0], xside) ? L_bev_li0 : L_bev_draw;
		ptbt[0] = (BYTE) tbtBest;
		ptbt[1] = (BYTE) (tbtBest >> 8);
		}
	else if (tbtBest != L_Get (ptbt))
		{
		ptbt[0] = (BYTE) tbtBest;
		ptbt[1] = (BYTE) (tbtBest >> 8);
		fChanged = true;
		}
	return true;
	}

//-----------------------------------------------------------------------------
//
// Verify table

int	iGlobalTb;

// Verify 8-bit table

static bool FVerify
	(
	color	side,
	color	xside
	)
	{
	CMove	*pmv;
	CMove	*pmvEnd;
	int		tbtTable;
	int		tbtBest;
	INDEX	indTable;
	INDEX	ind;
	PfnCalcIndex pfnci;
	bool	fWas_mimin;

	// Valid position?
	if (FAtacked (rgsqPieceList[xside][0], side))
		return false;

	// Calculate index of current position and probe table
	indTable = pfnCalcIndexSide (rgsqPieceList[x_colorWhite], rgsqPieceList[x_colorBlack],
								 Linear(sqEnPassant), false);

	tbtTable = TbtProbeTable (iGlobalTb, side, indTable);

	// Generate all pseudolegal moves
	pmvEnd = PmvGenPseudoLegalMoves (rgmv, side);

	// Now iterate through all of them
	pfnci = pfnCalcIndexXside;
	tbtBest = -65536;
	fWas_mimin = false;
	for (pmv = rgmv; pmv < pmvEnd; pmv ++)
		{
		VMakeMove (pmv);
		// Illegal move?
		if (FAtacked (rgsqPieceList[side][0], xside))
			{
			VUnmakeMove (pmv);
			continue;
			}

		tb_t tbtScore;

		// If capture/promotion, have to probe minor
		if (0 == pmv->m_ulFlags)
			{
			// Probe opposite table
			ind = pfnci (rgsqPieceList[x_colorWhite],
						 rgsqPieceList[x_colorBlack],
						 Linear(sqEnPassant), false);
			
			tbtScore = TbtProbeTable (iGlobalTb, xside, ind);
			if (bev_broken == tbtScore)
				VInvalid ("Broken opposite", side, indTable, bev_broken, bev_broken);
			}
		else
			{
			int	iTb;

			// Probe minor table
			iTb = IDescFindFromCounters	(rgiCount);
			if (0 != iTb)
				{
				if (iTb < 0)
					{
					INDEX indexChild;

					iTb = -iTb;
					indexChild = PfnIndCalc(iTb, side) (
										rgsqPieceList[x_colorBlack],
										rgsqPieceList[x_colorWhite],
										XX, true);
					tbtScore = TbtProbeTable (iTb, side, indexChild);
					if (bev_broken == tbtScore)
						VInvalid ("Broken minor", side, indTable, bev_broken, bev_broken);
					}
				else
					{
					INDEX indexChild;

					indexChild = PfnIndCalc(iTb, xside) (
										rgsqPieceList[x_colorWhite],
										rgsqPieceList[x_colorBlack],
										XX, false);
					tbtScore = TbtProbeTable (iTb, xside, indexChild);
					if (bev_broken == tbtScore)
						VInvalid ("Broken minor", side, indTable, bev_broken, bev_broken);
					}
				}
			else
				{
				if (2 != rgPieceCount[x_colorWhite]+rgPieceCount[x_colorBlack])
					{
					printf ("*** Unable to probe minor\n");
					VPrintPosition ();
					exit (1);
					}
				tbtScore = bev_draw;
				}
			}
		fWas_mimin |= (bev_mimin == tbtScore);
		tbtScore = TbtReplaceResult (tbtScore);
		if (FBetter (tbtBest, tbtScore))
			tbtBest = tbtScore;
		VUnmakeMove (pmv);
		}

	if (-65536 == tbtBest)	// No legal moves
		tbtBest = (tb_t) (FAtacked (rgsqPieceList[side][0], xside) ? bev_li0 : bev_draw);
	if (bev_limaxx == tbtBest && !fWas_mimin)
		VInvalid ("Score overflow", side, indTable, tbtBest, tbtTable);
	if (tbtBest != tbtTable)
		VInvalid ("Wrong result", side, indTable, tbtBest, tbtTable);
	return true;
	}

// Verify 16-bit table

static bool L_FVerify
	(
	color	side,
	color	xside
	)
	{
#if !defined (KPPKP_16BIT)
	CMove	*pmv;
	CMove	*pmvEnd;
	int		tbtTable;
	int		tbtBest;
	INDEX	indTable;
	INDEX	ind;
	PfnCalcIndex pfnci;

	// Valid position?
	if (FAtacked (rgsqPieceList[xside][0], side))
		return false;

	// Calculate index of current position and probe table
	indTable = pfnCalcIndexSide (rgsqPieceList[x_colorWhite], rgsqPieceList[x_colorBlack],
								 Linear(sqEnPassant), false);

	tbtTable = L_TbtProbeTable (iGlobalTb, side, indTable);

	// Generate all pseudolegal moves
	pmvEnd = PmvGenPseudoLegalMoves (rgmv, side);

	// Now iterate through all of them
	pfnci = pfnCalcIndexXside;
	tbtBest = -65536;
	for (pmv = rgmv; pmv < pmvEnd; pmv ++)
		{
		VMakeMove (pmv);
		// Illegal move?
		if (FAtacked (rgsqPieceList[side][0], xside))
			{
			VUnmakeMove (pmv);
			continue;
			}

		int	tbtScore;

		// If capture/promotion, have to probe minor
		if (0 == pmv->m_ulFlags)
			{
			// Probe opposite table
			ind = pfnci (rgsqPieceList[x_colorWhite],
						 rgsqPieceList[x_colorBlack],
						 Linear(sqEnPassant), false);
			tbtScore = L_TbtProbeTable (iGlobalTb, xside, ind);
			if (L_bev_broken == tbtScore)
				VInvalid ("Broken opposite", side, indTable, L_bev_broken, L_bev_broken);
			}
		else
			{
			int	iTb;

			// Probe minor table
			iTb = IDescFindFromCounters	(rgiCount);
			if (0 != iTb)
				{
				if (iTb < 0)
					{
					INDEX indexChild;

					iTb = -iTb;
					indexChild = PfnIndCalc(iTb, side) (
										rgsqPieceList[x_colorBlack],
										rgsqPieceList[x_colorWhite],
										XX, true);
					tbtScore = TbtProbeTable (iTb, side, indexChild);
					if (bev_broken == tbtScore)
						VInvalid ("Broken minor", side, indTable, bev_broken, bev_broken);
					}
				else
					{
					INDEX indexChild;

					indexChild = PfnIndCalc(iTb, xside) (
										rgsqPieceList[x_colorWhite],
										rgsqPieceList[x_colorBlack],
										XX, false);
					tbtScore = TbtProbeTable (iTb, xside, indexChild);
					if (bev_broken == tbtScore)
						VInvalid ("Broken minor", side, indTable, bev_broken, bev_broken);
					}
				tbtScore = S_to_L (tbtScore);
				}
			else
				{
				if (2 != rgPieceCount[x_colorWhite]+rgPieceCount[x_colorBlack])
					{
					printf ("*** Unable to probe minor\n");
					VPrintPosition ();
					exit (1);
					}
				tbtScore = 0;
				}
			}
		tbtScore = L_TbtReplaceResult (tbtScore);
		if (L_FBetter (tbtBest, tbtScore))
			tbtBest = tbtScore;
		VUnmakeMove (pmv);
		}

	if (-65536 == tbtBest)	// No legal moves
		tbtBest = (FAtacked (rgsqPieceList[side][0], xside) ? L_bev_li0 : L_bev_draw);
	if (tbtBest != tbtTable)
		VInvalid ("Wrong result", side, indTable, tbtBest, tbtTable);
	return true;
#else
	CMove	*pmv;
	CMove	*pmvEnd;
	int		tbtTable;
	int		tbtBest;
	INDEX	indTable;
	INDEX	ind;
	PfnCalcIndex pfnci;

	// Valid position?
	if (FAtacked (rgsqPieceList[xside][0], side))
		return false;

	// Calculate index of current position and probe table
	indTable = pfnCalcIndexSide (rgsqPieceList[x_colorWhite], rgsqPieceList[x_colorBlack],
								 Linear(sqEnPassant), false);

	indTable *= 2;
	tbtTable = (TbtProbeTable (iGlobalTb, side, indTable+1) << 8) +
			   (BYTE) TbtProbeTable (iGlobalTb, side, indTable);

	// Generate all pseudolegal moves
	pmvEnd = PmvGenPseudoLegalMoves (rgmv, side);

	// Now iterate through all of them
	pfnci = pfnCalcIndexXside;
	tbtBest = -65536;
	for (pmv = rgmv; pmv < pmvEnd; pmv ++)
		{
		VMakeMove (pmv);
		// Illegal move?
		if (FAtacked (rgsqPieceList[side][0], xside))
			{
			VUnmakeMove (pmv);
			continue;
			}

		int	tbtScore;

		// If capture/promotion, have to probe minor
		if (0 == pmv->m_ulFlags)
			{
			// Probe opposite table
			ind = pfnci (rgsqPieceList[x_colorWhite],
						 rgsqPieceList[x_colorBlack],
						 Linear(sqEnPassant), false);
			ind *= 2;
			
			tbtScore = (TbtProbeTable (iGlobalTb, xside, ind+1) << 8) +
					   (BYTE) TbtProbeTable (iGlobalTb, xside, ind);
			if (L_bev_broken == tbtScore)
				VInvalid ("Broken opposite", side, indTable, L_bev_broken, L_bev_broken);
			}
		else
			{
			int	iTb;

			// Probe minor table
			iTb = IDescFindFromCounters	(rgiCount);
			if (0 != iTb)
				{
				if (iTb < 0)
					{
					INDEX indexChild;

					iTb = -iTb;
					indexChild = PfnIndCalc(iTb, side) (
										rgsqPieceList[x_colorBlack],
										rgsqPieceList[x_colorWhite],
										XX, true);
					tbtScore = TbtProbeTable (iTb, side, indexChild);
					if (bev_broken == tbtScore)
						VInvalid ("Broken minor", side, indTable, bev_broken, bev_broken);
					}
				else
					{
					INDEX indexChild;

					indexChild = PfnIndCalc(iTb, xside) (
										rgsqPieceList[x_colorWhite],
										rgsqPieceList[x_colorBlack],
										XX, false);
					tbtScore = TbtProbeTable (iTb, xside, indexChild);
					if (bev_broken == tbtScore)
						VInvalid ("Broken minor", side, indTable, bev_broken, bev_broken);
					}
				tbtScore = S_to_L (tbtScore);
				}
			else
				{
				if (2 != rgPieceCount[x_colorWhite]+rgPieceCount[x_colorBlack])
					{
					printf ("*** Unable to probe minor\n");
					VPrintPosition ();
					exit (1);
					}
				tbtScore = 0;
				}
			}
		tbtScore = L_TbtReplaceResult (tbtScore);
		if (L_FBetter (tbtBest, tbtScore))
			tbtBest = tbtScore;
		VUnmakeMove (pmv);
		}

	if (-65536 == tbtBest)	// No legal moves
		tbtBest = (FAtacked (rgsqPieceList[side][0], xside) ? L_bev_li0 : L_bev_draw);
	if (tbtBest != tbtTable)
		VInvalid ("Wrong result", side, indTable, tbtBest, tbtTable);
	return true;
#endif
	}

//-----------------------------------------------------------------------------

static void VCollectStatistics
	(
	BYTE	*pb,
	ULONG	cb
	)
	{
	tb_t *ptbt;

	ptbt = (tb_t*) pb;
	do
		{
		prgulStatistics [128 + *ptbt] ++;
		ptbt ++;
		}
	while (-- cb);
	}

//-----------------------------------------------------------------------------

static void L_VCollectStatistics
	(
	BYTE	*pb,
	ULONG	cb
	)
	{
	do
		{
		prgulStatistics [32768 + L_Get (pb)] ++;
		pb += 2;
		}
	while (cb -= 2);
	}

//-----------------------------------------------------------------------------
//
//		Service routines

static void VTab
	(
	int iLevel
	)
	{
	for (int i = 0; i < iLevel; i ++)
		printf ("  ");
	}

// Name looks reasonable?

bool FReasonableName
	(
	char	*pch
	)
	{
	bool	fWasKing;
	char	*pchTemp;

	if (strlen (pch) > MAX_TOTAL_PIECES)
		return false;
	for (pchTemp = pch; *pchTemp; pchTemp ++)
		*pchTemp = (char) tolower (*pchTemp);
	if ('k' != *pch)
		return false;
	fWasKing = false;
	for (pchTemp = pch+1; *pchTemp; pchTemp ++)
		{
		switch (*pchTemp)
			{
		case 'k':
			if (fWasKing)
				return false;
			else
				fWasKing = true;
			break;
		case 'p':
		case 'n':
		case 'b':
		case 'r':
		case 'q':
			break;
		default:
			return false;
			}
		}
	return fWasKing;
	}

// Print 8-bit statistics

static void VPrintStatistics
	(
	FILE	*fp,
	int		iLevel,
	char	*szSide
	)
	{
	for (int i = 0; i < 256; i ++)
		{
		if (0 != prgulStatistics [i])
			{
			tb_t	bev;

			if (stdout == fp)
				VTab (iLevel + 1);
			bev = (tb_t) (i - 128);
			if (bev_draw == bev)
				fprintf (fp, "%s: Draws:            %8d\n", szSide, prgulStatistics [i]);
			else if (bev_broken == bev)
				fprintf (fp, "%s: Broken positions: %8d\n", szSide, prgulStatistics [i]);
			else if (bev >= bev_mimin && bev <= bev_mi1)
				fprintf (fp, "%s: Mate in %3d:      %8d\n", szSide, bev_mi1 - bev + 1,
															prgulStatistics [i]);
			else if (bev >= bev_li0 && bev <= bev_limax)
				fprintf (fp, "%s: Lost in %3d:      %8d\n", szSide,
														bev_mi1 + bev, prgulStatistics [i]);
			else if (bev_limaxx == bev)
				fprintf (fp, "%s: Lost in %3d:      %8d\n", szSide, 126, prgulStatistics [i]);
			else if (bev_miminx == bev)
				fprintf (fp, "%s: Mate in %3d:      %8d\n", szSide, 127, prgulStatistics [i]);
			else
				fprintf (fp, "%s: ?%3d:             %8d\n", szSide, bev, prgulStatistics [i]);
			}
		}
	fflush (fp);
	}

// Print 16-bit statistics

static void L_VPrintStatistics
	(
	FILE	*fp,
	int		iLevel,
	char	*szSide
	)
	{
	for (int i = 0; i < 65536; i ++)
		{
		if (0 != prgulStatistics [i])
			{
			int	bev;

			if (stdout == fp)
				VTab (iLevel + 1);
			bev = i - 32768;
			if (L_bev_draw == bev)
				fprintf (fp, "%s: Draws:            %8d\n", szSide, prgulStatistics [i]);
			else if (L_bev_broken == bev)
				fprintf (fp, "%s: Broken positions: %8d\n", szSide, prgulStatistics [i]);
			else if (bev >= L_bev_mimin && bev <= L_bev_mi1)
				fprintf (fp, "%s: Mate in %3d:      %8d\n", szSide, L_bev_mi1 - bev + 1,
															prgulStatistics [i]);
			else if (bev >= L_bev_li0 && bev <= L_bev_limax)
				fprintf (fp, "%s: Lost in %3d:      %8d\n", szSide,
														L_bev_mi1 + bev, prgulStatistics [i]);
			else
				fprintf (fp, "%s: ?%3d:             %8d\n", szSide, bev, prgulStatistics [i]);
			}
		}
	fflush (fp);
	}

//-----------------------------------------------------------------------------
//
//		Create pair of tablebases

static void	VProcessTb (char *pch, int iLevel);

static void VBuildIfNecessary
	(
	char	*pch,
	int		iLevel,
	color	side
	)
	{
	int	iTb;
	
	if (0 == strcmp (pch, "kk"))
		return;
	iTb = IDescFindByName (pch);
	if (iTb < 0)
		{
		iTb = -iTb;
		side = OtherSide (side);
		}
	if (0 == iTb ||
		NULL == rgtbdDesc[iTb].m_rgpchFileName[x_colorWhite] ||
		NULL == rgtbdDesc[iTb].m_rgpchFileName[x_colorBlack])
		{
		VProcessTb (pch, iLevel+1);
		iTb = IDescFindByName (pch);
		if (iTb < 0)
			{
			iTb = -iTb;
			side = OtherSide (side);
			}
		}
	
	int	fMapped;

	if (fMapFiles)
		{
#if defined (_WIN32)
		fMapped = FMapTableToMemory (iTb, side);
#else
		fMapped = FReadTableToMemory (iTb, side, NULL);
#endif
		if (!fMapped)
			{
			printf ("*** Unable to map %s", pch);
			exit (1);
			}
		}
	}

static void VRecurse
	(
	char	*pch,
	int		iLevel
	)
	{
	char	rgchRecursive[16];
	char	rgchCapPromote[16];
	char	*pchRecursive = rgchRecursive;
	char	*pchTemp;
	color	side = x_colorBlack;

	do
		{
		if ('k' == *pch)
			side = OtherSide (side);
		else
			{
			strcpy (pchRecursive, pch+1);
			VBuildIfNecessary (rgchRecursive, iLevel, side);
			if ('p' == *pch)
				{
				strcpy (pchRecursive+1, pch+1);
				for (int i = 0; i < 4; i ++)
					{
					int	j;

					// First, handle pawn promotion
					*pchRecursive = "nbrq"[i];
					VBuildIfNecessary (rgchRecursive, iLevel, OtherSide (side));
					// Now, handle promotion with capture
					if (NULL == strchr (pchRecursive, 'k'))
						{
						strcpy (rgchCapPromote, strrchr (rgchRecursive, 'k'));
						j = strlen (rgchCapPromote);
						pchTemp = & rgchRecursive [1];
						}
					else
						{
						rgchCapPromote[0] = 'k';
						for (j = 1; 'k' != rgchRecursive[j]; j ++)
							rgchCapPromote[j] = rgchRecursive[j];
						pchTemp = & rgchRecursive[j+1];
						}
					rgchCapPromote[j] = 'k';
					j ++;
					while ('\0' != *pchTemp && 'k' != *pchTemp)
						{
						if ('p' != *pchTemp)
							{
							int	k;

							for (k = 0; 'k' != pchTemp[1+k] && '\0' != pchTemp[1+k]; k ++)
								rgchCapPromote[j+k] = pchTemp[1+k];
							rgchCapPromote[j+k] = '\0';
							VBuildIfNecessary (rgchCapPromote, iLevel, x_colorBlack);
							VBuildIfNecessary (rgchCapPromote, iLevel, x_colorWhite);
							}
						rgchCapPromote[j] = *pchTemp;
						j ++;
						pchTemp ++;
						}
					}
				}
			}
		* pchRecursive ++ = * pch ++;
		}
	while (*pch);
	}

static void VPrintElapsedTime
	(
	time_t tmStart
	)
	{
	time_t	tmFinish;
	ULONG	ulSeconds, ulMinutes, ulHours;

	tmFinish = time (NULL);
	ulSeconds = (ULONG) (tmFinish - tmStart);
	ulMinutes = ulSeconds / 60;
	ulSeconds %= 60;
	ulHours = ulMinutes / 60;
	ulMinutes %= 60; 
	if (0 != ulHours)
		printf (" %lu hour%s", ulHours, ulHours > 1 ? "s" : "");
	if (0 != ulMinutes)
		printf (" %lu minute%s", ulMinutes, ulMinutes > 1 ? "s" : "");
	if ((0 != ulSeconds) || (0 == ulHours + ulMinutes))
		printf (" %lu second%s", ulSeconds, ulSeconds > 1 ? "s" : "");
	}

// Initialize tables necessary for iterations through positions

static void VInitIterations
	(
	const char	*pchTable
	)
	{
	color	sd;

	VSetCounters (rgiCount, pchTable);
	rgPieceCount[x_colorWhite] = 1;
	rgPieceCount[x_colorBlack] = 1;
	rgPieces[x_colorWhite][0] =
	rgPieces[x_colorBlack][0] = x_pieceKing;
	for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
		{
		for (int iPieceType = 0; iPieceType < 5; iPieceType ++)
			{
			for (int i = 0; i < rgiCount[5 * sd + iPieceType]; i ++)
				rgPieces[sd][rgPieceCount[sd]++] = rgpiPiecesLinear[iPieceType];
			}
		}
	rgfHasPawns[x_colorWhite] = (0 != rgiCount[0]);
	rgfHasPawns[x_colorBlack] = (0 != rgiCount[5]);
	fBothSidesHavePawns = rgfHasPawns[x_colorWhite] && rgfHasPawns[x_colorBlack];
	for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
		{
		for (int i = 0; i < rgPieceCount[sd]; i ++)
			rgpsqNext[sd][i] = (x_piecePawn == rgPieces[sd][i]) ? rgsqNextSquarePawn : rgsqNextSquare;
		}
	// Now use symmetry
#if defined (NEW)
	if (rgfHasPawns[x_colorWhite] | rgfHasPawns[x_colorBlack])
		rgpsqNext[x_colorWhite][0] = rgsqNextSquareHalf;
	else
		rgpsqNext[x_colorWhite][0] = rgsqNextSquareTriangle;
#else
	if (rgfHasPawns[x_colorWhite] | rgfHasPawns[x_colorBlack])
		{
		if (1 == rgPieceCount[x_colorBlack])
			{
			int iw = rgPieceCount[x_colorWhite]-1;

			rgpsqNext[x_colorWhite][iw] =
				(x_piecePawn == rgPieces[x_colorWhite][iw]) ? rgsqNextSquareHalfPawn :
															  rgsqNextSquareHalf;
			}
		else
			{
			int ib = rgPieceCount[x_colorBlack]-2;

			rgpsqNext[x_colorBlack][ib] =
				(x_piecePawn == rgPieces[x_colorBlack][ib]) ? rgsqNextSquareHalfPawn :
															  rgsqNextSquareHalf;
			}
		}
	else
		rgpsqNext[x_colorBlack][rgPieceCount[x_colorBlack]-1] = rgsqNextSquareTriangle;
#endif
	}

static void	VCreateTable
	(
	int iTb,
	int	iLevel
	)
	{
	char	*pchTable;
	time_t	tmStart;
	color	sd;
	int		iIteration;
	char	rgchTbName[2][16];
	char	rgchTempName[2][16];

	pchTable = rgtbdDesc[iTb].m_rgchName;
	VTab (iLevel);
	printf ("Starting %s\n", pchTable);
	fflush (stdout);

	// Check minor tables
	VRecurse (pchTable, iLevel);
	VTab (iLevel);
	printf ("  All minor for %s exists, generating it\n", pchTable);
	fflush (stdout);
	
	// Initialization
	tmStart = time (NULL);
	VInitIterations (pchTable);

	// Build tables
	ULONG	cbTables;

	cbTables = cbMemoryUsed;
#if defined (NEW)
	if (rgtbdDesc[iTb].m_rgcbLength[x_colorWhite]+rgtbdDesc[iTb].m_rgcbLength[x_colorBlack] < cbTables)
		cbTables = rgtbdDesc[iTb].m_rgcbLength[x_colorWhite]+rgtbdDesc[iTb].m_rgcbLength[x_colorBlack];
#endif
	if (tbid_kppkp == iTb)
		{
		cbTables *= 2;
		if (cbTables > cbMemoryUsed)
			cbTables = cbMemoryUsed;
		for (ULONG cb = 0; cb < cbTables; cb += 2)
			{
			pbMemory[cb]   = (BYTE) L_bev_broken;
			pbMemory[cb+1] = (BYTE) (L_bev_broken >> 8);
			}
		}
	else
		memset (pbMemory, bev_broken, cbTables);
	rgpbFile[x_colorWhite] = pbMemory;
	cbCalc = cbMemoryUsed;

	for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
		{
		pfnCalcIndexSide = PfnIndCalc(iTb, sd);
		pfnCalcIndexXside = PfnIndCalc(iTb, OtherSide(sd));
		// Find sizeof(file)
		VTab (iLevel);
		printf ("  %s %s: Determining file size\n", pchTable, PchTm (sd));
		fflush (stdout);
		rgCb[sd] = 0;
		VIterateThroughPositions (sd, tbid_kppkp == iTb ? &L_FCalcMaxIndex: &FCalcMaxIndex);
		rgCb[sd] += 1 + (tbid_kppkp == iTb);
		VTab (iLevel);
		printf ("  %s %s: File size: %lu bytes\n", pchTable, PchTm (sd), rgCb[sd]);
		fflush (stdout);
		if (x_colorWhite == sd)
			{
			cbCalc = (rgCb[x_colorWhite] > cbMemoryUsed) ? 0 : cbMemoryUsed-rgCb[x_colorWhite];
			rgpbFile[x_colorBlack] = rgpbFile[x_colorWhite] + rgCb[x_colorWhite];
			}
		}
	if (rgCb[x_colorWhite] + rgCb[x_colorBlack] > cbMemoryUsed)
		{
		printf ("*** Insufficient memory\n");
		exit (1);
		}
#if defined (NEW)
	if (iTb != tbid_kppkp &&
		rgtbdDesc[iTb].m_rgcbLength[x_colorWhite]+rgtbdDesc[iTb].m_rgcbLength[x_colorBlack] != rgCb[x_colorWhite]+rgCb[x_colorBlack])
		{
		printf ("*** Something wrong with table sizes\n");
		exit (1);
		}
#endif

	// Create files
	for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
		{
		char	*pchExt = PchExt (sd);

		VTab (iLevel);
		printf ("  %s %s: Creating file\n", pchTable, PchTm (sd));
		fflush (stdout);
		strcpy (rgchTbName[sd], rgtbdDesc[iTb].m_rgchName);
		strcat (rgchTbName[sd], pchExt);
		strcpy (rgchTempName[sd], "temp");
		strcat (rgchTempName[sd], pchExt);

		rgFp[sd] = fopen (rgchTempName[sd], "wb+");
		if (NULL == rgFp[sd])
			{
			printf ("*** Unable to create file");
			exit (1);
			}
		if (1 != fwrite (rgpbFile[sd], rgCb[sd], 1, rgFp[sd]))
			{
			printf ("*** Unable to initialize file (1)\n");
			exit (1);
			}
		// Close and re-open file, to be sure it's flushed
		if (0 != fclose (rgFp[sd]))
			{
			printf ("*** Unable to close file (1)\n");
			exit (1);
			}
#if !defined (KPPKP_16BIT)
		rgFp[sd] = (tbid_kppkp == iTb) ? fopen (rgchTempName[sd], "wb+"):
										 fopen (rgchTempName[sd], "rb+");
#else
		rgFp[sd] = fopen (rgchTempName[sd], "rb+");
#endif
		if (NULL == rgFp[sd])
			{
			printf ("*** Unable to re-open file");
			exit (1);
			}
		}

	// Now start iterations
	iIteration = 1;
	tbtLo = bev_li0;
	tbtHi = bev_mi1;
	L_tbtLo = L_bev_li0;
	L_tbtHi = L_bev_mi1;
	do
		{
		for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
			{
			pfnCalcIndexSide = PfnIndCalc(iTb, sd);
			pfnCalcIndexXside = PfnIndCalc(iTb, OtherSide(sd));
			VTab (iLevel);
			printf ("  %s %s: Starting major iteration %d\n",
					pchTable, PchTm (sd), iIteration);
			fflush (stdout);
			fChanged = false;
			VIterateThroughPositions (sd, tbid_kppkp == iTb ? &L_FProbeOpposite : &FProbeOpposite);
			}
		iIteration ++;
		tbtLo = (tb_t) (tbtLo + ((1 == rgPieceCount[x_colorBlack]) || (1 != (iIteration & 1))));
		tbtHi = (tb_t) (tbtHi - ((1 == rgPieceCount[x_colorBlack]) || (1 != (iIteration & 1))));
		L_tbtLo = (L_tbtLo + ((1 == rgPieceCount[x_colorBlack]) || (1 != (iIteration & 1))));
		L_tbtHi = (L_tbtHi - ((1 == rgPieceCount[x_colorBlack]) || (1 != (iIteration & 1))));
		}
	while (fChanged);

	// Collect statistics
	FILE	*fpStatistics;
	char	szStatistics[64];

	VTab (iLevel);
	printf ("  %s %s: Done\n", pchTable, PchTm (sd));
	fflush (stdout);

	strcpy (szStatistics, pchTable);
	strcat (szStatistics, ".tbs");
	fpStatistics = fopen (szStatistics, "wt+");
	if (NULL == fpStatistics)
		{
		printf ("*** Unable to create %s\n", szStatistics);
		exit (1);
		}
	memset (prgulStatistics, 0, 65536 * sizeof (unsigned long));
	if (tbid_kppkp == iTb)
		{
		L_VCollectStatistics (rgpbFile[x_colorWhite], rgCb[x_colorWhite]);
		if (fVerbose)
			L_VPrintStatistics (stdout, iLevel, "wtm");
		L_VPrintStatistics (fpStatistics, iLevel, "wtm");
		}
	else
		{
		VCollectStatistics (rgpbFile[x_colorWhite], rgCb[x_colorWhite]);
		if (fVerbose)
			VPrintStatistics (stdout, iLevel, "wtm");
		VPrintStatistics (fpStatistics, iLevel, "wtm");
		}
	memset (prgulStatistics, 0, 65536 * sizeof (unsigned long));
	if (tbid_kppkp == iTb)
		{
		L_VCollectStatistics (rgpbFile[x_colorBlack], rgCb[x_colorBlack]);
		if (fVerbose)
			L_VPrintStatistics (stdout, iLevel, "btm");
		L_VPrintStatistics (fpStatistics, iLevel, "btm");
		}
	else
		{
		VCollectStatistics (rgpbFile[x_colorBlack], rgCb[x_colorBlack]);
		if (fVerbose)
			VPrintStatistics (stdout, iLevel, "btm");
		VPrintStatistics (fpStatistics, iLevel, "btm");
		}
	fclose (fpStatistics);

	VWriteFile (x_colorWhite, rgpbFile[x_colorWhite], iTb);
	VWriteFile (x_colorBlack, rgpbFile[x_colorBlack], iTb);

	// All done!
	for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
		{
		if (fclose (rgFp[sd]))
			{
			printf ("*** Unable to close file (2)\n");
			exit (1);
			}
		remove (rgchTbName[sd]);
		if (rename (rgchTempName[sd], rgchTbName[sd]))
			{
			printf ("*** Unable to rename file\n");
			exit (1);
			}
		}

	// Register both files
	FCheckExistance (".", iTb, x_colorWhite);
	FCheckExistance (".", iTb, x_colorBlack);

	// Print statistics
	VTab (iLevel);
	printf ("%s done in", pchTable);
	VPrintElapsedTime (tmStart);
	printf ("\n\n");
	fflush (stdout);
	}

// Convert TB name into TB #

static int	IFindTb
	(
	char	*pch
	)
	{
	int	rgiCount1[10], rgiCount2[10];
	
	if (FReasonableName (pch))
		{
		VSetCounters (rgiCount1, pch);
		for (int iTb = 1; iTb < cTb; iTb ++)
			{
			VSetCounters (rgiCount2, rgtbdDesc[iTb].m_rgchName);
			if (0 == memcmp (rgiCount1, rgiCount2, 10 * sizeof (int)) ||
				(0 == memcmp (rgiCount1, rgiCount2+5, 5 * sizeof (int)) &&
				 0 == memcmp (rgiCount1+5, rgiCount2, 5 * sizeof (int))))
				return iTb;
			}
		}
	printf ("*** Illegal tablebase name: %s\n", pch);
	exit (1);
	return 0;	// To make compiler happy
	}

static void VProcessTb
	(
	char	*pch,
	int		iLevel
	)
	{
	VCreateTable (IFindTb (pch), iLevel);
	}

#if defined (CALCULATE_SIZEOF)

static void	VCalcTbSize
	(
	int iTb
	)
	{
	char	*pchTable;
	color	sd;
	int		iIteration;

	pchTable = rgtbdDesc[iTb].m_rgchName;

	// Initialization
	VInitIterations (pchTable);
	
	// Calculate sizeof(tables)

	cbCalc = 0;
	for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
		{
		pfnCalcIndexSide = PfnIndCalc(iTb, sd);
		pfnCalcIndexXside = PfnIndCalc(iTb, OtherSide(sd));
		// Find sizeof(file)
		rgCb[sd] = 0;
		VIterateThroughPositions (sd, &FCalcMaxIndex);
		rgCb[sd] ++;
		}
	}

#endif

//-----------------------------------------------------------------------------
//
//	Verify existing table

static void	VVerifyTb
	(
	char *pch
	)
	{
	char	*pchTable;
	time_t	tmStart;
	color	sd;
	int		iTb;
#if defined (_WIN32)
	HANDLE	rghFile[2];
	HANDLE	rghFileMapping[2];
#endif
	
	iTb = IFindTb (pch);
	pchTable = rgtbdDesc[iTb].m_rgchName;
	printf ("Verifying %s\n", pchTable);
	fflush (stdout);

	// Initialization
	tmStart = time (NULL);
	VInitIterations (pchTable);

	// Map/load tables, if necessary
	if (fMapFiles)
		{
		for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
			{
			int	fMapped;

#if defined (_WIN32)
			fMapped = FMapTableToMemory (iTb, sd, &rghFile[sd], &rghFileMapping[sd]);
#else
			fMapped = FReadTableToMemory (iTb, sd, NULL);
#endif
			if (!fMapped)
				{
				printf ("*** Unable to map %s", pch);
				exit (1);
				}
			}
		}

	// Now check both tables
	iGlobalTb = iTb;
	for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
		{
		pfnCalcIndexSide = PfnIndCalc(iTb, sd);
		pfnCalcIndexXside = PfnIndCalc(iTb, OtherSide(sd));
		VIterateThroughPositions (sd, iTb == tbid_kppkp ? &L_FVerify : &FVerify);
		}
	
	// Unmap files, if necessary
#if defined (_WIN32)
	if (fMapFiles)
		{
		for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
			FUnMapTableFromMemory (iTb, sd, rghFile[sd], rghFileMapping[sd]);
		}
#endif

	// Print statistics
	printf ("%s verified in", pchTable);
	VPrintElapsedTime (tmStart);
	printf ("\n\n");
	fflush (stdout);
	}

//-----------------------------------------------------------------------------

static void VQuery (void)
	{
	char	rgchPieces[128];
	int		rgiCounters[10];
	int		i;
	square	sq;
	piece	pi;
	color	side;
	color	probe;
	int		fInvert;
	int		iTb;
	INDEX	ind;
	piece	*psqW;
	piece	*psqB;
	int		tbValue;

again:
	for (i = 0; i < 120; i ++)
		rgColor[i] = x_colorNeutral;
	for (i = 0; i < 120; i ++)
		rgBoard[i] = x_pieceNone;
	rgPieceCount[x_colorWhite] = 
	rgPieceCount[x_colorBlack] = 1;
	memset (rgiCounters, 0, sizeof (rgiCounters));

	printf ("Enter piece locations (e.g. 'wke3 wpb4 bka6 bph6') or 'quit':\n");
	fflush (stdout);
	gets (rgchPieces);
	fflush (stdout);
	if (0 == memcmp (rgchPieces, "quit", 4))
		return;
	i = 0;
	while ('\0' != rgchPieces[i])
		{
		side = x_colorWhite;
		switch (rgchPieces[i])
			{
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			i ++;
			break;
		case 'b':
			side = x_colorBlack;
			// Fall through
		case 'w':
			if ('\0' == rgchPieces[i+1] ||
				rgchPieces[i+2] < 'a' || rgchPieces[i+2] > 'h' ||
				rgchPieces[i+3] < '1' || rgchPieces[i+3] > '8')
				goto error;
			sq = SquareMake (rgchPieces[i+3]-'1', rgchPieces[i+2] - 'a');
			switch (rgchPieces[i+1])
				{
			case 'p':
				pi = x_piecePawn;
				break;
			case 'n':
				pi = x_pieceKnight;
				break;
			case 'b':
				pi = x_pieceBishop;
				break;
			case 'r':
				pi = x_pieceRook;
				break;
			case 'q':
				pi = x_pieceQueen;
				break;
			case 'k':
				pi = x_pieceKing;
				break;
			default:
				goto error;
				}
			i += 4;
			rgColor[sq] = side;
			rgBoard[sq] = pi;
			if (x_pieceKing == pi)
				rgsqPieceList[side][0] = sq;
			else
				{
				rgiCounters[side*5+pi-x_piecePawn] ++;
				rgsqPieceList[side][rgPieceCount[side]] = sq;
				rgPieceCount[side] ++;
				}
			break;
		default:
			goto error;
			}
		}
		if (rgPieceCount[x_colorWhite] + rgPieceCount[x_colorBlack] < 3 ||
			rgPieceCount[x_colorWhite] + rgPieceCount[x_colorBlack] > 5)
			goto error;
		sqEnPassant = XX;
		for (side = x_colorWhite; side <= x_colorBlack; side ++)
			{
			if (FAtacked (rgsqPieceList[OtherSide(side)][0], side))
				continue;
			iTb = IDescFindFromCounters (rgiCounters);
			if (0 == iTb)
				{
				printf ("%s tablebase not found\n", PchTm (side));
				goto again;
				}
			if (iTb > 0)
				{
				probe = side;
				fInvert = false;
				psqW = rgsqPieceList[x_colorWhite];
				psqB = rgsqPieceList[x_colorBlack];
				}
			else
				{
				probe = OtherSide (side);
				fInvert = true;
				psqW = rgsqPieceList[x_colorBlack];
				psqB = rgsqPieceList[x_colorWhite];
				iTb = -iTb;
				}
			if (!FRegistered(iTb, probe))
				{
				printf ("%s tablebase not found\n", PchTm (side));
				goto again;
				}
			ind = PfnIndCalc(iTb, probe) (psqW, psqB, Linear(sqEnPassant), fInvert);
			tbValue = L_TbtProbeTable (iTb, probe, ind);
			if (tbValue >= L_bev_mimin && tbValue <= L_bev_mi1)
				printf ("%s: Mate in %3d\n", PchTm (side), L_bev_mi1 - tbValue + 1);
			else if (tbValue >= L_bev_li0 && tbValue <= L_bev_limax)
				printf ("%s: Lost in %3d\n", PchTm (side), L_bev_mi1 + tbValue);
			else if (0 == tbValue)
				printf ("%s: Draw\n", PchTm (side));
			else
				printf ("%s: Invalid table value %02X\n", PchTm (side), tbValue & 0xFF);
			}
		goto again;
error:
		printf ("Illegal input - reenter position or press ctrl-C\n");
		goto again;
	}

//-----------------------------------------------------------------------------

static void VShowHelp
	(
	bool fShowHeader
	)
	{
	if (fShowHeader)
		{
		printf
			(
			"New tablebase generator version 1.10\n"
			"(C) 1998 Eugene Nalimov, eugenen@microsoft.com\n"
			"\n"
			);
		}
	printf
		(
		"Usage: tbgen options tablebases\n"
		"\n"
		"Valid options:\n"
		"  -m number - specify memory size (in Mb)\n"
		"  -c number - specify TB cache size (in Mb)\n"
		"  -d dirs   - search TBs in the specified directories\n"
		"  -p        - use file mapping (Win32 only) or virtual memory\n"
		"              (do not use -p together with -c when creating tablebase)\n"
		"  -e        - verify existing tablebases\n"
		"              (you must to specify -c when verifying tablebase)\n"
		"  -v        - verbose mode\n"
		"  -q        - query tablebases\n"
#if defined (_WIN32)
		"  -i        - generator will run with low priority\n"
#endif
		"\n"
		"Example: tbgen -m 570 -p -d c:\\tb;d:\\tb kbpkb\n"
		);
	}

//-----------------------------------------------------------------------------
//
//		Main

static	bool	fVerify = false;
static	bool	fQuery = false;
static	char	*pszDirs = "";

int TB_CDECL main
	(
	int		argc,
	char	*argv[]
	)
	{
	time_t	tmStart;
	int		iArg;

#if defined (CALC_TOTAL_SIZE)
	unsigned uKbSize = 0;

	for (int iTb = 0; iTb < cTb; iTb ++)
		uKbSize +=  (rgtbdDesc[iTb].m_rgcbLength[0] + 1023)/1024 +
					(rgtbdDesc[iTb].m_rgcbLength[1] + 1023)/1024;
	printf ("Total TB size is %dKb\n", uKbSize);
	return 0;
#endif

	prgulStatistics = (unsigned long *) malloc (65536 * sizeof (unsigned long));
	if (NULL == prgulStatistics)
		{
		printf ("Unable to allocate %luKb of memory", 65536 * sizeof (unsigned long) / 1024);
		exit (1);
		}
	tmStart = time (NULL);
	if (1 == argc)
		{
		VShowHelp (false);
		exit (0);
 		}
	// Process non-TB arguments
	for (iArg = 1; iArg < argc;)
		{
		if ('-' == argv[iArg][0] || '/' == argv[iArg][0])
			{
			switch (argv[iArg][1])
				{
			case '?':
				VShowHelp (true);
				exit (0);
			case 'v':
			case 'V':
				fVerbose = true;
				break;
			case 'e':
			case 'E':
				fVerify = true;
				break;
			case 'r':
			case 'R':
				fPrint = true;
				break;
			case 'p':
			case 'P':
				fMapFiles = true;
				break;
#if defined (_WIN32)
			case 'i':
			case 'I':
				SetPriorityClass (GetCurrentProcess, IDLE_PRIORITY_CLASS);
				break;
#endif
			case 'd':
			case 'D':
				if (iArg+1 < argc)
					{
					pszDirs = argv[iArg+1];
					memmove (argv+iArg, argv+iArg+1, (argc-iArg) * sizeof(char*));
					argc --;
					break;
					}
				else
					goto error;
			case 'c':
			case 'C':
				if (iArg+1 < argc)
					{
					char *pchEnd;

					cbMemoryCache = strtoul (argv[iArg+1], &pchEnd, 10);
					if (!isdigit (argv[iArg+1][0]) || '\0' != *pchEnd || 0 == cbMemoryCache)
						goto error;
					cbMemoryCache *= 1024*1024;
					memmove (argv+iArg, argv+iArg+1, (argc-iArg) * sizeof(char*));
					argc --;
					break;
					}
				else
					goto error;
			case 'q':
			case 'Q':
				fQuery = true;
				break;
			case 'm':
			case 'M':
				if (iArg+1 < argc)
					{
					char *pchEnd;

					cbMemoryUsed = strtoul (argv[iArg+1], &pchEnd, 10);
					if (!isdigit (argv[iArg+1][0]) || '\0' != *pchEnd || 0 == cbMemoryUsed)
						goto error;
					cbMemoryUsed *= 1024*1024;
					memmove (argv+iArg, argv+iArg+1, (argc-iArg) * sizeof(char*));
					argc --;
					break;
					}
				// Fall through to error case
			default:
error:			printf ("Illegal option '%s'\n", argv[iArg]);
				VShowHelp (false);
				exit (1);
				}
			memmove (argv+iArg, argv+iArg+1, (argc-iArg) * sizeof(char*));
			argc --;
			}
		else
			iArg ++;
		}

	IInitializeTb (pszDirs);
	VInitializeAttacks();

	if (fQuery)
		{
		pbCache = (BYTE *) malloc (1024*1024);
		if (NULL == pbCache)
			{
			printf ("Unable to allocate %luMb of memory", cbMemoryCache/(1024*1024));
			exit (1);
			}
		FTbSetCacheSize (pbCache, 1024*1024);
		VQuery ();
		return 0;
		}

	if (0 == cbMemoryUsed && !fVerify || 0 == cbMemoryCache && !fMapFiles)
		{
		VShowHelp (false);
		exit (1);
		}
	pbMemory = (BYTE *) malloc (cbMemoryUsed);
	if (NULL == pbMemory)
		{
		printf ("Unable to allocate %luMb of memory", cbMemoryUsed/(1024*1024));
		exit (1);
		}
	if (!fMapFiles || fVerify)
		{
		pbCache = (BYTE *) malloc (cbMemoryCache);
		if (NULL == pbCache)
			{
			printf ("Unable to allocate %luMb of memory", cbMemoryCache/(1024*1024));
			exit (1);
			}
		}

#if defined (CALCULATE_SIZEOF)
	int iTb;

	for (iTb = tbid_kpppk; iTb < cTb; iTb ++)
		{
		VCalcTbSize (iTb);
		if (rgCb[0] != rgtbdDesc[iTb].m_rgcbLength[0] || rgCb[1] != rgtbdDesc[iTb].m_rgcbLength[1])
			printf ("%s: wtm size = %d, btm size = %d\n", rgtbdDesc[iTb].m_rgchName, rgCb[0], rgCb[1]);
		}
	exit (0);
#endif

	FTbSetCacheSize (pbCache, cbMemoryCache);
	for (iArg = 1; iArg < argc; iArg ++)
		{
		if (fVerify)
			VVerifyTb (argv[iArg]);
		else
			VProcessTb (argv[iArg], 0);
		}
	VTbCloseFiles ();

	// Print statistics
	printf ("All done in");
	VPrintElapsedTime (tmStart);
	printf ("\n");

	return 0;
	}
