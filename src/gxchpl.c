/* Copyright (C) 1988-2022 by George Mason University. See file COPYRIGHT for more information. */

/* Authored by B. Doty */

/* Plot character strings using the Hershey fonts */

#ifdef HAVE_CONFIG_H
#include "config.h"

/* If autoconfed, only include malloc.h when it's presen */
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#else /* undef HAVE_CONFIG_H */

#include <malloc.h>

#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <math.h>
#include "gatypes.h"
#include "gx.h"

/* function prototypes */
gadouble gxchplc (char, gaint, gadouble, gadouble, gadouble, gadouble, gadouble);
gadouble gxchqlc (char, gaint, gadouble);
void gxchplo (char *, int, gadouble, gadouble, gadouble, gadouble, gadouble);
void houtch (char, gaint, gadouble, gadouble, gadouble, gadouble, gadouble);
int iscbyte(char);

/* local variables */
static char *fch[10];     /* Pointers to font data once it is read in */
static gaint *foff[10];   /* Pointers to character offsets */
static gaint *flen[10];   /* Pointers to character lengths */
static gaint dfont;       /* Default font */


/* Initialize */

void gxchii (void) {
gaint i;
  for (i=0; i<10; i++) fch[i] = NULL;
  dfont = 0;
}

/* Change default font */

void gxchdf (gaint df) {
  if (df<0 || df>99) return;
  dfont = df;
}

/* Return default font number */

gaint gxqdf (void) {
  return dfont;
}

/* Plot character string */

void gxchpl (char *chrs, int len, gadouble x, gadouble y, gadouble height, gadouble width, gadouble angle) {
gadouble h,w,xoff,yoff,wact;
gaint fn, supsub, nfn;

char s_utf8[5]; // c-str with up to 4 non-NT chars/bytes
int  bval;
char b[9];
int i;
int cplot; // number of plotted characters in a step for UTF-8

  fn = dfont;
  angle = angle * M_PI/180.0;    /* convert angle from degrees to radians */
  supsub = 0;
  
  while (*chrs!='\0' && len>0) {
    while (*chrs=='`') {
      if (*(chrs+1)>='0' && *(chrs+1)<='9') {    /* get 1-digit font number */
        fn = (gaint)*(chrs+1) - 48;
        chrs+=2;
        len-=2;
      } else if (*(chrs+1)=='a') {    /* superscript */
        supsub = 1;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='b') {    /* subscript */
        supsub = 2;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='n') {    /* normal */
        supsub = 0;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='f') {    /* get 2-digit font number */
        if (len>3) {
          nfn = 10*((gaint)*(chrs+2) - 48) + ((gaint)*(chrs+3) - 48);
          if (nfn>=0 && nfn<100) fn = nfn;
        }
        chrs+=4;
        len-=4;
      } else break;
    }

    if (*chrs!='\0' && len>0) {
      if (supsub) {
        /* adjust size and position for superscripts and subscripts */
        h = height*0.45; w = width*0.45;
        if (supsub==1) yoff = height*0.58;
        else yoff = -0.20*height;
      } else {
        h = height; w = width; yoff = 0.0;
      }
      xoff = yoff*sin(angle);
      yoff = yoff*cos(angle);

      wact = 0.0;
      if (len == 1 || *chrs >= 0) { 
        /* if this char is non-negative, */
        /* it must be ASCII (0-127). */
        /* plot the ASCII character */
        wact = gxdrawch (*chrs, fn, x-xoff, y+yoff, w, h, angle);
        if (wact < -900.0 && fn<6) {
          /* draw instead with Hershey font */
          wact = gxchplc (*chrs, fn, x-xoff, y+yoff, w, h, angle); 
          if (wact < -900.0) return;
        }
        chrs++; len--;
      } else {
        /* try handling UTF-8 */
        /* need the first 5 bits of this potential leader byte. */
        /* convert this (negative) char to int using the signed char equivalent */
        cplot = 0;
        bval = (int)(*chrs) + 256;
        for (i=7; i>=0; i--) { // convert to binary
          b[i] = (bval % 2) ? '1' : '0';  
          bval = bval / 2;
        }

        if (b[0] == '1' && b[1] == '1') {
          /* this looks like a leader byte */
          if (b[2] == '0') {
            /* UTF-8 codepoint encoded to 2 bytes */
            /* verify byte 2 is a continuation byte */
            if (iscbyte(*(chrs+1))) {
              // verified... plot it
              snprintf(s_utf8,5,"%c%c",*(chrs),*(chrs+1));
              wact = gxdrawu8 (s_utf8, '2', fn, x-xoff, y+yoff, w, h, angle);
              cplot = 2;
            }
          } else if (b[3] == '0' && len > 2) {
            /* encoded to 3 bytes */
            /* verify bytes 2 & 3 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2))) {
              // verified... plot it
              snprintf(s_utf8,5,"%c%c%c",*(chrs),*(chrs+1),*(chrs+2));
              wact = gxdrawu8 (s_utf8, '3', fn, x-xoff, y+yoff, w, h, angle);
              cplot = 3;  
            }
          } else if (b[4] == '0' && len > 3) {
            /* encoded to 4 bytes */
            /* verify bytes 2-4 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2)) && iscbyte(*(chrs+3))) {
              // verified... plot it
              snprintf(s_utf8,5,"%c%c%c%c",*(chrs),*(chrs+1),*(chrs+2),*(chrs+3));
              wact = gxdrawu8 (s_utf8, '4', fn, x-xoff, y+yoff, w, h, angle);
              cplot = 4;
            }
          }
        } 

        if (cplot==0) {
          /* apparently nothing was plotted, */
          /* so it seems there was a problem detecting the UTF-8 symbol. */
          /* treat this char as ASCII. */
          wact = gxdrawch (*chrs, fn, x-xoff, y+yoff, w, h, angle);
          if (wact < -900.0 && fn<6) {
            /* draw instead with Hershey font */
            wact = gxchplc (*chrs, fn, x-xoff, y+yoff, w, h, angle); 
            if (wact < -900.0) return;
          }
          chrs++; len--;
        } else {
          /* a UTF-8 symbol was apparently plotted. */
          /* update stuff... */
          chrs += cplot;
          len  -= cplot;
        }
      }

      x = x + wact*cos(angle);
      y = y + wact*sin(angle);

    }

  }
}

/* Plot vertical character string  */
/* i.e.,  L 
 *        I
 *        K
 *        E
 *
 *        T
 *        H
 *        I
 *        S
 */
void gxchplv (char *chrs, int len, gadouble x, gadouble y, gadouble height, gadouble width, gadouble angle) {
gadouble h,w,xoff,yoff,wact;
gaint fn, supsub, nfn;

char s_utf8[5]; // c-str with up to 4 non-NT chars/bytes
int  bval;
char b[9];
int i;
int cplot; // number of plotted characters in a step for UTF-8
  
  // super/subscripts might be suspect for vert typing.
  fn = dfont;
  angle = angle * M_PI/180.0;    /* convert angle from degrees to radians */
  supsub = 0;
  while (*chrs!='\0' && len>0) {
    while (*chrs=='`') {
      if (*(chrs+1)>='0' && *(chrs+1)<='9') {    /* get 1-digit font number */
        fn = (gaint)*(chrs+1) - 48;
        chrs+=2;
        len-=2;
      } else if (*(chrs+1)=='a') {    /* superscript */
        supsub = 1;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='b') {    /* subscript */
        supsub = 2;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='n') {    /* normal */
        supsub = 0;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='f') {    /* get 2-digit font number */
        if (len>3) {
          nfn = 10*((gaint)*(chrs+2) - 48) + ((gaint)*(chrs+3) - 48);
          if (nfn>=0 && nfn<100) fn = nfn;
        }
        chrs+=4;
        len-=4;
      } else break;
    }

    if (*chrs!='\0' && len>0) {
      // related to super/subscripts. Don't worry about this for now. 
      if (supsub) {
        /* adjust size and position for superscripts and subscripts */
        h = height*0.45; w = width*0.45;
        if (supsub==1) yoff = height*0.58;
        else yoff = -0.20*height;
      } else {
        h = height; w = width; yoff = 0.0;
      }
      xoff = yoff*sin(angle);
      yoff = yoff*cos(angle);


      // this wact variable is important. For horizontal lines, it is the
      // width of the symbol drawn. Here, we need the height of the symbol
      // drawn for vertical lines.
      wact = 0.0;
      if (len == 1 || *chrs >= 0) { 
        /* if this char is non-negative, */
        /* it must be ASCII (0-127). */
        /* plot the ASCII character */
        wact = gxdrawchv (*chrs, fn, x-xoff, y+yoff, w, h, angle);
        if (wact < -900.0 && fn<6) {
          /* draw instead with Hershey font */
          /* for now we will use the width to represent height */
          /* very wrong for some chars but not important for now */
          wact = gxchplc (*chrs, fn, x-xoff, y+yoff, w, h, angle); 
          if (wact < -900.0) return;
        }
        chrs++; len--;
      } else {
        /* try handling UTF-8 */
        /* need the first 5 bits of this potential leader byte. */
        /* convert this (negative) char to int using the signed char equivalent */
        cplot = 0;
        bval = (int)(*chrs) + 256;
        for (i=7; i>=0; i--) { // convert to binary
          b[i] = (bval % 2) ? '1' : '0';  
          bval = bval / 2;
        }

        if (b[0] == '1' && b[1] == '1') {
          /* this looks like a leader byte. */
          if (b[2] == '0') {
            /* UTF-8 codepoint encoded to 2 bytes */
            /* verify byte 2 is a continuation byte */
            if (iscbyte(*(chrs+1))) {
              // verified... plot it
              snprintf(s_utf8,5,"%c%c",*(chrs),*(chrs+1));
              wact = gxdrawu8v (s_utf8, '2', fn, x-xoff, y+yoff, w, h, angle);
              cplot = 2;
            }
          } else if (b[3] == '0' && len > 2) {
            /* encoded to 3 bytes */
            /* verify bytes 2 & 3 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2))) {
              // verified... plot it
              snprintf(s_utf8,5,"%c%c%c",*(chrs),*(chrs+1),*(chrs+2));
              wact = gxdrawu8v (s_utf8, '3', fn, x-xoff, y+yoff, w, h, angle);
              cplot = 3;  
            }
          } else if (b[4] == '0' && len > 3) {
            /* encoded to 4 bytes */
            /* verify bytes 2-4 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2)) && iscbyte(*(chrs+3))) {
              // verified... plot it
              snprintf(s_utf8,5,"%c%c%c%c",*(chrs),*(chrs+1),*(chrs+2),*(chrs+3));
              wact = gxdrawu8v (s_utf8, '4', fn, x-xoff, y+yoff, w, h, angle);
              cplot = 4;
            }
          }
        } 

        if (cplot==0) {
          /* apparently nothing was plotted, */
          /* so it seems there was a problem detecting the UTF-8 symbol. */
          /* treat this char as ASCII. */
          wact = gxdrawchv (*chrs, fn, x-xoff, y+yoff, w, h, angle);
          if (wact < -900.0 && fn<6) {
            /* draw instead with Hershey font */
            wact = gxchplc (*chrs, fn, x-xoff, y+yoff, w, h, angle); 
            if (wact < -900.0) return;
          }
          chrs++; len--;
        } else {
          /* a UTF-8 symbol was apparently plotted. */
          /* update stuff... */
          chrs += cplot;
          len  -= cplot;
        }
      }

      // update plotting position based on symbol width (problems)
      x = x + wact*sin(angle);
      y = y - wact*cos(angle);
    }

  }
}

int iscbyte(char ch) {
  int bval, i;
  char b[9];

  bval = (int)(ch) + 256;
  for (i=7; i>=0; i--) {
    b[i] = (bval % 2) ? '1' : '0';  
    bval = bval / 2;
  }
  
  return (b[0] == '1' && b[1] == '0') ? 1 : 0;
}

/* Get actual width of a single character in the indicated Hershey font */

gadouble gxchqlc (char ccc, gaint fn, gadouble width) {
gadouble xs,w;
gaint cnt,ic,jc;
char *cdat;

  xs = width/21.0;
  cdat = gxchgc ( (gaint)ccc, fn, &cnt);
  if (cdat==NULL) return (-999.9);
  ic = (gaint)(*(cdat+3)) - 82;
  jc = (gaint)(*(cdat+4)) - 82;
  w = (gadouble)(jc-ic) * xs * 1.05;
  return (w);
}

/* plot a single char in the indicated hershey font with the indicated location, size, and angle, 
   and return the distance to advance after plotting */

gadouble gxchplc (char ccc, gaint fn, gadouble x, gadouble y, gadouble width, gadouble height, gadouble angle) {
gadouble xs,ys,w,d,xc,yc,ang,rx,ry;
gaint i,ic,jc,cnt,ipen;
char *cdat;

  xs = width/21.0;
  ys = height/22.0;
  cdat = gxchgc ( (gaint)ccc, fn, &cnt);
  if (cdat==NULL) return (-999.9);
  ic = (gaint)(*(cdat+3)) - 82;
  jc = (gaint)(*(cdat+4)) - 82;
  w = (gadouble)(jc-ic) * xs * 1.05;
  d = hypot(w/2.0,height*0.42);
  ang = atan2(height*0.42,w/2.0)+angle;
  xc = x + d*cos(ang);
  yc = y + d*sin(ang);
  cdat += 5;
  ipen = 3;
  for (i=1; i<cnt; i++) {
    ic = (gaint)*cdat;
    jc = (gaint)*(cdat+1);
    if (ic==32) ipen = 3;
    else {
      ic = ic - 82;
      jc = jc - 82;
      rx = ((gadouble)ic)*xs;
      ry = -1.0*((gadouble)jc)*ys;
      if (rx==0.0 && ry==0.0) {
        d = 0.0; ang = 0.0;
      } else {
        d = hypot(rx,ry);
        ang = atan2(ry,rx)+angle;
      }
      rx = xc + d*cos(ang);
      ry = yc + d*sin(ang);
      if (ipen==3) gxmove (rx,ry);
      else gxdraw (rx,ry);
      ipen = 2;
    }
    cdat += 2;
  }
  return(w);
}

/* Determine the length of a character string without plotting it. */
gaint gxchln (char *chrs, gaint len, gadouble width, gadouble *wret) {
gadouble w,wact,cw;
gaint fn, supsub, nfn;
char s_utf8[5]; // c-str with up to 4 non-NT chars/bytes
int  bval;
char b[9];
int i;
int cplot; // number of plotted characters in a step for UTF-8

  fn = dfont;
  supsub = 0;
  cw = 0;
  while (*chrs!='\0' && len>0) {
    while (*chrs=='`') {
      if (*(chrs+1)>='0' && *(chrs+1)<='9') {
        fn = (gaint)*(chrs+1) - 48;
        chrs+=2;
        len-=2;
      } else if (*(chrs+1)=='a') {
        supsub = 1;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='b') {
        supsub = 2;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='n') {
        supsub = 0;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='f') {
        if (len>3) {
          nfn = 10*((gaint)*(chrs+2) - 48) + ((gaint)*(chrs+3) - 48);
          if (nfn>=0 && nfn<100) fn = nfn;
        }
        chrs+=4;
        len-=4;
      } else break;
    }
    if (*chrs!='\0' && len>0) {
      if (supsub) {
        w = width*0.45;
      } else {
        w = width;
      }

      /* First see if the rendering back end wants to plot this, or punt.
         If it wants to punt, we get a -999 back, so we use Hershey instead  */
      wact = 0.0;
      if (len == 1 || *chrs >= 0) { 
        wact = gxqchl (*chrs, fn, w);
        if (wact < -900.0) {
	        wact = gxchqlc (*chrs, fn, w); 
        }
        chrs++; len--;
      } else {
        /* try handling UTF-8 */
        /* need the first 5 bits of this potential leader byte. */
        /* convert this (negative) char to int using the signed char equivalent */
        cplot = 0;
        bval = (int)(*chrs) + 256;
        for (i=7; i>=0; i--) { // convert to binary
          b[i] = (bval % 2) ? '1' : '0';  
          bval = bval / 2;
        }

        if (b[0] == '1' && b[1] == '1') {
          /* this looks like a leader byte. */
          if (b[2] == '0') {
            /* UTF-8 codepoint encoded to 2 bytes */
            /* verify byte 2 is a continuation byte */
            if (iscbyte(*(chrs+1))) {
              snprintf(s_utf8,5,"%c%c",*(chrs),*(chrs+1));
              wact = gxqu8l (s_utf8, fn, w);
              cplot = 2;
            }
          } else if (b[3] == '0' && len > 2) {
            /* encoded to 3 bytes */
            /* verify bytes 2 & 3 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2))) {
              snprintf(s_utf8,5,"%c%c%c",*(chrs),*(chrs+1),*(chrs+2));
              wact = gxqu8l (s_utf8, fn, w);
              cplot = 3;
            }
          } else if (b[4] == '0' && len > 3) {
            /* encoded to 4 bytes */
            /* verify bytes 2-4 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2)) && iscbyte(*(chrs+3))) {
              snprintf(s_utf8,5,"%c%c%c%c",*(chrs),*(chrs+1),*(chrs+2),*(chrs+3));
              wact = gxqu8l (s_utf8, fn, w);
              cplot = 4;
            }
          }
        } 

        if (cplot==0) {
          /* apparently nothing was decoded, */
          /* so it seems there was a problem detecting the UTF-8 symbol. */
          /* treat this char as ASCII. */
          wact = gxqchl (*chrs, fn, w);
          if (wact < -900.0 && fn<6) {
            // hershey
            wact = gxchqlc (*chrs, fn, w); 
          }
          chrs++; len--;
        } else {
          /* a UTF-8 symbol was apparently plotted. */
          /* update stuff... */
          chrs += cplot;
          len  -= cplot;
        }
      }
      cw = cw + wact;
    }
  }
  *wret = cw;
  return (cw);
}

/* Determine the height of a vertical character string without plotting it */
gaint gxchlnv (char *chrs, gaint len, gadouble height, gadouble *wret) {
gadouble w,wact,cw;
gaint fn, supsub, nfn;
char s_utf8[5]; // c-str with up to 4 non-NT chars/bytes
int  bval;
char b[9];
int i;
int cplot; // number of plotted characters in a step for UTF-8

  fn = dfont;
  supsub = 0;
  cw = 0;
  while (*chrs!='\0' && len>0) {
    while (*chrs=='`') {
      if (*(chrs+1)>='0' && *(chrs+1)<='9') {
        fn = (gaint)*(chrs+1) - 48;
        chrs+=2;
        len-=2;
      } else if (*(chrs+1)=='a') {
        supsub = 1;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='b') {
        supsub = 2;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='n') {
        supsub = 0;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='f') {
        if (len>3) {
          nfn = 10*((gaint)*(chrs+2) - 48) + ((gaint)*(chrs+3) - 48);
          if (nfn>=0 && nfn<100) fn = nfn;
        }
        chrs+=4;
        len-=4;
      } else break;
    }

    if (*chrs!='\0' && len>0) {
      if (supsub) {
        w = height*0.45;
      } else {
        w = height;
      }

      /* First see if the rendering back end wants to plot this, or punt.
         If it wants to punt, we get a -999 back, so we use Hershey instead  */
      wact = 0.0;
      if (len == 1 || *chrs >= 0) { 
        wact = gxqchh (*chrs, fn, w);
        if (wact < -900.0) {
	        wact = gxchqlc (*chrs, fn, w); 
        }
        chrs++; len--;
      } else {
        /* try handling UTF-8 */
        /* need the first 5 bits of this potential leader byte. */
        /* convert this (negative) char to int using the signed char equivalent */
        cplot = 0;
        bval = (int)(*chrs) + 256;
        for (i=7; i>=0; i--) { // convert to binary
          b[i] = (bval % 2) ? '1' : '0';  
          bval = bval / 2;
        }

        if (b[0] == '1' && b[1] == '1') {
          /* this looks like a leader byte. */
          if (b[2] == '0') {
            /* UTF-8 codepoint encoded to 2 bytes */
            /* verify byte 2 is a continuation byte */
            if (iscbyte(*(chrs+1))) {
              snprintf(s_utf8,5,"%c%c",*(chrs),*(chrs+1));
              wact = gxqu8h (s_utf8, fn, w);
              cplot = 2;
            }
          } else if (b[3] == '0' && len > 2) {
            /* encoded to 3 bytes */
            /* verify bytes 2 & 3 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2))) {
              snprintf(s_utf8,5,"%c%c%c",*(chrs),*(chrs+1),*(chrs+2));
              wact = gxqu8h (s_utf8, fn, w);
              cplot = 3;
            }
          } else if (b[4] == '0' && len > 3) {
            /* encoded to 4 bytes */
            /* verify bytes 2-4 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2)) && iscbyte(*(chrs+3))) {
              snprintf(s_utf8,5,"%c%c%c%c",*(chrs),*(chrs+1),*(chrs+2),*(chrs+3));
              wact = gxqu8h (s_utf8, fn, w);
              cplot = 4;
            }
          }
        } 

        if (cplot==0) {
          /* apparently nothing was decoded, */
          /* so it seems there was a problem detecting the UTF-8 symbol. */
          /* treat this char as ASCII. */
          wact = gxqchh (*chrs, fn, w);
          if (wact < -900.0 && fn<6) {
            // hershey
            wact = gxchqlc (*chrs, fn, w); 
          }
          chrs++; len--;
        } else {
          /* a UTF-8 symbol was apparently plotted. */
          /* update stuff... */
          chrs += cplot;
          len  -= cplot;
        }
      }
      cw = cw + wact;
    }
  }
  *wret = cw;
  return (cw);
}

/* Determine the width of a vertical character string without plotting it. */
/* basically, we just check the width of every symbol in the string, */
/* and the maximum width found is the width of the string object. */
gaint gxchlnvw (char *chrs, gaint len, gadouble width, gadouble *wret) {
gadouble w,wact,cw;
gaint fn, supsub, nfn;
char s_utf8[5]; // c-str with up to 4 non-NT chars/bytes
int  bval;
char b[9];
int i;
int cplot; // number of plotted characters in a step for UTF-8

  fn = dfont;
  supsub = 0;
  cw = 0.0;
  while (*chrs!='\0' && len>0) {
    while (*chrs=='`') {
      if (*(chrs+1)>='0' && *(chrs+1)<='9') {
        fn = (gaint)*(chrs+1) - 48;
        chrs+=2;
        len-=2;
      } else if (*(chrs+1)=='a') {
        supsub = 1;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='b') {
        supsub = 2;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='n') {
        supsub = 0;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='f') {
        if (len>3) {
          nfn = 10*((gaint)*(chrs+2) - 48) + ((gaint)*(chrs+3) - 48);
          if (nfn>=0 && nfn<100) fn = nfn;
        }
        chrs+=4;
        len-=4;
      } else break;
    }
    if (*chrs!='\0' && len>0) {
      if (supsub) {
        w = width*0.45;
      } else {
        w = width;
      }

      /* First see if the rendering back end wants to plot this, or punt.
         If it wants to punt, we get a -999 back, so we use Hershey instead  */
      wact = 0.0;
      if (len == 1 || *chrs >= 0) { 
        wact = gxqchl (*chrs, fn, w);
        if (wact < -900.0) {
	        wact = gxchqlc (*chrs, fn, w); 
        }
        chrs++; len--;
      } else {
        /* try handling UTF-8 */
        /* need the first 5 bits of this potential leader byte. */
        /* convert this (negative) char to int using the signed char equivalent */
        cplot = 0;
        bval = (int)(*chrs) + 256;
        for (i=7; i>=0; i--) { // convert to binary
          b[i] = (bval % 2) ? '1' : '0';  
          bval = bval / 2;
        }

        if (b[0] == '1' && b[1] == '1') {
          /* this looks like a leader byte. */
          if (b[2] == '0') {
            /* UTF-8 codepoint encoded to 2 bytes */
            /* verify byte 2 is a continuation byte */
            if (iscbyte(*(chrs+1))) {
              snprintf(s_utf8,5,"%c%c",*(chrs),*(chrs+1));
              wact = gxqu8l (s_utf8, fn, w);
              cplot = 2;
            }
          } else if (b[3] == '0' && len > 2) {
            /* encoded to 3 bytes */
            /* verify bytes 2 & 3 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2))) {
              snprintf(s_utf8,5,"%c%c%c",*(chrs),*(chrs+1),*(chrs+2));
              wact = gxqu8l (s_utf8, fn, w);
              cplot = 3;
            }
          } else if (b[4] == '0' && len > 3) {
            /* encoded to 4 bytes */
            /* verify bytes 2-4 are continuation bytes */
            if (iscbyte(*(chrs+1)) && iscbyte(*(chrs+2)) && iscbyte(*(chrs+3))) {
              snprintf(s_utf8,5,"%c%c%c%c",*(chrs),*(chrs+1),*(chrs+2),*(chrs+3));
              wact = gxqu8l (s_utf8, fn, w);
              cplot = 4;
            }
          }
        } 

        if (cplot==0) {
          /* apparently nothing was decoded, */
          /* so it seems there was a problem detecting the UTF-8 symbol. */
          /* treat this char as ASCII. */
          wact = gxqchl (*chrs, fn, w);
          if (wact < -900.0 && fn<6) {
            // hershey
            wact = gxchqlc (*chrs, fn, w); 
          }
          chrs++; len--;
        } else {
          /* a UTF-8 symbol was apparently plotted. */
          /* update stuff... */
          chrs += cplot;
          len  -= cplot;
        }
      }
      if (wact > cw) cw = wact;
    }
  }
  *wret = cw;
  return (cw);
}


/* Get location and length of particular character info
   for particular font */

char * gxchgc (int ch, int fn, int *cnt) {
int *clen, *coff, rc;
char *fdat;

  if (fch[fn]==NULL) {
    rc = gxchrd (fn);
    if (rc) return(NULL);
  }
  clen = flen[fn];
  coff = foff[fn];
  fdat = fch[fn];
  if (ch<32 || ch>127) ch=32;
  ch = ch - 32;
  *cnt = *(clen+ch);
  return (fdat + *(coff+ch));
}

/* Read in a font file */

int gxchrd (int fn) {
FILE *ifile;
gaint i,j,rc,tlen,*coff,*clen,flag;
char buff[20],*fname,*fdat;

  snprintf(buff,19,"font%i.dat",fn);

  ifile = NULL;
  fname = gxgnam(buff);
  if (fname!=NULL) ifile = fopen(fname,"rb");
  if (ifile==NULL) {
    ifile = fopen(buff,"rb");
    if (ifile==NULL) {
      printf ("Error opening stroke character data set \n");
      if (fname!=NULL) {
        printf ("  Data set names = %s ; %s\n",fname,buff);
        free (fname);
      }
      return(1);
    }
  }
  free(fname);

  fseek(ifile,0L,2);
  tlen = ftell(ifile);
  fseek(ifile,0L,0);

  fdat = (char *)malloc(tlen+1);
  if (fdat==NULL) {
    printf ("Error reading font data:  Memory allocation error\n");
    return(1);
  }
  coff = (gaint *)malloc(sizeof(gaint)*95);
  if (coff==NULL) {
    printf ("Error reading font data:  Memory allocation error\n");
    free (fdat);
    return(1);
  }
  clen = (gaint *)malloc(sizeof(gaint)*95);
  if (clen==NULL) {
    printf ("Error reading font data:  Memory allocation error\n");
    free (fdat);
    return(1);
  }

  rc = fread(fdat,1,tlen,ifile);
  if (rc!=tlen) {
    printf ("Error reading font data: I/O Error\n");
    return(1);
  }
  *(fdat+tlen) = '\0';

  /* Determine the locations of the start of each character */

  i = 0;
  j = 1;
  *(coff) = 0;
  flag = 0;
  while (*(fdat+i)) {
    if (*(fdat+i)<' ') {
      flag = 1;
    } else {
      if (flag) {
        *(coff+j) = i;
        j++;
      }
      flag = 0;
    }
    i++;
  }

  /* Determine the count on each character */

  for (i=0; i<95; i++) {
    for (j=0; j<3; j++) buff[j] = *(fdat+*(coff+i)+j);
    buff[3] = '\0';
    sscanf (buff,"%i",&rc);
    *(clen+i) = rc;
  }

  flen[fn] = clen;
  foff[fn] = coff;
  fch[fn] = fdat;

  return(0);
}

void gxchplo (char *chrs, int len, gadouble x, gadouble y, gadouble height, gadouble width, gadouble angle) {
gadouble xc,yc,xscl,yscl,xs,ys,w,d,ang,rx,ry,yoff;
gaint i,fn,ic,jc,cnt,ipen,supsub;
char *cdat;

  xscl = width/21.0;
  yscl = height/22.0;
  fn = dfont;
  angle = angle * 3.1416/180.0;
  supsub = 0;
  while (*chrs!='\0' && len>0) {
    while (*chrs=='`') {
      if (*(chrs+1)>='0' && *(chrs+1)<='9') {
        fn = (gaint)*(chrs+1) - 48;
        chrs+=2;
        len-=2;
      } else if (*(chrs+1)=='a') {
        supsub = 1;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='b') {
        supsub = 2;
        chrs+=2; len-=2;
      } else if (*(chrs+1)=='n') {
        supsub = 0;
        chrs+=2; len-=2;
      } else break;
    }
    if (*chrs!='\0' && len>0) {
      if (angle==0.0) {           /* Fast path for ang=0 */
        cdat = gxchgc ( (gaint)*(chrs), fn, &cnt);
        if (cdat==NULL) return;
        ic = (gaint)(*(cdat+3)) - 82;
        jc = (gaint)(*(cdat+4)) - 82;
        if (supsub) {
          xs = xscl*0.45; ys = yscl*0.45;
          if (supsub==1) yoff = height*0.35;
          else yoff = -1.0*height*0.42;
        } else {
          xs = xscl; ys = yscl; yoff = 0.0;
        }
        w = (gadouble)(jc-ic) * xs * 1.05;
        xc= x + w/2.0;
        yc = y + height*0.42 + yoff;
        cdat += 5;
        ipen = 3;
        for (i=1; i<cnt; i++) {
          ic = (gaint)*cdat;
          jc = (gaint)*(cdat+1);
          if (ic==32) ipen = 3;
          else {
            ic = ic - 82;
            jc = jc - 82;
            rx = xc + ((gadouble)ic)*xs;
            ry = yc - ((gadouble)jc)*ys;
            if (ipen==3) gxmove (rx,ry);
            else gxdraw (rx,ry);
            ipen = 2;
          }
          cdat += 2;
        }
        x = x + w;
        chrs++; len--;
      } else {
        cdat = gxchgc ( (gaint)*(chrs), fn, &cnt);
        if (cdat==NULL) return;
        ic = (gaint)(*(cdat+3)) - 82;
        jc = (gaint)(*(cdat+4)) - 82;
        if (supsub) {
          xs = xscl*0.45; ys = yscl*0.45;
          if (supsub==1) yoff = height*0.35;
          else yoff = -1.0*height*0.42;
        } else {
          xs = xscl; ys = yscl; yoff = 0.0;
        }
        w = (gadouble)(jc-ic) * xs * 1.05;
        d = hypot(w/2.0,height*0.42+yoff);
        ang = atan2(height*0.42+yoff,w/2.0)+angle;
        xc = x + d*cos(ang);
        yc = y + d*sin(ang);
        cdat += 5;
        ipen = 3;
        for (i=1; i<cnt; i++) {
          ic = (gaint)*cdat;
          jc = (gaint)*(cdat+1);
          if (ic==32) ipen = 3;
          else {
            ic = ic - 82;
            jc = jc - 82;
            rx = ((gadouble)ic)*xs;
            ry = -1.0*((gadouble)jc)*ys;
            if (rx==0.0 && ry==0.0) {
              d = 0.0; ang = 0.0;
            } else {
              d = hypot(rx,ry);
              ang = atan2(ry,rx)+angle;
            }
            rx = xc + d*cos(ang);
            ry = yc + d*sin(ang);
            if (ipen==3) gxmove (rx,ry);
            else gxdraw (rx,ry);
            ipen = 2;
          }
          cdat += 2;
        }
        x = x + w*cos(angle);
        y = y + w*sin(angle);
        chrs++; len--;
      }
    }
  }
}
