/**************************************************
*	calc.c
*	April  25  2016
*
*	Toshiki Okazaki (@kleecider)
*
*	Copyright (c) 2016, Toshiki Okazaki, Okayama University
*	All Rights Reserved.
*
***************************************************/

#include <stdio.h>
#include <unistd.h> //write(), close()
#include <fcntl.h> // O_RDONLY for open()

#define D() //printf("LINE: %d\n",__LINE__)
#define MAX_LINE_LEN 10000   //入力最大文字数
#define TOKENMAX 100        //トークンの最大文字数
#define TOKENnum 10000       //作成可能なトークンの数
#define NODEnum 10000       //作成可能なノードの数

int my_strcmp(const char *s1, const char *s2);

/* define.h */
/***************************************************/
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
/********************************************************/

/* lex.c */
//////////////////////////////////////////////////////////
/* 遷移表の定義 */
static StateType table[6][8] = {
	/* 0  1 ~ 9  .  +/-*()!%   a ~ z A ~ Z */
  {Zero,Int,Error2,Symbol,Alpha,Final,Error},/* Init: 初期状態 */
  {Int,Int,Double,Final,Final,Final,Error},/* Int: 整数 */
  {Double,Double,Final,Final,Final,Final,Error},/* Double: 少数 */
  {Final,Final,Double,Final,Final,Final,Error},/* Zero */
  {Final,Final,Final,Final,Final,Final,Error},/* Symbol:符号 */
  {Final,Final,Final,Final,Alpha,Final,Error},/* Alpha:英語 */
};

/* 文字を入力とし,文字の種類を返す関数 */
static CharType charToCharType(int c){
	if( c == '0' ) return zero;
	if( ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ) return alpha;
	if( c == '.') return dot;
	if( c == '%' || c == '!' || c == '(' || c == ')' || c == '^' ||
      c == '+' || c == '-' || c == '*' || c == '/' ) return others;
	if( '1' <= c && c <= '9' ) return number;
	if( c == ' ' || c == '\t' || c == '\n' ||c == EOF || c == '\0') return delim;

  return error;
}

/* 直前の状態と(トークンの)文字列を入力とし,トークンの種類を返す関数 */
static TokenType whichTokenType(char *s, StateType state){
	if(state == Init) return INIT;
	if(state == Int) return INTEGER;
	if(state == Double) return DOUBLE;
	if(state == Zero) return INTEGER;
	if(state == Symbol){
		if(*s == '+') return ADD;
		if(*s == '-') return SUB;
		if(*s == '*') return MUL;
		if(*s == '/') return DIV;
		if(*s == '%') return REST;
		if(*s == '(') return RBL;
		if(*s == ')') return RBR;
		if(*s == '!') return FAC;
		if(*s == '^') return POW;
	}
	if(state == Alpha){
		if(my_strcmp(s,"exp") == 0) return EXP;
		if(my_strcmp(s,"sin") == 0) return SIN;
		if(my_strcmp(s,"cos") == 0) return COS;
		if(my_strcmp(s,"tan") == 0) return TAN;
		if(my_strcmp(s,"sqrt") == 0) return SQRT;
		if(my_strcmp(s,"pi") == 0) return PI;
		if(my_strcmp(s,"e") == 0) return E;
		if(my_strcmp(s,"last") == 0) return LAST;
	}

	return ERROR;
}

//////////////////////////////////////////////////////////

/* oparser.c */
///////////////////////////////////
/* 演算子順位行列 */
static int orderMatrix[8][9] = {
/* ( ,), !, ^, etc, *%/, +-, $ */
{-1,0,-1,-1,-1,-1,-1,9}, /*  (  */ //>:1 <:-1 =:0 error:9 end:5
{9,1,1,1,1,1,1,1},/*  *)  */
{-1,1,1,1,1,1,1,1},/* ! */
{-1,1,-1,1,1,1,1,1},/* ^ */
{-1,1,-1,-1,1,1,1,1},/* sincostanetc*/
{-1,1,-1,-1,-1,1,1,1},/* *%/ */
{-1,1,-1,-1,-1,-1,1,1},/* +- */
{-1,9,-1,-1,-1,-1,-1,5},/* $ */
};

static OpeType typeToOpeType(TokenType type){
	if(type == ADD || type == SUB) return ot_PlusMinus2;
	if(type == MUL || type == DIV || type == REST ||
		type == GCD || type == LCM ) return ot_MultDiv;
	if(type == RBL) return ot_LPar;
	if(type == RBR) return ot_RPar;
	if(type == FAC) return ot_Fac;
	if(type == POW) return ot_Pow;
	if(type == EXP || type == LOG || type == LN ||
		type == SIN || type == COS || type == TAN ||
		type == SQRT) return ot_Other;

	return ot_Dara;
}
//////////////////////////////////

/* トークンを格納する構造体 */
typedef struct node0{
  char 		string[TOKENMAX];	/* トークンの文字列 */
  TokenType	type;			/* トークンのタイプ */
  int num;   /* このトークンは何番目のトークンなのか */
  int order;       /* トークンの優先順位 */
  //struct node *left;
  int left;
  //struct node *right;
  int right;
} TokenSt;

struct node0 token_store[TOKENnum];     // array of token
int i_for_token=0;         /* 作成したトークン数 */
int Sptr[2];
int node_stack[2][NODEnum];         /* 作業用スタック，0:数字 1:記号 */
int token_n = 0;       /* トークンを指している場所，指している位置から順に木を作成 */
int tree_top;		/* 構文木のトップ */
double last_answer = 0;           /* 前回の答え */
int error_flag = 0;

char *my_strcpy(char *dest, const char *src){
	int i;

	for (i = 0; src[i] != '\0'; i++){
		dest[i] = src[i];
    }
 	dest[i] = '\0';

 	return dest;
}

unsigned int my_strlen(const char *str){
	unsigned int length = 0;
	while(*str++ != '\0'){
		length++;
	}
	return length;
}

int my_strcmp(const char *s1, const char *s2){
	int i=0;

	for(i=0;*(s1 + i) == *(s2 +i);i++){
		if(*(s1+i)=='\0')return 0;
	}
	return *(s1+i)>*(s2+i) ? 1 : -1;
}

/* 文字列 str 中の文字 c1 を文字 c2 に置き換える */
int subst(char *str, char c1, char c2){
    int n=0;

    while(*str){
      if(*str == c1){
	*str = c2;
	n++;
      }
      str++;
    }
    return n;
}

/* 計算式を文字列として扱う */
int lex(char *line){

	//static char	FIFO[TOKENMAX];
	StateType	state, nstate; /* state:今の状態，nstate:次の状態 */
	CharType ct;
	int x=0; //標準入力のポインタ
	int i=0;
	int j=0;
	//int while_flag=0;
	i_for_token=0;

	/* 最初に$のトークンを作る */

	token_store[i_for_token].string[0] = '$';
	token_store[i_for_token].string[1] = '\0';
	token_store[i_for_token].type = DOLLAR;
	token_store[i_for_token].left = -1;
	token_store[i_for_token].right = -1;

	i_for_token++;
	/************************/
	//D();
	while(1){
		if((*(line+x))=='\0') {/*D()*/;break;}
		ct = charToCharType(*(line+x));
		x++;
		//D();
		i=0;
		while(ct == delim){
			//D();
			ct = charToCharType(*(line+x));
			x++;
			if(*(line+x-1)=='\0') {
				return 0;
			}
		}

		//printf("test5:%c\n",*(line+x));
		state = Init;
		nstate = table[state][ct];
		if (nstate == Error){
			printf("Error:undefined word is inputed\n");
			i_for_token=-1;
			return -1;
		}
		if (nstate == Error2){
			printf("Error:syntax error\n");
			i_for_token=-1;
			return -1;
		}

		/* tokenに一字ずつ格納 */
		while( nstate != Final ){
			(token_store[i_for_token]).string[i++] = *(line+x-1);
			ct = charToCharType(*(line+x));
			x++;
			state = nstate;
			nstate = table[state][ct];
			if(nstate == Final){
				(token_store[i_for_token]).string[i] = '\0';
				x--;
				break;
			}
			if (nstate == Error2){
				printf("Error:syntax error\n");
				i_for_token=-1;
				return -1;
			}

			if (nstate == Error){
				printf("Error:undefined word is inputed\n");
				i_for_token=-1;
				return -1;
			}
		}

		(token_store[i_for_token]).type = whichTokenType((token_store[i_for_token]).string, state);
		if((token_store[i_for_token]).type == DOUBLE && (((token_store[i_for_token]).string[my_strlen((token_store[i_for_token]).string)-1]) == '.')){
			printf("Error:syntax error5\n");
			i_for_token=-1;
			return -1;
		}
		(token_store[i_for_token]).num = i_for_token;
		if((token_store[i_for_token]).type == ERROR){
			printf("Error:undefined word is inputed\n");
			i_for_token=-1;
			return -1;
		}

		if((token_store[i_for_token]).type == INTEGER){
			if(my_strlen((token_store[i_for_token]).string) > 9){
				printf("Error:%s is too large or small. input number (< 10^9 or > -10^9)\n",(token_store[i_for_token]).string);
				i_for_token=-1;
				return -1;
			}
		}

		if((token_store[i_for_token]).type == DOUBLE){
			for(j=0;j<18;j++){
				if((token_store[i_for_token]).string[j]=='.')break;
			}

			if(j>9){
				printf("Error:%s is too large or small. input number (< 10^9 or > -10^9)\n",(token_store[i_for_token]).string);
				i_for_token=-1;
				return -1;
			}
			if(my_strlen((token_store[i_for_token]).string)-j>7){
				printf("Error:the decimal part of %s is too long. input the length of decimal part (< 7)\n",(token_store[i_for_token]).string);
				i_for_token=-1;
				return -1;
			}
		}

		i_for_token++;
	}
	//D();
	return 0;
}

int push(int S, int token_n2){

	if(Sptr[S] == NODEnum-1){
		printf("Error:stack overflow");
		return -1;
	}

	node_stack[S][Sptr[S]] = token_n2;
	Sptr[S]++;
	return 0;
}

/* エラーの場合に，負の数を返す */
int pop(int S){
	if(Sptr[S] == 0){
		printf("Error:syntax error\n");
		return -1;
	}

	Sptr[S] = Sptr[S] -1;
	return token_store[node_stack[S][Sptr[S]]].num;

}
/* stack のトップに格納されている token の配列の番目 */
int Top(){
	if ( Sptr[1] == 0 ){
		printf("Error:syntax error\n");
		return -1;
	}
	return token_store[node_stack[1][Sptr[1]-1]].num;
}

int check(){

	int order;
	int top;
	int N=-1;
	int i=0;

	top = Top();
	if(top < 0) return -1;

	token_store[top].order = typeToOpeType(token_store[top].type);
	token_store[token_n].order = typeToOpeType(token_store[token_n].type);
	order = orderMatrix[token_store[top].order][token_store[token_n].order];


	if(order == 5){
		//D();
		return 1;
	}
	else if(order == 9){
		printf("Error:syntax error\n");
		return -1; /* Error */
	}

	else if((order == -1) && (token_store[token_n].type == FAC)){
		D();
		N = pop(0);
		if(N<0) return -1;
		token_store[token_n].right = N;
		for(i=0;i<token_n;i++){
			if(token_store[i].right == N){
				//D();
				token_store[token_n].right = token_store[i].num;
				break;
			}
		}
		if(push(0,token_n)<0) return -1;
		tree_top=token_store[token_n].num;
		token_n++;
		return 0;
	}

	else if((token_store[token_n].type == DOLLAR) && (Sptr[0] == 0)){
		D();
		return 1;
	}
	else if( Sptr[1] == 0 || order == -1){
		if(push(1,token_n)<0) return -1;
		token_n++;
		return 0;
	}
	else if(order == 0){
		//D();
		if(pop(1)<0) return -1;
		if( token_store[node_stack[1][Sptr[1]]-1].type == SIN ||
				token_store[node_stack[1][Sptr[1]]-1].type == COS ||
				token_store[node_stack[1][Sptr[1]]-1].type == TAN ||
				token_store[node_stack[1][Sptr[1]]-1].type == EXP ||
				token_store[node_stack[1][Sptr[1]]-1].type == LOG ||
				token_store[node_stack[1][Sptr[1]]-1].type == LN ||
				token_store[node_stack[1][Sptr[1]]-1].type == SQRT){
					token_store[node_stack[1][Sptr[1]]-1].right = tree_top;
					tree_top = token_store[node_stack[1][Sptr[1]]-1].num;
					if(pop(1)<0)return -1;
				}
		token_n++;
		return 0;
	}
	else{ /* 木構造を作成 */
		//D();
		N = pop(1);
		if(N<0)return -1;
		tree_top = N;
		token_store[N].right = pop(0);
		if(token_store[N].right<0)return -1;
		token_store[N].left = pop(0);
		if(token_store[N].left<0)return -1;
		if(push(0,N)<0)return -1;
		return check();
	}

	printf("error¥n");
	return -1; /* エラーチェック */
}

int oparser(){

	int final;  /* 終了判定用 */

	Sptr[0] = 0;
	Sptr[1] = 0;
	token_n=0;
	tree_top = -1;

	push(1,token_n);
	token_n++;

	/* 終了状態ではない（0） */
	final=0;

	while(final == 0){

		token_store[token_n].left = -1;
		token_store[token_n].right = -1;

		if( token_store[token_n].type == INTEGER || token_store[token_n].type == DOUBLE ||
			token_store[token_n].type == LAST || token_store[token_n].type == E ||
			token_store[token_n].type == PI){
			if(push(0,token_n)<0)return -1;
			tree_top = token_store[token_n].num;
			token_n++;
		}
		else{        /* 符号の処理 */
			final = check();
			if(final<0) return -1;
		}

	}
	if (Sptr[0] > 1){
    	printf("Error:syntax error\n");
    	return -1;
	}
	return 0;
}

/* 入力から１行で文字列配置lineに格納 */
int get_line(FILE *fp, char *line){
	static int fd = 0;
	int l;
	if((l=read(fd,line,MAX_LINE_LEN))<0){
		printf("fail to read\n");
		return 0;   /* 失敗 */
    }
	if (l == 0) {
		line[0] = '\0';
		close(0);
		fd = open("/dev/tty", O_RDONLY);
		return 1;
	}
	if(line[0] == '\0'){
		printf("\nError:input only \\0, so end process\n\n");
		return 0;
	}
  return 1;/*成功*/
}



int my_isdigit(int c)
{
  /* 数字は0以外なら何でも大丈夫 */
  if('0' <= c && c <= '9')
    return 1;
  else
    return 0;
}

long double my_atof(char *s) {
    long double type  = 1;
    long double n = 0;
    long double i = 1;
    while(*s == ' ') { s++; }

    if ( *s == '+' ) {
        s++;
    }
    else if ( *s == '-' ) {
        s++;
        type = -1;
    }

    /* 整数部分の処理 */
    while(my_isdigit(*s)) {
        n = n * 10 + *s - '0';
        s++;
    }

    /* 小数点部分の処理 */
    if ( *s == '.' ) {
        i = 1;
        while(my_isdigit(*++s)){
            n += (*s - '0') * (i*=0.1);
        }
    }

    return n * type;
}

int factorial(int n){
	if(n<0){
		error_flag = 1;
		printf("Error:x must be positive number in x!\n\n");
		return 0.0;
	}
	if(n == 0) return 1;
	if(n > 12){
		//D();
		error_flag = 1;
		printf("Error:overflow\n\n");
		return 0.0;
	}

	return n*factorial(n-1);
}

long double my_pow(long double x, int n){
	int i;
	long double pow_result = 1;

	if(n == 0)
		return 1;
	else{
		for(i = 0;i < n;i++)
		{
			pow_result *= x;
			if(pow_result > 999999999.999999){
				error_flag = 2;
				return 0.0;
			}
		}
		return pow_result;
	}
}

long double my_sin(long double x){
	int nMAX = 50;
	int n;
	long double sum;
	long double t;

	x -= (int)(x / (2 * 3.14159265358979323846)) * 2 * 3.14159265358979323846;

	//printf("x = %Lf\n",x);
	t = x;
	sum = x;

	for(n=1; n<=nMAX;n++){
		t *= -(x*x)/((2*n+1)*(2*n));
		sum += t;
	}
	return sum;
}

long double my_cos(long double x){
    return my_sin(3.14159265358979323846 / 2 - x);
}

long double my_tan(long double x){
	long double cos = my_cos(x);
	if( cos < 0.000001 && cos > -0.000001){
		printf("Error:tan((2*n+1)*pi/2) is undefined.(n is integer number)\n\n");
		error_flag = 1;
		return 0.0;
	}

    return my_sin(x) / my_cos(x);
}

long double my_sqrt(long double f){
	long double s = f,t;

	if(f<0){
		error_flag = 1;
		printf("Error:x must be positive number in sqrt(x)\n");
		return 0.0;
	}
	if(f==0) return 0.0;
	do{
		t=s;
		s=(t+f/t)/2;
	}while(s<t);
	return t;
}

long double calculate(int local_top){

	long double answer;
	long double local_left;
	long double local_right;

	if(error_flag == 1) return 0.0;

	if(token_store[local_top].type == INTEGER || token_store[local_top].type == DOUBLE){
		return my_atof(token_store[local_top].string);
	}

	if(token_store[local_top].type == PI){
		return 3.14159265358979323846;
	}

	if(token_store[local_top].type == E){
		return 2.71828182845904523536;
	}
	if(token_store[local_top].type == LAST){
		return last_answer;
	}

	if(token_store[local_top].type == ADD){
		local_left = calculate(token_store[local_top].left);
		local_right = calculate(token_store[local_top].right);
		answer = local_left + local_right;
		if(answer >= 999999999.9999995 || answer <= -999999999.9999995){
			error_flag = 2;
			return 0;
		}
		//printf("answer2 = %Lf + %Lf = %Lf\n",local_left, local_right, answer);

		return answer;
	}

	if(token_store[local_top].type == SUB){
		local_left = calculate(token_store[local_top].left);
		local_right = calculate(token_store[local_top].right);
		answer = local_left - local_right;
		if(answer >= 999999999.9999995 || answer <= -999999999.9999995){
			error_flag = 2;
			return 0;
		}
		//printf("answer2 = %Lf - %Lf = %Lf\n",local_left, local_right, answer);

		return answer;
	}

	if(token_store[local_top].type == MUL){
		local_left = calculate(token_store[local_top].left);
		local_right = calculate(token_store[local_top].right);
		answer = local_left * local_right;
		if(answer >= 999999999.9999995 || answer <= -999999999.9999995){
			error_flag = 2;
			return 0;
		}

		return answer;
	}

	if(token_store[local_top].type == DIV){
		local_left = calculate(token_store[local_top].left);
		local_right = calculate(token_store[local_top].right);
		if(local_right < 0.000001 && local_right > -0.000001){
			printf("Error: divided by zero is forbidden\n\n");
			error_flag = 1;
		}
		answer = local_left / local_right;

		return answer;
	}

	if(token_store[local_top].type == REST){
		local_left = calculate(token_store[local_top].left);
		local_right = calculate(token_store[local_top].right);
		if(local_right < 0.000001 && local_right > -0.000001){
			printf("Error: divided by zero is forbidden\n\n");
			error_flag = 1;
			return 0;
		}
		answer = (int)local_left % (int)local_right;

		return answer;
	}

	if(token_store[local_top].type == SIN){
		return my_sin(calculate(token_store[local_top].right));
	}

	if(token_store[local_top].type == COS){
		return my_cos(calculate(token_store[local_top].right));
	}

	if(token_store[local_top].type == TAN){
		return my_tan(calculate(token_store[local_top].right));
	}

	if(token_store[local_top].type == POW){
		return my_pow(calculate(token_store[local_top].left), (int)(calculate(token_store[local_top].right)));
	}

	if(token_store[local_top].type == FAC){
		return 1.0*factorial((int)calculate(token_store[local_top].right));
	}

	if(token_store[local_top].type == EXP){
		return my_pow(2.718281828, (int)(calculate(token_store[local_top].right)));
	}

	if(token_store[local_top].type == SQRT){
		return my_sqrt(calculate(token_store[local_top].right));
	}

	/* エラーチェック */
	printf("error2\n");
	return 0;
}

void cmd_help(){
	char c = '>';
	printf("========================COMMAND====================================\n");
	printf("Help   :print this message.\n");
	printf("Exit   :exit from this program.\n");
	printf("=======================OPERATOR====================================\n");
	printf("+,-    :sign or addition, subtraction respectively\n");
	printf("*,/,%%  :multiplication, division, remainder respectively\n");
	printf("sin(x),cos(x),tan(x):a trigonometric function\n");
	printf("sqrt(x):the square root of x\n");
	printf("exp(x) :e^x. \"e\" is  Napier's constant.\n");
	printf("x !    :multiplying the successive integers from 1 through x\n");
	printf("( )    :the formula noted in bracket is calculated preferentially.\n");
	printf("x ^ y  :x^y is x to the power of y.\n");
	printf("- - - - - - - - - - - -CONSTANT - - - - - - - - - - - - - - - - - -\n");
	printf("pi     :circular constant ((3.141592...)\n");
	printf("e      :Napier's constant (2.718281...)\n");
	printf("last   :the latest answer.if you didn't calculate, last=0.\n");
	printf("-------------------------------------------------------------------\n");
	printf("the priority of operator is \n  ! %c +,-(sign) %c ^ %c *,/,%% %c +,-(arithmetic operator).\n",c,c,c,c);
	printf("x,y is decimal number or integer number.\n");
	printf("if x or y must be integer but decimal,they are rounded down to int.\n");
	printf("the range of input and output is |x|<10^10 && |x|>=10^(-6).\n");
	printf("===================================================================\n");

}

int parse_line(char *line){

	int i,j;
	long double answer=0.0;
	if(line[0] == '\0') return 0; /* 改行文字のみの場合，スルーする */

	if(my_strcmp(line,"Exit")==0){
		printf("good bye /~~\n");
		return -1;
	}
	if(my_strcmp(line,"Help")==0){
		cmd_help();
		return 0;
	}
	else{
		if((lex(line)) < 0){
			//printf("lex:%d\n",j);
			printf("\n");
			return 0;
		}

		for(i=0;i<i_for_token-1;i++){
			if(token_store[i].type == RBL && token_store[i+1].type == RBR){
				printf("Error:syntax error\n\n");
				return 0;
			}
		}

		token_store[i_for_token].string[0] = '$';
		token_store[i_for_token].string[1] = '\0';
		token_store[i_for_token].type = DOLLAR;
		token_store[i_for_token].num = i_for_token;
		i_for_token++;

		/* ここで，sin()や exp()等を排除する */
		for(i=0;i<i_for_token-2;i++){
			if( (token_store[i].type == SIN && token_store[i+1].type == RBL && token_store[i+2].type == RBR) ||
				(token_store[i].type == COS && token_store[i+1].type == RBL && token_store[i+2].type == RBR) ||
				(token_store[i].type == TAN && token_store[i+1].type == RBL && token_store[i+2].type == RBR) ||
				(token_store[i].type == EXP && token_store[i+1].type == RBL && token_store[i+2].type == RBR) ||
				(token_store[i].type == LN && token_store[i+1].type == RBL && token_store[i+2].type == RBR)  ||
				(token_store[i].type == LOG && token_store[i+1].type == RBL && token_store[i+2].type == RBR) ||
				(token_store[i].type == SQRT && token_store[i+1].type == RBL && token_store[i+2].type == RBR)
			){
				printf("Error:syntax error\n\n");
				return 0;
			}
		}
		/********************************************/
		/* 文頭の +- の処理 */
		if(token_store[1].type == ADD || token_store[1].type == SUB){
		//D();
			for(i=i_for_token;i>1;i--){
				D();
				my_strcpy(token_store[i].string,token_store[i-1].string);
				token_store[i].type = token_store[i-1].type;
				token_store[i].num = i;
			}
			token_store[1].type=INTEGER;
			token_store[1].string[0] = '0';
			token_store[1].string[1] = '\0';
			token_store[1].num = 1;
			i_for_token++;
		}
		/**************************************************/
		/* '(' の後の +- 処理 */
		//while(1){
			for(i=2;i<i_for_token-1;i++){
				if( (token_store[i-1].type == RBL && token_store[i].type == ADD) ||
					(token_store[i-1].type == RBL && token_store[i].type == SUB) ){

					for(j=i_for_token;j>=i;j--){
						my_strcpy(token_store[j].string,token_store[j-1].string);
						token_store[j].type = token_store[j-1].type;
						token_store[j].num = j;
					}
					token_store[i].type=INTEGER;
					token_store[i].string[0] = '0';
					token_store[i].string[1] = '\0';
					token_store[i].num = i;
					i_for_token++;
					continue;
				}
			}
			//if(i == i_for_token-2) break;
		//}
		/////////////////////////////////

		if(oparser()<0){
			printf("\n");
			return 0;
		}

		//while(1){
			for (i=1;i<i_for_token;i++){
				if(	token_store[i].type ==  SIN ||
					token_store[i].type ==  COS ||
					token_store[i].type ==  TAN ||
					token_store[i].type ==  EXP ||
					token_store[i].type ==  LOG ||
					token_store[i].type ==  LN  ||
					token_store[i].type ==  SQRT ){
					for(j=1;j<i_for_token;j++){
						//D();
						if(token_store[token_store[i].right].num == token_store[j].right && i!=j){
							token_store[j].right = token_store[i].num;

						}
						if(token_store[token_store[i].right].num == token_store[j].left && i!=j){
							token_store[j].left = token_store[i].num;

						}
					}
				}
			}
		//}



		if(tree_top < 1){
			printf("Error:syntax error\n\n");
			return 0;
		}

		answer = calculate(tree_top);

		if(error_flag == 2){
			printf("Error:overflow\n\n");
			error_flag = 0;
			return 0;
		}

		if(error_flag == 1){
			error_flag = 0;
			return 0;
		}

		if(answer < 1000000000 && answer > - 1000000000){
			last_answer = answer;
			printf("\t= %Lf\n",answer);
			return 0;
		}
		else{
			printf("Error:overflow\n\n");
			return 0;
		}
	}

	printf("error3\n");
	return 0;
}

int main(int args, char **argv){

  char line[MAX_LINE_LEN + 1];
	int i;
	int parse_line_flag = 0;

  //D();

	printf("===================================================================\n");
	printf("|+++++++++++++++++++++++++++++++-----------------------------------\n");
	printf("|+++++++++++++++++++++++++++++;::----------------------------------\n");
	printf("|+++++++++++++'`  `'++++++++++:  :---------------------------------\n");
	printf("|++++++++++++       ++++++++++:  :---------------------------------\n");
	printf("|+++++++++++    :,  ++++++++++:  :---------------------------------\n");
	printf("|++++++++++'  `+++++++++''++++:  :---------------------------------\n");
	printf("|++++++++++`  +++++++:     `++:  :-----:---------------------------\n");
	printf("|++++++++++   ++++++++  `   '+:  :--'      `)----------------------\n");
	printf("|++++++++++   ++++++++++++  :+:  :--   ;---------------------------\n");
	printf("|**********   *********,    :*:  //   /////////////////////////////\n");
	printf("|**********   '******;      :*:  //   /////////////////////////////\n");
	printf("|**********'   ******   **  :*:  //   /////////////////////////////\n");
	printf("|***********    ::` *   **  ,*:  //.   ';./  Y/////,  Y////////////\n");
	printf("|************       *`      .*:  ////        /////,   Y////////////\n");
	printf("|*************;`  `;**. `*,.,*;..///////://////////,  /////////////\n");
	printf("|*******************************///////////////////////////////////\n");
	printf("|*******************************///////////////////////////////////\n");
	printf("==================================================================\n");
	printf("|if you don't know any commands please input \"Help\"\n");
	printf("==================================================================\n");

	if(write(1,"calc. > ",8)==-1){
		printf("fail to write/n");
		return -1;
    }

	for(i=0;i<MAX_LINE_LEN;i++){
			line[i] = '\0';
		}

 	while(get_line(stdin, line)){
 		if (line[0] == '\0') {
 			continue;
 		}
		subst(line, EOF, '\0');
		subst(line, '\n', '\0');

		parse_line_flag = parse_line(line);

 		if(parse_line_flag == -1) return 0;

		if(write(1,"calc. > ",8)==-1){
			printf("fail to write/n");
			return -1;
		}


		for(i=0;i<MAX_LINE_LEN;i++){
			line[i] = '\0';
		}

	}

	return 0;
}
