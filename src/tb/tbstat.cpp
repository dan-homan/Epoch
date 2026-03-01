#define	NEW
#define	TB_CDECL

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

typedef	int	square;
typedef	unsigned char	BYTE;
typedef unsigned long	ULONG;
typedef unsigned long	INDEX;	// Index in a table

#define	XX	-1

#define PIECES_DECLARED

typedef	int	piece;
#define	x_pieceNone		0
#define	x_piecePawn		1
#define	x_pieceKnight	2
#define	x_pieceBishop	3
#define	x_pieceRook		4
#define	x_pieceQueen	5
#define	x_pieceKing		6

square SqFindPiece
	(
	square	*psq,
	piece	pi
	)
	{
	return 0;
	}

#define	SqFindKing(psq)			0
#define	SqFindOne(psq,pi)		0
#define	SqFindFirst(psq,pi)		0
#define	SqFindSecond(psq,pi)	0
#define	SqFindThird(psq,pi)		0

#define	ILLEGAL_POSSIBLE
#define	T41_INCLUDE

#include "tbindex.cpp"

static unsigned long rgulStatistics [256];

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
		rgulStatistics [128 + *ptbt] ++;
		ptbt ++;
		}
	while (-- cb);
	}

static void VPrintStatistics
	(
	FILE	*fp,
	char	*szSide
	)
	{
	for (int i = 0; i < 256; i ++)
		{
		if (0 != rgulStatistics [i])
			{
			tb_t	bev;

			bev = (tb_t) (i - 128);
			if (bev_draw == bev)
				fprintf (fp, "%s: Draws:            %8d\n", szSide, rgulStatistics [i]);
			else if (bev_broken == bev)
				fprintf (fp, "%s: Broken positions: %8d\n", szSide, rgulStatistics [i]);
			else if (bev >= bev_mimin && bev <= bev_mi1)
				fprintf (fp, "%s: Mate in %3d:      %8d\n", szSide, bev_mi1 - bev + 1,
															rgulStatistics [i]);
			else if (bev >= bev_li0 && bev <= bev_limax)
				fprintf (fp, "%s: Lost in %3d:      %8d\n", szSide,
														bev_mi1 + bev, rgulStatistics [i]);
			else if (bev_limaxx == bev)
				fprintf (fp, "%s: Lost in %3d:      %8d\n", szSide, 126, rgulStatistics [i]);
			else if (bev_miminx == bev)
				fprintf (fp, "%s: Mate in %3d:      %8d\n", szSide, 127, rgulStatistics [i]);
			else
				fprintf (fp, "%s: ?%3d:             %8d\n", szSide, bev, rgulStatistics [i]);
			}
		}
	fflush (fp);
	}

int TB_CDECL main
	(
	int		argc,
	char	*argv[],
	char	*envp[]
	)
	{
	FILE	*fp;
	char	c;
	char	szName[128];

	if (2 != argc)
		{
		printf ("*** No file name\n");
		exit (1);
		}
	strcpy (szName, argv[1]);
	fp = fopen (szName, "rb");
	if (NULL == fp)
		{
		printf ("*** cannot open file\n");
		exit (1);
		}
	for (;;)
		{
		ULONG	cb;
		BYTE	rgbTemp[4096];

		cb = fread (rgbTemp, 1, sizeof (rgbTemp), fp);
		if (0 == cb)
			break;
		VCollectStatistics (rgbTemp, cb);
		}
	fclose (fp);
	c = szName[strlen(szName)-1];
	VPrintStatistics (stdout, ('w' == c || 'W' == c) ? "wtm" : "btm");
	return 0;
	}
