
enum token {
  tok_menuname, 
  tok_menuoption, 
  tok_comment, 
  tok_bool, 
  tok_tristate, 
  tok_dep_tristate,
  tok_nop,
  tok_if, 
  tok_else, 
  tok_fi, 
  tok_int,
  tok_hex,
  tok_string,
  tok_define,
  tok_choose,
  tok_choice,
  tok_endmenu,
  tok_unknown
};

enum operator {
  op_eq,
  op_neq,
  op_and,
  op_and1,
  op_or,
  op_bang,
  op_lparen,
  op_rparen,
  op_variable,
  op_kvariable,
  op_shellcmd,
  op_constant,
  op_nuked
};

union var
{
  char * str;
  struct kconfig * cfg;
};

struct condition
{
  struct condition * next;
  enum operator op;
  union var variable;
};

#define GLOBAL_WRITTEN  1
#define CFG_DUP 	2
#define UNSAFE		4

struct kconfig
{
  struct kconfig 	* next;
  int 			  flags;
  enum 			  token tok;
  char   		  menu_number;
  char   		  menu_line;
  char   		  submenu_start;
  char   		  submenu_end;
  char 			* optionname;
  char 			* label;
  char 			* value;
  int   		  choice_value;
  struct kconfig        * choice_label;
  union var 		  depend;
  struct condition 	* cond;
};

extern struct kconfig * config;
extern struct kconfig * clast;
extern struct kconfig * koption;

/*
 * Prototypes
 */
void fix_conditionals(struct kconfig * scfg);	/* tkcond.c */
void dump_tk_script(struct kconfig *scfg);	/* tkgen.c  */
