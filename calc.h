/* トークンのタイプ */
typedef enum {
	INTEGER,      /* 整数		        00	*/
	DOUBLE,        /* 少数	        01	*/
	ADD,          /* 加算演算子 +	        02	*/
	SUB,          /* 減算演算子 -           03      */
	MUL,          /* 乗算演算子 *           04      */
	DIV,          /* 除算演算子 /           05      */
	REST,         /* 余り %                 06      */
	RBL,          /* 丸括弧 (               07     */
	RBR,          /* 丸括弧 )               08     */
	GCD,          /* 最大公約数             09     */
	LCM,          /* 最小公倍数             10   */
	EXP,          /* 指数 exp()             11     */
	LOG,          /* 常用対数 log           12*/
	LN,           /* 自然対数               13 */
	SIN,          /* 正弦関数 sin           14   */
	COS,          /* 余弦関数 cos           15   */
	TAN,          /* 傾き     tan           16 */
	SQRT,         /* 平方根                 17 */
	PI,           /* 円周率                 18 */
	E,            /* 常用対数の底           19 */
	LAST,         /* 直前の解               20 */
	FAC,          /* 階上 !                 21 */
	POW,          /* 累乗                   22 */
	ERROR,        /* エラー対処             23     */
	INIT,         /*空読み　24　*/
	DOLLAR,       /* '$' 25*/
} TokenType;

/* 状態のタイプ */
typedef enum {
  Init,		/* 初期状態			00*/
  Int,		/* 整数状態			01*/
  Double,	/* 少数状態			02*/
  Zero,		/* 整数0状態		03*/
  Symbol,	/* 符号状態			04*/
  Alpha,	/* 英単語状態		05*/
  Final,	/* 終了状態			06*/
  Error,	/* エラー状態	07*/
  Error2,   /* .におけるエラー状態  08*/
} StateType;

/* 文字のタイプ */
typedef enum {
  zero,         /* 数字 0                     00 */
  number,	    /* 数字 1~9				      01*/
  dot,          /* 点 .                       02*/
  others,       /* その他 ()+-* /%!           03*/
  alpha,        /* 英字 a～z, A～Z            04*/
  delim,	    /* 区切り記号(空白,TAB,改行)  05*/
  error,        /* 未定義文字                 06*/
} CharType;

typedef enum {
	ot_LPar, /* （        00*/
	ot_RPar, /* ）        01*/
	ot_Fac, /*  ! 02 */
	//ot_PlusMinus1, /* +2 -3 03*/
	ot_Pow, /*  ^ 04*/
	ot_Other, /*  exp ln log sin cos tan sqrt 05*/
	ot_MultDiv, /*  * / % gcd lcm  06*/
	ot_PlusMinus2, /* 3+2 5-3 07*/
	ot_Dara, /* $ 08*/
} OpeType;
