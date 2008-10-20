#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void
squeeze (const char *filename, const char *outfile)
{
  FILE *fh;
  FILE *outf;
  unsigned char videoram[40 * 25];
  unsigned char previous[40 * 25];
  int frame = 0;
  unsigned int totalsize = 0;
  unsigned int rletotal = 0;
  unsigned int blockidx = 0;
  const unsigned int blocksize = 4096;
  unsigned int blockno = 0;
  
  fh = fopen (filename, "r");
  outf = fopen (outfile, "w");

  memset (previous, '\0', sizeof (previous));
  memset (videoram, '\0', sizeof (videoram));

  while (!feof (fh))
    {
      int y, x, diff, pass;
      unsigned int diffsize = 0;
      unsigned int rlesize = 0;
      int fragments = 0;
      enum { RLE, DIFF, RAW } frametype;
      
      fscanf (fh, " ---");

      printf ("frame %d\n", frame);

      for (y = 0; y < 25; y++)
        for (x = 0; x < 40; x++)
	  {
	    unsigned int byte;
            fscanf (fh, " %2x", &byte);
	    //printf ("%x\n", byte);
	    videoram[y * 40 + x] = byte;
	  }

      for (pass = 0; pass < 2; pass++)
        {
	  if (pass == 1)
	    {
	      unsigned int framesize;
	      
	      switch (frametype)
	        {
		case RLE:
		  framesize = rlesize + 1;
		  break;
		
		case DIFF:
		  framesize = diffsize + 1;
		  break;
		  
		case RAW:
		  framesize = 40 * 25 + 1;
		  break;
		}
	
	      printf ("frame size: %d bytes\n", framesize);
	      
	      if (blockidx + framesize >= blocksize - 1)
	        {
		  /* Frame will not fit. Start new block.  */
		  blockno++;
		  blockidx = 0;
		  /* Output magic frame.  */
		  fputc (127, outf);
		  /* Seek to next block.  */
		  printf ("starting block %d\n", blockno);
		  fseek (outf, blockno * blocksize, SEEK_SET);
		}
	    
	      switch (frametype)
	        {
		case RLE:
		  printf ("frame type: rle, fragments %d\n", fragments);
		  fputc (fragments & 127, outf);
		  blockidx++;
		  break;
		
		case DIFF:
		  printf ("frame type: diff, fragments %d\n", fragments);
		  fputc ((fragments & 127) | 128, outf);
		  blockidx++;
		  break;
		
		case RAW:
		  printf ("frame type: raw\n");
		  fputc (255, outf);
		  blockidx++;
		  break;
		}
	    }

	  if (pass == 1 && frametype == RAW)
	    {
	      for (diff = 0; diff < 40 * 25; diff++)
	        fputc (videoram[diff], outf);
	      blockidx += 40 * 25;
	      continue;
	    }

	  for (diff = 0; diff < 40 * 25;)
            {
	      if (videoram[diff] != previous[diff])
		{
		  int diffend;
		  int i;
		  int seen = -1, count = 0;

		  for (diffend = diff;
		       diffend < 40 * 25 && diffend < diff + 256;
	               diffend++)
	            {
		      if (videoram[diffend] == previous[diffend])
			break;
		    }

                  if (pass == 0)
		    {
		      printf ("span start: %.4x span length: %d\n", diff,
	        	      diffend - diff);
		      for (i = diff; i < diffend; i++)
	        	printf ("%.2x", videoram[i]);
		      printf ("\n");
		      fragments++;
		      rlesize += 3;
		      diffsize += 3;
		    }
                  else
		    {
		      unsigned int runstart = 0x7c00 + diff;
		      /* For RLE or diff frames, emit a start address and
		         length.  *)
		      /* Diff start.  */
		      fputc (runstart & 255, outf);
		      fputc ((runstart >> 8) & 255, outf);
		      /* Diff length.  */
		      fputc ((diffend - diff) & 255, outf);
		      blockidx += 3;
	              /* Pass == 1, emit raw diff data.  */
		      if (frametype == DIFF)
		        {
			  for (i = diff; i < diffend; i++)
		            fputc (videoram[i], outf);
			  blockidx += diffend - diff;
			}
		    }

		  if (pass == 0 || frametype == RLE)
		    {
        	      /* RLE.  */
		      for (i = diff; i < diffend; i++)
	        	{
			  if (videoram[i] != seen)
			    {
			      if (seen != -1)
		        	{
				  if (pass == 0)
			            {
				      printf ("%d * %.2x, ", count, seen);
			              rlesize += 2;
				    }
				  else
				    {
				      fputc (count & 255, outf);
				      fputc (seen & 255, outf);
				      blockidx += 2;
				    }
				}
			      seen = videoram[i];
			      count = 1;
			    }
			  else
			    count++;
			}

		      if (pass == 0)
			{
			  printf ("%d * %2x\n", count, seen);
			  rlesize += 2;
			  diffsize += diffend - diff;
			}
		      else
		        {
			  fputc (count & 255, outf);
			  fputc (seen & 255, outf);
			  blockidx += 2;
			}
		    }

		  diff = diffend;
		}
	      else
	        diff++;
	    }

          if (pass == 0)
	    {
	      printf ("plain diff for frame: %d\n", diffsize);
	      printf ("rle diff for frame: %d\n", rlesize);
	      if (rlesize < diffsize && rlesize < 1000)
	        frametype = RLE;
	      else if (diffsize <= rlesize && diffsize < 1000)
	        frametype = DIFF;
	      else
	        frametype = RAW;
	      /* This is an encoding limit, but lower values may turn out to
	         be faster.  (Probably not though).  */
	      if (fragments > 126)
	        {
		  printf ("too many fragments (%d), encoding as raw\n",
			  fragments);
	          frametype = RAW;
		}
	    }
        }

      totalsize += diffsize;
      rletotal += rlesize;

      memcpy (previous, videoram, sizeof (previous));
      frame++;
    }

  fclose (fh);
  fclose (outf);
  printf ("total diffed size: %d\n", totalsize);
  printf ("total RLEs diffed size: %d\n", rletotal);
}

int
main (int argc, char *argv[])
{
  if (argc == 3)
    squeeze (argv[1], argv[2]);
  else
    {
      fprintf (stderr, "Usage: squeeze <infile> <outfile>\n");
      return 1;
    }
   
  return 0;
}
