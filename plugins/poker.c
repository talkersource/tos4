/* TalkerOS Plugin                    for version 4.01 or higher
   -------------------------------------------------------------
   Originally written by Robb Thomas.
   Plugin-compatible modifications by "William".
   100% compatible with TalkerOS ver 4.x.

   The initialization line for this plugin is:

           if (tmp=plugin_01x000_init(cmd)) cmd=cmd+tmp;

   The command call for this plugin is:

           if (!strcmp(plugin->registration,"01-000")) { plugin_01x000_main(user,str,comnum); return 1; }

   The following line MUST be added INSIDE the user structure in
   the tos401.h file:

        struct plugin_01x000_po_player *plugin_01x000_poker;
   ------------------------------------------------------------- */

#include "poker.h"

   extern UR_OBJECT create_user();
   extern UR_OBJECT get_user(char[]);
   /* extern struct plugin_01x000_po_game *plugin_01x000_get_po_game_here(RM_OBJECT); */
   extern CM_OBJECT create_cmd();
   extern PL_OBJECT create_plugin();

/* ------------------------------------------------------------- */

plugin_01x000_init(cm)
int cm;
{
PL_OBJECT plugin;
CM_OBJECT com;
int i,verFail;
i=0; verFail=0;
/* create plugin */
if ((plugin=create_plugin())==NULL) {
        write_syslog("ERROR: Unable to create new registry entry!\n",0,SYSLOG);
        return 0;
	}
strcpy(plugin->name,"5-Card Draw Poker");       /* Plugin Description   */
strcpy(plugin->author,"R.Thomas/W.Price");      /* Author's name        */
strcpy(plugin->registration,"01-000");          /* Plugin/Author ID     */
strcpy(plugin->ver,"1.0");                      /* Plugin version       */
strcpy(plugin->req_ver,"401");                  /* TOS version required */
plugin->id = cm;                                /* ID used as reference */
plugin->req_userfile = 1;                       /* Requires user data?  */
                                                /* (no separate file required
                                                    since it keeps its data
                                                    in a central file, but
                                                    we need to do housekeeping
                                                    procedures when the user
                                                    leaves, so we set this to
                                                    1 so that we are notified
                                                    when a user leaves.) */
plugin->triggerable = 1;                        /* This plugin is triggered
                                                   by the system timer, and
                                                   it will automatically
                                                   save the current poker
                                                   data when the boards
                                                   are automatically
                                                   checked. */
/* create associated command */
if ((com=create_cmd())==NULL) {
        write_syslog("ERROR: Unable to add command to registry!\n",0,SYSLOG);
        return 0;
	}
i++;                                            /* Keep track of number created */
strcpy(com->command,"poker");                   /* Name of command */
com->id = plugin->id;                           /* Command reference ID */
com->req_lev = XUSR;                            /* Required level for cmd. */
com->comnum = i;
com->plugin = plugin;
/* end creating command - repeat as needed for more commands */

/* initializing constant data */
plugin_01x000_po_game_first=NULL;
plugin_01x000_po_game_last=NULL; 
plugin_01x000_max_po_hist=0;     

/* load previously saved data */
if (plugin_01x000_load_po_hist()) write_syslog("PLUGIN:  Previously saved data from POKER loaded.\n",0,SYSLOG);
        else write_syslog("PLUGIN:  Failed to find and/or load POKER data.\n",0,SYSLOG);

return i;
}

plugin_01x000_main(user,str,comid)
UR_OBJECT user;
char *str;
int comid;
{
char filename[80];
sprintf(filename,"%s/%s.X",USERFILES,user->name);
switch (comid) {
        /* put case calls here for each command needed.
           remember that the commands will be in order, with the first
           command always being 1;
        */
        case -5: plugin_01x000_check_poker_hist(user); return 1;/* timed trigger */
        case -6: plugin_01x000_save_po_hist(NULL); return 1;
        case -7:
        case -8: if (user->plugin_01x000_poker!=NULL) plugin_01x000_leave_po(user);
                 unlink(filename);
                 return 1;
        case -9: plugin_01x000_signon(user); return 1;
        case  1: plugin_01x000_switch(user,str); return 1;
        default: return 0;
        }
}

/* Main Poker Command Switch.. combines individual commands into one or
   more instead. */
plugin_01x000_switch(user,str)
UR_OBJECT user;
char *str;
{
int i,pokcmd;
char options[15][20]={"start","join","leave","games","score","deal","fold",
                      "bet","check","raise","see","discard","hand","chips",
                      "ranks"};

pokcmd=-1;
for (i=0; i<15; i++) {
        if (!strncmp(options[i],word[1],strlen(word[1]))) { pokcmd=i; break; }
        }
if (pokcmd==-1 || word_count<2) {
        pokcmd=0;
        write_user(user,"-=-  Poker Commands -=-\n\n");
        for (i=0; i<15; i++) {
                if (i==13 && user->level<3) continue;
                sprintf(text,"  %-8s",options[i]);
                write_user(user,text);
                pokcmd++;
                if (pokcmd==7) { write_user(user,"\n"); pokcmd=0; }
                }
        if (pokcmd) write_user(user,"\n");
        write_user(user,"\n");
        return;
        }
switch (pokcmd) {
        case  0: if (user->room->access==FIXED_PUBLIC) write_user(user,"Please start the poker game in another room.\n");
                        else plugin_01x000_start_po(user);
                 break;
        case  1: if (user->room->access==FIXED_PUBLIC) write_user(user,"Please join the poker game in another room.\n");
                        else plugin_01x000_join_po(user);
                 break;
        case  2: plugin_01x000_leave_po(user); break;
        case  3: plugin_01x000_list_po_games(user); break;
        case  4: plugin_01x000_show_po_players(user); break;
        case  5: plugin_01x000_deal_po(user); break;
        case  6: plugin_01x000_fold_po(user,str); break;
        case  7: plugin_01x000_bet_po(user); break;
        case  8: plugin_01x000_check_po(user); break;
        case  9: plugin_01x000_raise_po(user); break;
        case 10: plugin_01x000_see_po(user); break;
        case 11: plugin_01x000_disc_po(user); break;
        case 12: plugin_01x000_hand_po(user); break;
        case 13: if (user->level < 3) write_user(user,"Unknown command.\n");
                        else plugin_01x000_chips_po(user);
                 break;
        case 14: plugin_01x000_rank_po(user); break;
        default: write_user(user,"Error.  Poker command not executed.\n");
        }
}

plugin_01x000_signon(user)
UR_OBJECT user;
{
user->plugin_01x000_poker=NULL;
}


struct plugin_01x000_po_game *plugin_01x000_get_po_game_here(room)
RM_OBJECT room;
{
struct plugin_01x000_po_game *game;
        
/* Search for game in the room */
for(game=plugin_01x000_po_game_first;game!=NULL;game=game->next) {
        if (game->room == room)  return game;   
        }
return NULL;
}

/*** Get po_game struct pointer from name ***/
struct plugin_01x000_po_game *plugin_01x000_get_po_game(name)
char *name;
{
struct plugin_01x000_po_game *game;

name[0]=toupper(name[0]);
/* Search for exact name */
for(game=plugin_01x000_po_game_first;game!=NULL;game=game->next) {
        if (!strcmp(game->name,name))  return game;
        }
/* Search for close match name */
for(game=plugin_01x000_po_game_first;game!=NULL;game=game->next) {
        if (strstr(game->name,name))  return game;
        }
return NULL;
}

/*** Create a po game ***/
struct plugin_01x000_po_game *plugin_01x000_create_po_game()
{
struct plugin_01x000_po_game *game;
int i;

if ((game=(struct plugin_01x000_po_game *)malloc(sizeof(struct plugin_01x000_po_game)))==NULL) {
        write_syslog("ERROR: Memory allocation failure in create_po_game().\n",0,SYSLOG);
        return NULL;
        }

/* Append object into linked list. */
if (plugin_01x000_po_game_first==NULL) {  
        plugin_01x000_po_game_first=game;  game->prev=NULL;  
        }
else {  
        plugin_01x000_po_game_last->next=game;  game->prev=plugin_01x000_po_game_last;  
        }
game->next=NULL;
plugin_01x000_po_game_last=game;

/* initialise the game */
game->name[0]='\0';
game->room=NULL;
game->players=NULL;
game->dealer=NULL;
game->newdealer=0;
game->num_players=0;
game->num_raises=0;
game->top_card=0;
game->bet=0;
game->pot=0;
game->state=0;
game->curr_player=NULL;
game->first_player=NULL;
game->last_player=NULL;

return game;
}


/*** Destruct a po game. ***/
plugin_01x000_destruct_po_game(game)
struct plugin_01x000_po_game *game;
{
/* Remove from linked list */
if (game==plugin_01x000_po_game_first) {
        plugin_01x000_po_game_first=game->next;
        if (game==plugin_01x000_po_game_last) plugin_01x000_po_game_last=NULL;
        else plugin_01x000_po_game_first->prev=NULL;
        }
else {
        game->prev->next=game->next;
        if (game==plugin_01x000_po_game_last) { 
                plugin_01x000_po_game_last=game->prev;  plugin_01x000_po_game_last->next=NULL; 
                }
        else game->next->prev=game->prev;
        }
sprintf(text, "# Game %s has ended.\n", game->name);
write_room(game->room, text);

sprintf(text, "Poker game %s ended.\n", game->name);
write_syslog(text,1,SYSLOG);

free(game);
}


/*** Create a po player ***/
struct plugin_01x000_po_player *plugin_01x000_create_po_player(game)
struct plugin_01x000_po_game *game;
{
struct plugin_01x000_po_player *player;
int i;

if ((player=(struct plugin_01x000_po_player *)malloc(sizeof(struct plugin_01x000_po_player)))==NULL) {
        write_syslog("ERROR: Memory allocation failure in create_po_player().\n",0,SYSLOG);
        return NULL;
        }

/* Append object into linked list. */
if (game->first_player==NULL) {
        game->first_player=player;  player->prev=NULL;  
        }
else {  
        game->last_player->next=player;  player->prev=game->last_player;  
        }
player->next=NULL;
game->last_player=player;

/* Keep track of num players */
game->num_players++;

/* initialise the player */
player->hand[0] = -1;
player->touched = 0;
player->putin = 0;
player->rank = 0;
player->user = NULL;
player->game = game;

return player;
}

/*** Destruct a po player. ***/
plugin_01x000_destruct_po_player(player)
struct plugin_01x000_po_player *player;
{
  struct plugin_01x000_po_game *game = player->game;
   /* if there are other players   */
   /* pass the turn before leaving */

  /* Keep track of num players */
  game->num_players--;

  sprintf(text, "# You leave the game %s.\n", game->name);
  write_user(player->user, text);
  sprintf(text, "# Player %s leaves the game %s.\n",player->user->name,game->name);
  write_room_except(player->user->room, text, player->user);

  /* Remove from linked list */
  if (player==game->first_player) {
        game->first_player=player->next;
        if (player==game->last_player) game->last_player=NULL;
        else game->first_player->prev=NULL;
  }
  else {
        player->prev->next=player->next;
        if (player==game->last_player) {
          game->last_player=player->prev; game->last_player->next=NULL;
        }
        else player->next->prev=player->prev;
  }

  if (game->num_players >= 1) {
        if (player->hand[0] != -1) {
          word_count = 2;
          plugin_01x000_fold_po(player->user);
        }

        /* Pass the turn */
        if (game->curr_player == player)
          plugin_01x000_next_po_player(game);

        /* Pass the honor of dealing */
        if (game->dealer == player)
          plugin_01x000_pass_the_deal(game);
  }

  if (game->state == 0 && game->num_players!=0) {
        game->curr_player = game->dealer;
        sprintf(text, "~OL# It's your turn to deal.\n");
        write_user(game->dealer->user, text);
        sprintf(text, "# It's %s's turn to deal.\n",
                        game->dealer->user->name);
        write_room_except(game->room, text, game->dealer->user);
  }
  player->user->plugin_01x000_poker=NULL;
  free(player);

  /* If the last player left, destruct the game */
  if (game->num_players==1){
        plugin_01x000_destruct_po_player(game->first_player);
        return;
        }
  if (game->first_player==NULL)
        plugin_01x000_destruct_po_game(game);

}


/*** Shuffle cards ***/
plugin_01x000_shuffle_cards(int deck[])
{
  int i, j, k, tmp;
  
  /* init the deck */
  for (i = 0; i < 52; i++) 
        deck[i] = i;

  /* do this 7 times */
  for (k = 0; k < 7; k++) {
        /* Swap a random card below */
        /* the ith card with the ith card */
        for (i = 0; i < 52; i++) {
          j = plugin_01x000_myRand(52-i);
          tmp = deck[j];   
          deck[j] = deck[i];
          deck[i] = tmp;
        }
  }
}

/*** look at hand ***/
plugin_01x000_hand_po(user)
UR_OBJECT user;
{
  if (user->plugin_01x000_poker == NULL) {
        write_user(user, "POKER: Please start/join a game first.\n");
        return;
  }

  if (user->plugin_01x000_poker->hand[0] == -1) {
        write_user(user, "POKER: You don't have any cards.\n");
        return;
  }

  plugin_01x000_print_hand(user, user->plugin_01x000_poker->hand);
}

/*** Print hand ***/
plugin_01x000_print_hand(user, hand)
UR_OBJECT user;
int hand[];
{
  int i, j, k;
  
  sprintf(text, "");
  for (i = 0; i < plugin_01x000_CARD_LENGTH; i++) {
        for (j = 0; j < 5; j++) {
          strcat(text, plugin_01x000_cards[hand[j]][i]);
          strcat(text, " ");
        }
        strcat(text, "\n");
        write_user(user,text);
        sprintf(text, "");
  }
  
  /** Print the labels */
  write_user(user, "  1     2     3     4     5  \n");
}


/*** Print hand for the room ***/
plugin_01x000_room_print_hand(user, hand)
UR_OBJECT user;
int hand[];
{
  int i, j, k;
  
  sprintf(text, "");
  for (i = 0; i < 3; i++) {
        for (j = 0; j < 5; j++) {
          strcat(text, plugin_01x000_cards[hand[j]][i]);
          strcat(text, " ");
        }
        strcat(text, "\n");
        write_room(user->room,text);
        sprintf(text, "");
  }
}

/*** Start a poker Game ***/
plugin_01x000_start_po(user)
UR_OBJECT user;
{
  struct plugin_01x000_po_game *game;
  struct plugin_01x000_cw_game *cwgame;
  int hist_index;
  
  if ((hist_index = plugin_01x000_get_po_player_hist_index(user)) == -1) {
        write_user(user, "POKER: You do not have any chips to play with.\n");
        return;
  }

  if (word_count < 3) {
        write_user(user, "Usage:  poker start <GameName>\n");
        return;
  }
  if (strlen(word[2])>=15) {
        write_user(user, "POKER: Game name is too long!\n");
        return;}

  if ((game=plugin_01x000_get_po_game(word[2]))!=NULL) {
        write_user(user, "POKER: Game already exists with that name.\n");
        return;
  }

  if ((game=plugin_01x000_get_po_game_here(user->room))!=NULL) {
        write_user(user, "POKER: This room is already in use.  Please choose another.\n");
        return;
  }
  /*
  if ((cwgame=plugin_01x000_get_cw_game(word[2]))!=NULL) {
        write_user(user, "POKER: Game already exists with that name.\n");
        return;
  }
  
  if ((cwgame=plugin_01x000_get_cw_game_here(user->room))!=NULL) {
        write_user(user, "POKER: This room is already in use.  Please choose another.\n");
        return;
  }
  */
  if (user->plugin_01x000_poker == NULL) {
        if ((game=plugin_01x000_create_po_game())==NULL) {
          write_syslog("ERROR: Memory allocation failure in plugin_01x000_start_po().\n",0,SYSLOG);
          write_user(user, "POKER:  An error occured while attempting to create a new game.\n");
          return;
        } else {
          if ((user->plugin_01x000_poker=plugin_01x000_create_po_player(game))==NULL) {
                write_syslog("ERROR: Memory allocation failure in plugin_01x000_start_po().\n",0,SYSLOG);
                write_user(user, "POKER:  An error occured while attempting to create a new game.\n");
                return;
          }
          user->plugin_01x000_poker->user = user;
          user->plugin_01x000_poker->hist = plugin_01x000_po_hist[hist_index];
          game->players = user->plugin_01x000_poker;
          game->curr_player = user->plugin_01x000_poker;
          game->dealer = user->plugin_01x000_poker;
          strcpy(game->name, word[2]);
          game->room = user->room;
          sprintf(text, "You have started a game of Poker called %s.\n", game->name);
          write_user(user,text);
          sprintf(text, "%s started a game of Poker called %s.\n",
                          user->name, game->name);
          write_room_except(user->room,text, user);
          sprintf(text,"%sIt's your turn to deal!\n",colors[CBOLD]);
          write_user(user,text);
        }
  } else {
        write_user(user, "POKER: You are already in a game.\n");
  }
}

/*** Join a PO Game ***/
plugin_01x000_join_po(user)
UR_OBJECT user;
{
  struct plugin_01x000_po_game *game;
  int hist_index;
  
  if ((hist_index = plugin_01x000_get_po_player_hist_index(user)) == -1) {
        write_user(user, "POKER: You need chips to bet with.\n");
        return;
  }
  /*
  if ((user->plugin_01x000_poker != NULL)&&(user->cwp != NULL)) {
        write_user(user, "POKER: You are already in a game.\n");
        return;
  }
  */
  if ((user->plugin_01x000_poker != NULL)&&(user->plugin_01x000_poker != NULL)) {
        write_user(user, "POKER: You are already in a game.\n");
        return;
  }

  if ((game=plugin_01x000_get_po_game_here(user->room))==NULL) {
        write_user(user, "POKER:  You must be in the same room as the game.\n");
        return;
  }

  if (game->num_players == 6) {
        write_user(user, "POKER: This game is full.\n");
        return;
  }

  if ((user->plugin_01x000_poker=plugin_01x000_create_po_player(game))==NULL) {
        write_syslog("ERROR: Memory allocation failure in plugin_01x000_join_po().\n",0,SYSLOG);
        write_user(user, "POKER:  Sorry, an error has occured.\n");
        return;
  } else {
        user->plugin_01x000_poker->user = user;
        user->plugin_01x000_poker->hist = plugin_01x000_po_hist[hist_index];
        sprintf(text, "POKER: You join the game called '%s'.\n", game->name);
        write_user(user, text);
        sprintf(text, "# %s joins the game %s.\n", user->name, game->name);
        write_room_except(user->room, text, user);
  }
}


/*** Leave a PO Game ***/
plugin_01x000_leave_po(user)
UR_OBJECT user;
{
  if (user->plugin_01x000_poker != NULL) {
        plugin_01x000_destruct_po_player(user->plugin_01x000_poker);
  } else {
        write_user(user, "POKER: You aren't in a game.\n");
  }
}  

/*** List PO Games ***/
plugin_01x000_list_po_games(user)
UR_OBJECT user;
{
  struct plugin_01x000_po_game *game;
  int count = 0;

      sprintf(text,"\n%s%s%s-=- Current poker games being played -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],colors[CBOLD]);
  write_user(user,text);
  write_user(user, "~FY~OLName            : Room                 : No. Players\n");
  for(game=plugin_01x000_po_game_first;game!=NULL;game=game->next) {
        sprintf(text, "%-15s : %-20s : %d players\n", game->name, game->room->name, game->num_players);
        write_user(user, text);
        count++;
  }
  
  sprintf(text,  "\nTotal of %d games.\n\n", count);
  write_user(user, text);
}

/*** add_po_player_hist ***/
int plugin_01x000_add_po_player_hist(u, total, given)
UR_OBJECT u;
int total, given;
{
  struct plugin_01x000_po_player_hist *player_hist;

  if ((player_hist=(struct plugin_01x000_po_player_hist *)malloc(sizeof(struct plugin_01x000_po_player_hist)))==NULL) {
        write_syslog("ERROR: Memory allocation failure in plugin_01x000_add_po_player_hist().\n",0,SYSLOG);
        return 0;
  }

  /* create a new po player history */
  strcpy(player_hist->name, u->name);
  player_hist->total = total;
  player_hist->given = given;

  /* add it to the list */
  plugin_01x000_po_hist[plugin_01x000_max_po_hist] = player_hist;
  plugin_01x000_max_po_hist++;
  
  plugin_01x000_sort_po_hist();
  
  return 1;
}

/*** get index for a player hist */
int plugin_01x000_get_po_player_hist_index(u)
UR_OBJECT u;
{
  int i;
  
  for (i = 0; i < plugin_01x000_max_po_hist; i++) {
        if (strcmp(plugin_01x000_po_hist[i]->name, u->name) == 0) {
          return i;
        }
  }
  
  return -1;
}

/*** po player hist compare ***/
int plugin_01x000_po_player_hist_cmp(a, b)
struct plugin_01x000_po_player_hist **a;
struct plugin_01x000_po_player_hist **b;
{
  int a_winnings;
  int b_winnings;

  a_winnings = (*a)->total;
  b_winnings = (*b)->total;

  if (a_winnings < b_winnings) {
        return 1;
  } else if (a_winnings > b_winnings) {
        return -1;
  } else {
        return 0;
  }
}                                                                        

/*** sort the poker history ***/
plugin_01x000_sort_po_hist()
{
  if (plugin_01x000_max_po_hist > 0) {
        qsort(plugin_01x000_po_hist, plugin_01x000_max_po_hist, sizeof(plugin_01x000_po_hist[0]),
                  (int (*) (const void *, const void *))plugin_01x000_po_player_hist_cmp);
  }
}

/*** show po player hist ***/
plugin_01x000_rank_po(user)
UR_OBJECT user;
{
  int i;
  int many = 0;

  if (word_count == 3) {
        many = atoi(word[2]);
  }

  if (many < 5) many = 5;
  if (many > plugin_01x000_max_po_hist) many = plugin_01x000_max_po_hist;

  write_user(user, "~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK- ~FG~OLPoker Ranking~RS~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=\n");
  write_user(user, "~FY~OLRank: Name         : Total             \n");
  write_user(user, "~FG---------------------------------------\n");
  
  for (i = 0; i < many; i++) {
        sprintf(text, "  %-2d: %-12s : $%-17d\n", i+1,
                        plugin_01x000_po_hist[i]->name, plugin_01x000_po_hist[i]->total, plugin_01x000_po_hist[i]->given);
        write_user(user, text);
  }

  for (i = 0; i < plugin_01x000_max_po_hist; i++) {
        if (!strcmp(user->name, plugin_01x000_po_hist[i]->name)) {
          if (i == many) {
                sprintf(text, "  %-2d: %-12s : $%-17d\n", i+1,plugin_01x000_po_hist[i]->name, plugin_01x000_po_hist[i]->total, plugin_01x000_po_hist[i]->given);
                write_user(user, text);
                break;
          } else if (i > many) {
                sprintf(text, "           :\n  %-2d: %-12s : $%-17d\n", i+1,plugin_01x000_po_hist[i]->name, plugin_01x000_po_hist[i]->total, plugin_01x000_po_hist[i]->given);
                write_user(user, text);
                break;
          } else {
                break;
          }
        }
  }

  write_user(user, "~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FG~OL Poker Ranking~RS~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=~FK-~FR=\n");
}
  
/*** Give chips to all the little people ***/
plugin_01x000_chips_po(user)
UR_OBJECT user;
{
  UR_OBJECT u;
  char text2[80];
  int ammount;
  int hist_index;
  
  if (word_count<3) {
        write_user(user,"~FRUsage: chips_po <user> <ammount>\n");  return;
  }
  
  ammount = atoi(word[3]);

  /* User currently logged in */
  if (u=get_user(word[2])) {
        /* add to player history */
        if ((hist_index = plugin_01x000_get_po_player_hist_index(u)) == -1) {
          /* not in po_hist */
          if (plugin_01x000_add_po_player_hist(u, ammount, ammount))
                hist_index = plugin_01x000_get_po_player_hist_index(u);
          else
                return;
        } else {
          /*
          sprintf(text, "hist_index = %d\n", hist_index);
          write_user(user, text);
          */
          /* already in hist */
          plugin_01x000_po_hist[hist_index]->total += ammount;
          plugin_01x000_po_hist[hist_index]->given += ammount;
          plugin_01x000_sort_po_hist();
        }
        sprintf(text,"~FR~OL%s has $%d poker chips.\n",
                        u->name, plugin_01x000_po_hist[hist_index]->total);
        write_user(user,text);
        sprintf(text,"~FT~OL%s has given you $%s worth of poker chips.\n",user->name,word[3]);
        write_user(u,text);
        sprintf(text,"%s gave %s $%s worth of poker chips.\n",user->name,u->name,word[3]);
        write_syslog(text,1,SYSLOG);
        sprintf(text,"%s[ %s gave %s $%s worth of poker chips. ]\n",colors[CSYSTEM],user->name,u->name,word[3]);
        write_duty(WIZ,text,NULL,user,0);
        return;
  }
  /* User not logged in */
  if ((u=create_user())==NULL) {
        sprintf(text,"~FR%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in chips_po().\n",0,SYSLOG);
        return;
  }
  strcpy(u->name,word[2]);
  if (!load_user_details(u)) {
        write_user(user,nosuchuser);
        destruct_user(u);
        destructed=0;
        return;
  }
  /* add to player hist */
  if ((hist_index = plugin_01x000_get_po_player_hist_index(u)) == -1) {
        /* not in po_hist */
        if (plugin_01x000_add_po_player_hist(u, ammount, ammount))
          hist_index = plugin_01x000_get_po_player_hist_index(u);
        else {
          destruct_user(u);
          destructed=0;
          return;
        } 
  } else {
        /* already in hist */
        plugin_01x000_po_hist[hist_index]->total += ammount;
        plugin_01x000_po_hist[hist_index]->given += ammount;
        plugin_01x000_sort_po_hist();
  }
  sprintf(text,"~FT~OL%s has $%d poker chips.\n",word[2],plugin_01x000_po_hist[hist_index]->total);
  write_user(user,text);
  sprintf(text,"%s gave %s $%s poker chips.\n",user->name,u->name,word[3]);
  write_syslog(text,1,SYSLOG);
  sprintf(text,"%s[ %s gave %s $%s poker chips. ]\n",colors[CSYSTEM],user->name,u->name,word[3]);
  write_duty(WIZ,text,NULL,user,0);
  u->socket=-2;
  strcpy(u->site,u->last_site);
  save_user_details(u,0);
  destruct_user(u);
  destructed=0;
  sprintf(text2,"~FT~OLYou have been given $%s worth of poker chips.~RS\n",word[3]);
  send_mail(NULL,word[2],text2);
}

/*** save poker history to disk ***/
int plugin_01x000_save_po_hist()
{
  FILE *fp;
  char filename[80];
  int i;
  
  sprintf(filename,"%s/%s",PLUGINFILES,plugin_01x000_POKERFILE);
  if (!(fp=fopen(filename,"w"))) {
        sprintf(text,"SAVE_PO_HIST: Failed to save poker history.\n");
        write_syslog(text,1,SYSLOG);
        return 0;
  }
  fprintf(fp,"%d\n",plugin_01x000_max_po_hist);
  for (i = 0; i < plugin_01x000_max_po_hist; i++) {
        fprintf(fp, "%s\n", plugin_01x000_po_hist[i]->name);
        fprintf(fp, "%d %d\n", plugin_01x000_po_hist[i]->total, plugin_01x000_po_hist[i]->given);
  }
  fclose(fp);
  return 1;
}

/*** load poker history from disk ***/
int plugin_01x000_load_po_hist()
{
  FILE *fp;
  char filename[80];
  int i;
  struct plugin_01x000_po_player_hist *player_hist;
  
  sprintf(filename,"%s/%s",PLUGINFILES,plugin_01x000_POKERFILE);
  if (!(fp=fopen(filename,"r"))) {
        printf("LOAD_PO_HIST: Failed to load poker history.\n");
        return 0;
  }
  fscanf(fp,"%d",&plugin_01x000_max_po_hist); 
  printf("max_po_hist = %d\n", plugin_01x000_max_po_hist);
  for (i = 0; i < plugin_01x000_max_po_hist; i++) {
        if ((player_hist=(struct plugin_01x000_po_player_hist *)malloc(sizeof(struct plugin_01x000_po_player_hist)))==NULL) {
          printf("ERROR: Memory allocation failure in load_po_hist().\n");
          return 0;
        }

        fscanf(fp, "%s", player_hist->name);
        fscanf(fp,"%d %d", &(player_hist->total), &(player_hist->given));
        printf("name = %s, total = %d\n", player_hist->name, player_hist->total);

        /* add it to the list */
        plugin_01x000_po_hist[i] = player_hist;
  }
  
  fclose(fp);

  plugin_01x000_sort_po_hist();
  
  printf("Loaded poker history...\n");
  return 1;
}

/*** Deal cards to all players ***/
plugin_01x000_deal_po(user)
UR_OBJECT user;
{
  int i;
  struct plugin_01x000_po_game *game;
  struct plugin_01x000_po_player *tmp_player;

  if (user->plugin_01x000_poker == NULL) {
        write_user(user, "You have to be in a Poker game to use this command.\n");
        return;
  }

  game = user->plugin_01x000_poker->game;

  if (user->plugin_01x000_poker != game->dealer) {
        sprintf(text, "%s is the dealer.\n", game->dealer->user->name);
        write_user(user, text);
        return;
  }

  if (game->state != 0) {
        write_user(user, "You can't deal now.\n");
        return;
  }

  if (game->num_players < 2) {
        write_user(user, "You need two people in to deal poker.\n");
        return;
  }

  /* Reset game state */
  game->top_card = 0;
  game->bet = 0;
  game->pot = 0;
  game->num_raises = 0;
  game->in_players = 0;
  game->opened = NULL;
  game->newdealer = 0;
  
  plugin_01x000_shuffle_cards(game->deck);

  /* Reset players */
  for (tmp_player = game->first_player; tmp_player != NULL;
           tmp_player = tmp_player->next) {
        if (tmp_player->hist->total >= 5) {
          tmp_player->hand[0] = -1;
          tmp_player->touched = 0;
          tmp_player->putin = 5;
          tmp_player->rank = 0;
          
          game->in_players++;
          
          write_user(tmp_player->user, "# You ante $5.\n");
        } else {
          write_user(tmp_player->user, "~FRYou need more poker chips!\n");
        }
  }
  
  plugin_01x000_sort_po_hist();

  if (game->in_players < 2) {
        write_room(user->room, "Not enough people have chips to ante.  You need at least two people.\n");
        return;
  }

  write_room(user->room, "# Everyone has anted $5.\n");
  /* record(user->room, "# Everyone has anted $5.\n"); */
  write_user(user, "# You shuffle and deal the cards.\n");
  sprintf(text, "# %s shuffles and deals the cards.\n", user->name);
  write_room_except(user->room, text, user);
  /* record(user->room,text); */

  /* Start with the player to the left of the dealer */
  game->curr_player = game->dealer;
  plugin_01x000_next_po_player(game);
  
  tmp_player = game->curr_player;

  /* deal five to each player */

  for (i = 0; i < 5; i++) {
        do {
          if (game->curr_player->putin == 5) {
                game->curr_player->hand[i] = game->deck[game->top_card];
                game->top_card++;

                if (i == 4) {
                  game->curr_player->hist->total -= 5;
                  game->pot += 5;
                  game->curr_player->putin = 0;
                  plugin_01x000_print_hand(game->curr_player->user, game->curr_player->hand);
                }
          }
          /* next player */
          plugin_01x000_next_po_player(game);
        } while (game->curr_player != tmp_player);
  }

  /* make this guy the default opened guy */
  game->opened = game->curr_player;

  write_user(game->curr_player->user,"# The first round of betting starts with you.\n");
  sprintf(text, "# The first round of betting starts with %s.\n",game->curr_player->user->name);
  write_room_except(user->room, text, game->curr_player->user);
  /* record(user->room, text); */
  
  game->state = 1;
}

/*** Next po player ***/
plugin_01x000_next_po_player(game)
struct plugin_01x000_po_game *game;
{
  if (game->curr_player->next == NULL) {
        game->curr_player = game->first_player;
  } else {
        game->curr_player = game->curr_player->next;
  }  
}

/*** Next in player ***/
plugin_01x000_next_in_player(game)
struct plugin_01x000_po_game *game;
{
  do {
        if (game->curr_player->next == NULL) {
          game->curr_player = game->first_player;
        } else {
          game->curr_player = game->curr_player->next;
        }  
  } while (game->curr_player->hand[0] == -1);
}

/*** Bet PO ***/
plugin_01x000_bet_po(user)
UR_OBJECT user;
{
  struct plugin_01x000_po_game *game;
  int player_bet;

  if (user->plugin_01x000_poker == NULL) {
        write_user(user, "You have to be in a Poker game to use this command.\n");
        return;
  }

  game = user->plugin_01x000_poker->game;

  /* Check if it's possible to bet */
  if ((game->state != 1)&&(game->state != 3)) {
        write_user(user, "You can't bet now.\n");
        return;
  }

  if (game->curr_player != user->plugin_01x000_poker) {
        write_user(user, "You can't bet unless it's your turn.\n");
        return;
  }

  if (word_count < 3) {
        write_user(user, "How much do you want to bet?\n");
        return;
  }
  
  player_bet = atoi(word[2]);
  if ((player_bet == 0) && (word[2][0] != '0')) {
        write_user(user, "Only numbers are allowed in a bet.\n");
        return;
  }

  if (player_bet < 0) {
        write_user(user, "Positive bets only, please.\n");
        return;
  }
  
  if (player_bet%5 != 0) {
        write_user(user, "Make your bet a multiple of $5 please.\n");
        return;
  }

  if (player_bet == 0) {
        plugin_01x000_check_po(user);
        return;
  }

  if (player_bet < game->bet) {
        sprintf(text, "You must bet at least $%d or fold.\n", game->bet);
        write_user(user, text);
        return;
  }

  if (player_bet - game->bet > 100) {
        if (game->bet == 0) {
          write_user(user, "The largest opening bet is $100.\n");
          return;
        } else {
          write_user(user, "The largest raise is $100.\n");
          return;
        }
  }

  if ((player_bet > game->bet) && (game->bet != 0)) {
        if (game->num_raises > 2) {
          write_user(user, "There is a limit of three raises.  Please see the bet, or fold.\n");
          return;
        }
        game->num_raises++;
  }
  
  if (user->plugin_01x000_poker->hist->total < player_bet) {
        sprintf(text, "You don't have enough poker chips to make that bet.\n");
        write_user(user, text);
        return;
  }

  plugin_01x000_bet_po_aux(user, player_bet);
}

/*** Aux bet function ***/
plugin_01x000_bet_po_aux(user, player_bet)
UR_OBJECT user;
int player_bet;
{
  struct plugin_01x000_po_game *game;

  game = user->plugin_01x000_poker->game;
  
  user->plugin_01x000_poker->touched = 1;

  if (player_bet == game->bet) {
        sprintf(text, "# You see the bet of $%d.\n", game->bet);
        write_user(user, text);
        sprintf(text, "# %s sees the bet of $%d.\n", user->name, game->bet);
        write_room_except(user->room, text, user);
        /* record(game->room,text); */
  } else if (game->bet == 0) {
        game->opened = user->plugin_01x000_poker;
        sprintf(text, "# You open the betting with $%d.\n", player_bet);
        write_user(user, text);
        sprintf(text, "# %s opens the betting with $%d.\n", user->name, player_bet);
        write_room_except(user->room, text, user);
        /* record(game->room,text); */
  } else {
        sprintf(text, "# You raise the bet to $%d.\n", player_bet);
        write_user(user, text);
        sprintf(text, "# %s raises the bet to $%d.\n", user->name, player_bet);
        write_room_except(user->room, text, user);
        /* record(game->room,text); */
  }
  
  game->bet = player_bet;
  game->pot += (player_bet - user->plugin_01x000_poker->putin);
  user->plugin_01x000_poker->hist->total -= (player_bet - user->plugin_01x000_poker->putin);
  user->plugin_01x000_poker->putin = player_bet;

  plugin_01x000_sort_po_hist();

  sprintf(text, "# The pot is now $%d.\n", game->pot);
  write_user(user, text);

  /* Go to next elegible player */
  plugin_01x000_next_in_player(game);

  /* Check if all players have called */
  plugin_01x000_all_called_check(game);

  plugin_01x000_bet_message(game);
}

/*** Raise PO ***/
plugin_01x000_raise_po(user, inpstr)
UR_OBJECT user;
char *inpstr;
{
  struct plugin_01x000_po_game *game;
  int player_raise;

  if (user->plugin_01x000_poker == NULL) {
        write_user(user, "You have to be in a Poker game to use this command.\n");
        return;
  }

  game = user->plugin_01x000_poker->game;

  /* Check if it's possible to raise */
  if ((game->state != 1)&&(game->state != 3)) {
        write_user(user, "You can't raise now.\n");
        return;
  }

  if (game->curr_player != user->plugin_01x000_poker) {
        write_user(user, "You can't raise unless it's your turn.\n");
        return;
  }

  if (word_count < 3) {
        write_user(user, "By how much do you want to raise?\n");
        return;
  }
  
  player_raise = atoi(word[2]);
  if ((player_raise == 0) && (word[2][0] != '0')) {
        write_user(user, "Please use numbers.\n");
        return;
  }

  if (player_raise < 0) {
        write_user(user, "Positive raises please.\n");
        return;
  }
  
  if (player_raise%5 != 0) {
        write_user(user, "Make your raise a multiple of $5 please.\n");
        return;
  }

  if (player_raise < 5) {
        write_user(user, "The smallest raise is $5.\n");
        return;
  }

  if (player_raise > 100) {
        write_user(user, "The largest raise is $100.\n");
        return;
  }

  if (player_raise == 0) {
        write_user(user, "The smallest raise is $5.\n");
        return;
  }

  if (game->num_raises > 2) {
        write_user(user, "There is a limit of three raises.  Please see the bet or fold.\n");
        return;
  }

  player_raise += game->bet;
  game->num_raises++;

  if (user->plugin_01x000_poker->hist->total < player_raise) {
        write_user(user, "You don't have enough poker chips to make that raise.\n");
        return;
  }

  plugin_01x000_bet_po_aux(user, player_raise);
}

/*** see a bet ***/
plugin_01x000_see_po(user)
UR_OBJECT user;
{
  struct plugin_01x000_po_game *game;
  int player_bet;

  if (user->plugin_01x000_poker == NULL) {
        write_user(user, "You have to be in a Poker game to use this command.\n");
        return;
  }

  game = user->plugin_01x000_poker->game;

  /* Check if it's possible to see a bet */
  if ((game->state != 1)&&(game->state != 3)) {
        write_user(user, "You can't see a bet now.\n");
        return;
  }

  if (game->curr_player != user->plugin_01x000_poker) {
        write_user(user, "You can't see a bet unless it's your turn.\n");
        return;
  }

  player_bet = game->bet;
  
  if (user->plugin_01x000_poker->hist->total < player_bet) {
        write_user(user, "You don't have enough poker chips to see that bet.\n");
        return;
  }

  if (player_bet == 0)
        plugin_01x000_check_po(user);
  else
        plugin_01x000_bet_po_aux(user, player_bet);
}

/*** All called check ***/
plugin_01x000_all_called_check(game)
struct plugin_01x000_po_game *game;
{
  struct plugin_01x000_po_player *tmp_player;

  if (game->curr_player->touched &&
          game->curr_player->putin == game->bet) {
        /* Everyone has called */

        /* reset the touched flags and putin ammts */
        for (tmp_player = game->first_player; tmp_player != NULL;
                 tmp_player = tmp_player->next) {
          tmp_player->touched = 0;
          tmp_player->putin = 0;
        }

        game->state++; /* next state */

        switch (game->state) {
        case 2:
          /* Start with the player to the left of the dealer */
          game->curr_player = game->dealer;
          plugin_01x000_next_in_player(game);
          
          plugin_01x000_hand_po(game->curr_player->user);
          sprintf(text, "# It's your turn to discard.\n");
          write_user(game->curr_player->user, text);
          sprintf(text, "# It's %s's turn to discard.\n", 
                          game->curr_player->user->name);
          write_room_except(game->room, text, game->curr_player->user);
          /* record(game->room, text); */
          break;
        case 4:
          write_room(game->room, "# Showdown...\n");
          plugin_01x000_showdown_po(game);
          break;
        }
  }
}


/*** pass the deal to the next player ***/
plugin_01x000_pass_the_deal(game)
struct plugin_01x000_po_game *game;
{
  if (game->dealer->next == NULL) {
        game->dealer = game->first_player;
  } else {
        game->dealer = game->dealer->next;
  }

  if (game->state != 0) {
        game->newdealer = 1;  /* mark that we have a new dealer */
  }
}


/*** Fold and pass turn to next player ***/
plugin_01x000_fold_po(user, inpstr)
UR_OBJECT user;
char *inpstr;
{
  struct plugin_01x000_po_game *game;
  struct plugin_01x000_po_player *tmp_player;

  if (user->plugin_01x000_poker == NULL) {
        write_user(user, "You have to be in a Poker game to use this command.\n");
        return;
  }

  if (user->plugin_01x000_poker->hand[0] == -1) {
        write_user(user, "You don't have any cards.\n");
        return;
  }
  game = user->plugin_01x000_poker->game;

  /* Mark folding by making the first card in hand -1 */
  user->plugin_01x000_poker->hand[0] = -1;
  game->in_players--;

  if (word_count < 3) {
        sprintf(text, "# You fold.\n");
        write_user(user, text);
        sprintf(text, "# %s folds.\n", user->name);
  } else {
        sprintf(text, "# You say, \"%s\" and fold.\n", inpstr);
        write_user(user, text);
        sprintf(text,"# %s says, \"%s\" and folds.\n", user->name, inpstr);
  }
  write_room_except(user->room, text, user);
  /* record(user->room,text); */

  /* Check if there is 1 player in */
  if (game->in_players == 1) {
        /* The in person wins */
        plugin_01x000_next_in_player(game);

        /* add to players total_bux */
        game->curr_player->hist->total += game->pot;

        plugin_01x000_sort_po_hist();

        sprintf(text, "# You win $%d!!!\n", game->pot);
        write_user(game->curr_player->user, text);
        sprintf(text, "# %s wins $%d!!!\n", 
                        game->curr_player->user->name, game->pot);
        write_room_except(game->room, text, game->curr_player->user);
        /* record(game->room, text); */

        game->pot = 0;
        game->bet = 0;
        game->state = 0;  /* reset and deal cards */

        for (tmp_player = game->first_player; tmp_player != NULL;
                 tmp_player = tmp_player->next) {
          /* clear players' hands */
          tmp_player->hand[0] = -1;
        }

        /* pass the deal if it hasn't already */
        if (!game->newdealer)
          plugin_01x000_pass_the_deal(game);

        game->curr_player = game->dealer;
        sprintf(text, "~OL# It's your turn to deal.\n");
        write_user(game->dealer->user, text);
        sprintf(text, "# It's %s's turn to deal.\n", 
                        game->dealer->user->name);
        write_room_except(game->room, text, game->dealer->user);
        /* record(game->room, text); */

  } else {
        /* check if it's my turn */
        if (game->curr_player == user->plugin_01x000_poker) {
          /* Go to next elegible player */
          plugin_01x000_next_in_player(game);

          /* Check what state we're in */
          plugin_01x000_all_called_check(game);
          
          plugin_01x000_bet_message(game);
        }
  }
}

/*** Check PO ***/
plugin_01x000_check_po(user)
UR_OBJECT user;
{
  struct plugin_01x000_po_game *game;
  int player_bet;
  
  if (user->plugin_01x000_poker == NULL) {
        write_user(user, "You have to be in a Poker game to use this command.\n");
        return;
  }

  game = user->plugin_01x000_poker->game;

  /* Check if it's possible to check */
  if ((game->state != 1)&&(game->state != 3)) {
        write_user(user, "You can't check now.\n");
        return;
  }

  if (game->curr_player != user->plugin_01x000_poker) {
        write_user(user, "You can't check unless it's your turn.\n");
        return;
  }

  if (game->bet > 0) {
        sprintf(text, "You must bet at least $%d or fold.\n", game->bet);
        write_user(user, text);
        return;
  }

  /* We've checked! */
  user->plugin_01x000_poker->touched = 1;

  sprintf(text, "# You check.\n");
  write_user(user, text);
  sprintf(text, "# %s checks.\n", user->name);
  write_room_except(user->room, text, user);
  /* record(game->room,text); */

  /* Go to next elegible player */
  plugin_01x000_next_in_player(game);

  /* Check what state we're in */
  plugin_01x000_all_called_check(game);

  plugin_01x000_bet_message(game);
}

/*** Bet message ***/
plugin_01x000_bet_message(game)
struct plugin_01x000_po_game *game;
{
  if (game->state == 1 || game->state == 3) {
        sprintf(text, "# You've put in $%d this round.  The bet is $%d to you.\n",game->curr_player->putin, game->bet);
        write_user(game->curr_player->user, text);
        sprintf(text, "# It's %s's turn to bet.\n", game->curr_player->user->name);
        write_room_except(game->room, text, game->curr_player->user);
        /* record(game->room,text); */
  }
}

/*** Disc PO ***/
plugin_01x000_disc_po(user)
UR_OBJECT user;
{
  struct plugin_01x000_po_game *game;
  struct plugin_01x000_po_player *tmp_player;
  int i;
  int have_ace;
  int disc_these[5];
  int choice;

  if (user->plugin_01x000_poker == NULL) {
        write_user(user, "You have to be in a Poker game to use this command.\n");
        return;
  }

  game = user->plugin_01x000_poker->game;

  /* Check if it's possible to discard */
  if (game->state != 2) {
        write_user(user, "You can't discard now.\n");
        return;
  }

  if (game->curr_player != user->plugin_01x000_poker) {
        write_user(user, "You can't discard unless it's your turn.\n");
        return;
  }
  
  if (word_count > 6) {
        write_user(user, "You can't discard five cards.\n");
        return;
  }

  if (word_count > 5) {
        /* Look for an ace */
        have_ace = 0;
        i = 0;
        do {
          if (user->plugin_01x000_poker->hand[i]%13 == 12)
                have_ace = 1;
          i++;
        } while ((i < 5) && (!have_ace));

        if (!have_ace) {
          write_user(user, "You can't discard four cards unless you have an ace.\n");
          return;
        }
  }
 
  user->plugin_01x000_poker->touched = 1;

  if ((word_count < 3) || (word[2][0] == '0')) {
        /* No discards */
        sprintf(text, "# You stand pat.\n");
        write_user(user, text);
        sprintf(text, "~OL# %s stands pat.~RS\n", user->name);
        write_room_except(user->room, text, user);
        /* record(game->room,text); */
  } else {
        /* discards */
        /** Init the array **/
        for (i = 0; i < 5; i++)
          disc_these[i] = 0;

        /** Get which cards to discard **/
        for (i = 2; i < word_count; i++) {
          choice = atoi(word[i]);
          if ((choice <= 0) || (choice > 5)) {
                sprintf(text, "Choose a number 1 through 5 please.\n");
                write_user(user, text);
                return;
          } else {
                disc_these[choice - 1] = 1; /* We're not keeping this one */
          }
        }

        /** draw cards **/
        for (i = 0; i < 5; i++) {
          if (disc_these[i]) {
                user->plugin_01x000_poker->hand[i] = game->deck[game->top_card];
                game->top_card++;
          }
        }

        sprintf(text, "# You have discarded %d card(s).\n", word_count-2);
        write_user(user, text);
        sprintf(text, "~OL# %s has discarded %d card(s)~RS.\n", user->name, word_count-2);
        write_room_except(user->room, text, user);
        /* record(game->room,text); */

        plugin_01x000_print_hand(user, user->plugin_01x000_poker->hand);
  }

  /* Go to next elegible player */
  plugin_01x000_next_in_player(game);
  
  if (game->curr_player->touched) {
        /* We've dealt cards to everyone */

        /* reset the touched flags */
        for (tmp_player = game->first_player; tmp_player != NULL;
                 tmp_player = tmp_player->next) {
          tmp_player->touched = 0;
        }

        /* reset the bet */
        game->bet = 0;
        game->num_raises = 0;

        /* Start with the player who opened if still in */
        game->curr_player = game->opened;
        if (game->curr_player->hand[0] == -1) {
          /* That person folded */
          plugin_01x000_next_in_player(game);
        }

        sprintf(text, "# The current pot is $%d.\n", game->pot);
        write_room(user->room, text);

        sprintf(text, "# The second round of betting starts with you.\n");
        write_user(game->curr_player->user, text);
        sprintf(text, "# The second round of betting starts with %s.\n", 
                        game->curr_player->user->name);
        write_room_except(user->room, text, game->curr_player->user);
        /* record(game->room,text); */

        game->state = 3;
  } else {
        plugin_01x000_hand_po(game->curr_player->user);
        sprintf(text, "# It's your turn to discard.\n");
        write_user(game->curr_player->user, text);
        sprintf(text, "# It's %s's turn to discard.\n", 
                        game->curr_player->user->name);
        write_room_except(user->room, text, game->curr_player->user);
        /* record(game->room,text); */
  }
}

/*** Showdown ***/
plugin_01x000_showdown_po(game)
struct plugin_01x000_po_game *game;
{
  struct plugin_01x000_po_player *tmp_player;
  struct plugin_01x000_po_player *winners[4];
  int num_winners;
  int i, j, temp;
  int loot;
  char rtext[20];
  
  winners[0] = game->first_player;
  num_winners = 1;

  /* assign ranks to all players hands */
  for (tmp_player = game->first_player; tmp_player != NULL;
           tmp_player = tmp_player->next) {

        /* If the player is not folded */
        if (tmp_player->hand[0] != -1) {

          /* sort cards */
          for(i=0;i<5;i++) {
                for(j=0;j<4;j++)
                  {
                        if(tmp_player->hand[j]%13 < tmp_player->hand[j+1]%13)
                          {
                                temp=tmp_player->hand[j];
                                tmp_player->hand[j]=tmp_player->hand[j+1];
                                tmp_player->hand[j+1]=temp;
                          }
                  }
          }
          /*      
          for (i = 0; i < 5; i++) {
                sprintf(text, "%s's hand[%d]mod 13 = %d\n",
                                tmp_player->user->name, i, tmp_player->hand[i]%13);
                write_room(game->room, text);
          }
          */
          /* check for straight or straight flush */
          /* Ace low 5432A */
          if ((tmp_player->hand[0]%13 == (tmp_player->hand[1]%13) + 1) &&
                  (tmp_player->hand[1]%13 == (tmp_player->hand[2]%13) + 1) &&
                  (tmp_player->hand[2]%13 == (tmp_player->hand[3]%13) + 1) &&
                  (tmp_player->hand[4]%13 == 12) && (tmp_player->hand[0]%13 == 0)) {
                tmp_player->rank = 5;
                /* arrange cards */
                plugin_01x000_swap_cards(tmp_player->hand, 0, 1);
                plugin_01x000_swap_cards(tmp_player->hand, 1, 2);
                plugin_01x000_swap_cards(tmp_player->hand, 2, 3);
                plugin_01x000_swap_cards(tmp_player->hand, 3, 4);
          } else { /* Ace high AKQJ10 or other straight */
                if ((tmp_player->hand[0]%13 == (tmp_player->hand[1]%13) + 1) &&
		    (tmp_player->hand[1]%13 == (tmp_player->hand[2]%13) + 1) &&
		    (tmp_player->hand[2]%13 == (tmp_player->hand[3]%13) + 1) &&
		    (tmp_player->hand[3]%13 == (tmp_player->hand[4]%13) + 1)) {
                  tmp_player->rank = 5;
                }        
          }

          /* check for flush */
          if (((tmp_player->hand[0] < 13) &&
                   (tmp_player->hand[1] < 13) &&
                   (tmp_player->hand[2] < 13) &&
                   (tmp_player->hand[3] < 13) &&
                   (tmp_player->hand[4] < 13)) ||
                  (((tmp_player->hand[0] < 26) && (tmp_player->hand[0] > 12)) &&
                   ((tmp_player->hand[1] < 26) && (tmp_player->hand[1] > 12)) &&
                   ((tmp_player->hand[2] < 26) && (tmp_player->hand[2] > 12)) &&
                   ((tmp_player->hand[3] < 26) && (tmp_player->hand[3] > 12)) &&
                   ((tmp_player->hand[4] < 26) && (tmp_player->hand[4] > 12))) ||
                  (((tmp_player->hand[0] < 39) && (tmp_player->hand[0] > 25)) &&
                   ((tmp_player->hand[1] < 39) && (tmp_player->hand[1] > 25)) &&
                   ((tmp_player->hand[2] < 39) && (tmp_player->hand[2] > 25)) &&
                   ((tmp_player->hand[3] < 39) && (tmp_player->hand[3] > 25)) &&
                   ((tmp_player->hand[4] < 39) && (tmp_player->hand[4] > 25))) ||
                  (((tmp_player->hand[0] < 52) && (tmp_player->hand[0] > 38)) &&
                   ((tmp_player->hand[1] < 52) && (tmp_player->hand[1] > 38)) &&
                   ((tmp_player->hand[2] < 52) && (tmp_player->hand[2] > 38)) &&
                   ((tmp_player->hand[3] < 52) && (tmp_player->hand[3] > 38)) &&
                   ((tmp_player->hand[4] < 52) && (tmp_player->hand[4] > 38)))) {
                /* We have a flush at least */
                if (tmp_player->rank > 0) { 
                  /* We have a straight flush */
                  tmp_player->rank = 9;
                } else {
                  /* We have a flush */
                  tmp_player->rank = 6;
                }
          }

          /* check for four of a kind */
          if ((tmp_player->rank == 0) && 
                  ((tmp_player->hand[0]%13 == tmp_player->hand[1]%13) &&
                   (tmp_player->hand[1]%13 == tmp_player->hand[2]%13) &&
                   (tmp_player->hand[2]%13 == tmp_player->hand[3]%13))) {
                tmp_player->rank = 8;
          }
          if ((tmp_player->rank == 0) &&
                   ((tmp_player->hand[1]%13 == tmp_player->hand[2]%13) &&
                        (tmp_player->hand[2]%13 == tmp_player->hand[3]%13) &&
                        (tmp_player->hand[3]%13 == tmp_player->hand[4]%13))) {
                /* we have four of a kind */
                tmp_player->rank = 8;
                /* arrange cards */
                plugin_01x000_swap_cards(tmp_player->hand, 0, 4);
          }
                           
          /* check for three of a kind or full house */
          if ((tmp_player->rank == 0) && 
                  ((tmp_player->hand[1]%13 == tmp_player->hand[2]%13) &&
                   (tmp_player->hand[2]%13 == tmp_player->hand[3]%13))) {
                /* we have three of a kind */
                tmp_player->rank = 4;
                /* arrange cards */
                plugin_01x000_swap_cards(tmp_player->hand, 0, 3);
          }
          if ((tmp_player->rank == 0) && 
                  ((tmp_player->hand[0]%13 == tmp_player->hand[1]%13) &&
                   (tmp_player->hand[1]%13 == tmp_player->hand[2]%13))) {
                if (tmp_player->hand[3]%13 == tmp_player->hand[4]%13) {
                  /* we have a full house */
                  tmp_player->rank = 7;
                } else {
                  /* we have three of a kind */
                  tmp_player->rank = 4;
                }
          }
          if ((tmp_player->rank == 0) && 
                  ((tmp_player->hand[2]%13 == tmp_player->hand[3]%13) &&
                   (tmp_player->hand[3]%13 == tmp_player->hand[4]%13))) {
                if (tmp_player->hand[0]%13 == tmp_player->hand[1]%13) {
                  /* we have a full house */
                  tmp_player->rank = 7;
                  /* arrange cards */
                  plugin_01x000_swap_cards(tmp_player->hand, 0, 3);
                  plugin_01x000_swap_cards(tmp_player->hand, 1, 4);
                } else {
                  /* we have three of a kind */
                  tmp_player->rank = 4;
                  /* arrange cards */
                  plugin_01x000_swap_cards(tmp_player->hand, 0, 3);
                  plugin_01x000_swap_cards(tmp_player->hand, 1, 4);
                }
          }

          /* check for two pair */
          if ((tmp_player->rank == 0) && 
                  ((tmp_player->hand[0]%13 == tmp_player->hand[1]%13) &&
                   (tmp_player->hand[2]%13 == tmp_player->hand[3]%13))) {
                /* we have two pair */
                tmp_player->rank = 3;
          }

          /* check for two pair */
          if ((tmp_player->rank == 0) && 
                  ((tmp_player->hand[0]%13 == tmp_player->hand[1]%13) &&
                   (tmp_player->hand[3]%13 == tmp_player->hand[4]%13))) {
                /* we have two pair */
                tmp_player->rank = 3;
                /* arrange cards */
                plugin_01x000_swap_cards(tmp_player->hand, 2, 4);
          }

          /* check for two pair */
          if ((tmp_player->rank == 0) && 
                  ((tmp_player->hand[1]%13 == tmp_player->hand[2]%13) &&
                   (tmp_player->hand[3]%13 == tmp_player->hand[4]%13))) {
                /* We have two pair */
                tmp_player->rank = 3;
                /* arrange cards */
                plugin_01x000_swap_cards(tmp_player->hand, 0, 2);
                plugin_01x000_swap_cards(tmp_player->hand, 2, 4);
          }

          /* check for one pair */
          if ((tmp_player->rank == 0) && 
                  (tmp_player->hand[0]%13 == tmp_player->hand[1]%13)) {
                /* we have a pair */
                tmp_player->rank = 2;
          }

          /* check for one pair */
          if ((tmp_player->rank == 0) && 
                  (tmp_player->hand[1]%13 == tmp_player->hand[2]%13)) {
                /* we have a pair */
                tmp_player->rank = 2;
                /* arrange cards */
                plugin_01x000_swap_cards(tmp_player->hand, 0, 2);
          }

          /* check for one pair */
          if ((tmp_player->rank == 0) && 
                  (tmp_player->hand[2]%13 == tmp_player->hand[3]%13)) {
                /* we have a pair */
                tmp_player->rank = 2;
                /* arrange cards */
                plugin_01x000_swap_cards(tmp_player->hand, 0, 2);
                plugin_01x000_swap_cards(tmp_player->hand, 1, 3);
          }

          /* check for one pair */
          if ((tmp_player->rank == 0) && 
                  (tmp_player->hand[3]%13 == tmp_player->hand[4]%13)) {
                /* We have a pair */
                tmp_player->rank = 2;
                /* arrange cards */
                plugin_01x000_swap_cards(tmp_player->hand, 0, 2);
                plugin_01x000_swap_cards(tmp_player->hand, 1, 3);
                plugin_01x000_swap_cards(tmp_player->hand, 0, 4);
          }
          
          if (tmp_player->rank == 0) {
                /* We must have a high card */
                tmp_player->rank = 1;
          }
        }
  }

  /* show all hands that are in */
  for (tmp_player = game->first_player; tmp_player != NULL;
           tmp_player = tmp_player->next) {
        if (tmp_player->rank > 0) {
          /* get max rank */
          if (tmp_player->rank > winners[0]->rank) { 
                winners[0] = tmp_player;
                num_winners = 1;
          } 
          if ((tmp_player->rank == winners[0]->rank) &&
                  (tmp_player != winners[0])){
                for (i = 0; i < 5; i++) {
                  if (tmp_player->hand[i]%13 > winners[0]->hand[i]%13) {
                        winners[0] = tmp_player;
                        num_winners = 1;
                        break;
                  } else if (tmp_player->hand[i]%13 < winners[0]->hand[i]%13) {
                        break;
                  } else if (i == 4) {
                        /* we have a tie */
                        winners[num_winners] = tmp_player;
                        num_winners++;
                  }
                }
          }               
        plugin_01x000_room_print_hand(tmp_player->user, tmp_player->hand);
        switch (tmp_player->rank) {
        case 1: sprintf(rtext, "a high card."); break;
        case 2: sprintf(rtext, "a pair."); break;
        case 3: sprintf(rtext, "two pair."); break;
        case 4: sprintf(rtext, "three of a kind."); break;
        case 5: sprintf(rtext, "a straight."); break;
        case 6: sprintf(rtext, "a flush."); break;
        case 7: sprintf(rtext, "a full house."); break;
        case 8: sprintf(rtext, "four of a kind."); break;
        case 9: sprintf(rtext, "a straight flush."); break;
        }
        sprintf(text, "# %s has %s\n", tmp_player->user->name, rtext);
        write_room(game->room, text);
        /* record(game->room,text); */
        }
  }
  /*
  sprintf(text, "num_winners = %d\n", num_winners);
  write_room(game->room, text);
  */
  /* divide the loot */
  /* curr_player == the player who was called */
  for (i = 0; i < num_winners; i++) {
        temp = (game->pot / (num_winners * 5));
        loot = 5 * temp;
        /*
        sprintf(text, "temp = %d\n", temp);
        write_room(game->room, text);
        */
        /* player called gets remainder */
        if (winners[i] == game->curr_player) {
          loot += game->pot%(num_winners*5);
        }
        winners[i]->hist->total += loot;
        sprintf(text, "~OL# You win $%d!!!\n", loot);
        write_user(winners[i]->user, text);
        sprintf(text, "# %s wins $%d!!!\n", 
                        winners[i]->user->name, loot);
        write_room_except(game->room, text, winners[i]->user);
        /* record(game->room, text); */
  }
  
  plugin_01x000_sort_po_hist();

  game->pot = 0;
  game->bet = 0;
  game->state = 0;  /* reset and deal cards */

  for (tmp_player = game->first_player; tmp_player != NULL;
           tmp_player = tmp_player->next) {
        /* clear players' hands */
        tmp_player->hand[0] = -1;
  }

  /* pass the deal if we haven't already */
  if (!game->newdealer)
        plugin_01x000_pass_the_deal(game);

  game->curr_player = game->dealer;
  sprintf(text, "~OL# It's your turn to deal.\n");
  write_user(game->dealer->user, text);
  sprintf(text, "# It's %s's turn to deal.\n", 
                  game->dealer->user->name);
  write_room_except(game->room, text, game->dealer->user);
  /* record(game->room, text); */
}
  

/*** swap_cards ***/
plugin_01x000_swap_cards(hand, c1, c2)
int hand[];
int c1;
int c2;
{
  int tmp;
  
  tmp = hand[c1];
  hand[c1] = hand[c2];
  hand[c2] = tmp;
}

plugin_01x000_magic_po(user)
UR_OBJECT user;
{
  user->plugin_01x000_poker->hand[0] = atoi(word[2]);
  user->plugin_01x000_poker->hand[1] = atoi(word[3]);
  user->plugin_01x000_poker->hand[2] = atoi(word[4]);
  user->plugin_01x000_poker->hand[3] = atoi(word[5]);
  user->plugin_01x000_poker->hand[4] = atoi(word[6]);

  sprintf(text, "%s fiddles the cards.\n", user->name);
  write_room(user->room, text);
}

/**** The New World Order **********************************/

/*** Show PO players ***/
plugin_01x000_show_po_players(user)
UR_OBJECT user;
{
  struct plugin_01x000_po_game *game;
  struct plugin_01x000_po_player *player;
  char turn_text[80];
  char text2[80];

  if (word_count < 3) {
        if (user->plugin_01x000_poker == NULL) {
          if ((game=plugin_01x000_get_po_game_here(user->room))==NULL) {
                write_user(user, "Which game are you interested in?\n");
                return;
          }
          /* else game = the game in this room */
        } else {
          game = user->plugin_01x000_poker->game;
        }
  } else {
        if ((game=plugin_01x000_get_po_game(word[2]))==NULL) {
          write_user(user, "No games by that name are being played.\n");
          return;
        }
  }

  /* show who is in */
  /* fix opening bet bug */
  /* fix bug in fold and quit */

  sprintf(text,  "\n~BT~FB*** Table information for game %s ***\n\n", game->name);
  write_user(user, text);
  write_user(user, "~FY~OLName         :  state  : Chips\n----------------------------------\n");
  
  switch (game->state) {
  case 0: sprintf(turn_text, "~FT<<< Turn to deal.~RS\n"); break;
  case 1: sprintf(turn_text, "~FT<<< Turn to bet.(1st rnd.)~RS\n"); break;
  case 2: sprintf(turn_text, "~FT<<< Turn to discard.~RS\n"); break;
  case 3: sprintf(turn_text, "~FT<<< Turn to bet.(2nd rnd.)~RS\n"); break;
  case 4: sprintf(turn_text, "~FR<<<\n~RS"); break;
  } 

  for (player = game->first_player; player != NULL; player = player->next) {
        sprintf(text, "%-12s : ", player->user->name);
        if (game->state == 0) {
          strcat(text, "~FR~OLwaiting~RS : ");
        } else {
          if (player->hand[0] == -1) {
                strcat(text, " ~FR~OLfolded~RS : ");
          } else {
                strcat(text, "~FG~OLplaying~RS : ");
          }
        }
        sprintf(text2, "$%-6d ", player->hist->total);
        strcat(text, text2);
        if (game->curr_player == player) {
          strcat(text, turn_text);
        } else {
          strcat(text, "\n");
        }
        write_user(user, text);
  }
  write_user(user, "----------------------------------\n");
  if ((game->state == 1)||(game->state == 3)) {
        if (user->plugin_01x000_poker != NULL) {
        sprintf(text, "You have put $%d into the pot during this betting round.\n", user->plugin_01x000_poker->putin);
        write_user(user, text); }
        sprintf(text, "The current bet is $%d.\n", game->bet );
        write_user(user, text);
  }
  sprintf(text, "The current pot is $%d.\n", game->pot );
  write_user(user, text);
}

plugin_01x000_check_poker_hist(user)
UR_OBJECT user;
{
static int done=0;

if (mesg_check_hour==thour && mesg_check_min==tmin) {
  if (done) return;
}
else {  done=0;  return;  }


done=1;

plugin_01x000_save_po_hist();

if (user)
  write_user(user, "~OL~FGSaved poker rankings.\n");
}


/************************************************************************/

/* My random function **/
int plugin_01x000_myRand(int max)
{
  int n;
  int mask;
  
  /* Mask out as many bits as possible */
  for (mask = 1; mask < max; mask *= 2)
        ;
  mask -= 1;
  
  /* Reroll until a number <= max is returned */
  do {
        n = random()&mask;
  } while (n >= max);

  return(n);
}

