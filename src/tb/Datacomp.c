#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC CLK_TCK
#endif

#include "complib.h"

static int proceed (char *name, FILE *fd, int check_crc)
{
  int i, err;
  COMP_FILE info;
  COMP_BLOCK block;
  unsigned long tm, total = 0;

  /* first we should open file */
  err = comp_open_file (&info, name, check_crc);
  if (info == 0)
  {
    printf ("%s: cannot open compressed file\n", name);
    return (err);
  }

  /* then we should create new block */
  err = comp_alloc_block (&block, info);
  if (block == 0)
  {
    printf ("%s: cannot allocate new block\n", name);
    return (err);
  }

  /* start timer */
  tm = clock ();

  /* now walk through all blocks and check them */
  for (i = 0; i < comp_tell_blocks (info); ++i)
  {
    /* exclude I/O time */
    tm -= clock ();

    /* initialize block parameters and read in compressed data	*/
    if ((err = comp_read_block (block, i)) != COMP_ERR_NONE)
    {
      printf ("%s [%4d]: block read error\n", name, i);
    err1:
      comp_free_block (block);
      comp_close_file (info);
      return (err);
    }

    tm += clock ();

    /* indeed, for checking purposes it is not necessary to decode	*/
    if ((err = comp_decode_block (block, block->total)) != COMP_ERR_NONE)
    {
      printf ("%s [%4d]: decode error\n", name, i);
      goto err1;
    }

    /* write data block if necessary */
    if (fd != 0)
    {
      tm -= clock ();
      if (fwrite (block->ptr, block->decoded, 1, fd) != 1)
      {
	err = COMP_ERR_WRITE;
	goto err1;
      }
      tm += clock ();
    }

    /* decode until end of block and compare checksums of original and	*/
    /* decoded data							*/
    /* important notice: if file was opened with "check_crc" set to 0,	*/
    /* this call will return ERR_NONE even if decoded data is wrong	*/
    if ((err = comp_check_crc (block)) != COMP_ERR_NONE)
    {
      printf ("%s [%4d]: wrong CRC\n", name, i);
      goto err1;
    }

    total += block->total;
  }

  /* stop timer */
  tm = clock () - tm;

  /* release memory occupied by data block				*/
  comp_free_block (block);

  /* now close the file and release memory used by decoder	 	*/
  comp_close_file (info);

  printf ("%s (%lu bytes, %.3f sec, %.0f bytes/sec, CRC32 checking is",
    name, total,
    (double) tm / CLOCKS_PER_SEC, total / ((double) tm / CLOCKS_PER_SEC));
  if (check_crc)
    printf (" ON)\n");
  else
    printf (" OFF)\n");

  return (COMP_ERR_NONE);
}

static char comp_ext[5] = ".emd";

int main (int ac, char **av)
{
  int i, res, block_size;
  char action, name[1024];
  FILE *fd;
  COMP_STAT data = 0;

  block_size = comp_max_block ();

  if (ac < 3)
  {
  err:
    printf ("datacomp -- multidimentional data compression utility\n");
    printf ("copyright (c) 1991--1998 by Andrew Kadatach\n");
    printf ("usage: datacomp command file1 file2 file3 ...\n");
    printf ("  commands:\n");
    printf ("  e[:NNN] -- encode files using block of NNN bytes (default is %d)\n", block_size);
    printf ("             (all files will be encoded using first file statistics)\n");
    printf ("             suffix \"%s\" will be added to each name of file with encoded data\n", comp_ext);
    printf ("  d       -- decode original files out of \"%s\"-files\n", comp_ext);
    printf ("             last suffix will be removed from each name\n");
    printf ("  t       -- test integrity of compressed \"%s\"-files\n", comp_ext);
    printf ("  v       -- test integrity of compressed \"%s\"-files and decompression speed\n", comp_ext);
    return (1);
  }

  switch (action = av[1][0])
  {
  case 'e':
    if (av[1][1] != 0)
    {
      char *endptr;
      if (av[1][1] != ':' || av[1][2] < '0' || av[1][2] > '9')
	goto err1;
      block_size = (int) strtol (av[1] + 2, &endptr, 10);
      if (*endptr != 0 || block_size <= 0)
	goto err1;
    }
    break;
  case 'd':
  case 't':
  case 'v':
    if (av[1][1] == 0)
      break;
  default:
  err1:
    printf ("datacomp: wrong command \"%s\"\n\n", av[1]);
    goto err;
  }

  for (i = 2; i < ac; ++i)
  {
    res = 0;
    if (action == 'e')
    {
      if (data == 0)
	res = comp_analyze_file (&data, av[i], block_size);
      if (res == 0 && data != 0)
      {
	strcpy (name, av[i]);
	strcat (name, comp_ext);
	res = comp_encode_file (av[i], name, block_size, data);
      }
    }
    else
    {
      if (strlen (av[i]) < sizeof (comp_ext) || strcmp (av[i] + strlen (av[i]) - sizeof (comp_ext) + 1, comp_ext) != 0)
      {
	printf ("datacomp: file \"%s\" should have extension \"%s\"\n", av[i], comp_ext);
	continue;
      }
      switch (action)
      {
      case 't':
	res = proceed (av[i], 0, 1);
	break;
      case 'v':
	res = proceed (av[i], 0, 1);
	if (res == 0)
	  res = proceed (av[i], 0, 0);
	break;
      case 'd':
	strcpy (name, av[i]);
	{
	char *pch = strrchr (name, '.');
	if (NULL == pch)
	{
	  printf ("datacomp: cannot remove suffix from \"%s\"\n", name);
	  return (0);
	}
	*pch = '\0';
	}
	if ((fd = fopen (name, "wb")) == 0)
	{
	  printf ("datacomp: cannot create \"%s\"\n", name);
	  res = 0;
	}
	else
	{
	  res = proceed (av[i], fd, 1);
	  fclose (fd);
	  if (res != 0) unlink (name);
	}
      }
    }
    if (res != 0)
    {
      printf ("datacomp: ");
      switch (res & 0xff)
      {
      case COMP_ERR_OPEN:   printf ("cannot open file"); break;
      case COMP_ERR_READ:   printf ("read error"); break;
      case COMP_ERR_CREATE: printf ("cannot create output file"); break;
      case COMP_ERR_WRITE:  printf ("write error"); break;
      case COMP_ERR_NOMEM:  printf ("no enough memory"); break;
      case COMP_ERR_BROKEN: printf ("damaged or incorred compressed data"); break;
      case COMP_ERR_PARAM:  printf ("incorrect input parameters"); break;
      case COMP_ERR_LOCKED: printf ("data is locked by some another thread or process"); break;
      default:              printf ("internal error"); break;
      }
      printf (", generated by line %d of \"complib.c\"\n", res >> 8);
    }
  }

  return (0);
}
