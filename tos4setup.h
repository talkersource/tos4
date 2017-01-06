/*  T a l k e r O S                        version 4.03 public release
    ------------------------------------------------------------------

    Welcome to TalkerOS 4!   Begin making the customizations to your
    unique talker right HERE.  This file will control how your talker
    looks, responds, and functions.  Please follow the instructions
    that are provided in the documentation.  Many items look self-
    explanitory, but an incorrect setting may cause you many headaches
    and hours of debugging.  When experimenting with settings, it is
    suggested that you modify and test ONE SETTING AT A TIME to make
    it easier to pinpoint problems.

  Other files include:

        datafiles/config  -  General/Room/Link setup
                tos403.h  -  Main System Initialization (no mods required)
              security.h  -  Proprietary firewall setup
                 music.h  -  Pueblo Jukebox file setup


    TalkerOS is based on the NUTS 3.3.3 talker code by Neil Robertson.
    ------------------------------------------------------------------
     TalkerOS (c)1998, William Price             All Rights Reserved.
     http://maddhouse.com/~talkeros            talkeros@maddhouse.com
    ------------------------------------------------------------------  */


/* System Information
   ------------------
   Set the following values to complete the TalkerOS setup.
   It is important that you fill in each value with the correct
   information. */

char *reg_sysinfo[12]={
/* DON'T TOUCH     */ "*",
/* Talker's Name   */ "New Talker",
/* Serial Number   */ "000000000000",
/* Registered User */ "",
/* Server's DNS    */ "host.dns",
/* Server's IP     */ "207.0.0.1",
/* Talker's E-Mail */ "user@host.dns",
/* Talker's Website*/ "http://www.host.dns/~user/",
/* Sysop Real Name */ "My Name",
/* Sysop User Name */ "MyUserName",
/* Pueblo Web Dir. */ "media/",
/* Graphic Image   */ "img_title.gif"
};


/* High-Level Debugger
   -------------------
   TalkerOS includes some powerful features to assist you in developing
   additions to the code.  Some are more powerful than others and some
   can invade the privacy of other users.  If you wish to disable such
   debugging tools, set this to zero(0) and the lower-level (safe) tools
   will still be available.  If you wish to leave these higher-level
   tools enabled, set this to one(1).
   
   NOTE: Users will be informed in the .tinfo command if the higher
         level tools are in use, enabled, or disabled.                */

int tos_highlev_debug=0;


/* Level and Ranking Setup
   -----------------------
   TalkerOS supports 3 user levels, 3 administration levels, and 1
   system level.  In level_name[], the first three fields are the names
   of your user levels, fields 4 through 6 are the names of your
   administration levels, field 7 is the system level reserved for your
   TalkerBOT (if used) and should be set to "Bot".  The last field MUST
   be "*" since the talker uses this to determine the end of the levels.
*/

char *level_name[]={
"Newbie","User","SuperUser","Wizard","Admin","SysOp","Bot","*"
};

/* Additional Level Information
   ----------------------------
   TalkerOS will provide users with level information using the .ranks
   command.  This additional information is optional, but can be useful.
   Enter a requirement description for each level. (Up to 22 characters.)
   Then enter a short description.  Recommended size is less than 22 chars. */

char *lev_req[]={
"-",
"insert your",
"level promotion",
"info here.",
"-",
"-",
"(System Level)"
};

char *lev_desc[]={
"New user (guest)",
"Standard user",
"SuperUser (regular)",
"Helps Users/Newbies",
"Administration duties",
"Ruler",
"Automated Response User",
};

/* Pueblo-Enhancement Setup
   ------------------------
   TalkerOS offers compatibility with the Pueblo telnet client
   for Windows95.  To enable these features in your talker, set
   the defaults below. 1=yes 0=no */

int pueblo_enh=0;       /* Operate in Pueblo-enhanced mode? */
int pblo_usr_mm_def=1;  /* User set at login to hear/see multimedia? */
int pblo_usr_pg_def=1;  /* User set at login to hear pages? */


/* AutoAFK Feature
   ---------------
   This feature will automatically set any user AFK who has idled for
   over 1/3 of the idle_timeout setting.  This helps to keep people from
   getting frustrated when and idler doesn't respond, but isn't AFK.
   Set 1=ON, 0=OFF.  */
   
int auto_afk=0;		/* Will we assume a user is idle and set them AFK? */


/* Map Setup
   ---------
   TalkerOS supports multiple map files.  Maps are used to help your
   users find their way around the rooms of your talker.  If you use
   only one map file, call it "mapfile" and set the number below to 1.
   If you use more than one, name each map "mapfile.#" (where # is the
   map number).  If you are using more than one map, the file "mapfile"
   will be used as an index to list the other maps.  */

int totalmaps=1;


/* Swear and Word Ban List Setup
   -----------------------------
   These MUST be in lower case - the contains_swearing() function converts
   the string to be checked to lower case before it compares it against
   these. Also, even if you dont want to ban any words you must keep the 
   star in the array. */

char *swear_words[]={
"fuck","shit","cunt","goddam","bitch","whore","nigger","slut","asshole",
"pussy","penis","bastard","*"
};


/* System Text and Messaging Setup
   -------------------------------
   TalkerOS allows you to change a few common phrases in order to
   make them better-fit your talker's theme. */

/* System Error    */   char *syserror="SYSTEM ERROR!";
/* Room Not Found  */   char *nosuchroom="-=- Room does not exist. -=-\n";
/* User Not Found  */   char *nosuchuser="-=- User does not exist. -=-\n";
/* User Not Online */   char *notloggedon="-=- User is not connected. -=-\n";
/* Invisible Name  */   char *invisname="Someone";
/* Invisible user
   enters the room */   char *invisenter="The door opens mysteriously...\n";
/* Invisible user
   leaves the room */   char *invisleave="The door closes mysteriously...\n";
/* User->Invisible */   char *goinvis="disappears in a flash of light!";
/* User-> Visible  */   char *govis="suddenly appears is a poof of smoke!";
/* Teleport OUT    */   char *tele_out="steps into a dimensional rift and disappears!";
/* Teleport IN     */   char *tele_in ="steps out of a dimensional rift.";
/* Teleport MOVEOUT*/   char *tele_mvo="falls into a dimensional rift.";
/* Teleport MOVE IN*/   char *tele_mvi="falls out of a dimensional rift.";
/* Teleport DO MOVE*/   char *tele_mv ="creates a dimensional rift...";
/* Teleport USER   */   char *tele_usr="A dimensional rift forms below your feet and you fall in!!";
/* Swear Caught    */   char *noswearing="-=- Text contained banned word(s). -=-\n";
/* New user desc   */   char *newbiedesc="is a new user here.";


/* New Accounts Setup
   ------------------
   TalkerOS includes multiple ways of handling new accounts at your
   talker.  These options can be set below. 1=yes 0=no */

int req_read_rules=1; /* Require user to read .rules before .accreq? */
int autopromote=1;    /* Auto-promote to level 1 after .accreq? */
int nonewbies=0;      /* Lockout newbies by default?  (1=YES, 0=NO) */
int saveaccts=0;      /* Save userfile for level 0? */


/* Modular ColorCode Array
   -----------------------
   Changing these values will alter the color layout in your talker.
   You do not need to go through each line in the .c file to do this.
   Simply change the values in the following array. ORDER IS IMPORTANT,
   so please be careful when altering.  The description of the item
   you are altering is listed to the LEFT of the item.  */

char *colors[]={
/* Default   */ "~RS",    /* Highlight */ "~BB",    /* Text      */ "~FW",
/* Bold      */ "~OL",    /* System    */ "~FR",    /* Sys. Bold */ "~OL~FY",
/* Warning   */ "~FM",    /* Who User  */ "~FY",    /* Who Info  */ "~FT",
/* People Hi.*/ "~BR",    /* People    */ "~FR",    /* User      */ "~FR",
/* Self      */ "~FT",    /* Emote     */ "~RS",    /* SEmote    */ "~FT",
/* PEmote    */ "~FM",    /* Think     */ "~FY",    /* Tell User */ "~FG",
/* Tell Self */ "~FY",    /* Tell Text */ "~RS",    /* Shout     */ "~FT",
/* Mail Head */ "~OL~FG", /* Mail Date */ "~RS~FY", /* BoardHead */ "~OL~FT",
/* BoardDate */ "~RS"
};


/* TalkerBOT version 2.3 Setup
   ---------------------------
   TalkerOS includes a virtual user, called a BOT that can respond to
   'triggers' in the text said by your users.  This is the general setup.
*/

char *BOT_def_name="MyBot";  /* Default bot username (capitalize 1st letter) */

/* Define trigger information */
/* Trigger keywords: MUST be lowercase AND last MUST
                     be "*"!  This is very important! */

char *BOT_triggers[]={
"greet","kill","stab","pummel","kick","slap","punch","stupid","hello",
"hi ","hey","bye","kiss","lick","hug","thank","say","talk","speak","shut","sit",
"smile","is a","wink","how are you","what are you","who are you",
"*"};

/* Files for above keywords.  Each needs to be in the same order as those
   listed above.  There needs to be a default file of non-specific stuff,
   and its filename should be in the same location as the "*" above. */

char *BOT_trigger_files[]={
"greet","kill","stab","pummel","kick","slap","punch","stupidbot","hello",
"hello","hello","goodbye","kiss","lick","hug","thank","speak","speak","speak","shutup",
"sitdown","smile","iswhat","wink","howru","whatru","whoru",
"default"
};

