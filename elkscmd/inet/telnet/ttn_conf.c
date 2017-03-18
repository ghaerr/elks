/*
ttn_conf.c
*/

#include "ttn.h"

#ifndef __linux__
int DO_echo= FALSE;
int DO_echo_allowed= TRUE;
int WILL_terminal_type= FALSE;
int WILL_terminal_type_allowed= TRUE;
int DO_suppress_go_ahead= FALSE;
int DO_suppress_go_ahead_allowed= TRUE;
#else
int DO_echo= TRUE;
int DO_echo_allowed= TRUE;
int WILL_terminal_type= FALSE;
int WILL_terminal_type_allowed= TRUE;
int DO_suppress_go_ahead= TRUE;
int DO_suppress_go_ahead_allowed= TRUE;

#endif