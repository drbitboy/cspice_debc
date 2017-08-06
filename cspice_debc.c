/***********************************************************************
 * cspice_debc.c
 *
 *   Remove bounds checking from one F2Ced CSPICE *[^_]?.c routine
 *
 * Usage (BASH):
 *
 *   ./cspice_debc src/cspice/dskx02.c ./dskx02_debc.c
 *

### CHDIR to CSPICE top level directory
cd [.../]cspice

### Move original CSPICE source (src/) to base_src/
mv src base_src

### Duplicate original src/ sub-directory
rsync -a[v] base_src/ src/ --exclude='*_c.c'

### DE-Bounds-Check CSPICE source from base_src/cspice/ to src/cspice/
for C in base_src/cspice/*[^_]?.c ; do
  ./cspice_debc "${C} | ./cspice_debc | ./cspice_debc - ${C#base_}
done

### Rebuild CSPICE libraries
./makeall.csh

 **********************************************************************/
#include <stdio.h>
#include <string.h>

typedef enum
{ WANTfirstOpenParenOrBracket = 0
, WANTfirstOpenParen
, WANTi
, WANT_first
, WANT_second
, WANTnum
, WANTspaceAfterNum
, WANTfirstEqual
, WANTspaceAfterEqual
, WANTnonspaceAfterSpace  /* set pStartIndex here */
, WANTfirstCloseParen     /* set pStopIndex here */
, WANTspaceAfterCloseParen
, WANTltAfterSpace
, WANTspaceAfterLt
, WANTanotherSpaceAfterLt
, WANTquery
, WANTspaceAfterquery
, WANTanotherSpaceAfterQuery
, WANTcolon
, WANTspaceAfterColon
, WANTsAfterSpace
, WANT_Afters
, WANTrAfter_
, WANTnAfterr
, WANTgAftern
, WANTeAfterg
, WANTopenParenAftere
, WANTcloseParenAftere

/* After newline */
, WANTtabOrFirstSpace
, WANTsecondSpace
, WANTthirdSpace
, WANTfourthSpace
} ENUM_WANT;

#define copenparen '('
#define ccloseparen ')'
#define copenbracket '['
#define cclosebracket ']'
#define ci 'i'
#define c_ '_'
#define c0 '0'
#define c1 '1'
#define c9 '9'
#define cspace ' '
#define cequal '='
#define clt '<'
#define cquery '?'
#define ccolon ':'
#define cs 's'
#define cr 'r'
#define cn 'n'
#define cg 'g'
#define ce 'e'
#define csemi ';'

#define ctick '\''
#define cbackslash '\\'
#define cnl '\n'
#define ctab '\t'

int main(int argc, char** argv) {

/* Input and output filenames; "-" is stdin or stdout */
char* fnIn = { "-" };
char* fnOut = { "-" };

/* Input and output FILE*s */
FILE* fIn = stdin;
FILE* fOut = stdout;

/* Input buffer to store characters from input FILE* */
char inBuffer[2048];

/* Pointer to most recently read character in input buffer */
/* Value of the most recently read character */
char* pCurrentChar;
int currentChar;

/* Pointer to where to place the next character read in */
char* pNextChar;

/* Pointers to start and stop in input buffer of calculated index */
char* pStartIndex;
char* pStopIndex;

/* Pointers to use input buffer storage for input */
char* pStartBackup;
char* pStopBackup;

/* Keep track of depth of parentheses () and brackets [] */
int parenDepth;
int bracketDepth;

/* Saved depths at start of parsing */
int savedParenDepth;
int savedBracketDepth;

/* Single character stack; highest priority alternate input */
int pushedChar;
int usePushedChar;

/* State storage */

/* - Two states enum:  bounds check; newline handling */
typedef enum
{ STATE_BOUNDSCHECK_SEQ = 0
, STATE_NEWLINE_SEQ
, STATE_COUNT_SEQ
} STATE_SEQ;

/* - Two state variables */
ENUM_WANT state[STATE_COUNT_SEQ];

/* - Which state variable is currently active */
STATE_SEQ stateDepth;


  /* Open input file for read, and output for write */
  if (argc > 1) fnIn = argv[1];
  if (argc > 2) fnOut = argv[2];

  if (strcmp("-",fnIn)) if (!(fIn = fopen(fnIn,"rb"))) { perror(fnIn); }
  /* fprintf(stderr,"%s is open for reading\n", fnIn); */

  if (strcmp("-",fnOut)) if (!(fOut = fopen(fnOut,"wb"))) { perror(fnOut); }
  /* fprintf(stderr,"%s is open for writing\n", fnOut); */


  /* Macro to reset state when parsing fails */

# define DOFULLRESET \
  { \
    if (pNextChar > inBuffer) { \
      \
      if (!pStartBackup && !pStopBackup && !stateDepth && state[0] > WANTfirstEqual) { \
      int saveChar; \
        \
        /* If parsing got to the first equals sign, */ \
        /* then set backup input start to 5-byte offset from start of */ \
        /* buffer, set backup input end of buffered chars (pNextChar), */ \
        /* and write first five byes of buffer >>>(i__N<<< to fOut */ \
        \
        pStartBackup = inBuffer + 5; \
        pStopBackup = pNextChar; \
        fwrite(inBuffer, sizeof(char), (size_t)(pStartBackup-inBuffer), fOut); \
        \
        /* Reset paren depth */ \
        \
        parenDepth = savedParenDepth + 1; \
        \
        /* Log what was done to stderr */ \
        \
        saveChar = *pStartBackup; \
        \
        *pStartBackup = '\0'; \
        fprintf(stderr,"%s:  Wrote %ld char%s; last is %02x[%c]; state[%d] = %d; savedBracket/bracket/parenDepth=%d/%d/%d\n>>>%s<<<\n" \
                      , argc>1 ? argv[1] : "[stdin]" \
                      , (long)(pStartBackup-inBuffer) \
                      , (pStartBackup-inBuffer)>1 ? "s" : "" \
                      , (int)((unsigned char) pStartBackup[-1]) \
                      , pStartBackup[-1] \
                      , (int) stateDepth \
                      , (int) state[stateDepth] \
                      , savedBracketDepth \
                      , bracketDepth \
                      , parenDepth \
                      , inBuffer \
               ); \
        *pStartBackup = saveChar; \
        *pNextChar = '\0'; \
        fprintf(stderr,"  - Full buffer=\n>>>%s<<<\n",inBuffer); \
        \
      } else { \
        \
        /* Write all bytes, which have been read into buffer, to fOut */ \
        \
        fwrite(inBuffer, sizeof(char), (size_t)(pNextChar-inBuffer), fOut); \
        \
        if (stateDepth==STATE_NEWLINE_SEQ || state[stateDepth] > WANTfirstEqual) { \
          \
          /* If parsing got to the first equals sign, */ \
          /* then log what was done to stderr */ \
          \
          *pNextChar = '\0'; \
          fprintf(stderr,"%s:  Wrote %ld char%s; last is %02x[%c]; state[%d] = %d; savedBracket/bracket/parenDepth=%d/%d/%d\n>>>%s<<<\n" \
                        , argc>1 ? argv[1] : "[stdin]" \
                        , (long)(pNextChar-inBuffer) \
                        , (pNextChar-inBuffer)>1 ? "s" : "" \
                        , (int)((unsigned char) pNextChar[-1]) \
                        , pNextChar[-1] \
                        , (int) stateDepth \
                        , (int) state[stateDepth] \
                        , savedBracketDepth \
                        , bracketDepth \
                        , parenDepth \
                        , inBuffer \
                 ); \
          /* End of loggin */ \
        } \
      } \
    } \
    /* Reset next character pointer to start of buffer, reset state */ \
    pNextChar = inBuffer; \
    state[stateDepth = STATE_BOUNDSCHECK_SEQ] = WANTfirstOpenParenOrBracket; \
  }

/* NOTTICKED macro ensures none of '(', '['. ')' or ']' change paren or
 * bracket depth
 */
# define BACKCHAR(N) (((pCurrentChar-(N)) >= inBuffer) ? pCurrentChar[N] : 0)
# define LC1 BACKCHAR(-1)
# define LC3 BACKCHAR(-3)
# define LC4 BACKCHAR(-4)
# define NOTTICKED (LC1==ctick ? (LC3==ctick ? 1 : (LC3==cbackslash && LC4==ctick ? 1 : 0)) : 1)


  /* Initialize state variables */

  /* Paren and bracket depth */
  bracketDepth = 0;
  parenDepth = 0;

  /* No input from backup storage */
  pStartBackup = 0;
  pStopBackup = 0;

  /* No input from pushed char */
  usePushedChar = 0;

  /* Next character goes to start of input buffer; use macro to reset 
   * everything else
   */
  pNextChar = inBuffer;
  DOFULLRESET

  /* Get next character; exit on EOF */
  while (EOF != ( *(pCurrentChar = pNextChar++) = currentChar =        /* Input character: */
                  ( usePushedChar
                    ? ((usePushedChar=0), pushedChar)                  /* - First priority is from pushed char */
                    : ( (pStartBackup && (pStartBackup<pStopBackup))
                        ? *(pStartBackup++)                            /* - Second priority is from backup storage from later (after pNextChar) in input buffer */
                        : ((pStartBackup=pStopBackup=0),fgetc(fIn))    /* - Lowest priority is from fIn */
                      )
                  )
                )
        ) {

#   if 0
    /* Debug logging from backup storage */
    if (pStartBackup) {
      fprintf(stderr,"- Backup char %x[%c]\n", currentChar, (char)currentChar);
    }
#   endif

    /* IFF currently parsing a bounds check, then start a newline check */
    if (stateDepth==STATE_BOUNDSCHECK_SEQ && (state[stateDepth] != WANTfirstOpenParenOrBracket)) {
      switch (currentChar) {
      case cnl:
        state[stateDepth = STATE_NEWLINE_SEQ] = WANTtabOrFirstSpace;
        break;
      default: break;
      }
    }

    /* Keep track of paren and bracket depth */
    switch (currentChar) {
    case copenparen: if (NOTTICKED) { ++parenDepth; } break;
    case ccloseparen: if (NOTTICKED) { --parenDepth; } break;
    case copenbracket: if (NOTTICKED) { ++bracketDepth; } break;
    case cclosebracket: if (NOTTICKED) { --bracketDepth; } break;
    default: break;
    }

    /* A semi-colon always resets the state */
    if (currentChar==csemi && NOTTICKED) { DOFULLRESET continue; }


    /******************************************************************/
    /* Interpret the next character depending on current sequence
     * (bounds-check or handling newline sequence), and state within
     * that sequence
     */
    switch(state[stateDepth]) {

    /* Bounds check always starts with regex
     *
     *   /[[(](i__[1-9][0-9]* = [^ ]/
     *
     */
    case WANTfirstOpenParenOrBracket:
      if (currentChar==copenparen || currentChar==copenbracket) {
        savedParenDepth = parenDepth;
        ++state[stateDepth];
        fputc(currentChar, fOut);
        pNextChar = inBuffer;
        break;
      }
      DOFULLRESET break;

    case WANTfirstOpenParen:
      if (currentChar==copenparen) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTi:
      /* If currentChar is another open paren, then adjust saved paren
       * depth & repeat
       */
      if (currentChar==copenparen) {
        fputc(currentChar, fOut);
        --pNextChar;
        savedParenDepth = parenDepth - 1; break;
      }
      if (currentChar==ci) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    /* Two underscores */
    case WANT_first:
    case WANT_second:
      if (currentChar==c_) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    /* A digit [1:9] */
    case WANTnum:
      if (currentChar>=c1 && currentChar<=c9) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    /* A space, possibly preceeded by digits [0:9] */
    case WANTspaceAfterNum:
      if (currentChar==cspace) { ++state[stateDepth]; break; }
      if (currentChar>=c0 && currentChar<=c9) { break; }
      DOFULLRESET break;

    /* The first equal sign */
    case WANTfirstEqual:
      if (currentChar==cequal) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTspaceAfterEqual:
      if (currentChar==cspace) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTnonspaceAfterSpace:
      if (currentChar!=cspace) {
        pStartIndex = pCurrentChar;        /* set pStartIndex */
        savedBracketDepth = bracketDepth;  /* Assume currentChar is not [ or ] */
        ++state[stateDepth];
        break;
      }
      DOFULLRESET break;

    /* Initial parentheses will have matched regex
     *
     *   /[[(](i__[1-9][0-9]* = [^ ]/
     *
     * followed by index expression, ending before this final close paren
     */
    case WANTfirstCloseParen:
      if (currentChar==ccloseparen && parenDepth==savedParenDepth) {
        pStopIndex = pCurrentChar;     /* set pStopIndex here */
        ++state[stateDepth]; break;
      }
      break;

    case WANTspaceAfterCloseParen:
      if (currentChar==cspace) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTltAfterSpace:
      if (currentChar==clt) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTspaceAfterLt:
      if (currentChar==cspace) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTanotherSpaceAfterLt:
      if (currentChar==cspace
      && parenDepth==savedParenDepth
      && bracketDepth == savedBracketDepth
         ) { ++state[stateDepth]; break; }
      break;

    case WANTquery:
      if (currentChar==cquery) { ++state[stateDepth]; break; }
      if (currentChar!=cspace) { --state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTspaceAfterquery:
      if (currentChar==cspace) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTanotherSpaceAfterQuery:
      if (currentChar==cspace) { ++state[stateDepth]; break; }
      break;

    case WANTcolon:
      if (currentChar==ccolon) { ++state[stateDepth]; break; }
      if (currentChar!=cspace) { --state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTspaceAfterColon:
      if (currentChar==cspace) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTsAfterSpace:
      if (currentChar==cs) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANT_Afters:
      if (currentChar==c_) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTrAfter_:
      if (currentChar==cr) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTnAfterr:
      if (currentChar==cn) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTgAftern:
      if (currentChar==cg) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTeAfterg:
      if (currentChar==ce) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTopenParenAftere:
      if (currentChar==copenparen) { ++state[stateDepth]; break; }
      DOFULLRESET break;

    case WANTcloseParenAftere:
      /* Here's the beef:
       *
       * 1) Write the index expression [pStartIndex to pStopIndex-1]
       * 2) Discard everything else from the buffer and reset
       */
      if (currentChar==ccloseparen
      && parenDepth==savedParenDepth
      && bracketDepth == savedBracketDepth
         ) {
        fwrite(pStartIndex, sizeof(char), pStopIndex-pStartIndex, fOut); \
        pNextChar = inBuffer;
        DOFULLRESET
      }
      break;


    /******************************************************************/
    /* After newline, somewhere in bounds-check sequence above */
    case WANTtabOrFirstSpace:

      switch (currentChar) {
        /* If currentChar is a tab or a newline, then no change in state */
        case ctab: break;
        case cnl: break;

        /* If currentChar is a space, then it is either the start of a
         * sequence of four spaces (a half-tab), or it will be followed
         * by a non-space and it should be preserved for the bounds-check
         * sequence above
         */
        case cspace: ++state[stateDepth]; break;

        default:
          /* - Otherwise:
           *
           *   1) Put the currentChar back into backup storage or fIn
           *   1.1) Reset buffer pointer
           *   1.2) Revert any paren depth change
           */
          if (pStartBackup && pStartBackup<=pStopBackup) {
            --pStartBackup;
          } else {
            ungetc(currentChar,fIn); 
          }
          --pNextChar;

          switch (currentChar) {
          case copenparen: --parenDepth; break;
          case ccloseparen: ++parenDepth; break;
          case copenbracket: --bracketDepth; break;
          case cclosebracket: ++bracketDepth; break;
          default: break;
          }

          /* Reset parsing to bounds-check */
          stateDepth = STATE_BOUNDSCHECK_SEQ;

          break;
      }
      break;

    case WANTsecondSpace:

      /* Last character was a first space after a newline */

      /* - If currentChar is a second space, then it should be the start of
       *   a sequence of four spaces
       */
      if (currentChar==cspace) { ++state[stateDepth]; break; }

      /* - Otherwise undo this character and that previous first space:
       *
       *   1) Put the currentChar back into backup storage or fIn
       *   1.1) Reset buffer pointer
       *   1.2) Revert any paren depth change
       */
      if (pStartBackup && pStartBackup<=pStopBackup) {
        --pStartBackup;
      } else {
        ungetc(currentChar,fIn); 
      }
      --pNextChar;

      switch (currentChar) {
      case copenparen: --parenDepth; break;
      case ccloseparen: ++parenDepth; break;
      case copenbracket: --bracketDepth; break;
      case cclosebracket: ++bracketDepth; break;
      default: break;
      }

      /*   2)  That previous first-space-after-a-newline needs to be
       *       preserved and passed to the bounds-check sequence
       */
      usePushedChar = 1;
      pushedChar = cspace;
      --pNextChar;

      /* Reset parsing to bounds-check */
      stateDepth = STATE_BOUNDSCHECK_SEQ;

      break;

    case WANTthirdSpace:

      /* Got the third space */
      if (currentChar==cspace) { ++state[stateDepth]; break; }

      /* Or not */
      DOFULLRESET break;

    case WANTfourthSpace:

      /* Got the fourth space:  reset parsing to bounds-check sequence */
      if (currentChar==cspace) {
        stateDepth = STATE_BOUNDSCHECK_SEQ;
        break;
      }

      /* Or not */
      DOFULLRESET break;

    }
  } /* while (0==feof(fIn) || (usePushedChar==1 && pushedChar!=EOF)) */

  /*DOFULLRESET*/ /* write anything left over, typically last newline */
  if (fIn!=stdin) { fclose(fIn); }
  if (fOut!=stdout) { fclose(fOut); }

  return 0;
}
