#ifndef NTETRIS_H
#define NTETRIS_H

/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code is NanoTetris.
 * 
 * The Initial Developer of the Original Code is Alex Holden.
 * Portions created by Alex Holden are Copyright (C) 2000, 2002
 * Alex Holden <alex@alexholden.net>. All Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public license (the  "[GNU] License"), in which case the
 * provisions of [GNU] License are applicable instead of those
 * above.  If you wish to allow use of your version of this file only
 * under the terms of the [GNU] License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting  the provisions above and replace  them with the notice and
 * other provisions required by the [GNU] License.  If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the [GNU] License.
 */

/*
 * Anything which is configurable should be in this file.
 * Unfortunately I haven't really bothered to comment anything except for the
 * array of shape descriptions (you can add your own new shapes quite easily).
 */

#ifndef __ECOS
//#define USE_HISCORE_FILE
#define HISCORE_FILE "/usr/games/nanotetris.hiscore"
#endif
#define BLOCK_SIZE 10
#define BORDER_WIDTH 10
#define CONTROL_BAR_WIDTH 90
#define BUTTON_HEIGHT 20
#define BUTTON_WIDTH (CONTROL_BAR_WIDTH - BORDER_WIDTH)
#define BUTTON_BACKGROUND_COLOUR GR_COLOR_GRAY75
#define BUTTON_FOREGROUND_COLOUR GR_COLOR_BLACK
#define MOVEMENT_BUTTON_WIDTH ((BUTTON_WIDTH / 2) - 1)
#define TEXT_X_POSITION 5
#define TEXT_Y_POSITION 15
#define TEXT2_Y_POSITION 30
#define WELL_WIDTH 12
#define WELL_HEIGHT 28
#define WELL_VISIBLE_HEIGHT 24
#define WELL_NOTVISIBLE (WELL_HEIGHT - WELL_VISIBLE_HEIGHT)
#define LEVEL_DIVISOR 500
#ifdef __ECOS
#define DROP_BLOCK_DELAY 10
#else
#define DROP_BLOCK_DELAY 25
#endif
#define DELETE_LINE_DELAY 300
#define SCORE_INCREMENT 100

#define MAIN_WINDOW_X_POSITION 0
#define MAIN_WINDOW_Y_POSITION 0
#define MAIN_WINDOW_WIDTH (CONTROL_BAR_WIDTH + (2 * BORDER_WIDTH) + \
					(WELL_WIDTH * BLOCK_SIZE))
#define MAIN_WINDOW_HEIGHT ((2 * BORDER_WIDTH) + \
					(WELL_VISIBLE_HEIGHT * BLOCK_SIZE))
#define MAIN_WINDOW_BACKGROUND_COLOUR GR_COLOR_NAVYBLUE

#define SCORE_WINDOW_WIDTH BUTTON_WIDTH
#define SCORE_WINDOW_HEIGHT 35
#define SCORE_WINDOW_X_POSITION BORDER_WIDTH
#define SCORE_WINDOW_Y_POSITION BORDER_WIDTH
#define SCORE_WINDOW_BACKGROUND_COLOUR GR_COLOR_BLACK
#define SCORE_WINDOW_FOREGROUND_COLOUR GR_COLOR_GREEN

#define NEXT_SHAPE_WINDOW_SIZE 6
#define NEXT_SHAPE_WINDOW_WIDTH (NEXT_SHAPE_WINDOW_SIZE * BLOCK_SIZE)
#define NEXT_SHAPE_WINDOW_HEIGHT (NEXT_SHAPE_WINDOW_SIZE * BLOCK_SIZE)
#define NEXT_SHAPE_WINDOW_X_POSITION (BORDER_WIDTH + 10)
#define NEXT_SHAPE_WINDOW_Y_POSITION ((2 * BORDER_WIDTH) + SCORE_WINDOW_HEIGHT)
#define NEXT_SHAPE_WINDOW_BACKGROUND_COLOUR GR_COLOR_BLACK

#define NEW_GAME_BUTTON_WIDTH BUTTON_WIDTH
#define NEW_GAME_BUTTON_HEIGHT BUTTON_HEIGHT
#define NEW_GAME_BUTTON_X_POSITION BORDER_WIDTH
#define NEW_GAME_BUTTON_Y_POSITION ((3 * BORDER_WIDTH) + SCORE_WINDOW_HEIGHT \
						+ NEXT_SHAPE_WINDOW_HEIGHT)

#define PAUSE_CONTINUE_BUTTON_WIDTH BUTTON_WIDTH
#define PAUSE_CONTINUE_BUTTON_HEIGHT BUTTON_HEIGHT
#define PAUSE_CONTINUE_BUTTON_X_POSITION BORDER_WIDTH
#define PAUSE_CONTINUE_BUTTON_Y_POSITION ((4 * BORDER_WIDTH) + \
			SCORE_WINDOW_HEIGHT + NEXT_SHAPE_WINDOW_HEIGHT + \
						NEW_GAME_BUTTON_HEIGHT)

#define ANTICLOCKWISE_BUTTON_WIDTH MOVEMENT_BUTTON_WIDTH
#define ANTICLOCKWISE_BUTTON_HEIGHT BUTTON_HEIGHT
#define ANTICLOCKWISE_BUTTON_X_POSITION BORDER_WIDTH
#define ANTICLOCKWISE_BUTTON_Y_POSITION ((5 * BORDER_WIDTH) + \
			SCORE_WINDOW_HEIGHT + NEXT_SHAPE_WINDOW_HEIGHT + \
			NEW_GAME_BUTTON_HEIGHT + PAUSE_CONTINUE_BUTTON_HEIGHT)

#define CLOCKWISE_BUTTON_WIDTH MOVEMENT_BUTTON_WIDTH
#define CLOCKWISE_BUTTON_HEIGHT BUTTON_HEIGHT
#define CLOCKWISE_BUTTON_X_POSITION (ANTICLOCKWISE_BUTTON_X_POSITION + \
					ANTICLOCKWISE_BUTTON_WIDTH + 2)
#define CLOCKWISE_BUTTON_Y_POSITION ANTICLOCKWISE_BUTTON_Y_POSITION

#define LEFT_BUTTON_WIDTH MOVEMENT_BUTTON_WIDTH
#define LEFT_BUTTON_HEIGHT BUTTON_HEIGHT
#define LEFT_BUTTON_X_POSITION BORDER_WIDTH
#define LEFT_BUTTON_Y_POSITION (ANTICLOCKWISE_BUTTON_Y_POSITION + \
				ANTICLOCKWISE_BUTTON_HEIGHT + 2)

#define RIGHT_BUTTON_WIDTH MOVEMENT_BUTTON_WIDTH
#define RIGHT_BUTTON_HEIGHT BUTTON_HEIGHT
#define RIGHT_BUTTON_X_POSITION (LEFT_BUTTON_X_POSITION + LEFT_BUTTON_WIDTH + 2)
#define RIGHT_BUTTON_Y_POSITION LEFT_BUTTON_Y_POSITION

#define DROP_BUTTON_WIDTH BUTTON_WIDTH
#define DROP_BUTTON_HEIGHT BUTTON_HEIGHT
#define DROP_BUTTON_X_POSITION BORDER_WIDTH
#define DROP_BUTTON_Y_POSITION (LEFT_BUTTON_Y_POSITION + LEFT_BUTTON_HEIGHT + 2)

#define WELL_WINDOW_WIDTH (BLOCK_SIZE * WELL_WIDTH)
#define WELL_WINDOW_HEIGHT (BLOCK_SIZE * WELL_VISIBLE_HEIGHT)
#define WELL_WINDOW_X_POSITION (CONTROL_BAR_WIDTH + BORDER_WIDTH)
#define WELL_WINDOW_Y_POSITION BORDER_WIDTH
#define WELL_WINDOW_BACKGROUND_COLOUR GR_COLOR_BLACK

enum {
	STATE_STOPPED,
	STATE_RUNNING,
	STATE_PAUSED,
	STATE_NEWGAME,
	STATE_EXIT,
	STATE_UNKNOWN
};

typedef GR_COLOR block;

struct ntetris_shape {
	int type;
	int orientation;
	GR_COLOR colour;
	int x;
	int y;
};
typedef struct ntetris_shape shape;

struct ntetris_state {
	block blocks[2][WELL_HEIGHT][WELL_WIDTH];
	int score;
	int hiscore;
	int fhiscore;
	int level;
	int state;
	int running_buttons_mapped;
	shape current_shape;
	shape next_shape;
	GR_WINDOW_ID main_window;
	GR_WINDOW_ID score_window;
	GR_WINDOW_ID next_shape_window;
	GR_WINDOW_ID new_game_button;
	GR_WINDOW_ID pause_continue_button;
	GR_WINDOW_ID anticlockwise_button;
	GR_WINDOW_ID clockwise_button;
	GR_WINDOW_ID left_button;
	GR_WINDOW_ID right_button;
	GR_WINDOW_ID drop_button;
	GR_WINDOW_ID well_window;
	GR_GC_ID scoregcf;
	GR_GC_ID scoregcb;
	GR_GC_ID nextshapegcf;
	GR_GC_ID nextshapegcb;
	GR_GC_ID buttongcf;
	GR_GC_ID buttongcb;
	GR_GC_ID wellgc;
	GR_EVENT event;
	struct timeval timeout;
};
typedef struct ntetris_state nstate;

void *my_malloc(size_t size);
void read_hiscore(nstate *state);
void write_hiscore(nstate *state);
int will_collide(nstate *state, int x, int y, int orientation);
void draw_shape(nstate *state, GR_COORD x, GR_COORD y, int erase);
void draw_well(nstate *state, int forcedraw);
void draw_score(nstate *state);
void draw_next_shape(nstate *state);
void draw_new_game_button(nstate *state);
void draw_anticlockwise_button(nstate *state);
void draw_clockwise_button(nstate *state);
void draw_left_button(nstate *state);
void draw_right_button(nstate *state);
void draw_drop_button(nstate *state);
void draw_pause_continue_button(nstate *state);
int block_is_all_in_well(nstate *state);
void delete_line(nstate *state, int line);
void block_reached_bottom(nstate *state);
void move_block(nstate *state, int direction);
void rotate_block(nstate *state, int direction);
void drop_block(nstate *state);
void handle_exposure_event(nstate *state);
void handle_mouse_event(nstate *state);
void handle_keyboard_event(nstate *state);
void handle_event(nstate *state);
void clear_well(nstate *state);
void choose_new_shape(nstate *state);
void new_game(nstate *state);
void init_game(nstate *state);
void calculate_timeout(nstate *state);
unsigned long timeout_delay(nstate *state);
void do_update(nstate *state);
void do_pause(nstate *state);
void wait_for_start(nstate *state);
void run_game(nstate *state);
void main_game_loop(nstate *state);

#define LEVELS 12
static const int delays[] = {600, 550, 500, 450, 400, 350,
				300, 250, 200, 150, 100, 50};
#define MAX_BLOCK_COLOUR 3
static const GR_COLOR block_colours[] = {
	GR_COLOR_RED, GR_COLOR_GREEN, GR_COLOR_BLUE, GR_COLOR_YELLOW
};

/*
 * This isn't the most space efficient way of storing the shape patterns, but
 * it's quick and easy to parse, and only takes a few hundred bytes of code
 * space anyway. 0 means "not filled", 1 means "filled", 2 means "go to next
 * line", and 3 means "end of pattern". There are four patterns for each shape; 
 * one for each orientation. Adding new shapes is quite easy- just increase
 * MAXSHAPES by the number of shapes you add, then add the four pattern
 * descriptions (each one should be the previous one rotated clockwise) for
 * each shape. If you need to use more than MAXROWS rows (including the "end
 * of line" marker), increase MAXROWS. Ditto for MAXCOLS. Increasing
 * MAXORIENTATIONS (to get more than four orientations) may work, but it's
 * untested, and remember that if you increase it you'll have to add the new
 * orientations for all of the original shapes as well as your new ones. Also
 * remember to add the new shapes the the array of shape sizes below.
 */
#define MAXSHAPES 7
#define MAXORIENTATIONS 4
#define MAXCOLS 5
#define MAXROWS 4
static const char shapes[MAXSHAPES][MAXORIENTATIONS][MAXROWS][MAXCOLS] = {
	{ /* **** */
		{
			{1,1,1,1,3}
		},
		{
			{1,2},
			{1,2},
			{1,2},
			{1,3}
		},
		{
			{1,1,1,1,3}
		},
		{
			{1,2},
			{1,2},
			{1,2},
			{1,3}
		}
	},
	{ /*    *
	      ***  */
		{
			{0,0,1,2},
			{1,1,1,3}
		},
		{
			{1,2},
			{1,2},
			{1,1,3}
		},
		{
			{1,1,1,2},
			{1,3}
		},
		{
			{1,1,2},
			{0,1,2},
			{0,1,3}
		}
	},
	{ /*  *
	      ***  */
		{
			{1,2},
			{1,1,1,3}
		},
		{
			{1,1,2},
			{1,2},
			{1,3}
		},
		{
			{1,1,1,2},
			{0,0,1,3}
		},
		{
			{0,1,2},
			{0,1,2},
			{1,1,3}
		}
	},
	{ /*  **
	       **  */
		{
			{1,1,2},
			{0,1,1,3}
		},
		{
			{0,1,2},
			{1,1,2},
			{1,3}
		},
		{
			{1,1,2},
			{0,1,1,3}
		},
		{
			{0,1,2},
			{1,1,2},
			{1,3}
		}
	},
	{ /*   **
	      **   */
		{
			{0,1,1,2},
			{1,1,3}
		},
		{
			{1,2},
			{1,1,2},
			{0,1,3}
		},
		{
			{0,1,1,2},
			{1,1,3}
		},
		{
			{1,2},
			{1,1,2},
			{0,1,3}
		}
	},
	{ /*  **
	      **  */
		{
			{1,1,2},
			{1,1,3}
		},
		{
			{1,1,2},
			{1,1,3}
		},
		{
			{1,1,2},
			{1,1,3}
		},
		{
			{1,1,2},
			{1,1,3}
		}
	},
	{ /*   *
	      ***  */
		{
			{0,1,2},
			{1,1,1,3}
		},
		{
			{1,2},
			{1,1,2},
			{1,3}
		},
		{
			{1,1,1,2},
			{0,1,3}
		},
		{
			{0,1,2},
			{1,1,2},
			{0,1,3}
		}
	}
};
static const unsigned char shape_sizes[MAXSHAPES][MAXORIENTATIONS][2] = {
	{{4,1},{1,4},{4,1},{1,4}},
	{{3,2},{2,3},{3,2},{2,3}},
	{{3,2},{2,3},{3,2},{2,3}},
	{{3,2},{2,3},{3,2},{2,3}},
	{{3,2},{2,3},{3,2},{2,3}},
	{{2,2},{2,2},{2,2},{2,2}},
	{{3,2},{2,3},{3,2},{2,3}}
};
#endif
