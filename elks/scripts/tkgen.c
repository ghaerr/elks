/* Generate tk script based upon config.in
 *
 * Version 1.0
 * Eric Youngdale
 * 10/95
 *
 * 1996 01 04
 * Avery Pennarun - Aesthetic improvements.
 *
 * 1996 01 24
 * Avery Pennarun - Bugfixes and more aesthetics.
 *
 * 1996 03 08
 * Avery Pennarun - The int and hex config.in commands work right.
 *                - Choice buttons are more user-friendly.
 *                - Disabling a text entry line greys it out properly.
 *                - dep_tristate now works like in Configure. (not pretty)
 *                - No warnings in gcc -Wall. (Fixed some "interesting" bugs.)
 *                - Faster/prettier "Help" lookups.
 *
 * 1996 03 15
 * Avery Pennarun - Added new sed script from Axel Boldt to make help even
 *                  faster. (Actually awk is downright slow on some machines.)
 *                - Fixed a bug I introduced into Choice dependencies.  Thanks
 *                  to Robert Krawitz for pointing this out.
 *
 * 1996 03 16
 * Avery Pennarun - basic "do_make" support added to let sound config work.
 *
 * 1996 03 25
 *     Axel Boldt - Help now works on "choice" buttons.
 *
 * 1996 04 06
 * Avery Pennarun - Improved sound config stuff. (I think it actually works
 *                  now!)
 *                - Window-resize-limits don't use ugly /usr/lib/tk4.0 hack.
 *                - int/hex work with tk3 again. (The "cget" error.)
 *                - Next/Prev buttons switch between menus.  I can't take
 *                  much credit for this; the code was already there, but
 *                  ifdef'd out for some reason.  It flickers a lot, but
 *                  I suspect there's no "easy" fix for that.
 *                - Labels no longer highlight as you move the mouse over
 *                  them (although you can still press them... oh well.)
 *                - Got rid of the last of the literal color settings, to
 *                  help out people with mono X-Windows systems. 
 *                  (Apparently there still are some out there!)
 *                - Tabstops seem sensible now.
 *
 * 1996 04 14
 * Avery Pennarun - Reduced flicker when creating windows, even with "update
 *                  idletasks" hack.
 *
 * 1997 12 08
 * Michael Chastain - Remove sound driver special cases.
 *
 * 1997 11 15
 * Michael Chastain - For choice buttons, write values for all options,
 *                    not just the single chosen one.  This is compatible
 *                    with 'make config' and 'make oldconfig', and is
 *                    needed so smart-config dependencies work if the
 *                    user switches from one configuration method to
 *                    another.
 *
 * 1998 03 09
 * Axel Boldt - Smaller layout of main menu - it's still too big for 800x600.
 *            - Display help in text window to allow for cut and paste.
 *            - Allow for empty lines in help texts.
 *            - update_define should not set all variables unconditionally to
 *              0: they may have been set to 1 elsewhere. CONFIG_NETLINK is
 *              an example.
 *
 * TO DO:
 *   - clean up - there are useless ifdef's everywhere.
 *   - better comments throughout - C code generating tcl is really cryptic.
 *   - eliminate silly "update idletasks" hack to improve display speed and
 *     reduce flicker.  But how?
 *   - make canvas contents resize with the window (good luck).
 *   - some way to make submenus inside of submenus (ie. Main->Networking->IP)
 *           (perhaps a button where the description would be)
 *   - make the main menu use the same tcl code as the submenus.
 *   - make choice and int/hex input types line up vertically with
 *           bool/tristate.
 *   - general speedups - how?  The canvas seems to slow it down a lot.
 *   - clean up +/- 16 confusion for enabling/disabling variables; causes
 *           (theoretical, at the moment) problems with dependencies.
 *   
 */
#include <stdio.h>
#include <unistd.h>
#include "tkparse.h"

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

/*
 * This is the total number of submenus that we have.
 */
static int tot_menu_num =0;

/*
 * Generate portion of wish script for the beginning of a submenu.
 * The guts get filled in with the various options.
 */
static void start_proc(char * label, int menu_num, int flag)
{
  if( flag )
    printf("menu_option menu%d %d \"%s\"\n", menu_num, menu_num, label);
  printf("proc menu%d {w title} {\n", menu_num);
  printf("\tcatch {destroy $w}\n");
  printf("\ttoplevel $w -class Dialog\n");
  printf("\twm withdraw $w\n");
  printf("\tmessage $w.m -width 400 -aspect 300 -text \\\n");
  printf("\t\t\"%s\"  -relief raised\n",label);
  printf("\tpack $w.m -pady 10 -side top -padx 10\n");
  printf("\twm title $w \"%s\" \n\n", label);
  
  /*
   * Attach the "Prev", "Next" and "OK" buttons at the end of the window.
   */
  printf("\tset oldFocus [focus]\n");
  printf("\tframe $w.f\n");
  printf("\tbutton $w.f.back -text \"Main Menu\" \\\n"
         "\t\t-width 15 -command \"destroy $w; focus $oldFocus; update_mainmenu $w\"\n");
  printf("\tbutton $w.f.next -text \"Next\" \\\n"
         "\t\t-width 15 -command \" destroy $w; focus $oldFocus;  menu%d .menu%d \\\"$title\\\"\"\n",
         	menu_num+1, menu_num+1);
  if (menu_num == tot_menu_num)
  	printf("\t$w.f.next configure -state disabled\n");
  printf("\tbutton $w.f.prev -text \"Prev\" \\\n"
         "\t\t-width 15 -command \" destroy $w; focus $oldFocus; menu%d .menu%d \\\"$title\\\"\"\n",
       		menu_num-1, menu_num-1);
  if (1 == menu_num)
  	printf("\t$w.f.prev configure -state disabled\n");
  printf("\tpack $w.f.back $w.f.next $w.f.prev -side left -expand on\n");
  printf("\tpack $w.f -pady 10 -side bottom -anchor w -fill x\n");

  /*
   * Lines between canvas and other areas of the window.
   */
  printf("\tframe $w.topline -relief ridge -borderwidth 2 -height 2\n");
  printf("\tpack $w.topline -side top -fill x\n\n");
  printf("\tframe $w.botline -relief ridge -borderwidth 2 -height 2\n");
  printf("\tpack $w.botline -side bottom -fill x\n\n");
  
  /*
   * The "config" frame contains the canvas and a scrollbar.
   */
  printf("\tframe $w.config\n");
  printf("\tpack $w.config -fill y -expand on\n\n");
  printf("\tscrollbar $w.config.vscroll -command \"$w.config.canvas yview\"\n");
  printf("\tpack $w.config.vscroll -side right -fill y\n\n");
  
  /*
   * The scrollable canvas itself, where the real work (and mess) gets done.
   */
  printf("\tcanvas $w.config.canvas -height 1\\\n"
  	 "\t\t-relief flat -borderwidth 0 -yscrollcommand \"$w.config.vscroll set\" \\\n"
  	 "\t\t-width [expr [winfo screenwidth .] * 1 / 2] \n");
  printf("\tframe $w.config.f\n");
  printf("\tpack $w.config.canvas -side right -fill y\n");
  
  printf("\n\n");
}

/*
 * Each proc we create needs a global declaration for any global variables we
 * use.  To minimize the size of the file, we set a flag each time we output
 * a global declaration so we know whether we need to insert one for a
 * given function or not.
 */
void clear_globalflags(struct kconfig * cfg)
{
  for(; cfg != NULL; cfg = cfg->next)
  {
    cfg->flags &= ~GLOBAL_WRITTEN;
  }
}

/*
 * Output a "global" line for a given variable.  Also include the
 * call to "vfix".  (If vfix is not needed, then it's fine to just printf
 * a "global" line).
 */
void inline global(char *var)
{
  printf("\tglobal %s; vfix %s\n", var, var);
}

/*
 * This function walks the chain of conditions that we got from cond.c,
 * and creates a wish conditional to enable/disable a given widget.
 */
void generate_if(struct kconfig * item,
	    struct condition * cond,
	    int menu_num,
	    int line_num)
{
  struct condition * ocond;

  ocond = cond;

  /*
   * First write any global declarations we need for this conditional.
   */
  while(cond != NULL )
    {
      switch(cond->op){
      case op_variable:
      	global(cond->variable.str);
	break;
      case op_kvariable:
	if(cond->variable.cfg->flags & GLOBAL_WRITTEN) break;
	cond->variable.cfg->flags |= GLOBAL_WRITTEN;
	global(cond->variable.cfg->optionname);
	break;
      default:
	break;
      }
      cond = cond->next;
    }
  
  /*
   * Now write this option.
   */
  if(   (item->flags & GLOBAL_WRITTEN) == 0
     && (item->optionname != NULL) )
    {
      global(item->optionname);
      item->flags |= GLOBAL_WRITTEN;
    }
  /*
   * Now generate the body of the conditional.
   */
  printf("\tif {");
  cond = ocond;
  while(cond != NULL )
    {
      switch(cond->op){
      case op_bang:
	printf(" ! ");
	break;
      case op_eq:
	printf(" == ");
	break;
      case op_neq:
	printf(" != ");
	break;
      case op_and:
      case op_and1:
	printf(" && ");
	break;
      case op_or:
	printf(" || ");
	break;
      case op_lparen:
	printf("(");
	break;
      case op_rparen:
	printf(")");
	break;
      case op_variable:
	printf("$%s", cond->variable.str);
	break;
      case op_kvariable:
	printf("$%s", cond->variable.cfg->optionname);
	break;
      case op_shellcmd:
	printf("[exec %s]", cond->variable.str);
	break;
      case op_constant:
	if( strcmp(cond->variable.str, "y") == 0 )
	  printf("1");
	else if( strcmp(cond->variable.str, "n") == 0 )
	  printf("0");
	else if( strcmp(cond->variable.str, "m") == 0 )
	  printf("2");
	else
	  printf("\"%s\"", cond->variable.str);
	break;
      default:
        break;
      }
      cond = cond->next;
    }

  /*
   * Now we generate what we do depending upon the value of the conditional.
   * Depending upon what the token type is, there are different things
   * we must do to enable/disable the given widget - this code needs to
   * be closely coordinated with the widget creation procedures in header.tk.
   */
  switch(item->tok)
    {
    case tok_define:
      printf("} then { set %s %s } \n",  item->optionname, item->value);
      break;
    case tok_menuoption:
      printf("} then { .f0.x%d configure -state normal } else { .f0.x%d configure -state disabled }\n",
	     menu_num, menu_num);
      break;
    case tok_int:
    case tok_hex:
    case tok_string:
      printf("} then { ");
      printf(".menu%d.config.f.x%d.x configure -state normal -fore [ cget .ref -foreground ]; ", menu_num, line_num);
      printf(".menu%d.config.f.x%d.l configure -state normal; ", menu_num, line_num);
      printf("} else { ");
      printf(".menu%d.config.f.x%d.x configure -state disabled -fore [ cget .ref -disabledforeground ];", menu_num, line_num );
      printf(".menu%d.config.f.x%d.l configure -state disabled;", menu_num, line_num );
      printf("}\n");
      break;
    case tok_bool:
#ifdef BOOL_IS_BUTTON
      /*
       * If a bool is just a button, then use this definition.
       */
      printf("} then { .menu%d.config.f.x%d configure -state normal } else { .menu%d.config.f.x%d configure -state disabled }\n",
	     menu_num, line_num,
	     menu_num, line_num );
#else
      /*
       * If a bool is a radiobutton, then use this instead.
       */
      printf("} then { ");
      printf(".menu%d.config.f.x%d.y configure -state normal;",menu_num, line_num);
      printf(".menu%d.config.f.x%d.n configure -state normal;",menu_num, line_num);
      printf(".menu%d.config.f.x%d.l configure -state normal;",menu_num, line_num);
      printf("set %s [expr $%s&15];", item->optionname, item->optionname);
      printf("} else { ");
      printf(".menu%d.config.f.x%d.y configure -state disabled;",menu_num, line_num);
      printf(".menu%d.config.f.x%d.n configure -state disabled;",menu_num, line_num);
      printf(".menu%d.config.f.x%d.l configure -state disabled;",menu_num, line_num);
      printf("set %s [expr $%s|16];", item->optionname, item->optionname);
      printf("}\n");
#endif
      break;
    case tok_tristate:
    case tok_dep_tristate:
      printf("} then { ");
      if( item->tok == tok_dep_tristate )
	{
	  global(item->depend.str);
	  printf("if { $%s != 1 && $%s != 0 } then {", 
	  	item->depend.str,item->depend.str);
	  printf(".menu%d.config.f.x%d.y configure -state disabled;",menu_num, line_num);
	  printf("} else {");
	  printf(".menu%d.config.f.x%d.y configure -state normal;",menu_num, line_num);
	  printf("}; ");
	}
      else
	{
	  printf(".menu%d.config.f.x%d.y configure -state normal;",menu_num, line_num);
	}
      
      printf(".menu%d.config.f.x%d.n configure -state normal;",menu_num, line_num);
      printf(".menu%d.config.f.x%d.m configure -state normal;",menu_num, line_num);
      printf(".menu%d.config.f.x%d.l configure -state normal;",menu_num, line_num);
      /*
       * Or in a bit to the variable - this causes all of the radiobuttons
       * to be deselected (i.e. not be red).
       */
      printf("set %s [expr $%s&15];", item->optionname, item->optionname);
      printf("} else { ");
      printf(".menu%d.config.f.x%d.y configure -state disabled;",menu_num, line_num);
      printf(".menu%d.config.f.x%d.n configure -state disabled;",menu_num, line_num);
      printf(".menu%d.config.f.x%d.m configure -state disabled;",menu_num, line_num);
      printf(".menu%d.config.f.x%d.l configure -state disabled;",menu_num, line_num);
      /*
       * Clear the disable bit - this causes the correct radiobutton
       * to appear selected (i.e. turn red).
       */
      printf("set %s [expr $%s|16];", item->optionname, item->optionname);
      printf("}\n");
      break;
    case tok_choose:
    case tok_choice:
      fprintf(stderr,"Fixme\n");
      exit(0);
    default:
      break;
    }
}

/*
 * Similar to generate_if, except we come here when generating an
 * output file.  Thus instead of enabling/disabling a widget, we
 * need to decide whether to write out a given configuration variable
 * to the output file.
 */
void generate_if_for_outfile(struct kconfig * item,
	    struct condition * cond)
{
  struct condition * ocond;

  /*
   * First write any global declarations we need for this conditional.
   */
  ocond = cond;
  for(; cond != NULL; cond = cond->next )
    {
      switch(cond->op){
      case op_variable:
        global(cond->variable.str);
	break;
      case op_kvariable:
	if(cond->variable.cfg->flags & GLOBAL_WRITTEN) break;
	cond->variable.cfg->flags |= GLOBAL_WRITTEN;
	global(cond->variable.cfg->optionname);
	break;
      default:
	break;
      }
    }

  /*
   * Now generate the body of the conditional.
   */
  printf("\tif {");
  cond = ocond;
  while(cond != NULL )
    {
      switch(cond->op){
      case op_bang:
	printf(" ! ");
	break;
      case op_eq:
	printf(" == ");
	break;
      case op_neq:
	printf(" != ");
	break;
      case op_and:
      case op_and1:
	printf(" && ");
	break;
      case op_or:
	printf(" || ");
	break;
      case op_lparen:
	printf("(");
	break;
      case op_rparen:
	printf(")");
	break;
      case op_variable:
	printf("$%s", cond->variable.str);
	break;
      case op_shellcmd:
	printf("[exec %s]", cond->variable.str);
	break;
      case op_kvariable:
	printf("$%s", cond->variable.cfg->optionname);
	break;
      case op_constant:
	if( strcmp(cond->variable.str, "y") == 0 )
	  printf("1");
	else if( strcmp(cond->variable.str, "n") == 0 )
	  printf("0");
	else if( strcmp(cond->variable.str, "m") == 0 )
	  printf("2");
	else
	  printf("\"%s\"", cond->variable.str);
	break;
      default:
        break;
      }
      cond = cond->next;
    }

  /*
   * Now we generate what we do depending upon the value of the
   * conditional.  Depending upon what the token type is, there are
   * different things we must do write the value the given widget -
   * this code needs to be closely coordinated with the widget
   * creation procedures in header.tk.  
   */
  switch(item->tok)
    {
    case tok_define:
      printf("} then {write_tristate $cfg $autocfg %s %s $notmod }\n", item->optionname, item->value);
      break;
    case tok_comment:
      printf("} then {write_comment $cfg $autocfg \"%s\"}\n", item->label);
      break;
    case tok_dep_tristate:
      printf("} then { write_tristate $cfg $autocfg %s $%s $%s } \n", 
	     item->optionname, item->optionname, item->depend.str);
      break;
    case tok_tristate:
    case tok_bool:
      printf("} then { write_tristate $cfg $autocfg %s $%s $notmod }\n", 
	     item->optionname, item->optionname);
      break;
    case tok_int:
      printf("} then { write_int $cfg $autocfg %s $%s $notmod }\n",
             item->optionname, item->optionname);
      break;
    case tok_hex:
      printf("} then { write_hex $cfg $autocfg %s $%s $notmod }\n",
             item->optionname, item->optionname);
      break;
    case tok_string:
      printf("} then { write_string $cfg $autocfg %s $%s $notmod }\n",
             item->optionname, item->optionname);
      break;
    case tok_choose:
    case tok_choice:
      fprintf(stderr,"Fixme\n");
      exit(0);
    default:
      break;
    }
}

/*
 * Generates a fragment of wish script that closes out a submenu procedure.
 */
static void end_proc(int menu_num)
{
  struct kconfig * cfg;

  printf("\n\n\n");
  printf("\tfocus $w\n");
  printf("\tupdate_menu%d $w.config.f\n", menu_num);
  printf("\tglobal winx; global winy\n");
  printf("\tset winx [expr [winfo x .]+30]; set winy [expr [winfo y .]+30]\n");
  printf("\twm geometry $w +$winx+$winy\n");
  
  /*
   * Now that the whole window is in place, we need to wait for an "update"
   * so we can tell the canvas what its virtual size should be.
   *
   * Unfortunately, this causes some ugly screen-flashing because the whole
   * window is drawn, and then it is immediately resized.  It seems
   * unavoidable, though, since "frame" objects won't tell us their size
   * until after an update, and "canvas" objects can't automatically pack
   * around frames.  Sigh.
   */
  printf("\tupdate idletasks\n");
  printf("\t$w.config.canvas create window 0 0 -anchor nw -window $w.config.f\n\n");
  printf("\t$w.config.canvas configure \\\n"
  	 "\t\t-width [expr [winfo reqwidth $w.config.f] + 1]\\\n"
  	 "\t\t-scrollregion \"-1 -1 [expr [winfo reqwidth $w.config.f] + 1] \\\n"
  	 "\t\t\t [expr [winfo reqheight $w.config.f] + 1]\"\n\n");
  	 
  /*
   * If the whole canvas will fit in 3/4 of the screen height, do it;
   * otherwise, resize to around 1/2 the screen and let us scroll.
   */
  printf("\tset winy [expr [winfo reqh $w] - [winfo reqh $w.config.canvas]]\n");
  printf("\tset scry [expr [winfo screenh $w] / 2]\n");
  printf("\tset maxy [expr [winfo screenh $w] * 3 / 4]\n");
  printf("\tset canvtotal [expr [winfo reqh $w.config.f] + 2]\n");
  printf("\tif [expr $winy + $canvtotal < $maxy] {\n"
  	 "\t\t$w.config.canvas configure -height $canvtotal\n"
  	 "\t} else {\n"
  	 "\t\t$w.config.canvas configure -height [expr $scry - $winy]\n"
  	 "\t}\n");
  
  /*
   * Limit the min/max window size.  Height can vary, but not width,
   * because of the limitations of canvas and our laziness.
   */
  printf("\tupdate idletasks\n");
  printf("\twm maxsize $w [winfo width $w] [winfo screenheight $w]\n");
  printf("\twm minsize $w [winfo width $w] 100\n\n");
  printf("\twm deiconify $w\n");
    
  printf("}\n\n\n");

  /*
   * Now we generate the companion procedure for the menu we just
   * generated.  This procedure contains all of the code to
   * disable/enable widgets based upon the settings of the other
   * widgets, and will be called first when the window is mapped,
   * and each time one of the buttons in the window are clicked.
   */
  printf("proc update_menu%d {w}  {\n", menu_num);

  printf("\tupdate_define\n");
  clear_globalflags(config);
  for(cfg = config;cfg != NULL; cfg = cfg->next)
    {
      /*
       * Skip items not for this menu, or ones having no conditions.
       */
      if (cfg->menu_number != menu_num ) continue;
      if (cfg->tok != tok_define) continue;
      /*
       * Clear all of the booleans that are defined in this menu.
       */
      if(   (cfg->flags & GLOBAL_WRITTEN) == 0
	 && (cfg->optionname != NULL) )
	{
	  printf("\tglobal %s\n", cfg->optionname);
	  cfg->flags |= GLOBAL_WRITTEN;
	  printf("\tset %s 0\n", cfg->optionname);
	}

    }
  for(cfg = config;cfg != NULL; cfg = cfg->next)
    {
      /*
       * Skip items not for this menu, or ones having no conditions.
       */
      if (cfg->menu_number != menu_num ) continue;
      if (cfg->tok == tok_menuoption) continue;
      if (cfg->cond != NULL ) 
	generate_if(cfg, cfg->cond, menu_num, cfg->menu_line);
      else
	{
	  /*
	   * If this token has no conditionals, check to see whether
	   * it is a tristate - if so, then generate the conditional
	   * to enable/disable the "y" button based upon the setting
	   * of the option it depends upon.
	   */
	  if(cfg->tok == tok_dep_tristate)
	    {
	      global(cfg->depend.str);
	      printf("\tif {$%s != 1 && $%s != 0 } then { .menu%d.config.f.x%d.y configure -state disabled } else { .menu%d.config.f.x%d.y configure -state normal}\n",
		     cfg->depend.str,cfg->depend.str,
		     menu_num, cfg->menu_line,
		     menu_num, cfg->menu_line);
	    }
	}

    }


  printf("}\n\n\n");
}

/*
 * This function goes through and counts up the number of items in
 * each submenu. If there are too many options, we need to split it
 * into submenus.  This function just calculates how many submenus,
 * and how many items go in each submenu.
 */
static void find_menu_size(struct kconfig *cfg,
			  int *menu_max, 
			  int *menu_maxlines)

{
  struct kconfig * pnt;
  int tot;
  
  /*
   * First count up the number of options in this menu.
   */
  tot = 0;
  for(pnt = cfg->next; pnt; pnt = pnt->next)
  {
    if( pnt->tok == tok_menuoption) break;
    switch (pnt->tok)
      {
      case tok_bool:
      case tok_tristate:
      case tok_dep_tristate:
      case tok_int:
      case tok_hex:
      case tok_string:
      case tok_choose:
	tot++;
	break;
      case tok_choice:
      default:
	break;
      }
  }

  *menu_max = cfg->menu_number;
  *menu_maxlines = tot;
}

/*
 * This is the top level function for generating the tk script.
 */
void dump_tk_script(struct kconfig *scfg)
{
  int menu_num =0;
  int menu_max =0;
  int menu_min =0;
  int menu_line = 0;
  int menu_maxlines = 0;
  struct kconfig * cfg;
  struct kconfig * cfg1 = NULL;
  char * menulabel = "tkgen error";

  /*
   * Start by assigning menu numbers, and submenu numbers.
   */
  for(cfg = scfg;cfg != NULL; cfg = cfg->next)
    {
      switch (cfg->tok)
	{
	case tok_menuname:
	  break;
	case tok_menuoption:
	  /*
	   * At the start of a new menu, calculate the number of items
	   * we will put into each submenu so we know when to bump the
	   * menu number. The submenus are really no different from a
	   * normal menu, but the top level buttons only access the first
	   * of the chain of menus, and the prev/next buttons are used
	   * access the submenus.
	   */
	  cfg->menu_number = ++menu_num;
	  find_menu_size(cfg, &menu_max, &menu_maxlines);
	  cfg->submenu_start = menu_num;
	  cfg->submenu_end = menu_max;
	  menu_line = 0;
	  break;
	case tok_bool:
	case tok_tristate:
	case tok_dep_tristate:
	case tok_int:
	case tok_hex:
	case tok_string:
	case tok_choose:
	  /*
	   * If we have overfilled the menu, then go to the next one.
	   */
	  if( menu_line == menu_maxlines )
	    {
	      menu_line = 0;
	      menu_num++;
	    }
	  cfg->menu_number = menu_num;
	  cfg->submenu_start = menu_min;
	  cfg->submenu_end = menu_max;
	  cfg->menu_line = menu_line++;
	  break;
	case tok_define:
	  cfg->menu_number = -1;
	case tok_choice:
	default:
	  break;
	};
    }

  /*
   * Record this so we can set up the prev/next buttons correctly.
   */
  tot_menu_num = menu_num;

  /*
   * Now start generating the actual wish script that we will use.
   * We need to keep track of the menu numbers of the min/max menu
   * for a range of submenus so that we can correctly limit the
   * prev and next buttons so that they don't go over into some other
   * category.
   */
  for(cfg = scfg; cfg != NULL; cfg = cfg->next)
    {
      switch (cfg->tok)
	{
	case tok_menuname:
	  printf("mainmenu_name \"%s\"\n", cfg->label);
	  break;
	case tok_menuoption:
	  /*
	   * We are at the start of a new menu. If we had one that
	   * we were working on before, close it out, and then generate
	   * the script to start the new one.
	   */
	  if( cfg->menu_number > 1 )
	    {
	      end_proc(menu_num);
	    }
	  menulabel = cfg->label;
	  start_proc(cfg->label, cfg->menu_number, TRUE);
	  menu_num = cfg->menu_number;
	  menu_max = cfg->submenu_end;
	  menu_min = cfg->submenu_start;
	  break;
	case tok_bool:
	  /*
	   * If we reached the point where we need to switch over
	   * to the next submenu, then bump the menu number and generate
	   * the code to close out the old menu and start the new one.
	   */
	  if( cfg->menu_number != menu_num )
	    {
	      end_proc(menu_num);
	      start_proc(menulabel, cfg->menu_number, FALSE);
	      menu_num = cfg->menu_number;
	    }
	  printf("\tbool $w.config.f %d %d \"%s\" %s\n",
		 cfg->menu_number,
		 cfg->menu_line,
		 cfg->label,
		 cfg->optionname);
	  break;

	case tok_choice:
	  printf("\t$w.config.f.x%d.x.menu add radiobutton -label \"%s\" -variable %s -value \"%s\" -command \"update_menu%d .menu%d.config.f\"\n",
		 cfg1->menu_line,
		 cfg->label,
		 cfg1->optionname,
		 cfg->label,
		 cfg1->menu_number, cfg1->menu_number);
	  break;
	case tok_choose:
	  if( cfg->menu_number != menu_num )
	    {
	      end_proc(menu_num);
	      start_proc(menulabel, cfg->menu_number, FALSE);
	      menu_num = cfg->menu_number;
	    }
	  printf("\tglobal %s\n",cfg->optionname);
	  printf("\tminimenu $w.config.f %d %d \"%s\" %s %s\n",
	  	cfg->menu_number,
	  	cfg->menu_line,
	  	cfg->label,
	  	cfg->optionname,
	  	/*
	  	 * We rely on the fact that the first tok_choice corresponding
	  	 * to the current tok_choose is cfg->next (compare parse() in
	  	 * tkparse.c).  We need its name to pick out the right help
	  	 * text from Configure.help.
	  	 */
	  	cfg->next->optionname);
	  printf("\tmenu $w.config.f.x%d.x.menu\n", cfg->menu_line);
	  cfg1 = cfg;
	  break;
	case tok_tristate:
	  if( cfg->menu_number != menu_num )
	    {
	      end_proc(menu_num);
	      start_proc(menulabel, cfg->menu_number, FALSE);
	      menu_num = cfg->menu_number;
	    }
	  printf("\ttristate $w.config.f %d %d \"%s\" %s\n",
		 cfg->menu_number,
		 cfg->menu_line,
		 cfg->label,
		 cfg->optionname);
	  break;
	case tok_dep_tristate:
	  if( cfg->menu_number != menu_num )
	    {
	      end_proc(menu_num);
	      start_proc(menulabel, cfg->menu_number, FALSE);
	      menu_num = cfg->menu_number;
	    }
	  printf("\tdep_tristate $w.config.f %d %d \"%s\" %s %s\n",
		 cfg->menu_number,
		 cfg->menu_line,
		 cfg->label,
		 cfg->optionname,
		 cfg->depend.str);
	  break;
	case tok_int:
	  if( cfg->menu_number != menu_num )
	    {
	      end_proc(menu_num);
	      start_proc(menulabel, cfg->menu_number, FALSE);
	      menu_num = cfg->menu_number;
	    }
	  printf("\tint $w.config.f %d %d \"%s\" %s\n",
		 cfg->menu_number,
		 cfg->menu_line,
		 cfg->label,
		 cfg->optionname);
	  break;
	case tok_hex:
	  if( cfg->menu_number != menu_num )
	    {
	      end_proc(menu_num);
	      start_proc(menulabel, cfg->menu_number, FALSE);
	      menu_num = cfg->menu_number;
	    }
	  printf("\thex $w.config.f %d %d \"%s\" %s\n",
		 cfg->menu_number,
		 cfg->menu_line,
		 cfg->label,
		 cfg->optionname);
	  break;
	case tok_string:
	  if( cfg->menu_number != menu_num )
	    {
	      end_proc(menu_num);
	      start_proc(menulabel, cfg->menu_number, FALSE);
	      menu_num = cfg->menu_number;
	    }
	  printf("\tistring $w.config.f %d %d \"%s\" %s\n",
		 cfg->menu_number,
		 cfg->menu_line,
		 cfg->label,
		 cfg->optionname);
	  break;
	default:
	  break;
	}

    }

  /*
   * Generate the code to close out the last menu.
   */
  end_proc(menu_num);

  /*
   * The top level menu also needs an update function.  When we exit a
   * submenu, we may need to disable one or more of the submenus on
   * the top level menu, and this procedure will ensure that things are
   * correct.
   */
  printf("proc update_mainmenu {w}  {\n");
  for(cfg = scfg; cfg != NULL; cfg = cfg->next)
    {
      switch (cfg->tok)
	{
	case tok_menuoption:
	  if (cfg->cond != NULL ) 
	    generate_if(cfg, cfg->cond, cfg->menu_number, cfg->menu_line);
	  break;
	default:
	  break;
	}
    }

  printf("}\n\n\n");

#if 0
  /*
   * Generate some code to set the variables that are "defined".
   */
  for(cfg = config;cfg != NULL; cfg = cfg->next)
    {
      /*
       * Skip items not for this menu, or ones having no conditions.
       */
      if( cfg->tok != tok_define) continue;
      if (cfg->cond != NULL ) 
	generate_if(cfg, cfg->cond, menu_num, cfg->menu_line);
      else
	{
	  printf("\twrite_define %s %s\n", cfg->optionname, cfg->value);
	}

    }
#endif

  /*
   * Now generate code to load the default settings into the variables.
   * Note that the script in tail.tk will attempt to load .config,
   * which may override these settings, but that's OK.
   */
  for(cfg = scfg; cfg != NULL; cfg = cfg->next)
    {
      switch (cfg->tok)
	{
	case tok_bool:
	case tok_tristate:
	case tok_dep_tristate:
	case tok_choice:
	  printf("set %s 0\n", cfg->optionname);
	  break;
	case tok_int:
	case tok_hex:
	case tok_string:
	  printf("set %s %s\n", cfg->optionname, cfg->value);
	  break;
	case tok_choose:
	  printf("set %s \"(not set)\"\n",cfg->optionname);
	default:
	  break;
	}
    }

  /*
   * Next generate a function that can be called from the main menu that will
   * write all of the variables out.  This also serves double duty - we can
   * save configuration to a file using this.
   */
  printf("proc writeconfig {file1 file2} {\n");
  printf("\tset cfg [open $file1 w]\n");
  printf("\tset autocfg [open $file2 w]\n");
  printf("\tset notmod 1\n");
  printf("\tset notset 0\n");
  clear_globalflags(config);
  printf("\tputs $cfg \"#\"\n");
  printf("\tputs $cfg \"# Automatically generated make config: don't edit\"\n");
  printf("\tputs $cfg \"#\"\n");

  printf("\tputs $autocfg \"/*\"\n");
  printf("\tputs $autocfg \" * Automatically generated C config: don't edit\"\n");
  printf("\tputs $autocfg \" */\"\n");
  printf("\tputs $autocfg \"#define AUTOCONF_INCLUDED\"\n");
  for(cfg = scfg; cfg != NULL; cfg = cfg->next)
    {
      switch (cfg->tok)
	{
	case tok_int:
	case tok_hex:
	case tok_string:
	case tok_bool:
	case tok_tristate:
	case tok_dep_tristate:
	case tok_define:
	case tok_choose:
	  if(!(cfg->flags & GLOBAL_WRITTEN))
	    {
	      cfg->flags |= GLOBAL_WRITTEN;
	      printf("\tglobal %s\n", cfg->optionname);
	    }
	  /* fall through */
	case tok_comment:
	  if (cfg->cond != NULL ) 
	    generate_if_for_outfile(cfg, cfg->cond);
	  else
	    {
	      if(cfg->tok == tok_dep_tristate)
		{
		  printf("\tif {$%s == 0 } then {\n"
		  	 "\t\twrite_tristate $cfg $autocfg %s $notset $notmod\n"
		  	 "\t} else {\n"
		  	 "\t\twrite_tristate $cfg $autocfg %s $%s $%s\n"
		  	 "\t}\n",
		  	 cfg->depend.str,
			 cfg->optionname,
			 cfg->optionname,
			 cfg->optionname,
			 cfg->depend.str);
		}
	      else if(cfg->tok == tok_comment)
		{
		  printf("\twrite_comment $cfg $autocfg \"%s\"\n", cfg->label);
		}
#if 0
	      else if(cfg->tok == tok_define)
		{
		  printf("\twrite_define %s %s\n", cfg->optionname,
			 cfg->value);
		}
#endif
	      else if (cfg->tok == tok_choose )
		{
		  for(cfg1 = cfg->next; 
		      cfg1 != NULL && cfg1->tok == tok_choice;
		      cfg1 = cfg1->next)
		    {
		      printf("\tif { $%s == \"%s\" } then { write_tristate $cfg $autocfg %s 1 $notmod } else { write_tristate $cfg $autocfg %s 0 $notmod }\n",
			     cfg->optionname,
			     cfg1->label,
			     cfg1->optionname,
			     cfg1->optionname);
		    }
		}
	      else if (cfg->tok == tok_int )
	        {
		  printf("\twrite_int $cfg $autocfg %s $%s $notmod\n",
			 cfg->optionname,
			 cfg->optionname);
	        }
	      else if (cfg->tok == tok_hex )
	        {
		  printf("\twrite_hex $cfg $autocfg %s $%s $notmod\n",
			 cfg->optionname,
			 cfg->optionname);
	        }
	      else if (cfg->tok == tok_string )
	        {
		  printf("\twrite_string $cfg $autocfg %s $%s $notmod\n",
			 cfg->optionname,
			 cfg->optionname);
	        }
	      else
		{
		  printf("\twrite_tristate $cfg $autocfg %s $%s $notmod\n",
			 cfg->optionname,
			 cfg->optionname);
		}
	    }
	  break;
	default:
	  break;
	}
    }
  printf("\tclose $cfg\n");
  printf("\tclose $autocfg\n");
  printf("}\n\n\n");

  /*
   * Finally write a simple function that updates the master choice
   * variable depending upon what values were loaded from a .config
   * file.  
   */
  printf("proc clear_choices { } {\n");
  for(cfg = scfg; cfg != NULL; cfg = cfg->next)
    {
      if( cfg->tok != tok_choose ) continue;
      for(cfg1 = cfg->next; 
	  cfg1 != NULL && cfg1->tok == tok_choice;
	  cfg1 = cfg1->next)
	{
	  printf("\tglobal %s; set %s 0\n",cfg1->optionname,cfg1->optionname);
	}
    }
  printf("}\n\n\n");

  printf("proc update_choices { } {\n");
  for(cfg = scfg; cfg != NULL; cfg = cfg->next)
    {
      if( cfg->tok != tok_choose ) continue;
      printf("\tglobal %s\n", cfg->optionname);
      for(cfg1 = cfg->next; 
	  cfg1 != NULL && cfg1->tok == tok_choice;
	  cfg1 = cfg1->next)
	{
	  printf("\tglobal %s\n", cfg1->optionname);
	  printf("\tif { $%s == 1 } then { set %s \"%s\" }\n",
		 cfg1->optionname,
		 cfg->optionname,
		 cfg1->label);
	}
    }
  printf("}\n\n\n");

  printf("proc update_define { } {\n");
  clear_globalflags(config);
  for(cfg = scfg; cfg != NULL; cfg = cfg->next)
    {
      if( cfg->tok != tok_define ) continue;
      printf("\tglobal %s\n",  cfg->optionname);
      cfg->flags |= GLOBAL_WRITTEN;
    }
  for(cfg = scfg; cfg != NULL; cfg = cfg->next)
    {
      if( cfg->tok != tok_define ) continue;
      if (cfg->cond != NULL ) 
	generate_if(cfg, cfg->cond, -1, 0);
      else
	{
	  printf("\tset %s %s\n",
		 cfg->optionname, cfg->value);
	}
    }
  printf("}\n\n\n");
  /*
   * That's it.  We are done.  The output of this file will have header.tk
   * prepended and tail.tk appended to create an executable wish script.
   */
}
