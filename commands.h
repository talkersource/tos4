/* ---------------------------------------------------
    TalkerOS version 4.03  -  Permanent Command Setup
   --------------------------------------------------- */

/* Original NUTS commands... (some have been deleted or renamed)
   a few have been added due to position requirements.           */
char *command[]={
"quit",    "look",     "mode",      "say",      "shout",
"who",     "people",   "help",      "shutdown", "news",
"tell",    "read",     "write",     "wipe",     "search",
"echo",	   "go",       "ignall",    "prompt",   "desc",
"inphr",   "outphr",   "public",    "private",  "letmein",
"invite",  "to",       "topic",     "move",     "bcast",
"emote",   "semote",   "pemote",    "remote",   "review",
"home",    "status",   "version",   "rmail",    "smail",
"dmail",   "from",     "profile",   "examine",  "rmst",
"rmsn",    "netstat",  "netdata",   "connect",  "disconnect",
"passwd",  "kill",     "promote",   "demote",   "listbans",
"ban",     "unban",    "hide",      "site",     "page",
"wizshout","commands", "muzzle",    "unmuzzle", "map",
"logging", "minlogin", "system",    "charecho", "clearline",
"fix",     "unfix",    "viewlog",   "accreq",   "revclr",
"clone",   "destroy",  "myclones",  "allclones","switch",
"csay",    "chear",    "rstat",     "swban",    "afk",
"cls",     "color",    "ignshout",  "igntell",  "suicide",
"delete",  "reboot",   "recount",   "revtell",  "*"
};

char *toscom[]={
/* TalkerOS Additions - USER COMMANDS */
"think",   "shift",    "sshift",    "sing",     "waitfor",
"clrtopic","set",      "tinfo",     "myroom",   "prstat",
"getout",  "rmdesc",   "rules",     "ranks",    "admins",
"alias",   "lalias",   "memo",      "ppic",     "paudio",
"jukebox", "idle",     "ignuser",   "exits",    "suggest",
"rsuggest",

/* TalkerOS Additions - ADMIN COMMANDS */
"duty",    "arrest",   "release",   "possess",  "warn",
"wizch",   "wizemote", "secho",     "settime",  "firewall",
"nonew",   "cloak",    "flc",       "tpromote", "tdemote",
"loadroom","prtoggle", "bot",       "plugreg",  "urename",
"debugger","awrite",   "aread",     "awipe",    "wsuggest",
"wizremove","memory",
"*"
};


/* Values of commands , used in switch in exec_com()... (original NUTS) */
enum comvals {
QUIT,     LOOK,     MODE,     SAY,      SHOUT,
WHO,      PEOPLE,   HELP,     SHUTDOWN, NEWS,
TELL,     READ,     WRITE,    WIPE,     SEARCH,
ECHO,     GO,       IGNALL,   PROMPT,   DESC,
INPHRASE, OUTPHRASE,PUBCOM,   PRIVCOM,  LETMEIN,
INVITE,   DIRECTSAY,TOPIC,    MOVE,     BCAST,
EMOTE,    SEMOTE,   PEMOTE,   REMOTE,   REVIEW,
HOME,     STATUS,   VER,      RMAIL,    SMAIL,
DMAIL,    FROM,     ENTPRO,   EXAMINE,  RMST,
RMSN,     NETSTAT,  NETDATA,  CONN,     DISCONN,
PASSWD,   KILL,     PROMOTE,  DEMOTE,   LISTBANS,
BAN,      UNBAN,    HIDE,     SITE,     WAKE,
WIZSHOUT, COMMANDS, MUZZLE,   UNMUZZLE, MAP,
LOGGING,  MINLOGIN, SYSTEM,   CHARECHO, CLEARLINE,
FIX,      UNFIX,    VIEWLOG,  ACCREQ,   REVCLR,
CREATE,   DESTROY,  MYCLONES, ALLCLONES,SWITCH,
CSAY,     CHEAR,    RSTAT,    SWBAN,    AFK,
CLS,      COLOUR,   IGNSHOUT, IGNTELL,  SUICIDE,
DELETE,   REBOOT,   RECOUNT,  REVTELL
} com_num;

enum toscomvals {  /* TalkerOS Commands as used in exec_com()'s alt. switch */
THINK,    SHIFT,    SSHIFT,   SING,     WAITFOR,
CLRTOPIC, SETUSER,  TLKRINFO, MYROOM,   PRSTAT,
GETOUT,   RMDESC,   RULES,    RANKS,    WIZLIST,
SETNAMES, GETNAMES, USERMEMO, PBLOIMG,  PBLOAUD,
JUKEBOX,  IDLETIME, IGNUSER,  LSTEXITS, SUGBRD,
RSUGBRD,

DUTY,     ARREST,   RELEASE,  POSSESS,  WARN,
WIZCH,    WIZCHE,   SECHO,    SETUTIME, FWINFO,
NONEWBIES,CLOAK,    FLCASE,   TPROMO,   TDEMO,
LOADRM,   ALLOWPR,  BOTCMDS,  VIEWPLUG, URENAMER,
DEBUGGER, ADBRDWT,  ADBRDRE,  ADBRDWI,  WSUGBRD,
WIZREM,   MEMVIEW
} toscom_num;


/* These are the minimum levels at which the commands can be executed. 
   Alter to suit. */
int com_level[]={
NEW, NEW, NEW, NEW, USER,
NEW, WIZ, NEW, GOD, USER,
USER,NEW, USER,WIZ, XUSR,
USER,NEW, USER,NEW, USER,
USER,USER,USER,USER,USER,
USER,XUSR,USER,WIZ, WIZ,
USER,USER,USER,USER,USER,
NEW, NEW, NEW, NEW, USER,
USER,USER,USER,USER,XUSR,
XUSR,WIZ, ARCH,GOD, GOD,
USER,ARCH,WIZ ,WIZ ,WIZ,
ARCH,ARCH,WIZ, WIZ, USER,
WIZ, NEW, WIZ, WIZ, NEW,
GOD, GOD, WIZ, NEW, ARCH,
GOD, GOD, WIZ ,NEW, XUSR,
ARCH,ARCH,ARCH,XUSR,ARCH,
ARCH,ARCH,WIZ, ARCH,USER,
USER,NEW, USER,USER,NEW,
GOD, ARCH,WIZ, XUSR
};

int toscom_level[]={
USER,XUSR,XUSR,XUSR,XUSR,
XUSR,USER,USER,XUSR,XUSR,
XUSR,XUSR,NEW, NEW, NEW,
XUSR,XUSR,XUSR,XUSR,XUSR,
XUSR,XUSR,USER,USER,USER,
USER,

WIZ, ARCH,ARCH,WIZ, WIZ,
WIZ, WIZ, GOD, GOD, ARCH,
WIZ, ARCH,ARCH,ARCH,ARCH,
GOD, ARCH,WIZ, ARCH,GOD,
GOD, WIZ, WIZ, GOD, GOD,
ARCH,GOD
};
