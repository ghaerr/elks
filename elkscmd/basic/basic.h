#include <stdint.h>

#define MEMORY_SIZE	10240		// max tokenized memory bytes for program
#define TOKEN_BUF_SIZE  256             // max tokenized bytes per line
#define MAX_IDENT_LEN	8
#define MAX_NUMBER_LEN	30
#define MAX_PATH_LEN	64		// for LOAD/SAVE filename strings

#define false		0
#define true		1

#define TOKEN_EOL		0
#define TOKEN_IDENT		1	// special case - identifier follows
#define TOKEN_INTEGER	        2	// special case - long follows (line number)
#define TOKEN_NUMBER	        3	// special case - number follows
#define TOKEN_STRING	        4	// special case - string follows

#define FIRST_NON_ALPHA_TOKEN   8
#define TOKEN_LBRACKET	        8
#define TOKEN_RBRACKET	        9
#define TOKEN_PLUS	    	10
#define TOKEN_MINUS		11
#define TOKEN_MULT		12
#define TOKEN_DIV		13
#define TOKEN_EQUALS	        14
#define TOKEN_GT		15
#define TOKEN_LT		16
#define TOKEN_NOT_EQ	        17
#define TOKEN_GT_EQ		18
#define TOKEN_LT_EQ		19
#define TOKEN_CMD_SEP	        20
#define TOKEN_SEMICOLON	        21
#define TOKEN_COMMA		22
#define LAST_NON_ALPHA_TOKEN    22
#define FIRST_IDENT_TOKEN       23
#define TOKEN_AND		23	// FIRST_IDENT_TOKEN
#define TOKEN_OR		24
#define TOKEN_NOT		25
#define TOKEN_PRINT		26
#define TOKEN_LET		27
#define TOKEN_LIST		28
#define TOKEN_RUN		29
#define TOKEN_GOTO		30
#define TOKEN_REM		31
#define TOKEN_STOP		32
#define TOKEN_INPUT	        33
#define TOKEN_CONT		34
#define TOKEN_IF		35
#define TOKEN_THEN		36
#define TOKEN_LEN		37
#define TOKEN_VAL		38
#define TOKEN_RND		39
#define TOKEN_INT		40
#define TOKEN_STR		41
#define TOKEN_FOR		42
#define TOKEN_TO		43
#define TOKEN_STEP		44
#define TOKEN_NEXT		45
#define TOKEN_MOD		46
#define TOKEN_NEW		47
#define TOKEN_GOSUB		48
#define TOKEN_RETURN	        49
#define TOKEN_DIM		50
#define TOKEN_LEFT		51
#define TOKEN_RIGHT	        52
#define TOKEN_MID		53
#define TOKEN_CLS               54
#define TOKEN_PAUSE             55
#define TOKEN_POSITION          56
#define TOKEN_PIN               57
#define TOKEN_PINMODE           58
#define TOKEN_INKEY             59
#define TOKEN_SAVE              60
#define TOKEN_LOAD              61
#define TOKEN_PINREAD           62
#define TOKEN_ANALOGRD          63
#define TOKEN_DIR               64
#define TOKEN_DELETE            65
#define TOKEN_PI		66
#define TOKEN_ABS		67
#define TOKEN_COS		68
#define TOKEN_SIN		69
#define TOKEN_TAN		70
#define TOKEN_ACS		71
#define TOKEN_ASN		72
#define TOKEN_ATN		73
#define TOKEN_EXP		74
#define TOKEN_LN		75
#define TOKEN_POW		76
#define TOKEN_CHR		77
#define TOKEN_CODE		78
#define TOKEN_DATA		79
#define TOKEN_READ		80
#define TOKEN_RESTORE		81
#define TOKEN_MODE		82
#define TOKEN_PLOT		83
#define LAST_IDENT_TOKEN	83

#define ERROR_NONE				0
// parse errors
#define ERROR_LEXER_BAD_NUM			1
#define ERROR_LEXER_TOO_LONG			2
#define ERROR_LEXER_UNEXPECTED_INPUT	        3
#define ERROR_LEXER_UNTERMINATED_STRING         4
#define ERROR_EXPR_MISSING_BRACKET		5
#define ERROR_UNEXPECTED_TOKEN			6	/* Error in expr */
#define ERROR_EXPR_EXPECTED_NUM			7
#define ERROR_EXPR_EXPECTED_STR			8
#define ERROR_LINE_NUM_TOO_BIG			9
// runtime errors
#define ERROR_OUT_OF_MEMORY			10
#define ERROR_EXPR_DIV_ZERO			11
#define ERROR_VARIABLE_NOT_FOUND		12
#define ERROR_UNEXPECTED_CMD			13
#define ERROR_BAD_LINE_NUM			14
#define ERROR_BREAK_PRESSED			15
#define ERROR_NEXT_WITHOUT_FOR			16
#define ERROR_STOP_STATEMENT			17
#define ERROR_MISSING_THEN			18
#define ERROR_RETURN_WITHOUT_GOSUB		19
#define ERROR_WRONG_ARRAY_DIMENSIONS	        20
#define ERROR_ARRAY_SUBSCRIPT_OUT_RANGE	        21
#define ERROR_STR_SUBSCRIPT_OUT_RANGE	        22
#define ERROR_IN_VAL_INPUT			23
#define ERROR_BAD_PARAMETER                     24
#define ERROR_EOF				25
#define ERROR_FILE_ERROR			26
#define ERROR_FUNCTION_NOT_BUILTIN		27
#define ERROR_WRONG_NUM_FUNCTION_ARGS		28
#define ERROR_NO_DATA_FOR_READ  		29

extern unsigned char mem[];
extern int sysPROGEND;
extern int sysSTACKSTART;
extern int sysSTACKEND;
extern int sysVARSTART;
extern int sysVAREND;
extern int sysGOSUBSTART;
extern int sysGOSUBEND;

extern uint16_t lineNumber;	// 0 = input buffer

typedef struct {
    float val;
    float step;
    float end;
    uint16_t lineNumber;
    uint16_t stmtNumber;
} 
ForNextData;

typedef struct {
    char *token;
    uint8_t format;
} 
TokenTableEntry;

extern const char* const errorTable[];

void reset();
int tokenize(unsigned char *input, unsigned char *output, int outputSize);
int processInput(unsigned char *tokenBuf);
void listProg(uint16_t first, uint16_t last);
