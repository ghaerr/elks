/************************************************************************
 *   IRC - Internet Relay Chat, include/numeric.h
 *   Copyright (C) 1990 Jarkko Oikarinen
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * -- Avalon -- 1 Sep 1992
 *
 * Added RPL_TRACELOG, RPL_STATSOLINE
 */

/*
 * -- Avalon -- 13 Aug 1992
 *
 * Added ERR_BADCHANNELKEY, ERR_KEYSET
 */

/*
 * -- Avalon -- 10 Aug 1992
 *
 * Added RPL_SUMMONING
 */

/*
 * -- Avalon -- 5  Jul 1992
 *
 * Added ERR_NICKCOLLISION
 */

/*
 * -- Avalon -- 14 Jul 1992
 *
 * Added RPL_UNAWAY, RPL_NOWAWAY, ERR_NOORIGIN, ERR_FILEERROR, ERR_NOLOGIN,
 * ERR_SUMMONDISABLED, ERR_USERSDISABLED, RPL_USERSSTART, RPL_USERS,
 * RPL_ENDOFUSERS, RPL_NOUSERS
 */

/*
 * -- Avalon -- 12 Jul 1992
 *
 * Added RPL_CLOSING RPL_CLOSEEND
 */

/*
 * -- Avalon -- 10-11 Jul 1992
 *
 * Added RPL_MOTD, RPL_MOTDSTART, RPL_ENDOFMOTD, ERR_NOMOTD,
 * RPL_INFO, RPL_INFOSTART, RPL_ENDOFINFO, ERR_CANTKILLSERVER,
 * RPL_LUSERCLIENT, RPL_LUSEROP, RPL_LUSERUNKNOWN, RPL_LUSERCHAN, RPL_LUSERME,
 * RPL_STATSUPTIME, RPL_ADMINLOC1, RPL_ADMINLOC2, RPL_ADMINME,
 * RPL_ADMINEMAIL, ERR_NOADMININFO
 */

/*
 * -- Avalon -- 28 Jun 1992
 *
 * Added ERR_BADCHANMASK and RPL_ENDOFWHOWAS
 */

/*
 * -- Avalon -- 13 May 1992
 *
 * Added RPL_STATSLLINE
 */

/*
 * -- Avalon -- 12 Jan 1992
 *
 * Added RPL_TRACELINK
 */

/*
 * -- Wumpus -- 30 Nov 1991
 *
 * It's very important that you never change what a numeric means --
 * you can delete old ones (maybe) and add new ones, but never ever
 * take a number and make it suddenly mean something else, or change
 * an old number just for the hell of it.
 */

/*
 * -- avalon -- 19 Nov 1991
 * Added ERR_USERSDONTMATCH 
 *
 * -- avalon -- 06 Nov 1991
 * Added RPL_BANLIST, RPL_BANLISTEND, ERR_BANNEDFROMCHAN
 *
 * -- avalon -- 15 Oct 1991
 * Added RPL_TRACEs (201-209)
 * Added RPL_STATSs (211-219)
 */

/* -- Jto -- 16 Jun 1990
 * A couple of new numerics added...
 */

/* -- Jto -- 03 Jun 1990
 * Added ERR_YOUWILLBEBANNED and Check defines (sigh, had to put 'em here..)
 * Added ERR_UNKNOWNMODE...
 * Added ERR_CANNOTSENDTOCHAN...
 */

/*
 * Reserve numerics 000-099 for server-client connections where the client
 * is local to the server. If any server is passed a numeric in this range
 * from another server then it is remapped to 100-199. -avalon
 */
#define	RPL_WELCOME          001
#define	RPL_YOURHOST         002
#define	RPL_CREATED          003
#define	RPL_MYINFO           004

/*
 * Errors are in the range from 400-599 currently and are grouped by what
 * commands they come from.
 */
#define ERR_NOSUCHNICK       401
#define ERR_NOSUCHSERVER     402
#define ERR_NOSUCHCHANNEL    403
#define ERR_CANNOTSENDTOCHAN 404
#define ERR_TOOMANYCHANNELS  405
#define ERR_WASNOSUCHNICK    406
#define ERR_TOOMANYTARGETS   407
#define ERR_NOSUCHSERVICE    408
#define	ERR_NOORIGIN         409

#define ERR_NORECIPIENT      411
#define ERR_NOTEXTTOSEND     412
#define ERR_NOTOPLEVEL       413
#define ERR_WILDTOPLEVEL     414

#define ERR_UNKNOWNCOMMAND   421
#define	ERR_NOMOTD           422
#define	ERR_NOADMININFO      423
#define	ERR_FILEERROR        424

#define ERR_NONICKNAMEGIVEN  431
#define ERR_ERRONEUSNICKNAME 432
#define ERR_NICKNAMEINUSE    433
#define ERR_SERVICENAMEINUSE 434
#define ERR_SERVICECONFUSED  435
#define	ERR_NICKCOLLISION    436

#define ERR_USERNOTINCHANNEL 441
#define ERR_NOTONCHANNEL     442
#define	ERR_USERONCHANNEL    443
#define ERR_NOLOGIN          444
#define	ERR_SUMMONDISABLED   445
#define ERR_USERSDISABLED    446

#define ERR_NOTREGISTERED    451

#define ERR_NEEDMOREPARAMS   461
#define ERR_ALREADYREGISTRED 462
#define ERR_NOPERMFORHOST    463
#define ERR_PASSWDMISMATCH   464
#define ERR_YOUREBANNEDCREEP 465
#define ERR_YOUWILLBEBANNED  466
#define	ERR_KEYSET           467

#define ERR_CHANNELISFULL    471
#define ERR_UNKNOWNMODE      472
#define ERR_INVITEONLYCHAN   473
#define ERR_BANNEDFROMCHAN   474
#define	ERR_BADCHANNELKEY    475
#define	ERR_BADCHANMASK      476

#define ERR_NOPRIVILEGES     481
#define ERR_CHANOPRIVSNEEDED 482
#define	ERR_CANTKILLSERVER   483

#define ERR_NOOPERHOST       491
#define ERR_NOSERVICEHOST    492

#define ERR_UMODEUNKNOWNFLAG 501
#define ERR_USERSDONTMATCH   502

/*
 * Numberic replies from server commands.
 * These are currently in the range 200-399.
 */
#define	RPL_NONE             300
#define RPL_AWAY             301
#define RPL_USERHOST         302
#define RPL_ISON             303
#define RPL_TEXT             304
#define	RPL_UNAWAY           305
#define	RPL_NOWAWAY          306

#define RPL_WHOISUSER        311
#define RPL_WHOISSERVER      312
#define RPL_WHOISOPERATOR    313

#define RPL_WHOWASUSER       314
/* rpl_endofwho below (315) */
#define	RPL_ENDOFWHOWAS      369

#define RPL_WHOISCHANOP      316 /* redundant and not needed but reserved */
#define RPL_WHOISIDLE        317

#define RPL_ENDOFWHOIS       318
#define RPL_WHOISCHANNELS    319

#define RPL_LISTSTART        321
#define RPL_LIST             322
#define RPL_LISTEND          323
#define RPL_CHANNELMODEIS    324

#define RPL_NOTOPIC          331
#define RPL_TOPIC            332

#define RPL_INVITING         341
#define	RPL_SUMMONING        342

#define RPL_VERSION          351

#define RPL_WHOREPLY         352
#define RPL_ENDOFWHO         315
#define RPL_NAMREPLY         353
#define RPL_ENDOFNAMES       366

#define RPL_KILLDONE         361
#define	RPL_CLOSING          362
#define RPL_CLOSEEND         363
#define RPL_LINKS            364
#define RPL_ENDOFLINKS       365
/* rpl_endofnames above (366) */
#define RPL_BANLIST          367
#define RPL_ENDOFBANLIST     368
/* rpl_endofwhowas above (369) */

#define	RPL_INFO             371
#define	RPL_MOTD             372
#define	RPL_INFOSTART        373
#define	RPL_ENDOFINFO        374
#define	RPL_MOTDSTART        375
#define	RPL_ENDOFMOTD        376

#define RPL_YOUREOPER        381
#define RPL_REHASHING        382
#define RPL_YOURESERVICE     383
#define RPL_MYPORTIS         384
#define RPL_NOTOPERANYMORE   385

#define RPL_TIME             391
#define	RPL_USERSSTART       392
#define	RPL_USERS            393
#define	RPL_ENDOFUSERS       394
#define	RPL_NOUSERS          395

#define RPL_TRACELINK        200
#define RPL_TRACECONNECTING  201
#define RPL_TRACEHANDSHAKE   202
#define RPL_TRACEUNKNOWN     203
#define RPL_TRACEOPERATOR    204
#define RPL_TRACEUSER        205
#define RPL_TRACESERVER      206
#define RPL_TRACESERVICE     207
#define RPL_TRACENEWTYPE     208
#define RPL_TRACECLASS       209

#define RPL_STATSLINKINFO    211
#define RPL_STATSCOMMANDS    212
#define RPL_STATSCLINE       213
#define RPL_STATSNLINE       214
#define RPL_STATSILINE       215
#define RPL_STATSKLINE       216
#define RPL_STATSQLINE       217
#define RPL_STATSYLINE       218
#define RPL_ENDOFSTATS       219

#define RPL_UMODEIS          221

#define RPL_SERVICEINFO      231
#define RPL_ENDOFSERVICES    232
#define	RPL_SERVICE          233
#define RPL_SERVLIST         234
#define RPL_SERVLISTEND      235

#define	RPL_STATSLLINE       241
#define	RPL_STATSUPTIME      242
#define	RPL_STATSOLINE       243
#define	RPL_STATSHLINE       244
#define	RPL_STATSSLINE       245

#define	RPL_LUSERCLIENT      251
#define RPL_LUSEROP          252
#define	RPL_LUSERUNKNOWN     253
#define	RPL_LUSERCHANNELS    254
#define	RPL_LUSERME          255
#define	RPL_ADMINME          256
#define	RPL_ADMINLOC1        257
#define	RPL_ADMINLOC2        258
#define	RPL_ADMINEMAIL       259

#define	RPL_TRACELOG         261
