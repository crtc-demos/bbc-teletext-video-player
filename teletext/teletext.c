/* MODE 7 emulation, stolen from BeebEm.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL.h>
#include <SDL_image.h>

struct VideoState
{
  unsigned char *DataPtr;
  int PixmapLine;
};

struct window
{
  unsigned int cols[8];
};

/* Video state.  */
static struct VideoState VideoState;
static struct window *mainWin;
static const unsigned int CRTC_HorizontalDisplayed = 40;
static const unsigned int CRTC_ScanLinesPerChar = 10;
static const int THalfMode = 0;
static int TeletextStyle = 1;

static unsigned int EM7Font[3][96][20]; // 20 rows to account for "half pixels"

static int Mode7FlashOn = 1;
static int Mode7DoubleHeightFlags[80];
static int CurrentLineBottom = 0;
static int NextLineBottom = 0;

SDL_Surface *screen = NULL;
SDL_Surface *image = NULL;

static void BuildMode7Font(void) {
  FILE *m7File;
  unsigned char m7cc,m7cy;
  unsigned int m7cb;
  unsigned int row1,row2,row3; // row builders for mode 7 graphics
  char TxtFnt[256];
  /* cout <<"Building mode 7 font data structures\n"; */
 // Build enhanced mode 7 font

//->
//--  strcpy(TxtFnt,RomPath);
//--  strcat(TxtFnt,"teletext.fnt");
//++
//  strcpy(TxtFnt, DATA_DIR);
//  strcat(TxtFnt, "/resources/teletext.fnt");

  //GetLocation_teletext(TxtFnt, 256);
  strcpy (TxtFnt, "teletext.fnt");
  //pINFO(dL"Loading teletext font: '%s'", dR, TxtFnt);
//<-

  m7File=fopen(TxtFnt,"rb");
  if (m7File == NULL)
  {
    char errstr[200];
    sprintf(errstr, "Cannot open Teletext font file teletext.fnt");
    //MessageBox(GETHWND,errstr,"BBC Emulator",MB_OK|MB_ICONERROR);
    exit(1);
  }
  for (m7cc=32;m7cc<=127;m7cc++) {
    for (m7cy=0;m7cy<=17;m7cy++) {
      m7cb=fgetc(m7File);
      m7cb|=fgetc(m7File)<<8;
      EM7Font[0][m7cc-32][m7cy+2]=m7cb<<2;
      EM7Font[1][m7cc-32][m7cy+2]=m7cb<<2;
      EM7Font[2][m7cc-32][m7cy+2]=m7cb<<2;
    }
    EM7Font[0][m7cc-32][0]=EM7Font[1][m7cc-32][0]=EM7Font[2][m7cc-32][0]=0;
    EM7Font[0][m7cc-32][1]=EM7Font[1][m7cc-32][1]=EM7Font[2][m7cc-32][1]=0;
  }
  fclose(m7File);
  // Now fill in the graphics - this is built from an algorithm, but has certain lines/columns
  // blanked for separated graphics.
  for (m7cc=0;m7cc<96;m7cc++) {
    // here's how it works: top two blocks: 1 & 2
    // middle two blocks: 4 & 8
    // bottom two blocks: 16 & 64
    // its only a grpahics character if bit 5 (32) is clear.
    if (!(m7cc & 32)) {
      row1=0; row2=0; row3=0;
      // left block has a value of 4032, right 63 and both 4095
      if (m7cc & 1) row1|=4032;
      if (m7cc & 2) row1|=63;
      if (m7cc & 4) row2|=4032;
      if (m7cc & 8) row2|=63;
      if (m7cc & 16) row3|=4032;
      if (m7cc & 64) row3|=63;
      // now input these values into the array
      // top row of blocks - continuous
      EM7Font[1][m7cc][0]=EM7Font[1][m7cc][1]=EM7Font[1][m7cc][2]=row1;
      EM7Font[1][m7cc][3]=EM7Font[1][m7cc][4]=EM7Font[1][m7cc][5]=row1;
      // Separated
      row1&=975; // insert gaps
      EM7Font[2][m7cc][0]=EM7Font[2][m7cc][1]=EM7Font[2][m7cc][2]=row1;
      EM7Font[2][m7cc][3]=row1; EM7Font[2][m7cc][4]=EM7Font[2][m7cc][5]=0;
      // middle row of blocks - continuous
      EM7Font[1][m7cc][6]=EM7Font[1][m7cc][7]=EM7Font[1][m7cc][8]=row2;
      EM7Font[1][m7cc][9]=EM7Font[1][m7cc][10]=EM7Font[1][m7cc][11]=row2;
      EM7Font[1][m7cc][12]=EM7Font[1][m7cc][13]=row2;
      // Separated
      row2&=975; // insert gaps
      EM7Font[2][m7cc][6]=EM7Font[2][m7cc][7]=EM7Font[2][m7cc][8]=row2;
      EM7Font[2][m7cc][9]=EM7Font[2][m7cc][10]=EM7Font[2][m7cc][11]=row2;
      EM7Font[2][m7cc][12]=EM7Font[2][m7cc][13]=0;
      // Bottom row - continuous
      EM7Font[1][m7cc][14]=EM7Font[1][m7cc][15]=EM7Font[1][m7cc][16]=row3;
      EM7Font[1][m7cc][17]=EM7Font[1][m7cc][18]=EM7Font[1][m7cc][19]=row3;
      // Separated
      row3&=975; // insert gaps
      EM7Font[2][m7cc][14]=EM7Font[2][m7cc][15]=EM7Font[2][m7cc][16]=row3;
      EM7Font[2][m7cc][17]=row3; EM7Font[2][m7cc][18]=EM7Font[2][m7cc][19]=0;
    } // check for valid char to modify
  } // character loop.
}; /* BuildMode7Font */

void
horizline (unsigned long colour, int y, int sx, int width)
{
  int x;
  unsigned int *p = screen->pixels;
  
  p += y * screen->pitch / 4 + sx;
  
  for (x = 0; x < width; x++)
    *p++ = colour;
}

/* Do all the pixel rows for one row of teletext characters                                                    */
static void DoMode7Row(void) {
  char *CurrentPtr = (char *) VideoState.DataPtr;
  int CurrentChar;
  int XStep;
  unsigned char byte;
  unsigned int tmp;

  unsigned int Foreground=mainWin->cols[7];
  unsigned int ActualForeground;
  unsigned int Background=mainWin->cols[0];
  int Flash=0; /* i.e. steady */
  int DoubleHeight=0; /* Normal */
  int Graphics=0; /* I.e. alpha */
  int Separated=0; /* i.e. continuous graphics */
  int HoldGraph=0; /* I.e. don't hold graphics - I don't know what hold graphics is anyway! */
  // That's ok. Nobody else does either, and nor do I. - Richard Gellman.
  int HoldGraphChar=32; // AHA! we know what it is now, this is the character to "hold" during control codes
  unsigned int CurrentCol[20]={0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff
  ,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff,0xffffff};
  int CurrentLen[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int CurrentStartX[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int CurrentScanLine;
  int CurrentX=0;
  int CurrentPixel;
  unsigned int col;
  int FontTypeIndex=0; /* 0=alpha, 1=contiguous graphics, 2=separated graphics */

  if (CRTC_HorizontalDisplayed>80) return; /* Not possible on beeb - and would break the double height lookup array */

  XStep=1;

  for(CurrentChar=0;CurrentChar<CRTC_HorizontalDisplayed;CurrentChar++) {
    byte=CurrentPtr[CurrentChar]; 
    if (byte<32) byte+=128; // fix for naughty programs that use 7-bit control codes - Richard Gellman
    if ((byte & 32) && (Graphics)) HoldGraphChar=byte;
    if ((byte>=128) && (byte<=159)) {
    switch (byte) {
      case 129:
  case 130:
  case 131:
  case 132:
  case 133:
  case 134:
  case 135:
    Foreground=mainWin->cols[byte-128];
    Graphics=0;
    break;

  case 136:
    Flash=1;
    break;

  case 137:
    Flash=0;
    break;

  case 140:
    DoubleHeight=0;
    break;

  case 141:
      if (!CurrentLineBottom) NextLineBottom=1;
    DoubleHeight=1;
    break;

  case 145:
  case 146:
  case 147:
  case 148:
  case 149:
  case 150:
  case 151:
    Foreground=mainWin->cols[byte-144];
    Graphics=1;
    break;

  case 152: /* Conceal display - not sure about this */
    Foreground=Background;
    break;

  case 153:
    Separated=0;
    break;

  case 154:
    Separated=1;
    break;

  case 156:
    Background=mainWin->cols[0];
    break;

  case 157:
    Background=Foreground;
    break;

  case 158:
    HoldGraph=1;
    break;

  case 159:
    HoldGraph=0;
    break;
      }; /* Special character switch */
    // This next line hides any non double height characters on the bottom line
      /* Fudge so that the special character is just displayed in background */
      if ((HoldGraph==1) && (Graphics==1)) byte=HoldGraphChar; else byte=32;
      FontTypeIndex=Graphics?(Separated?2:1):0;
    }; /* test for special character */
  if ((CurrentLineBottom) && ((byte&127)>31) && (!DoubleHeight)) byte=32;
  if ((CRTC_ScanLinesPerChar<=9) || (THalfMode)) TeletextStyle=2; else TeletextStyle=1;
    /* Top bit never reaches character generator */
    byte&=127;
    /* Our font table goes from character 32 up */
    if (byte<32) byte=0; else byte-=32; 

    /* Conceal flashed text if necessary */
    ActualForeground=(Flash && !Mode7FlashOn)?Background:Foreground;
    if (!DoubleHeight) {
      for(CurrentScanLine=0+(TeletextStyle-1);CurrentScanLine<20;CurrentScanLine+=TeletextStyle) {
  tmp=EM7Font[FontTypeIndex][byte][CurrentScanLine];
    //tmp=1365;
  if ((tmp==0) || (tmp==255)) {
    col=(tmp==0)?Background:ActualForeground;
    if (col==CurrentCol[CurrentScanLine]) CurrentLen[CurrentScanLine]+=12*XStep; else {
      if (CurrentLen[CurrentScanLine])
        horizline(CurrentCol[CurrentScanLine],VideoState.PixmapLine+CurrentScanLine,CurrentStartX[CurrentScanLine],CurrentLen[CurrentScanLine]);
      CurrentCol[CurrentScanLine]=col;
      CurrentStartX[CurrentScanLine]=CurrentX;
      CurrentLen[CurrentScanLine]=12*XStep;
    }; /* same colour */
  } else {
    for(CurrentPixel=0x800;CurrentPixel;CurrentPixel=CurrentPixel>>1) {
      /* Background or foreground ? */
      col=(tmp & CurrentPixel)?ActualForeground:Background;

      /* Do we need to draw ? */
      if (col==CurrentCol[CurrentScanLine]) CurrentLen[CurrentScanLine]+=XStep; else {
        if (CurrentLen[CurrentScanLine]) 
    horizline(CurrentCol[CurrentScanLine],VideoState.PixmapLine+CurrentScanLine,CurrentStartX[CurrentScanLine],CurrentLen[CurrentScanLine]);
        CurrentCol[CurrentScanLine]=col;
        CurrentStartX[CurrentScanLine]=CurrentX;
        CurrentLen[CurrentScanLine]=XStep;
      }; /* Fore/back ground */
      CurrentX+=XStep;
    }; /* Pixel within byte */
    CurrentX-=12*XStep;
  }; /* tmp!=0 */
      }; /* Scanline for */
      CurrentX+=12*XStep;
      Mode7DoubleHeightFlags[CurrentChar]=1; /* Not double height - so if the next line is double height it will be top half */
    } else {
      int ActualScanLine;
      /* Double height! */
      for(CurrentPixel=0x800;CurrentPixel;CurrentPixel=CurrentPixel>>1) {
  for(CurrentScanLine=0+(TeletextStyle-1);CurrentScanLine<20;CurrentScanLine+=TeletextStyle) {
    if (!CurrentLineBottom) ActualScanLine=CurrentScanLine >> 1; else ActualScanLine=10+(CurrentScanLine>>1);
    /* Background or foreground ? */
    col=(EM7Font[FontTypeIndex][byte][ActualScanLine] & CurrentPixel)?ActualForeground:Background;

    /* Do we need to draw ? */
    if (col==CurrentCol[CurrentScanLine]) CurrentLen[CurrentScanLine]+=XStep; else {
      if (CurrentLen[CurrentScanLine])  {
        horizline(CurrentCol[CurrentScanLine],VideoState.PixmapLine+CurrentScanLine,CurrentStartX[CurrentScanLine],CurrentLen[CurrentScanLine]);
      };
      CurrentCol[CurrentScanLine]=col;
      CurrentStartX[CurrentScanLine]=CurrentX;
      CurrentLen[CurrentScanLine]=XStep;
    }; /* Fore/back ground */
  }; /* Scanline for */
  CurrentX+=XStep;
      }; /* Pixel within byte */
      Mode7DoubleHeightFlags[CurrentChar]^=1; /* Not double height - so if the next line is double height it will be top half */
  };
  }; /* character loop */

  /* Finish off right bits of scan line */
  for(CurrentScanLine=0+(TeletextStyle-1);CurrentScanLine<20;CurrentScanLine+=TeletextStyle) {
    if (CurrentLen[CurrentScanLine])
      horizline(CurrentCol[CurrentScanLine],VideoState.PixmapLine+CurrentScanLine,CurrentStartX[CurrentScanLine],CurrentLen[CurrentScanLine]);
  };
  CurrentLineBottom=NextLineBottom;
  NextLineBottom=0;
}; /* DoMode7Row */

/* Teletext image match optimisation.  */

void
render_row (unsigned char *videoram, int row)
{
  int i;

  if (SDL_MUSTLOCK (screen))
    SDL_LockSurface (screen);

  VideoState.DataPtr = &videoram[row * 40];
  VideoState.PixmapLine = row * 20;

  for (i = row * 20; i < (row + 1) * 20; i++)
    DoMode7Row ();

  if (SDL_MUSTLOCK (screen))
    SDL_UnlockSurface (screen);
}

#define COSTBITS (1U << 7)

int
state_for_char (unsigned char schar, int oldstate)
{
  int fgcol = oldstate & 7, bgcol = (oldstate >> 3) & 7;
  int sepgraph = (oldstate >> 6) & 1;
  
  switch (schar)
    {
    case 0x91: case 0x92: case 0x93:
    case 0x94: case 0x95: case 0x96: case 0x97:
      fgcol = schar - 0x90;
      break;

    case 0x99:
      sepgraph = 0;
      break;

    case 0x9a:
      sepgraph = 1;
      break;

    case 0x9c:
      bgcol = 0;
      break;

    case 0x9d:
      bgcol = fgcol;
      break;

    default:
      return oldstate;
    }

  return fgcol | (bgcol << 3) | (sepgraph << 6);
}

int
char_cost (int row, int col)
{
  int cost = 0;
  float char_cost = 0;
  int x, y;

  if (SDL_MUSTLOCK (screen))
    SDL_LockSurface (screen);
    
  for (y = row * 20; y < (row + 1) * 20; y++)
    {
      unsigned int *scr = screen->pixels;
      unsigned int *img = image->pixels;

      scr += y * screen->pitch / 4;
      img += y * image->pitch / 4;

      for (x = col * 12; x < (col + 1) * 12; x++)
        {
          unsigned int spix = scr[x], ipix = img[x];
#if 1
          int ir, ig, ib, sr, sg, sb;

          sb = (spix >> 16) & 0xff;
          sg = (spix >> 8) & 0xff;
          sr = spix & 0xff;
          ib = (ipix >> 16) & 0xff;
          ig = (ipix >> 8) & 0xff;
          ir = ipix & 0xff;
#else
          Uint8 ir, ig, ib, sr, sg, sb;
          SDL_GetRGB (spix, screen->format, &sr, &sg, &sb);
          SDL_GetRGB (ipix, image->format, &ir, &ig, &ib);
#endif

          char_cost += (ir - sr) * (ir - sr) + (ig - sg) * (ig - sg)
                       + (ib - sb) * (ib - sb);
        }
    }

  /* The division is just to avoid integer overflow.  */
  cost = char_cost / 100.0;

  if (SDL_MUSTLOCK (screen))
    SDL_UnlockSurface (screen);
  
  return cost;
}

int
choose_char (int row, int col, unsigned int state, int ctrl,
             unsigned char *choice)
{
  int fgcol = state & 7, bgcol = (state >> 3) & 7, sepgfx = (state >> 6) & 1;
  int x, y;
  int fr = (fgcol & 1) ? 76 : 0;
  int fg = (fgcol & 2) ? 151 : 0;
  int fb = (fgcol & 4) ? 28 : 0;
  int br = (bgcol & 1) ? 76 : 0;
  int bg = (bgcol & 2) ? 151 : 0;
  int bb = (bgcol & 4) ? 28 : 0;
  int cost_on[6] = {0, 0, 0, 0, 0, 0};
  int cost_off[6] = {0, 0, 0, 0, 0, 0};
  const int blockbit[6] = {1, 2, 4, 8, 16, 64};
  int i;
  int cost_total = 0;
  unsigned char makechar = 0;
  
  for (y = 0; y < 20; y++)
    {
      int yblk = (y < 6) ? 0 : (y < 14) ? 1 : 2;
      int yrow = row * 20 + y;
      unsigned int *img = image->pixels;
      
      img += yrow * image->pitch / 4;

      for (x = 0; x < 12; x++)
        {
	  int xblk = (x < 6) ? 0 : 1;
	  int xrow = col * 12 + x;
	  unsigned int ipix = img[xrow];
	  int ir, ig, ib;
	  int sfr = fr, sfg = fg, sfb = fb;
	  int block = yblk * 2 + xblk;
	  
	  ir = (((ipix >> 16) & 0xff) * 76) >> 8;
	  ig = (((ipix >> 8) & 0xff) * 151) >> 8;
	  ib = ((ipix & 0xff) * 28) >> 8;
	  
#if 1
	  if (sepgfx
	      && ((x < 2 || (x >= 6 && x < 8))
	          || ((y >= 4 && y < 6) || (y >= 12 && y < 14) || y >= 18)))
	    {
	      sfr = br;
	      sfg = bg;
	      sfb = bb;
	    }
#else
	  if (sepgfx)
	    {
	      sfr = (fr * 9 + br * 7) >> 4;
	      sfg = (fg * 9 + bg * 7) >> 4;
	      sfb = (fb * 9 + bb * 7) >> 4;
	    }
#endif

          if (!ctrl)
	    cost_on[block] += (ir - sfr) * (ir - sfr)
	                      + (ig - sfg) * (ig - sfg)
			      + (ib - sfb) * (ib - sfb);
	  cost_off[block] += (ir - br) * (ir - br)
	                     + (ig - bg) * (ig - bg)
			     + (ib - bb) * (ib - bb);
	}
    }
  
  makechar = 0x20;

  for (i = 0; i < 6; i++)
    if ((cost_on[i] < cost_off[i]) && !ctrl)
      {
        cost_total += cost_on[i];
	makechar |= blockbit[i];
      }
    else
      cost_total += cost_off[i];

  if (choice)
    *choice= makechar;
  
  return cost_total;
}

#define NEWALGO 1

static int rhs_costs_in_state[COSTBITS][41];
static unsigned char char_for_xpos_in_state[COSTBITS][41];

#ifndef NEWALGO
static int char_costs_at_xpos_in_state[COSTBITS][41][256] = {0};
#endif

void
clear_rhs_costs_in_state (void)
{
  int b, r;
#ifndef NEWALGO
  int x;
#endif
  
  for (b = 0; b < COSTBITS; b++)
    for (r = 0; r <= 40; r++)
      {
        rhs_costs_in_state[b][r] = -1;
        char_for_xpos_in_state[b][r] = 'X';
#ifndef NEWALGO
        for (x = 0; x < 256; x++)
          char_costs_at_xpos_in_state[b][r][x] = -1;
#endif
      }
}

#define LHS 0
#define RHS 40


static unsigned char output[40];

int
select_char (unsigned char *videoram, int row, int xpos, int state)
{
  unsigned char chars[] =
    {
#ifdef NEWALGO
      0x0,  /* Magic value: calculate best (graphics) character to use.  */
#else
      0x20, 0x21, 0x22, 0x24, 0x28, 0x30, 0x60,   /* block gfx.  */
#endif
      0x9c, 0x9d,                                 /* fill/don't fill.  */
      0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,   /* colours.  */
      0x99, 0x9a                                  /* sep/contiguous gfx.  */
    };
#ifndef NEWALGO
  int block_costs[7];
  int rest_costs_plain = -1;
  int combined_block_cost;
  unsigned int bitmask = 0;
  unsigned char *prow = &videoram[row * 40];
#endif
  int try;
  int lowest_cost = INT_MAX;
  int lowest_char = 0;
  int fill;
  
  if (xpos == RHS)
    return 0;
  
  if (rhs_costs_in_state[state][xpos] != -1)
    return rhs_costs_in_state[state][xpos];
      
  for (try = 0; try < sizeof (chars); try++)
    {
      int cost, this_cost, rest_costs, newstate;
      unsigned char try_char = chars[try];

      newstate = state_for_char (try_char, state);

#ifdef NEWALGO
      /* Can't choose a graphics char if we've not selected a graphical
         foreground colour.  */
      if (try_char == 0x0 && (newstate & 7) == 0)
        continue;

      if (try_char == 0x0)
        this_cost = choose_char (row, xpos, newstate, 0, &try_char);
      else
        this_cost = choose_char (row, xpos, newstate, 1, 0);
      rest_costs = select_char (videoram, row, xpos + 1, newstate);
#else
      if (char_costs_at_xpos_in_state[newstate][xpos][try_char] == -1)
        {
          if (try_char < 0x80 || try_char > 0x9f)
            {
              for (fill = xpos; fill < 40; fill++)
                prow[fill] = try_char;

              render_row (videoram, row);

              for (fill = xpos; fill < 40; fill++)
                char_costs_at_xpos_in_state[newstate][fill][try_char]
                  = char_cost (row, fill);
            }
          /* Control chars render as blank (as long as we don't support
             hold-graphics mode).  */
          else if (char_costs_at_xpos_in_state[newstate][xpos][0x20] != -1)
            char_costs_at_xpos_in_state[newstate][xpos][try_char]
              = char_costs_at_xpos_in_state[newstate][xpos][0x20];
          else
            {
              prow[xpos] = try_char;
              render_row (videoram, row);
              char_costs_at_xpos_in_state[newstate][xpos][try_char]
                = char_cost (row, xpos);
            }
        }

      this_cost = char_costs_at_xpos_in_state[newstate][xpos][try_char];
      if (rest_costs_plain == -1 || state != newstate)
        rest_costs = select_char (videoram, row, xpos + 1, newstate);
      else
        rest_costs = rest_costs_plain;

      if (try < 7)
        {
          block_costs[try] = this_cost;
          if (rest_costs_plain == -1)
            rest_costs_plain = rest_costs;
        }
#endif

      cost = this_cost + rest_costs;

      if (rhs_costs_in_state[newstate][xpos + 1] == -1)
        {
          rhs_costs_in_state[newstate][xpos + 1] = rest_costs;
          char_for_xpos_in_state[newstate][xpos + 1] = output[xpos + 1];
        }

      if (cost < lowest_cost)
        {
          lowest_cost = cost;
          lowest_char = try_char;
        }
    }

#ifndef NEWALGO
  if ((state & 0x7) != 0)
    {
      /* Try combining blocks to get a better match (only do this if we've
         selected a graphics foreground colour).  */
      bitmask = 0x20;
      bitmask |= (block_costs[1] < block_costs[0]) ? 1 : 0;
      bitmask |= (block_costs[2] < block_costs[0]) ? 2 : 0;
      bitmask |= (block_costs[3] < block_costs[0]) ? 4 : 0;
      bitmask |= (block_costs[4] < block_costs[0]) ? 8 : 0;
      bitmask |= (block_costs[5] < block_costs[0]) ? 16 : 0;
      bitmask |= (block_costs[6] < block_costs[0]) ? 64 : 0;

      combined_block_cost = rest_costs_plain + block_costs[0]
        - ((block_costs[1] < block_costs[0])
           ? block_costs[0] - block_costs[1] : 0)
        - ((block_costs[2] < block_costs[0])
           ? block_costs[0] - block_costs[2] : 0)
        - ((block_costs[3] < block_costs[0])
           ? block_costs[0] - block_costs[3] : 0)
        - ((block_costs[4] < block_costs[0])
           ? block_costs[0] - block_costs[4] : 0)
        - ((block_costs[5] < block_costs[0])
           ? block_costs[0] - block_costs[5] : 0)
        - ((block_costs[6] < block_costs[0])
           ? block_costs[0] - block_costs[6] : 0);

      if (combined_block_cost < lowest_cost)
        {
          lowest_cost = combined_block_cost;
          lowest_char = bitmask;
        }
    }
#endif

  output[xpos] = lowest_char;
  
  /* ??? Fix output.  */
  if (xpos == LHS)
    for (fill = LHS + 1; fill < RHS; fill++)
      {
        state = state_for_char (output[fill - 1], state);
        output[fill] = char_for_xpos_in_state[state][fill];
      }
  
  return lowest_cost;
}

void
randomize (void)
{
  int x, y;
  
  for (y = 0; y < 500; y++)
    {
      unsigned int *img = image->pixels;
      
      img += y * image->pitch / 4;
      
      for (x = 0; x < 480; x++)
	{
	  unsigned int ipix = img[x];
	  int r, g, b;
	  
	  r = (ipix >> 16) & 0xff;
	  g = (ipix >> 8) & 0xff;
	  b = ipix & 0xff;
	  
	  r += (rand () & 255) - 127;
	  g += (rand () & 255) - 127;
	  b += (rand () & 255) - 127;
	  
	  if (r < 0)
	    r = 0;
	  if (g < 0)
	    g = 0;
	  if (b < 0)
	    b = 0;
	  if (r > 255)
	    r = 255;
	  if (g > 255)
	    g = 255;
	  if (b > 255)
	    b = 255;
	  
	  img[x] = b | (g << 8) | (r << 16);
	}
    }
}

void
block (int xpos, int ypos, int xsize, int ysize, int read,
       int *radj, int *gadj, int *badj)
{
  int x, y;
  int rtot = 0, gtot = 0, btot = 0;
  
  if ((xpos + xsize) >= 480 || xpos < 0 || (ypos + ysize) >= 500 || ypos < 0)
    return;
  
  for (y = 0; y < ysize; y++)
    {
      unsigned int *img = image->pixels;
      
      img += (ypos + y) * image->pitch / 4;

      for (x = 0; x < xsize; x++)
	{
	  unsigned int ipix = img[xpos + x];
	  int r, g, b;
	  
	  r = (ipix >> 16) & 0xff;
	  g = (ipix >> 8) & 0xff;
	  b = ipix & 0xff;
	  
	  if (read)
	    {
	      rtot += r;
	      gtot += g;
	      btot += b;
	    }
	  else
	    {
	      /* Warning: fudge factor below!  */
	      r += *radj / 3;
	      g += *gadj / 3;
	      b += *badj / 3;
	      
	      r = (r < 0) ? 0 : (r > 255) ? 255 : r;
	      g = (g < 0) ? 0 : (g > 255) ? 255 : g;
	      b = (b < 0) ? 0 : (b > 255) ? 255 : b;

	      img[xpos + x] = b | (g << 8) | (r << 16);
	    }
	}
    }
  
  if (read)
    {
      *radj = rtot / (xsize * ysize);
      *gadj = gtot / (xsize * ysize);
      *badj = btot / (xsize * ysize);
    }
}

void
diffuse (void)
{
  int x, y;
  int l_to_r = 1;
  
  for (y = 0; y < 500; y += 20)
    {
      int offsets[3] = {0, 6, 14};
      int sizes[3] = {6, 8, 6};
      int row;
      
      for (row = 0; row < 3; row++)
        {
	  int yblktop = y + offsets[row];
	  int ynexttop = y + offsets[row] + sizes[row];
	  int ynextsize = sizes[(row + 1) % 3];
	  
	  if (l_to_r)
	    for (x = 0; x < 480; x += 6)
              {
		int ravg, gavg, bavg;
		int rsat, gsat, bsat;
		int rerr, gerr, berr;

		block (x, yblktop, 6, sizes[row], 1, &ravg, &gavg, &bavg);
		rsat = (ravg > 127) ? 255 : 0;
		gsat = (gavg > 127) ? 255 : 0;
		bsat = (bavg > 127) ? 255 : 0;
		rerr = ((ravg - rsat) * 7) / 16;
		gerr = ((gavg - gsat) * 7) / 16;
		berr = ((bavg - bsat) * 7) / 16;
		block (x + 6, yblktop, 6, sizes[row], 0, &rerr, &gerr, &berr);
		rerr = ((ravg - rsat) * 3) / 16;
		gerr = ((gavg - gsat) * 3) / 16;
		berr = ((bavg - bsat) * 3) / 16;
		block (x - 6, ynexttop, 6, ynextsize, 0, &rerr, &gerr, &berr);
		rerr = ((ravg - rsat) * 5) / 16;
		gerr = ((gavg - gsat) * 5) / 16;
		berr = ((bavg - bsat) * 5) / 16;
		block (x, ynexttop, 6, ynextsize, 0, &rerr, &gerr, &berr);
		rerr = ((ravg - rsat) * 1) / 16;
		gerr = ((gavg - gsat) * 1) / 16;
		berr = ((bavg - bsat) * 1) / 16;
		block (x + 6, ynexttop, 6, ynextsize, 0, &rerr, &gerr, &berr);
	      }
	  else
            for (x = 474; x >= 0; x -= 6)
	      {
		int ravg, gavg, bavg;
		int rsat, gsat, bsat;
		int rerr, gerr, berr;

		block (x, yblktop, 6, sizes[row], 1, &ravg, &gavg, &bavg);
		rsat = (ravg > 127) ? 255 : 0;
		gsat = (gavg > 127) ? 255 : 0;
		bsat = (bavg > 127) ? 255 : 0;
		rerr = ((ravg - rsat) * 7) / 16;
		gerr = ((gavg - gsat) * 7) / 16;
		berr = ((bavg - bsat) * 7) / 16;
		block (x - 6, yblktop, 6, sizes[row], 0, &rerr, &gerr, &berr);
		rerr = ((ravg - rsat) * 3) / 16;
		gerr = ((gavg - gsat) * 3) / 16;
		berr = ((bavg - bsat) * 3) / 16;
		block (x + 6, ynexttop, 6, ynextsize, 0, &rerr, &gerr, &berr);
		rerr = ((ravg - rsat) * 5) / 16;
		gerr = ((gavg - gsat) * 5) / 16;
		berr = ((bavg - bsat) * 5) / 16;
		block (x, ynexttop, 6, ynextsize, 0, &rerr, &gerr, &berr);
		rerr = ((ravg - rsat) * 1) / 16;
		gerr = ((gavg - gsat) * 1) / 16;
		berr = ((bavg - bsat) * 1) / 16;
		block (x - 6, ynexttop, 6, ynextsize, 0, &rerr, &gerr, &berr);
	      }

	  l_to_r = !l_to_r;
        }
    }
}

int
do_playback (const char *filename)
{
  FILE *fh;
  unsigned char videoram[40 * 25];
  
  fh = fopen (filename, "r");

  while (!feof (fh))
    {
      int y, x;
      SDL_Event ev;
      
      fscanf (fh, " ---");

      for (y = 0; y < 25; y++)
        for (x = 0; x < 40; x++)
	  {
	    unsigned int byte;
            fscanf (fh, " %2x", &byte);
	    videoram[y * 40 + x] = byte;
	  }

      for (y = 0; y < 25; y++)
        render_row (&videoram[0], y);

      SDL_UpdateRect (screen, 0, 0, screen->w, screen->h);

      usleep (30);

      while (SDL_PollEvent (&ev))
        {
          switch (ev.type)
            {
            case SDL_QUIT:
              exit (0);
            }
        }
    }

  fclose (fh);
  
  return 0;
}

int
main (int argc, char *argv[])
{
  unsigned char videoram[40 * 25];
  struct window win;
  SDL_Event ev;
  int y, arg;
  SDL_Surface *tmp;
  char *infile = NULL, *outfile = NULL, *hexfile = NULL;
  char *playback = NULL;
  int keep_running = 1;
  FILE *hf;

  BuildMode7Font ();

  for (arg = 1; arg < argc; arg++)
    {
      if (strcmp (argv[arg], "-o") == 0)
        outfile = argv[++arg];
      if (strcmp (argv[arg], "-a") == 0)
        hexfile = argv[++arg];
      else if (strcmp (argv[arg], "-x") == 0)
        keep_running = 0;
      else if (strcmp (argv[arg], "-p") == 0)
        playback = argv[++arg];
      else
        infile = argv[arg];
    }
  
  if (!playback)
    {
      if (infile)
	tmp = IMG_Load (infile);
      else
	{
	  fprintf (stderr, "No input file\n");
	  exit (1);
	}

      if (!tmp)
	{
	  fprintf (stderr, "Couldn't load image: %s\n", SDL_GetError ());
	  exit (1);
	}
    }

  if (SDL_Init (SDL_INIT_VIDEO) < 0)
    {
      fprintf (stderr, "Couldn't initialise SDL: %s\n", SDL_GetError ());
      exit (1);
    }
  
  atexit (SDL_Quit);
  
  screen = SDL_SetVideoMode (480, 500, 32, SDL_SWSURFACE);
  if (!screen)
    {
      fprintf (stderr, "Couldn't set video mode: %s\n", SDL_GetError ());
      exit (1);
    }

  /* Convert image into format of display.  */
#if 1
  if (!playback)
    {
    
      image = SDL_DisplayFormat (tmp);
      SDL_FreeSurface (tmp);
    }
#else
  image = tmp;
#endif

  if (!playback)  
    diffuse ();
    // randomize ();
  
  /*SDL_BlitSurface (image, NULL, screen, NULL);*/
  
  VideoState.DataPtr = &videoram[0];
  VideoState.PixmapLine = 0;
  
  win.cols[0] = SDL_MapRGB (screen->format, 0x00, 0x00, 0x00);
  win.cols[1] = SDL_MapRGB (screen->format, 0xff, 0x00, 0x00);
  win.cols[2] = SDL_MapRGB (screen->format, 0x00, 0xff, 0x00);
  win.cols[3] = SDL_MapRGB (screen->format, 0xff, 0xff, 0x00);
  win.cols[4] = SDL_MapRGB (screen->format, 0x00, 0x00, 0xff);
  win.cols[5] = SDL_MapRGB (screen->format, 0xff, 0x00, 0xff);
  win.cols[6] = SDL_MapRGB (screen->format, 0x00, 0xff, 0xff);
  win.cols[7] = SDL_MapRGB (screen->format, 0xff, 0xff, 0xff);
  mainWin = &win;

  if (playback)
    return do_playback (playback);
  
  memset (&videoram[0], ' ', sizeof (videoram));

  for (y = 0; y < 25; y++)
    {
      int rcost, x;
      
      clear_rhs_costs_in_state ();
      
      rcost = select_char (&videoram[0], y, LHS, 0);
      /*printf ("row %d, cost=%d\n", y, rcost);*/

      for (x = 0; x < 40; x++)
        videoram[y * 40 + x] = output[x];

      render_row (&videoram[0], y);

      SDL_UpdateRect (screen, 0, 0, screen->w, screen->h);

      while (SDL_PollEvent (&ev))
        {
          switch (ev.type)
            {
            case SDL_QUIT:
              exit (0);
            }
        }
    }

  //memset (&videoram[0], 'X', sizeof (videoram));

  if (hexfile)
    {
      hf = fopen (hexfile, "a");
      if (!hf)
        {
	  fprintf (stderr, "Can't open '%s' for appending\n", hexfile);
	  exit (1);
	}

      fprintf (hf, "---\n");

      for (y = 0; y < 25; y++)
	{
	  int x;
	  for (x = 0; x < 40; x++)
            fprintf (hf, "%.2x", videoram[y * 40 + x]);
	  fprintf (hf, "\n");
	  render_row (&videoram[0], y);
	}

      fclose (hf);
    }

  if (outfile)
    SDL_SaveBMP (screen, outfile);

  if (!keep_running)
    exit (0);
  
  /* Test pattern:
  for (y = 0; y < 25; y++)
    memcpy (&videoram[y * 40], "0123456789012345678901234567890123456789", 40);
  */
  
  while (1)
    {
      SDL_Delay (100);

      while (SDL_PollEvent (&ev))
        {
          switch (ev.type)
            {
            case SDL_QUIT:
              exit (0);
            }
        }

      SDL_UpdateRect (screen, 0, 0, screen->w, screen->h);
    }
  
  return 0;
}
