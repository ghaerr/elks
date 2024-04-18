#include "ttypong.h"

unsigned long speed = 40000L;

void
init_curses(void)
{
    int bg;

    initscr();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(FALSE);
    clear();

    /* On check la taille du terminal */
    if(COLS < FW || LINES < FH)
    {
        endwin();
        fprintf(stderr, "Terminal too small (request %dx%d).\n", FH, FW);
        free(pong);
        exit(EXIT_FAILURE);
    }

    start_color();
    bg = (use_default_colors() == OK) ? -1 : COLOR_BLACK;
    init_pair(0, COLOR_GREEN, bg);
    init_pair(BALL,   COLOR_WHITE, bg);
    init_pair(WALL,   COLOR_WHITE, COLOR_WHITE);
    init_pair(RACKET, bg, COLOR_GREEN);
    init_pair(OUT,    bg, bg);

    return;
}

/* Initialisation de la frame */
void
init_frame(void)
{
    int i, j;

    for(i = 0; i <= FH; ++i)
        for(j = 0; j <= FW; ++j)
            pong->frame[i][j] = 0;

    for(i = 0; i <= FW; ++i)
        pong->frame[0][i] = pong->frame[FH][i] = WALL;

    racket_move((FH / 2) - RACKETL / 2 - 1);

    return;
}

void
draw_frame(void)
{
    int i, j;
    int c, v, lastv = -1;

    for(i = 0; i <= FH; ++i)
        for(j = 0; j <= FW; ++j)
        {
            v = pong->frame[i][j];
            if (v != lastv) {
                COL(v);
                lastv = v;
            }
            c = (v == BALL) ? 'O' : ' ';
            if (j == 0)
                mvaddch(i, j + GX, c);
            else addch(c);
        }
    return;
}

void
key_event(void)
{
    int c;

    switch((c = getch()))
    {
        case KEY_UP:
        case 'k':
            racket_move(pong->racket.y - 1);
            break;

        case KEY_DOWN:
        case 'j':
            racket_move(pong->racket.y + 1);
            break;

             /* Touche q ou Q pour quitter */
        case 'q':
        case 'Q':
            pong->running = 0;
            break;

        /* Pause avec p ou P .. */
        case 'p':
        case 'P':
            /* On attends tant que getch() soit pas egale à p ou P */
            while(getch() != c);
            break;
    }

    return;
}

/* C'est ici que l'on fait rebondir la balle */
void
manage_ball(void)
{
    if(pong->ball.x < 2
      || pong->ball.x > FW - 2)
        pong->ball.a = -pong->ball.a;
    if(pong->ball.y < 2
      || pong->ball.y > FH - 2)
        pong->ball.b = -pong->ball.b;

    /* On efface la balle à son ancienne position */
    pong->frame[pong->ball.y][pong->ball.x] = 0;

    pong->ball.x += pong->ball.a;
    pong->ball.y += pong->ball.b;

    pong->frame[pong->ball.y][pong->ball.x] = BALL;

    refresh();

    return;
}

/* Movement de la raquette */
void
racket_move(int y)
{
    int i, offset = y - pong->racket.y;

    if(y <= 0 || y > FH - RACKETL)
        return;

    if(offset >= RACKETL
      || offset <= -RACKETL)
    {
        for(i = pong->racket.y; i < pong->racket.y + RACKETL; ++i)
            pong->frame[i][0] = 0;
        for(i = y; i < y + RACKETL; ++i)
            pong->frame[i][0] = RACKET;
    }
    else if(offset > 0)
    {
        for(i = pong->racket.y; i < y; ++i)
            pong->frame[i][0] = 0;
        for(i = pong->racket.y + RACKETL; i < y + RACKETL; ++i)
            pong->frame[i][0] = RACKET;
    }
    else
    {
        for(i = y + RACKETL; i < pong->racket.y + RACKETL; ++i)
            pong->frame[i][0] = 0;
        for(i = y; i < pong->racket.y; ++i)
            pong->frame[i][0] = RACKET;
    }

    pong->racket.y = y;

    return;

}

/* Raquette de l'IA (enfin IA...) */
void
ia_racket(void)
{
    int i;

    pong->racket.iay = pong->ball.y - (RACKETL / 2);

    if(pong->ball.y < 4)
        ++pong->racket.iay;
    if(pong->ball.y > FH - 3)
        --pong->racket.iay;

    for(i = 1; i < FH; ++i)
        pong->frame[i][FW] = 0;

    for(i = 0; i < RACKETL; ++i)
        pong->frame[i + pong->racket.iay][FW] = RACKET;

    return;
}

int
main(int argc, char **argv)
{
    int time;

    if (argc > 1)
        speed = atol(argv[1]);

    pong = malloc(sizeof(Pong));

    pong->running = 1;
    pong->ball.a = 1;
    pong->ball.b = -1;
    pong->ball.x = 10;
    pong->ball.y = 10;
    pong->racket.y = 1;

    init_curses();
    init_frame();

    /* Boucle principale */
    for(time = 0; pong->running; ++time)
    {
        key_event();

        /*if(time > TIMEDELAY) {*/
            ia_racket();
            manage_ball();
            time = 0;
        /*}*/

        draw_frame();
        usleep(speed);
    }

    move(LINES-2, 0);
    endwin();
    free(pong);

    return 1;
}

