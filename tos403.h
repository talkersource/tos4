/* ======================================================
   ==   TalkerOS version 4.03  -  System Header File   ==
   ======================================================

   I m p o r t a n t   N o t i c e ! !
   -----------------------------------
   Please do not alter this file unless you *absolutely* know EXACTLY
   what you are doing.  Incorrect settings may cause your copy of
   TalkerOS to operate improperly or not at all.  This file initializes
   the main system of TalkerOS and NO modification is necessary.

*/

/* directories */
#define DATAFILES   "datafiles"
#define USERFILES   "userfiles"
#define HELPFILES   "helpfiles"
#define MAILSPOOL   "mailspool"
#define BOTFILES    "botfiles"
#define PLUGINFILES "plugins"

/* general filenames */
#define CONFIGFILE "config"
#define NEWSFILE   "newsfile"
#define MAPFILE    "mapfile"
#define MOTD1      "motd1"
#define MOTD2      "motd2"
#define SUGBOARD   "suggest"
#define WIZBOARD   "admin"

/* ban filenames */
#define SITEBAN    "site.ban"
#define USERBAN    "user.ban"
#define NEWBIEBAN  "new.ban"

/* log filenames */
#define SYSLOG     "system.log"
#define USERLOG    "user.log"
#define ACCREQLOG  "account.log"
#define FIRELOG    "firewall.log"
#define BANLOG     "ban.log"

/* array sizes & dimensions */
#define OUT_BUFF_SIZE 1000
#define MAX_WORDS     10
#define WORD_LEN      40
#define ARR_SIZE      1000
#define MAX_LINES     15
#define NUM_COLS      21

/* global variables */
#define USER_NAME_LEN  12
#define USER_DESC_LEN  30
#define AFK_MESG_LEN   60
#define PHRASE_LEN     40
#define PASS_LEN       20 /* only the 1st 8 chars will be used by crypt() though */
#define BUFSIZE        1000
#define ROOM_NAME_LEN  20
#define ROOM_LABEL_LEN 5
#define ROOM_DESC_LEN  810 /* 10 lines of 80 chars each + 10 nl */
#define TOPIC_LEN      60
#define MAX_LINKS      10
#define SERV_NAME_LEN  80
#define SITE_NAME_LEN  80
#define VERIFY_LEN     20
#define REVIEW_LINES   18
#define REVTELL_LINES  18
#define REVIEW_LEN     200

/* DNL (Date Number Length) will have to become 12 on Sun Sep 9 02:46:40 2001
   when all the unix timers will flip to 1000000000 :) */
#define DNL 11 

/* Room settings */
#define PUBLIC        0
#define PRIVATE       1
#define FIXED         2
#define FIXED_PUBLIC  2
#define FIXED_PRIVATE 3
#define JAIL_PRIVATE  4
#define PERSONAL_PRIV 5

/* Rank levels */
#define NEW    0     /* Newbie    */
#define USER   1     /* User      */
#define XUSR   2     /* SuperUser */
#define WIZ    3     /* Wizard    */
#define ARCH   4     /* Admin     */
#define GOD    5     /* Sysop     */
#define BOTLEV 6     /* Bot       */

/* User and clone types */
#define USER_TYPE          0
#define CLONE_TYPE         1
#define REMOTE_TYPE        2
#define BOT_TYPE           3
#define CLONE_HEAR_NOTHING 0
#define CLONE_HEAR_SWEARS  1
#define CLONE_HEAR_ALL     2

/* Netlink stuff */
#define UNCONNECTED 0 
#define INCOMING    1  
#define OUTGOING    2
#define DOWN        0
#define VERIFYING   1
#define UP          2
#define ALL         0
#define IN          1
#define OUT         2

/* Define Modular ColorCode Array Index */
#define CDEFAULT 0
#define CHIGHLIGHT 1
#define CTEXT 2
#define CBOLD 3
#define CSYSTEM 4
#define CSYSBOLD 5
#define CWARNING 6
#define CWHOUSER 7
#define CWHOINFO 8
#define CPEOPLEHI 9
#define CPEOPLE 10
#define CUSER 11
#define CSELF 12
#define CEMOTE 13
#define CSEMOTE 14
#define CPEMOTE 15
#define CTHINK 16
#define CTELLUSER 17
#define CTELLSELF 18
#define CTELL 19
#define CSHOUT 20
#define CMAILHEAD 21
#define CMAILDATE 22
#define CBOARDHEAD 23
#define CBOARDDATE 24

/* Define TalkerOS System Information */
#define TALKERNAME 1
#define SERIALNUM  2
#define REGUSER    3
#define SERVERDNS  4
#define SERVERIP   5 
#define TALKERMAIL 6
#define TALKERHTTP 7
#define SYSOPNAME  8
#define SYSOPUNAME 9
#define PUEBLOWEB 10
#define PUEBLOPIC 11

/* The elements vis, ignall, prompt, command_mode etc could all be bits in
   one flag variable as they're only ever 0 or 1, but I tried it and it
   made the code unreadable. Better to waste a few bytes */
struct user_struct {
        char name[USER_NAME_LEN+1];
        char desc[USER_DESC_LEN+1];
        char pass[PASS_LEN+6];
        char in_phrase[PHRASE_LEN+1],out_phrase[PHRASE_LEN+1];
        char buff[BUFSIZE],site[81],last_site[81],page_file[81];
        char mail_to[WORD_LEN+1],revbuff[REVTELL_LINES][REVIEW_LEN+2];
        char afk_mesg[AFK_MESG_LEN+1],inpstr_old[REVIEW_LEN+1];
        struct room_struct *room,*invite_room;
        int type,port,site_port,login,socket,attempts,buffpos,filepos;
        int vis,ignall,prompt,command_mode,muzzled,charmode_echo; 
        int level,misc_op,remote_com,edit_line,charcnt,warned;
        int accreq,last_login_len,ignall_store,clone_hear,afk;
        int edit_op,colour,ignshout,igntell,revline;
        time_t last_input,last_login,total_login,read_mail;
        char *malloc_start,*malloc_end;
        struct netlink_struct *netlink,*pot_netlink;
        struct user_struct *prev,*next,*owner;

        /* TalkerOS - USER STRUCTURE EXPANSIONS */
        int arrested,orig_level,duty,reverse,alert,gender,age,pstats;
        int cloaklev,flc,can_edit_rooms,readrules,primsg,idle;
        char waitfor[USER_NAME_LEN+1],alias[USER_NAME_LEN+1],ignuser[USER_NAME_LEN+1];
        char room_topic[TOPIC_LEN+1];
        char http[58],email[58];
        time_t created;

        /* TalkerOS - PUEBLO ENHANCEMENT SESSION VARIABLES */
        int pueblo,pueblo_mm,pueblo_pg,voiceprompt,pblodetect;
        char md5[30],pblover[5];

        /* Plugin additions */
        struct plugin_01x000_po_player *plugin_01x000_poker;
        };

typedef struct user_struct* UR_OBJECT;
UR_OBJECT user_first,user_last,user_debug;

struct room_struct {
        char name[ROOM_NAME_LEN+1];
        char label[ROOM_LABEL_LEN+1];
        char desc[ROOM_DESC_LEN+1];
        char topic[TOPIC_LEN+1];
        char revbuff[REVIEW_LINES][REVIEW_LEN+2];
        int inlink; /* 1 if room accepts incoming net links */
        int access; /* public , private etc */
        int revline; /* line number for review */
        int mesg_cnt;
        char netlink_name[SERV_NAME_LEN+1]; /* temp store for config parse */
        char link_label[MAX_LINKS][ROOM_LABEL_LEN+1]; /* temp store for parse */
        struct netlink_struct *netlink; /* for net links, 1 per room */
        struct room_struct *link[MAX_LINKS];
        struct room_struct *next;

        /* TalkerOS - ROOM STRUCTURE EXPANSION */
        int  personal;
        char owner[USER_NAME_LEN+1];
        };

typedef struct room_struct *RM_OBJECT;
RM_OBJECT room_first,room_last;
RM_OBJECT create_room();

/* Structure for net links, ie server initiates them */
struct netlink_struct {
        char service[SERV_NAME_LEN+1];
        char site[SITE_NAME_LEN+1];
        char verification[VERIFY_LEN+1];
        char buffer[ARR_SIZE*2];
        char mail_to[WORD_LEN+1];
        char mail_from[WORD_LEN+1];
        FILE *mailfile;
        time_t last_recvd; 
        int port,socket,type,connected;
        int stage,lastcom,allow,warned,keepalive_cnt;
        int ver_major,ver_minor,ver_patch;
        int TOSserver,TOSpeer,TOSver;  /* not used yet, but will be for
                                          TalkerOS peer data transfer */
        struct user_struct *mesg_user;
        struct room_struct *connect_room;
        struct netlink_struct *prev,*next;
        };

typedef struct netlink_struct *NL_OBJECT;
NL_OBJECT nl_first,nl_last;
NL_OBJECT create_netlink();

/* 
Colcode values equal the following:
RESET,BOLD,BLINK,REVERSE

Foreground & background colours in order..
BLACK,RED,GREEN,YELLOW/ORANGE,
BLUE,MAGENTA,TURQUIOSE,WHITE
*/

char *colcode[NUM_COLS]={
/* Standard stuff */
"\033[0m", "\033[1m", "\033[4m", "\033[5m", "\033[7m",
/* Foreground colour */
"\033[30m","\033[31m","\033[32m","\033[33m",
"\033[34m","\033[35m","\033[36m","\033[37m",
/* Background colour */
"\033[40m","\033[41m","\033[42m","\033[43m",
"\033[44m","\033[45m","\033[46m","\033[47m"
};

/* Codes used in a string to produce the colours when prepended with a '~' */
char *colcom[NUM_COLS]={
"RS","OL","UL","LI","RV",
"FK","FR","FG","FY",
"FB","FM","FT","FW",
"BK","BR","BG","BY",
"BB","BM","BT","BW"
};


char *month[12]={
"January","February","March","April","May","June",
"July","August","September","October","November","December"
};

char *day[7]={
"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

char *noyes1[]={ " NO","YES","  ?" };
char *noyes2[]={ "NO ","YES","?  " };
char *offon[]={ "OFF","ON ","ASK" };

char verification[SERV_NAME_LEN+1];
char text[ARR_SIZE*2],store_debug[ARR_SIZE*2];
char word[MAX_WORDS][WORD_LEN+1];
char wrd[8][81];
char progname[40],confile[40];
time_t boot_time;
jmp_buf jmpvar;

int port[3],listen_sock[3],wizport_level,minlogin_level;
int colour_def,password_echo,ignore_sigterm;
int max_users,max_clones,num_of_users,num_of_logins,heartbeat;
int login_idle_time,user_idle_time,config_line,word_count;
int tyear,tmonth,tday,tmday,twday,thour,tmin,tsec;
int mesg_life,system_logging,prompt_def,no_prompt;
int force_listen,gatecrash_level,min_private_users;
int ignore_mp_level,rem_user_maxlevel,rem_user_deflevel;
int destructed,mesg_check_hour,mesg_check_min,net_idle_time;
int keepalive_interval,auto_connect,ban_swearing,crash_action;
int time_out_afks,allow_caps_in_name,rs_countdown;
int charecho_def,time_out_maxlevel;
int totlogins,totnewbies;
time_t rs_announce,rs_which;
UR_OBJECT rs_user;

#if SYSTEM!=FREEBSD
extern char *sys_errlist[];
#endif
char *long_date();

/* TalkerOS Plugin Registry -- For Reduced Headaches: DO NOT MODIFY */
struct plugin_struct {
        char    name[27],author[22],ver[4],req_ver[4];
        char    registration[8];
        int     id,req_userfile,triggerable;
        struct plugin_struct *prev,*next;
        };
typedef struct plugin_struct *PL_OBJECT;
PL_OBJECT plugin_first,plugin_last;

struct plugin_cmd {
        char    command[10];    /* What will the command name be? */
        int     id,req_lev;     /* id = reference ... req_lev = required level */
        int     comnum;         /* Identify the command per plugin since
                                   some plugins may have more than 1. */
        struct plugin_cmd *prev,*next;
        struct plugin_struct *plugin;
        };
typedef struct plugin_cmd *CM_OBJECT;
CM_OBJECT cmds_first,cmds_last;


/* Genders - The males are duplicated in case there comes a time in which
             the opposite of the user's gender will need to be used.  This
             way, adding 1 to the gender will cause the opposite (unless
             the gender is 0-n/a). */
char *gender[]={ "n/a","Male","Female","Male" };
char *heshe[]={ "it","he","she","he" };
char *posgen[]={ "its","his","her","his" };
char *objgen[]={ "him/her","him","her","him" };
char *oppgen[]={ "thing","girl","guy","girl" };

int skipauth=0;
int allowpr=1;
int is_using_old_inpstr=0;
int debug_input=0;  /* CAUTION!:  NEVER set this value to 1. */
int highlev_debug_on=0;
char *lev_type[]={"USER","ADMIN"};
char TOS_VER[4]="4.0"; /* TalkerOS version information - DO NOT MODIFY */
char RUN_VER[4]="403"; /* Internal version information - I KILL YOU IF YOU DO */
