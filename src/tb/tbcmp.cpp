#define	NDEBUG
#define	ILLEGAL_POSSIBLE
#define	T41_INCLUDED

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#define	NEW

#if defined (_MSC_VER)

#undef	TB_CDECL
#define	TB_CDECL	__cdecl
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

#define	Linear(sq)			(rgindLinear [sq]) /* ((INDEX) (((sq) & 7) | (((sq) >> 1) & 0x38))) */
#define	PchColor(side)		(x_colorWhite == side ? "white" : "black")
#define	PchTm(side)			(x_colorWhite == side ? "wtm" : "btm")

#define	MAX_PIECES	4		/* Maximum # of white or black pieces */

color	rgColor [120];
piece	rgBoard [120];
static	square	rgsqPieceList[2][MAX_PIECES];
static	int		rgPieceCount[2];
static	square	sqEnPassant;
int		iTb;

static	const square *rgpsqNext[2][MAX_PIECES];

static const INDEX rgindLinear[] =
	{
	 0,  1,  2,  3,  4,  5,  6,  7, -1, -1, -1, -1, -1, -1, -1, -1,
	 8,  9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1,
	16, 17, 18, 19, 20, 21, 22, 23, -1, -1, -1, -1, -1, -1, -1, -1,
	24, 25, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1, -1, -1, -1,
	32, 33, 34, 35, 36, 37, 38, 39, -1, -1, -1, -1, -1, -1, -1, -1,
	40, 41, 42, 43, 44, 45, 46, 47, -1, -1, -1, -1, -1, -1, -1, -1,
	48, 49, 50, 51, 52, 53, 54, 55, -1, -1, -1, -1, -1, -1, -1, -1,
	56, 57, 58, 59, 60, 61, 62, 63, -1, -1, -1, -1, -1, -1, -1, -1
	};

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

#include "tbindex.cpp"

#undef NEW

#undef	PchExt
#define	rgsqReflectXY			OLD_rgsqReflectXY
#define	rgsqReflectMaskY		OLD_rgsqReflectMaskY
#define	rgsqReflectMaskYandX	OLD_rgsqReflectMaskYandX
#define	rgsqReflectInvertMask	OLD_rgsqReflectInvertMask
#define	T21						OLD_T21
#define	T22						OLD_T22
#define	T31						OLD_T31
#define	T32						OLD_T32
#define	T41						OLD_T41
#define	CTbDesc					OLD_CTbDesc
#define	rgtbdDesc				OLD_rgtbdDesc
#define	CUTbReference			OLD_CUTbReference
#define	rgutbReference			OLD_rgutbReference
#define	PchSetHalfCounters		OLD_PchSetHalfCounters
#define	VSetCounters			OLD_VSetCounters
#define	IDescFindFromCounters	OLD_IDescFindFromCounters
#define	IDescFind				OLD_IDescFind
#define	IDescFindByName			OLD_IDescFindByName
#define	PutbrCreateSubtable		OLD_PutbrCreateSubtable
#define	VCreateEmptyTbTable		OLD_VCreateEmptyTbTable
#define	FRegisterHalf			OLD_FRegisterHalf
#define	FRegisterTb				OLD_FRegisterTb
#define	CTbCache				OLD_CTbCache
#define	CTbCacheBucket			OLD_CTbCacheBucket
#define	ptbcTbCache				OLD_ptbcTbCache
#define	ctbcTbCache				OLD_ctbcTbCache
#define	ptbcHead				OLD_ptbcHead
#define	ptbcTail				OLD_ptbcTail
#define	ptbcFree				OLD_ptbcFree
#define	VTbCloseFile			OLD_VTbCloseFile
#define	VTbCloseFiles			OLD_VTbCloseFiles
#define	VTbClearCache			OLD_VTbClearCache
#define	FTbSetCacheSize			OLD_FTbSetCacheSize
#define	FRegisteredFun			OLD_FRegisteredFun
#undef	FRegistered
#define	PfnIndCalcFun			OLD_PfnIndCalcFun
#undef	PfnIndCalc
#define	FReadTableToMemory		OLD_FReadTableToMemory
#define	TbtProbeTable			OLD_TbtProbeTable
#define	L_TbtProbeTable			OLD_L_TbtProbeTable
#define	FCheckExistance			OLD_FCheckExistance
#define	IInitializeTb			OLD_IInitializeTb
#define	fPrint					OLD_fPrint
#define	fVerbose				OLD_fVerbose
#define	cbAllocated				OLD_cbAllocated
#define	FMapTableToMemory		OLD_FMapTableToMemory
#define	PvMalloc				OLD_PvMalloc
#define	PbMapFileForRead		OLD_PbMapFileForRead
#define	VUnmapFile				OLD_VUnmapFile
#define	fTbTableCreated			OLD_fTbTableCreated
#define	FUnMapTableFromMemory	OLD_FUnMapTableFromMemory
#define	cbEGTBCompBytes			OLD_cbEGTBCompBytes
#define	TB_CRC_CHECK			OLD_TB_CRC_CHECK
#define	cCompressed				OLD_cCompressed
#define	rgpdbDecodeBlocks		OLD_rgpdbDecodeBlocks

int	cbEGTBCompBytes;
#include "tbindex.cpp"

#undef	rgtbdDesc
#undef	FTbSetCacheSize
#undef	TbtProbeTable
#undef	IInitializeTb
#undef	FMapTableToMemory
#undef	FReadTableToMemory

static	piece	rgPieces[2][MAX_PIECES];
static	bool	rgfHasPawns[2];
static	bool	fBothSidesHavePawns;
static	int		rgiCount[10];

typedef bool (* PfnBool) (color side, color xside);

#define	OtherSide(side)		(color) ((side) ^ 1)

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
	
		// Loop through black pieces
		for (int iBlack = 0;;)
			{
			pi = rgPieces[x_colorBlack][iBlack];
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
			(*pfn) (side, xside);

			// Move black piece to the next position
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
					if (0 != iBlack)
						goto nextB;
					goto nextW;
					}
				}
			while (x_pieceNone != rgBoard[sq]);
			assert (FValidSquare (sq));
			VPutPiece (x_colorBlack, sq, pi, iBlack-1);
			}

		// Move white piece to the next position
nextW:
		if (1 == iWhite)
			{
			printf (".");
			fflush (stdout);
			}
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

// Name looks reasonable?

bool FReasonableName
	(
	char	*pch
	)
	{
	bool	fWasKing;

	if (strlen (pch) > MAX_TOTAL_PIECES)
		return false;
	for (char *pchTemp = pch; *pchTemp; pchTemp ++)
		*pchTemp = tolower (*pchTemp);
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

// Compare values

static bool	FCompare
	(
	color	side,
	color	xside
	)
	{
	INDEX	indNew, indOld;
	tb_t	tbtNew, tbtOld;

	indNew = rgtbdDesc[iTb].m_rgpfnCalcIndex[side]
				(rgsqPieceList[x_colorWhite], rgsqPieceList[x_colorBlack], XX, false);
	if (indNew >= rgtbdDesc[iTb].m_rgcbLength[side])
		return false;
	indOld = OLD_rgtbdDesc[iTb].m_rgpfnCalcIndex[side]
				(rgsqPieceList[x_colorWhite], rgsqPieceList[x_colorBlack], XX, false);

	tbtNew = TbtProbeTable (iTb, side, indNew);
	if (bev_broken == tbtNew)
		return false;
	tbtOld = OLD_TbtProbeTable (iTb, side, indOld);
	if (tbtNew != tbtOld)
		{
		int	i;

		printf ("\n*** Differs: new index = %08X old index = %08X\n"
			    "             new value = %02X old value = %02X\n"
				"             side = %d\n",
				indNew, indOld, tbtNew, tbtOld, side);
		printf ("White pieces: %02X", rgsqPieceList[x_colorWhite][0]);
		for (i = rgPieceCount[x_colorWhite]-1; i > 0; i --)
			printf (" %02X", rgsqPieceList[x_colorWhite][i]);
		printf ("\nBlack pieces: %02X", rgsqPieceList[x_colorBlack][0]);
		for (i = rgPieceCount[x_colorBlack]-1; i > 0; i --)
			printf (" %02X", rgsqPieceList[x_colorBlack][i]);
		printf ("\n");
		exit (1);
		}
	return true;
	}

// Main

int TB_CDECL main
	(
	int		argc,
	char	*argv[],
	char	*envp[]
	)
	{
	char	*pchTable;

	if (3 != argc)
		{
		printf ("Usage: tbcmp tablebase old_directory\n");
		exit (1);
		}
	IInitializeTb (".");
	OLD_IInitializeTb (argv[2]);

	pchTable = argv[1];
	int	rgiCount1[10], rgiCount2[10];
	
	if (FReasonableName (pchTable))
		{
		VSetCounters (rgiCount1, pchTable);
		for (iTb = 1; iTb < cTb; iTb ++)
			{
			VSetCounters (rgiCount2, rgtbdDesc[iTb].m_rgchName);
			if (0 == memcmp (rgiCount1, rgiCount2, 10 * sizeof (int)) ||
				(0 == memcmp (rgiCount1, rgiCount2+5, 5 * sizeof (int)) &&
				 0 == memcmp (rgiCount1+5, rgiCount2, 5 * sizeof (int))))
				goto found;
			}
		}
	printf ("Illegal tablebase name: %s\n", pchTable);
	exit (1);
found:
	color	sd;

	// Registered?
	if (NULL == rgtbdDesc[iTb].m_rgpchFileName[0] || NULL == rgtbdDesc[iTb].m_rgpchFileName[1])
		{
		printf ("*** New TB not found\n");
		exit (1);
		}
	// Registered?
	if (NULL == OLD_rgtbdDesc[iTb].m_rgpchFileName[0] || NULL == OLD_rgtbdDesc[iTb].m_rgpchFileName[1])
		{
		printf ("*** Old TB not found\n");
		exit (1);
		}

	// Map it
#if defined (_WIN32)
	if (!FMapTableToMemory (iTb, x_colorWhite) ||
		!FMapTableToMemory (iTb, x_colorBlack) ||
		!OLD_FMapTableToMemory (iTb, x_colorWhite) ||
		!OLD_FMapTableToMemory (iTb, x_colorBlack))
#else
	if (!FReadTableToMemory (iTb, x_colorWhite, NULL) ||
		!FReadTableToMemory (iTb, x_colorBlack, NULL) ||
		!OLD_FReadTableToMemory (iTb, x_colorWhite, NULL) ||
		!OLD_FReadTableToMemory (iTb, x_colorBlack, NULL))
#endif
		{
		printf ("*** Unable to map/read tables\n");
		exit (1);
		}

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
	if (rgfHasPawns[x_colorWhite] | rgfHasPawns[x_colorBlack])
		rgpsqNext[x_colorWhite][0] = rgsqNextSquareHalf;
	else
		rgpsqNext[x_colorWhite][0] = rgsqNextSquareTriangle;

	// Call comparison
	VIterateThroughPositions (x_colorWhite, &FCompare);
	VIterateThroughPositions (x_colorBlack, &FCompare);

	printf ("\nTables are identical\n");
	return 0;
	}
