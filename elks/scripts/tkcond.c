/* parser config.in
 *
 * Version 1.0
 * Eric Youngdale
 * 10/95
 *
 * The general idea here is that we want to parse a config.in file and 
 * from this, we generate a wish script which gives us effectively the
 * same functionality that the original config.in script provided.
 *
 * This task is split roughly into 3 parts.  The first parse is the parse
 * of the input file itself.  The second part is where we analyze the 
 * #ifdef clauses, and attach a linked list of tokens to each of the
 * menu items.  In this way, each menu item has a complete list of
 * dependencies that are used to enable/disable the options.
 * The third part is to take the configuration database we have build,
 * and build the actual wish script.
 *
 * This file contains the code to further process the conditions from
 * the "ifdef" clauses.
 *
 * The conditions are assumed to be one of the following formats
 *
 * simple_condition:= "$VARIABLE" == y/n/m
 * simple_condition:= "$VARIABLE != y/n/m
 *
 * simple_condition -a simple_condition
 *
 * If the input condition contains '(' or ')' it would screw us up, but for now
 * this is not a problem.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tkparse.h"


/*
 * Walk a condition chain and invert it so that the logical result is
 * inverted.
 */
static void invert_condition(struct condition * cnd)
{
  /*
   * This is simple.  Just walk through the list, and invert
   * all of the operators.
   */
  for(;cnd; cnd = cnd->next)
    {
      switch(cnd->op)
	{
	case op_and:
	  cnd->op = op_or;
	  break;
	case op_or:
	  /*
	   * This is not turned into op_and - we need to keep track
	   * of what operators were used here since we have an optimization
	   * later on to remove duplicate conditions, and having
	   * inverted ors in there would make it harder if we did not
	   * distinguish an inverted or from an and we inserted because
	   * of nested ifs.
	   */
	  cnd->op = op_and1;
	  break;
	case op_neq:
	  cnd->op = op_eq;
	  break;
	case op_eq:
	  cnd->op = op_neq;
	  break;
	default:
	  break;
	}
    }
}

/*
 * Walk a condition chain, and free the memory associated with it.
 */
static void free_condition(struct condition * cnd)
{
  struct condition * next;
  for(;cnd; cnd = next)
    {
      next = cnd->next;

      if( cnd->variable.str != NULL )
	free(cnd->variable.str);

      free(cnd);
    }
}

/*
 * Walk all of the conditions, and look for choice values.  Convert
 * the tokens into something more digestible.
 */
void fix_choice_cond(void)
{
  struct condition * cond;
  struct condition * cond2;
  struct kconfig * cfg;
  char tmpbuf[255];

  for(cfg = config;cfg != NULL; cfg = cfg->next)
    {
      if( cfg->cond == NULL )
	{
	  continue;
	}

      for(cond = cfg->cond; cond != NULL; cond = cond->next)
	{
	  if( cond->op != op_kvariable )
	    continue;

	  if( cond->variable.cfg->tok != tok_choice )
	    continue;

	  /*
	   * Look ahead for what we are comparing this to.  There should
	   * be one operator in between.
	   */
	  cond2 = cond->next->next;
	  strcpy(tmpbuf, cond->variable.cfg->label);

	  if( strcmp(cond2->variable.str, "y") == 0 )
	    {
	      cond->variable.cfg = cond->variable.cfg->choice_label;
	      cond2->variable.str = strdup(tmpbuf);
	    }
	  else
	    {
	      fprintf(stderr,"Ooops\n");
	      exit(0);
	    }
	}

    }
}

/*
 * Walk the stack of conditions, and clone all of them with "&&" operators
 * gluing them together.  The conditions from each level of the stack
 * are wrapped in parenthesis so as to guarantee that the results
 * are logically correct.
 */
struct condition * get_token_cond(struct condition ** cond, int depth)
{
  int i;
  struct condition * newcond;
  struct condition * tail;
  struct condition * new;
  struct condition * ocond;
  struct kconfig * cfg;

  newcond = tail = NULL;
  for(i=0; i<depth; i++, cond++)
    {
      /*
       * First insert the left parenthesis
       */
      new = (struct condition *) malloc(sizeof(struct condition));
      memset(new, 0, sizeof(*new));
      new->op = op_lparen;
      if( tail == NULL )
	{
	  newcond = tail = new;
	}
      else
	{
	  tail->next = new;
	  tail = new;
	}

      /*
       * Now duplicate the chain.
       */
      ocond = *cond;
      for(;ocond != NULL; ocond = ocond->next)
	{
	  new = (struct condition *) malloc(sizeof(struct condition));
	  memset(new, 0, sizeof(*new));
	  new->op = ocond->op;
	  if( ocond->variable.str != NULL )
	    {
	      if( ocond->op == op_variable )
		{
		  /*
		   * Search for structure to insert here.
		   */
		  for(cfg = config;cfg != NULL; cfg = cfg->next)
		    {
		      if( cfg->tok != tok_bool
		         && cfg->tok != tok_int
		         && cfg->tok != tok_hex
		         && cfg->tok != tok_string
			 && cfg->tok != tok_tristate 
			 && cfg->tok != tok_choice
			 && cfg->tok != tok_dep_tristate)
			{
			  continue;
			}
		      if( strcmp(cfg->optionname, ocond->variable.str) == 0)
			{
			  new->variable.cfg = cfg;
			  new->op = op_kvariable;
			  break;
			}
		    }
		  if( cfg == NULL )
		    {
		      new->variable.str = strdup(ocond->variable.str);
		    }
		}
	      else
		{
		  new->variable.str = strdup(ocond->variable.str);
		}
	    }
	  tail->next = new;
	  tail = new;
	}

      /*
       * Next insert the left parenthesis
       */
      new = (struct condition *) malloc(sizeof(struct condition));
      memset(new, 0, sizeof(*new));
      new->op = op_rparen;
      tail->next = new;
      tail = new;

      /*
       * Insert an and operator, if we have another condition.
       */
      if( i < depth - 1 )
	{
	  new = (struct condition *) malloc(sizeof(struct condition));
	  memset(new, 0, sizeof(*new));
	  new->op = op_and;
	  tail->next = new;
	  tail = new;
	}

    }

  return newcond;
}

/*
 * Walk a single chain of conditions and clone it.  These are assumed
 * to be created/processed by  get_token_cond in a previous pass.
 */
struct condition * get_token_cond_frag(struct condition * cond, 
				       struct condition ** last)
{
  struct condition * newcond;
  struct condition * tail;
  struct condition * new;
  struct condition * ocond;

  newcond = tail = NULL;

  /*
   * Now duplicate the chain.
   */
  for(ocond = cond;ocond != NULL; ocond = ocond->next)
    {
      new = (struct condition *) malloc(sizeof(struct condition));
      memset(new, 0, sizeof(*new));
      new->op = ocond->op;
      new->variable.cfg = ocond->variable.cfg;
      if( tail == NULL )
	{
	  newcond = tail = new;
	}
      else
	{
	  tail->next = new;
	  tail = new;
	}
    }

  new = (struct condition *) malloc(sizeof(struct condition));
  memset(new, 0, sizeof(*new));
  new->op = op_and;
  tail->next = new;
  tail = new;
  
  *last = tail;
  return newcond;
}

/*
 * Walk through the if conditionals and maintain a chain.
 */
void fix_conditionals(struct kconfig * scfg)
{
  int depth = 0;
  int i;
  struct kconfig * cfg;
  struct kconfig * cfg1;
  struct condition * conditions[25];
  struct condition * cnd;
  struct condition * cnd1;
  struct condition * cnd2;
  struct condition * cnd3;
  struct condition * newcond;
  struct condition * last;

  /*
   * Start by walking the chain.  Every time we see an ifdef, push
   * the condition chain on the stack.  When we see an "else", we invert
   * the condition at the top of the stack, and when we see an "endif"
   * we free all of the memory for the condition at the top of the stack
   * and remove the condition from the top of the stack.
   *
   * For any other type of token (i.e. a bool), we clone a new condition chain
   * by anding together all of the conditions that are currently stored on
   * the stack.  In this way, we have a correct representation of whatever
   * conditions govern the usage of each option.
   */
  memset(conditions, 0, sizeof(conditions));
  for(cfg=scfg;cfg != NULL; cfg = cfg->next)
    {
      switch(cfg->tok)
	{
	case tok_if:
	  /*
	   * Push this condition on the stack, and nuke the token
	   * representing the ifdef, since we no longer need it.
	   */
	  conditions[depth] = cfg->cond;
	  depth++;
	  cfg->tok = tok_nop;
	  cfg->cond =  NULL;
	  break;
	case tok_else:
	  /*
	   * For an else, we just invert the condition at the top of
	   * the stack.  This is done in place with no reallocation
	   * of memory taking place.
	   */
	  invert_condition(conditions[depth-1]);
	  cfg->tok = tok_nop;
	  break;
	case tok_fi:
	  depth--;
	  free_condition(conditions[depth]);
	  conditions[depth] = NULL;
	  cfg->tok = tok_nop;
	  break;
	case tok_comment:
	case tok_define:
	case tok_menuoption:
	case tok_bool:
	case tok_tristate:
	case tok_int:
	case tok_hex:
	case tok_string:
	case tok_choice:
	  /*
	   * We need to duplicate the chain of conditions and attach them to
	   * this token.
	   */
	  cfg->cond = get_token_cond(&conditions[0], depth);
	  break;
	case tok_dep_tristate:
	  /*
	   * Same as tok_tristate et al except we have a temporary
	   * conditional. (Sort of a hybrid tok_if, tok_tristate, tok_fi
	   * option)
	   */
	  conditions[depth] = cfg->cond;
	  depth++;
	  cfg->cond = get_token_cond(&conditions[0], depth);
	  depth--;
	  free_condition(conditions[depth]);
	  conditions[depth] = NULL;
	default:
	  break;
	}
    }

  /*
   * Fix any conditions involving the "choice" operator.
   */
  fix_choice_cond();

  /*
   * Walk through and see if there are multiple options that control the
   * same kvariable.  If there are we need to treat them a little bit
   * special.
   */
  for(cfg=scfg;cfg != NULL; cfg = cfg->next)
    {
      switch(cfg->tok)
	{
	case tok_bool:
	case tok_tristate:
	case tok_dep_tristate:
	case tok_int:
	case tok_hex:
	case tok_string:
	  for(cfg1=cfg;cfg1 != NULL; cfg1 = cfg1->next)
	    {
	      switch(cfg1->tok)
		{
		case tok_define:
		case tok_bool:
		case tok_tristate:
		case tok_dep_tristate:
		case tok_int:
		case tok_hex:
		case tok_string:
		  if( strcmp(cfg->optionname, cfg1->optionname) == 0)
		    {
		      cfg->flags |= CFG_DUP;
		      cfg1->flags |= CFG_DUP;
		    }
		  break;
		default:
		  break;
		}
	    }
	  break;
	default:
	  break;
	}
    }

  /*
   * Now go through the list, and every time we see a kvariable, check
   * to see whether it also has some dependencies.  If so, then
   * append it to our list.  The reason we do this is that we might have
   * option CONFIG_FOO which is only used if CONFIG_BAR is set.  It may
   * turn out that in config.in that the default value for CONFIG_BAR is
   * set to "y", but that CONFIG_BAR is not enabled because CONFIG_XYZZY
   * is not set.  The current condition chain does not reflect this, but
   * we can fix this by searching for the tokens that this option depends
   * upon and cloning the conditions and merging them with the list.
   */
  for(cfg=scfg;cfg != NULL; cfg = cfg->next)
    {
      /*
       * Search for a token that has a condition list.
       */
      if(cfg->cond == NULL) continue;
      for(cnd = cfg->cond; cnd; cnd=cnd->next)
	{
	  /*
	   * Now search the condition list for a known configuration variable
	   * that has conditions of its own.
	   */
	  if(cnd->op != op_kvariable) continue;
	  if(cnd->variable.cfg->cond == NULL) continue;

	  if(cnd->variable.cfg->flags & CFG_DUP) continue; 
	  /*
	   * OK, we have some conditions to append to cfg.  Make  a clone
	   * of the conditions,
	   */
	  newcond = get_token_cond_frag(cnd->variable.cfg->cond, &last);

	  /*
	   * Finally, we splice it into our list.
	   */
	  last->next = cfg->cond;
	  cfg->cond = newcond;

	}
    }

  /*
   * There is a strong possibility that we have duplicate conditions
   * in here.  It would make the script more efficient and readable to
   * remove these.  Here is where we assume here that there are no
   * parenthesis in the input script.
   */
  for(cfg=scfg;cfg != NULL; cfg = cfg->next)
    {
      /*
       * Search for configuration options that have conditions.
       */
      if(cfg->cond == NULL) continue;
      for(cnd = cfg->cond; cnd; cnd=cnd->next)
	{
	  /*
	   * Search for a left parenthesis.
	   */
	  if(cnd->op != op_lparen) continue;
	  for(cnd1 = cnd->next; cnd1; cnd1=cnd1->next)
	    {
	      /*
	       * Search after the previous left parenthesis, and try
	       * and find a second left parenthesis.
	       */
	      if(cnd1->op != op_lparen) continue;

	      /*
	       * Now compare the next 5 tokens to see if they are
	       * identical.  We are looking for two chains that
	       * are like: '(' $VARIABLE operator constant ')'.
	       */
	      cnd2 = cnd;
	      cnd3 = cnd1;
	      for(i=0; i<5; i++, cnd2=cnd2->next, cnd3=cnd3->next)
		{
		  if(!cnd2 || !cnd3) break;
		  if(cnd2->op != cnd3->op) break;
		  if(i == 1 && (cnd2->op != op_kvariable 
		     || cnd2->variable.cfg != cnd3->variable.cfg) ) break;
		  if(i==2 && cnd2->op != op_eq && cnd2->op != op_neq) break;
		  if(i == 3 && cnd2->op != op_constant &&
		     strcmp(cnd2->variable.str, cnd3->variable.str) != 0)
		    break;
		  if(i==4 && cnd2->op != op_rparen) break;
		}
	      /*
	       * If these match, and there is an and gluing these together,
	       * then we can nuke the second one.
	       */
	      if(i==5 && ((cnd3 && cnd3->op == op_and)
			  ||(cnd2 && cnd2->op == op_and)))
		{
		  /*
		   * We have a duplicate.  Nuke 5 ops.
		   */
		  cnd3 = cnd1;
		  for(i=0; i<5; i++, cnd3=cnd3->next)
		    {
		      cnd3->op = op_nuked;
		    }
		  /*
		   * Nuke the and that glues the conditions together.
		   */
		  if(cnd3 && cnd3->op == op_and) cnd3->op = op_nuked;
		  else if(cnd2 && cnd2->op == op_and) cnd2->op = op_nuked;
		}
	    }
	}
    }
}
