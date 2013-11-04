#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#define true 1
#define false 0
#undef DEBUG

typedef int bool;

static unsigned char delim1[] = "<<<";
static unsigned char delim2[] = ">>>";

static char *progname;

static void error(char *s) {
     fprintf(stderr, "%s: %s\n",progname,s);
     exit(1);
     }

static void usage() {
     fprintf(stderr,"usage: %s filename.tex filename.out > filename-m2.tex\n",
	  progname);
     exit(1);
     }

static bool neednewline = false;

static int LINEWIDTH = 72;

static char *translateTable[256];

static char *getmem(unsigned int n) {
     char *p = malloc(n);
     if (p == NULL) error ("out of memory");
     return p;
     }

static void setup() {
     int i;
     char *special = "\\%";	/* special characters */
     char *special2 = " ${}";	/* special characters X for which \X is defined */
     char *p;
     for (i = 0; i< 256; i++) {
	  p = getmem(11);
	  sprintf(p,"{\\char%d}", i);
	  translateTable[i] = p;
	  }
     for (i = 32; i < 127; i++) {
	  p = getmem(2);
	  sprintf(p,"%c", i);
	  translateTable[i] = p;
	  }
     for (; *special; special++) {
	  p = getmem(11);
	  sprintf(p,"{\\char`\\%c}", *special);
	  translateTable[(int)*special] = p;
	  }
     for (; *special2; special2++) {
	  p = getmem(3);
	  sprintf(p,"\\%c", *special2);
	  translateTable[(int)*special2] = p;
	  }
     translateTable['\n'] = "\\\\\n";
     translateTable['\r'] = "\r";
     translateTable['\t'] = "\t";
     translateTable[' '] = "\\ ";
     translateTable['_'] = "\\_";
     translateTable['^'] = "\\^";
     }

static char *translate(int c) {
  static int column = 0;
  char *r = translateTable[(int)c];
  if (c == '\n') column = 0; else column++;
  return r;
}

static unsigned char delay_buf[4];
static int  delay_n = 0;
static int delay_putc(unsigned char c, FILE *o) { /* only works with one FILE at a time */
  int ret = 0;
  if (delay_n == sizeof delay_buf) {
    ret = fputs(translate((int)delay_buf[0]),o);
    memmove(delay_buf,delay_buf+1,sizeof delay_buf - 1);
    delay_n --;
  }
  delay_buf[delay_n++] = c;
  return ret;
}

static void delay_clear() {
  delay_n = 0;
}

static int delay_flush(FILE *o) {
  int r = 0;
  int i;
  for (i=0; i<delay_n; i++) r |= fputs(translate((int)delay_buf[i]),o);
  delay_n = 0;
  return r;
}

static int passverbatim(unsigned char *s, FILE *f, FILE *o) {
     unsigned char *p;
     int c;
     int column = 0;
     fputs("\\beginOutput\n",stdout);
     for (p=s; *p;) {
	  c = getc(f);
	  if (c == EOF) break;
	  if (*p == c) {
	       p++;
	       }
	  else {
	       unsigned char *q;
	       for (q=s; q<p; q++) {
		 delay_putc(*q,o);
		 column++;
	       }
	       if (c == '\n') {
		    if (column == 0) {
		         delay_flush(stdout);
			 fputs("\\emptyLine\n",stdout);
			 }
		    else {
		         delay_putc(c,stdout);
		         }
		    column = 0;
		    }
	       else {
		    if (column == LINEWIDTH) {
		         delay_clear();
			 fputs(" $\\cdot\\cdot\\cdot$",stdout);	/* ... */
			 column += 4;
			 }
		    else if (column < LINEWIDTH) {
			 delay_putc(c,stdout);
			 column++;
			 }
		    }
	       p = s;
	       }
	  }
     delay_flush(stdout);
     fputs("\\endOutput",stdout);
     neednewline = true;
     return true;
     }

void trap(){
     }

int pass(unsigned char *s, FILE *f, FILE *o) {
     unsigned char *p;
     int c;
     # ifdef DEBUG
     fprintf(stderr, "looking for '%s'\n", s);
     # endif
     for (p=s; *p;) {
	  c = getc(f);
	  if (c == EOF) return EOF;
	  if (*p == c) {
	       p++;
	       }
	  else {
	       if (o != NULL) {
	       	    unsigned char *q;
	       	    for (q=s; q<p; q++) {
			 putc(*q,o);
			 # ifdef DEBUG
			 fprintf(stderr, "using '%c'\n", *q);
			 # endif
			 }
		    if (neednewline) {
			 if (c != '\n') putc('\n',o);
		    	 neednewline = false;
			 }
	       	    putc(c,o);
		    }
	       # ifdef DEBUG
	       fprintf(stderr, "passing '%c'\n", c);
	       # endif
	       p = s;
	       }
	  }
     # ifdef DEBUG
     fprintf(stderr, "found '%s'\n", s);
     # endif
     return true;
     }

int main(int argc, char **argv) {
     FILE *TeXinfile = NULL;
     FILE *infile = NULL;
     progname = argv[0];
     setup();
     if (0 == strcmp(argv[1],"-w") && argc >= 3) {
       LINEWIDTH = atoi(argv[2]);
       argv += 2;
       argc -= 2;
     }
     if (argc != 3) usage();
     TeXinfile = fopen(argv[1],"r");
     if (TeXinfile == NULL) {
	  char buf[100];
	  sprintf(buf, "%s: couldn't open file %s for reading",progname,argv[1]);
	  perror(buf);
	  exit(1);
	  }
     infile = fopen(argv[2],"r");
     if (infile == NULL) {
	  char buf[100];
	  sprintf(buf, "%s: couldn't open file %s for reading",progname,argv[2]);
	  perror(buf);
	  exit(1);
	  }
     fputs("\\input merge.tex\n",stdout);
     pass((unsigned char *)"\371",infile,NULL);
     while (true) {
     	  int c = pass(delim1,TeXinfile,stdout);
	  if (c == EOF) exit(0);
	  c = passverbatim(delim2,TeXinfile,NULL);
	  if (c == EOF) {
	       error("end of file reached within program input");
	       exit(1);
	       }
	  passverbatim((unsigned char *)"Coq < ",infile,stdout);
	  pass((unsigned char *)"\371",infile,NULL);
	  neednewline = true;
	  }
     }

	  
