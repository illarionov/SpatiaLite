/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is included that follows the "include" declaration
** in the input grammar file. */
#include <stdio.h>

/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/* 
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands. 
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash 
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    ParseTOKENTYPE     is the data type used for minor tokens given 
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    ParseARG_SDECL     A static variable declaration for the %extra_argument
**    ParseARG_PDECL     A parameter declaration for the %extra_argument
**    ParseARG_STORE     Code to store %extra_argument into yypParser
**    ParseARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
#define YYCODETYPE unsigned char
#define YYNOCODE 125
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE void *
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 1000000
#endif
#define ParseARG_SDECL  struct vanuatu_data *p_data ;
#define ParseARG_PDECL , struct vanuatu_data *p_data 
#define ParseARG_FETCH  struct vanuatu_data *p_data  = yypParser->p_data 
#define ParseARG_STORE yypParser->p_data  = p_data 
#define YYNSTATE 358
#define YYNRULE 153
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
static const YYMINORTYPE yyzerominor = { 0 };

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   166,  228,  229,  230,  231,  232,  233,  234,  235,  236,
 /*    10 */   237,  238,  239,  240,  241,  242,  243,  244,  245,  246,
 /*    20 */   247,  248,  249,  250,  251,  252,  253,  254,  255,  256,
 /*    30 */   257,  258,  259,  260,  358,  263,  167,  512,    1,  169,
 /*    40 */   171,  173,  174,   51,   54,   57,   60,   63,   66,   72,
 /*    50 */    78,   84,   90,   92,   94,   96,   98,  103,  108,  113,
 /*    60 */   118,  123,  128,  133,  138,  145,  152,  159,  167,  139,
 /*    70 */   143,  144,  140,  141,  142,  169,   54,  146,  150,  151,
 /*    80 */    66,   57,  147,  148,  149,   72,  171,  153,  157,  158,
 /*    90 */   170,  262,   60,   47,  173,  270,   78,  154,  155,  156,
 /*   100 */    63,  172,  273,   49,   84,  160,  164,  165,   48,  161,
 /*   110 */   162,  163,   14,  168,  175,   55,  176,   46,   46,   46,
 /*   120 */    56,  265,  177,   58,   46,   47,   47,   59,   50,  179,
 /*   130 */    47,   49,   61,   62,   49,   49,  181,   51,   64,   51,
 /*   140 */   184,   65,   51,  185,   46,  186,   70,   46,  189,   46,
 /*   150 */    46,   47,  267,  190,  191,   76,   47,   47,   47,  194,
 /*   160 */    52,   49,  195,  196,   49,   49,   82,   53,   49,  199,
 /*   170 */    51,  200,   51,  201,   51,   88,   51,   91,   95,   93,
 /*   180 */    49,   46,   47,  269,   97,   51,   16,  271,   17,   19,
 /*   190 */   178,  274,   20,   22,  276,  180,  277,   23,  279,   25,
 /*   200 */   182,  280,   67,  282,   26,   69,  187,   73,   68,  286,
 /*   210 */    30,   75,  183,   71,  192,  285,   79,   74,  290,  188,
 /*   220 */   289,   77,   34,   81,  197,   80,   85,  193,   83,  294,
 /*   230 */    38,  293,  202,   86,   87,  198,  297,   89,   42,  203,
 /*   240 */   298,   43,  300,  204,   44,  302,  205,   45,  304,  206,
 /*   250 */    99,  306,  100,  102,  101,  104,  207,  309,  308,  105,
 /*   260 */   106,  109,  208,  107,  311,  110,  312,  209,  111,  112,
 /*   270 */   114,  314,  315,  116,  115,  117,  119,  120,  210,  121,
 /*   280 */   124,  317,  122,  318,  126,  125,  129,  131,  211,  130,
 /*   290 */   321,  320,  132,  127,  134,  136,    2,  135,    3,  212,
 /*   300 */     4,  323,  137,    5,    6,  324,    8,    7,    9,  227,
 /*   310 */    10,  213,  513,  261,   11,   15,  264,   12,   13,  326,
 /*   320 */   327,  266,  268,  272,  275,   18,  214,  329,  278,  281,
 /*   330 */    21,   24,  330,  283,  513,   27,   28,  215,  332,   29,
 /*   340 */   333,  334,  216,  337,  217,  284,  287,   31,   32,  218,
 /*   350 */   339,  340,  341,  219,  220,  344,  221,   33,  346,  288,
 /*   360 */   291,  347,  348,   35,  222,  223,   36,  351,   37,  224,
 /*   370 */   292,  295,  353,  354,   39,  513,  355,  225,  226,   40,
 /*   380 */    41,  296,  299,  301,  303,  305,  307,  310,  313,  316,
 /*   390 */   319,  322,  325,  328,  331,  335,  336,  338,  342,  343,
 /*   400 */   345,  349,  350,  352,  356,  357,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    37,   38,   39,   40,   41,   42,   43,   44,   45,   46,
 /*    10 */    47,   48,   49,   50,   51,   52,   53,   54,   55,   56,
 /*    20 */    57,   58,   59,   60,   61,   62,   63,   64,   65,   66,
 /*    30 */    67,   68,   69,   70,    0,    8,    2,   35,   36,    5,
 /*    40 */     6,    7,   74,   75,   10,   11,   12,   13,   14,   15,
 /*    50 */    16,   17,   18,   19,   20,   21,   22,   23,   24,   25,
 /*    60 */    26,   27,   28,   29,   30,   31,   32,   33,    2,   43,
 /*    70 */    44,   45,   43,   44,   45,    5,   10,   57,   58,   59,
 /*    80 */    14,   11,   57,   58,   59,   15,    6,   50,   51,   52,
 /*    90 */    72,   75,   12,   75,    7,   80,   16,   50,   51,   52,
 /*   100 */    13,   73,   81,   75,   17,   64,   65,   66,   75,   64,
 /*   110 */    65,   66,    3,   71,   71,   71,   76,   75,   75,   75,
 /*   120 */    71,   75,   72,   72,   75,   75,   75,   72,   75,   73,
 /*   130 */    75,   75,   73,   73,   75,   75,   74,   75,   74,   75,
 /*   140 */    71,   74,   75,   71,   75,   71,   71,   75,   72,   75,
 /*   150 */    75,   75,   75,   72,   72,   72,   75,   75,   75,   73,
 /*   160 */    75,   75,   73,   73,   75,   75,   73,   75,   75,   74,
 /*   170 */    75,   74,   75,   74,   75,   74,   75,   71,   73,   72,
 /*   180 */    75,   75,   75,   75,   74,   75,    9,   76,    3,    9,
 /*   190 */    77,   77,    3,    9,   82,   78,   78,    3,   83,    9,
 /*   200 */    79,   79,    3,   84,    3,    9,   76,    3,   88,   85,
 /*   210 */     3,    9,   89,   88,   77,   89,    3,   90,   86,   91,
 /*   220 */    91,   90,    3,    9,   78,   92,    3,   93,   92,   87,
 /*   230 */     3,   93,   79,   94,    9,   95,   95,   94,    3,   76,
 /*   240 */    96,    3,   97,   77,    3,   98,   78,    3,   99,   79,
 /*   250 */     3,  100,   80,   80,    9,    3,  104,  101,  104,   81,
 /*   260 */     9,    3,  105,   81,  105,   82,  102,  106,    9,   82,
 /*   270 */     3,  106,  103,    9,   83,   83,    3,   84,  107,    9,
 /*   280 */     3,  107,   84,  108,    9,   85,    3,    9,  112,   86,
 /*   290 */   109,  112,   86,   85,    3,    9,    3,   87,    9,  113,
 /*   300 */     3,  113,   87,    9,    3,  110,    3,    9,    9,    1,
 /*   310 */     3,  114,  124,    4,    3,    9,    4,    3,    3,  114,
 /*   320 */   111,    4,    4,    4,    4,    9,  115,  115,    4,    4,
 /*   330 */     9,    9,  116,    4,  124,    9,    9,  120,  120,    9,
 /*   340 */   120,  120,  120,  117,  120,    4,    4,    9,    9,  121,
 /*   350 */   121,  121,  121,  121,  121,  118,  122,    9,  122,    4,
 /*   360 */     4,  122,  122,    9,  122,  122,    9,  119,    9,  123,
 /*   370 */     4,    4,  123,  123,    9,  124,  123,  123,  123,    9,
 /*   380 */     9,    4,    4,    4,    4,    4,    4,    4,    4,    4,
 /*   390 */     4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
 /*   400 */     4,    4,    4,    4,    4,    4,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 226
static const short yy_shift_ofst[] = {
 /*     0 */    -1,   34,   66,   66,   70,   70,   80,   80,   87,   87,
 /*    10 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    20 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    30 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    40 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    50 */    27,   27,   27,   27,  109,  177,  177,  185,  180,  180,
 /*    60 */   189,  184,  184,  194,  190,  190,  199,  201,  196,  201,
 /*    70 */   177,  196,  204,  207,  202,  207,  180,  202,  213,  219,
 /*    80 */   214,  219,  184,  214,  223,  227,  225,  227,  190,  225,
 /*    90 */   235,  177,  238,  180,  241,  184,  244,  190,  247,  109,
 /*   100 */   245,  109,  245,  252,  185,  251,  185,  251,  258,  189,
 /*   110 */   259,  189,  259,  267,  194,  264,  194,  264,  273,  199,
 /*   120 */   270,  199,  270,  277,  204,  275,  204,  275,  283,  213,
 /*   130 */   278,  213,  278,  291,  223,  286,  223,  286,  293,  289,
 /*   140 */   289,  289,  289,  289,  289,  297,  294,  294,  294,  294,
 /*   150 */   294,  294,  301,  298,  298,  298,  298,  298,  298,  303,
 /*   160 */   299,  299,  299,  299,  299,  299,  308,  307,  309,  311,
 /*   170 */   312,  314,  317,  315,  318,  306,  319,  316,  320,  321,
 /*   180 */   324,  322,  325,  329,  326,  327,  330,  341,  342,  338,
 /*   190 */   339,  348,  355,  356,  354,  357,  359,  366,  367,  365,
 /*   200 */   370,  371,  377,  378,  379,  380,  381,  382,  383,  384,
 /*   210 */   385,  386,  387,  388,  389,  390,  391,  392,  393,  394,
 /*   220 */   395,  396,  397,  398,  399,  400,  401,
};
#define YY_REDUCE_USE_DFLT (-38)
#define YY_REDUCE_MAX 165
static const short yy_reduce_ofst[] = {
 /*     0 */     2,  -37,   26,   29,   20,   25,   37,   47,   41,   45,
 /*    10 */    42,   18,   28,  -32,   43,   44,   49,   50,   51,   55,
 /*    20 */    56,   59,   60,   62,   64,   67,   69,   72,   74,   75,
 /*    30 */    76,   81,   82,   83,   86,   89,   90,   93,   95,   97,
 /*    40 */    99,  101,  106,  107,  105,  110,   16,   33,   46,   53,
 /*    50 */    77,   85,   92,  108,   15,   40,  111,   21,  113,  114,
 /*    60 */   112,  117,  118,  115,  121,  122,  119,  120,  123,  125,
 /*    70 */   130,  126,  124,  127,  128,  131,  137,  129,  132,  133,
 /*    80 */   134,  136,  146,  138,  142,  139,  140,  143,  153,  141,
 /*    90 */   144,  163,  145,  166,  147,  168,  149,  170,  151,  172,
 /*   100 */   152,  173,  154,  156,  178,  157,  182,  159,  164,  183,
 /*   110 */   161,  187,  165,  169,  191,  171,  192,  174,  175,  193,
 /*   120 */   176,  198,  179,  181,  200,  186,  208,  188,  195,  203,
 /*   130 */   197,  206,  205,  209,  210,  211,  215,  212,  216,  217,
 /*   140 */   218,  220,  221,  222,  224,  226,  228,  229,  230,  231,
 /*   150 */   232,  233,  237,  234,  236,  239,  240,  242,  243,  248,
 /*   160 */   246,  249,  250,  253,  254,  255,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   359,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*    10 */   511,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*    20 */   511,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*    30 */   511,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*    40 */   511,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*    50 */   511,  511,  511,  511,  511,  403,  403,  511,  405,  405,
 /*    60 */   511,  407,  407,  511,  409,  409,  511,  511,  428,  511,
 /*    70 */   403,  428,  511,  511,  431,  511,  405,  431,  511,  511,
 /*    80 */   434,  511,  407,  434,  511,  511,  437,  511,  409,  437,
 /*    90 */   511,  403,  511,  405,  511,  407,  511,  409,  511,  511,
 /*   100 */   452,  511,  452,  511,  511,  455,  511,  455,  511,  511,
 /*   110 */   458,  511,  458,  511,  511,  461,  511,  461,  511,  511,
 /*   120 */   468,  511,  468,  511,  511,  471,  511,  471,  511,  511,
 /*   130 */   474,  511,  474,  511,  511,  477,  511,  477,  511,  486,
 /*   140 */   486,  486,  486,  486,  486,  511,  493,  493,  493,  493,
 /*   150 */   493,  493,  511,  500,  500,  500,  500,  500,  500,  511,
 /*   160 */   507,  507,  507,  507,  507,  507,  511,  511,  511,  511,
 /*   170 */   511,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*   180 */   511,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*   190 */   511,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*   200 */   511,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*   210 */   511,  511,  511,  511,  511,  511,  511,  511,  511,  511,
 /*   220 */   511,  511,  511,  511,  511,  511,  511,  360,  361,  362,
 /*   230 */   363,  364,  365,  366,  367,  368,  369,  370,  371,  372,
 /*   240 */   373,  374,  375,  376,  377,  378,  379,  380,  381,  382,
 /*   250 */   383,  384,  385,  386,  387,  388,  389,  390,  391,  392,
 /*   260 */   393,  394,  398,  402,  395,  399,  396,  400,  397,  401,
 /*   270 */   411,  404,  415,  412,  406,  416,  413,  408,  417,  414,
 /*   280 */   410,  418,  419,  423,  427,  429,  420,  424,  430,  432,
 /*   290 */   421,  425,  433,  435,  422,  426,  436,  438,  439,  443,
 /*   300 */   440,  444,  441,  445,  442,  446,  447,  451,  453,  448,
 /*   310 */   454,  456,  449,  457,  459,  450,  460,  462,  463,  467,
 /*   320 */   469,  464,  470,  472,  465,  473,  475,  466,  476,  478,
 /*   330 */   479,  483,  487,  488,  489,  484,  485,  480,  490,  494,
 /*   340 */   495,  496,  491,  492,  481,  497,  501,  502,  503,  498,
 /*   350 */   499,  482,  504,  508,  509,  510,  505,  506,
};
#define YY_SZ_ACTTAB (int)(sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt;                 /* Shifts left before out of the error */
  ParseARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void ParseTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "VANUATU_NEWLINE",  "VANUATU_POINT",  "VANUATU_OPEN_BRACKET",
  "VANUATU_CLOSE_BRACKET",  "VANUATU_POINT_M",  "VANUATU_POINT_Z",  "VANUATU_POINT_ZM",
  "VANUATU_NUM",   "VANUATU_COMMA",  "VANUATU_LINESTRING",  "VANUATU_LINESTRING_M",
  "VANUATU_LINESTRING_Z",  "VANUATU_LINESTRING_ZM",  "VANUATU_POLYGON",  "VANUATU_POLYGON_M",
  "VANUATU_POLYGON_Z",  "VANUATU_POLYGON_ZM",  "VANUATU_MULTIPOINT",  "VANUATU_MULTIPOINT_M",
  "VANUATU_MULTIPOINT_Z",  "VANUATU_MULTIPOINT_ZM",  "VANUATU_MULTILINESTRING",  "VANUATU_MULTILINESTRING_M",
  "VANUATU_MULTILINESTRING_Z",  "VANUATU_MULTILINESTRING_ZM",  "VANUATU_MULTIPOLYGON",  "VANUATU_MULTIPOLYGON_M",
  "VANUATU_MULTIPOLYGON_Z",  "VANUATU_MULTIPOLYGON_ZM",  "VANUATU_GEOMETRYCOLLECTION",  "VANUATU_GEOMETRYCOLLECTION_M",
  "VANUATU_GEOMETRYCOLLECTION_Z",  "VANUATU_GEOMETRYCOLLECTION_ZM",  "error",         "main",        
  "in",            "state",         "program",       "geo_text",    
  "geo_textz",     "geo_textm",     "geo_textzm",    "point",       
  "linestring",    "polygon",       "multipoint",    "multilinestring",
  "multipolygon",  "geocoll",       "pointz",        "linestringz", 
  "polygonz",      "multipointz",   "multilinestringz",  "multipolygonz",
  "geocollz",      "pointm",        "linestringm",   "polygonm",    
  "multipointm",   "multilinestringm",  "multipolygonm",  "geocollm",    
  "pointzm",       "linestringzm",  "polygonzm",     "multipointzm",
  "multilinestringzm",  "multipolygonzm",  "geocollzm",     "point_coordxy",
  "point_coordxym",  "point_coordxyz",  "point_coordxyzm",  "coord",       
  "extra_pointsxy",  "extra_pointsxym",  "extra_pointsxyz",  "extra_pointsxyzm",
  "linestring_text",  "linestring_textm",  "linestring_textz",  "linestring_textzm",
  "polygon_text",  "polygon_textm",  "polygon_textz",  "polygon_textzm",
  "ring",          "extra_rings",   "ringm",         "extra_ringsm",
  "ringz",         "extra_ringsz",  "ringzm",        "extra_ringszm",
  "multipoint_text",  "multipoint_textm",  "multipoint_textz",  "multipoint_textzm",
  "multilinestring_text",  "multilinestring_textm",  "multilinestring_textz",  "multilinestring_textzm",
  "multilinestring_text2",  "multilinestring_textm2",  "multilinestring_textz2",  "multilinestring_textzm2",
  "multipolygon_text",  "multipolygon_textm",  "multipolygon_textz",  "multipolygon_textzm",
  "multipolygon_text2",  "multipolygon_textm2",  "multipolygon_textz2",  "multipolygon_textzm2",
  "geocoll_text",  "geocoll_textm",  "geocoll_textz",  "geocoll_textzm",
  "geocoll_text2",  "geocoll_textm2",  "geocoll_textz2",  "geocoll_textzm2",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "main ::= in",
 /*   1 */ "in ::=",
 /*   2 */ "in ::= in state VANUATU_NEWLINE",
 /*   3 */ "state ::= program",
 /*   4 */ "program ::= geo_text",
 /*   5 */ "program ::= geo_textz",
 /*   6 */ "program ::= geo_textm",
 /*   7 */ "program ::= geo_textzm",
 /*   8 */ "geo_text ::= point",
 /*   9 */ "geo_text ::= linestring",
 /*  10 */ "geo_text ::= polygon",
 /*  11 */ "geo_text ::= multipoint",
 /*  12 */ "geo_text ::= multilinestring",
 /*  13 */ "geo_text ::= multipolygon",
 /*  14 */ "geo_text ::= geocoll",
 /*  15 */ "geo_textz ::= pointz",
 /*  16 */ "geo_textz ::= linestringz",
 /*  17 */ "geo_textz ::= polygonz",
 /*  18 */ "geo_textz ::= multipointz",
 /*  19 */ "geo_textz ::= multilinestringz",
 /*  20 */ "geo_textz ::= multipolygonz",
 /*  21 */ "geo_textz ::= geocollz",
 /*  22 */ "geo_textm ::= pointm",
 /*  23 */ "geo_textm ::= linestringm",
 /*  24 */ "geo_textm ::= polygonm",
 /*  25 */ "geo_textm ::= multipointm",
 /*  26 */ "geo_textm ::= multilinestringm",
 /*  27 */ "geo_textm ::= multipolygonm",
 /*  28 */ "geo_textm ::= geocollm",
 /*  29 */ "geo_textzm ::= pointzm",
 /*  30 */ "geo_textzm ::= linestringzm",
 /*  31 */ "geo_textzm ::= polygonzm",
 /*  32 */ "geo_textzm ::= multipointzm",
 /*  33 */ "geo_textzm ::= multilinestringzm",
 /*  34 */ "geo_textzm ::= multipolygonzm",
 /*  35 */ "geo_textzm ::= geocollzm",
 /*  36 */ "point ::= VANUATU_POINT VANUATU_OPEN_BRACKET point_coordxy VANUATU_CLOSE_BRACKET",
 /*  37 */ "pointm ::= VANUATU_POINT_M VANUATU_OPEN_BRACKET point_coordxym VANUATU_CLOSE_BRACKET",
 /*  38 */ "pointz ::= VANUATU_POINT_Z VANUATU_OPEN_BRACKET point_coordxyz VANUATU_CLOSE_BRACKET",
 /*  39 */ "pointzm ::= VANUATU_POINT_ZM VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_CLOSE_BRACKET",
 /*  40 */ "point_coordxy ::= coord coord",
 /*  41 */ "point_coordxym ::= coord coord coord",
 /*  42 */ "point_coordxyz ::= coord coord coord",
 /*  43 */ "point_coordxyzm ::= coord coord coord coord",
 /*  44 */ "coord ::= VANUATU_NUM",
 /*  45 */ "extra_pointsxy ::=",
 /*  46 */ "extra_pointsxy ::= VANUATU_COMMA point_coordxy extra_pointsxy",
 /*  47 */ "extra_pointsxym ::=",
 /*  48 */ "extra_pointsxym ::= VANUATU_COMMA point_coordxym extra_pointsxym",
 /*  49 */ "extra_pointsxyz ::=",
 /*  50 */ "extra_pointsxyz ::= VANUATU_COMMA point_coordxyz extra_pointsxyz",
 /*  51 */ "extra_pointsxyzm ::=",
 /*  52 */ "extra_pointsxyzm ::= VANUATU_COMMA point_coordxyzm extra_pointsxyzm",
 /*  53 */ "linestring ::= VANUATU_LINESTRING linestring_text",
 /*  54 */ "linestringm ::= VANUATU_LINESTRING_M linestring_textm",
 /*  55 */ "linestringz ::= VANUATU_LINESTRING_Z linestring_textz",
 /*  56 */ "linestringzm ::= VANUATU_LINESTRING_ZM linestring_textzm",
 /*  57 */ "linestring_text ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET",
 /*  58 */ "linestring_textm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET",
 /*  59 */ "linestring_textz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET",
 /*  60 */ "linestring_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET",
 /*  61 */ "polygon ::= VANUATU_POLYGON polygon_text",
 /*  62 */ "polygonm ::= VANUATU_POLYGON_M polygon_textm",
 /*  63 */ "polygonz ::= VANUATU_POLYGON_Z polygon_textz",
 /*  64 */ "polygonzm ::= VANUATU_POLYGON_ZM polygon_textzm",
 /*  65 */ "polygon_text ::= VANUATU_OPEN_BRACKET ring extra_rings VANUATU_CLOSE_BRACKET",
 /*  66 */ "polygon_textm ::= VANUATU_OPEN_BRACKET ringm extra_ringsm VANUATU_CLOSE_BRACKET",
 /*  67 */ "polygon_textz ::= VANUATU_OPEN_BRACKET ringz extra_ringsz VANUATU_CLOSE_BRACKET",
 /*  68 */ "polygon_textzm ::= VANUATU_OPEN_BRACKET ringzm extra_ringszm VANUATU_CLOSE_BRACKET",
 /*  69 */ "ring ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET",
 /*  70 */ "extra_rings ::=",
 /*  71 */ "extra_rings ::= VANUATU_COMMA ring extra_rings",
 /*  72 */ "ringm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET",
 /*  73 */ "extra_ringsm ::=",
 /*  74 */ "extra_ringsm ::= VANUATU_COMMA ringm extra_ringsm",
 /*  75 */ "ringz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET",
 /*  76 */ "extra_ringsz ::=",
 /*  77 */ "extra_ringsz ::= VANUATU_COMMA ringz extra_ringsz",
 /*  78 */ "ringzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET",
 /*  79 */ "extra_ringszm ::=",
 /*  80 */ "extra_ringszm ::= VANUATU_COMMA ringzm extra_ringszm",
 /*  81 */ "multipoint ::= VANUATU_MULTIPOINT multipoint_text",
 /*  82 */ "multipointm ::= VANUATU_MULTIPOINT_M multipoint_textm",
 /*  83 */ "multipointz ::= VANUATU_MULTIPOINT_Z multipoint_textz",
 /*  84 */ "multipointzm ::= VANUATU_MULTIPOINT_ZM multipoint_textzm",
 /*  85 */ "multipoint_text ::= VANUATU_OPEN_BRACKET point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET",
 /*  86 */ "multipoint_textm ::= VANUATU_OPEN_BRACKET point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET",
 /*  87 */ "multipoint_textz ::= VANUATU_OPEN_BRACKET point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET",
 /*  88 */ "multipoint_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET",
 /*  89 */ "multilinestring ::= VANUATU_MULTILINESTRING multilinestring_text",
 /*  90 */ "multilinestringm ::= VANUATU_MULTILINESTRING_M multilinestring_textm",
 /*  91 */ "multilinestringz ::= VANUATU_MULTILINESTRING_Z multilinestring_textz",
 /*  92 */ "multilinestringzm ::= VANUATU_MULTILINESTRING_ZM multilinestring_textzm",
 /*  93 */ "multilinestring_text ::= VANUATU_OPEN_BRACKET linestring_text multilinestring_text2 VANUATU_CLOSE_BRACKET",
 /*  94 */ "multilinestring_text2 ::=",
 /*  95 */ "multilinestring_text2 ::= VANUATU_COMMA linestring_text multilinestring_text2",
 /*  96 */ "multilinestring_textm ::= VANUATU_OPEN_BRACKET linestring_textm multilinestring_textm2 VANUATU_CLOSE_BRACKET",
 /*  97 */ "multilinestring_textm2 ::=",
 /*  98 */ "multilinestring_textm2 ::= VANUATU_COMMA linestring_textm multilinestring_textm2",
 /*  99 */ "multilinestring_textz ::= VANUATU_OPEN_BRACKET linestring_textz multilinestring_textz2 VANUATU_CLOSE_BRACKET",
 /* 100 */ "multilinestring_textz2 ::=",
 /* 101 */ "multilinestring_textz2 ::= VANUATU_COMMA linestring_textz multilinestring_textz2",
 /* 102 */ "multilinestring_textzm ::= VANUATU_OPEN_BRACKET linestring_textzm multilinestring_textzm2 VANUATU_CLOSE_BRACKET",
 /* 103 */ "multilinestring_textzm2 ::=",
 /* 104 */ "multilinestring_textzm2 ::= VANUATU_COMMA linestring_textzm multilinestring_textzm2",
 /* 105 */ "multipolygon ::= VANUATU_MULTIPOLYGON multipolygon_text",
 /* 106 */ "multipolygonm ::= VANUATU_MULTIPOLYGON_M multipolygon_textm",
 /* 107 */ "multipolygonz ::= VANUATU_MULTIPOLYGON_Z multipolygon_textz",
 /* 108 */ "multipolygonzm ::= VANUATU_MULTIPOLYGON_ZM multipolygon_textzm",
 /* 109 */ "multipolygon_text ::= VANUATU_OPEN_BRACKET polygon_text multipolygon_text2 VANUATU_CLOSE_BRACKET",
 /* 110 */ "multipolygon_text2 ::=",
 /* 111 */ "multipolygon_text2 ::= VANUATU_COMMA polygon_text multipolygon_text2",
 /* 112 */ "multipolygon_textm ::= VANUATU_OPEN_BRACKET polygon_textm multipolygon_textm2 VANUATU_CLOSE_BRACKET",
 /* 113 */ "multipolygon_textm2 ::=",
 /* 114 */ "multipolygon_textm2 ::= VANUATU_COMMA polygon_textm multipolygon_textm2",
 /* 115 */ "multipolygon_textz ::= VANUATU_OPEN_BRACKET polygon_textz multipolygon_textz2 VANUATU_CLOSE_BRACKET",
 /* 116 */ "multipolygon_textz2 ::=",
 /* 117 */ "multipolygon_textz2 ::= VANUATU_COMMA polygon_textz multipolygon_textz2",
 /* 118 */ "multipolygon_textzm ::= VANUATU_OPEN_BRACKET polygon_textzm multipolygon_textzm2 VANUATU_CLOSE_BRACKET",
 /* 119 */ "multipolygon_textzm2 ::=",
 /* 120 */ "multipolygon_textzm2 ::= VANUATU_COMMA polygon_textzm multipolygon_textzm2",
 /* 121 */ "geocoll ::= VANUATU_GEOMETRYCOLLECTION geocoll_text",
 /* 122 */ "geocollm ::= VANUATU_GEOMETRYCOLLECTION_M geocoll_textm",
 /* 123 */ "geocollz ::= VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz",
 /* 124 */ "geocollzm ::= VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm",
 /* 125 */ "geocoll_text ::= VANUATU_OPEN_BRACKET point geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 126 */ "geocoll_text ::= VANUATU_OPEN_BRACKET linestring geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 127 */ "geocoll_text ::= VANUATU_OPEN_BRACKET polygon geocoll_text2 VANUATU_CLOSE_BRACKET",
 /* 128 */ "geocoll_text2 ::=",
 /* 129 */ "geocoll_text2 ::= VANUATU_COMMA point geocoll_text2",
 /* 130 */ "geocoll_text2 ::= VANUATU_COMMA linestring geocoll_text2",
 /* 131 */ "geocoll_text2 ::= VANUATU_COMMA polygon geocoll_text2",
 /* 132 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET pointm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 133 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET linestringm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 134 */ "geocoll_textm ::= VANUATU_OPEN_BRACKET polygonm geocoll_textm2 VANUATU_CLOSE_BRACKET",
 /* 135 */ "geocoll_textm2 ::=",
 /* 136 */ "geocoll_textm2 ::= VANUATU_COMMA pointm geocoll_textm2",
 /* 137 */ "geocoll_textm2 ::= VANUATU_COMMA linestringm geocoll_textm2",
 /* 138 */ "geocoll_textm2 ::= VANUATU_COMMA polygonm geocoll_textm2",
 /* 139 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET pointz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 140 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET linestringz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 141 */ "geocoll_textz ::= VANUATU_OPEN_BRACKET polygonz geocoll_textz2 VANUATU_CLOSE_BRACKET",
 /* 142 */ "geocoll_textz2 ::=",
 /* 143 */ "geocoll_textz2 ::= VANUATU_COMMA pointz geocoll_textz2",
 /* 144 */ "geocoll_textz2 ::= VANUATU_COMMA linestringz geocoll_textz2",
 /* 145 */ "geocoll_textz2 ::= VANUATU_COMMA polygonz geocoll_textz2",
 /* 146 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET pointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 147 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET linestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 148 */ "geocoll_textzm ::= VANUATU_OPEN_BRACKET polygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET",
 /* 149 */ "geocoll_textzm2 ::=",
 /* 150 */ "geocoll_textzm2 ::= VANUATU_COMMA pointzm geocoll_textzm2",
 /* 151 */ "geocoll_textzm2 ::= VANUATU_COMMA linestringzm geocoll_textzm2",
 /* 152 */ "geocoll_textzm2 ::= VANUATU_COMMA polygonzm geocoll_textzm2",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to Parse and ParseFree.
*/
void *ParseAlloc(void *(*mallocProc)(size_t)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (size_t)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
#ifndef NDEBUG
  if( yyTraceFILE && pParser->yyidx>=0 ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yymajor = yytos->major;
  yy_destructor(pParser, yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/* 
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from ParseAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void ParseFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int ParseStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>YY_SHIFT_MAX || (i = yy_shift_ofst[stateno])==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    if( iLookAhead>0 ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        return yy_find_shift_action(pParser, iFallback);
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( j>=0 && j<YY_SZ_ACTTAB && yy_lookahead[j]==YYWILDCARD ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
    }
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_MAX ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_MAX );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_SZ_ACTTAB );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser, YYMINORTYPE *yypMinor){
   ParseARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */

     fprintf(stderr, "Giving up.  Parser stack overflow\n");
   ParseARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser, yypMinor);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser, yypMinor);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor = *yypMinor;
#ifndef NDEBUG
  if( yyTraceFILE && yypParser->yyidx>0 ){
    int i;
    fprintf(yyTraceFILE,"%sShift %d\n",yyTracePrompt,yyNewState);
    fprintf(yyTraceFILE,"%sStack:",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE," %s",yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"\n");
  }
#endif
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 35, 1 },
  { 36, 0 },
  { 36, 3 },
  { 37, 1 },
  { 38, 1 },
  { 38, 1 },
  { 38, 1 },
  { 38, 1 },
  { 39, 1 },
  { 39, 1 },
  { 39, 1 },
  { 39, 1 },
  { 39, 1 },
  { 39, 1 },
  { 39, 1 },
  { 40, 1 },
  { 40, 1 },
  { 40, 1 },
  { 40, 1 },
  { 40, 1 },
  { 40, 1 },
  { 40, 1 },
  { 41, 1 },
  { 41, 1 },
  { 41, 1 },
  { 41, 1 },
  { 41, 1 },
  { 41, 1 },
  { 41, 1 },
  { 42, 1 },
  { 42, 1 },
  { 42, 1 },
  { 42, 1 },
  { 42, 1 },
  { 42, 1 },
  { 42, 1 },
  { 43, 4 },
  { 57, 4 },
  { 50, 4 },
  { 64, 4 },
  { 71, 2 },
  { 72, 3 },
  { 73, 3 },
  { 74, 4 },
  { 75, 1 },
  { 76, 0 },
  { 76, 3 },
  { 77, 0 },
  { 77, 3 },
  { 78, 0 },
  { 78, 3 },
  { 79, 0 },
  { 79, 3 },
  { 44, 2 },
  { 58, 2 },
  { 51, 2 },
  { 65, 2 },
  { 80, 6 },
  { 81, 6 },
  { 82, 6 },
  { 83, 6 },
  { 45, 2 },
  { 59, 2 },
  { 52, 2 },
  { 66, 2 },
  { 84, 4 },
  { 85, 4 },
  { 86, 4 },
  { 87, 4 },
  { 88, 10 },
  { 89, 0 },
  { 89, 3 },
  { 90, 10 },
  { 91, 0 },
  { 91, 3 },
  { 92, 10 },
  { 93, 0 },
  { 93, 3 },
  { 94, 10 },
  { 95, 0 },
  { 95, 3 },
  { 46, 2 },
  { 60, 2 },
  { 53, 2 },
  { 67, 2 },
  { 96, 4 },
  { 97, 4 },
  { 98, 4 },
  { 99, 4 },
  { 47, 2 },
  { 61, 2 },
  { 54, 2 },
  { 68, 2 },
  { 100, 4 },
  { 104, 0 },
  { 104, 3 },
  { 101, 4 },
  { 105, 0 },
  { 105, 3 },
  { 102, 4 },
  { 106, 0 },
  { 106, 3 },
  { 103, 4 },
  { 107, 0 },
  { 107, 3 },
  { 48, 2 },
  { 62, 2 },
  { 55, 2 },
  { 69, 2 },
  { 108, 4 },
  { 112, 0 },
  { 112, 3 },
  { 109, 4 },
  { 113, 0 },
  { 113, 3 },
  { 110, 4 },
  { 114, 0 },
  { 114, 3 },
  { 111, 4 },
  { 115, 0 },
  { 115, 3 },
  { 49, 2 },
  { 63, 2 },
  { 56, 2 },
  { 70, 2 },
  { 116, 4 },
  { 116, 4 },
  { 116, 4 },
  { 120, 0 },
  { 120, 3 },
  { 120, 3 },
  { 120, 3 },
  { 117, 4 },
  { 117, 4 },
  { 117, 4 },
  { 121, 0 },
  { 121, 3 },
  { 121, 3 },
  { 121, 3 },
  { 118, 4 },
  { 118, 4 },
  { 118, 4 },
  { 122, 0 },
  { 122, 3 },
  { 122, 3 },
  { 122, 3 },
  { 119, 4 },
  { 119, 4 },
  { 119, 4 },
  { 123, 0 },
  { 123, 3 },
  { 123, 3 },
  { 123, 3 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  ParseARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    fprintf(yyTraceFILE, "%sReduce [%s].\n", yyTracePrompt,
      yyRuleName[yyruleno]);
  }
#endif /* NDEBUG */

  /* Silence complaints from purify about yygotominor being uninitialized
  ** in some cases when it is copied into the stack after the following
  ** switch.  yygotominor is uninitialized when a rule reduces that does
  ** not set the value of its left-hand side nonterminal.  Leaving the
  ** value of the nonterminal uninitialized is utterly harmless as long
  ** as the value is never used.  So really the only thing this code
  ** accomplishes is to quieten purify.  
  **
  ** 2007-01-16:  The wireshark project (www.wireshark.org) reports that
  ** without this code, their parser segfaults.  I'm not sure what there
  ** parser is doing to make this happen.  This is the second bug report
  ** from wireshark this week.  Clearly they are stressing Lemon in ways
  ** that it has not been previously stressed...  (SQLite ticket #2172)
  */
  /*memset(&yygotominor, 0, sizeof(yygotominor));*/
  yygotominor = yyzerominor;


  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 8: /* geo_text ::= point */
      case 9: /* geo_text ::= linestring */ yytestcase(yyruleno==9);
      case 10: /* geo_text ::= polygon */ yytestcase(yyruleno==10);
      case 11: /* geo_text ::= multipoint */ yytestcase(yyruleno==11);
      case 12: /* geo_text ::= multilinestring */ yytestcase(yyruleno==12);
      case 13: /* geo_text ::= multipolygon */ yytestcase(yyruleno==13);
      case 14: /* geo_text ::= geocoll */ yytestcase(yyruleno==14);
      case 15: /* geo_textz ::= pointz */ yytestcase(yyruleno==15);
      case 16: /* geo_textz ::= linestringz */ yytestcase(yyruleno==16);
      case 17: /* geo_textz ::= polygonz */ yytestcase(yyruleno==17);
      case 18: /* geo_textz ::= multipointz */ yytestcase(yyruleno==18);
      case 19: /* geo_textz ::= multilinestringz */ yytestcase(yyruleno==19);
      case 20: /* geo_textz ::= multipolygonz */ yytestcase(yyruleno==20);
      case 21: /* geo_textz ::= geocollz */ yytestcase(yyruleno==21);
      case 22: /* geo_textm ::= pointm */ yytestcase(yyruleno==22);
      case 23: /* geo_textm ::= linestringm */ yytestcase(yyruleno==23);
      case 24: /* geo_textm ::= polygonm */ yytestcase(yyruleno==24);
      case 25: /* geo_textm ::= multipointm */ yytestcase(yyruleno==25);
      case 26: /* geo_textm ::= multilinestringm */ yytestcase(yyruleno==26);
      case 27: /* geo_textm ::= multipolygonm */ yytestcase(yyruleno==27);
      case 28: /* geo_textm ::= geocollm */ yytestcase(yyruleno==28);
      case 29: /* geo_textzm ::= pointzm */ yytestcase(yyruleno==29);
      case 30: /* geo_textzm ::= linestringzm */ yytestcase(yyruleno==30);
      case 31: /* geo_textzm ::= polygonzm */ yytestcase(yyruleno==31);
      case 32: /* geo_textzm ::= multipointzm */ yytestcase(yyruleno==32);
      case 33: /* geo_textzm ::= multilinestringzm */ yytestcase(yyruleno==33);
      case 34: /* geo_textzm ::= multipolygonzm */ yytestcase(yyruleno==34);
      case 35: /* geo_textzm ::= geocollzm */ yytestcase(yyruleno==35);
{ p_data->result = yymsp[0].minor.yy0; }
        break;
      case 36: /* point ::= VANUATU_POINT VANUATU_OPEN_BRACKET point_coordxy VANUATU_CLOSE_BRACKET */
{ yygotominor.yy0 = vanuatu_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0); }
        break;
      case 37: /* pointm ::= VANUATU_POINT_M VANUATU_OPEN_BRACKET point_coordxym VANUATU_CLOSE_BRACKET */
      case 38: /* pointz ::= VANUATU_POINT_Z VANUATU_OPEN_BRACKET point_coordxyz VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==38);
      case 39: /* pointzm ::= VANUATU_POINT_ZM VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==39);
{ yygotominor.yy0 = vanuatu_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0);  }
        break;
      case 40: /* point_coordxy ::= coord coord */
{ yygotominor.yy0 = (void *) vanuatu_point_xy( p_data, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 41: /* point_coordxym ::= coord coord coord */
{ yygotominor.yy0 = (void *) vanuatu_point_xym( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 42: /* point_coordxyz ::= coord coord coord */
{ yygotominor.yy0 = (void *) vanuatu_point_xyz( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 43: /* point_coordxyzm ::= coord coord coord coord */
{ yygotominor.yy0 = (void *) vanuatu_point_xyzm( p_data, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 44: /* coord ::= VANUATU_NUM */
      case 81: /* multipoint ::= VANUATU_MULTIPOINT multipoint_text */ yytestcase(yyruleno==81);
      case 82: /* multipointm ::= VANUATU_MULTIPOINT_M multipoint_textm */ yytestcase(yyruleno==82);
      case 83: /* multipointz ::= VANUATU_MULTIPOINT_Z multipoint_textz */ yytestcase(yyruleno==83);
      case 84: /* multipointzm ::= VANUATU_MULTIPOINT_ZM multipoint_textzm */ yytestcase(yyruleno==84);
      case 89: /* multilinestring ::= VANUATU_MULTILINESTRING multilinestring_text */ yytestcase(yyruleno==89);
      case 90: /* multilinestringm ::= VANUATU_MULTILINESTRING_M multilinestring_textm */ yytestcase(yyruleno==90);
      case 91: /* multilinestringz ::= VANUATU_MULTILINESTRING_Z multilinestring_textz */ yytestcase(yyruleno==91);
      case 92: /* multilinestringzm ::= VANUATU_MULTILINESTRING_ZM multilinestring_textzm */ yytestcase(yyruleno==92);
      case 105: /* multipolygon ::= VANUATU_MULTIPOLYGON multipolygon_text */ yytestcase(yyruleno==105);
      case 106: /* multipolygonm ::= VANUATU_MULTIPOLYGON_M multipolygon_textm */ yytestcase(yyruleno==106);
      case 107: /* multipolygonz ::= VANUATU_MULTIPOLYGON_Z multipolygon_textz */ yytestcase(yyruleno==107);
      case 108: /* multipolygonzm ::= VANUATU_MULTIPOLYGON_ZM multipolygon_textzm */ yytestcase(yyruleno==108);
      case 121: /* geocoll ::= VANUATU_GEOMETRYCOLLECTION geocoll_text */ yytestcase(yyruleno==121);
      case 122: /* geocollm ::= VANUATU_GEOMETRYCOLLECTION_M geocoll_textm */ yytestcase(yyruleno==122);
      case 123: /* geocollz ::= VANUATU_GEOMETRYCOLLECTION_Z geocoll_textz */ yytestcase(yyruleno==123);
      case 124: /* geocollzm ::= VANUATU_GEOMETRYCOLLECTION_ZM geocoll_textzm */ yytestcase(yyruleno==124);
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 45: /* extra_pointsxy ::= */
      case 47: /* extra_pointsxym ::= */ yytestcase(yyruleno==47);
      case 49: /* extra_pointsxyz ::= */ yytestcase(yyruleno==49);
      case 51: /* extra_pointsxyzm ::= */ yytestcase(yyruleno==51);
      case 70: /* extra_rings ::= */ yytestcase(yyruleno==70);
      case 73: /* extra_ringsm ::= */ yytestcase(yyruleno==73);
      case 76: /* extra_ringsz ::= */ yytestcase(yyruleno==76);
      case 79: /* extra_ringszm ::= */ yytestcase(yyruleno==79);
      case 94: /* multilinestring_text2 ::= */ yytestcase(yyruleno==94);
      case 97: /* multilinestring_textm2 ::= */ yytestcase(yyruleno==97);
      case 100: /* multilinestring_textz2 ::= */ yytestcase(yyruleno==100);
      case 103: /* multilinestring_textzm2 ::= */ yytestcase(yyruleno==103);
      case 110: /* multipolygon_text2 ::= */ yytestcase(yyruleno==110);
      case 113: /* multipolygon_textm2 ::= */ yytestcase(yyruleno==113);
      case 116: /* multipolygon_textz2 ::= */ yytestcase(yyruleno==116);
      case 119: /* multipolygon_textzm2 ::= */ yytestcase(yyruleno==119);
      case 128: /* geocoll_text2 ::= */ yytestcase(yyruleno==128);
      case 135: /* geocoll_textm2 ::= */ yytestcase(yyruleno==135);
      case 142: /* geocoll_textz2 ::= */ yytestcase(yyruleno==142);
      case 149: /* geocoll_textzm2 ::= */ yytestcase(yyruleno==149);
{ yygotominor.yy0 = NULL; }
        break;
      case 46: /* extra_pointsxy ::= VANUATU_COMMA point_coordxy extra_pointsxy */
      case 48: /* extra_pointsxym ::= VANUATU_COMMA point_coordxym extra_pointsxym */ yytestcase(yyruleno==48);
      case 50: /* extra_pointsxyz ::= VANUATU_COMMA point_coordxyz extra_pointsxyz */ yytestcase(yyruleno==50);
      case 52: /* extra_pointsxyzm ::= VANUATU_COMMA point_coordxyzm extra_pointsxyzm */ yytestcase(yyruleno==52);
{ ((gaiaPointPtr)yymsp[-1].minor.yy0)->Next = (gaiaPointPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 53: /* linestring ::= VANUATU_LINESTRING linestring_text */
      case 54: /* linestringm ::= VANUATU_LINESTRING_M linestring_textm */ yytestcase(yyruleno==54);
      case 55: /* linestringz ::= VANUATU_LINESTRING_Z linestring_textz */ yytestcase(yyruleno==55);
      case 56: /* linestringzm ::= VANUATU_LINESTRING_ZM linestring_textzm */ yytestcase(yyruleno==56);
{ yygotominor.yy0 = vanuatu_buildGeomFromLinestring( p_data, (gaiaLinestringPtr)yymsp[0].minor.yy0); }
        break;
      case 57: /* linestring_text ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) vanuatu_linestring_xy( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 58: /* linestring_textm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) vanuatu_linestring_xym( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 59: /* linestring_textz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) vanuatu_linestring_xyz( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 60: /* linestring_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) vanuatu_linestring_xyzm( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 61: /* polygon ::= VANUATU_POLYGON polygon_text */
      case 62: /* polygonm ::= VANUATU_POLYGON_M polygon_textm */ yytestcase(yyruleno==62);
      case 63: /* polygonz ::= VANUATU_POLYGON_Z polygon_textz */ yytestcase(yyruleno==63);
      case 64: /* polygonzm ::= VANUATU_POLYGON_ZM polygon_textzm */ yytestcase(yyruleno==64);
{ yygotominor.yy0 = vanuatu_buildGeomFromPolygon( p_data, (gaiaPolygonPtr)yymsp[0].minor.yy0); }
        break;
      case 65: /* polygon_text ::= VANUATU_OPEN_BRACKET ring extra_rings VANUATU_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_polygon_xy( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 66: /* polygon_textm ::= VANUATU_OPEN_BRACKET ringm extra_ringsm VANUATU_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_polygon_xym( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 67: /* polygon_textz ::= VANUATU_OPEN_BRACKET ringz extra_ringsz VANUATU_CLOSE_BRACKET */
{  
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_polygon_xyz( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 68: /* polygon_textzm ::= VANUATU_OPEN_BRACKET ringzm extra_ringszm VANUATU_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_polygon_xyzm( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 69: /* ring ::= VANUATU_OPEN_BRACKET point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy VANUATU_COMMA point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_ring_xy( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 71: /* extra_rings ::= VANUATU_COMMA ring extra_rings */
      case 74: /* extra_ringsm ::= VANUATU_COMMA ringm extra_ringsm */ yytestcase(yyruleno==74);
      case 77: /* extra_ringsz ::= VANUATU_COMMA ringz extra_ringsz */ yytestcase(yyruleno==77);
      case 80: /* extra_ringszm ::= VANUATU_COMMA ringzm extra_ringszm */ yytestcase(yyruleno==80);
{
		((gaiaRingPtr)yymsp[-1].minor.yy0)->Next = (gaiaRingPtr)yymsp[0].minor.yy0;
		yygotominor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 72: /* ringm ::= VANUATU_OPEN_BRACKET point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym VANUATU_COMMA point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_ring_xym( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 75: /* ringz ::= VANUATU_OPEN_BRACKET point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz VANUATU_COMMA point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_ring_xyz( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 78: /* ringzm ::= VANUATU_OPEN_BRACKET point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm VANUATU_COMMA point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_ring_xyzm( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 85: /* multipoint_text ::= VANUATU_OPEN_BRACKET point_coordxy extra_pointsxy VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multipoint_xy( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 86: /* multipoint_textm ::= VANUATU_OPEN_BRACKET point_coordxym extra_pointsxym VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multipoint_xym( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 87: /* multipoint_textz ::= VANUATU_OPEN_BRACKET point_coordxyz extra_pointsxyz VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multipoint_xyz( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 88: /* multipoint_textzm ::= VANUATU_OPEN_BRACKET point_coordxyzm extra_pointsxyzm VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multipoint_xyzm( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 93: /* multilinestring_text ::= VANUATU_OPEN_BRACKET linestring_text multilinestring_text2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multilinestring_xy( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 95: /* multilinestring_text2 ::= VANUATU_COMMA linestring_text multilinestring_text2 */
      case 98: /* multilinestring_textm2 ::= VANUATU_COMMA linestring_textm multilinestring_textm2 */ yytestcase(yyruleno==98);
      case 101: /* multilinestring_textz2 ::= VANUATU_COMMA linestring_textz multilinestring_textz2 */ yytestcase(yyruleno==101);
      case 104: /* multilinestring_textzm2 ::= VANUATU_COMMA linestring_textzm multilinestring_textzm2 */ yytestcase(yyruleno==104);
{ ((gaiaLinestringPtr)yymsp[-1].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 96: /* multilinestring_textm ::= VANUATU_OPEN_BRACKET linestring_textm multilinestring_textm2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multilinestring_xym( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 99: /* multilinestring_textz ::= VANUATU_OPEN_BRACKET linestring_textz multilinestring_textz2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multilinestring_xyz( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 102: /* multilinestring_textzm ::= VANUATU_OPEN_BRACKET linestring_textzm multilinestring_textzm2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multilinestring_xyzm( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 109: /* multipolygon_text ::= VANUATU_OPEN_BRACKET polygon_text multipolygon_text2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multipolygon_xy( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 111: /* multipolygon_text2 ::= VANUATU_COMMA polygon_text multipolygon_text2 */
      case 114: /* multipolygon_textm2 ::= VANUATU_COMMA polygon_textm multipolygon_textm2 */ yytestcase(yyruleno==114);
      case 117: /* multipolygon_textz2 ::= VANUATU_COMMA polygon_textz multipolygon_textz2 */ yytestcase(yyruleno==117);
      case 120: /* multipolygon_textzm2 ::= VANUATU_COMMA polygon_textzm multipolygon_textzm2 */ yytestcase(yyruleno==120);
{ ((gaiaPolygonPtr)yymsp[-1].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 112: /* multipolygon_textm ::= VANUATU_OPEN_BRACKET polygon_textm multipolygon_textm2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multipolygon_xym( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 115: /* multipolygon_textz ::= VANUATU_OPEN_BRACKET polygon_textz multipolygon_textz2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multipolygon_xyz( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 118: /* multipolygon_textzm ::= VANUATU_OPEN_BRACKET polygon_textzm multipolygon_textzm2 VANUATU_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) vanuatu_multipolygon_xyzm( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 125: /* geocoll_text ::= VANUATU_OPEN_BRACKET point geocoll_text2 VANUATU_CLOSE_BRACKET */
      case 126: /* geocoll_text ::= VANUATU_OPEN_BRACKET linestring geocoll_text2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==126);
      case 127: /* geocoll_text ::= VANUATU_OPEN_BRACKET polygon geocoll_text2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==127);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_geomColl_xy( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 129: /* geocoll_text2 ::= VANUATU_COMMA point geocoll_text2 */
      case 130: /* geocoll_text2 ::= VANUATU_COMMA linestring geocoll_text2 */ yytestcase(yyruleno==130);
      case 131: /* geocoll_text2 ::= VANUATU_COMMA polygon geocoll_text2 */ yytestcase(yyruleno==131);
      case 136: /* geocoll_textm2 ::= VANUATU_COMMA pointm geocoll_textm2 */ yytestcase(yyruleno==136);
      case 137: /* geocoll_textm2 ::= VANUATU_COMMA linestringm geocoll_textm2 */ yytestcase(yyruleno==137);
      case 138: /* geocoll_textm2 ::= VANUATU_COMMA polygonm geocoll_textm2 */ yytestcase(yyruleno==138);
      case 143: /* geocoll_textz2 ::= VANUATU_COMMA pointz geocoll_textz2 */ yytestcase(yyruleno==143);
      case 144: /* geocoll_textz2 ::= VANUATU_COMMA linestringz geocoll_textz2 */ yytestcase(yyruleno==144);
      case 145: /* geocoll_textz2 ::= VANUATU_COMMA polygonz geocoll_textz2 */ yytestcase(yyruleno==145);
      case 150: /* geocoll_textzm2 ::= VANUATU_COMMA pointzm geocoll_textzm2 */ yytestcase(yyruleno==150);
      case 151: /* geocoll_textzm2 ::= VANUATU_COMMA linestringzm geocoll_textzm2 */ yytestcase(yyruleno==151);
      case 152: /* geocoll_textzm2 ::= VANUATU_COMMA polygonzm geocoll_textzm2 */ yytestcase(yyruleno==152);
{
		((gaiaGeomCollPtr)yymsp[-1].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[0].minor.yy0;
		yygotominor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 132: /* geocoll_textm ::= VANUATU_OPEN_BRACKET pointm geocoll_textm2 VANUATU_CLOSE_BRACKET */
      case 133: /* geocoll_textm ::= VANUATU_OPEN_BRACKET linestringm geocoll_textm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==133);
      case 134: /* geocoll_textm ::= VANUATU_OPEN_BRACKET polygonm geocoll_textm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==134);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_geomColl_xym( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 139: /* geocoll_textz ::= VANUATU_OPEN_BRACKET pointz geocoll_textz2 VANUATU_CLOSE_BRACKET */
      case 140: /* geocoll_textz ::= VANUATU_OPEN_BRACKET linestringz geocoll_textz2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==140);
      case 141: /* geocoll_textz ::= VANUATU_OPEN_BRACKET polygonz geocoll_textz2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==141);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_geomColl_xyz( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 146: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET pointzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */
      case 147: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET linestringzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==147);
      case 148: /* geocoll_textzm ::= VANUATU_OPEN_BRACKET polygonzm geocoll_textzm2 VANUATU_CLOSE_BRACKET */ yytestcase(yyruleno==148);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) vanuatu_geomColl_xyzm( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      default:
      /* (0) main ::= in */ yytestcase(yyruleno==0);
      /* (1) in ::= */ yytestcase(yyruleno==1);
      /* (2) in ::= in state VANUATU_NEWLINE */ yytestcase(yyruleno==2);
      /* (3) state ::= program */ yytestcase(yyruleno==3);
      /* (4) program ::= geo_text */ yytestcase(yyruleno==4);
      /* (5) program ::= geo_textz */ yytestcase(yyruleno==5);
      /* (6) program ::= geo_textm */ yytestcase(yyruleno==6);
      /* (7) program ::= geo_textzm */ yytestcase(yyruleno==7);
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact < YYNSTATE ){
#ifdef NDEBUG
    /* If we are not debugging and the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yypParser->yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
      yymsp->minor = yygotominor;
    }else
#endif
    {
      yy_shift(yypParser,yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YYNSTATE + YYNRULE + 1 );
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE yyminor            /* The minor type of the error token */
){
  ParseARG_FETCH;
#define TOKEN (yyminor.yy0)

/* 
** Sandro Furieri 2010 Apr 4
** when the LEMON parser encounters an error
** then this global variable is set 
*/
	p_data->vanuatu_parse_error = 1;
	p_data->result = NULL;
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  ParseARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  ParseARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void Parse(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE yyminor       /* The value for the token */
  ParseARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(yypParser, &yyminorunion);
      return;
    }
#endif
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  ParseARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput %s\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact<YYNSTATE ){
      assert( !yyendofinput );  /* Impossible to shift the $ token */
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      yymajor = YYNOCODE;
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor,yyminorunion);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor,yyminorunion);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
