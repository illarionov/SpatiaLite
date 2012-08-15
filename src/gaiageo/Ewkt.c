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
#define YYNOCODE 109
#define YYACTIONTYPE unsigned short int
#define ParseTOKENTYPE void *
typedef union {
  int yyinit;
  ParseTOKENTYPE yy0;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 1000000
#endif
#define ParseARG_SDECL  struct ewkt_data *p_data ;
#define ParseARG_PDECL , struct ewkt_data *p_data 
#define ParseARG_FETCH  struct ewkt_data *p_data  = yypParser->p_data 
#define ParseARG_STORE yypParser->p_data  = p_data 
#define YYNSTATE 350
#define YYNRULE 151
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
 /*     0 */   159,  222,  223,  224,  225,  226,  227,  228,  229,  230,
 /*    10 */   231,  232,  233,  234,  235,  236,  237,  238,  239,  240,
 /*    20 */   241,  242,  243,  244,  245,  246,  247,  248,  249,  250,
 /*    30 */   251,  252,  350,  257,  160,  161,  162,  164,  163,   54,
 /*    40 */    12,   71,   13,   86,   15,   95,   16,  106,   18,  123,
 /*    50 */    20,  152,  128,  136,  144,  134,  142,  150,  135,  143,
 /*    60 */   151,  166,  168,    4,  170,   54,  175,  180,   55,  185,
 /*    70 */    54,   92,   93,  160,   94,   54,  207,  131,   14,   12,
 /*    80 */   132,   13,  129,  133,  130,  211,  139,   64,  215,  140,
 /*    90 */    62,  137,  141,  138,  145,  147,  146,   56,  148,  164,
 /*   100 */    74,  149,    5,   71,   78,   86,   82,  153,  157,  158,
 /*   110 */   154,  155,  156,  260,    6,  261,  262,  502,    1,  274,
 /*   120 */    60,  275,  276,  290,   59,  291,  292,  165,  298,   57,
 /*   130 */   299,  300,   97,   65,  100,  103,   62,  310,  256,  311,
 /*   140 */   312,  111,  167,  115,  119,  264,  322,   61,  323,  324,
 /*   150 */   172,   59,   57,   69,   66,   70,   66,   72,   73,   57,
 /*   160 */    57,  175,  176,  177,   76,   59,   59,   59,   59,  180,
 /*   170 */   181,   17,   62,   62,  182,   80,   19,   62,   62,  185,
 /*   180 */    66,  186,   66,  187,   66,   84,   66,  190,  191,   57,
 /*   190 */    57,  192,   90,   57,   57,   96,  166,   57,  168,    2,
 /*   200 */    59,   62,  170,   66,  161,   23,  162,   58,   59,   62,
 /*   210 */   163,   66,  259,  263,   63,  266,   25,   27,   67,  169,
 /*   220 */   267,   68,   28,  171,  269,   30,  173,  272,  271,   75,
 /*   230 */    31,  178,   79,   35,   39,   83,  183,  174,   87,   77,
 /*   240 */   188,   43,  279,   89,  195,   47,  193,  179,   81,  286,
 /*   250 */   194,  196,  282,  197,   98,  184,   85,   88,  285,  189,
 /*   260 */    91,   48,  289,   99,  101,   49,  102,  104,   50,  198,
 /*   270 */   296,  105,  107,  108,  109,  112,  110,  113,   74,  302,
 /*   280 */   114,  117,  116,  199,  304,  118,  200,  307,  306,  120,
 /*   290 */   201,  121,  309,   78,  122,  124,  126,    7,   82,   10,
 /*   300 */     8,  202,  125,  314,  260,    9,  127,  274,   11,  221,
 /*   310 */     3,  203,  316,  253,  261,  204,  254,  319,  255,  275,
 /*   320 */   318,   21,   22,  205,  258,  265,  321,  276,  262,   24,
 /*   330 */   268,   26,  270,  273,   29,  277,  206,   32,  326,   33,
 /*   340 */    34,  278,  280,   36,  281,  327,   37,  328,  208,  209,
 /*   350 */    38,  283,  210,   40,  284,   41,  332,  333,  334,  212,
 /*   360 */   213,   42,  214,  287,   44,   45,  338,   46,  288,  293,
 /*   370 */   339,  340,  343,  216,  294,  295,  297,  217,  218,  301,
 /*   380 */   303,  345,  346,  305,  347,  219,  220,  308,  313,  315,
 /*   390 */   317,  320,  325,   51,  503,  329,  330,  331,  335,   52,
 /*   400 */   503,  336,  337,   53,  503,  503,  341,  342,  344,  503,
 /*   410 */   503,  348,  349,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*    10 */    33,   34,   35,   36,   37,   38,   39,   40,   41,   42,
 /*    20 */    43,   44,   45,   46,   47,   48,   49,   50,   51,   52,
 /*    30 */    53,   54,    0,    6,    2,   55,   56,    5,   58,   59,
 /*    40 */     8,    9,   10,   11,   12,   13,   14,   15,   16,   17,
 /*    50 */    18,   19,   27,   28,   29,   30,   31,   32,   33,   34,
 /*    60 */    35,   55,   56,    3,   58,   59,   55,   56,   59,   58,
 /*    70 */    59,   55,   56,    2,   58,   59,    2,   27,    3,    8,
 /*    80 */    30,   10,    8,   33,   10,    2,   28,   56,    2,   31,
 /*    90 */    59,    8,   34,   10,    8,   29,   10,   59,   32,    5,
 /*   100 */    72,   35,    3,    9,   76,   11,   78,   48,   49,   50,
 /*   110 */    48,   49,   50,   64,    3,   66,   67,   21,   22,   68,
 /*   120 */    55,   70,   71,   80,   59,   82,   83,   57,   84,   59,
 /*   130 */    86,   87,   64,   56,   66,   67,   59,   92,   59,   94,
 /*   140 */    95,   68,   60,   70,   71,   60,  100,   55,  102,  103,
 /*   150 */    57,   59,   59,   58,   59,   58,   59,   57,   57,   59,
 /*   160 */    59,   55,   55,   55,   55,   59,   59,   59,   59,   56,
 /*   170 */    56,    3,   59,   59,   56,   56,    3,   59,   59,   58,
 /*   180 */    59,   58,   59,   58,   59,   58,   59,   57,   57,   59,
 /*   190 */    59,   57,   57,   59,   59,   57,   55,   59,   56,    3,
 /*   200 */    59,   59,   58,   59,   55,    7,   56,   59,   59,   59,
 /*   210 */    58,   59,   59,   59,   59,   59,    7,    7,   59,   62,
 /*   220 */    62,   59,    3,   63,   63,    7,   61,   61,   65,    7,
 /*   230 */     3,   60,    7,    3,    3,    7,   62,   73,    3,   72,
 /*   240 */    63,    3,   73,    7,   62,    3,   61,   77,   76,   69,
 /*   250 */    60,   63,   77,   61,    7,   79,   78,   74,   79,   75,
 /*   260 */    74,    3,   75,   64,    7,    3,   66,    7,    3,   88,
 /*   270 */    81,   67,    3,   65,    7,    7,   65,    3,   72,   88,
 /*   280 */    68,    3,    7,   90,   90,   70,   91,   85,   91,    7,
 /*   290 */    89,    3,   89,   76,   71,    3,    7,    7,   78,    3,
 /*   300 */     7,   96,   69,   96,   64,    7,   69,   68,    7,    1,
 /*   310 */     3,   98,   98,    4,   66,   99,    4,   93,    4,   70,
 /*   320 */    99,    3,    7,   97,    4,    4,   97,   71,   67,    7,
 /*   330 */     4,    7,    4,    4,    7,    4,  104,    7,  104,    7,
 /*   340 */     7,    4,    4,    7,    4,  104,    7,  104,  104,  104,
 /*   350 */     7,    4,  106,    7,    4,    7,  106,  106,  106,  106,
 /*   360 */   106,    7,  107,    4,    7,    7,  107,    7,    4,    4,
 /*   370 */   107,  107,  101,  107,    4,    4,    4,  107,  105,    4,
 /*   380 */     4,  105,  105,    4,  105,  105,  105,    4,    4,    4,
 /*   390 */     4,    4,    4,    3,  108,    4,    4,    4,    4,    3,
 /*   400 */   108,    4,    4,    3,  108,  108,    4,    4,    4,  108,
 /*   410 */   108,    4,    4,
};
#define YY_SHIFT_USE_DFLT (-1)
#define YY_SHIFT_MAX 220
static const short yy_shift_ofst[] = {
 /*     0 */    -1,   32,   71,   27,   27,   27,   27,   74,   83,   86,
 /*    10 */    94,   94,   60,   75,   99,  111,  168,   60,  173,   75,
 /*    20 */   196,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    30 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    40 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    50 */    27,   27,   27,   27,   27,   27,   27,   27,   27,   27,
 /*    60 */   198,  198,   27,   27,  209,  209,   27,   27,   27,  210,
 /*    70 */   210,  219,  218,  218,  222,  227,  198,  222,  225,  230,
 /*    80 */   209,  225,  228,  231,  210,  228,  235,  238,  236,  238,
 /*    90 */   218,  236,  198,  209,  210,  242,  218,  247,  258,  247,
 /*   100 */   257,  262,  257,  260,  265,  260,  269,  219,  267,  219,
 /*   110 */   267,  268,  274,  227,  268,  275,  278,  230,  275,  282,
 /*   120 */   288,  231,  282,  292,  235,  289,  235,  289,  290,  258,
 /*   130 */   274,  290,  290,  290,  290,  290,  293,  262,  278,  293,
 /*   140 */   293,  293,  293,  293,  298,  265,  288,  298,  298,  298,
 /*   150 */   298,  298,  296,  301,  301,  301,  301,  301,  301,  308,
 /*   160 */   307,  309,  312,  314,  318,  320,  315,  321,  322,  326,
 /*   170 */   324,  328,  327,  329,  331,  330,  332,  333,  337,  338,
 /*   180 */   336,  339,  343,  340,  347,  346,  348,  354,  350,  359,
 /*   190 */   357,  358,  360,  364,  365,  370,  371,  372,  375,  376,
 /*   200 */   379,  383,  384,  385,  386,  387,  388,  390,  391,  392,
 /*   210 */   393,  396,  394,  397,  398,  400,  402,  403,  404,  407,
 /*   220 */   408,
};
#define YY_REDUCE_USE_DFLT (-24)
#define YY_REDUCE_MAX 158
static const short yy_reduce_ofst[] = {
 /*     0 */    96,  -23,   25,  -20,    6,   11,   16,   50,   58,   66,
 /*    10 */    59,   62,   49,   51,   28,   43,   44,   68,   45,   73,
 /*    20 */    46,   70,   65,   92,   31,   77,   95,   97,   93,  100,
 /*    30 */   101,  106,  107,  108,  109,  113,  114,  118,  119,  121,
 /*    40 */   123,  125,  127,  130,  131,  134,  135,  138,  141,  142,
 /*    50 */   144,  149,  150,  152,    9,   38,   79,  148,  153,  154,
 /*    60 */    82,   85,  155,  156,  157,  158,  159,  162,   79,  160,
 /*    70 */   161,  163,  165,  166,  164,  167,  171,  169,  170,  172,
 /*    80 */   174,  175,  176,  178,  177,  179,  180,  183,  184,  186,
 /*    90 */   185,  187,  190,  182,  188,  189,  192,  181,  199,  191,
 /*   100 */   193,  200,  194,  195,  204,  197,  202,  208,  201,  211,
 /*   110 */   203,  205,  212,  206,  207,  213,  215,  217,  214,  216,
 /*   120 */   223,  220,  221,  224,  233,  226,  237,  229,  232,  240,
 /*   130 */   239,  234,  241,  243,  244,  245,  246,  248,  249,  250,
 /*   140 */   251,  252,  253,  254,  255,  261,  256,  259,  263,  264,
 /*   150 */   266,  270,  271,  273,  276,  277,  279,  280,  281,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   351,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    10 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    20 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    30 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    40 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*    50 */   501,  501,  501,  501,  501,  388,  390,  501,  501,  501,
 /*    60 */   393,  393,  501,  501,  397,  397,  501,  501,  501,  399,
 /*    70 */   399,  501,  395,  395,  418,  501,  393,  418,  424,  501,
 /*    80 */   397,  424,  427,  501,  399,  427,  501,  501,  421,  501,
 /*    90 */   395,  421,  393,  397,  399,  501,  395,  442,  501,  442,
 /*   100 */   448,  501,  448,  451,  501,  451,  501,  501,  445,  501,
 /*   110 */   445,  458,  501,  501,  458,  464,  501,  501,  464,  467,
 /*   120 */   501,  501,  467,  501,  501,  461,  501,  461,  476,  501,
 /*   130 */   501,  476,  476,  476,  476,  476,  490,  501,  501,  490,
 /*   140 */   490,  490,  490,  490,  497,  501,  501,  497,  497,  497,
 /*   150 */   497,  497,  501,  483,  483,  483,  483,  483,  483,  501,
 /*   160 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   170 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   180 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   190 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   200 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   210 */   501,  501,  501,  501,  501,  501,  501,  501,  501,  501,
 /*   220 */   501,  352,  353,  354,  355,  356,  357,  358,  359,  360,
 /*   230 */   361,  362,  363,  364,  365,  366,  367,  368,  369,  370,
 /*   240 */   371,  372,  373,  374,  375,  376,  377,  378,  379,  380,
 /*   250 */   381,  382,  383,  384,  385,  387,  391,  392,  386,  389,
 /*   260 */   401,  403,  404,  388,  394,  405,  390,  398,  407,  400,
 /*   270 */   408,  402,  396,  406,  409,  411,  412,  413,  417,  419,
 /*   280 */   415,  423,  425,  416,  426,  428,  410,  414,  420,  422,
 /*   290 */   429,  431,  432,  433,  435,  436,  430,  434,  437,  439,
 /*   300 */   440,  441,  443,  447,  449,  450,  452,  438,  444,  446,
 /*   310 */   453,  455,  456,  457,  459,  463,  465,  466,  468,  454,
 /*   320 */   460,  462,  469,  471,  472,  473,  477,  478,  479,  474,
 /*   330 */   475,  487,  491,  492,  493,  488,  489,  494,  498,  499,
 /*   340 */   500,  495,  496,  470,  480,  484,  485,  486,  481,  482,
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
  "$",             "EWKT_NEWLINE",  "EWKT_POINT",    "EWKT_OPEN_BRACKET",
  "EWKT_CLOSE_BRACKET",  "EWKT_POINT_M",  "EWKT_NUM",      "EWKT_COMMA",  
  "EWKT_LINESTRING",  "EWKT_LINESTRING_M",  "EWKT_POLYGON",  "EWKT_POLYGON_M",
  "EWKT_MULTIPOINT",  "EWKT_MULTIPOINT_M",  "EWKT_MULTILINESTRING",  "EWKT_MULTILINESTRING_M",
  "EWKT_MULTIPOLYGON",  "EWKT_MULTIPOLYGON_M",  "EWKT_GEOMETRYCOLLECTION",  "EWKT_GEOMETRYCOLLECTION_M",
  "error",         "main",          "in",            "state",       
  "program",       "geo_text",      "geo_textm",     "point",       
  "pointz",        "pointzm",       "linestring",    "linestringz", 
  "linestringzm",  "polygon",       "polygonz",      "polygonzm",   
  "multipoint",    "multipointz",   "multipointzm",  "multilinestring",
  "multilinestringz",  "multilinestringzm",  "multipolygon",  "multipolygonz",
  "multipolygonzm",  "geocoll",       "geocollz",      "geocollzm",   
  "pointm",        "linestringm",   "polygonm",      "multipointm", 
  "multilinestringm",  "multipolygonm",  "geocollm",      "point_coordxy",
  "point_coordxyz",  "point_coordxym",  "point_coordxyzm",  "coord",       
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
 /*   2 */ "in ::= in state EWKT_NEWLINE",
 /*   3 */ "state ::= program",
 /*   4 */ "program ::= geo_text",
 /*   5 */ "program ::= geo_textm",
 /*   6 */ "geo_text ::= point",
 /*   7 */ "geo_text ::= pointz",
 /*   8 */ "geo_text ::= pointzm",
 /*   9 */ "geo_text ::= linestring",
 /*  10 */ "geo_text ::= linestringz",
 /*  11 */ "geo_text ::= linestringzm",
 /*  12 */ "geo_text ::= polygon",
 /*  13 */ "geo_text ::= polygonz",
 /*  14 */ "geo_text ::= polygonzm",
 /*  15 */ "geo_text ::= multipoint",
 /*  16 */ "geo_text ::= multipointz",
 /*  17 */ "geo_text ::= multipointzm",
 /*  18 */ "geo_text ::= multilinestring",
 /*  19 */ "geo_text ::= multilinestringz",
 /*  20 */ "geo_text ::= multilinestringzm",
 /*  21 */ "geo_text ::= multipolygon",
 /*  22 */ "geo_text ::= multipolygonz",
 /*  23 */ "geo_text ::= multipolygonzm",
 /*  24 */ "geo_text ::= geocoll",
 /*  25 */ "geo_text ::= geocollz",
 /*  26 */ "geo_text ::= geocollzm",
 /*  27 */ "geo_textm ::= pointm",
 /*  28 */ "geo_textm ::= linestringm",
 /*  29 */ "geo_textm ::= polygonm",
 /*  30 */ "geo_textm ::= multipointm",
 /*  31 */ "geo_textm ::= multilinestringm",
 /*  32 */ "geo_textm ::= multipolygonm",
 /*  33 */ "geo_textm ::= geocollm",
 /*  34 */ "point ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxy EWKT_CLOSE_BRACKET",
 /*  35 */ "pointz ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyz EWKT_CLOSE_BRACKET",
 /*  36 */ "pointm ::= EWKT_POINT_M EWKT_OPEN_BRACKET point_coordxym EWKT_CLOSE_BRACKET",
 /*  37 */ "pointzm ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyzm EWKT_CLOSE_BRACKET",
 /*  38 */ "point_coordxy ::= coord coord",
 /*  39 */ "point_coordxym ::= coord coord coord",
 /*  40 */ "point_coordxyz ::= coord coord coord",
 /*  41 */ "point_coordxyzm ::= coord coord coord coord",
 /*  42 */ "coord ::= EWKT_NUM",
 /*  43 */ "extra_pointsxy ::=",
 /*  44 */ "extra_pointsxy ::= EWKT_COMMA point_coordxy extra_pointsxy",
 /*  45 */ "extra_pointsxym ::=",
 /*  46 */ "extra_pointsxym ::= EWKT_COMMA point_coordxym extra_pointsxym",
 /*  47 */ "extra_pointsxyz ::=",
 /*  48 */ "extra_pointsxyz ::= EWKT_COMMA point_coordxyz extra_pointsxyz",
 /*  49 */ "extra_pointsxyzm ::=",
 /*  50 */ "extra_pointsxyzm ::= EWKT_COMMA point_coordxyzm extra_pointsxyzm",
 /*  51 */ "linestring ::= EWKT_LINESTRING linestring_text",
 /*  52 */ "linestringm ::= EWKT_LINESTRING_M linestring_textm",
 /*  53 */ "linestringz ::= EWKT_LINESTRING linestring_textz",
 /*  54 */ "linestringzm ::= EWKT_LINESTRING linestring_textzm",
 /*  55 */ "linestring_text ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET",
 /*  56 */ "linestring_textm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET",
 /*  57 */ "linestring_textz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET",
 /*  58 */ "linestring_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  59 */ "polygon ::= EWKT_POLYGON polygon_text",
 /*  60 */ "polygonm ::= EWKT_POLYGON_M polygon_textm",
 /*  61 */ "polygonz ::= EWKT_POLYGON polygon_textz",
 /*  62 */ "polygonzm ::= EWKT_POLYGON polygon_textzm",
 /*  63 */ "polygon_text ::= EWKT_OPEN_BRACKET ring extra_rings EWKT_CLOSE_BRACKET",
 /*  64 */ "polygon_textm ::= EWKT_OPEN_BRACKET ringm extra_ringsm EWKT_CLOSE_BRACKET",
 /*  65 */ "polygon_textz ::= EWKT_OPEN_BRACKET ringz extra_ringsz EWKT_CLOSE_BRACKET",
 /*  66 */ "polygon_textzm ::= EWKT_OPEN_BRACKET ringzm extra_ringszm EWKT_CLOSE_BRACKET",
 /*  67 */ "ring ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET",
 /*  68 */ "extra_rings ::=",
 /*  69 */ "extra_rings ::= EWKT_COMMA ring extra_rings",
 /*  70 */ "ringm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET",
 /*  71 */ "extra_ringsm ::=",
 /*  72 */ "extra_ringsm ::= EWKT_COMMA ringm extra_ringsm",
 /*  73 */ "ringz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET",
 /*  74 */ "extra_ringsz ::=",
 /*  75 */ "extra_ringsz ::= EWKT_COMMA ringz extra_ringsz",
 /*  76 */ "ringzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  77 */ "extra_ringszm ::=",
 /*  78 */ "extra_ringszm ::= EWKT_COMMA ringzm extra_ringszm",
 /*  79 */ "multipoint ::= EWKT_MULTIPOINT multipoint_text",
 /*  80 */ "multipointm ::= EWKT_MULTIPOINT_M multipoint_textm",
 /*  81 */ "multipointz ::= EWKT_MULTIPOINT multipoint_textz",
 /*  82 */ "multipointzm ::= EWKT_MULTIPOINT multipoint_textzm",
 /*  83 */ "multipoint_text ::= EWKT_OPEN_BRACKET point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET",
 /*  84 */ "multipoint_textm ::= EWKT_OPEN_BRACKET point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET",
 /*  85 */ "multipoint_textz ::= EWKT_OPEN_BRACKET point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET",
 /*  86 */ "multipoint_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET",
 /*  87 */ "multilinestring ::= EWKT_MULTILINESTRING multilinestring_text",
 /*  88 */ "multilinestringm ::= EWKT_MULTILINESTRING_M multilinestring_textm",
 /*  89 */ "multilinestringz ::= EWKT_MULTILINESTRING multilinestring_textz",
 /*  90 */ "multilinestringzm ::= EWKT_MULTILINESTRING multilinestring_textzm",
 /*  91 */ "multilinestring_text ::= EWKT_OPEN_BRACKET linestring_text multilinestring_text2 EWKT_CLOSE_BRACKET",
 /*  92 */ "multilinestring_text2 ::=",
 /*  93 */ "multilinestring_text2 ::= EWKT_COMMA linestring_text multilinestring_text2",
 /*  94 */ "multilinestring_textm ::= EWKT_OPEN_BRACKET linestring_textm multilinestring_textm2 EWKT_CLOSE_BRACKET",
 /*  95 */ "multilinestring_textm2 ::=",
 /*  96 */ "multilinestring_textm2 ::= EWKT_COMMA linestring_textm multilinestring_textm2",
 /*  97 */ "multilinestring_textz ::= EWKT_OPEN_BRACKET linestring_textz multilinestring_textz2 EWKT_CLOSE_BRACKET",
 /*  98 */ "multilinestring_textz2 ::=",
 /*  99 */ "multilinestring_textz2 ::= EWKT_COMMA linestring_textz multilinestring_textz2",
 /* 100 */ "multilinestring_textzm ::= EWKT_OPEN_BRACKET linestring_textzm multilinestring_textzm2 EWKT_CLOSE_BRACKET",
 /* 101 */ "multilinestring_textzm2 ::=",
 /* 102 */ "multilinestring_textzm2 ::= EWKT_COMMA linestring_textzm multilinestring_textzm2",
 /* 103 */ "multipolygon ::= EWKT_MULTIPOLYGON multipolygon_text",
 /* 104 */ "multipolygonm ::= EWKT_MULTIPOLYGON_M multipolygon_textm",
 /* 105 */ "multipolygonz ::= EWKT_MULTIPOLYGON multipolygon_textz",
 /* 106 */ "multipolygonzm ::= EWKT_MULTIPOLYGON multipolygon_textzm",
 /* 107 */ "multipolygon_text ::= EWKT_OPEN_BRACKET polygon_text multipolygon_text2 EWKT_CLOSE_BRACKET",
 /* 108 */ "multipolygon_text2 ::=",
 /* 109 */ "multipolygon_text2 ::= EWKT_COMMA polygon_text multipolygon_text2",
 /* 110 */ "multipolygon_textm ::= EWKT_OPEN_BRACKET polygon_textm multipolygon_textm2 EWKT_CLOSE_BRACKET",
 /* 111 */ "multipolygon_textm2 ::=",
 /* 112 */ "multipolygon_textm2 ::= EWKT_COMMA polygon_textm multipolygon_textm2",
 /* 113 */ "multipolygon_textz ::= EWKT_OPEN_BRACKET polygon_textz multipolygon_textz2 EWKT_CLOSE_BRACKET",
 /* 114 */ "multipolygon_textz2 ::=",
 /* 115 */ "multipolygon_textz2 ::= EWKT_COMMA polygon_textz multipolygon_textz2",
 /* 116 */ "multipolygon_textzm ::= EWKT_OPEN_BRACKET polygon_textzm multipolygon_textzm2 EWKT_CLOSE_BRACKET",
 /* 117 */ "multipolygon_textzm2 ::=",
 /* 118 */ "multipolygon_textzm2 ::= EWKT_COMMA polygon_textzm multipolygon_textzm2",
 /* 119 */ "geocoll ::= EWKT_GEOMETRYCOLLECTION geocoll_text",
 /* 120 */ "geocollm ::= EWKT_GEOMETRYCOLLECTION_M geocoll_textm",
 /* 121 */ "geocollz ::= EWKT_GEOMETRYCOLLECTION geocoll_textz",
 /* 122 */ "geocollzm ::= EWKT_GEOMETRYCOLLECTION geocoll_textzm",
 /* 123 */ "geocoll_text ::= EWKT_OPEN_BRACKET point geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 124 */ "geocoll_text ::= EWKT_OPEN_BRACKET linestring geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 125 */ "geocoll_text ::= EWKT_OPEN_BRACKET polygon geocoll_text2 EWKT_CLOSE_BRACKET",
 /* 126 */ "geocoll_text2 ::=",
 /* 127 */ "geocoll_text2 ::= EWKT_COMMA point geocoll_text2",
 /* 128 */ "geocoll_text2 ::= EWKT_COMMA linestring geocoll_text2",
 /* 129 */ "geocoll_text2 ::= EWKT_COMMA polygon geocoll_text2",
 /* 130 */ "geocoll_textm ::= EWKT_OPEN_BRACKET pointm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 131 */ "geocoll_textm ::= EWKT_OPEN_BRACKET linestringm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 132 */ "geocoll_textm ::= EWKT_OPEN_BRACKET polygonm geocoll_textm2 EWKT_CLOSE_BRACKET",
 /* 133 */ "geocoll_textm2 ::=",
 /* 134 */ "geocoll_textm2 ::= EWKT_COMMA pointm geocoll_textm2",
 /* 135 */ "geocoll_textm2 ::= EWKT_COMMA linestringm geocoll_textm2",
 /* 136 */ "geocoll_textm2 ::= EWKT_COMMA polygonm geocoll_textm2",
 /* 137 */ "geocoll_textz ::= EWKT_OPEN_BRACKET pointz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 138 */ "geocoll_textz ::= EWKT_OPEN_BRACKET linestringz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 139 */ "geocoll_textz ::= EWKT_OPEN_BRACKET polygonz geocoll_textz2 EWKT_CLOSE_BRACKET",
 /* 140 */ "geocoll_textz2 ::=",
 /* 141 */ "geocoll_textz2 ::= EWKT_COMMA pointz geocoll_textz2",
 /* 142 */ "geocoll_textz2 ::= EWKT_COMMA linestringz geocoll_textz2",
 /* 143 */ "geocoll_textz2 ::= EWKT_COMMA polygonz geocoll_textz2",
 /* 144 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET pointzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 145 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET linestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 146 */ "geocoll_textzm ::= EWKT_OPEN_BRACKET polygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET",
 /* 147 */ "geocoll_textzm2 ::=",
 /* 148 */ "geocoll_textzm2 ::= EWKT_COMMA pointzm geocoll_textzm2",
 /* 149 */ "geocoll_textzm2 ::= EWKT_COMMA linestringzm geocoll_textzm2",
 /* 150 */ "geocoll_textzm2 ::= EWKT_COMMA polygonzm geocoll_textzm2",
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

     spatialite_e( "Giving up.  Parser stack overflow\n");
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
  { 21, 1 },
  { 22, 0 },
  { 22, 3 },
  { 23, 1 },
  { 24, 1 },
  { 24, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 25, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 26, 1 },
  { 27, 4 },
  { 28, 4 },
  { 48, 4 },
  { 29, 4 },
  { 55, 2 },
  { 57, 3 },
  { 56, 3 },
  { 58, 4 },
  { 59, 1 },
  { 60, 0 },
  { 60, 3 },
  { 61, 0 },
  { 61, 3 },
  { 62, 0 },
  { 62, 3 },
  { 63, 0 },
  { 63, 3 },
  { 30, 2 },
  { 49, 2 },
  { 31, 2 },
  { 32, 2 },
  { 64, 6 },
  { 65, 6 },
  { 66, 6 },
  { 67, 6 },
  { 33, 2 },
  { 50, 2 },
  { 34, 2 },
  { 35, 2 },
  { 68, 4 },
  { 69, 4 },
  { 70, 4 },
  { 71, 4 },
  { 72, 10 },
  { 73, 0 },
  { 73, 3 },
  { 74, 10 },
  { 75, 0 },
  { 75, 3 },
  { 76, 10 },
  { 77, 0 },
  { 77, 3 },
  { 78, 10 },
  { 79, 0 },
  { 79, 3 },
  { 36, 2 },
  { 51, 2 },
  { 37, 2 },
  { 38, 2 },
  { 80, 4 },
  { 81, 4 },
  { 82, 4 },
  { 83, 4 },
  { 39, 2 },
  { 52, 2 },
  { 40, 2 },
  { 41, 2 },
  { 84, 4 },
  { 88, 0 },
  { 88, 3 },
  { 85, 4 },
  { 89, 0 },
  { 89, 3 },
  { 86, 4 },
  { 90, 0 },
  { 90, 3 },
  { 87, 4 },
  { 91, 0 },
  { 91, 3 },
  { 42, 2 },
  { 53, 2 },
  { 43, 2 },
  { 44, 2 },
  { 92, 4 },
  { 96, 0 },
  { 96, 3 },
  { 93, 4 },
  { 97, 0 },
  { 97, 3 },
  { 94, 4 },
  { 98, 0 },
  { 98, 3 },
  { 95, 4 },
  { 99, 0 },
  { 99, 3 },
  { 45, 2 },
  { 54, 2 },
  { 46, 2 },
  { 47, 2 },
  { 100, 4 },
  { 100, 4 },
  { 100, 4 },
  { 104, 0 },
  { 104, 3 },
  { 104, 3 },
  { 104, 3 },
  { 101, 4 },
  { 101, 4 },
  { 101, 4 },
  { 105, 0 },
  { 105, 3 },
  { 105, 3 },
  { 105, 3 },
  { 102, 4 },
  { 102, 4 },
  { 102, 4 },
  { 106, 0 },
  { 106, 3 },
  { 106, 3 },
  { 106, 3 },
  { 103, 4 },
  { 103, 4 },
  { 103, 4 },
  { 107, 0 },
  { 107, 3 },
  { 107, 3 },
  { 107, 3 },
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
      case 6: /* geo_text ::= point */
      case 7: /* geo_text ::= pointz */ yytestcase(yyruleno==7);
      case 8: /* geo_text ::= pointzm */ yytestcase(yyruleno==8);
      case 9: /* geo_text ::= linestring */ yytestcase(yyruleno==9);
      case 10: /* geo_text ::= linestringz */ yytestcase(yyruleno==10);
      case 11: /* geo_text ::= linestringzm */ yytestcase(yyruleno==11);
      case 12: /* geo_text ::= polygon */ yytestcase(yyruleno==12);
      case 13: /* geo_text ::= polygonz */ yytestcase(yyruleno==13);
      case 14: /* geo_text ::= polygonzm */ yytestcase(yyruleno==14);
      case 15: /* geo_text ::= multipoint */ yytestcase(yyruleno==15);
      case 16: /* geo_text ::= multipointz */ yytestcase(yyruleno==16);
      case 17: /* geo_text ::= multipointzm */ yytestcase(yyruleno==17);
      case 18: /* geo_text ::= multilinestring */ yytestcase(yyruleno==18);
      case 19: /* geo_text ::= multilinestringz */ yytestcase(yyruleno==19);
      case 20: /* geo_text ::= multilinestringzm */ yytestcase(yyruleno==20);
      case 21: /* geo_text ::= multipolygon */ yytestcase(yyruleno==21);
      case 22: /* geo_text ::= multipolygonz */ yytestcase(yyruleno==22);
      case 23: /* geo_text ::= multipolygonzm */ yytestcase(yyruleno==23);
      case 24: /* geo_text ::= geocoll */ yytestcase(yyruleno==24);
      case 25: /* geo_text ::= geocollz */ yytestcase(yyruleno==25);
      case 26: /* geo_text ::= geocollzm */ yytestcase(yyruleno==26);
      case 27: /* geo_textm ::= pointm */ yytestcase(yyruleno==27);
      case 28: /* geo_textm ::= linestringm */ yytestcase(yyruleno==28);
      case 29: /* geo_textm ::= polygonm */ yytestcase(yyruleno==29);
      case 30: /* geo_textm ::= multipointm */ yytestcase(yyruleno==30);
      case 31: /* geo_textm ::= multilinestringm */ yytestcase(yyruleno==31);
      case 32: /* geo_textm ::= multipolygonm */ yytestcase(yyruleno==32);
      case 33: /* geo_textm ::= geocollm */ yytestcase(yyruleno==33);
{ p_data->result = yymsp[0].minor.yy0; }
        break;
      case 34: /* point ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxy EWKT_CLOSE_BRACKET */
      case 35: /* pointz ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyz EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==35);
      case 37: /* pointzm ::= EWKT_POINT EWKT_OPEN_BRACKET point_coordxyzm EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==37);
{ yygotominor.yy0 = ewkt_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0); }
        break;
      case 36: /* pointm ::= EWKT_POINT_M EWKT_OPEN_BRACKET point_coordxym EWKT_CLOSE_BRACKET */
{ yygotominor.yy0 = ewkt_buildGeomFromPoint( p_data, (gaiaPointPtr)yymsp[-1].minor.yy0);  }
        break;
      case 38: /* point_coordxy ::= coord coord */
{ yygotominor.yy0 = (void *) ewkt_point_xy( p_data, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 39: /* point_coordxym ::= coord coord coord */
{ yygotominor.yy0 = (void *) ewkt_point_xym( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 40: /* point_coordxyz ::= coord coord coord */
{ yygotominor.yy0 = (void *) ewkt_point_xyz( p_data, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 41: /* point_coordxyzm ::= coord coord coord coord */
{ yygotominor.yy0 = (void *) ewkt_point_xyzm( p_data, (double *)yymsp[-3].minor.yy0, (double *)yymsp[-2].minor.yy0, (double *)yymsp[-1].minor.yy0, (double *)yymsp[0].minor.yy0); }
        break;
      case 42: /* coord ::= EWKT_NUM */
      case 79: /* multipoint ::= EWKT_MULTIPOINT multipoint_text */ yytestcase(yyruleno==79);
      case 80: /* multipointm ::= EWKT_MULTIPOINT_M multipoint_textm */ yytestcase(yyruleno==80);
      case 81: /* multipointz ::= EWKT_MULTIPOINT multipoint_textz */ yytestcase(yyruleno==81);
      case 82: /* multipointzm ::= EWKT_MULTIPOINT multipoint_textzm */ yytestcase(yyruleno==82);
      case 87: /* multilinestring ::= EWKT_MULTILINESTRING multilinestring_text */ yytestcase(yyruleno==87);
      case 88: /* multilinestringm ::= EWKT_MULTILINESTRING_M multilinestring_textm */ yytestcase(yyruleno==88);
      case 89: /* multilinestringz ::= EWKT_MULTILINESTRING multilinestring_textz */ yytestcase(yyruleno==89);
      case 90: /* multilinestringzm ::= EWKT_MULTILINESTRING multilinestring_textzm */ yytestcase(yyruleno==90);
      case 103: /* multipolygon ::= EWKT_MULTIPOLYGON multipolygon_text */ yytestcase(yyruleno==103);
      case 104: /* multipolygonm ::= EWKT_MULTIPOLYGON_M multipolygon_textm */ yytestcase(yyruleno==104);
      case 105: /* multipolygonz ::= EWKT_MULTIPOLYGON multipolygon_textz */ yytestcase(yyruleno==105);
      case 106: /* multipolygonzm ::= EWKT_MULTIPOLYGON multipolygon_textzm */ yytestcase(yyruleno==106);
      case 119: /* geocoll ::= EWKT_GEOMETRYCOLLECTION geocoll_text */ yytestcase(yyruleno==119);
      case 120: /* geocollm ::= EWKT_GEOMETRYCOLLECTION_M geocoll_textm */ yytestcase(yyruleno==120);
      case 121: /* geocollz ::= EWKT_GEOMETRYCOLLECTION geocoll_textz */ yytestcase(yyruleno==121);
      case 122: /* geocollzm ::= EWKT_GEOMETRYCOLLECTION geocoll_textzm */ yytestcase(yyruleno==122);
{ yygotominor.yy0 = yymsp[0].minor.yy0; }
        break;
      case 43: /* extra_pointsxy ::= */
      case 45: /* extra_pointsxym ::= */ yytestcase(yyruleno==45);
      case 47: /* extra_pointsxyz ::= */ yytestcase(yyruleno==47);
      case 49: /* extra_pointsxyzm ::= */ yytestcase(yyruleno==49);
      case 68: /* extra_rings ::= */ yytestcase(yyruleno==68);
      case 71: /* extra_ringsm ::= */ yytestcase(yyruleno==71);
      case 74: /* extra_ringsz ::= */ yytestcase(yyruleno==74);
      case 77: /* extra_ringszm ::= */ yytestcase(yyruleno==77);
      case 92: /* multilinestring_text2 ::= */ yytestcase(yyruleno==92);
      case 95: /* multilinestring_textm2 ::= */ yytestcase(yyruleno==95);
      case 98: /* multilinestring_textz2 ::= */ yytestcase(yyruleno==98);
      case 101: /* multilinestring_textzm2 ::= */ yytestcase(yyruleno==101);
      case 108: /* multipolygon_text2 ::= */ yytestcase(yyruleno==108);
      case 111: /* multipolygon_textm2 ::= */ yytestcase(yyruleno==111);
      case 114: /* multipolygon_textz2 ::= */ yytestcase(yyruleno==114);
      case 117: /* multipolygon_textzm2 ::= */ yytestcase(yyruleno==117);
      case 126: /* geocoll_text2 ::= */ yytestcase(yyruleno==126);
      case 133: /* geocoll_textm2 ::= */ yytestcase(yyruleno==133);
      case 140: /* geocoll_textz2 ::= */ yytestcase(yyruleno==140);
      case 147: /* geocoll_textzm2 ::= */ yytestcase(yyruleno==147);
{ yygotominor.yy0 = NULL; }
        break;
      case 44: /* extra_pointsxy ::= EWKT_COMMA point_coordxy extra_pointsxy */
      case 46: /* extra_pointsxym ::= EWKT_COMMA point_coordxym extra_pointsxym */ yytestcase(yyruleno==46);
      case 48: /* extra_pointsxyz ::= EWKT_COMMA point_coordxyz extra_pointsxyz */ yytestcase(yyruleno==48);
      case 50: /* extra_pointsxyzm ::= EWKT_COMMA point_coordxyzm extra_pointsxyzm */ yytestcase(yyruleno==50);
{ ((gaiaPointPtr)yymsp[-1].minor.yy0)->Next = (gaiaPointPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 51: /* linestring ::= EWKT_LINESTRING linestring_text */
      case 52: /* linestringm ::= EWKT_LINESTRING_M linestring_textm */ yytestcase(yyruleno==52);
      case 53: /* linestringz ::= EWKT_LINESTRING linestring_textz */ yytestcase(yyruleno==53);
      case 54: /* linestringzm ::= EWKT_LINESTRING linestring_textzm */ yytestcase(yyruleno==54);
{ yygotominor.yy0 = ewkt_buildGeomFromLinestring( p_data, (gaiaLinestringPtr)yymsp[0].minor.yy0); }
        break;
      case 55: /* linestring_text ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) ewkt_linestring_xy( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 56: /* linestring_textm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) ewkt_linestring_xym( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 57: /* linestring_textz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) ewkt_linestring_xyz( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 58: /* linestring_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   ((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0;
	   yygotominor.yy0 = (void *) ewkt_linestring_xyzm( p_data, (gaiaPointPtr)yymsp[-4].minor.yy0);
	}
        break;
      case 59: /* polygon ::= EWKT_POLYGON polygon_text */
      case 60: /* polygonm ::= EWKT_POLYGON_M polygon_textm */ yytestcase(yyruleno==60);
      case 61: /* polygonz ::= EWKT_POLYGON polygon_textz */ yytestcase(yyruleno==61);
      case 62: /* polygonzm ::= EWKT_POLYGON polygon_textzm */ yytestcase(yyruleno==62);
{ yygotominor.yy0 = ewkt_buildGeomFromPolygon( p_data, (gaiaPolygonPtr)yymsp[0].minor.yy0); }
        break;
      case 63: /* polygon_text ::= EWKT_OPEN_BRACKET ring extra_rings EWKT_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_polygon_xy( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 64: /* polygon_textm ::= EWKT_OPEN_BRACKET ringm extra_ringsm EWKT_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_polygon_xym( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 65: /* polygon_textz ::= EWKT_OPEN_BRACKET ringz extra_ringsz EWKT_CLOSE_BRACKET */
{  
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_polygon_xyz( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 66: /* polygon_textzm ::= EWKT_OPEN_BRACKET ringzm extra_ringszm EWKT_CLOSE_BRACKET */
{ 
		((gaiaRingPtr)yymsp[-2].minor.yy0)->Next = (gaiaRingPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_polygon_xyzm( p_data, (gaiaRingPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 67: /* ring ::= EWKT_OPEN_BRACKET point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy EWKT_COMMA point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_ring_xy( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 69: /* extra_rings ::= EWKT_COMMA ring extra_rings */
      case 72: /* extra_ringsm ::= EWKT_COMMA ringm extra_ringsm */ yytestcase(yyruleno==72);
      case 75: /* extra_ringsz ::= EWKT_COMMA ringz extra_ringsz */ yytestcase(yyruleno==75);
      case 78: /* extra_ringszm ::= EWKT_COMMA ringzm extra_ringszm */ yytestcase(yyruleno==78);
{
		((gaiaRingPtr)yymsp[-1].minor.yy0)->Next = (gaiaRingPtr)yymsp[0].minor.yy0;
		yygotominor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 70: /* ringm ::= EWKT_OPEN_BRACKET point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym EWKT_COMMA point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_ring_xym( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 73: /* ringz ::= EWKT_OPEN_BRACKET point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz EWKT_COMMA point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_ring_xyz( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 76: /* ringzm ::= EWKT_OPEN_BRACKET point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm EWKT_COMMA point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
{
		((gaiaPointPtr)yymsp[-8].minor.yy0)->Next = (gaiaPointPtr)yymsp[-6].minor.yy0; 
		((gaiaPointPtr)yymsp[-6].minor.yy0)->Next = (gaiaPointPtr)yymsp[-4].minor.yy0;
		((gaiaPointPtr)yymsp[-4].minor.yy0)->Next = (gaiaPointPtr)yymsp[-2].minor.yy0; 
		((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_ring_xyzm( p_data, (gaiaPointPtr)yymsp[-8].minor.yy0);
	}
        break;
      case 83: /* multipoint_text ::= EWKT_OPEN_BRACKET point_coordxy extra_pointsxy EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipoint_xy( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 84: /* multipoint_textm ::= EWKT_OPEN_BRACKET point_coordxym extra_pointsxym EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipoint_xym( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 85: /* multipoint_textz ::= EWKT_OPEN_BRACKET point_coordxyz extra_pointsxyz EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipoint_xyz( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 86: /* multipoint_textzm ::= EWKT_OPEN_BRACKET point_coordxyzm extra_pointsxyzm EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPointPtr)yymsp[-2].minor.yy0)->Next = (gaiaPointPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipoint_xyzm( p_data, (gaiaPointPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 91: /* multilinestring_text ::= EWKT_OPEN_BRACKET linestring_text multilinestring_text2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multilinestring_xy( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 93: /* multilinestring_text2 ::= EWKT_COMMA linestring_text multilinestring_text2 */
      case 96: /* multilinestring_textm2 ::= EWKT_COMMA linestring_textm multilinestring_textm2 */ yytestcase(yyruleno==96);
      case 99: /* multilinestring_textz2 ::= EWKT_COMMA linestring_textz multilinestring_textz2 */ yytestcase(yyruleno==99);
      case 102: /* multilinestring_textzm2 ::= EWKT_COMMA linestring_textzm multilinestring_textzm2 */ yytestcase(yyruleno==102);
{ ((gaiaLinestringPtr)yymsp[-1].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 94: /* multilinestring_textm ::= EWKT_OPEN_BRACKET linestring_textm multilinestring_textm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multilinestring_xym( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 97: /* multilinestring_textz ::= EWKT_OPEN_BRACKET linestring_textz multilinestring_textz2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multilinestring_xyz( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 100: /* multilinestring_textzm ::= EWKT_OPEN_BRACKET linestring_textzm multilinestring_textzm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaLinestringPtr)yymsp[-2].minor.yy0)->Next = (gaiaLinestringPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multilinestring_xyzm( p_data, (gaiaLinestringPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 107: /* multipolygon_text ::= EWKT_OPEN_BRACKET polygon_text multipolygon_text2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipolygon_xy( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 109: /* multipolygon_text2 ::= EWKT_COMMA polygon_text multipolygon_text2 */
      case 112: /* multipolygon_textm2 ::= EWKT_COMMA polygon_textm multipolygon_textm2 */ yytestcase(yyruleno==112);
      case 115: /* multipolygon_textz2 ::= EWKT_COMMA polygon_textz multipolygon_textz2 */ yytestcase(yyruleno==115);
      case 118: /* multipolygon_textzm2 ::= EWKT_COMMA polygon_textzm multipolygon_textzm2 */ yytestcase(yyruleno==118);
{ ((gaiaPolygonPtr)yymsp[-1].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[0].minor.yy0;  yygotominor.yy0 = yymsp[-1].minor.yy0; }
        break;
      case 110: /* multipolygon_textm ::= EWKT_OPEN_BRACKET polygon_textm multipolygon_textm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipolygon_xym( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 113: /* multipolygon_textz ::= EWKT_OPEN_BRACKET polygon_textz multipolygon_textz2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipolygon_xyz( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 116: /* multipolygon_textzm ::= EWKT_OPEN_BRACKET polygon_textzm multipolygon_textzm2 EWKT_CLOSE_BRACKET */
{ 
	   ((gaiaPolygonPtr)yymsp[-2].minor.yy0)->Next = (gaiaPolygonPtr)yymsp[-1].minor.yy0; 
	   yygotominor.yy0 = (void *) ewkt_multipolygon_xyzm( p_data, (gaiaPolygonPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 123: /* geocoll_text ::= EWKT_OPEN_BRACKET point geocoll_text2 EWKT_CLOSE_BRACKET */
      case 124: /* geocoll_text ::= EWKT_OPEN_BRACKET linestring geocoll_text2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==124);
      case 125: /* geocoll_text ::= EWKT_OPEN_BRACKET polygon geocoll_text2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==125);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_geomColl_xy( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 127: /* geocoll_text2 ::= EWKT_COMMA point geocoll_text2 */
      case 128: /* geocoll_text2 ::= EWKT_COMMA linestring geocoll_text2 */ yytestcase(yyruleno==128);
      case 129: /* geocoll_text2 ::= EWKT_COMMA polygon geocoll_text2 */ yytestcase(yyruleno==129);
      case 134: /* geocoll_textm2 ::= EWKT_COMMA pointm geocoll_textm2 */ yytestcase(yyruleno==134);
      case 135: /* geocoll_textm2 ::= EWKT_COMMA linestringm geocoll_textm2 */ yytestcase(yyruleno==135);
      case 136: /* geocoll_textm2 ::= EWKT_COMMA polygonm geocoll_textm2 */ yytestcase(yyruleno==136);
      case 141: /* geocoll_textz2 ::= EWKT_COMMA pointz geocoll_textz2 */ yytestcase(yyruleno==141);
      case 142: /* geocoll_textz2 ::= EWKT_COMMA linestringz geocoll_textz2 */ yytestcase(yyruleno==142);
      case 143: /* geocoll_textz2 ::= EWKT_COMMA polygonz geocoll_textz2 */ yytestcase(yyruleno==143);
      case 148: /* geocoll_textzm2 ::= EWKT_COMMA pointzm geocoll_textzm2 */ yytestcase(yyruleno==148);
      case 149: /* geocoll_textzm2 ::= EWKT_COMMA linestringzm geocoll_textzm2 */ yytestcase(yyruleno==149);
      case 150: /* geocoll_textzm2 ::= EWKT_COMMA polygonzm geocoll_textzm2 */ yytestcase(yyruleno==150);
{
		((gaiaGeomCollPtr)yymsp[-1].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[0].minor.yy0;
		yygotominor.yy0 = yymsp[-1].minor.yy0;
	}
        break;
      case 130: /* geocoll_textm ::= EWKT_OPEN_BRACKET pointm geocoll_textm2 EWKT_CLOSE_BRACKET */
      case 131: /* geocoll_textm ::= EWKT_OPEN_BRACKET linestringm geocoll_textm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==131);
      case 132: /* geocoll_textm ::= EWKT_OPEN_BRACKET polygonm geocoll_textm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==132);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_geomColl_xym( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 137: /* geocoll_textz ::= EWKT_OPEN_BRACKET pointz geocoll_textz2 EWKT_CLOSE_BRACKET */
      case 138: /* geocoll_textz ::= EWKT_OPEN_BRACKET linestringz geocoll_textz2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==138);
      case 139: /* geocoll_textz ::= EWKT_OPEN_BRACKET polygonz geocoll_textz2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==139);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_geomColl_xyz( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      case 144: /* geocoll_textzm ::= EWKT_OPEN_BRACKET pointzm geocoll_textzm2 EWKT_CLOSE_BRACKET */
      case 145: /* geocoll_textzm ::= EWKT_OPEN_BRACKET linestringzm geocoll_textzm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==145);
      case 146: /* geocoll_textzm ::= EWKT_OPEN_BRACKET polygonzm geocoll_textzm2 EWKT_CLOSE_BRACKET */ yytestcase(yyruleno==146);
{ 
		((gaiaGeomCollPtr)yymsp[-2].minor.yy0)->Next = (gaiaGeomCollPtr)yymsp[-1].minor.yy0;
		yygotominor.yy0 = (void *) ewkt_geomColl_xyzm( p_data, (gaiaGeomCollPtr)yymsp[-2].minor.yy0);
	}
        break;
      default:
      /* (0) main ::= in */ yytestcase(yyruleno==0);
      /* (1) in ::= */ yytestcase(yyruleno==1);
      /* (2) in ::= in state EWKT_NEWLINE */ yytestcase(yyruleno==2);
      /* (3) state ::= program */ yytestcase(yyruleno==3);
      /* (4) program ::= geo_text */ yytestcase(yyruleno==4);
      /* (5) program ::= geo_textm */ yytestcase(yyruleno==5);
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
** when the LEMON parser encounters an error
** then this global variable is set 
*/
	p_data->ewkt_parse_error = 1;
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
