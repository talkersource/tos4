/*****************************************************************************
 TalkerOS ver 4.03 - Copyright (c) William Price 1998 - All rights reserved.

        TalkerOS is a very powerful version of the popular NUTS talker
        code by Neil Robertson.  Beginning in the winter of 1997,
        TalkerOS went under development to be one of the easiest, most
        powerful, and friendly talker code with virtually unlimited
        capacity to expand beyond its initial boundaries.  This was
        accomplished by the proprietary plugin system that makes this
        code unique, and the only one of its kind.

        - Easy plugin expandibility             - Built-in advanced features
        - TalkerBOT v2.3 standard               - Operating system with
        - PUEBLO-Multimedia enhancement           unique and advanced
        - Easy setup options designed to          plugin expandibility
          offer ease in choosing                - Familiar NUTS look & feel
          configuration settings                - Designed for an easy,
        - Advanced and dependable security        no-hassle upgrading process
          features, incl. a proprietary           to newer versions.
          firewall for administrators

-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
Notes to programmers:

   This code is designed to require VERY LITTLE modification.  This is
   so that even the most inexperienced user may set up a talker.  It is
   not meant in any way to say that modifications can not be made.

   However, the TalkerOS plugin system is a unique way to distribute your
   modifications and code enhancements.  For information on coding your
   custom modifications as plugins, please visit the TalkerOS website.
   Once coded in a FULLY compatible plugin format, your modifications
   can be exported to ANY other TalkerOS talker (of the same or higher
   version) without any trouble, if you so choose.

   If you submit a copy of your plugin to the TalkerOS website, it will
   be reviewed and possibly approved by the TalkerOS staff.  Once approved
   to be 100% compatible, it will be posted for use by other TalkerOS
   users, if you so choose.  This 100% compatibility certification is a
   guarantee that it will comply to the ease of use standards of TalkerOS.

   The writer of TalkerOS and its support staff will not answer questions
   regarding problems induced by the modification of other persons' plugins
   or the TalkerOS system code itself.  Once modified, you are on your own.
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
                     Last update: 18th November 1996

 This software is provided as is. It is not intended as any sort of bullet
 proof system for commercial operation (though you may use it to set up a 
 pay-to-use system, just don't attempt to sell the code itself) and I accept 
 no liability for any problems that may arise from you using it. Since this is 
 freeware (NOT public domain , I have not relinquished the copyright) you may 
 distribute it as you see fit and you may alter the code to suit your needs.

 Read the COPYRIGHT file for further information.

 Neil Robertson. 

 Email    : neil@ogham.demon.co.uk
 Home page: http://www.wso.co.uk/neil.html (need JavaScript enabled browser)
 NUTS page: http://www.wso.co.uk/nuts.html
 Newsgroup: alt.talkers.nuts

 NB: This program listing looks best when the tab length is 5 chars which is
 "set ts=5" in vi.
 *****************************************************************************/

#include <stdio.h>
#ifdef _AIX
#include <sys/select.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>

#include "tos403.h"
#include "tos4setup.h"
#include "commands.h"
#include "security.h"
#include "music.h"

/* ---------------------------------------
    Begin PLUGIN INCLUDE code lines here. 
   --------------------------------------- */

#include "./plugins/poker.c"
#include "./plugins/eightball.c"

/* -------------------------------------
    End PLUGIN INCLUDE code lines here. 
   ------------------------------------- */

#define VERSION "3.3.3"

/*** This function calls all the setup routines and also contains the
        main program loop ***/
main(argc,argv)
int argc;
char *argv[];
{
fd_set readmask; 
int i,len,oldpid; 
char inpstr[ARR_SIZE];
char *remove_first();
char plural[2][2]={"s",""};
UR_OBJECT user,next;
NL_OBJECT nl;

strcpy(progname,argv[0]);
if (argc<2) { strcpy(confile,CONFIGFILE);  totlogins=0;  totnewbies=0; }
else strcpy(confile,argv[1]);

/* Startup */
printf("\n<*> Loading TalkerOS version %s <*>   (NUTS base %s)\n\n",TOS_VER,VERSION);
init_globals();
write_syslog("\n*** Attempting to load TalkerOS ***\n",0,SYSLOG);
set_date_time();
init_signals();
load_and_parse_config();
init_sockets();
if (auto_connect) init_connections();
else printf("Skipping connect stage.\n");
check_messages(NULL,1);
/* TOS4: Check for additional plugins and verify components. */
tos4_load();

/* Run in background automatically. */
switch(fork()) {
        case -1: boot_exit(11);  /* fork failure */
        case  0: break; /* child continues */
        default: sleep(1); exit(0);  /* parent dies */
        }
reset_alarm();
printf("\n*** Booted with PID %d ***\n\n",getpid());
sprintf(text,"*** Booted successfully with PID %d %s ***\n\n",getpid(),long_date(1));
write_syslog(text,0,SYSLOG);

/**** Main program loop. *****/
setjmp(jmpvar); /* jump to here if we crash and crash_action = IGNORE */
while(1) {
        /* set up mask then wait */
        setup_readmask(&readmask);
        if (select(FD_SETSIZE,&readmask,0,0,0)==-1) continue;

        /* check for connection to listen sockets */
        for(i=0;i<3;++i) {
                if (FD_ISSET(listen_sock[i],&readmask)) 
                        accept_connection(listen_sock[i],i);
                }

        /* Cycle through client-server connections to other talkers */
        for(nl=nl_first;nl!=NULL;nl=nl->next) {
                no_prompt=0;
                if (nl->type==UNCONNECTED || !FD_ISSET(nl->socket,&readmask)) 
                        continue;
                /* See if remote site has disconnected */
                if (!(len=read(nl->socket,inpstr,sizeof(inpstr)-3))) {
                        if (nl->stage==UP)
                                sprintf(text,"NETLINK: Remote disconnect by %s.\n",nl->service);
                        else sprintf(text,"NETLINK: Remote disconnect by site %s.\n",nl->site);
                        write_syslog(text,1,SYSLOG);
                        sprintf(text,"%sSYSTEM:~RS Lost link to %s in the %s.\n",colors[CSYSBOLD],nl->service,nl->connect_room->name);
                        write_room(NULL,text);
                        shutdown_netlink(nl);
                        continue;
                        }
                inpstr[len]='\0'; 
                exec_netcom(nl,inpstr);
                }

        /* Cycle through users. Use a while loop instead of a for because
            user structure may be destructed during loop in which case we
            may lose the user->next link. */
        user=user_first;
        while(user!=NULL) {
                next=user->next; /* store in case user object is destructed */
                /* If remote user or clone ignore */
                if (user->type!=USER_TYPE) {  user=next;  continue; }

                /* see if any data on socket else continue */
                if (!FD_ISSET(user->socket,&readmask)) { user=next;  continue; }
        
                /* see if client (eg telnet) has closed socket */
                inpstr[0]='\0';
                if (!(len=read(user->socket,inpstr,sizeof(inpstr)))) {
                        disconnect_user(user);  user=next;
                        continue;
                        }
                /* ignore control code replies */
                if ((unsigned char)inpstr[0]==255) { user=next;  continue; }

                /* Deal with input chars. If the following if test succeeds we
                   are dealing with a character mode client so call function. */
                if (inpstr[len-1]>=32 || user->buffpos) {
                        if (get_charclient_line(user,inpstr,len)) goto GOT_LINE;
                        user=next;  continue;
                        }
                else terminate(inpstr);

                GOT_LINE:
                no_prompt=0;  
                com_num=-1;
                force_listen=0; 
                destructed=0;
                user->buff[0]='\0';  
                user->buffpos=0;
                user->idle = user->idle + ((int)(time(0) - user->last_input)/60);
                user->last_input=time(0);
                if (user->login) {
                        login(user,inpstr);  user=next;  continue;  
                        }

                /* Store DEBUG user ID */
                user_debug=user;

                /* If a dot on its own then execute last inpstr unless its a misc
                   op or the user is on a remote site */
                if (!user->misc_op) {

                        /* Store DEBUG text recieved */
                        sprintf(store_debug,"%s",inpstr);
                        if (debug_input) {
                                sprintf(text,"Input DEBUG:: User %s - %s\n",user_debug,inpstr);
                                write_syslog(text,1,SYSLOG);
                                }
                        if (!strcmp(inpstr,".") && user->inpstr_old[0]) {
                                strcpy(inpstr,user->inpstr_old);
                                sprintf(text,"%s\n",inpstr);
                                write_user(user,text);
                                is_using_old_inpstr=1;
                                }
                        /* else save current one for next time */
                        else {
                                if (inpstr[0]) strncpy(user->inpstr_old,inpstr,REVIEW_LEN);
                                is_using_old_inpstr=0;
                                }
                        }

                /* Main input check */
                clear_words();
                word_count=wordfind(inpstr);
                 if (user->afk) {
                        if (user->afk==2) {
                                if (!word_count) {  
                                        if (user->command_mode) prompt(user);
                                        user=next;  continue;  
                                        }
                                if (strcmp((char *)crypt(word[0],"NU"),user->pass)) {
                                        write_user(user,"Incorrect password.\n"); 
                                        prompt(user);  user=next;  continue;
                                        }
                                cls(user);
                                write_user(user,"Session unlocked, you are no longer AFK.\n");
                                echo_on(user);
                                }       
                        else write_user(user,"You are no longer AFK.\n");  
                        user->afk_mesg[0]='\0';
                        sprintf(text,"%s comes back from being AFK.\n",user->name);
                        if (user->vis) write_room_except(user->room,text,user);
                                else write_duty(user->level,text,user->room,user,0);
                        if (user->afk==2) {
                                user->afk=0;  prompt(user);  user=next;  continue;
                                }
                        user->afk=0;
                        if (user->primsg) {
                                sprintf(text,"--> You have had %d private message%s while AFK.  See ~FG.revtell\n",user->primsg,plural[(user->primsg==1)]);
                                write_user(user,text);
                                sprintf(text,"    ( Only the last %d messages are available. %d scrolled away. )\n",REVTELL_LINES,(user->primsg - REVTELL_LINES));
                                if (user->primsg > REVTELL_LINES) write_user(user,text);
                                }
                        }
                if (!word_count) {
                        if (misc_ops(user,inpstr))  {  user=next;  continue;  }
                        if (user->room==NULL) {
                                sprintf(text,"ACT %s NL\n",user->name);
                                write_sock(user->netlink->socket,text);
                                }
                        if (user->command_mode) prompt(user);
                        user=next;  continue;
                        }
                if (misc_ops(user,inpstr))  {  user=next;  continue;  }
                com_num=-1;
                if (user->flc) {
                        for (i=0; i<strlen(inpstr); i++)
                                inpstr[i] = tolower(inpstr[i]);
                        }
                if (user->command_mode || strchr(".;!<>-@[]/)#",inpstr[0]))
                        exec_com(user,inpstr);
                else if (!(chk_pblo(user,inpstr))) say(user,inpstr);
                if (!destructed) {
                        if (user->room!=NULL)  prompt(user); 
                        else {
                                switch(com_num) {
                                        case -1  : /* Unknown command */
                                        case HOME:
                                        case QUIT:
                                        case MODE:
                                        case PROMPT: 
                                        case SUICIDE:
                                        case REBOOT:
                                        case SHUTDOWN: prompt(user);
                                        }
                                }
                        }
                user=next;
                }
        } /* end while */
}


/************ MAIN LOOP FUNCTIONS ************/

/*** Set up readmask for select ***/
setup_readmask(mask)
fd_set *mask;
{
UR_OBJECT user;
NL_OBJECT nl;
int i;

FD_ZERO(mask);
for(i=0;i<3;++i) FD_SET(listen_sock[i],mask);
/* Do users */
for (user=user_first;user!=NULL;user=user->next) 
        if (user->type==USER_TYPE) FD_SET(user->socket,mask);

/* Do client-server stuff */
for(nl=nl_first;nl!=NULL;nl=nl->next) 
        if (nl->type!=UNCONNECTED) FD_SET(nl->socket,mask);
}


/*** Accept incoming connections on listen sockets ***/
accept_connection(lsock,num)
int lsock,num;
{
UR_OBJECT user,create_user();
NL_OBJECT create_netlink();
char *get_ip_address(),site[80];
struct sockaddr_in acc_addr;
int accept_sock,size;

size=sizeof(struct sockaddr_in);
accept_sock=accept(lsock,(struct sockaddr *)&acc_addr,&size);
if (num==2) {
        accept_server_connection(accept_sock,acc_addr);  return;
        }
strcpy(site,get_ip_address(acc_addr));
if (site_banned(site)) {
        write_sock(accept_sock,"\n\rLogins from your site/domain are banned.\n\n\r");
        close(accept_sock);
        sprintf(text,"Attempted login from banned site %s.\n",site);
        write_syslog(text,1,BANLOG);
        sprintf(text,"%s<>~RS %s%sAttempted connection from banned site.~RS %s(%s)\n",colors[CWARNING],colors[CBOLD],colors[CSYSBOLD],colors[CSYSTEM],site);
        write_duty(ARCH,text,NULL,NULL,2);
        return;
        }
more(NULL,accept_sock,MOTD1); /* send pre-login message */
if (num_of_users+num_of_logins>=max_users && !num) {
        write_sock(accept_sock,"\n\rSorry, the talker is full at the moment.\n\n\r");
        close(accept_sock);  
        sprintf(text,"%s<>~RS %s%sConnection Denied - Talker is full!\n",colors[CWARNING],colors[CBOLD],colors[CSYSBOLD]);
        write_duty(ARCH,text,NULL,NULL,2);
        return;
        }
if ((user=create_user())==NULL) {
        sprintf(text,"\n\r%s: unable to create session.\n\n\r",syserror);
        write_sock(accept_sock,text);
        close(accept_sock);  
        sprintf(text,"%s<>~RS %s%sConnection Denied - Couldn't create session!\n",colors[CWARNING],colors[CBOLD],colors[CSYSBOLD]);
        write_duty(ARCH,text,NULL,NULL,2);
        return;
        }
user->socket=accept_sock;
user->login=3;
user->last_input=time(0);
if (!num) user->port=port[0];
else {
        user->port=port[1];
        write_user(user,"\n-=- WizPort Login -=-\n");
        }
if (pueblo_enh) {
        sprintf(text,"TalkerOS version %s - This world is Pueblo 1.10 Enhanced.\n\nEnter your username: ",TOS_VER);
        write_user(user,text);
        }
else {
        sprintf(text,"TalkerOS version %s\n\nEnter your username: ",TOS_VER);
        write_user(user,text);
        }
strcpy(user->site,site);
user->site_port=(int)ntohs(acc_addr.sin_port);
echo_on(user);
sprintf(text,"%s<>~RS %sConnection established. ~RS%s(%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],user->site,user->site_port);
write_duty(ARCH,text,NULL,NULL,2);
num_of_logins++;
}


/*** Get net address of accepted connection ***/
char *get_ip_address(acc_addr)
struct sockaddr_in acc_addr;
{
static char site[80];
struct hostent *host;

strcpy(site,(char *)inet_ntoa(acc_addr.sin_addr)); /* get number addr */
if ((host=gethostbyaddr((char *)&acc_addr.sin_addr,4,AF_INET))!=NULL)
        strcpy(site,host->h_name); /* copy name addr. */
strtolower(site);
return site;
}


/*** See if users site is banned ***/
site_banned(site)
char *site;
{
FILE *fp;
char line[82],filename[80];

sprintf(filename,"%s/%s",DATAFILES,SITEBAN);
if (!(fp=fopen(filename,"r"))) return 0;
fscanf(fp,"%s",line);
while(!feof(fp)) {
        if (strstr(site,line)) {  fclose(fp);  return 1;  }
        fscanf(fp,"%s",line);
        }
fclose(fp);
return 0;
}


/*** See if user is banned ***/
user_banned(name)
char *name;
{
FILE *fp;
char line[82],filename[80];

sprintf(filename,"%s/%s",DATAFILES,USERBAN);
if (!(fp=fopen(filename,"r"))) return 0;
fscanf(fp,"%s",line);
while(!feof(fp)) {
        if (!strcmp(line,name)) {  fclose(fp);  return 1;  }
        fscanf(fp,"%s",line);
        }
fclose(fp);
return 0;
}

/*** See if users site is banned from making new accounts ***/
newbie_site_banned(site)
char *site;
{
FILE *fp;
char line[82],filename[80];

sprintf(filename,"%s/%s",DATAFILES,NEWBIEBAN);
if (!(fp=fopen(filename,"r"))) return 0;
fscanf(fp,"%s",line);
while(!feof(fp)) {
        if (strstr(site,line)) {  fclose(fp);  return 1;  }
        fscanf(fp,"%s",line);
        }
fclose(fp);
return 0;
}



/*** Attempt to get '\n' terminated line of input from a character
     mode client else store data read so far in user buffer. ***/
get_charclient_line(user,inpstr,len)
UR_OBJECT user;
char *inpstr;
int len;
{
int l;

for(l=0;l<len;++l) {
        /* see if delete entered */
        if (inpstr[l]==8 || inpstr[l]==127) {
                if (user->buffpos) {
                        user->buffpos--;
                        if (user->charmode_echo) write_user(user,"\b \b");
                        }
                continue;
                }
        user->buff[user->buffpos]=inpstr[l];
        /* See if end of line */
        if (inpstr[l]<32 || user->buffpos+2==ARR_SIZE) {
                terminate(user->buff);
                strcpy(inpstr,user->buff);
                if (user->charmode_echo) write_user(user,"\n");
                return 1;
                }
        user->buffpos++;
        }
if (user->charmode_echo
    && ((user->login!=2 && user->login!=1) || password_echo)) 
        write(user->socket,inpstr,len);
return 0;
}


/*** Put string terminate char. at first char < 32 ***/
terminate(str)
char *str;
{
int i;
for (i=0;i<ARR_SIZE;++i)  {
        if (*(str+i)<32) {  *(str+i)=0;  return;  } 
        }
str[i-1]=0;
}


/*** Get words from sentence. This function prevents the words in the 
     sentence from writing off the end of a word array element. This is
     difficult to do with sscanf() hence I use this function instead. ***/
wordfind(inpstr)
char *inpstr;
{
int wn,wpos;

wn=0; wpos=0;
do {
        while(*inpstr<33) if (!*inpstr++) return wn;
        while(*inpstr>32 && wpos<WORD_LEN-1) {
                word[wn][wpos]=*inpstr++;  wpos++;
                }
        word[wn][wpos]='\0';
        wn++;  wpos=0;
        } while (wn<MAX_WORDS);
return wn-1;
}


/*** clear word array etc. ***/
clear_words()
{
int w;
for(w=0;w<MAX_WORDS;++w) word[w][0]='\0';
word_count=0;
}


/************ PARSE CONFIG FILE **************/

load_and_parse_config()
{
FILE *fp;
char line[81]; /* Should be long enough */
char c,filename[80];
int i,section_in,got_init,got_rooms,got_tos;
RM_OBJECT rm1,rm2;
NL_OBJECT nl;

section_in=0;
got_init=0;
got_tos=0;
got_rooms=0;

sprintf(filename,"%s/%s",DATAFILES,confile);
printf("Parsing config file \"%s\"...\n",filename);
if (!(fp=fopen(filename,"r"))) {
        perror("NUTS: Can't open config file");  boot_exit(1);
        }
/* Main reading loop */
config_line=0;
fgets(line,81,fp);
while(!feof(fp)) {
        config_line++;
        for(i=0;i<8;++i) wrd[i][0]='\0';
        sscanf(line,"%s %s %s %s %s %s %s %s",wrd[0],wrd[1],wrd[2],wrd[3],wrd[4],wrd[5],wrd[6],wrd[7]);
        if (wrd[0][0]=='#' || wrd[0][0]=='\0') {
                fgets(line,81,fp);  continue;
                }
        /* See if new section */
        if (wrd[0][strlen(wrd[0])-1]==':') {
                if (!strcmp(wrd[0],"INIT:")) section_in=1;
                else if (!strcmp(wrd[0],"ROOMS:")) section_in=2;
                        else if (!strcmp(wrd[0],"SITES:")) section_in=3; 
                                else {
                                        fprintf(stderr,"NUTS: Unknown section header on line %d.\n",config_line);
                                        fclose(fp);  boot_exit(1);
                                        }
                }
        switch(section_in) {
                case 1: parse_init_section();  got_init=1;  break;
                case 2: parse_rooms_section(); got_rooms=1; break;
                case 3: parse_sites_section(); break;
                default:
                        fprintf(stderr,"NUTS: Section header expected on line %d.\n",config_line);
                        boot_exit(1);
                }
        fgets(line,81,fp);
        }
fclose(fp);

/* See if required sections were present (SITES is optional) and if
   required parameters were set. */
if (!got_init) {
        fprintf(stderr,"NUTS: INIT section missing from config file.\n");
        boot_exit(1);
        }
if (!got_rooms) {
        fprintf(stderr,"NUTS: ROOMS section missing from config file.\n");
        boot_exit(1);
        }
if (!verification[0]) {
        fprintf(stderr,"NUTS: Verification not set in config file.\n");
        boot_exit(1);
        }
if (!port[0]) {
        fprintf(stderr,"NUTS: Main port number not set in config file.\n");
        boot_exit(1);
        }
if (!port[1]) {
        fprintf(stderr,"NUTS: Wiz port number not set in config file.\n");
        boot_exit(1);
        }
if (!port[2]) {
        fprintf(stderr,"NUTS: Link port number not set in config file.\n");
        boot_exit(1);
        }
if (port[0]==port[1] || port[1]==port[2] || port[0]==port[2]) {
        fprintf(stderr,"NUTS: Port numbers must be unique.\n");
        boot_exit(1);
        }
if (room_first==NULL) {
        fprintf(stderr,"NUTS: No rooms configured in config file.\n");
        boot_exit(1);
        }

/* Parsing done, now check data is valid. Check room stuff first. */
for(rm1=room_first;rm1!=NULL;rm1=rm1->next) {
        for(i=0;i<MAX_LINKS;++i) {
                if (!rm1->link_label[i][0]) break;
                for(rm2=room_first;rm2!=NULL;rm2=rm2->next) {
                        if (rm1==rm2) continue;
                        if (!strcmp(rm1->link_label[i],rm2->label)) {
                                rm1->link[i]=rm2;  break;
                                }
                        }
                if (rm1->link[i]==NULL) {
                        fprintf(stderr,"NUTS: Room %s has undefined link label '%s'.\n",rm1->name,rm1->link_label[i]);
                        boot_exit(1);
                        }
                }
        }

/* Check external links */
for(rm1=room_first;rm1!=NULL;rm1=rm1->next) {
        for(nl=nl_first;nl!=NULL;nl=nl->next) {
                if (!strcmp(nl->service,rm1->name)) {
                        fprintf(stderr,"NUTS: Service name %s is also the name of a room.\n",nl->service);
                        boot_exit(1);
                        }
                if (rm1->netlink_name[0] 
                    && !strcmp(rm1->netlink_name,nl->service)) {
                        rm1->netlink=nl;  break;
                        }
                }
        if (rm1->netlink_name[0] && rm1->netlink==NULL) {
                fprintf(stderr,"NUTS: Service name %s not defined for room %s.\n",rm1->netlink_name,rm1->name);
                boot_exit(1);
                }
        }

/* Load room descriptions */
for(rm1=room_first;rm1!=NULL;rm1=rm1->next) {
        if (rm1->personal) continue;
        sprintf(filename,"%s/%s.R",DATAFILES,rm1->name);
        if (!(fp=fopen(filename,"r"))) {
                fprintf(stderr,"NUTS: Can't open description file for room %s.\n",rm1->name);
                sprintf(text,"ERROR: Couldn't open description file for room %s.\n",rm1->name);
                write_syslog(text,0,SYSLOG);
                continue;
                }
        i=0;
        c=getc(fp);
        while(!feof(fp)) {
                if (i==ROOM_DESC_LEN) {
                        fprintf(stderr,"NUTS: Description too long for room %s.\n",rm1->name);
                        sprintf(text,"ERROR: Description too long for room %s.\n",rm1->name);
                        write_syslog(text,0,SYSLOG);
                        break;
                        }
                rm1->desc[i]=c;  
                c=getc(fp);  ++i;
                }
        rm1->desc[i]='\0';
        fclose(fp);
        }
}



/*** Parse init section ***/
parse_init_section()
{
static int in_section=0;
int op,val,temp1,temp2;
char *ccode;
char *options[]={ 
"mainport","wizport","linkport","system_logging","minlogin_level","mesg_life",
"wizport_level","prompt_def","gatecrash_level","min_private","ignore_mp_level",
"rem_user_maxlevel","rem_user_deflevel","verification","mesg_check_time",
"max_users","heartbeat","login_idle_time","user_idle_time","password_echo",
"ignore_sigterm","auto_connect","max_clones","ban_swearing","crash_action",
"colour_def","time_out_afks","allow_caps_in_name","charecho_def",
"time_out_maxlevel","*"
};

if (!strcmp(wrd[0],"INIT:")) { 
        if (++in_section>1) {
                fprintf(stderr,"NUTS: Unexpected INIT section header on line %d.\n",config_line);
                boot_exit(1);
                }
        return;
        }
op=0;
while(strcmp(options[op],wrd[0])) {
        if (options[op][0]=='*') {
                fprintf(stderr,"NUTS: Unknown INIT option on line %d.\n",config_line);
                boot_exit(1);
                }
        ++op;
        }
if (!wrd[1][0]) {
        fprintf(stderr,"NUTS: Required parameter missing on line %d.\n",config_line);
        boot_exit(1);
        }
if (wrd[2][0] && wrd[2][0]!='#') {
        fprintf(stderr,"NUTS: Unexpected word following init parameter on line %d.\n",config_line);
        boot_exit(1);
        }
val=atoi(wrd[1]);
switch(op) {
        case 0: /* main port */
        case 1:
        case 2:
        if ((port[op]=val)<1 || val>65535) {
                fprintf(stderr,"NUTS: Illegal port number on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 3:  
        if ((system_logging=onoff_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: System_logging must be ON or OFF on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 4:
        if ((minlogin_level=get_level(wrd[1]))==-1) {
                if (!strcmp(wrd[1],"NONE")) {
                        minlogin_level=-1;
                        }
                else {
                        fprintf(stderr,"NUTS: Unknown level specifier for minlogin_level on line %d.\n",config_line);
                        boot_exit(1);
                        }
                }
        return;

        case 5:  /* message lifetime */
        if ((mesg_life=val)<1) {
                fprintf(stderr,"NUTS: Illegal message lifetime on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 6: /* wizport_level */
        if ((wizport_level=get_level(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Unknown level specifier for wizport_level on line %d.\n",config_line);
                boot_exit(1);   
                }
        return;

        case 7: /* prompt defaults */
        if ((prompt_def=onoff_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Prompt_def must be ON or OFF or PROMPT on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 8: /* gatecrash level */
        if ((gatecrash_level=get_level(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Unknown level specifier for gatecrash_level on line %d.\n",config_line);
                boot_exit(1);   
                }
        return;

        case 9:
        if (val<1) {
                fprintf(stderr,"NUTS: Number too low for min_private_users on line %d.\n",config_line);
                boot_exit(1);
                }
        min_private_users=val;
        return;

        case 10:
        if ((ignore_mp_level=get_level(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Unknown level specifier for ignore_mp_level on line %d.\n",config_line);
                boot_exit(1);   
                }
        return;

        case 11: 
        /* Max level a remote user can remotely log in if he doesn't have a local
           account. ie if level set to WIZ a GOD can only be a WIZ if logging in 
           from another server unless he has a local account of level GOD */
        if ((rem_user_maxlevel=get_level(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Unknown level specifier for rem_user_maxlevel on line %d.\n",config_line);
                boot_exit(1);   
                }
        return;

        case 12:
        /* Default level of remote user who does not have an account on site and
           connection is from a server of version 3.3.0 or lower. */
        if ((rem_user_deflevel=get_level(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Unknown level specifier for rem_user_deflevel on line %d.\n",config_line);
                boot_exit(1);   
                }
        return;

        case 13:
        if (strlen(wrd[1])>VERIFY_LEN) {
                fprintf(stderr,"NUTS: Verification too long on line %d.\n",config_line);
                boot_exit(1);   
                }
        strcpy(verification,wrd[1]);
        return;

        case 14: /* mesg_check_time */
        if (wrd[1][2]!=':'
            || strlen(wrd[1])>5
            || !isdigit(wrd[1][0]) 
            || !isdigit(wrd[1][1])
            || !isdigit(wrd[1][3]) 
            || !isdigit(wrd[1][4])) {
                fprintf(stderr,"NUTS: Invalid message check time on line %d.\n",config_line);
                boot_exit(1);
                }
        sscanf(wrd[1],"%d:%d",&mesg_check_hour,&mesg_check_min);
        if (mesg_check_hour>23 || mesg_check_min>59) {
                fprintf(stderr,"NUTS: Invalid message check time on line %d.\n",config_line);
                boot_exit(1);   
                }
        return;

        case 15:
        if ((max_users=val)<1) {
                fprintf(stderr,"NUTS: Invalid value for max_users on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 16:
        if ((heartbeat=val)<1) {
                fprintf(stderr,"NUTS: Invalid value for heartbeat on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 17:
        if ((login_idle_time=val)<10) {
                fprintf(stderr,"NUTS: Invalid value for login_idle_time on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 18:
        if ((user_idle_time=val)<10) {
                fprintf(stderr,"NUTS: Invalid value for user_idle_time on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 19: 
        if ((password_echo=yn_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Password_echo must be YES or NO on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 20: 
        if ((ignore_sigterm=yn_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Ignore_sigterm must be YES or NO on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 21:
        if ((auto_connect=yn_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Auto_connect must be YES or NO on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 22:
        if ((max_clones=val)<0) {
                fprintf(stderr,"NUTS: Invalid value for max_clones on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 23:
        if ((ban_swearing=yn_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Ban_swearing must be YES or NO on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 24:
        if (!strcmp(wrd[1],"NONE")) crash_action=0;
        else if (!strcmp(wrd[1],"IGNORE")) crash_action=1;
                else if (!strcmp(wrd[1],"REBOOT")) crash_action=2;
                        else {
                                fprintf(stderr,"NUTS: Crash_action must be NONE, IGNORE or REBOOT on line %d.\n",config_line);
                                boot_exit(1);
                                }
        return;

        case 25:
        if ((colour_def=onoff_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Colour_def must be ON or OFF or PROMPT on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 26:
        if ((time_out_afks=yn_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Time_out_afks must be YES or NO on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 27:
        if ((allow_caps_in_name=yn_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Allow_caps_in_name must be YES or NO on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 28:
        if ((charecho_def=onoff_check(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Charecho_def must be ON or OFF or PROMPT on line %d.\n",config_line);
                boot_exit(1);
                }
        return;

        case 29:
        if ((time_out_maxlevel=get_level(wrd[1]))==-1) {
                fprintf(stderr,"NUTS: Unknown level specifier for time_out_maxlevel on line %d.\n",config_line);
                boot_exit(1);   
                }
        return;
        }
}



/*** Parse rooms section ***/
parse_rooms_section()
{
static int in_section=0;
int i,have_jail;
char *ptr1,*ptr2,c;
RM_OBJECT room;

have_jail=0;
if (!strcmp(wrd[0],"ROOMS:")) {
        if (++in_section>1) {
                fprintf(stderr,"NUTS: Unexpected ROOMS section header on line %d.\n",config_line);
                boot_exit(1);
                }
        return;
        }
if (!wrd[2][0]) {
        fprintf(stderr,"NUTS: Required parameter(s) missing on line %d.\n",config_line);
        boot_exit(1);
        }
if (strlen(wrd[0])>ROOM_LABEL_LEN) {
        fprintf(stderr,"NUTS: Room label too long on line %d.\n",config_line);
        boot_exit(1);
        }
if (strlen(wrd[1])>ROOM_NAME_LEN) {
        fprintf(stderr,"NUTS: Room name too long on line %d.\n",config_line);
        boot_exit(1);
        }
/* Check for duplicate label or name */
for(room=room_first;room!=NULL;room=room->next) {
        if (!strcmp(room->label,wrd[0])) {
                fprintf(stderr,"NUTS: Duplicate room label on line %d.\n",config_line);
                boot_exit(1);
                }
        if (!strcmp(room->name,wrd[1])) {
                fprintf(stderr,"NUTS: Duplicate room name on line %d.\n",config_line);
                boot_exit(1);
                }
        }
room=create_room();
strcpy(room->label,wrd[0]);
strcpy(room->name,wrd[1]);

/* Parse internal links bit ie hl,gd,of etc. MUST NOT be any spaces between
   the commas */
i=0;
ptr1=wrd[2];
ptr2=wrd[2];
while(1) {
        while(*ptr2!=',' && *ptr2!='\0') ++ptr2;
        if (*ptr2==',' && *(ptr2+1)=='\0') {
                fprintf(stderr,"NUTS: Missing link label on line %d.\n",config_line);
                boot_exit(1);
                }
        c=*ptr2;  *ptr2='\0';
        if (!strcmp(ptr1,room->label)) {
                fprintf(stderr,"NUTS: Room has a link to itself on line %d.\n",config_line);
                boot_exit(1);
                }
        strcpy(room->link_label[i],ptr1);
        if (c=='\0') break;
        if (++i>=MAX_LINKS) {
                fprintf(stderr,"NUTS: Too many links on line %d.\n",config_line);
                boot_exit(1);
                }
        *ptr2=c;
        ptr1=++ptr2;  
        }

/* Parse access privs */
room->personal=0;
if (wrd[3][0]=='#') {  room->access=PUBLIC;  return;  }
if (!wrd[3][0] || !strcmp(wrd[3],"BOTH")) room->access=PUBLIC;
else if (!strcmp(wrd[3],"PUB")) room->access=FIXED_PUBLIC;
        else if (!strcmp(wrd[3],"PRIV")) room->access=FIXED_PRIVATE;
                else if (!strcmp(wrd[3],"JAIL")) {
                        if (have_jail) {
                                fprintf(stderr,"TalkerOS:  Too many rooms with JAIL designation!  No more than 1 allowed.\n");
                                boot_exit(1);
                                }
                        room->access=FIXED_PRIVATE;
                        for(i=0;i<MAX_LINKS;++i) {
                                room->link_label[i][0]='\0';  room->link[i]=NULL;
                                }
                        }
                        else if (!strcmp(wrd[3],"PERSONAL")) {
                                room->access=PRIVATE;
                                room->owner[0]='\0';
                                room->personal=1;
                                }
                else {
                        fprintf(stderr,"NUTS: Unknown room access type on line %d.\n",config_line);
                        boot_exit(1);
                        }
/* Parse external link stuff */
if (!wrd[4][0] || wrd[4][0]=='#') return;
if (!strcmp(wrd[4],"ACCEPT")) {  
        if (wrd[5][0] && wrd[5][0]!='#') {
                fprintf(stderr,"NUTS: Unexpected word following ACCEPT keyword on line %d.\n",config_line);
                boot_exit(1);
                }
        if (room->personal) {
                fprintf(stderr,"TalkerOS:  Personal Room cannot contain NETLINK!\n");
                boot_exit(1);
                }
        if (room->link_label[0]=='\0') {
                fprintf(stderr,"TalkerOS:  NETLINK cannot be contained in the Jail.\n");
                boot_exit(1);
                }
        room->inlink=1;  
        return;
        }
if (!strcmp(wrd[4],"CONNECT")) {
        if (!wrd[5][0]) {
                fprintf(stderr,"NUTS: External link name missing on line %d.\n",config_line);
                boot_exit(1);
                }
        if (wrd[6][0] && wrd[6][0]!='#') {
                fprintf(stderr,"NUTS: Unexpected word following external link name on line %d.\n",config_line);
                boot_exit(1);
                }
        if (room->personal) {
                fprintf(stderr,"TalkerOS:  Personal Room cannot contain NETLINK!\n");
                boot_exit(1);
                }
        if (room->link_label[0]=='\0') {
                fprintf(stderr,"TalkerOS:  NETLINK cannot be contained in the Jail.\n");
                boot_exit(1);
                }
        strcpy(room->netlink_name,wrd[5]);
        return;
        }
fprintf(stderr,"NUTS: Unknown connection option on line %d.\n",config_line);
boot_exit(1);
}



/*** Parse sites section ***/
parse_sites_section()
{
NL_OBJECT nl;
static int in_section=0;

if (!strcmp(wrd[0],"SITES:")) { 
        if (++in_section>1) {
                fprintf(stderr,"NUTS: Unexpected SITES section header on line %d.\n",config_line);
                boot_exit(1);
                }
        return;
        }
if (!wrd[3][0]) {
        fprintf(stderr,"NUTS: Required parameter(s) missing on line %d.\n",config_line);
        boot_exit(1);
        }
if (strlen(wrd[0])>SERV_NAME_LEN) {
        fprintf(stderr,"NUTS: Link name length too long on line %d.\n",config_line);
        boot_exit(1);
        }
if (strlen(wrd[3])>VERIFY_LEN) {
        fprintf(stderr,"NUTS: Verification too long on line %d.\n",config_line);
        boot_exit(1);
        }
if ((nl=create_netlink())==NULL) {
        fprintf(stderr,"NUTS: Memory allocation failure creating netlink on line %d.\n",config_line);
        boot_exit(1);
        }
if (!wrd[4][0] || wrd[4][0]=='#' || !strcmp(wrd[4],"ALL")) nl->allow=ALL;
else if (!strcmp(wrd[4],"IN")) nl->allow=IN;
        else if (!strcmp(wrd[4],"OUT")) nl->allow=OUT;
                else {
                        fprintf(stderr,"NUTS: Unknown netlink access type on line %d.\n",config_line);
                        boot_exit(1);
                        }
if ((nl->port=atoi(wrd[2]))<1 || nl->port>65535) {
        fprintf(stderr,"NUTS: Illegal port number on line %d.\n",config_line);
        boot_exit(1);
        }
strcpy(nl->service,wrd[0]);
strtolower(wrd[1]);
strcpy(nl->site,wrd[1]);
strcpy(nl->verification,wrd[3]);
}


yn_check(wd)
char *wd;
{
if (!strcmp(wd,"YES")) return 1;
if (!strcmp(wd,"NO")) return 0;
if (!strcmp(wd,"PROMPT")) return 2;  /* See explanation of PROMPT below. */
return -1;
}


onoff_check(wd)
char *wd;
{
if (!strcmp(wd,"ON")) return 1;
if (!strcmp(wd,"OFF")) return 0;
if (!strcmp(wd,"PROMPT")) return 2;  /* Used for cases like color, where you
                                         want the user to have color as a
                                         default, but you need it off for
                                         those people who do not have color
                                         terminals.  If it is set to PROMPT,
                                         it will ask every new user at login
                                         if they want to use color or not.  */
return -1;
}


/************ INITIALISATION FUNCTIONS *************/

/*** Initialise globals ***/
init_globals()
{
verification[0]='\0';
port[0]=0;
port[1]=0;
port[2]=0;
auto_connect=1;
max_users=50;
max_clones=1;
ban_swearing=0;
heartbeat=2;
keepalive_interval=60; /* DO NOT TOUCH!!! */
net_idle_time=300; /* Must be > than the above */
login_idle_time=180;
user_idle_time=300;
time_out_afks=0;
wizport_level=WIZ;
minlogin_level=-1;
mesg_life=1;
no_prompt=0;
num_of_users=0;
num_of_logins=0;
system_logging=1;
password_echo=0;
ignore_sigterm=0;
crash_action=2;
prompt_def=1;
colour_def=1;
charecho_def=0;
time_out_maxlevel=USER;
mesg_check_hour=0;
mesg_check_min=0;
allow_caps_in_name=1;
rs_countdown=0;
rs_announce=0;
rs_which=-1;
rs_user=NULL;
gatecrash_level=BOTLEV+1; /* minimum user level which can enter private rooms */
min_private_users=2; /* minimum num. of users in room before can set to priv */
ignore_mp_level=BOTLEV; /* User level which can ignore the above var. */
rem_user_maxlevel=USER;
rem_user_deflevel=USER;
user_first=NULL;
user_last=NULL;
room_first=NULL;
room_last=NULL; /* This variable isn't used yet */
nl_first=NULL;
nl_last=NULL;
clear_words();
time(&boot_time);
totlogins=0;
totnewbies=0;
}


/*** Initialise the signal traps etc ***/
init_signals()
{
void sig_handler();

signal(SIGTERM,sig_handler);
signal(SIGSEGV,sig_handler);
signal(SIGBUS,sig_handler);
signal(SIGILL,SIG_IGN);
signal(SIGTRAP,SIG_IGN);
signal(SIGIOT,SIG_IGN);
signal(SIGTSTP,SIG_IGN);
signal(SIGCONT,SIG_IGN);
signal(SIGHUP,SIG_IGN);
signal(SIGINT,SIG_IGN);
signal(SIGQUIT,SIG_IGN);
signal(SIGABRT,SIG_IGN);
signal(SIGFPE,SIG_IGN);
signal(SIGPIPE,SIG_IGN);
signal(SIGTTIN,SIG_IGN);
signal(SIGTTOU,SIG_IGN);
}


/*** Talker signal handler function. Can either shutdown , ignore or reboot
        if a unix error occurs though if we ignore it we're living on borrowed
        time as usually it will crash completely after a while anyway. ***/
void sig_handler(sig)
int sig;
{
force_listen=1;
switch(sig) {
        case SIGTERM:
        if (ignore_sigterm) {
                write_syslog("SIGTERM (termination) signal received - ignoring.\n",1,SYSLOG);
                return;
                }
        sprintf(text,"\n\n%sSYSTEM:%s~LI SIGTERM received, initiating shutdown!\n\n",colors[CSYSBOLD],colors[CSYSTEM]);
        write_room(NULL,text);
        talker_shutdown(NULL,"a termination signal (SIGTERM)",0); 

        case SIGSEGV:
        write_syslog("TalkerOS Debugger - SYSTEM CRASH ANALYSIS\n",1,SYSLOG);
        sprintf(text,"                -----------------------------------------\n                    Last User: %s\n",user_debug->name);
        write_syslog(text,0,SYSLOG);
        sprintf(text,"                Text Recieved: %s\n",store_debug);
        write_syslog(text,0,SYSLOG);
        switch(crash_action) {
                case 0: 
                sprintf(text,"\n\n\07%sSYSTEM:%s~LI PANIC - Segmentation fault, initiating shutdown!\n\n",colors[CSYSBOLD],colors[CSYSTEM]);
                write_room(NULL,text);
                talker_shutdown(NULL,"a segmentation fault (SIGSEGV)",0); 

                case 1: 
                sprintf(text,"\n\n\07%sSYSTEM:%s~LI WARNING - A segmentation fault has just occured!\n\n",colors[CSYSBOLD],colors[CSYSTEM]);
                write_room(NULL,text);
                write_syslog("WARNING: A segmentation fault occured!\n",1,SYSLOG);
                longjmp(jmpvar,0);

                case 2:
                sprintf(text,"\n\n\07%sSYSTEM:%s~LI PANIC - Segmentation fault, initiating reboot!\n\n",colors[CSYSBOLD],colors[CSYSTEM]);
                write_room(NULL,text);
                talker_shutdown(NULL,"a segmentation fault (SIGSEGV)",1);
                }

        case SIGBUS:
        switch(crash_action) {
                case 0:
                sprintf(text,"\n\n\07%sSYSTEM:%s~LI PANIC - Bus error, initiating shutdown!\n\n",colors[CSYSBOLD],colors[CSYSTEM]);
                write_room(NULL,text);
                talker_shutdown(NULL,"a bus error (SIGBUS)",0);

                case 1:
                sprintf(text,"\n\n\07%sSYSTEM:%s~LI WARNING - A bus error has just occured!\n\n",colors[CSYSBOLD],colors[CSYSTEM]);
                write_room(NULL,text);
                write_syslog("WARNING: A bus error occured!\n",1,SYSLOG);
                longjmp(jmpvar,0);

                case 2:
                sprintf(text,"\n\n\07%sSYSTEM:%s~LI PANIC - Bus error, initiating reboot!\n\n",colors[CSYSBOLD],colors[CSYSTEM]);
                write_room(NULL,text);
                talker_shutdown(NULL,"a bus error (SIGBUS)",1);
                }
        }
}

        
/*** Initialise sockets on ports ***/
init_sockets()
{
struct sockaddr_in bind_addr;
int i,on,size;

printf("Initialising sockets on ports: %d, %d, %d\n",port[0],port[1],port[2]);
on=1;
size=sizeof(struct sockaddr_in);
bind_addr.sin_family=AF_INET;
bind_addr.sin_addr.s_addr=INADDR_ANY;
for(i=0;i<3;++i) {
        /* create sockets */
        if ((listen_sock[i]=socket(AF_INET,SOCK_STREAM,0))==-1) boot_exit(i+2);

        /* allow reboots on port even with TIME_WAITS */
        setsockopt(listen_sock[i],SOL_SOCKET,SO_REUSEADDR,(char *)&on,sizeof(on));

        /* bind sockets and set up listen queues */
        bind_addr.sin_port=htons(port[i]);
        if (bind(listen_sock[i],(struct sockaddr *)&bind_addr,size)==-1) 
                boot_exit(i+5);
        if (listen(listen_sock[i],10)==-1) boot_exit(i+8);

        /* Set to non-blocking , do we need this? Not really. */
        fcntl(listen_sock[i],F_SETFL,O_NDELAY);
        }
}


/*** Initialise connections to remote servers. Basically this tries to connect
     to the services listed in the config file and it puts the open sockets in 
        the NL_OBJECT linked list which the talker then uses ***/
init_connections()
{
NL_OBJECT nl;
RM_OBJECT rm;
int ret,cnt=0;

printf("Connecting to remote servers...\n");
errno=0;
for(rm=room_first;rm!=NULL;rm=rm->next) {
        if ((nl=rm->netlink)==NULL) continue;
        ++cnt;
        printf("  Trying service %s at %s %d: ",nl->service,nl->site,nl->port);
        fflush(stdout);
        if ((ret=connect_to_site(nl))) {
                if (ret==1) {
                        printf("%s.\n",sys_errlist[errno]);
                        sprintf(text,"NETLINK: Failed to connect to %s: %s.\n",nl->service,sys_errlist[errno]);
                        }
                else {
                        printf("Unknown hostname.\n");
                        sprintf(text,"NETLINK: Failed to connect to %s: Unknown hostname.\n",nl->service);
                        }
                write_syslog(text,1,SYSLOG);
                }
        else {
                printf("CONNECTED.\n");
                sprintf(text,"NETLINK: Connected to %s (%s %d).\n",nl->service,nl->site,nl->port);
                write_syslog(text,1,SYSLOG);
                nl->connect_room=rm;
                }
        }
if (cnt) printf("  See system log for any further information.\n");
else printf("  No remote connections configured.\n");
}


/*** Do the actual connection ***/
connect_to_site(nl)
NL_OBJECT nl;
{
struct sockaddr_in con_addr;
struct hostent *he;
int inetnum;
char *sn;

sn=nl->site;
/* See if number address */
while(*sn && (*sn=='.' || isdigit(*sn))) sn++;

/* Name address given */
if(*sn) {
        if(!(he=gethostbyname(nl->site))) return 2;
        memcpy((char *)&con_addr.sin_addr,he->h_addr,(size_t)he->h_length);
        }
/* Number address given */
else {
        if((inetnum=inet_addr(nl->site))==-1) return 1;
        memcpy((char *)&con_addr.sin_addr,(char *)&inetnum,(size_t)sizeof(inetnum));
        }
/* Set up stuff and disable interrupts */
if ((nl->socket=socket(AF_INET,SOCK_STREAM,0))==-1) return 1;
con_addr.sin_family=AF_INET;
con_addr.sin_port=htons(nl->port);
signal(SIGALRM,SIG_IGN);

/* Attempt the connect. This is where the talker may hang. */
if (connect(nl->socket,(struct sockaddr *)&con_addr,sizeof(con_addr))==-1) {
        reset_alarm();  return 1;
        }
reset_alarm();
nl->type=OUTGOING;
nl->stage=VERIFYING;
nl->last_recvd=time(0);
return 0;
}

        

/************* WRITE FUNCTIONS ************/

/*** Write a NULL terminated string to a socket ***/
write_sock(sock,str)
int sock;
char *str;
{
write(sock,str,strlen(str));
}



/*** Send message to user ***/
write_user(user,str)
UR_OBJECT user;
char *str;
{
int buffpos,sock,i,pblo;
char *start,buff[OUT_BUFF_SIZE],mesg[ARR_SIZE],*colour_com_strip();
if (user==NULL) return;
if (user->type==BOT_TYPE) return;
if (user->type==REMOTE_TYPE) {
        if (user->netlink->ver_major<=3 
            && user->netlink->ver_minor<2) str=colour_com_strip(str);
        if (str[strlen(str)-1]!='\n') 
                sprintf(mesg,"MSG %s\n%s\nEMSG\n",user->name,str);
        else sprintf(mesg,"MSG %s\n%sEMSG\n",user->name,str);
        write_sock(user->netlink->socket,mesg);
        return;
        }
start=str;
buffpos=0;
sock=user->socket;
/* Process string and write to buffer. We use pointers here instead of arrays 
   since these are supposedly much faster (though in reality I guess it depends
   on the compiler) which is necessary since this routine is used all the 
   time. */
while(*str) {
        if (*str=='\n') {
                if (buffpos>OUT_BUFF_SIZE-6) {
                        write(sock,buff,buffpos);  buffpos=0;
                        }
                /* Reset terminal before every newline */
                if (user->colour) {
                        memcpy(buff+buffpos,"\033[0m",4);  buffpos+=4;
                        }
                *(buff+buffpos)='\n';  *(buff+buffpos+1)='\r';  
                buffpos+=2;  ++str;
                }
        else {  
                /* See if its a \ before a ~ , if so then we print colour command
                   as text */
                if (*str=='\\' && *(str+1)=='~') {  ++str;  continue;  }
                if (str!=start && *str=='~' && *(str-1)=='\\') {
                        *(buff+buffpos)=*str;  goto CONT;
                        }
                /* Process colour commands eg ~FR. We have to strip out the commands 
                   from the string even if user doesnt have colour switched on hence 
                   the user->colour check isnt done just yet */
                if (*str=='~') {
                        if (buffpos>OUT_BUFF_SIZE-6) {
                                write(sock,buff,buffpos);  buffpos=0;
                                }
                        ++str;
                        for(i=0;i<NUM_COLS;++i) {
                                if (!strncmp(str,colcom[i],2)) {
                                        if (user->colour && !user->login) {
                                                memcpy(buff+buffpos,colcode[i],strlen(colcode[i]));
                                                buffpos+=strlen(colcode[i])-1;  
                                                }
                                        else buffpos--;
                                        ++str;
                                        goto CONT;
                                        }
                                }
                        *(buff+buffpos)=*(--str);
                        }
                else *(buff+buffpos)=*str;
                CONT:
                ++buffpos;   ++str; 
                }
        if (buffpos==OUT_BUFF_SIZE) {
                write(sock,buff,OUT_BUFF_SIZE);  buffpos=0;
                }
        }
if (buffpos) write(sock,buff,buffpos);
/* Reset terminal at end of string */
if (user->colour) write_sock(sock,"\033[0m"); 
}



/*** Write to users of level 'level' and above or below depending on above
     variable; if 1 then above else below. . . Rarely used now.  This has
     been replaced by the write_duty() function in TalkerOS. ***/
write_level(level,above,str,user)
int level,above;
char *str;
UR_OBJECT user;
{
UR_OBJECT u;

for(u=user_first;u!=NULL;u=u->next) {
        if (u!=user && !u->login && u->type!=CLONE_TYPE) {
                if (user!=NULL && !strcmp(u->ignuser,user->name)) continue;
                switch(u->misc_op) {
                        case 3: /* writing on board */
                        case 4: /* Writing mail */
                        case 5: /* doing profile */
                        case 8: /* writing personal room */
                        case 9: /* main room description */
                        case 10: /* writing memo (mail to self) */
                        case 11: /* writing administration board */
                        case 12: /* writing suggestion board */
                                 continue;
                        default: break;
                        }
                if ((above && u->level>=level) || (!above && u->level<=level))
                        write_user(u,str);
                }
        }
}



/*** Subsid function to below but this one is used the most ***/
write_room(rm,str)
RM_OBJECT rm;
char *str;
{
write_room_except(rm,str,NULL);
}



/*** Write to everyone in room rm except for "user". If rm is NULL write 
     to all rooms. ***/
write_room_except(rm,str,user)
RM_OBJECT rm;
char *str;
UR_OBJECT user;
{
UR_OBJECT u;
char text2[ARR_SIZE];

for(u=user_first;u!=NULL;u=u->next) {
        if (u->login 
            || u->room==NULL 
            || (u->room!=rm && rm!=NULL) 
            || (u->ignall && !force_listen)
            || (u->ignshout && (com_num==SHOUT || com_num==SEMOTE))
            || u==user) continue;
        if (user!=NULL && !strcmp(u->ignuser,user->name)) continue;
        if (u->type==CLONE_TYPE) {
                if (u->clone_hear==CLONE_HEAR_NOTHING || u->owner->ignall) continue;
                /* Ignore anything not in clones room, eg shouts, system messages
                   and semotes since the clones owner will hear them anyway. */
                if (rm!=u->room) continue;
                if (u->clone_hear==CLONE_HEAR_SWEARS) {
                        if (!contains_swearing(str)) continue;
                        }
                if (!rm->personal) sprintf(text2,"%s[ %s ]:~RS %s",colors[CSYSTEM],u->room->name,str);
                else sprintf(text2,"%s[ %s ]:~RS %s",colors[CSYSTEM],u->room->owner,str);
                write_user(u->owner,text2);
                }
        else write_user(u,str); 
        }
}



/*** Write a string to system log ***/
write_syslog(str,write_time,log)
char *str,*log;
int write_time;
{
FILE *fp;

/* check to see if logging is set on or off */
if (!system_logging) return;

if (!(fp=fopen(log,"a"))) return;
if (!write_time) fputs(str,fp);
else fprintf(fp,"%02d/%02d %02d:%02d:%02d: %s",tmonth+1,tmday,thour,tmin,tsec,str);
fclose(fp);
}



/******** LOGIN/LOGOUT FUNCTIONS ********/

/*** Login function. Lots of nice inline code :) ***/
login(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
int i;
char name[ARR_SIZE],passwd[ARR_SIZE];

name[0]='\0';  passwd[0]='\0';
switch(user->login) {
        case 3:
        if (pueblo_enh && user->pueblo!=-1) {
                if (!strncmp(inpstr,"PUEBLOCLIENT",12)) {
                        user->pueblo = -1; /* Set to -1 so we don't keep repeating the welcome screen */
                        cls(user);
                        sprintf(text,"</xch_mudtext><center><img src=\"%s%s%s\"></center><xch_mudtext>",reg_sysinfo[TALKERHTTP],reg_sysinfo[PUEBLOWEB],reg_sysinfo[PUEBLOPIC]);
                        write_user(user,text);
                        sprintf(text,"</xch_mudtext><br><br><center><b><font size=+3>Welcome to %s!</font></b><br><font size=+1>TalkerOS version %s</font></center><br><xch_mudtext>\n",reg_sysinfo[TALKERNAME],TOS_VER);
                        write_user(user,text);
                        sprintf(text,"%s<>~RS %sMultimedia client has been Auto-detected.\n",colors[CWARNING],colors[CSYSBOLD]);
                        write_duty(ARCH,text,NULL,NULL,2);
                        write_user(user,"\nEnter your username: ");
                        user->login=3;
                        name[0]='\0';
                        return; 
                        }
                }
        sscanf(inpstr,"%s",name);
        if(name[0]<33) {
                write_user(user,"\nEnter your username: ");  return;
                }
        if (!strcmp(name,"quit")) {
                write_user(user,"\n\n*** Abandoning login attempt ***\n\n");
                sprintf(text,"%s<>~RS %sLogin Aborted! ~RS%s(%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],user->site,user->site_port);
                write_duty(ARCH,text,NULL,NULL,2);
                disconnect_user(user);  return;
                }
        if (!strcmp(name,"who")) { who(user,0); write_user(user,"\nEnter your username: ");  return; }
        if (!strcmp(name,"version")) {
                sprintf(text,"\nNUTS base-version: %6s\nTalkerOS code version: %s (%s)\n\n Enter your username: ",VERSION,TOS_VER,RUN_VER);
                /* sprintf(text,"\nNUTS base-version %s\nTalkerOS ver %s installed.\n\nEnter your username: ",VERSION,TOS_VER); */
                write_user(user,text);  return;
                }
        if (strlen(name)<3 || strlen(name)>USER_NAME_LEN) {
                sprintf(text,"\n-=- Username must be 3 to %d characters long. -=-\n\n",USER_NAME_LEN);
                write_user(user,text);
                attempts(user);  return;
                }
        /* see if only letters in login */
        for (i=0;i<strlen(name);++i) {
                if (!isalpha(name[i])) {
                        write_user(user,"\n-=- Please do not use any numbers, spaces, or punctuation. -=-\n\n");
                        attempts(user);  return;
                        }
                }
        if (!allow_caps_in_name) strtolower(name);
        name[0]=toupper(name[0]);
        if (user_banned(name)) {
                sprintf(text,"\n-=- You are banned from %s! -=-\n\n",reg_sysinfo[TALKERNAME]);
                write_user(user,text);
                sprintf(text,"%s<>~RS %sAttempted login by banned user!~RS%s [%s] (%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],name,user->site,user->site_port);
                write_duty(ARCH,text,NULL,NULL,2);
                disconnect_user(user);
                sprintf(text,"Attempted login by banned user %s.\n",name);
                write_syslog(text,1,BANLOG);
                return;
                }
        strcpy(user->name,name);
        /* After we store the name, convert the name to complete lowercase
           so that we can check if the name is "puebloclient", and if so,
           it needs to be revoked since it's not a good idea to let this
           be a username. */
        strtolower(name);
        if (!strcmp(name,"puebloclient")) {
                write_user(user,"\n-=- Invalid username.  Please try again. -=-\n\nEnter your username: ");
                return;
                }
        /* If user has hung on another login clear that session */
        for(u=user_first;u!=NULL;u=u->next) {
                if (u->login && u!=user && !strcmp(u->name,user->name)) {
                        disconnect_user(u);  break;
                        }
                }       
        if (!load_user_details(user)) {
                if (user->port==port[1]) {
                        sprintf(text,"\n-=- Please use port %d to create a new account. -=-\n\n",port[0]);
                        write_user(user,text);
                        sprintf(text,"%s<>~RS %sWizPort Access Denied. ~RS%s(%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],user->site,user->site_port);
                        write_duty(ARCH,text,NULL,NULL,2);
                        disconnect_user(user);
                        return;
                        }
                if (minlogin_level>-1 || nonewbies) {
                        write_user(user,"\n-=- Sorry, new accounts cannot be created at this time. -=-\n\n");
                        sprintf(text,"%s<>~RS %sNew User Denied. ~RS%s(%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],user->site,user->site_port);
                        write_duty(ARCH,text,NULL,NULL,2);
                        disconnect_user(user);  
                        return;
                        }
                if (newbie_site_banned(user->site)) {
                        sprintf(text,"New account creation denied for banned site: %s\n",user->site);
                        write_syslog(text,1,BANLOG);
                        sprintf(text,"%s<>~RS %sNew account denied. (Newbie-banned) ~RS%s(%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],user->site,user->site_port);
                        write_duty(ARCH,text,NULL,NULL,2);
                        write_user(user,"\n\r-=- New accounts from your site/domain are banned. -=-\n\n\r");
                        disconnect_user(user);
                        return;
                        }
                write_user(user,"-=- Creating NEW user... -=-\n");
                /* Set create time. */
                user->created=(int)(time(0));
                /* Check & delete any aliases that happen to be the new user's name. */
                for (u=user_first;u!=NULL;u=u->next) {
                        if (!strcmp(user->name,u->alias)) {
                                write_user(u,"ATTENTION:  New user logging on with your current alias.\n            Your alias is now being cleared... Sorry!\n");
                                u->alias[0]='\0';
                                }
                        }
                }
        else {
                if (user->port==port[1] && user->level<wizport_level) {
                        sprintf(text,"\n-=- Sorry, this port is for level %s and above only. -=-\n    Use port %d.\n\n",level_name[wizport_level],port[0]);
                        write_user(user,text);
                        sprintf(text,"%s<>~RS %sWizPort Access Denied. ~RS%s(%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],user->site,user->site_port);
                        write_duty(ARCH,text,NULL,NULL,2);
                        disconnect_user(user);  
                        return;
                        }
                if (user->level<minlogin_level) {
                        write_user(user,"\n-=- Sorry, your level is currently locked out. -=-\n\n");
                        sprintf(text,"%s<>~RS %sLogin Denied: Minlogin Mismatch ~RS%s(%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],user->site,user->site_port);
                        write_duty(ARCH,text,NULL,NULL,2);
                        disconnect_user(user);  
                        return;
                        }
                }
        write_user(user,"Enter your password: ");
        echo_off(user);
        user->login=2;
        return;

        case 2:
        sscanf(inpstr,"%s",passwd);
        if (strlen(passwd)<3) {
                write_user(user,"\n\n-=- Password too short. -=-\n\n");  
                attempts(user);  return;
                }
        if (strlen(passwd)>PASS_LEN) {
                write_user(user,"\n\n-=- Password too long. -=-\n\n");
                attempts(user);  return;
                }
        /* if new user... */
        if (!user->pass[0]) {
                strcpy(user->pass,(char *)crypt(passwd,"NU"));
                write_user(user,"\n   Confirm password: ");
                user->login=1;   
                }
        else {
                if (!strcmp(user->pass,(char *)crypt(passwd,"NU"))) {
                        echo_on(user);  connect_user(user);  return;
                        }
                write_user(user,"\n\n-=- Incorrect login! -=-\n\n");
                user->level=0; user->orig_level=0;
                attempts(user);
                }
        return;

        case 1:
        sscanf(inpstr,"%s",passwd);
        if (strcmp(user->pass,(char*)crypt(passwd,"NU"))) {
                write_user(user,"\n\n-=- Passwords do not match. -=-\n\n");
                attempts(user);
                return;
                }
        echo_on(user);
        strcpy(user->desc,newbiedesc);
        strcpy(user->in_phrase,"enters");       
        strcpy(user->out_phrase,"goes");        
        user->last_site[0]='\0';
        user->level=0;
        user->muzzled=0;
        user->command_mode=0;
        if (prompt_def!=2) user->prompt=prompt_def; else user->prompt=1;
                /* Determine whether we will use prompt on or off from now on.
                   If the default is anything other than YES, then set to
                   off.  If it is set to PROMPT, then alert the user on the
                   first login. */

        if (colour_def!=2) user->colour=colour_def; else user->colour=0;
                /* Determine whether we will use color on or off from now on.
                   If the default is anything other than YES, then set to
                   off.  If it is set to PROMPT, then alert the user on the
                   first login. */

        if (charecho_def!=2) user->charmode_echo=charecho_def; else user->charmode_echo=0;
                /* Determine whether we will use remote echo on or off from now on.
                   If the default is anything other than YES, then set to
                   off.  If it is set to PROMPT, then alert the user on the
                   first login. */

        user->orig_level=0;
        user->arrested=0;
        user->duty=0;
        user->waitfor[0]='\0';
        user->alert=0;
        user->age=-1;
        user->gender=0;
        user->pstats=1;
        user->cloaklev = user->level;
        user->orig_level = user->level;
        user->reverse = 0;
        user->pueblo_mm = pblo_usr_mm_def;
        user->pueblo_pg = pblo_usr_pg_def;
        if (saveaccts) save_user_details(user,1);
        sprintf(text,"New user ID created: '%s'\n",user->name);
        write_syslog(text,1,USERLOG);
        connect_user(user);
        if (prompt_def==2) { write_user(user,"SETUP:  This talker has a PROMPT that is posted after each line you type.\n        If you would like to deactivate it, type  .prompt  now.\n"); }
        if (colour_def==2) { write_user(user,"SETUP:  This talker supports ANSI color.  If you would like to use color,\n        please use the command:  .color   to activate it.\n"); }
        if (charecho_def==2) { write_user(user,"SETUP:  This talker can emulate local echo if you cannot see what you\n        type.  If you find that you can't see what you type, or if you find\n        that you see two of every character you type, please type: .charecho\n"); }
        }
}
        


/*** Count up attempts made by user to login ***/
attempts(user)
UR_OBJECT user;
{
user->attempts++;
if (user->attempts==3) {
        write_user(user,"\n-=- Maximum attempts reached!... Disconnecting. -=-\n\n");
        sprintf(text,"%s<>~RS %sMaximum Attempts Reached. ~RS%s(%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],user->site,user->site_port);
        write_duty(ARCH,text,NULL,NULL,2);
        disconnect_user(user);  return;
        }
user->login=3;
tos403_bugfix_01(user);                         // Login security problem.
                                                // See function for more info.
write_user(user,"Enter your username: ");
echo_on(user);
}



/*** Load the users details ***/
load_user_details(user)
UR_OBJECT user;
{
FILE *fp;
char line[81],filename[80];
int temp1,temp2,temp3,temp4;

sprintf(filename,"%s/%s.D",USERFILES,user->name);
if (!(fp=fopen(filename,"r"))) return 0;

fscanf(fp,"%s\n",user->pass); /* datafile version identification and/or pwd */

if (!strncmp(user->pass,"4.0",3)
        || !strncmp(user->pass,"401",3)
        || !strncmp(user->pass,"402",3)
        || !strncmp(user->pass,"403",3)) {      /* Userfile is in lastest format. */

                fscanf(fp,"%s\n",user->pass); /* NOW get the password */

                /* TIME DATA */
                fscanf(fp,"%d %d %d %d %d\n",&temp1,&temp2,&user->last_login_len,&temp3,&temp4);
                user->last_login=(time_t)temp1;
                user->total_login=(time_t)temp2;
                user->read_mail=(time_t)temp3;
                user->created=(time_t)temp4;

                /* USER PRIVLIDGES */
                fscanf(fp,"%d %d %d %d %d\n",&user->level,&user->muzzled,&user->arrested,&user->reverse,&user->can_edit_rooms);
                user->orig_level=user->level;
                user->cloaklev = user->orig_level;

                /* USER SESSION VARIABLES */
                fscanf(fp,"%d %d %d %d %d %d\n",&user->prompt,&user->charmode_echo,&user->command_mode,&user->colour,&user->duty,&user->vis);

                /* USER SETTINGS and STATS */
                fscanf(fp,"%d %d %d %d %d %d\n",&user->gender,&user->age,&user->pstats,&user->pueblo_mm,&user->pueblo_pg,&user->voiceprompt);

                /* last site */
                fscanf(fp,"%s\n",user->last_site);
        
                /* description */
                fgets(line,USER_DESC_LEN+2,fp);
                line[strlen(line)-1]=0;
                strcpy(user->desc,line); 

                /* enter prhase */
                fgets(line,PHRASE_LEN+2,fp);
                line[strlen(line)-1]=0;
                strcpy(user->in_phrase,line); 

                /* exit phrase */
                fgets(line,PHRASE_LEN+2,fp);
                line[strlen(line)-1]=0;
                strcpy(user->out_phrase,line); 

                /* personal room topic */
                fgets(line,TOPIC_LEN+2,fp);
                line[strlen(line)-1]=0;
                strcpy(user->room_topic,line); 

                /* email */
                fgets(line,58,fp);
                line[strlen(line)-1]=0;
                strcpy(user->email,line); 

                /* http address */
                fgets(line,58,fp);
                line[strlen(line)-1]=0;
                strcpy(user->http,line); 

        fclose(fp);
        /* If the datafile is not for a bot, we don't want anyone to be
        able to login at or above the bot level... This is for a few reasons,
        first: it could produce bugs if the bot gets killed... two: its a
        security risk since no one should ever be at or above the bot. */
        if (user->level >= BOTLEV && user->type!=BOT_TYPE) {
                while (user->level >= BOTLEV) user->level--;
                sprintf(text,"%s's datafile listed a level at or above BOTLEV.\nThe system has demoted %s down to: %s\n",user->name,objgen[user->gender],level_name[user->level]);
                write_syslog(text,0,USERLOG);
                user->orig_level=user->level;
                user->cloaklev=user->level;
                }
        return 1;
        }
else {
        /* backwards compatibility */
/*        fscanf(fp,"%s",user->pass);  Not needed anymore. */
        fscanf(fp,"%d %d %d %d %d %d %d %d %d %d",
                &temp1,&temp2,&user->last_login_len,&temp3,&user->level,&user->prompt,&user->muzzled,&user->charmode_echo,&user->command_mode,&user->colour);
        user->orig_level = user->level;
        user->cloaklev = user->orig_level;
        user->last_login=(time_t)temp1;
        user->total_login=(time_t)temp2;
        user->read_mail=(time_t)temp3;
        fscanf(fp,"%s\n",user->last_site);
        
        /* Need to do the rest like this 'cos they may be more than 1 word each */
        fgets(line,USER_DESC_LEN+2,fp);
        line[strlen(line)-1]=0;
        strcpy(user->desc,line); 
        fgets(line,PHRASE_LEN+2,fp);
        line[strlen(line)-1]=0;
        strcpy(user->in_phrase,line); 
        fgets(line,PHRASE_LEN+2,fp);
        line[strlen(line)-1]=0;
        strcpy(user->out_phrase,line); 
        fclose(fp);
        return 1;
        }
}


/*** Save a users stats ***/

/*      We always save the user file in the current format so that we
        don't have to bother with converting all the time.  In this way,
        files are upgraded after an old file has been imported by the
        load_user_details() function.  We don't worry about exporting.
        In other words, we can read backwards-compatible, but not write.  */

save_user_details(user,save_current)
UR_OBJECT user;
int save_current;
{
FILE *fp;
char filename[80];
if (user->orig_level==0 && !saveaccts) return 1;
if (user->type==REMOTE_TYPE || user->type==CLONE_TYPE) return 0;
sprintf(filename,"%s/%s.D",USERFILES,user->name);
if (!(fp=fopen(filename,"w"))) {
        sprintf(text,"%s: failed to save your details.\n",syserror);    
        write_user(user,text);
        sprintf(text,"SAVE_USER_STATS: Failed to save %s's details.\n",user->name);
        write_syslog(text,1,USERLOG);
        return 0;
        }
fprintf(fp,"%s\n",RUN_VER);     /* write internal runtime version number */
fprintf(fp,"%s\n",user->pass);  /* write password /*

                /* TIME DATA */
                if (save_current)
                        fprintf(fp,"%d %d %d %d %d\n",(int)time(0),(int)user->total_login,(int)(time(0)-user->last_login),(int)user->read_mail,(int)user->created);
                else    fprintf(fp,"%d %d %d %d %d\n",(int)user->last_login,(int)user->total_login,user->last_login_len,(int)user->read_mail,(int)user->created);

                /* USER PRIVLIDGES */
                fprintf(fp,"%d %d %d %d %d\n",user->orig_level,user->muzzled,user->arrested,user->reverse,user->can_edit_rooms);

                /* USER SESSION VARIABLES */
                fprintf(fp,"%d %d %d %d %d %d\n",user->prompt,user->charmode_echo,user->command_mode,user->colour,user->duty,user->vis);

                /* USER SETTINGS and STATS */
                fprintf(fp,"%d %d %d %d %d %d\n",user->gender,user->age,user->pstats,user->pueblo_mm,user->pueblo_pg,user->voiceprompt);

                /* last site */
                if (save_current) fprintf(fp,"%s\n",user->site);
                else              fprintf(fp,"%s\n",user->last_site);
        
                /* description */
                fprintf(fp,"%s\n",user->desc);

                /* enter phrase */
                fprintf(fp,"%s\n",user->in_phrase);

                /* exit phrase */
                fprintf(fp,"%s\n",user->out_phrase);

                /* personal room topic */
                fprintf(fp,"%s\n",user->room_topic);

                /* email */
                fprintf(fp,"%s\n",user->email);

                /* http address */
                fprintf(fp,"%s\n",user->http);

fclose(fp);
return 1;
}


/*** Connect the user to the talker proper ***/
connect_user(user)
UR_OBJECT user;
{
UR_OBJECT u,u2;
RM_OBJECT rm;
char temp[30];
char loginvis[2][2]={"*"," "};

/* See if user already connected */
for(u=user_first;u!=NULL;u=u->next) {
        if (user!=u && user->type!=CLONE_TYPE && !strcmp(user->name,u->name)) {
                rm=u->room;
                if (u->type==REMOTE_TYPE) {
                        sprintf(text,"\n%sYou are pulled back through cyberspace...\n",colors[CBOLD]);
                        write_user(u,text);
                        sprintf(text,"REMVD %s\n",u->name);
                        write_sock(u->netlink->socket,text);
                        sprintf(text,"%s vanishes.\n",u->name);
                        destruct_user(u);
                        write_room(rm,text);
                        reset_access(rm);
                        num_of_users--;
                        break;
                        }
                if (u->type==BOT_TYPE) close(user->socket);  /* Don't want ppl logging in as the bot. */
                write_user(user,"\n\nYou are already connected - switching to old session...\n");
                sprintf(text,"%s swapped sessions.\n",user->name);
                write_syslog(text,1,USERLOG);
                close(u->socket);
                u->socket=user->socket;
                strcpy(u->site,user->site);
                u->site_port=user->site_port;
                destruct_user(user);
                num_of_logins--;

                /* Reset pueblo status */
                if (u->pueblo!=-1) {    /* if -1 then already been detected. */
                        u->pueblo=0;             /* Default to pueblo-incompatible */
                        u->pblodetect=1;        /* Enable pueblo to be re-detected */
                        }
                sprintf(text,"%sRE-CONNECT:~RS %s %s\n",colors[CSYSBOLD],u->name,u->desc,colors[CSYSBOLD]);
                write_room_except(NULL,text,u);
                if (rm==NULL) {
                        sprintf(text,"ACT %s look\n",u->name);
                        write_sock(u->netlink->socket,text);
                        }
                else {
                        look(u);  prompt(u);
                        }
                /* Reset the sockets on any clones */
                for(u2=user_first;u2!=NULL;u2=u2->next) {
                        if (u2->type==CLONE_TYPE && u2->owner==user) {
                                u2->socket=u->socket;  u->owner=u;
                                }
                        }
                chk_firewall(u);
                return;
                }
        }
/* Announce users logon. You're probably wondering why Ive done it this strange
   way , well its to avoid a minor bug that arose when as in 3.3.1 it created 
   the string in 2 parts and sent the parts seperately over a netlink. If you 
   want more details email me. */       
sprintf(text,"%s~FG~OLSIGN ON:~RS%s %s %s~RS (%s)\n",loginvis[user->vis],colors[CWHOUSER],user->name,user->desc,long_date(2));
if (user->level>WIZ && !user->vis) write_duty(user->level,text,NULL,user,0);
else {
        write_level(USER,1,text,NULL);
        user->vis=1;
        }
sprintf(text,"%s          (%s:%d) [ %s ]\n",colors[CSYSTEM],user->site,user->site_port,level_name[user->level]);
write_duty(user->level,text,NULL,NULL,2);

/* send post-login message and other logon stuff to user */
user->login=0;
write_user(user,"\n");
more(user,user->socket,MOTD2); 

/* TalkerOS System Information */
sprintf(text,"~OL~FY%s is using TalkerOS v%s - S/N:%s (%s)\n",reg_sysinfo[TALKERNAME],TOS_VER,reg_sysinfo[SERIALNUM],reg_sysinfo[REGUSER]);
write_user(user,text);
sprintf(text,"Email: %s - Website: %s\n\n",reg_sysinfo[TALKERMAIL],reg_sysinfo[TALKERHTTP]);
write_user(user,text);

if (user->last_site[0]) {
        sprintf(temp,"%s",ctime(&user->last_login));
        temp[strlen(temp)-1]=0;
        sprintf(text,"Welcome %s %s...\n\n%s%s%sYou were last logged in on %s from %s.\n\n",level_name[user->level],user->name,colors[CHIGHLIGHT],colors[CBOLD],colors[CTEXT],temp,user->last_site);
        }
else sprintf(text,"Welcome %s...\n\n",user->name);
write_user(user,text);
user->room=room_first;
user->last_login=time(0); /* set to now */
if (user->arrested) {
        sprintf(text,"%s%s%s is re-captured and tossed back in jail!\n",colors[CBOLD],colors[CSYSTEM],user->name);
        write_room_except(room_first,text,user);
        for(rm=room_first;rm!=NULL;rm=rm->next)
                if (rm->link[0]==NULL) break;
        user->room=rm;
        write_room_except(user->room,text,user);
        }

/* Perform any login procedures. */
do_login_event(user);
}


/*** Disconnect user from talker ***/
disconnect_user(user)
UR_OBJECT user;
{
RM_OBJECT rm;
NL_OBJECT nl;
int hr,min;
char plural[2][2]={"s",""};

rm=user->room;
if (user->login) {
        close(user->socket);  
        destruct_user(user);
        num_of_logins--;  
        return;
        }
/* Get total connected time. */
min=0; hr=0;
min=(int)(time(0) - user->last_login)/60;
while (min>=60) {
        min=min-60;
        hr++;
        }

/* There was a major bug in the netlink code.  Whenever someone .quit or
lost their connection while at a remote talker, both talkers would crash.
I am hoping this will remedy the situation. */
if (user->room==NULL && user->type!=REMOTE_TYPE) { home(user);  rm=user->room; }

if (user->type!=REMOTE_TYPE) {
        if (user->level!=user->orig_level) user->level=user->orig_level;
        if (user->level < toscom_level[DUTY]) user->duty=0;
        save_user_details(user,1);
        save_plugin_data(user);
        sprintf(text,"%s logged out.\n",user->name);
        write_syslog(text,1,USERLOG);
        sprintf(text,"\nGoodbye. - Thank you for visiting %s, and please come back soon!\n%s%sYou were logged in for %d hour%s and %d minute%s.\n\n",reg_sysinfo[TALKERNAME],colors[CBOLD],colors[CSYSBOLD],hr,plural[(hr==1)],min,plural[(min==1)]);
        write_user(user,text);
        close(user->socket);
        sprintf(text,"~OL~FRSIGN OFF:~RS%s %s %s~RS (%s)\n",colors[CWHOUSER],user->name,user->desc,long_date(2));
        if (user->level>WIZ && !user->vis) write_duty(user->level,text,NULL,user,0);
        else {
                write_level(NEW,1,text,NULL);
                user->vis=1;
                }
        sprintf(text,"%s          (%s:%d) [ %s ]\n",colors[CSYSTEM],user->site,user->site_port,level_name[user->level]);
        write_duty(user->level,text,NULL,NULL,2);
        }
else {
        save_plugin_data(user);
        sprintf(text,"\n%s%s~OLYou are pulled back in disgrace to your own domain...\n",colors[CBOLD],colors[CWARNING]);
        write_user(user,text);
        sprintf(text,"REMVD %s\n",user->name);
        write_sock(user->netlink->socket,text);
        sprintf(text,"%s~OL%s is banished from here!\n",colors[CSYSTEM],user->name);
        write_room_except(rm,text,user);
        sprintf(text,"NETLINK: Remote user %s removed.\n",user->name);
        write_syslog(text,1,USERLOG);
        }
reset_ignuser(user);
if (user->malloc_start!=NULL) free(user->malloc_start);
num_of_users--;

/* Destroy any clones */
destroy_user_clones(user);
destruct_user(user);
reset_access(rm);
destructed=0;
}


/*** Tell telnet not to echo characters - for password entry ***/
echo_off(user)
UR_OBJECT user;
{
char seq[4];

if (password_echo) return;
sprintf(seq,"%c%c%c",255,251,1);
write_user(user,seq);
}


/*** Tell telnet to echo characters ***/
echo_on(user)
UR_OBJECT user;
{
char seq[4];

if (password_echo) return;
sprintf(seq,"%c%c%c",255,252,1);
write_user(user,seq);
}



/************ MISCELLANIOUS FUNCTIONS *************/

/*** Stuff that is neither speech nor a command is dealt with here ***/
misc_ops(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char plural[2][2]={"s",""};
switch(user->misc_op) {
        case 1: 
        if (toupper(inpstr[0])=='Y') {
                if (rs_countdown && !rs_which) {
                        if (rs_countdown>60) 
                                sprintf(text,"\n\07~OLSYSTEM: ~FR~LISHUTDOWN INITIATED, shutdown in %d minute%s, %d second%s!\n\n",rs_countdown/60,plural[((rs_countdown/60)==1)],rs_countdown%60,plural[((rs_countdown%60)==1)]);
                        else sprintf(text,"\n\07~OLSYSTEM: ~FR~LISHUTDOWN INITIATED, shutdown in %d seconds!\n\n",rs_countdown);
                        write_room(NULL,text);
                        audioprompt(NULL,6,0);  /* Audio warning */
                        sprintf(text,"%s initiated a %d second SHUTDOWN countdown.\n",user->name,rs_countdown);
                        write_syslog(text,1,SYSLOG);
                        write_syslog(text,1,USERLOG);
                        rs_user=user;
                        rs_announce=time(0);
                        user->misc_op=0;  
                        prompt(user);
                        return 1;
                        }
                talker_shutdown(user,NULL,0); 
                }
        /* This will reset any reboot countdown that was started, oh well */
        rs_countdown=0;
        rs_announce=0;
        rs_which=-1;
        rs_user=NULL;
        user->misc_op=0;  
        prompt(user);
        return 1;

        case 2: 
        if (toupper(inpstr[0])=='E'
            || more(user,user->socket,user->page_file)!=1) {
                user->misc_op=0;  user->filepos=0;  user->page_file[0]='\0';
                prompt(user); 
                }
        return 1;

        case 3: /* writing on board */
        case 4: /* Writing mail */
        case 5: /* doing profile */
        case 8: /* writing personal room */
        case 9: /* main room description */
        case 10: /* writing memo (mail to self) */
        case 11: /* writing administration board */
        case 12: /* writing suggestion board */
        editor(user,inpstr);  return 1;

        case 6:
        if (toupper(inpstr[0])=='Y') delete_user(user,1); 
        else {  user->misc_op=0;  prompt(user);  }
        return 1;

        case 7:
        if (toupper(inpstr[0])=='Y') {
                if (rs_countdown && rs_which==1) {
                        if (rs_countdown>60) 
                                sprintf(text,"\n\07~OLSYSTEM: ~FY~LIREBOOT INITIATED, rebooting in %d minute%s, %d second%s!\n\n",rs_countdown/60,plural[((rs_countdown/60)==1)],rs_countdown%60,plural[((rs_countdown%60)==1)]);
                        else sprintf(text,"\n\07~OLSYSTEM: ~FY~LIREBOOT INITIATED, rebooting in %d seconds!\n\n",rs_countdown);
                        write_room(NULL,text);
                        audioprompt(NULL,6,0);  /* Audio warning */
                        sprintf(text,"%s initiated a %d second REBOOT countdown.\n",user->name,rs_countdown);
                        write_syslog(text,1,SYSLOG);
                        write_syslog(text,1,USERLOG);
                        rs_user=user;
                        rs_announce=time(0);
                        user->misc_op=0;  
                        prompt(user);
                        return 1;
                        }
                talker_shutdown(user,NULL,1); 
                }
        if (rs_which==1 && rs_countdown && rs_user==NULL) {
                rs_countdown=0;
                rs_announce=0;
                rs_which=-1;
                }
        user->misc_op=0;  
        prompt(user);
        return 1;
        }
return 0;
}


/*** The editor used for writing profiles, mail and messages on the boards ***/
editor(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int cnt,line;
char *edprompt="\n~FW[P]review~RS, ~FG[S]ave~RS, ~FY[R]edo~RS or ~FR[A]bort~RS : ";
char *ptr,*name;

if (user->vis) name=user->name;  else name=invisname;
if (user->edit_op) {
        switch(toupper(*inpstr)) {
                case 'S':
                if (user->misc_op!=8 && user->misc_op!=9) sprintf(text,"%s finishes composing some text.\n",name);
                else sprintf(text,"%s finishes writing a room description.\n",name);
                write_room_except(user->room,text,user);
                switch(user->misc_op) {
                        case 3: write_board(user,NULL,1,0);  break;
                        case 4: smail(user,NULL,1,0);  break;
                        case 5: enter_profile(user,1);  break;
                        case 8: rmdesc(user,1);  break;
                        case 9: editroom(user,1); break;
                        case 10: smail(user,NULL,1,1); break;
                        case 11: write_board(user,NULL,1,1);  break;
                        case 12: write_board(user,NULL,1,2);  break;
                        }
                editor_done(user);
                return;

                case 'R':
                user->edit_op=0;
                user->edit_line=1;
                user->charcnt=0;
                user->malloc_end=user->malloc_start;
                *user->malloc_start='\0';
                if (user->misc_op!=8 && user->misc_op!=9) sprintf(text,"\nRedo message...\n  |-------------------------- Editor Margin Width ---------------------------|\n\n%d>",user->edit_line);
                else sprintf(text,"\nRedo room description...\n  |-------------------------- Editor Margin Width ---------------------------|\n\n%d>",user->edit_line);
                write_user(user,text);
                return;

                case 'A':
                if (user->misc_op!=8 && user->misc_op!=9) write_user(user,"\nMessage aborted.\n");
                        else write_user(user,"\nRoom description aborted.\n");
                if (user->misc_op!=8 && user->misc_op!=9) sprintf(text,"%s gives up composing some text.\n",name);
                else sprintf(text,"%s gives up writing a room description.\n",name);
                write_room_except(user->room,text,user);
                editor_done(user);  
                return;

                case 'P':
                write_user(user,"\nFinal Edit Preview:\n\n");
                write_user(user,user->malloc_start);
                write_user(user,edprompt);
                return;

                default:
                write_user(user,edprompt);
                return;
                }
        }
/* Allocate memory if user has just started editing */
if (user->malloc_start==NULL) {
        if (user->misc_op!=8 && user->misc_op!=9) {
                if ((user->malloc_start=(char *)malloc(MAX_LINES*81))==NULL) {
                        sprintf(text,"%s: failed to allocate buffer memory.\n",syserror);
                        write_user(user,text);
                        write_syslog("ERROR: Failed to allocate memory in editor().\n",0,SYSLOG);
                        user->misc_op=0;
                        prompt(user);
                        return;
                        }
                }
        else {
                if ((user->malloc_start=(char *)malloc(10*81))==NULL) {
                        sprintf(text,"%s: failed to allocate buffer memory.\n",syserror);
                        write_user(user,text);
                        write_syslog("ERROR: Failed to allocate memory in editor().\n",0,SYSLOG);
                        user->misc_op=0;
                        prompt(user);
                        return;
                        }
                }
        user->ignall_store=user->ignall;
        user->ignall=1; /* Dont want chat mucking up the edit screen */
        user->edit_line=1;
        user->charcnt=0;
        user->malloc_end=user->malloc_start;
        *user->malloc_start='\0';
        if (user->misc_op!=8 && user->misc_op!=9) sprintf(text,"~FTMaximum of %d lines, end with a '.' on a line by itself.\n  |-------------------------- Editor Margin Width ---------------------------|\n\n1>",MAX_LINES);
        else sprintf(text,"~FTMaximum of 10 lines, end with a '.' on a line by itself.\n  |-------------------------- Editor Margin Width ---------------------------|\n\n1>");
        write_user(user,text);
        if (user->misc_op!=8 && user->misc_op!=9) sprintf(text,"%s starts composing some text...\n",name);
        else sprintf(text,"%s starts writing a room description...\n",name);
        write_room_except(user->room,text,user);
        return;
        }
/* Check for empty line */
if (!word_count) {
        if (!user->charcnt) {
                sprintf(text,"%d>",user->edit_line);
                write_user(user,text);
                return;
                }
        *user->malloc_end++='\n';
        if (user->misc_op!=8 && user->misc_op!=9 && user->edit_line==MAX_LINES) goto END;
        if ((user->misc_op==8 || user->misc_op==9) && user->edit_line==10) goto END;
        sprintf(text,"%d>",++user->edit_line);
        write_user(user,text);
        user->charcnt=0;
        return;
        }
/* If nothing carried over and a dot is entered then end */
if (!user->charcnt && !strcmp(inpstr,".")) goto END;

line=user->edit_line;
cnt=user->charcnt;

/* loop through input and store in allocated memory */
while(*inpstr) {
        *user->malloc_end++=*inpstr++;
        if (++cnt==80) {  user->edit_line++;  cnt=0;  }
        if (user->misc_op!=8 && user->misc_op!=9
            && (user->edit_line>MAX_LINES || (user->malloc_end - user->malloc_start)>=(MAX_LINES*81)))
                 goto END;
        if ((user->misc_op==8 || user->misc_op==9)
            && (user->edit_line>10 || (user->malloc_end - user->malloc_start)>=810))
                 goto END;
        }
if (line!=user->edit_line) {
        ptr=(char *)(user->malloc_end-cnt);
        *user->malloc_end='\0';
        sprintf(text,"%d>%s",user->edit_line,ptr);
        write_user(user,text);
        user->charcnt=cnt;
        return;
        }
else {
        *user->malloc_end++='\n';
        user->charcnt=0;
        }
if (user->misc_op!=8 && user->misc_op!=9 && user->edit_line!=MAX_LINES) {
        sprintf(text,"%d>",++user->edit_line);
        write_user(user,text);
        return;
        }
if ((user->misc_op==8 || user->misc_op==9) && user->edit_line!=10) {
        sprintf(text,"%d>",++user->edit_line);
        write_user(user,text);
        return;
        }

/* User has finished his message , prompt for what to do now */
END:
*user->malloc_end='\0';
if (*user->malloc_start) {
        write_user(user,edprompt);
        user->edit_op=1;  return;
        }
write_user(user,"\nNo text entered.  Editing aborted!\n");
if (user->misc_op!=8 && user->misc_op!=9) sprintf(text,"%s gives up composing some text.\n",name);
else sprintf(text,"%s gives up writing a room description.\n",name);
write_room_except(user->room,text,user);
editor_done(user);
}


/*** Reset some values at the end of editing ***/
editor_done(user)
UR_OBJECT user;
{
char plural[2][2]={"s",""};
user->misc_op=0;
user->edit_op=0;
user->edit_line=0;
free(user->malloc_start);
user->malloc_start=NULL;
user->malloc_end=NULL;
user->ignall=user->ignall_store;
if (user->primsg) {
        sprintf(text,"--> You have had %d private message%s while editing.  See ~FG.revtell\n",user->primsg,plural[(user->primsg==1)]);
        write_user(user,text);
        sprintf(text,"    ( Only the last %d messages are available. %d scrolled away. )\n",REVTELL_LINES,(user->primsg - REVTELL_LINES));
        if (user->primsg > REVTELL_LINES) write_user(user,text);
        }
prompt(user);
}


/*** Record speech and emotes in the room. ***/
record(rm,str)
RM_OBJECT rm;
char *str;
{
strncpy(rm->revbuff[rm->revline],str,REVIEW_LEN);
rm->revbuff[rm->revline][REVIEW_LEN]='\n';
rm->revbuff[rm->revline][REVIEW_LEN+1]='\0';
rm->revline=(rm->revline+1)%REVIEW_LINES;
}


/*** Records tells and remotes sent to the user. ***/
record_tell(user,str)
UR_OBJECT user;
char *str;
{
strncpy(user->revbuff[user->revline],str,REVIEW_LEN);
user->revbuff[user->revline][REVIEW_LEN]='\n';
user->revbuff[user->revline][REVIEW_LEN+1]='\0';
user->revline=(user->revline+1)%REVTELL_LINES;
}



/*** Set room access back to public if not enough users in room ***/
reset_access(rm)
RM_OBJECT rm;
{
UR_OBJECT u;
int cnt;

if (rm==NULL) return;
if (rm->personal) {
        cnt=0;
        for(u=user_first;u!=NULL;u=u->next) if (u->room==rm) ++cnt;
        if (cnt) return;
        rm->owner[0]='\0';
        rm->topic[0]='\0';
        clear_revbuff(rm); 
        return;
        }
if (rm->access!=PRIVATE) return;
cnt=0;
for(u=user_first;u!=NULL;u=u->next) if (u->room==rm) ++cnt;
if (cnt<min_private_users) {
        write_room(rm,"Room access returned to ~FGPUBLIC.\n");
        rm->access=PUBLIC;

        /* Reset any invites into the room & clear review buffer */
        for(u=user_first;u!=NULL;u=u->next) {
                if (u->invite_room==rm) u->invite_room=NULL;
                }
        clear_revbuff(rm);
        }
}



/*** Exit because of error during bootup ***/
boot_exit(code)
int code;
{
switch(code) {
        case 1:
        write_syslog("BOOT FAILURE: Error while parsing configuration file.\n",0,SYSLOG);
        exit(1);

        case 2:
        perror("NUTS: Can't open main port listen socket");
        write_syslog("BOOT FAILURE: Can't open main port listen socket.\n",0,SYSLOG);
        exit(2);

        case 3:
        perror("NUTS: Can't open wiz port listen socket");
        write_syslog("BOOT FAILURE: Can't open wiz port listen socket.\n",0,SYSLOG);
        exit(3);

        case 4:
        perror("NUTS: Can't open link port listen socket");
        write_syslog("BOOT FAILURE: Can't open link port listen socket.\n",0,SYSLOG);
        exit(4);

        case 5:
        perror("NUTS: Can't bind to main port");
        write_syslog("BOOT FAILURE: Can't bind to main port.\n",0,SYSLOG);
        exit(5);

        case 6:
        perror("NUTS: Can't bind to wiz port");
        write_syslog("BOOT FAILURE: Can't bind to wiz port.\n",0,SYSLOG);
        exit(6);

        case 7:
        perror("NUTS: Can't bind to link port");
        write_syslog("BOOT FAILURE: Can't bind to link port.\n",0,SYSLOG);
        exit(7);
        
        case 8:
        perror("NUTS: Listen error on main port");
        write_syslog("BOOT FAILURE: Listen error on main port.\n",0,SYSLOG);
        exit(8);

        case 9:
        perror("NUTS: Listen error on wiz port");
        write_syslog("BOOT FAILURE: Listen error on wiz port.\n",0,SYSLOG);
        exit(9);

        case 10:
        perror("NUTS: Listen error on link port");
        write_syslog("BOOT FAILURE: Listen error on link port.\n",0,SYSLOG);
        exit(10);

        case 11:
        perror("NUTS: Failed to fork");
        write_syslog("BOOT FAILURE: Failed to fork.\n",0,SYSLOG);
        exit(11);
        }
}



/*** User prompt ***/
prompt(user)
UR_OBJECT user;
{
int hr,min;

if (no_prompt) return;
if (user->type==REMOTE_TYPE) {
        sprintf(text,"PRM %s\n",user->name);
        write_sock(user->netlink->socket,text);  
        return;
        }
if (user->command_mode && !user->misc_op) {  
        if (!user->vis) write_user(user,"~FTCOM+> ");
        else write_user(user,"~FTCOM> ");  
        return;  
        }
if (!user->prompt || user->misc_op) return;
hr=(int)(time(0)-user->last_login)/3600;
min=((int)(time(0)-user->last_login)%3600)/60;
if (!user->vis)
        sprintf(text,"~FT<%02d:%02d, %02d:%02d, %s+>\n",thour,tmin,hr,min,user->name);
else sprintf(text,"~FT<%02d:%02d, %02d:%02d, %s>\n",thour,tmin,hr,min,user->name);
write_user(user,text);
}



/*** Page a file out to user. Colour commands in files will only work if 
     user!=NULL since if NULL we dont know if his terminal can support colour 
     or not. Return values: 
                0 = cannot find file, 1 = found file, 2 = found and finished ***/
more(user,sock,filename)
UR_OBJECT user;
int sock;
char *filename;
{
int i,buffpos,num_chars,lines,retval,len;
char buff[OUT_BUFF_SIZE],text2[83],*str,*colour_com_strip();
FILE *fp;

if (!(fp=fopen(filename,"r"))) {
        if (user!=NULL) user->filepos=0;  
        return 0;
        }
/* jump to reading posn in file */
if (user!=NULL) fseek(fp,user->filepos,0);

text[0]='\0';  
buffpos=0;  
num_chars=0;
retval=1; 
len=0;

/* If user is remote then only do 1 line at a time */
if (sock==-1) {
        lines=1;  fgets(text2,82,fp);
        }
else {
        lines=0;  fgets(text,sizeof(text)-1,fp);
        }

/* Go through file */
while(!feof(fp) && (lines<23 || user==NULL)) {
        if (sock==-1) {
                lines++;  
                if (user->netlink->ver_major<=3 && user->netlink->ver_minor<2) 
                        str=colour_com_strip(text2);
                else str=text2;
                if (str[strlen(str)-1]!='\n') 
                        sprintf(text,"MSG %s\n%s\nEMSG\n",user->name,str);
                else sprintf(text,"MSG %s\n%sEMSG\n",user->name,str);
                write_sock(user->netlink->socket,text);
                num_chars+=strlen(str);
                fgets(text2,82,fp);
                continue;
                }
        str=text;

        /* Process line from file */
        while(*str) {
                if (*str=='\n') {
                        if (buffpos>OUT_BUFF_SIZE-6) {
                                write(sock,buff,buffpos);  buffpos=0;
                                }
                        /* Reset terminal before every newline */
                        if (user!=NULL && user->colour) {
                                memcpy(buff+buffpos,"\033[0m",4);  buffpos+=4;
                                }
                        *(buff+buffpos)='\n';  *(buff+buffpos+1)='\r';  
                        buffpos+=2;  ++str;
                        }
                else {  
                        /* Process colour commands in the file. See write_user()
                           function for full comments on this code. */
                        if (*str=='\\' && *(str+1)=='~' && !contains_pueblo(str)) {  ++str;  continue;  }
                        if (str!=text && *str=='~' && *(str-1)=='\\' && !contains_pueblo(str)) {
                                *(buff+buffpos)=*str;  goto CONT;
                                }
                        if (*str=='~') {
                                if (buffpos>OUT_BUFF_SIZE-6) {
                                        write(sock,buff,buffpos);  buffpos=0;
                                        }
                                ++str;
                                for(i=0;i<NUM_COLS;++i) {
                                        if (!strncmp(str,colcom[i],2)) {
                                                if (user!=NULL && user->colour) {
                                                        memcpy(buffpos+buff,colcode[i],strlen(colcode[i]));
                                                        buffpos+=strlen(colcode[i])-1;
                                                        }
                                                else buffpos--;
                                                ++str;
                                                goto CONT;
                                                }
                                        }
                                *(buff+buffpos)=*(--str);
                                }
                        else *(buff+buffpos)=*str;
                        CONT:
                        ++buffpos;   ++str;
                        }
                if (buffpos==OUT_BUFF_SIZE) {
                        write(sock,buff,OUT_BUFF_SIZE);  buffpos=0;
                        }
                }
        len=strlen(text);
        num_chars+=len;
        lines+=len/80+(len<80);
        fgets(text,sizeof(text)-1,fp);
        }
if (buffpos && sock!=-1) write(sock,buff,buffpos);

/* if user is logging on dont page file */
if (user==NULL) {  fclose(fp);  return 2;  };
if (feof(fp)) {
        user->filepos=0;  no_prompt=0;  retval=2;
        }
else  {
        /* store file position and file name */
        user->filepos+=num_chars;
        strcpy(user->page_file,filename);
        /* We use E here instead of Q because when on a remote system and
           in COMMAND mode the Q will be intercepted by the home system and 
           quit the user */
        sprintf(text,"           %s%s%s-=- Press <return> to continue, 'e'<return> to exit -=-",colors[CHIGHLIGHT],colors[CTEXT],colors[CBOLD]);
        write_user(user,text);
        if (user->pueblo) write_user(user,"\n");
        no_prompt=1;
        }
fclose(fp);
return retval;
}



/*** Set global vars. hours,minutes,seconds,date,day,month,year ***/
set_date_time()
{
struct tm *tm_struct; /* structure is defined in time.h */
time_t tm_num;

/* Set up the structure */
time(&tm_num);
tm_struct=localtime(&tm_num);

/* Get the values */
tday=tm_struct->tm_yday;
tyear=1900+tm_struct->tm_year; /* Will this work past the year 2000? Hmm... */
tmonth=tm_struct->tm_mon;
tmday=tm_struct->tm_mday;
twday=tm_struct->tm_wday;
thour=tm_struct->tm_hour;
tmin=tm_struct->tm_min;
tsec=tm_struct->tm_sec; 
}



/*** Return pos. of second word in inpstr ***/
char *remove_first(inpstr)
char *inpstr;
{
char *pos=inpstr;
while(*pos<33 && *pos) ++pos;
while(*pos>32) ++pos;
while(*pos<33 && *pos) ++pos;
return pos;
}


/*** Get user struct pointer from name ***/
UR_OBJECT get_user(name)
char *name;
{
UR_OBJECT u;

name[0]=toupper(name[0]);
/* Search for exact name */
for(u=user_first;u!=NULL;u=u->next) {
        if (u->login || u->type==CLONE_TYPE) continue;
        if (!strcmp(u->name,name))  return u;
        }
/* Search for close match name */
for(u=user_first;u!=NULL;u=u->next) {
        if (u->login || u->type==CLONE_TYPE) continue;
        if (strstr(u->name,name))  return u;
        }
/* Search for exact alias */
for(u=user_first;u!=NULL;u=u->next) {
        if (u->login || u->type==CLONE_TYPE || u->alias[0]=='\0') continue;
        if (!strcmp(u->alias,name))  return u;
        }
/* Search for close match alias */
for(u=user_first;u!=NULL;u=u->next) {
        if (u->login || u->type==CLONE_TYPE || u->alias[0]=='\0') continue;
        if (strstr(u->alias,name))  return u;
        }
return NULL;
}


/*** Get room struct pointer from abbreviated name ***/
RM_OBJECT get_room(name)
char *name;
{
RM_OBJECT rm;

for(rm=room_first;rm!=NULL;rm=rm->next)
     if (!strncmp(rm->name,name,strlen(name))) return rm;
     else if (!strncmp(rm->owner,name,strlen(name))) return rm;
return NULL;
}


/*** Return level value based on level name
     (altered to accept abbreviations and to be case-insensitive) ***/
get_level(getname)
char *getname;
{
int i,pos;
char levname[20],name[20];

strcpy(name,getname);   strtolower(name);
i=0;
while(level_name[i][0]!='*') {
        strcpy(levname,level_name[i]);
        strtolower(levname);
        /* if (!strcmp(level_name[i],getname)) return i; */
        if (!strncmp(levname,name,strlen(name))) return i;
        ++i;
        }
return -1;
}


/*** See if a user has access to a room. If room is fixed to private then
        it is considered a wizroom so grant permission to any user of WIZ and
        above for those.  Personal rooms however pay no attention to the
        user's level or the gatecrash level.  You must be the owner or
        you must have an .invite                                        ***/
has_room_access(user,rm)
UR_OBJECT user;
RM_OBJECT rm;
{
if (rm->personal && !strcmp(rm->owner,user->name)) return 1;
if (user->level>=gatecrash_level && rm->personal && user->invite_room!=rm) return 0;
if ((rm->access & PRIVATE)
    && (user->level<gatecrash_level)
    && user->invite_room!=rm
    && !((rm->access & FIXED) && user->level>=WIZ)) return 0;
return 1;
}


/*** See if user has unread mail, mail file has last read time on its 
     first line ***/
has_unread_mail(user)
UR_OBJECT user;
{
FILE *fp;
int tm;
char filename[80];

sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(fp=fopen(filename,"r"))) return 0;
fscanf(fp,"%d",&tm);
fclose(fp);
if (tm>(int)user->read_mail) return 1;
return 0;
}


/*** This is function that sends mail to other users ***/
send_mail(user,to,ptr)
UR_OBJECT user;
char *to,*ptr;
{
NL_OBJECT nl;
UR_OBJECT u;
FILE *infp,*outfp;
char *c,d,*service,filename[80],line[DNL+1];

/* See if remote mail */
c=to;  service=NULL;
while(*c) {
        if (*c=='@') {  
                service=c+1;  *c='\0'; 
                for(nl=nl_first;nl!=NULL;nl=nl->next) {
                        if (!strcmp(nl->service,service) && nl->stage==UP) {
                                send_external_mail(nl,user,to,ptr);
                                return;
                                }
                        }
                sprintf(text,"Service %s is unavailable.\n",service);
                write_user(user,text); 
                return;
                }
        ++c;
        }

/* Local mail */
if (!(outfp=fopen("tempfile","w"))) {
        write_user(user,"Error in mail delivery.\n");
        write_syslog("ERROR: Couldn't open tempfile in send_mail().\n",0,SYSLOG);
        return;
        }
/* Write current time on first line of tempfile */
fprintf(outfp,"%d\r",(int)time(0));

/* Copy current mail file into tempfile if it exists */
sprintf(filename,"%s/%s.M",USERFILES,to);
if (infp=fopen(filename,"r")) {
        /* Discard first line of mail file. */
        fgets(line,DNL,infp);

        /* Copy rest of file */
        d=getc(infp);  
        while(!feof(infp)) {  putc(d,outfp);  d=getc(infp);  }
        fclose(infp);
        }

/* Put new mail in tempfile */
if (user!=NULL) {
        if (user->type==REMOTE_TYPE)
                fprintf(outfp,"%sFrom: %s@%s  ~RS%s%s\n",colors[CMAILHEAD],user->name,user->netlink->service,colors[CMAILDATE],long_date(0));
        else if (user==get_user(to))
                fprintf(outfp,"%s(MEMO)              ~RS%s%s\n",colors[CMAILHEAD],colors[CMAILDATE],long_date(0));
        else fprintf(outfp,"%sFrom: %-12s  ~RS%s%s\n",colors[CMAILHEAD],user->name,colors[CMAILDATE],long_date(0));
        }                
else fprintf(outfp,"%sFrom: TalkerOS v%s SYSTEM  ~RS%s%s\n",colors[CMAILHEAD],TOS_VER,colors[CMAILDATE],long_date(0));

fputs(ptr,outfp);
fputs("\n",outfp);
fclose(outfp);
rename("tempfile",filename);
if (user==get_user(to)) { write_user(user,"~OL~FT~LI-=- Memo Saved -=-\n"); return; }
sprintf(text,"-=- Mail has been SENT to: %s -=-\n",to);
write_user(user,text);
write_user(get_user(to),"\07~FT~OL~LI-=- YOU HAVE NEW MAIL -=-\n");
u=get_user(to);
if (u!=NULL) {
        if (u->type==BOT_TYPE) {
                sprintf(text,"%s[ The bot has new mail! ]\n",colors[CSYSTEM]);
                write_duty(WIZ,text,NULL,user);
                }
        }       
}


/*** Spool mail file and ask for confirmation of users existence on remote
        site ***/
send_external_mail(nl,user,to,ptr)
NL_OBJECT nl;
UR_OBJECT user;
char *to,*ptr;
{
FILE *fp;
char filename[80];

/* Write out to spool file first */
sprintf(filename,"%s/OUT_%s_%s@%s",MAILSPOOL,user->name,to,nl->service);
if (!(fp=fopen(filename,"a"))) {
        sprintf(text,"%s: unable to spool mail.\n",syserror);
        write_user(user,text);
        sprintf(text,"ERROR: Couldn't open file %s to append in send_external_mail().\n",filename);
        write_syslog(text,0,SYSLOG);
        return;
        }
putc('\n',fp);
fputs(ptr,fp);
fclose(fp);

/* Ask for verification of users existence */
sprintf(text,"EXISTS? %s %s\n",to,user->name);
write_sock(nl->socket,text);

/* Rest of delivery process now up to netlink functions */
write_user(user,"Mail sent.\n");
}


/*** See if string contains any swearing ***/
contains_swearing(str)
char *str;
{
char *s;
int i;

if ((s=(char *)malloc(strlen(str)+1))==NULL) {
        write_syslog("ERROR: Failed to allocate memory in contains_swearing().\n",0,SYSLOG);
        return 0;
        }
strcpy(s,str);
strtolower(s); 
i=0;
while(swear_words[i][0]!='*') {
        if (strstr(s,swear_words[i])) {  free(s);  return 1;  }
        ++i;
        }
/* check to see if it contains a fake pueblo command from another user. */
if (strstr(s,"</xch_mudtext>")) { free(s); return 1; }
if (strstr(s,"this world is pueblo") && strstr(s,"enhanced")) { free(s); return 1; }
free(s);
return 0;
}

contains_pueblo(str)
char *str;
{
char *s;

if ((s=(char *)malloc(strlen(str)+1))==NULL) {
        write_syslog("ERROR: Failed to allocate memory in contains_pueblo().\n",0,SYSLOG);
        return 0;
        }
strcpy(s,str);
strtolower(s); 
/* check to see if it contains a pueblo flag or a http:// so we don't
   convert "/~username/file.ext" into thinking its a color command. */
if (strstr(s,"</xch_mudtext>")) { free(s); return 1; }
if (strstr(s,"http://")) { free(s); return 1; }
free(s);
return 0;
}

contains_extension(str,type)
char *str;
int type;
{
char *s;
int ok;
if ((s=(char *)malloc(strlen(str)+1))==NULL) {
        write_syslog("ERROR: Failed to allocate memory in contains_extention().\n",0,SYSLOG);
        return 0;
        }
strcpy(s,str);
strtolower(s); 
ok=0;
if (type==0) {  /* Images  (.gif/.jpg) */
        if (strstr(s,".gif")) ok=1;
        if (strstr(s,".jpg")) ok=1;
        if (strstr(s,".jpeg")) ok=1;
        if (ok) { free(s); return 1; }
        }
if (type==1) {  /* Audio   (.wav/.mid) */
        if (strstr(s,".wav")) ok=1;
        if (strstr(s,".mid")) ok=1;
        if (strstr(s,".midi")) ok=1;
        if (ok) { free(s); return 1; }
        }
free(s);
return 0;
}


/*** Count the number of colour commands in a string ***/
colour_com_count(str)
char *str;
{
char *s;
int i,cnt;

s=str;  cnt=0;
while(*s) {
     if (*s=='~') {
          ++s;
          for(i=0;i<NUM_COLS;++i) {
               if (!strncmp(s,colcom[i],2)) {
                    cnt++;  s++;  continue;
                    }
               }
          continue;
          }
     ++s;
     }
return cnt;
}


/*** Strip out colour commands from string for when we are sending strings
     over a netlink to a talker that doesn't support them ***/
char *colour_com_strip(str)
char *str;
{
char *s,*t;
static char text2[ARR_SIZE];
int i;

s=str;  t=text2;
while(*s) {
        if (*s=='~') {
                ++s;
                for(i=0;i<NUM_COLS;++i) {
                        if (!strncmp(s,colcom[i],2)) {  s++;  goto CONT;  }
                        }
                --s;  *t++=*s;
                }
        else *t++=*s;
        CONT:
        s++;
        }       
*t='\0';
return text2;
}


/*** Date string for board messages, mail, .who and .allclones ***/
char *long_date(which)
int which;
{
static char dstr[80];
int hr,min,ap;
char AmPm[2][2]={"a","p"};

/* Do friendly 12 hour clock for users so they don't have to convert */
hr = thour;  min = tmin;
if (hr >= 12) { if (hr > 12) hr = hr - 12;  /* only remove 12 hrs if > 12 */
                ap = 1; }         /* Remove the extra 12 hrs. and set to PM. */
         else { ap = 0;  if (hr == 0) hr = 12; } /* Set to AM and set to 12 if midnight. */
if (which) {
        if (which==1) sprintf(dstr,"on %s, %s %d, %d at %2d:%02d%s",day[twday],month[tmonth],tmday,tyear,hr,min,AmPm[ap]);
        if (which==2) sprintf(dstr,"%s %d, %d - %2d:%02d%s",month[tmonth],tmday,tyear,hr,min,AmPm[ap]);
        }
else sprintf(dstr,"[ %s, %s %d, %d at %2d:%02d%s ]",day[twday],month[tmonth],tmday,tyear,hr,min,AmPm[ap]);
return dstr;
}


/*** Clear the review buffer in the room ***/
clear_revbuff(rm)
RM_OBJECT rm;
{
int c;
for(c=0;c<REVIEW_LINES;++c) rm->revbuff[c][0]='\0';
rm->revline=0;
}


/*** Clear the screen ***/
cls(user)
UR_OBJECT user;
{
int i;

for(i=0;i<5;++i) write_user(user,"\n\n\n\n\n\n\n\n\n\n");               
}


/*** Convert string to upper case ***/
strtoupper(str)
char *str;
{
while(*str) {  *str=toupper(*str);  str++; }
}


/*** Convert string to lower case ***/
strtolower(str)
char *str;
{
while(*str) {  *str=tolower(*str);  str++; }
}


/*** Returns 1 if string is a positive number ***/
isnumber(str)
char *str;
{
while(*str) if (!isdigit(*str++)) return 0;
return 1;
}


/************ OBJECT FUNCTIONS ************/

/*** Construct user/clone object ***/
UR_OBJECT create_user()
{
UR_OBJECT user;
int i;

if ((user=(UR_OBJECT)malloc(sizeof(struct user_struct)))==NULL) {
        write_syslog("ERROR: Memory allocation failure in create_user().\n",0,SYSLOG);
        return NULL;
        }

/* Append object into linked list. */
if (user_first==NULL) {  
        user_first=user;  user->prev=NULL;  
        }
else {  
        user_last->next=user;  user->prev=user_last;  
        }
user->next=NULL;
user_last=user;

/* initialise user structure */
user->type=USER_TYPE;
user->name[0]='\0';
user->desc[0]='\0';
user->in_phrase[0]='\0'; 
user->out_phrase[0]='\0';   
user->afk_mesg[0]='\0';
user->pass[0]='\0';
user->site[0]='\0';
user->site_port=0;
user->last_site[0]='\0';
user->page_file[0]='\0';
user->mail_to[0]='\0';
user->inpstr_old[0]='\0';
user->buff[0]='\0';  
user->buffpos=0;
user->filepos=0;
user->read_mail=time(0);
user->room=NULL;
user->invite_room=NULL;
user->port=0;
user->login=0;
user->socket=-1;
user->attempts=0;
user->command_mode=0;
user->level=0;
user->vis=1;
user->ignall=0;
user->ignall_store=0;
user->ignshout=0;
user->igntell=0;
user->muzzled=0;
user->remote_com=-1;
user->netlink=NULL;
user->pot_netlink=NULL; 
user->last_input=time(0);
user->last_login=time(0);
user->last_login_len=0;
user->total_login=0;
user->prompt=prompt_def;
user->colour=colour_def;
user->charmode_echo=charecho_def;
user->misc_op=0;
user->edit_op=0;
user->edit_line=0;
user->charcnt=0;
user->warned=0;
user->accreq=0;
user->afk=0;
user->revline=0;
user->clone_hear=CLONE_HEAR_ALL;
user->malloc_start=NULL;
user->malloc_end=NULL;
user->owner=NULL;

user->reverse=0;
user->arrested=0;
user->duty=0;
user->room_topic[0]='\0';
user->orig_level=0;
user->gender=0;
user->age=-1;
user->alert=0;
user->email[0]='\0';
user->http[0]='\0';
user->pueblo=0;
user->pueblo_mm=0;
user->pueblo_pg=0;
user->voiceprompt=0;
user->pblodetect=1;
user->pstats=1;
user->cloaklev=0;
user->can_edit_rooms=0;
user->readrules=0;
user->alias[0]='\0';
user->waitfor[0]='\0';
user->primsg=0;
user->idle=0;
user->ignuser[0]='\0';
user->md5[0]='\0';
for(i=0;i<REVTELL_LINES;++i) user->revbuff[i][0]='\0';
return user;
}



/*** Destruct an object. ***/
destruct_user(user)
UR_OBJECT user;
{
/* Remove from linked list */
if (user==user_first) {
        user_first=user->next;
        if (user==user_last) user_last=NULL;
        else user_first->prev=NULL;
        }
else {
        user->prev->next=user->next;
        if (user==user_last) { 
                user_last=user->prev;  user_last->next=NULL; 
                }
        else user->next->prev=user->prev;
        }
free(user);
destructed=1;
}


/*** Construct room object ***/
RM_OBJECT create_room()
{
RM_OBJECT room;
int i;

if ((room=(RM_OBJECT)malloc(sizeof(struct room_struct)))==NULL) {
        fprintf(stderr,"NUTS: Memory allocation failure in create_room().\n");
        boot_exit(1);
        }
room->name[0]='\0';
room->label[0]='\0';
room->desc[0]='\0';
room->topic[0]='\0';
room->access=-1;
room->revline=0;
room->mesg_cnt=0;
room->inlink=0;
room->netlink=NULL;
room->netlink_name[0]='\0';
room->next=NULL;
room->owner[0]='\0';
room->personal=0;
for(i=0;i<MAX_LINKS;++i) {
        room->link_label[i][0]='\0';  room->link[i]=NULL;
        }
for(i=0;i<REVIEW_LINES;++i) room->revbuff[i][0]='\0';
if (room_first==NULL) room_first=room;
else room_last->next=room;
room_last=room;
return room;
}


/*** Construct link object ***/
NL_OBJECT create_netlink()
{
NL_OBJECT nl;

if ((nl=(NL_OBJECT)malloc(sizeof(struct netlink_struct)))==NULL) {
        sprintf(text,"NETLINK: Memory allocation failure in create_netlink().\n");
        write_syslog(text,1,SYSLOG);
        return NULL;
        }
if (nl_first==NULL) { 
        nl_first=nl;  nl->prev=NULL;  nl->next=NULL;
        }
else {  
        nl_last->next=nl;  nl->next=NULL;  nl->prev=nl_last;
        }
nl_last=nl;

nl->service[0]='\0';
nl->site[0]='\0';
nl->verification[0]='\0';
nl->mail_to[0]='\0';
nl->mail_from[0]='\0';
nl->mailfile=NULL;
nl->buffer[0]='\0';
nl->ver_major=0;
nl->ver_minor=0;
nl->ver_patch=0;
nl->keepalive_cnt=0;
nl->last_recvd=0;
nl->port=0;
nl->socket=0;
nl->mesg_user=NULL;
nl->connect_room=NULL;
nl->type=UNCONNECTED;
nl->stage=DOWN;
nl->connected=0;
nl->lastcom=-1;
nl->allow=ALL;
nl->warned=0;
nl->TOSserver=0;
return nl;
}


/*** Destruct a netlink (usually a closed incoming one). ***/
destruct_netlink(nl)
NL_OBJECT nl;
{
if (nl!=nl_first) {
        nl->prev->next=nl->next;
        if (nl!=nl_last) nl->next->prev=nl->prev;
        else { nl_last=nl->prev; nl_last->next=NULL; }
        }
else {
        nl_first=nl->next;
        if (nl!=nl_last) nl_first->prev=NULL;
        else nl_last=NULL; 
        }
free(nl);
}


/*** Destroy all clones belonging to given user ***/
destroy_user_clones(user)
UR_OBJECT user;
{
UR_OBJECT u;

for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE && u->owner==user) {
                sprintf(text,"The clone of %s shimmers and vanishes.\n",u->name);
                write_room(u->room,text);
                destruct_user(u);
                }
        }
}


/************ NUTS PROTOCOL AND LINK MANAGEMENT FUNCTIONS ************/
/* Please don't alter these functions. If you do you may introduce 
   incompatabilities which may prevent other systems connecting or cause
   bugs on the remote site and yours. You may think it looks simple but
   even the newline count is important in some places. */

/*** Accept incoming server connection ***/
accept_server_connection(sock,acc_addr)
int sock;
struct sockaddr_in acc_addr;
{
NL_OBJECT nl,nl2,create_netlink();
RM_OBJECT rm;
char site[81];

/* Send server type id and version number */
sprintf(text,"NUTS %s\n",VERSION);
write_sock(sock,text);
strcpy(site,get_ip_address(acc_addr));
sprintf(text,"NETLINK: Received request connection from site %s.\n",site);
write_syslog(text,1,SYSLOG);

/* See if legal site, ie site is in config sites list. */
for(nl2=nl_first;nl2!=NULL;nl2=nl2->next) 
        if (!strcmp(nl2->site,site)) goto OK;
write_sock(sock,"DENIED CONNECT 1\n");
close(sock);
write_syslog("NETLINK: Request denied, remote site not in valid sites list.\n",1,SYSLOG);
return;

/* Find free room link */
OK:
for(rm=room_first;rm!=NULL;rm=rm->next) {
        if (rm->netlink==NULL && rm->inlink) {
                if ((nl=create_netlink())==NULL) {
                        write_sock(sock,"DENIED CONNECT 2\n");  
                        close(sock);  
                        write_syslog("NETLINK: Request denied, unable to create netlink object.\n",1,SYSLOG);
                        return;
                        }
                rm->netlink=nl;
                nl->socket=sock;
                nl->type=INCOMING;
                nl->stage=VERIFYING;
                nl->connect_room=rm;
                nl->allow=nl2->allow;
                nl->last_recvd=time(0);
                strcpy(nl->service,"<verifying>");
                strcpy(nl->site,site);
                write_sock(sock,"GRANTED CONNECT\n");
                write_syslog("NETLINK: Request granted.\n",1,SYSLOG);
                return;
                }
        }
write_sock(sock,"DENIED CONNECT 3\n");
close(sock);
write_syslog("NETLINK: Request denied, no free room links.\n",1,SYSLOG);
}
                

/*** Deal with netlink data on link nl ***/
exec_netcom(nl,inpstr)
NL_OBJECT nl;
char *inpstr;
{
int netcom_num,lev;
char w1[ARR_SIZE],w2[ARR_SIZE],w3[ARR_SIZE],*c,ctemp;

/* The most used commands have been truncated to save bandwidth, ie ACT is
   short for action, EMSG is short for end message. Commands that don't get
   used much ie VERIFICATION have been left long for readability. */
char *netcom[]={
"DISCONNECT","TRANS","REL","ACT","GRANTED",
"DENIED","MSG","EMSG","PRM","VERIFICATION",
"VERIFY","REMVD","ERROR","EXISTS?","EXISTS_NO",
"EXISTS_YES","MAIL","ENDMAIL","MAILERROR","KA",
"RSTAT","*"
};

/* The buffer is large (ARR_SIZE*2) but if a bug occurs with a remote system
   and no newlines are sent for some reason it may overflow and this will 
   probably cause a crash. Oh well, such is life. */
if (nl->buffer[0]) {
        strcat(nl->buffer,inpstr);  inpstr=nl->buffer;
        }
nl->last_recvd=time(0);

/* Go through data */
while(*inpstr) {
        w1[0]='\0';  w2[0]='\0';  w3[0]='\0';  lev=0;
        if (*inpstr!='\n') sscanf(inpstr,"%s %s %s %d",w1,w2,w3,&lev);
        /* Find first newline */
        c=inpstr;  ctemp=1; /* hopefully we'll never get char 1 in the string */
        while(*c) {
                if (*c=='\n') {  ctemp=*(c+1); *(c+1)='\0';  break; }
                ++c;
                }
        /* If no newline then input is incomplete, store and return */
        if (ctemp==1) {  
                if (inpstr!=nl->buffer) strcpy(nl->buffer,inpstr);  
                return;  
                }
        /* Get command number */
        netcom_num=0;
        while(netcom[netcom_num][0]!='*') {
                if (!strcmp(netcom[netcom_num],w1))  break;
                netcom_num++;
                }
        /* Deal with initial connects */
        if (nl->stage==VERIFYING) {
                if (nl->type==OUTGOING) {
                        if (strcmp(w1,"NUTS")) {
                                sprintf(text,"NETLINK: Incorrect connect message from %s.\n",nl->service);
                                write_syslog(text,1,SYSLOG);
                                shutdown_netlink(nl);
                                return;
                                }       
                        /* Store remote version for compat checks */
                        nl->stage=UP;
                        w2[10]='\0'; 
                        sscanf(w2,"%d.%d.%d",&nl->ver_major,&nl->ver_minor,&nl->ver_patch);
                        goto NEXT_LINE;
                        }
                else {
                        /* Incoming */
                        if (netcom_num!=9) {
                                /* No verification, no connection */
                                sprintf(text,"NETLINK: No verification sent by site %s.\n",nl->site);
                                write_syslog(text,1,SYSLOG);
                                shutdown_netlink(nl);  
                                return;
                                }
                        nl->stage=UP;
                        }
                }
        /* If remote is currently sending a message relay it to user, don't
           interpret it unless its EMSG or ERROR */
        if (nl->mesg_user!=NULL && netcom_num!=7 && netcom_num!=12) {
                /* If -1 then user logged off before end of mesg received */
                if (nl->mesg_user!=(UR_OBJECT)-1) write_user(nl->mesg_user,inpstr);   
                goto NEXT_LINE;
                }
        /* Same goes for mail except its ENDMAIL or ERROR */
        if (nl->mailfile!=NULL && netcom_num!=17) {
                fputs(inpstr,nl->mailfile);  goto NEXT_LINE;
                }
        nl->lastcom=netcom_num;
        switch(netcom_num) {
                case  0: 
                if (nl->stage==UP) {
                        sprintf(text,"~OLSYSTEM:~FY~RS Disconnecting from service %s in the %s.\n",nl->service,nl->connect_room->name);
                        write_room(NULL,text);
                        }
                shutdown_netlink(nl);  
                break;

                case  1: nl_transfer(nl,w2,w3,lev,inpstr);  break;
                case  2: nl_release(nl,w2);  break;
                case  3: nl_action(nl,w2,inpstr);  break;
                case  4: nl_granted(nl,w2);  break;
                case  5: nl_denied(nl,w2,inpstr);  break;
                case  6: nl_mesg(nl,w2); break;
                case  7: nl->mesg_user=NULL;  break;
                case  8: nl_prompt(nl,w2);  break;
                case  9: nl_verification(nl,w2,w3,0);  break;
                case 10: nl_verification(nl,w2,w3,1);  break;
                case 11: nl_removed(nl,w2);  break;
                case 12: nl_error(nl);  break;
                case 13: nl_checkexist(nl,w2,w3);  break;
                case 14: nl_user_notexist(nl,w2,w3);  break;
                case 15: nl_user_exist(nl,w2,w3);  break;
                case 16: nl_mail(nl,w2,w3);  break;
                case 17: nl_endmail(nl);  break;
                case 18: nl_mailerror(nl,w2,w3);  break;
                case 19: /* Keepalive signal, do nothing */ break;
                case 20: nl_rstat(nl,w2);  break;
                default: 
                        sprintf(text,"NETLINK: Received unknown command '%s' from %s.\n",w1,nl->service);
                        write_syslog(text,1,SYSLOG);
                        write_sock(nl->socket,"ERROR\n"); 
                }
        NEXT_LINE:
        /* See if link has closed */
        if (nl->type==UNCONNECTED) return;
        *(c+1)=ctemp;
        inpstr=c+1;
        }
nl->buffer[0]='\0';
}


/*** Deal with user being transfered over from remote site ***/
nl_transfer(nl,name,pass,lev,inpstr)
NL_OBJECT nl;
char *name,*pass,*inpstr;
int lev;
{
UR_OBJECT u,create_user();

/* link for outgoing users only */
if (nl->allow==OUT) {
        sprintf(text,"DENIED %s 4\n",name);
        write_sock(nl->socket,text);
        return;
        }
if (strlen(name)>USER_NAME_LEN) name[USER_NAME_LEN]='\0';

/* See if user is banned */
if (user_banned(name)) {
        if (nl->ver_major==3 && nl->ver_minor>=3 && nl->ver_patch>=3) 
                sprintf(text,"DENIED %s 9\n",name); /* new error for 3.3.3 */
        else sprintf(text,"DENIED %s 6\n",name); /* old error to old versions */
        write_sock(nl->socket,text);
        return;
        }

/* See if user is already on here */
if (u=get_user(name)) {
        sprintf(text,"DENIED %s 5\n",name);
        write_sock(nl->socket,text);
        return;
        }

/* See if user of this name exists on this system by trying to load up
   datafile */
if ((u=create_user())==NULL) {          
        sprintf(text,"DENIED %s 6\n",name);
        write_sock(nl->socket,text);
        return;
        }
u->type=REMOTE_TYPE;
strcpy(u->name,name);
if (load_user_details(u)) {
        if (strcmp(u->pass,pass)) {
                /* Incorrect password sent */
                sprintf(text,"DENIED %s 7\n",name);
                write_sock(nl->socket,text);
                destruct_user(u);
                destructed=0;
                return;
                }
        }
else {
        /* Get the users description */
        if (nl->ver_major<=3 && nl->ver_minor<=3 && nl->ver_patch<1) 
                strcpy(text,remove_first(remove_first(remove_first(inpstr))));
        else strcpy(text,remove_first(remove_first(remove_first(remove_first(inpstr)))));
        text[USER_DESC_LEN]='\0';
        terminate(text);
        strcpy(u->desc,text);
        strcpy(u->in_phrase,"enters");
        strcpy(u->out_phrase,"goes");
        if (nl->ver_major==3 && nl->ver_minor>=3 && nl->ver_patch>=1) {
                if (lev>rem_user_maxlevel) u->level=rem_user_maxlevel;
                else u->level=lev; 
                }
        else u->level=rem_user_deflevel;
        }
/* See if users level is below minlogin level */
if (u->level<minlogin_level) {
        if (nl->ver_major==3 && nl->ver_minor>=3 && nl->ver_patch>=3) 
                sprintf(text,"DENIED %s 8\n",u->name); /* new error for 3.3.3 */
        else sprintf(text,"DENIED %s 6\n",u->name); /* old error to old versions */
        write_sock(nl->socket,text);
        destruct_user(u);
        destructed=0;
        return;
        }
strcpy(u->site,nl->service);
sprintf(text,"NETLINK: Remote user %s received from %s.\n",u->name,nl->service);
write_syslog(text,1,USERLOG);
u->room=nl->connect_room;
u->netlink=nl;
u->read_mail=time(0);
u->last_login=time(0);
u->orig_level=u->level;
u->cloaklev=u->level;
num_of_users++;
sprintf(text,"GRANTED %s\n",name);
write_sock(nl->socket,text);
sprintf(text,"~OL~FGLINK:~RS  %s enters into the %s from %s\n",u->name,u->room->name,nl->service);
write_room_except(NULL,text,u);
}
                

/*** User is leaving this system ***/
nl_release(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;

if ((u=get_user(name))!=NULL && u->type==REMOTE_TYPE) {
        save_plugin_data(u);
        sprintf(text,"~OL~FRLINK:~RS  %s leaves %s to go back to %s\n",u->name,reg_sysinfo[TALKERNAME],nl->service);
        write_room_except(NULL,text,u);
        sprintf(text,"NETLINK: Remote user %s released.\n",u->name);
        write_syslog(text,1,USERLOG);
        destroy_user_clones(u);
        destruct_user(u);
        num_of_users--;
        return;
        }
sprintf(text,"NETLINK: Release requested for unknown/invalid user %s from %s.\n",name,nl->service);
write_syslog(text,1,USERLOG);
}


/*** Remote user performs an action on this system ***/
nl_action(nl,name,inpstr)
NL_OBJECT nl;
char *name,*inpstr;
{
UR_OBJECT u;
char *c,ctemp;

if (!(u=get_user(name))) {
        sprintf(text,"DENIED %s 8\n",name);
        write_sock(nl->socket,text);
        return;
        }
if (u->socket!=-1) {
        sprintf(text,"NETLINK: Action requested for local user %s from %s.\n",name,nl->service);
        write_syslog(text,1,USERLOG);
        return;
        }
inpstr=remove_first(remove_first(inpstr));
/* remove newline character */
c=inpstr; ctemp='\0';
while(*c) {
        if (*c=='\n') {  ctemp=*c;  *c='\0';  break;  }
        ++c;
        }
u->last_input=time(0);
if (u->misc_op) {
        if (!strcmp(inpstr,"NL")) misc_ops(u,"\n");  
        else misc_ops(u,inpstr+4);
        return;
        }
if (u->afk) {
        write_user(u,"You are no longer AFK.\n");  
        sprintf(text,"%s comes back from being AFK.\n",u->name);
        if (u->vis) write_room_except(u->room,text,u);
                else write_duty(u->level,text,u->room,u,0);
        u->afk=0;
        }
word_count=wordfind(inpstr);
if (!strcmp(inpstr,"NL")) return; 
exec_com(u,inpstr);
if (ctemp) *c=ctemp;
if (!u->misc_op) prompt(u);
}


/*** Grant received from remote system ***/
nl_granted(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;
RM_OBJECT old_room;

if (!strcmp(name,"CONNECT")) {
        sprintf(text,"NETLINK: Connection to %s granted.\n",nl->service);
        write_syslog(text,1,SYSLOG);
        /* Send our verification and version number */
        sprintf(text,"VERIFICATION %s %s\n",verification,VERSION);
        write_sock(nl->socket,text);
        return;
        }
if (!(u=get_user(name))) {
        sprintf(text,"NETLINK: Grant received for unknown user %s from %s.\n",name,nl->service);
        write_syslog(text,1,USERLOG);
        return;
        }
/* This will probably occur if a user tried to go to the other site , got 
   lagged then changed his mind and went elsewhere. Don't worry about it. */
if (u->remote_com!=GO) {
        sprintf(text,"NETLINK: Unexpected grant for %s received from %s.\n",name,nl->service);
        write_syslog(text,1,SYSLOG);
        return;
        }
/* User has been granted permission to move into remote talker */
sprintf(text,"%s%sYou traverse cyberspace...\n",colors[CBOLD],colors[CSELF]);
write_user(u,text);
if (u->vis) {
        sprintf(text,"%s%s %s to the %s.\n",colors[CWHOUSER],u->name,u->out_phrase,nl->service);
        write_room_except(u->room,text,u);
        }
else write_room_except(u->room,invisleave,u);
sprintf(text,"NETLINK: %s transfered to %s.\n",u->name,nl->service);
write_syslog(text,1,USERLOG);
old_room=u->room;
u->room=NULL; /* Means on remote talker */
u->netlink=nl;
u->pot_netlink=NULL;
u->remote_com=-1;
u->misc_op=0;  
u->filepos=0;  
u->page_file[0]='\0';
reset_access(old_room);
sprintf(text,"ACT %s look\n",u->name);
write_sock(nl->socket,text);
}


/*** Deny received from remote system ***/
nl_denied(nl,name,inpstr)
NL_OBJECT nl;
char *name,*inpstr;
{
UR_OBJECT u,create_user();
int errnum;
char *neterr[]={
"this site is not in the remote services valid sites list",
"the remote service is unable to create a link",
"the remote service has no free room links",
"the link is for incoming users only",
"a user with your name is already logged on the remote site",
"the remote service was unable to create a session for you",
"incorrect password. Use '.go <service> <remote password>'",
"your level there is below the remote services current minlogin level",
"you are banned from that service"
};

errnum=0;
sscanf(remove_first(remove_first(inpstr)),"%d",&errnum);
if (!strcmp(name,"CONNECT")) {
        sprintf(text,"NETLINK: Connection to %s denied, %s.\n",nl->service,neterr[errnum-1]);
        write_syslog(text,1,SYSLOG);
        /* If wiz initiated connect let them know its failed */
        sprintf(text,"%s[SYSTEM:  Connection to %s failed, %s.]\n",colors[CSYSTEM],nl->service,neterr[errnum-1]);
        write_duty(com_level[CONN],text,NULL,NULL,0);
        close(nl->socket);
        nl->type=UNCONNECTED;
        nl->stage=DOWN;
        return;
        }
/* Is for a user */
if (!(u=get_user(name))) {
        sprintf(text,"NETLINK: Deny for unknown user %s received from %s.\n",name,nl->service);
        write_syslog(text,1,USERLOG);
        return;
        }
sprintf(text,"NETLINK: Deny %d for user %s received from %s.\n",errnum,name,nl->service);
write_syslog(text,1,USERLOG);
sprintf(text,"Sorry, %s.\n",neterr[errnum-1]);
write_user(u,text);
prompt(u);
u->remote_com=-1;
u->pot_netlink=NULL;
}


/*** Text received to display to a user on here ***/
nl_mesg(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;

if (!(u=get_user(name))) {
        sprintf(text,"NETLINK: Message received for unknown user %s from %s.\n",name,nl->service);
        write_syslog(text,1,USERLOG);
        nl->mesg_user=(UR_OBJECT)-1;
        return;
        }
nl->mesg_user=u;
}


/*** Remote system asking for prompt to be displayed ***/
nl_prompt(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;

if (!(u=get_user(name))) {
        sprintf(text,"NETLINK: Prompt received for unknown user %s from %s.\n",name,nl->service);
        write_syslog(text,1,USERLOG);
        return;
        }
if (u->type==REMOTE_TYPE) {
        sprintf(text,"NETLINK: Prompt received for remote user %s from %s.\n",name,nl->service);
        write_syslog(text,1,USERLOG);
        return;
        }
prompt(u);
}


/*** Verification received from remote site ***/
nl_verification(nl,w2,w3,com)
NL_OBJECT nl;
char *w2,*w3;
int com;
{
NL_OBJECT nl2;

if (!com) {
        /* We're verifiying a remote site */
        if (!w2[0]) {
                shutdown_netlink(nl);  return;
                }
        for(nl2=nl_first;nl2!=NULL;nl2=nl2->next) {
                if (!strcmp(nl->site,nl2->site) && !strcmp(w2,nl2->verification)) {
                        switch(nl->allow) {
                                case IN : write_sock(nl->socket,"VERIFY OK IN\n");  break;
                                case OUT: write_sock(nl->socket,"VERIFY OK OUT\n");  break;
                                case ALL: write_sock(nl->socket,"VERIFY OK ALL\n"); 
                                }
                        strcpy(nl->service,nl2->service);

                        /* Only 3.2.0 and above send version number with verification */
                        sscanf(w3,"%d.%d.%d",&nl->ver_major,&nl->ver_minor,&nl->ver_patch);
                        sprintf(text,"NETLINK: Connected to %s in the %s.\n",nl->service,nl->connect_room->name);
                        write_syslog(text,1,SYSLOG);
                        sprintf(text,"~OLSYSTEM:~RS New connection to service %s in the %s.\n",nl->service,nl->connect_room->name);
                        write_room(NULL,text);
                        return;
                        }
                }
        write_sock(nl->socket,"VERIFY BAD\n");
        shutdown_netlink(nl);
        return;
        }
/* The remote site has verified us */
if (!strcmp(w2,"OK")) {
        /* Set link permissions */
        if (!strcmp(w3,"OUT")) {
                if (nl->allow==OUT) {
                        sprintf(text,"NETLINK: WARNING - Permissions deadlock, both sides are outgoing only.\n");
                        write_syslog(text,1,SYSLOG);
                        }
                else nl->allow=IN;
                }
        else {
                if (!strcmp(w3,"IN")) {
                        if (nl->allow==IN) {
                                sprintf(text,"NETLINK: WARNING - Permissions deadlock, both sides are incoming only.\n");
                                write_syslog(text,1,SYSLOG);
                                }
                        else nl->allow=OUT;
                        }
                }
        sprintf(text,"NETLINK: Connection to %s verified.\n",nl->service);
        write_syslog(text,1,SYSLOG);
        sprintf(text,"~OLSYSTEM:~RS New connection to service %s in the %s.\n",nl->service,nl->connect_room);
        write_room(NULL,text);
        return;
        }
if (!strcmp(w2,"BAD")) {
        sprintf(text,"NETLINK: Connection to %s has bad verification.\n",nl->service);
        write_syslog(text,1,SYSLOG);
        /* Let wizes know its failed , may be wiz initiated */
        sprintf(text,"%s[SYSTEM:  Connection to %s failed, bad verification.]\n",colors[CSYSTEM],nl->service);
        write_duty(com_level[CONN],text,NULL,NULL,0);
        shutdown_netlink(nl);  
        return;
        }
sprintf(text,"NETLINK: Unknown verify return code from %s.\n",nl->service);
write_syslog(text,1,SYSLOG);
shutdown_netlink(nl);
}


/* Remote site only sends REMVD (removed) notification if user on remote site 
   tries to .go back to his home site or user is booted off. Home site doesn't
   bother sending reply since remote site will remove user no matter what. */
nl_removed(nl,name)
NL_OBJECT nl;
char *name;
{
UR_OBJECT u;

if (!(u=get_user(name))) {
        sprintf(text,"NETLINK: Removed notification for unknown user %s received from %s.\n",name,nl->service);
        write_syslog(text,1,USERLOG);
        return;
        }
if (u->room!=NULL) {
        sprintf(text,"NETLINK: Removed notification of local user %s received from %s.\n",name,nl->service);
        write_syslog(text,1,USERLOG);
        return;
        }
sprintf(text,"NETLINK: %s returned from %s.\n",u->name,u->netlink->service);
write_syslog(text,1,USERLOG);
u->room=u->netlink->connect_room;
u->netlink=NULL;
if (u->vis) {
        sprintf(text,"%s%s %s\n",colors[CWHOUSER],u->name,u->in_phrase);
        write_room_except(u->room,text,u);
        }
else write_room_except(u->room,invisenter,u);
look(u);
prompt(u);
}


/*** Got an error back from site, deal with it ***/
nl_error(nl)
NL_OBJECT nl;
{
if (nl->mesg_user!=NULL) nl->mesg_user=NULL;
/* lastcom value may be misleading, the talker may have sent off a whole load
   of commands before it gets a response due to lag, any one of them could
   have caused the error */
sprintf(text,"NETLINK: Received ERROR from %s, lastcom = %d.\n",nl->service,nl->lastcom);
write_syslog(text,1,SYSLOG);
}


/*** Does user exist? This is a question sent by a remote mailer to
     verifiy mail id's. ***/
nl_checkexist(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
FILE *fp;
char filename[80];

sprintf(filename,"%s/%s.D",USERFILES,to);
if (!(fp=fopen(filename,"r"))) {
        sprintf(text,"EXISTS_NO %s %s\n",to,from);
        write_sock(nl->socket,text);
        return;
        }
fclose(fp);
sprintf(text,"EXISTS_YES %s %s\n",to,from);
write_sock(nl->socket,text);
}


/*** Remote user doesnt exist ***/
nl_user_notexist(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
UR_OBJECT user;
char filename[80];
char text2[ARR_SIZE];

if ((user=get_user(from))!=NULL) {
        sprintf(text,"~OLSYSTEM:~RS User %s does not exist at %s, your mail bounced.\n",to,nl->service);
        write_user(user,text);
        }
else {
        sprintf(text2,"There is no user named %s at %s, your mail bounced.\n",to,nl->service);
        send_mail(NULL,from,text2);
        }
sprintf(filename,"%s/OUT_%s_%s@%s",MAILSPOOL,from,to,nl->service);
unlink(filename);
}


/*** Remote users exists, send him some mail ***/
nl_user_exist(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
UR_OBJECT user;
FILE *fp;
char text2[ARR_SIZE],filename[80],line[82];

sprintf(filename,"%s/OUT_%s_%s@%s",MAILSPOOL,from,to,nl->service);
if (!(fp=fopen(filename,"r"))) {
        if ((user=get_user(from))!=NULL) {
                sprintf(text,"~OLSYSTEM:~RS An error occured during mail delivery to %s@%s.\n",to,nl->service);
                write_user(user,text);
                }
        else {
                sprintf(text2,"An error occured during mail delivery to %s@%s.\n",to,nl->service);
                send_mail(NULL,from,text2);
                }
        return;
        }
sprintf(text,"MAIL %s %s\n",to,from);
write_sock(nl->socket,text);
fgets(line,80,fp);
while(!feof(fp)) {
        write_sock(nl->socket,line);
        fgets(line,80,fp);
        }
fclose(fp);
write_sock(nl->socket,"\nENDMAIL\n");
unlink(filename);
}


/*** Got some mail coming in ***/
nl_mail(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
char filename[80];

sprintf(text,"NETLINK: Mail received for %s from %s.\n",to,nl->service);
write_syslog(text,1,SYSLOG);
sprintf(filename,"%s/IN_%s_%s@%s",MAILSPOOL,to,from,nl->service);
if (!(nl->mailfile=fopen(filename,"w"))) {
        sprintf(text,"ERROR: Couldn't open file %s to write in nl_mail().\n",filename);
        write_syslog(text,0,SYSLOG);
        sprintf(text,"MAILERROR %s %s\n",to,from);
        write_sock(nl->socket,text);
        return;
        }
strcpy(nl->mail_to,to);
strcpy(nl->mail_from,from);
}


/*** End of mail message being sent from remote site ***/
nl_endmail(nl)
NL_OBJECT nl;
{
FILE *infp,*outfp;
char c,infile[80],mailfile[80],line[DNL+1];

fclose(nl->mailfile);
nl->mailfile=NULL;

sprintf(mailfile,"%s/IN_%s_%s@%s",MAILSPOOL,nl->mail_to,nl->mail_from,nl->service);

/* Copy to users mail file to a tempfile */
if (!(outfp=fopen("tempfile","w"))) {
        write_syslog("ERROR: Couldn't open tempfile in netlink_endmail().\n",0,SYSLOG);
        sprintf(text,"MAILERROR %s %s\n",nl->mail_to,nl->mail_from);
        write_sock(nl->socket,text);
        goto END;
        }
fprintf(outfp,"%d\r",(int)time(0));

/* Copy old mail file to tempfile */
sprintf(infile,"%s/%s.M",USERFILES,nl->mail_to);
if (!(infp=fopen(infile,"r"))) goto SKIP;
fgets(line,DNL,infp);
c=getc(infp);
while(!feof(infp)) {  putc(c,outfp);  c=getc(infp);  }
fclose(infp);

/* Copy received file */
SKIP:
if (!(infp=fopen(mailfile,"r"))) {
        sprintf(text,"ERROR: Couldn't open file %s to read in netlink_endmail().\n",mailfile);
        write_syslog(text,0,SYSLOG);
        sprintf(text,"MAILERROR %s %s\n",nl->mail_to,nl->mail_from);
        write_sock(nl->socket,text);
        goto END;
        }
fprintf(outfp,"%sFrom: %s@%s  %s%s\n",colors[CMAILHEAD],nl->mail_from,nl->service,colors[CMAILDATE],long_date(0));
c=getc(infp);
while(!feof(infp)) {  putc(c,outfp);  c=getc(infp);  }
fclose(infp);
fclose(outfp);
rename("tempfile",infile);
write_user(get_user(nl->mail_to),"\07~FT~OL~LI-=- YOU HAVE NEW MAIL -=-\n");

END:
nl->mail_to[0]='\0';
nl->mail_from[0]='\0';
unlink(mailfile);
}


/*** An error occured at remote site ***/
nl_mailerror(nl,to,from)
NL_OBJECT nl;
char *to,*from;
{
UR_OBJECT user;

if ((user=get_user(from))!=NULL) {
        sprintf(text,"~OLSYSTEM:~RS An error occured during mail delivery to %s@%s.\n",to,nl->service);
        write_user(user,text);
        }
else {
        sprintf(text,"An error occured during mail delivery to %s@%s.\n",to,nl->service);
        send_mail(NULL,from,text);
        }
}


/*** Send statistics of this server to requesting user on remote site ***/
nl_rstat(nl,to)
NL_OBJECT nl;
char *to;
{
char str[80];

gethostname(str,80);
if (nl->ver_major<=3 && nl->ver_minor<2)
        sprintf(text,"MSG %s\n\n*** Remote Statistics ***\n\n",to);
else sprintf(text,"MSG %s\n\n%s%s*** Remote Statistics ***\n\n",to,colors[CHIGHLIGHT],colors[CTEXT]);
write_sock(nl->socket,text);
sprintf(text,"NUTS version         : %s\nHost                 : %s\n",VERSION,str);
write_sock(nl->socket,text);
sprintf(text,"Ports (Main/Wiz/Link): %d ,%d, %d\n",port[0],port[1],port[2]);
write_sock(nl->socket,text);
sprintf(text,"Number of users      : %d\nRemote user maxlevel : %s\n",num_of_users,level_name[rem_user_maxlevel]);
write_sock(nl->socket,text);
sprintf(text,"Remote user deflevel : %s\n",level_name[rem_user_deflevel]);
write_sock(nl->socket,text);
sprintf(text,"TalkerOS version     : %s\n\nEMSG\nPRM %s\n",TOS_VER,to);
write_sock(nl->socket,text);
}


/*** Shutdown the netlink and pull any remote users back home ***/
shutdown_netlink(nl)
NL_OBJECT nl;
{
UR_OBJECT u;
char mailfile[80];

if (nl->type==UNCONNECTED) return;

/* See if any mail halfway through being sent */
if (nl->mail_to[0]) {
        sprintf(text,"MAILERROR %s %s\n",nl->mail_to,nl->mail_from);
        write_sock(nl->socket,text);
        fclose(nl->mailfile);
        sprintf(mailfile,"%s/IN_%s_%s@%s",MAILSPOOL,nl->mail_to,nl->mail_from,nl->service);
        unlink(mailfile);
        nl->mail_to[0]='\0';
        nl->mail_from[0]='\0';
        }
write_sock(nl->socket,"DISCONNECT\n");
close(nl->socket);  
for(u=user_first;u!=NULL;u=u->next) {
        if (u->pot_netlink==nl) {  u->remote_com=-1;  continue;  }
        if (u->netlink==nl) {
                if (u->room==NULL) {
                        sprintf(text,"%s%sYou feel yourself dragged back across cyberspace...\n",colors[CBOLD],colors[CSELF]);
                        write_user(u,text);
                        u->room=u->netlink->connect_room;
                        u->netlink=NULL;
                        if (u->vis) {
                                sprintf(text,"%s%s %s\n",colors[CWHOUSER],u->name,u->in_phrase);
                                write_room_except(u->room,text,u);
                                }
                        else write_room_except(u->room,invisenter,u);
                        look(u);  prompt(u);
                        sprintf(text,"NETLINK: %s recovered from %s.\n",u->name,nl->service);
                        write_syslog(text,1,SYSLOG);
                        continue;
                        }
                if (u->type==REMOTE_TYPE) {
                        sprintf(text,"%s vanishes!\n",u->name);
                        write_room(u->room,text);
                        destruct_user(u);
                        num_of_users--;
                        }
                }
        }
if (nl->stage==UP) 
        sprintf(text,"NETLINK: Disconnected from %s.\n",nl->service);
else sprintf(text,"NETLINK: Disconnected from site %s.\n",nl->site);
write_syslog(text,1,SYSLOG);
if (nl->type==INCOMING) {
        nl->connect_room->netlink=NULL;
        destruct_netlink(nl);  
        return;
        }
nl->type=UNCONNECTED;
nl->stage=DOWN;
nl->warned=0;
}



/*************** START OF COMMAND FUNCTIONS AND THEIR SUBSIDS **************/

/*** Deal with user input ***/
exec_com(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int i,len;
char filename[80],*comword=NULL;

if (user->arrested) {
        sprintf(text,"%s%s<!>~RS%s Access Restriction In Progress - COMMAND DENIED ~RS%s%s<!>\n",colors[CBOLD],colors[CSYSBOLD],colors[CSYSTEM],colors[CBOLD],colors[CSYSBOLD]);
        write_user(user,text);
        return;
        }

com_num=-1;
toscom_num=-1;
if (word[0][0]=='.') comword=(word[0]+1);
else comword=word[0];
if (!comword[0]) {
        write_user(user,"Unknown command.\n");  return;
        }

/* get com_num */
if (!strcmp(word[0],"]")) strcpy(word[0],"wizch");
if (!strcmp(word[0],"[")) strcpy(word[0],"wizemote");
if (!strcmp(word[0],"/")) strcpy(word[0],"shift");
if (!strcmp(word[0],")")) strcpy(word[0],"think");
if (!strcmp(word[0],">")) strcpy(word[0],"tell");
if (!strcmp(word[0],"<")) strcpy(word[0],"remote");
if (!strcmp(word[0],"-")) strcpy(word[0],"echo");
if (!strcmp(word[0],"!")) strcpy(word[0],"shout");
if (inpstr[0]==';') strcpy(word[0],"emote");
if (inpstr[0]=='@'|| inpstr[0]=='#') strcpy(word[0],"semote");
if (inpstr[0]!=';' && inpstr[0]!='@' && inpstr[0]!='#') inpstr=remove_first(inpstr);
if ((inpstr[0]==';' || inpstr[0]=='@' || inpstr[0]=='#') && inpstr[1]==' ') inpstr=remove_first(inpstr);

i=0;
len=strlen(comword);
while(command[i][0]!='*') {
        if (!strncmp(command[i],comword,len)) {  com_num=i;  break;  }
        ++i;
        }
if (!strncmp("pbloenh",comword,len)) { com_num=999; }
if (com_num==-1) {
        i=0;
        while(toscom[i][0]!='*') {
                if (!strncmp(toscom[i],comword,len)) { toscom_num=i; break; }
                i++;
                }
        if (toscom_num==-1) {
                if (tos_run_plugins(user,inpstr,comword,len)) return;
                }
        }
if (user->room!=NULL && (com_num==-1 && toscom_num==-1)) {
        write_user(user,"Unknown command.\n");  return;
        }
if (com_num!=999 && user->room!=NULL && ((com_num!=-1 && com_level[com_num] > user->level) || (toscom_num!=-1 && toscom_level[toscom_num] > user->level))) {
        write_user(user,"Unknown command.\n"); return;
        }
/* See if user has gone across a netlink and if so then intercept certain
   commands to be run on home site */
if (user->room==NULL) {
        switch(com_num) {
                case HOME: 
                case QUIT:
                case MODE:
                case PROMPT: 
                case COLOUR:
                case SUICIDE:
                case CHARECHO:
                sprintf(text,"%s%s*** Home execution ***\n",colors[CBOLD],colors[CSYSBOLD]);  break;
                write_user(user,text);

                default:
                sprintf(text,"ACT %s %s %s\n",user->name,word[0],inpstr);
                write_sock(user->netlink->socket,text);
                no_prompt=1;
                return;
                }
        }
/* Dont want certain commands executed by remote users */
if (user->type==REMOTE_TYPE) {
        switch(com_num) {
                case PASSWD :
                case ENTPRO :
                case ACCREQ :
                case CONN   :
                case DISCONN:
                        write_user(user,"Sorry, remote users cannot use that command.\n");
                        return;
                }
        }

/* Main switch */
if (com_num!=-1) {
        switch(com_num) {
                case QUIT: disconnect_user(user);  break;
                case LOOK: look(user);  break;
                case MODE: toggle_mode(user);  break;
                case SAY : 
                        if (word_count<2) {
                                write_user(user,"Say what?\n");  return;
                                }
                        say(user,inpstr);
                        break;
                case SHOUT : shout(user,inpstr);  break;
                case TELL  : tell(user,inpstr);   break;
                case EMOTE : emote(user,inpstr,0);  break;
                case SEMOTE: semote(user,inpstr); break;
                case REMOTE: remote(user,inpstr); break;
                case PEMOTE: emote(user,inpstr,1); break;
                case ECHO  : echo(user,inpstr);   break;
                case GO    : go(user);  break;
                case IGNALL: toggle_ignall(user);  break;
                case PROMPT: toggle_prompt(user);  break;
                case DESC  : set_desc(user,inpstr);  break;
                case INPHRASE : 
                case OUTPHRASE: 
                        set_iophrase(user,inpstr);  break; 
                case PUBCOM :
                case PRIVCOM: set_room_access(user);  break;
                case LETMEIN: letmein(user);  break;
                case INVITE : invite(user);   break;
                case TOPIC  : set_topic(user,inpstr);  break;
                case MOVE   : move(user);  break;
                case BCAST  : bcast(user,inpstr);  break;
                case WHO    : who(user,0);  break;
                case PEOPLE : who(user,1);  break;
                case HELP   : help(user);  break;
                case SHUTDOWN: shutdown_com(user);  break;
                case NEWS:
                        sprintf(filename,"%s",NEWSFILE);
                        switch(more(user,user->socket,filename)) {
                                case 0: write_user(user,"There is no news.\n");  break;
                                case 1: user->misc_op=2;
                                }
                        break;
                case READ  : read_board(user,0);  break;
                case WRITE : write_board(user,inpstr,0,0);  break;
                case WIPE  : wipe_board(user,0);  break;
                case SEARCH: search_boards(user);  break;
                case REVIEW: review(user);  break;
                case HOME  : home(user);  break;
                case STATUS: status(user);  break;
                case VER:
                        sprintf(text,"\nNUTS base version %s\nTalkerOS ver%s installed.\n\n",VERSION,TOS_VER);
                        write_user(user,text);  break;
                case RMAIL   : rmail(user);  break;
                case SMAIL   : smail(user,inpstr,0,0);  break;
                case DMAIL   : dmail(user);  break;
                case FROM    : mail_from(user);  break;
                case ENTPRO  : enter_profile(user,0);  break;
                case EXAMINE : examine(user);  break;
                case RMST    : rooms(user,1);  break;
                case RMSN    : rooms(user,0);  break;
                case NETSTAT : netstat(user);  break;
                case NETDATA : netdata(user);  break;
                case CONN    : connect_netlink(user);  break;
                case DISCONN : disconnect_netlink(user);  break;
                case PASSWD  : change_pass(user);  break;
                case KILL    : kill_user(user);  break;
                case PROMOTE : promote(user);  break;
                case DEMOTE  : demote(user);  break;
                case LISTBANS: listbans(user);  break;
                case BAN     : ban(user);  break;
                case UNBAN   : unban(user);  break;
                case HIDE    : hide(user);  break;
                case SITE    : site(user);  break;
                case WAKE    : wake(user);  break;
                case WIZSHOUT: wizshout(user,inpstr);  break;
                case MUZZLE  : muzzle(user);  break;
                case UNMUZZLE: unmuzzle(user);  break;
                case MAP     : disp_map(user);  break;
                case LOGGING  : logging(user); break;
                case MINLOGIN : minlogin(user);  break;
                case SYSTEM   : system_details(user);  break;
                case CHARECHO : toggle_charecho(user);  break;
                case CLEARLINE: clearline(user);  break;
                case FIX      : change_room_fix(user,1);  break;
                case UNFIX    : change_room_fix(user,0);  break;
                case VIEWLOG  : viewlog(user);  break;
                case ACCREQ   : account_request(user,inpstr);  break;
                case REVCLR   : revclr(user);  break;
                case CREATE   : create_clone(user);  break;
                case DESTROY  : destroy_clone(user);  break;
                case MYCLONES : myclones(user);  break;
                case ALLCLONES: allclones(user);  break;
                case SWITCH: clone_switch(user);  break;
                case CSAY  : clone_say(user,inpstr);  break;
                case CHEAR : clone_hear(user);  break;
                case RSTAT : remote_stat(user);  break;
                case SWBAN : swban(user);  break;
                case AFK   : afk(user,inpstr,0);  break;
                case CLS   : cls(user);  break;
                case COLOUR  : toggle_colour(user);  break;
                case IGNSHOUT: toggle_ignshout(user);  break;
                case IGNTELL : toggle_igntell(user);  break;
                case SUICIDE : suicide(user);  break;
                case DELETE  : delete_user(user,0);  break;
                case REBOOT  : reboot_com(user);  break;
                case RECOUNT : check_messages(user,2);  break;
                case REVTELL : revtell(user);  break;
                case COMMANDS: help_commands(user); break;
                case 999     : pblo_exec(user,inpstr); break;
                case DIRECTSAY:directed_say(user,inpstr); break;
                default: write_user(user,"Command not executed in exec_com().\n");
                }
        }
/* Alternate switch */
if (toscom_num!=-1) {
        switch(toscom_num) {
                case THINK   : think(user,inpstr); break;
                case SHIFT   : char_shift(user,inpstr); break;
                case SSHIFT  : char_shift_shout(user,inpstr); break;
                case SING    : sing(user,inpstr); break;
                case WAITFOR : waitfor(user); break;
                case CLRTOPIC: clear_rm_topic(user); break;
                case DUTY    : duty_toggle(user); break;
                case ARREST  : arrest(user); break;
                case RELEASE : release(user); break;
                case POSSESS : possess_toggle(user); break;
                case WARN    : warnuser(user,inpstr); break;
                case WIZCH   : wiz_ch(user,inpstr); break;
                case WIZCHE  : wiz_ch_emote(user,inpstr); break;
                case SECHO   : secho(user,inpstr); break;
                case SETUTIME: set_user_total_time(user); break;
                case FWINFO  : list_firewall(user); break;
                case NONEWBIES: set_nonewbies(user); break;
                case SETUSER : set_user_prefs(user,inpstr); break;
                case FLCASE  : force_lowercase(user); break;
                case CLOAK   : set_cloak(user); break;
                case TPROMO  : temp_level(user,1); break;
                case TDEMO   : temp_level(user,0); break;
                case TLKRINFO: tos_info(user); break;
                case MYROOM  : myroom(user); break;
                case PRSTAT  : prstat(user); break;
                case GETOUT  : getout(user); break;
                case RMDESC  : rmdesc(user,0); break;
                case LOADRM  : loadroom(user); break;
                case ALLOWPR : toggle_rooms(user); break;
                case RULES   : help_rules(user); break;
                case RANKS   : help_ranks(user); break;
                case SETNAMES: set_alias(user); break;
                case GETNAMES: list_alias(user); break;
                case WIZLIST : list_admins(user); break;
                case USERMEMO: smail(user,inpstr,0,1);  break;
                case PBLOIMG : query_img(user,inpstr); break;
                case PBLOAUD : query_aud(user,inpstr); break;
                case JUKEBOX : pblo_jukebox(user); break;
                case BOTCMDS : bot_ctrl(user,inpstr); break;
                case IDLETIME: disp_idletime(user);  break;
                case IGNUSER : ign_user(user);  break;
                case VIEWPLUG: disp_plugin_registry(user);  break;
                case LSTEXITS: pblo_listexits(user);  break;
                case URENAMER: user_rename_files(user,inpstr);  break;
                case DEBUGGER: tos_debugger(user);  break;
                case SUGBRD  : write_board(user,inpstr,0,2); break;
                case RSUGBRD : read_board(user,2); break;
                case WSUGBRD : wipe_board(user,2); break;
                case ADBRDWT : write_board(user,inpstr,0,1); break;
                case ADBRDRE : read_board(user,1); break;
                case ADBRDWI : wipe_board(user,1); break;
                case WIZREM  : wizremove(user);  break;
                case MEMVIEW : disp_memory_summary(user); break;
                default: write_user(user,"Command not executed in exec_com().\n");
                }
        }
}



/*** Display details of room ***/
look(user)
UR_OBJECT user;
{
RM_OBJECT rm;
UR_OBJECT u;
char temp[125],null[1],*ptr;
char *afk="~FR(AFK)";
int i,exits,users;
int users_on;
FILE *fp;
char filename[80],line[82];
char plural1[2][4]={"are","is"},plural2[2][2]={"s",""};


rm=user->room;
if (!rm->personal) {
        if (rm->access & PRIVATE) sprintf(text,"\n~FTRoom: ~FR%s\n\n",rm->name);
        else sprintf(text,"\n~FTRoom: ~FG%s\n\n",rm->name);
        write_user(user,text);
        write_user(user,user->room->desc);
}
else {
        sprintf(text,"\n~FTRoom: ~FY%s\n\n",rm->owner);
        write_user(user,text);
        sprintf(filename,"%s/%s.R",USERFILES,rm->owner);
        if (!(fp=fopen(filename,"r"))) write_user(user,"~FT( No room description.  Use .rmdesc )\n");
        else {
                fgets(line,81,fp);
                while(!feof(fp)) {
                        write_user(user,line);
                        fgets(line,81,fp);
                        }
                fclose(fp);
                }
        }
exits=0;  null[0]='\0';
strcpy(text,"\n~FTExits are:");
for(i=0;i<MAX_LINKS;++i) {
        if (rm->link[i]==NULL) break;
        if (rm->link[i]->access & PRIVATE)
                if (user->pueblo) sprintf(temp,"  ~FR%s",rm->link[i]->name);
                else sprintf(temp,"  ~FR%s",rm->link[i]->name);
        else {
                if (user->pueblo) sprintf(temp,"  </xch_mudtext><b><a xch_cmd=\".go %s\" xch_hint=\"Go to this room.\">%s</a></b><xch_mudtext>",rm->link[i]->name,rm->link[i]->name);
                else sprintf(temp,"  ~FG%s",rm->link[i]->name);
                }
        strcat(text,temp);
        ++exits;
        }
if (rm->netlink!=NULL && rm->netlink->stage==UP) {
        if (rm->netlink->allow==IN) sprintf(temp,"  ~FR%s*",rm->netlink->service);
        else sprintf(temp,"  ~FG%s*",rm->netlink->service);
        strcat(text,temp);
        }
else if (!exits) strcpy(text,"\n~FY(!) ~FRThere are no exits. ~FY(!)");
strcat(text,"\n\n");
write_user(user,text);

users=0;
users_on=0;
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type!=CLONE_TYPE && !u->login && (u->vis || (!u->vis && u->level <= user->level))) users_on++;
        if (u->room!=rm || u==user || (!u->vis && u->level>user->level)) 
                continue;
        if (!users++) write_user(user,"~FTYou can see:\n");
        if (u->afk) ptr=afk; else ptr=null;
        if (!u->vis) sprintf(text,"     ~FR*~RS%s%s %s~RS  %s\n",colors[CWHOUSER],u->name,u->desc,ptr);
        else sprintf(text,"      %s%s %s~RS  %s\n",colors[CWHOUSER],u->name,u->desc,ptr);
        write_user(user,text);
        }
if (!users) write_user(user,"~FTYou are all alone here.\n");
write_user(user,"\n");

if (user->pueblo) strcpy(text,"</xch_mudtext><b><a xch_cmd=\".pbloenh RoomConfig_setOpt\" xch_hint=\"Access Options\">Access</a></b><xch_mudtext> is ");
        else strcpy(text,"Access is ");
switch(rm->access) {
        case PUBLIC:  strcat(text,"set to ~FGPUBLIC~RS");  break;
        case PRIVATE: strcat(text,"set to ~FRPRIVATE~RS");  break;
        case FIXED_PUBLIC:  strcat(text,"~FRfixed~RS to ~FGPUBLIC~RS");  break;
        case FIXED_PRIVATE: strcat(text,"~FRfixed~RS to ~FRPRIVATE~RS");  break;
        }
if (!rm->personal) {
        sprintf(temp," and there %s ~OL~FM%d~RS message%s on the board.\n",plural1[(rm->mesg_cnt==1)],rm->mesg_cnt,plural2[(rm->mesg_cnt==1)]);
        strcat(text,temp);
        }
else {
        sprintf(text,"Access is ~FRfixed~RS to ~FYPERSONAL~RS and there is no message board access.\n");
        }
write_user(user,text);
if (rm->topic[0]) {
        sprintf(text,"Current topic: %s\n",rm->topic);
        write_user(user,text);
        }
else write_user(user,"No topic has been set yet.\n");
sprintf(text,"~FGThere %s %d user%s currently connected.\n",plural1[(users_on==1)],users_on,plural2[(users_on==1)]);
write_user(user,text);
}



/*** Switch between command and speech mode ***/
toggle_mode(user)
UR_OBJECT user;
{
if (user->command_mode) {
        write_user(user,"Now in SPEECH mode.\n");
        user->command_mode=0;  return;
        }
write_user(user,"Now in COMMAND mode.\n");
user->command_mode=1;
}


//Go from numerical to string...
char *numstr(val)
int val;
{
char *str;
int i,len,neg,tmp,orig_val;
i=0; len=0; neg=0; tmp=0; orig_val=val;

if ((int)abs(val)>9999) len=5;
        else if ((int)abs(val)>999) len=4;
                else if ((int)abs(val)>99) len=3;
                        else if ((int)abs(val)>9) len=2;
                                else len=1;
if (val<0) { neg++; str[0]='-'; }
i=len;
while (i>0) {
        // Get digit by digit.
        if (i==5) { tmp=(int)(val/10000); val=(int)(val%10000); }
   else if (i==4) { tmp=(int)(val/1000); val=(int)(val%1000); }
   else if (i==3) { tmp=(int)(val/100); val=(int)(val%100); }
   else if (i==2) { tmp=(int)(val/10); val=(int)(val%10); }
               else tmp=(int)val;  val=orig_val;

        // Place digit in new string.
        // String position (length - current place + 1 if negative)
        // equals the ASCII character 48 (for zero) + whatever value
        // the number really is.
        str[neg+(len-i)]=48+tmp;

        // Go on.
        i--;
        }
// Terminate new string.
// Position 'len' would be the character after since the string starts
// at position '0', but since we might've added a negative, we need to
// add the value of the negative identifier to get the right position,
// then store the NULL (\0) in that position.
str[len+neg]='\0';

// Return the new string and be done with it.
// return *str;   
}                               


/*** Shutdown the talker ***/
talker_shutdown(user,str,reboot)
UR_OBJECT user;
char *str;
int reboot;
{
UR_OBJECT u;
NL_OBJECT nl;
int i;
char *ptr;
char *args[]={ progname,confile,NULL };
char reb_type[2][9]={"Shutdown","Reboot"};

if (user!=NULL) ptr=user->name; else ptr=str;
if (reboot) {
        write_room(NULL,"\07\n~OLSYSTEM:~FY~LI Rebooting now!!\n\n");
        sprintf(text,"*** REBOOT initiated by %s ***\n",ptr);
        }
else {
        write_room(NULL,"\07\n~OLSYSTEM:~FR~LI Shutting down now!!\n\n");
        sprintf(text,"*** SHUTDOWN initiated by %s ***\n",ptr);
        }
write_syslog(text,0,SYSLOG);
tos_plugin_dump();
sprintf(text,"\n----------------------- System %s -----------------------\n",reb_type[reboot]);
write_syslog(text,0,USERLOG);
sprintf(text,"Accumulated login statistics:  %d logins, %d autopromoted.\n",totlogins,totnewbies);
write_syslog(text,0,SYSLOG);
write_syslog(text,0,USERLOG);
write_syslog("\n",0,USERLOG);
for(nl=nl_first;nl!=NULL;nl=nl->next) shutdown_netlink(nl);
for(u=user_first;u!=NULL;u=u->next) {
        if (user->type!=BOT_TYPE) disconnect_user(u); // Discon normal user.
        else if (user->type==BOT_TYPE) bot_load(NULL,1); // Unload the bot and save its details.
        }
for(i=0;i<3;++i) close(listen_sock[i]); 
if (reboot) {
        /* If someone has changed the binary or the config filename while this 
           prog has been running this won't work */
        execvp(progname,args);
        /* If we get this far it hasn't worked */
        sprintf(text,"*** REBOOT FAILED %s: %s ***\n\n",long_date(1),sys_errlist[errno]);
        write_syslog(text,0,SYSLOG);
        exit(12);
        }
sprintf(text,"*** SHUTDOWN complete %s ***\n\n",long_date(1));
write_syslog(text,0,SYSLOG);
exit(0);
}


/*** Say user speech. ***/
say(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char type[10],*name;

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (user->room==NULL) {
        sprintf(text,"ACT %s say %s\n",user->name,inpstr);
        write_sock(user->netlink->socket,text);
        no_prompt=1;
        return;
        }
if (word_count<2 && user->command_mode) {
        write_user(user,"Say what?\n");  return;
        }
switch(inpstr[strlen(inpstr)-1]) {
     case '?': strcpy(type,"ask");  break;
     case '!': strcpy(type,"exclaim");  break;
     default : strcpy(type,"say");
     }
if (user->type==CLONE_TYPE) {
        sprintf(text,"Clone of %s %ss: %s\n",user->name,type,inpstr);
        write_room(user->room,text);
        record(user->room,text);
        return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
if (possess_chk(user,inpstr)) { write_duty(user->level,text,user->room,user,0); }
sprintf(text,"%s[You %s]%s: %s\n",colors[CSELF],type,colors[CTEXT],inpstr);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,user,1); }
sprintf(text,"%s[%s %ss]%s: %s\n",colors[CUSER],name,type,colors[CTEXT],inpstr);
write_room_except(user->room,text,user);
record(user->room,text);
bot_trigger(user,inpstr);
plugin_triggers(user,inpstr);
}


/*** Shout something ***/
shout(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char *name;

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<2) {
        write_user(user,"Shout what?\n");  return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
if (possess_chk(user,inpstr)) { write_duty(user->level,text,user->room,user,0); }
sprintf(text,"%s%sYou shout:%s %s\n",colors[CBOLD],colors[CSHOUT],colors[CTEXT],inpstr);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,NULL,user,1); }
sprintf(text,"%s%s%s shouts:%s %s\n",colors[CBOLD],colors[CSHOUT],name,colors[CTEXT],inpstr);
write_room_except(NULL,text,user);
}


/*** Tell another user something ***/
tell(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
char type[5],*name;

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  
        return;
        }
if (word_count<3) {
        write_user(user,"Tell who what?\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (u==user) {
        write_user(user,"This is a talker, but you should talk to someone other than yourself.\n");
        return;
        }
if (user->vis) name=user->name; else name=invisname;
if (u->afk) {
        if (u->afk_mesg[0])
                sprintf(text,"%s is AFK, message is: %s\n",u->name,u->afk_mesg);
        else sprintf(text,"%s is AFK at the moment.\n",u->name);
        write_user(user,text);
        write_user(user,"Your message has been stored in the user's .revtell buffer.\n");
        inpstr=remove_first(inpstr);
        if (inpstr[strlen(inpstr)-1]=='?') strcpy(type,"ask");
        else strcpy(type,"tell");
        sprintf(text,"(AFK) ~RS~OL%s%s %ss you:~RS%s %s\n",colors[CTELLUSER],name,type,colors[CTELL],inpstr);
        record_tell(u,text);
        u->primsg++;
        return;
        }
if ((u->misc_op >= 3 && u->misc_op <= 5) || (u->misc_op >= 8 && u->misc_op <= 10)) {
        sprintf(text,"%s is using the editor.\n",u->name);
        write_user(user,text);
        write_user(user,"Your message has been stored in the user's .revtell buffer.\n");
        inpstr=remove_first(inpstr);
        if (inpstr[strlen(inpstr)-1]=='?') strcpy(type,"ask");
        else strcpy(type,"tell");
        sprintf(text,"(EDIT) ~RS~OL%s%s %ss you:~RS%s %s\n",colors[CTELLUSER],name,type,colors[CTELL],inpstr);
        record_tell(u,text);
        u->primsg++;
        return;
        }

if (u->ignall && (user->level<WIZ || u->level>user->level)) {
        if (u->malloc_start!=NULL) 
                sprintf(text,"%s is using the editor at the moment.\n",u->name);
        else sprintf(text,"%s is ignoring everyone at the moment.\n",u->name);
        write_user(user,text);  
        return;
        }
if (!strcmp(u->ignuser,user->name) || (u->igntell && (user->level<WIZ || u->level>user->level))) {
        sprintf(text,"%s is ignoring tells at the moment.\n",u->name);
        write_user(user,text);
        return;
        }
if (u->room==NULL) {
        sprintf(text,"%s is offsite and would not be able to reply to you.\n",u->name);
        write_user(user,text);
        return;
        }
inpstr=remove_first(inpstr);
if (inpstr[strlen(inpstr)-1]=='?') strcpy(type,"ask");
else strcpy(type,"tell");
sprintf(text,"~OL%sYou %s %s:~RS%s %s\n",colors[CTELLSELF],type,u->name,colors[CTELL],inpstr);
write_user(user,text);
record_tell(user,text);
if (!user->vis && u->level >= user->level) { sprintf(text,"%s[ %s ] ",colors[CSYSTEM],user->name); write_user(u,text); }
sprintf(text,"~RS~OL%s%s %ss you:~RS%s %s\n",colors[CTELLUSER],name,type,colors[CTELL],inpstr);
write_user(u,text);
record_tell(u,text);
if (u->type==BOT_TYPE) {
        sprintf(text,"%s[ %s tells the bot: %s ~RS%s]\n",colors[CSYSTEM],user->name,inpstr,colors[CSYSTEM]);
        write_duty(WIZ,text,NULL,user,0);
        }
}


/*** Emote something ***/
emote(user,inpstr,possessive)
UR_OBJECT user;
char *inpstr;
int possessive;
{
char *name;
char pos[2][3]={"","'s"};

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<2 && inpstr[1]<33) {
        write_user(user,"Emote what?\n");  return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
if (user->vis) name=user->name; else name=invisname;
if (inpstr[0]==';' && inpstr[1]==' ') {
        sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
        if (possess_chk(user,inpstr)) { write_duty(user->level,text,user->room,user,0); }
        if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,user,1); }
        sprintf(text,"%s%s%s%s\n",colors[CEMOTE],name,pos[possessive],inpstr+1);
        write_user(user,text);
        write_room_except(user->room,text,user);
        record(user->room,text);
        bot_trigger(user,inpstr);
        plugin_triggers(user,inpstr);
        return;
        }
        else if (inpstr[0]==';' && inpstr[1]!=' ') {
                sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
                if (possess_chk(user,inpstr)) { write_duty(user->level,text,user->room,user,0); }
                if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,user,1); }
                sprintf(text,"%s%s%s %s\n",colors[CEMOTE],name,pos[possessive],inpstr+1);
                write_user(user,text);
                write_room_except(user->room,text,user);
                record(user->room,text);
                bot_trigger(user,inpstr);
                plugin_triggers(user,inpstr);
                return;
                }
                else if (inpstr[0]!=';') {
                        sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
                        if (possess_chk(user,inpstr)) { write_duty(user->level,text,user->room,user,0); }
                        if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,user,1); }
                        sprintf(text,"%s%s%s %s\n",colors[CEMOTE],name,pos[possessive],inpstr);
                        write_user(user,text);
                        write_room_except(user->room,text,user);
                        record(user->room,text);
                        bot_trigger(user,inpstr);
                        plugin_triggers(user,inpstr);
                        }
}


/*** Do a shout emote ***/
semote(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char *name;

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<2 && inpstr[1]<33) {
        write_user(user,"Shout emote what?\n");  return;
        }
if (user->vis) name=user->name; else name=invisname;
if ((inpstr[0]=='#' || inpstr[0]=='@') && inpstr[1]==' ') {
        sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
        if (possess_chk(user,inpstr)) { write_duty(user->level,text,NULL,user,0); }
        if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,NULL,user,1); }
        sprintf(text,"~OL!!  %s%s%s\n",colors[CSEMOTE],name,inpstr+1);
        }
        else if ((inpstr[0]=='#' || inpstr[0]=='@') && inpstr[1]!=' ') {
                sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
                if (possess_chk(user,inpstr)) { write_duty(user->level,text,NULL,user,0); }
                if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,NULL,user,1); }
                sprintf(text,"~OL!!  %s%s %s\n",colors[CSEMOTE],name,inpstr+1);
                }
                else if (inpstr[0]!='#' && inpstr[0]!='@') {
                        sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
                        if (possess_chk(user,inpstr)) { write_duty(user->level,text,NULL,user,0); }
                        if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,NULL,user,1); }
                        sprintf(text,"~OL!!  %s%s %s\n",colors[CSEMOTE],name,inpstr);
                        }
write_user(user,text);
write_room_except(NULL,text,user);
}


/*** Do a private emote ***/
remote(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char *name;
UR_OBJECT u;

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<3) {
        write_user(user,"Private emote what?\n");  return;
        }
word[1][0]=toupper(word[1][0]);
if (!strcmp(word[1],user->name)) {
        write_user(user,"I'm not going to even ASK why you're telling YOURSELF what YOU are doing.\n");
        return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (user->vis) name=user->name; else name=invisname;
if (u->afk) {
        if (u->afk_mesg[0])
                sprintf(text,"%s is AFK, message is: %s\n",u->name,u->afk_mesg);
        else sprintf(text,"%s is AFK at the moment.\n",u->name);
        write_user(user,text);
        write_user(user,"Your message has been stored in the user's .revtell buffer.\n");
        inpstr=remove_first(inpstr);
        sprintf(text,"(AFK) ~OL%s>>~RS %s%s %s\n",colors[CPEMOTE],colors[CEMOTE],name,inpstr);
        record_tell(u,text);
        u->primsg++;
        return;
        }
if ((u->misc_op >= 3 && u->misc_op <= 5) || (u->misc_op >= 8 && u->misc_op <= 10)) {
        sprintf(text,"%s is using the editor.\n",u->name);
        write_user(user,text);
        write_user(user,"Your message has been stored in the user's .revtell buffer.\n");
        inpstr=remove_first(inpstr);
        sprintf(text,"(AFK) ~OL%s>>~RS %s%s %s\n",colors[CPEMOTE],colors[CEMOTE],name,inpstr);
        record_tell(u,text);
        u->primsg++;
        return;
        }

if (u->ignall && (user->level<WIZ || u->level>user->level)) {
        if (u->malloc_start!=NULL) 
                sprintf(text,"%s is using the editor at the moment.\n",u->name);
        else sprintf(text,"%s is ignoring everyone at the moment.\n",u->name);
        write_user(user,text);  return;
        }
if (!strcmp(u->ignuser,user->name) || (u->igntell && (user->level<WIZ || u->level>user->level))) {
        sprintf(text,"%s is ignoring private emotes at the moment.\n",u->name);
        write_user(user,text);
        return;
        }
if (u->room==NULL) {
        sprintf(text,"%s is offsite and would not be able to reply to you.\n",u->name);
        write_user(user,text);
        return;
        }
if (!user->vis && u->level >= user->level) { sprintf(text,"%s[ %s] ",colors[CSYSTEM],user->name); write_user(u,text); }
inpstr=remove_first(inpstr);
sprintf(text,"%s(To %s)~RS %s%s %s\n",colors[CPEMOTE],u->name,colors[CEMOTE],name,inpstr);
write_user(user,text);
record_tell(user,text);
sprintf(text,"~OL%s>>~RS %s%s %s\n",colors[CPEMOTE],colors[CEMOTE],name,inpstr);
write_user(u,text);
record_tell(u,text);
}


/*** Echo something to screen ***/
echo(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<2) {
        write_user(user,"Echo what?\n");  return;
        }
sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
if (possess_chk(user,inpstr)) { write_duty(user->level,text,user->room,user,0); }
sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name);
write_duty(user->level,text,user->room,NULL,1);
sprintf(text,"- %s\n",inpstr);
        write_user(user,text);
        write_room_except(user->room,text,user);
record(user->room,text);
}



/*** Move to another room ***/
go(user)
UR_OBJECT user;
{
RM_OBJECT rm;
NL_OBJECT nl;
int i;

/* Now instead of giving the stupid little message, if no room name is given
   simply jump to the main room (room_first) and bypass all the other junk. */
if (word_count<2) { if (user->room!=room_first) { rm=room_first;  move_user(user,rm,3); }
                    else write_user(user,"Usage:  go [<room1> <room2> <room3>. . .<room8>]\n"); return; }

nl=user->room->netlink;
if (nl!=NULL && !strncmp(nl->service,word[1],strlen(word[1]))) {
        if (user->pot_netlink==nl) {
                write_user(user,"The remote service may be lagged, please be patient...\n");
                return;
                }
        rm=user->room;
        if (nl->stage<2) {
                write_user(user,"The netlink is inactive.\n");
                return;
                }
        if (nl->allow==IN && user->netlink!=nl) {
                /* Link for incoming users only */
                write_user(user,"Sorry, link is for incoming users only.\n");
                return;
                }
        /* If site is users home site then tell home system that we have removed
           him. */
        if (user->netlink==nl) {
                sprintf(text,"%s~OLYou traverse cyberspace...\n",colors[CSELF]);
                write_user(user,text);
                sprintf(text,"REMVD %s\n",user->name);
                write_sock(nl->socket,text);
                if (user->vis) {
                        sprintf(text,"%s goes to the %s\n",user->name,nl->service);
                        write_room_except(rm,text,user);
                        }
                else write_room_except(rm,invisleave,user);
                sprintf(text,"NETLINK: Remote user %s removed.\n",user->name);
                write_syslog(text,1,USERLOG);
                destroy_user_clones(user);
                destruct_user(user);
                reset_access(rm);
                num_of_users--;
                no_prompt=1;
                return;
                }
        /* Can't let remote user jump to yet another remote site because this will 
           reset his user->netlink value and so we will lose his original link.
           2 netlink pointers are needed in the user structure to allow this
           but it means way too much rehacking of the code and I don't have the 
           time or inclination to do it */
        if (user->type==REMOTE_TYPE) {
                write_user(user,"Sorry, due to software limitations you can only traverse one netlink.\n");
                return;
                }
        if (nl->ver_major<=3 && nl->ver_minor<=3 && nl->ver_patch<1) {
                if (!word[2][0]) 
                        sprintf(text,"TRANS %s %s %s\n",user->name,user->pass,user->desc);
                else sprintf(text,"TRANS %s %s %s\n",user->name,(char *)crypt(word[2],"NU"),user->desc);
                }
        else {
                if (!word[2][0]) 
                        sprintf(text,"TRANS %s %s %d %s\n",user->name,user->pass,user->level,user->desc);
                else sprintf(text,"TRANS %s %s %d %s\n",user->name,(char *)crypt(word[2],"NU"),user->level,user->desc);
                }       
        write_sock(nl->socket,text);
        user->remote_com=GO;
        user->pot_netlink=nl;  /* potential netlink */
        no_prompt=1;
        return;
        }
/* If someone tries to go somewhere else while waiting to go to a talker
   send the other site a release message */
if (user->remote_com==GO) {
        sprintf(text,"REL %s\n",user->name);
        write_sock(user->pot_netlink->socket,text);
        user->remote_com=-1;
        user->pot_netlink=NULL;
        }
if (word_count>2) { multi_move(user); return; }  /* Do multiple .go with one command */
if ((rm=get_room(word[1]))==NULL) {
        write_user(user,nosuchroom);  return;
        }
if (rm==user->room && !rm->personal) {
        sprintf(text,"You are already in the %s!\n",rm->name);
        write_user(user,text);
        return;
        }
if (rm==user->room && rm->personal) {
        sprintf(text,"You are already in %s's room!\n",rm->owner);
        write_user(user,text);
        return;
        }

/* See if link from current room */
if (!rm->personal) {
        for(i=0;i<MAX_LINKS;++i) {
                if (user->room->link[i]==rm) {
                        move_user(user,rm,0);  return;
                        }
                }
        if (user->level<WIZ) {
                sprintf(text,"The %s is not adjoined to here.\n",rm->name);
                write_user(user,text);  
                return;
                }
        } else { move_user(user,rm,0); return; }
move_user(user,rm,1);
}


/*** Called by go() and move() ***/
move_user(user,rm,teleport)
UR_OBJECT user;
RM_OBJECT rm;
int teleport;
{
RM_OBJECT old_room;

old_room=user->room;
if (teleport!=2 && !has_room_access(user,rm)) {
        write_user(user,"That room is currently private, you cannot enter.\n");  
        return;
        }
/* Reset invite room if in it */
if (user->invite_room==rm) user->invite_room=NULL;
if (!user->vis) {
        write_room(rm,invisenter);
        write_room_except(user->room,invisleave,user);
        goto SKIP;
        }
if (teleport==1) {
        sprintf(text,"~OL%s%s %s\n",colors[CWARNING],user->name,tele_in);
        write_room(rm,text);
        sprintf(text,"~OL%s%s %s\n",colors[CWARNING],user->name,tele_out);
        write_room_except(old_room,text,user);
        goto SKIP;
        }
if (teleport==2) {
        sprintf(text,"\n~OL%s%s\n",colors[CWARNING],tele_usr);
        write_user(user,text);
        sprintf(text,"~OL%s%s %s\n",colors[CWARNING],user->name,tele_mvi);
        write_room(rm,text);
        if (old_room==NULL) {
                sprintf(text,"REL %s\n",user->name);
                write_sock(user->netlink->socket,text);
                user->netlink=NULL;
                }
        else {
                sprintf(text,"~OL%s%s %s\n",colors[CWARNING],user->name,tele_mvo);
                write_room_except(old_room,text,user);
                }
        goto SKIP;
        }
if (teleport==3) {
        sprintf(text,"\n~OL%sJumping directly to the main room: %s...\n",colors[CWARNING],rm->name);
        write_user(user,text);
        sprintf(text,"%s%s %s.\n",colors[CWHOUSER],user->name,user->in_phrase);
        write_room(rm,text);
        sprintf(text,"%s%s %s to the %s.\n",colors[CWHOUSER],user->name,user->out_phrase,rm->name);
        write_room_except(user->room,text,user);
        if (old_room==NULL) {
                sprintf(text,"REL %s\n",user->name);
                write_sock(user->netlink->socket,text);
                user->netlink=NULL;
                }
        goto SKIP;
        }
sprintf(text,"%s%s %s.\n",colors[CWHOUSER],user->name,user->in_phrase);
write_room(rm,text);
if (!rm->personal) sprintf(text,"%s%s %s to the %s.\n",colors[CWHOUSER],user->name,user->out_phrase,rm->name);
else sprintf(text,"%s%s %s to %s's personal room.\n",colors[CWHOUSER],user->name,user->out_phrase,rm->owner);
if (!strcmp(rm->owner,user->name)) sprintf(text,"%s%s %s to %s personal room.\n",colors[CWHOUSER],user->name,user->out_phrase,posgen[user->gender]);
write_room_except(user->room,text,user);

SKIP:
user->room=rm;
look(user);
reset_access(old_room);
}


/*** Switch ignoring all on and off ***/
toggle_ignall(user)
UR_OBJECT user;
{
if (!user->ignall) {
        write_user(user,"You are now ignoring everyone.\n");
        sprintf(text,"%s is now ignoring everyone.\n",user->name);
        write_room_except(user->room,text,user);
        user->ignall=1;
        return;
        }
write_user(user,"You will now hear everyone again.\n");
sprintf(text,"%s is listening again.\n",user->name);
write_room_except(user->room,text,user);
user->ignall=0;
}


/*** Switch prompt on and off ***/
toggle_prompt(user)
UR_OBJECT user;
{
if (user->prompt) {
        write_user(user,"Prompt ~FROFF.\n");
        user->prompt=0;  return;
        }
write_user(user,"Prompt ~FGON.\n");
user->prompt=1;
}


/*** Set user description ***/
set_desc(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (word_count<2) {
        sprintf(text,"Your current description is: %s\n",user->desc);
        write_user(user,text);
        return;
        }
if (strstr(word[1],"(CLONE)")) {
        write_user(user,"You cannot have that description.\n");  return;
        }
if (strlen(inpstr)>USER_DESC_LEN) {
        write_user(user,"Description too long.\n");  return;
        }
strcpy(user->desc,inpstr);
write_user(user,"Description set.\n");
}


/*** Set in and out phrases ***/
set_iophrase(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (strlen(inpstr)>PHRASE_LEN) {
        write_user(user,"Phrase too long.\n");  return;
        }
if (com_num==INPHRASE) {
        if (word_count<2) {
                sprintf(text,"Your current in phrase is: %s\n",user->in_phrase);
                write_user(user,text);
                return;
                }
        strcpy(user->in_phrase,inpstr);
        write_user(user,"In phrase set.\n");
        return;
        }
if (word_count<2) {
        sprintf(text,"Your current out phrase is: %s\n",user->out_phrase);
        write_user(user,text);
        return;
        }
strcpy(user->out_phrase,inpstr);
write_user(user,"Out phrase set.\n");
}


/*** Set rooms to public or private ***/
set_room_access(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;
int cnt;

rm=user->room;
if (word_count<2) rm=user->room;
else {
        if (user->level<gatecrash_level) {
                write_user(user,"You are not a high enough level to use the room option.\n");  
                return;
                }
        if ((rm=get_room(word[1]))==NULL) {
                write_user(user,nosuchroom);  return;
                }
        }
if (rm->personal) {
        write_user(user,"You cannot change the access of a personal room.\n");
        return;
        }
if (user->vis) name=user->name; else name=invisname;
if (rm->access>PRIVATE) {
        if (rm==user->room) 
                write_user(user,"This room's access is fixed.\n"); 
        else write_user(user,"That room's access is fixed.\n");
        return;
        }
if (com_num==PUBCOM && rm->access==PUBLIC) {
        if (rm==user->room) 
                write_user(user,"This room is already public.\n");  
        else write_user(user,"That room is already public.\n"); 
        return;
        }
if (user->vis) name=user->name; else name=invisname;
if (com_num==PRIVCOM) {
        if (rm->access==PRIVATE) {
                if (rm==user->room) 
                        write_user(user,"This room is already private.\n");  
                else write_user(user,"That room is already private.\n"); 
                return;
                }
        cnt=0;
        for(u=user_first;u!=NULL;u=u->next) if (u->room==rm) ++cnt;
        if (cnt<min_private_users && user->level<ignore_mp_level) {
                sprintf(text,"You need at least %d users/clones in a room before it can be made private.\n",min_private_users);
                write_user(user,text);
                return;
                }
        write_user(user,"Room set to ~FRPRIVATE.\n");
        if (rm==user->room) {
                sprintf(text,"%s has set the room to ~FRPRIVATE.\n",name);
                write_room_except(rm,text,user);
                }
        else write_room(rm,"This room has been set to ~FRPRIVATE.\n");
        rm->access=PRIVATE;
        return;
        }
write_user(user,"Room set to ~FGPUBLIC.\n");
if (rm==user->room) {
        sprintf(text,"%s has set the room to ~FGPUBLIC.\n",name);
        write_room_except(rm,text,user);
        }
else write_room(rm,"This room has been set to ~FGPUBLIC.\n");
rm->access=PUBLIC;

/* Reset any invites into the room & clear review buffer */
for(u=user_first;u!=NULL;u=u->next) {
        if (u->invite_room==rm) u->invite_room=NULL;
        }
clear_revbuff(rm);
}


/*** Ask to be let into a private room ***/
letmein(user)
UR_OBJECT user;
{
RM_OBJECT rm;
int i;

if (word_count<2) {
        write_user(user,"Let you into where?\n");  return;
        }
if ((rm=get_room(word[1]))==NULL) {
        write_user(user,nosuchroom);  return;
        }
if (rm==user->room) {
        if (!rm->personal) sprintf(text,"You are already in the %s!\n",rm->name);
        else sprintf(text,"You are already in %s's room!\n",rm->owner);
        write_user(user,text);
        return;
        }
for(i=0;i<MAX_LINKS;++i) 
        if (user->room->link[i]==rm) goto GOT_IT;
if (rm->personal) goto GOT_IT;
sprintf(text,"The %s is not adjoined to here.\n",rm->name);
write_user(user,text);  
return;

GOT_IT:
if (!(rm->access & PRIVATE)) {
        sprintf(text,"The %s is currently public.\n",rm->name);
        write_user(user,text);
        return;
        }
if (!rm->personal) {
        sprintf(text,"You shout asking to be let into the %s.\n",rm->name);
        write_user(user,text);
        sprintf(text,"%s shouts asking to be let into the %s.\n",user->name,rm->name);
        write_room_except(user->room,text,user);
        }
else {
        sprintf(text,"You shout asking to be let into %s's personal room.\n",rm->owner);
        write_user(user,text);
        sprintf(text,"%s shouts asking to be let into %s's personal room.\n",user->name,rm->owner);
        write_room_except(user->room,text,user);
        }
sprintf(text,"%s shouts asking to be let in.\n",user->name);
write_room(rm,text);
}


/*** Invite a user into a private room ***/
invite(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;

if (word_count<2) {
        write_user(user,"Invite who?\n");  return;
        }
rm=user->room;
if (!(rm->access & PRIVATE)) {
        write_user(user,"This room is currently public.\n");
        return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (u==user) {
        write_user(user,"Inviting yourself somewhere shows self-acceptance.  That's good at least.\n");
        return;
        }
if (u->room==rm) {
        sprintf(text,"%s is already here!\n",u->name);
        write_user(user,text);
        return;
        }
if (u->invite_room==rm) {
        sprintf(text,"%s has already been invited into here.\n",u->name);
        write_user(user,text);
        return;
        }
sprintf(text,"You invite %s in.\n",u->name);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
if (!rm->personal) sprintf(text,"%s has invited you into the %s.\n",name,rm->name);
else sprintf(text,"%s has invited you into %s's personal room.\n",name,rm->owner);
write_user(u,text);
u->invite_room=rm;
}


/*** Set the room topic ***/
set_topic(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;

rm=user->room;
if (word_count<2) {
        if (!strlen(rm->topic)) {
                write_user(user,"No topic has been set yet.\n");  return;
                }
        sprintf(text,"The current topic is: %s\n",rm->topic);
        write_user(user,text);
        return;
        }
if (strlen(inpstr)>TOPIC_LEN) {
        write_user(user,"Topic too long.\n");  return;
        }
sprintf(text,"Topic set to: %s\n",inpstr);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"%s has set the topic to: %s\n",name,inpstr);
write_room_except(rm,text,user);
strcpy(rm->topic,inpstr);
if (rm->personal) {
        if (!(u=get_user(rm->owner))) return;
        strcpy(u->room_topic,inpstr);
        }
}


/*** Wizard moves a user to another room ***/
move(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;

if (word_count<2) {
        write_user(user,"Usage: move <user> [<room>]\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (word_count<3) rm=user->room;
else {
        if ((rm=get_room(word[2]))==NULL) {
                write_user(user,nosuchroom);  return;
                }
        }
if (user==u) {
        write_user(user,"Hey!  It's as simple as:  .go\n");  return;
        }
if (u->level>=user->level) {
        write_user(user,"You cannot move a user of equal or higher level than yourself.\n");
        return;
        }
if (rm==u->room) {
        sprintf(text,"%s is already in the %s.\n",u->name,rm->name);
        write_user(user,text);
        return;
        };
if (rm->personal) {
        write_user(user,"You cannot move a user into a personal room.\n");
        return;
        }
if (!has_room_access(user,rm)) {
        sprintf(text,"The %s is currently private, %s cannot be moved there.\n",rm->name,u->name);
        write_user(user,text);  
        return;
        }
write_user(user,"Okay.\n");
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"~OL%s%s %s\n",colors[CWARNING],name,tele_mv);
write_room_except(user->room,text,user);
move_user(u,rm,2);
prompt(u);
sprintf(text,"%s[ %s MOVED %s to the %s. ]\n",colors[CSYSTEM],user->name,u->name,u->room->name);
write_duty(user->level,text,NULL,NULL,0);
}


/*** Broadcast an important message ***/
bcast(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
if (word_count<2) {
        write_user(user,"Usage: bcast <message>\n");  return;
        }
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  
        return;
        }
force_listen=1;
if (user->vis) 
        sprintf(text,"\07\n%s%s-=- BROADCAST MESSAGE from %s -=-\n%s\n\n",colors[CPEOPLEHI],colors[CSYSBOLD],user->name,inpstr);
else sprintf(text,"\07\n%s%s-=- BROADCAST MESSAGE -=-\n%s\n\n",colors[CPEOPLEHI],colors[CSYSBOLD],inpstr);
write_room(NULL,text);
for (u=user_first; u!=NULL; u=u->next) if (u->pueblo && !audioprompt(u,2,1)) write_user(u,"</xch_mudtext><img xch_alert><xch_mudtext>");
}

/*** Show who is on ***/
who(user,people)
UR_OBJECT user;
int people;
{
UR_OBJECT u;
int cnt,total,invis,mins,remote,idle,logins,wholen;
char line[(USER_NAME_LEN+USER_DESC_LEN*2)+110];
char rname[ROOM_NAME_LEN+1],portstr[5],idlestr[6],sockstr[3];
char plural1[2][4]={"are","is"},plural2[2][2]={"s",""};
total=0;  invis=0;  remote=0;  logins=0;

sprintf(text,"\n%s~OL%s-=-   Users at %s %s   -=-\n\n",colors[CPEOPLEHI],colors[CTEXT],reg_sysinfo[TALKERNAME],long_date(1));
write_user(user,text);
if (people) write_user(user,"~FTName         : Level      Line Ign Vis Idle  Mins Port Site/Service\n\r");
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE) continue;
        mins=(int)(time(0) - u->last_login)/60;
        idle=(int)(time(0) - u->last_input)/60;
        if (u->type==BOT_TYPE) strcpy(portstr,"Sys ");
        else if (u->type==REMOTE_TYPE) strcpy(portstr,"Link");
        else {
                if (u->port==port[0]) strcpy(portstr,"Main");
                else strcpy(portstr,"Wiz ");
                }
        if (u->type!=USER_TYPE && u->type!=REMOTE_TYPE) strcpy(portstr,"Sys ");
        if (u->login) {
                if (!people) continue;
                sprintf(text,"~FY[ Login %d ]  : -           %2d   -   -  %4d     - %s %s:%d\n",4 - u->login,u->socket,idle,portstr,u->site,u->site_port);
                write_user(user,text);
                logins++;
                continue;
                }
        if (u->type==REMOTE_TYPE) ++remote;
        if (!u->vis) { 
                if (u->level>user->level) continue;  
                ++invis;
                }
        else ++total;
        if (people) {
                sprintf(idlestr,"%4d",idle);
                if (u->type==REMOTE_TYPE || u->type==BOT_TYPE) strcpy(sockstr," -");
                else sprintf(sockstr,"%2d",u->socket);
                sprintf(text,"%s%-12s : %-10s  %s  %s %s %s %5d %s %s\n",colors[CPEOPLE],u->name,level_name[u->cloaklev],sockstr,noyes1[u->ignall],noyes1[u->vis],idlestr,mins,portstr,u->site);
                write_user(user,text);
                continue;
                }

        /* Pueblo enhanced to examine users by clicking their name */
        if (user->pueblo && !user->login)
                sprintf(line,"   </xch_mudtext><b><a xch_cmd=\".examine %s\" xch_hint=\"Examine this user.\">[]</a></b><xch_mudtext> %s%s %s~RS",u->name,colors[CWHOUSER],u->name,u->desc);
           else sprintf(line,"   %s%s %s~RS",colors[CWHOUSER],u->name,u->desc);

        /* set a length var if user has pueblo so the alignment is okay. */
        wholen = 152 - ( 12 - strlen(u->name) );

        if (u->ignshout) line[0]='!';                 /* Ign: Shout         */
        if (u->igntell) line[0]='>';                  /* Ign: Tells         */
        if (u->igntell && u->ignshout) line[0]='&';   /* Ign: Shout & Tells */
        if (u->ignall) line[0]='x';                   /* Editing / Ign: All */
        if (u->cloaklev!=u->level && u->level <= user->level) line[1]='c';  /* cloaked */
        if (!u->vis) line[1]='*';                     /* hidden             */
        if (u->type==REMOTE_TYPE) line[2]='@';
        if (u->room==NULL) sprintf(rname,"@%s",u->netlink->service);
        else strcpy(rname,u->room->name);
        if (u->room!=NULL && u->room->personal) strcpy(rname,u->room->owner);
        /* Count number of colour coms to be taken account of when formatting */
        cnt=colour_com_count(line);
        if (!user->pueblo || user->login) sprintf(text,"~FR%-*s %s:%-20s:%d m",46+(cnt*3),line,colors[CWHOINFO],rname,mins);
        else sprintf(text,"~FR%-*s %s:%-20s:%d m",wholen+(cnt*3),line,colors[CWHOINFO],rname,mins);
        if (u->afk) strcat(text," ~FRAFK\n"); else strcat(text,"\n");
        write_user(user,text);
        }
sprintf(text,"\n~FGThere %s %d visible user%s. (%d remote user%s.)\n",plural1[(total==1)],total,plural2[(total==1)],remote,plural2[(remote==1)]);
write_user(user,text);
if (user->level >= WIZ) {
        sprintf(text,"~FG%d user%s %s currently hidden",invis,plural2[(invis==1)],plural1[(invis==1)]);
        write_user(user,text);
        if (people) sprintf(text,"~FG and %d connection%s %s waiting at login.\n\n",logins,plural2[(logins==1)],plural1[(logins==1)]);
                else sprintf(text,"~FG.\n\n");
        write_user(user,text);
        }
else write_user(user,"\n");
}


/*** Do the help ***/
help(user)
UR_OBJECT user;
{
CM_OBJECT cmd;
int ret,com,validcom;
char filename[80];
char *c;
validcom=0;

if (word_count<2) {
        sprintf(filename,"%s/mainhelp",HELPFILES);
        if (!(ret=more(user,user->socket,filename))) {
                write_user(user,"There is no main help at the moment.\n");
                return;
                }
        if (ret==1) user->misc_op=2;
        return;
        }
if (!strcmp(word[1],"commands")) {  help_commands(user);  return;  }
if (!strcmp(word[1],"credits")) {  help_credits(user);  return;  }
if (!strcmp(word[1],"rules"))   { help_rules(user);  return; }
if (!strcmp(word[1],"ranks"))   { help_ranks(user);  return; }

/* Check for any illegal crap in searched for filename so they cannot list 
   out the /etc/passwd file for instance. */
c=word[1];
while(*c) {
        if (*c=='.' || *c++=='/') {
                write_user(user,"-=- Invalid topic name! -=-\n");
                sprintf(text,"%s[ %s requested a RESTRICTED FILENAME: %s ]\n",colors[CSYSTEM],user->name,word[1]);
                write_duty(WIZ,text,NULL,NULL,0);
                return;
                }
        }
com=0;
while(command[com][0]!='*') {
        if (!strcmp(word[1],command[com])) {
                if (com_level[com] > user->level) { write_user(user,"-=- Help topic not found. -=-\n"); return; }
                validcom=1;
                break;
                }
        com++;
        }
if (!validcom) {
        com=0;
        while(toscom[com][0]!='*') {
                if (!strcmp(word[1],toscom[com])) {
                        if (toscom_level[com] > user->level) { write_user(user,"-=- Help topic not found. -=-\n"); return; }
                        validcom=2;
                        break;
                        }
                com++;
                }
        }
if (!validcom) {
        for (cmd=cmds_first; cmd!=NULL; cmd=cmd->next) {
                if (!strcmp(word[1],cmd->command)) {
                        if (cmd->req_lev > user->level) { write_user(user,"-=- Help topic not found. -=-\n"); return; }
                        validcom=3;
                        break;
                        }
                }
        }
if (!validcom) {
        sprintf(filename,"%s/%s",HELPFILES,word[1]);
        if (!(ret=more(user,user->socket,filename)))
                write_user(user,"-=- Help topic not found. -=-\n");
        if (ret==1) user->misc_op=2;
        }
else {
        if (validcom==1) sprintf(text,"\nCommand: ~FT%-12s            ~RSMinimum Level: ~FT%-10s\n\n",command[com],level_name[com_level[com]]);
                else if (validcom==2) sprintf(text,"\nCommand: ~FT%-12s            ~RSMinimum Level: ~FT%-10s\n\n",toscom[com],level_name[toscom_level[com]]);
                        else if (validcom==3) sprintf(text,"\nCommand: ~FT%-12s            ~RSMinimum Level: ~FT%-10s\n\n",cmd->command,level_name[cmd->req_lev]);
        write_user(user,text);
        sprintf(filename,"%s/%s",HELPFILES,word[1]);
        if (!(ret=more(user,user->socket,filename)))
                write_user(user,"-=- Command helpfile not available. -=-\n");
        if (ret==1) user->misc_op=2;
        }
}

help_rules(user)
UR_OBJECT user;
{
int ret;
char filename[80];
ret=0;
sprintf(filename,"rulefile");
sprintf(text,"%s[ %s is reading the RULES ]\n",colors[CSYSTEM],user->name);
if (user->level==NEW) write_duty(WIZ,text,NULL,NULL,0);
if (!(ret=more(user,user->socket,filename)))
        write_user(user,"-=- Rules: File Not Found -=-\n");
if (ret==1) user->misc_op=2;
user->readrules=1;
}

help_ranks(user)
UR_OBJECT user;
{
int lev,usr,adm,typ,tot;

sprintf(text,"\n%s%s-=- Rank Heirarchy at %s -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],reg_sysinfo[TALKERNAME]);
write_user(user,text);
lev=0; usr=0; adm=0; typ=0; tot=0;
for (lev=NEW; level_name[lev][0]!='*'; lev++) {
        if (lev==NEW) write_user(user,"Level : Type  : Name       : Requirements           : Description\n\n");
        if (lev > XUSR) { adm++; typ=1; tot++; }
                  else  { usr++; typ=0; tot++; }
        sprintf(text," %2d   : %-5s : %-10s : %-22s : %s\n",lev,lev_type[typ],level_name[lev],lev_req[lev],lev_desc[lev]);
        write_user(user,text);
        }
sprintf(text,"\nThere are %d user levels and %d administration levels.\n",usr,adm);
write_user(user,text);
sprintf(text,"Total of %d levels available.\n\n",tot);
write_user(user,text);
}


/*** Show the command available ***/

/*  We can now specify what commands to show.  You can specify a starting
    AND an ending level and the level range (inclusive) will be displayed.
    The word "only" can be used as the ending level and ONLY the level given
    for the start will be displayed.  Leaving out the end level will cause
    the commands to be displayed from the starting level up to (inclusive)
    the user's level.  If both are omited, all commands that are available
    to the user are displayed.  The starting and ending levels must be
    within the user's level.  Unfortunately this is an un-noticed feature.
    I have written a helpfile for it, but since .help commands pulls up
    the command list, you can't see it.  So you will have to tell the users. */

help_commands(user)
UR_OBJECT user;
{
PL_OBJECT plugin;
CM_OBJECT plcom;
int com,cnt,lev,total,levtotal,getlev,tolev;
char temp[20];

if (!strcmp(word[1],"commands")) {
        if (word_count<3 || user->type==REMOTE_TYPE) { getlev=0; tolev=user->level; }
        else {
                if ((getlev=get_level(word[2]))==-1) { write_user(user,"-=- Invalid starting level name. -=-\n"); return; }
                if (getlev > user->level) { write_user(user,"You may not display commands for a level greater than your own.\n"); return; }
                }        
        if (!strcmp(word[3],"only")) tolev=getlev;
        else if (word[3][0]) {
                if ((tolev=get_level(word[3]))==-1) { write_user(user,"-=- Invalid ending level name. -=-\n"); return; }
                if (tolev > user->level) { write_user(user,"You may not display commands for a level greater than your own.\n"); return; }
                }
                else tolev=user->level;
        if (tolev < getlev) { write_user(user,"ERROR:  Starting level MUST come BEFORE ending level!\n"); return; }
        }
else {
        if (word_count<2 || user->type==REMOTE_TYPE) { getlev=0; tolev=user->level; }
        else {
                if ((getlev=get_level(word[1]))==-1) { write_user(user,"-=- Invalid level name. -=-\n"); return; }
                if (getlev > user->level) { write_user(user,"You may not display commands for a level greater than your own.\n"); return; }
                }        
        if (!strcmp(word[2],"only")) tolev=getlev;
        else if (word[2][0]) {
                if ((tolev=get_level(word[2]))==-1) { write_user(user,"-=- Invalid ending level name. -=-\n"); return; }
                if (tolev > user->level) { write_user(user,"You may not display commands for a level greater than your own.\n"); return; }
                }
                else tolev=user->level;
        if (tolev < getlev) { write_user(user,"ERROR:  Starting level MUST come BEFORE ending level!\n"); return; }
        }
if ((getlev==0 && tolev==user->level) || getlev==tolev) sprintf(text,"\n%s%s-=- Commands available for level: ~OL%s~RS%s%s -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],level_name[tolev],colors[CHIGHLIGHT],colors[CTEXT]);
else sprintf(text,"\n%s%s-=- Commands available for levels: ~OL%s to %s~RS%s%s -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],level_name[getlev],level_name[tolev],colors[CHIGHLIGHT],colors[CTEXT]);
write_user(user,text);
total=0; levtotal=0;
for(lev=getlev;lev<=tolev;++lev) {
        com=0; levtotal=0;
        while(command[com][0]!='*') {
                if (com_level[com]==lev) levtotal++;
                com++;
                }
        com=0;
        while(toscom[com][0]!='*') {
                if (toscom_level[com]==lev) levtotal++;
                com++;
                }
        for(plcom=cmds_first; plcom!=NULL; plcom=plcom->next) if (plcom->req_lev==lev) levtotal++;
        sprintf(text,"~FT[%-10s : %3d]\n",level_name[lev],levtotal);
        write_user(user,text);
        com=0;  cnt=0;  text[0]='\0';
        while(command[com][0]!='*') {
                if (com_level[com]!=lev) {  com++;  continue;  }
                sprintf(temp,"%-10s ",command[com]);
                strcat(text,temp);
                if (cnt==6) {  
                        strcat(text,"\n");  
                        write_user(user,text);  
                        text[0]='\0';  cnt=-1;  
                        }
                com++; cnt++; total++;
                }
        com=0;
        while(toscom[com][0]!='*') {
                if (toscom_level[com]!=lev) {  com++;  continue;  }
                sprintf(temp,"%-10s ",toscom[com]);
                strcat(text,temp);
                if (cnt==6) {  
                        strcat(text,"\n");  
                        write_user(user,text);  
                        text[0]='\0';  cnt=-1;  
                        }
                com++; cnt++; total++;
                }
        for(plcom=cmds_first; plcom!=NULL; plcom=plcom->next) {
                if (plcom->req_lev!=lev) continue;
                sprintf(temp,"%-10s ",plcom->command);
                strcat(text,temp);
                if (cnt==6) {  
                        strcat(text,"\n");  
                        write_user(user,text);  
                        text[0]='\0';  cnt=-1;  
                        }
                cnt++; total++;
                }
        if (cnt) {
                strcat(text,"\n");  write_user(user,text);
                }
        }
if ((getlev==0  && tolev==user->level) || getlev==tolev) sprintf(text,"\nTotal of ~OL~FT%d~RS commands available.\n",total);
else sprintf(text,"\nTotal of ~OL~FT%d~RS commands available for levels %s to %s.\n",total,level_name[getlev],level_name[tolev]);
write_user(user,text);
write_user(user,"Type '~FG.help <command name>~RS' for specific help on a command.\nRemember, you can use a '.' on its own to repeat your last command or speech.\n\n");
}

help_credits(user)
UR_OBJECT user;
{
if (!strcmp(word[2],"nuts")) { help_credits_neil(user); return; }
if (!strcmp(word[2],"tos"))  { help_credits_tos(user);  return; }
write_user(user,"Use ~FG.help credits nuts~RS for the NUTS credits by Neil Robertson.\n");
write_user(user,"Use ~FG.help credits tos~RS  for the TalkerOS credits by William Price.\n\n");
your_credits_here(user);  /* don't really put your credits here,
                             they go a few lines down.  */
}

help_credits_neil(user)
UR_OBJECT user;
{
sprintf(text,"\n~BRNUTS version %s, Copyright (C) Neil Robertson 1996.\n\n",VERSION);
write_user(user,text);
write_user(user,"NUTS stands for Neils Unix Talk Server, a program which started out as a\nuniversity project in autumn 1992 and has progressed from thereon. In no\nparticular order thanks go to the following people who helped me develop or\n");
write_user(user,"debug this code in one way or another over the years:\n   ~FTDarren Seryck, Steve Guest, Dave Temple, Satish Bedi, Tim Bernhardt,\n   ~FTKien Tran, Jesse Walton, Pak Chan, Scott MacKenzie and Bryan McPhail.\n"); 
write_user(user,"Also thanks must go to anyone else who has emailed me with ideas and/or bug\nreports and all the people who have used NUTS over the intervening years.\n");
write_user(user,"I know I've said this before but this time I really mean it - this is the final\nversion of NUTS 3. In a few years NUTS 4 may spring forth but in the meantime\nthat, as they say, is that. :)\n\n");
write_user(user,"If you wish to email me my address is '~FGneil@ogham.demon.co.uk~RS' and should\nremain so for the forseeable future.\n\nNeil Robertson - November 1996.\n\n");
}

help_credits_tos(user)
UR_OBJECT user;
{
   sprintf(text,"\n~BW~FB~OL  ~FW~BB TalkerOS version %s ~BW~FB -  (c)1998 by William Price.  All rights reserved.   \n\n",TOS_VER);
write_user(user,text);
write_user(user," ~FYTalker~FMOS~RS is a heavily modified version of the NUTS talker by Neil Robertson.\n");
write_user(user," Its many features, pueblo enhancements, tools, and overall appearance would\n");
write_user(user," not be possible without the help of many, many people in its initial develop-\n");
write_user(user," ment.  These people helped debug, or provided services during the creation of\n");
write_user(user," this wonderful code.  Without them, this would have never been possible.  To\n");
write_user(user," all of these people:  You have my sincere thanks.         - William\n\n");
write_user(user,"~FT          'Garth'~RS - ~FGInitial development environment on his personal UNIX box.\n");
write_user(user,"~FT           'Madd'~RS - ~FGProvided the account on the first server.\n");
write_user(user,"~FT        'Whizkid'~RS - ~FGFor the ideas that spawned many others, even though he\n");
write_user(user,"                    ~FGdid not realize his involvment.\n");
write_user(user,"\n Also: ~FTCyber, Steph, Galadriel, Houy, Jed, MacDonald, Kindrea, & Stones\n\n");
write_user(user,"\n Last, but definately not least, the people who have been so kind to use and\n");
write_user(user," support ~FYTalker~FMOS~RS and make it one of the best talker codes available.\n\n");
}

your_credits_here(user)
UR_OBJECT user;
{
/* Put your own credits here using the write_user() statement.
   Delete the help_credits_tos() line when you replace with your own. */
help_credits_tos(user);
}

/*** Read the message board ***/
read_board(user,boardtype)
UR_OBJECT user;
int boardtype;
{
RM_OBJECT rm;
char filename[80],*name;
char specialboards[2][15]={"Administration","Suggestion"};
int ret;

if (word_count<2 || boardtype) rm=user->room;
else {
        if ((rm=get_room(word[1]))==NULL) {
                write_user(user,nosuchroom);  return;
                }
        if (!has_room_access(user,rm)) {
                write_user(user,"That room is currently private, you cannot read the board.\n");
                return;
                }
        }       
if (rm->personal && !boardtype) { write_user(user,"Personal rooms have no message boards.\n"); return; }
if (!boardtype) sprintf(text,"\n%s%s%s-=- The %s Message Board -=-\n\n",colors[CHIGHLIGHT],colors[CBOLD],colors[CTEXT],rm->name);
        else sprintf(text,"\n%s%s%s-=- The %s Board -=-\n\n",colors[CHIGHLIGHT],colors[CBOLD],colors[CTEXT],specialboards[boardtype-1]);
write_user(user,text);
if (!boardtype) sprintf(filename,"%s/%s.B",DATAFILES,rm->name);
        else if (boardtype==1) sprintf(filename,"%s/%s.SysBoard",DATAFILES,WIZBOARD);
                else if (boardtype==2) sprintf(filename,"%s/%s.SysBoard",DATAFILES,SUGBOARD);
                                        else { sprintf(text,"ERROR:  System recieved unknown messageboard type!\n        Please contact an administrator.\n");
                                               write_user(user,text);
                                               sprintf(text,"READ_BOARD:  System recieved board type \"%d\", which is unknown.\n",boardtype);
                                               write_syslog(text,1,SYSLOG);
                                               return;
                                               }
if (!(ret=more(user,user->socket,filename))) 
        write_user(user,"~FT( There are no messages on the board. )\n\n");
else if (ret==1) user->misc_op=2;
if (boardtype) return;
if (user->vis) name=user->name; else name=invisname;
if (rm==user->room) {
        sprintf(text,"%s reads the message board.\n",name);
        write_room_except(user->room,text,user);
        }
}


/*** Write on the message board ***/
write_board(user,inpstr,done_editing,boardtype)
UR_OBJECT user;
char *inpstr;
int done_editing,boardtype;
{
FILE *fp;
int cnt,inp;
char *ptr,*name,filename[80];
char specialboards[2][15]={"Administration","Suggestion"};

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  
        return;
        }
if (user->room->personal && !boardtype) {
        write_user(user,"Personal rooms do not have message boards.\n");
        return;
        }
if (!done_editing) {
        if (word_count<2) {
                if (user->type==REMOTE_TYPE) {
                        /* Editor won't work over netlink cos all the prompts will go
                           wrong, I'll address this in a later version. */
                        write_user(user,"Sorry, due to software limitations remote users cannot use the line editor.\nUse the '.write <mesg>' method instead.\n");
                        return;
                        }
                if (!boardtype) sprintf(text,"\n%s%s-=- Writing Board Message -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
                        else sprintf(text,"\n%s%s-=- Writing %s Board Message -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],specialboards[boardtype-1]);
                write_user(user,text);
                if (!boardtype) user->misc_op=3;
                        else if (boardtype==1) user->misc_op=11;
                                else if (boardtype==2) user->misc_op=12;
                                        else { sprintf(text,"ERROR:  System recieved unknown messageboard type!\n        Please contact an administrator.\n");
                                               write_user(user,text);
                                               sprintf(text,"WRITE_BOARD:  System recieved board type \"%d\", which is unknown.\n",boardtype);
                                               write_syslog(text,1,SYSLOG);
                                               return;
                                               }
                editor(user,NULL);
                return;
                }
        ptr=inpstr;
        inp=1;
        }
else {
        ptr=user->malloc_start;  inp=0;
        }

if (!boardtype) sprintf(filename,"%s/%s.B",DATAFILES,user->room->name);
        else if (boardtype==1) sprintf(filename,"%s/%s.SysBoard",DATAFILES,WIZBOARD);
                else if (boardtype==2) sprintf(filename,"%s/%s.SysBoard",DATAFILES,SUGBOARD);
if (!(fp=fopen(filename,"a"))) {
        sprintf(text,"%s: cannot write to file.\n",syserror);
        write_user(user,text);
        sprintf(text,"ERROR: Couldn't open file %s to append in write_board().\n",filename);
        write_syslog(text,0,SYSLOG);
        return;
        }
if (user->vis) name=user->name; else name=invisname;
/* The posting time (PT) is the time its written in machine readable form, this 
   makes it easy for this program to check the age of each message and delete 
   as appropriate in check_messages() */
if (user->type==REMOTE_TYPE) 
        sprintf(text,"PT: %d\r%sFrom: %s@%s  ~RS%s%s\n",(int)(time(0)),colors[CBOARDHEAD],name,user->netlink->service,colors[CBOARDDATE],long_date(0));
else sprintf(text,"PT: %d\r%sFrom: %s  ~RS%s%s\n",(int)(time(0)),colors[CBOARDHEAD],name,colors[CBOARDDATE],long_date(0));
fputs(text,fp);
cnt=0;
while(*ptr!='\0') {
        putc(*ptr,fp);
        if (*ptr=='\n') cnt=0; else ++cnt;
        if (cnt==80) { putc('\n',fp); cnt=0; }
        ++ptr;
        }
if (inp) fputs("\n\n",fp); else putc('\n',fp);
fclose(fp);
if (!boardtype) {
        write_user(user,"You write the message on the board.\n");
        sprintf(text,"%s writes a message on the board.\n",name);
        write_room_except(user->room,text,user);
        user->room->mesg_cnt++;
        }
else {
        if (boardtype==1) {
                sprintf(text,"%s[ %s has added a message to the Administration Board. ]\n",colors[CSYSTEM],user->name);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                sprintf(text,"%s[ Administration message added. ]\n",colors[CSYSTEM]);
                write_user(user,text);
                }
        if (boardtype==2) {
                sprintf(text,"%s[ %s has added a message to the Suggestion Board. ]\n",colors[CSYSTEM],user->name);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                write_user(user,"Your suggestion has been added.\n");
                }
        }                

}



/*** Wipe some messages off the board ***/
wipe_board(user,boardtype)
UR_OBJECT user;

{
int num,cnt,valid;
char infile[80],line[82],id[82],*name;
FILE *infp,*outfp;
RM_OBJECT rm;

if (word_count<2 || ((num=atoi(word[1]))<1 && strcmp(word[1],"all"))) {
        write_user(user,"Usage: wipe <number of messages>/all\n");  return;
        }
rm=user->room;
if (rm->personal && !boardtype) {
        write_user(user,"Personal rooms do not have message boards.\n");
        return;
        }
if (user->vis) name=user->name; else name=invisname;
if (!boardtype) sprintf(infile,"%s/%s.B",DATAFILES,rm->name);
        else if (boardtype==1) sprintf(infile,"%s/%s.SysBoard",DATAFILES,WIZBOARD);
                else if (boardtype==2) sprintf(infile,"%s/%s.SysBoard",DATAFILES,SUGBOARD);
                                        else { sprintf(text,"ERROR:  System recieved unknown messageboard type!\n        Please contact an administrator.\n");
                                               write_user(user,text);
                                               sprintf(text,"WIPE_BOARD:  System recieved board type \"%d\", which is unknown.\n",boardtype);
                                               write_syslog(text,1,SYSLOG);
                                               return;
                                               }
if (!(infp=fopen(infile,"r"))) {
        write_user(user,"The message board is empty.\n");
        return;
        }
if (!strcmp(word[1],"all")) {
        fclose(infp);
        unlink(infile);
        if (!boardtype) {
                write_user(user,"All messages deleted.\n");
                sprintf(text,"%s wipes the message board.\n",name);
                write_room_except(rm,text,user);
                sprintf(text,"%s wiped all messages from the board in the %s.\n",user->name,rm->name);
                write_syslog(text,1,USERLOG);
                rm->mesg_cnt=0;
                }
        if (boardtype==1) {
                write_user(user,"All administration messages deleted.\n");
                sprintf(text,"%s[ %s has wiped the Administration Board. ]\n",colors[CSYSTEM],user->name);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                sprintf(text,"%s wiped all messages from the Administration Board.\n",user->name);
                write_syslog(text,1,USERLOG);
                }
        if (boardtype==2) {
                write_user(user,"All suggestions deleted.\n");
                sprintf(text,"%s[ %s has wiped the Suggestion Board. ]\n",colors[CSYSTEM],user->name);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                sprintf(text,"%s wiped all messages from the Suggestion Board.\n",user->name);
                write_syslog(text,1,USERLOG);
                }
        return;
        }
if (!(outfp=fopen("tempfile","w"))) {
        sprintf(text,"%s: couldn't open tempfile.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open tempfile in wipe_board().\n",0,SYSLOG);
        fclose(infp);
        return;
        }
cnt=0; valid=1;
fgets(line,82,infp); /* max of 80+newline+terminator = 82 */
while(!feof(infp)) {
        if (cnt<=num) {
                if (*line=='\n') valid=1;
                sscanf(line,"%s",id);
                if (valid && !strcmp(id,"PT:")) {
                        if (++cnt>num) fputs(line,outfp);
                        valid=0;
                        }
                }
        else fputs(line,outfp);
        fgets(line,82,infp);
        }
fclose(infp);
fclose(outfp);
unlink(infile);
if (cnt<num) {
        unlink("tempfile");
        if (!boardtype) {
                sprintf(text,"There were only %d messages on the board, all now deleted.\n",cnt);
                write_user(user,text);
                sprintf(text,"%s wipes the message board.\n",name);
                write_room_except(rm,text,user);
                sprintf(text,"%s wiped all messages from the board in the %s.\n",user->name,rm->name);
                write_syslog(text,1,USERLOG);
                rm->mesg_cnt=0;
                }
        if (boardtype==1) {
                sprintf(text,"There were only %d messages on the Administration Board - All deleted.\n",cnt);
                write_user(user,text);
                sprintf(text,"%s[ %s has wiped the Administration Board. ]\n",colors[CSYSTEM],user->name);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                sprintf(text,"%s wiped all messages from the Administration Board.\n",user->name);
                write_syslog(text,1,USERLOG);
                }
        if (boardtype==2) {
                sprintf(text,"There were only %d messages on the Suggestion Board - All deleted.\n",cnt);
                write_user(user,text);
                sprintf(text,"%s[ %s has wiped the Suggestion Board. ]\n",colors[CSYSTEM],user->name);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                sprintf(text,"%s wiped all messages from the Suggestion Board.\n",user->name);
                write_syslog(text,1,USERLOG);
                }
        return;
        }
if (cnt==num) {
        unlink("tempfile"); /* cos it'll be empty anyway */
        if (!boardtype) {
                write_user(user,"All messages deleted.\n");
                user->room->mesg_cnt=0;
                sprintf(text,"%s wiped all messages from the board in the %s.\n",user->name,rm->name);
                }
        if (boardtype==1) {
                write_user(user,"All administration messages deleted.\n");
                sprintf(text,"%s[ %s has wiped the Administration Board. ]\n",colors[CSYSTEM],user->name);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                sprintf(text,"%s wiped all messages from the Administration Board.\n",user->name);
                }
        if (boardtype==2) {
                write_user(user,"All suggestions deleted.\n");
                sprintf(text,"%s[ %s has wiped the Suggestion Board. ]\n",colors[CSYSTEM],user->name);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                sprintf(text,"%s wiped all messages from the Suggestion Board.\n",user->name);
                }
        }
else {
        rename("tempfile",infile);
        if (!boardtype) {
                sprintf(text,"%d messages deleted.\n",num);
                write_user(user,text);
                user->room->mesg_cnt-=num;
                sprintf(text,"%s wiped %d messages from the board in the %s.\n",user->name,num,rm->name);
                }
        if (boardtype==1) {
                sprintf(text,"You have wiped %d messages from the Administration Board.\n",num);
                write_user(user,text);
                sprintf(text,"%s[ %s wiped %d messages from the Administration Board. ]\n",colors[CSYSTEM],user->name,num);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                sprintf(text,"%s wiped %d messages from the Administration Board.\n",user->name,num);
                }
        if (boardtype==2) {
                sprintf(text,"You have wiped %d messages from the Suggestion Board.\n",num);
                write_user(user,text);
                sprintf(text,"%s[ %s wiped %d messages from the Suggestion Board. ]\n",colors[CSYSTEM],user->name,num);
                if (user->vis) write_duty(WIZ,text,NULL,user);
                        else write_duty(user->level,text,NULL,user);
                sprintf(text,"%s wiped %d messages from the Suggestion Board.\n",user->name,num);
                }
        }
write_syslog(text,1,USERLOG);
if (!boardtype) { sprintf(text,"%s wipes the message board.\n",name);
                  write_room_except(rm,text,user);
                  }
}

        

/*** Search all the boards for the words given in the list. Rooms fixed to
        private will be ignore if the users level is less than gatecrash_level ***/
search_boards(user)
UR_OBJECT user;
{
RM_OBJECT rm;
FILE *fp;
char filename[80],line[82],buff[(MAX_LINES+1)*82],w1[81];
int w,cnt,message,yes,room_given;

if (word_count<2) {
        write_user(user,"Usage: search <word list>\n");  return;
        }
/* Go through rooms */
cnt=0;
for(rm=room_first;rm!=NULL;rm=rm->next) {
        sprintf(filename,"%s/%s.B",DATAFILES,rm->name);
        if (!(fp=fopen(filename,"r"))) continue;
        if (!has_room_access(user,rm)) {  fclose(fp);  continue;  }

        /* Go through file */
        fgets(line,81,fp);
        yes=0;  message=0;  
        room_given=0;  buff[0]='\0';
        while(!feof(fp)) {
                if (*line=='\n') {
                        if (yes) {  strcat(buff,"\n");  write_user(user,buff);  }
                        message=0;  yes=0;  buff[0]='\0';
                        }
                if (!message) {
                        w1[0]='\0';  
                        sscanf(line,"%s",w1);
                        if (!strcmp(w1,"PT:")) {  
                                message=1;  
                                strcpy(buff,remove_first(remove_first(line)));
                                }
                        }
                else strcat(buff,line);
                for(w=1;w<word_count;++w) {
                        if (!yes && strstr(line,word[w])) {  
                                if (!room_given) {
                                        sprintf(text,"~BB*** %s ***\n\n",rm->name);
                                        write_user(user,text);
                                        room_given=1;
                                        }
                                yes=1;  cnt++;  
                                }
                        }
                fgets(line,81,fp);
                }
        if (yes) {  strcat(buff,"\n");  write_user(user,buff);  }
        fclose(fp);
        }
if (cnt) {
        sprintf(text,"Total of %d matching messages.\n\n",cnt);
        write_user(user,text);
        }
else write_user(user,"No occurences found.\n");
}



/*** See review of conversation ***/
review(user)
UR_OBJECT user;
{
RM_OBJECT rm=user->room;
int i,line,cnt;

if (word_count<2) rm=user->room;
else {
        if ((rm=get_room(word[1]))==NULL) {
                write_user(user,nosuchroom);  return;
                }
        if (!has_room_access(user,rm)) {
                write_user(user,"That room is currently private, you cannot review the conversation.\n");
                return;
                }
        }
cnt=0;
for(i=0;i<REVIEW_LINES;++i) {
        line=(rm->revline+i)%REVIEW_LINES;
        if (rm->revbuff[line][0]) {
                cnt++;
                if (cnt==1) {
                        if (!rm->personal) sprintf(text,"\n~BB~FG-=- Review buffer for the %s -=-\n\n",rm->name);
                        else sprintf(text,"\n~BB~FG-=- Review buffer for %s's personal room -=-\n\n",rm->owner);
                        write_user(user,text);
                        }
                write_user(user,rm->revbuff[line]); 
                }
        }
if (!cnt) write_user(user,"Review buffer is empty.\n");
else write_user(user,"\n~BB~FG-=- End -=-\n\n");
}


/*** Return to home site ***/
home(user)
UR_OBJECT user;
{
if (user->room!=NULL) {
        write_user(user,"You are already on your home system.\n");
        return;
        }
sprintf(text,"~OL%sYou traverse cyberspace...\n",colors[CSELF]);
write_user(user,text);
sprintf(text,"REL %s\n",user->name);
write_sock(user->netlink->socket,text);
sprintf(text,"NETLINK: %s returned from %s.\n",user->name,user->netlink->service);
write_syslog(text,1,USERLOG);
user->room=user->netlink->connect_room;
user->netlink=NULL;
if (user->vis) {
        sprintf(text,"%s%s %s\n",colors[CWHOUSER],user->name,user->in_phrase);
        write_room_except(user->room,text,user);
        }
else write_room_except(user->room,invisenter,user);
look(user);
}


/*** Show some user stats ***/
status(user)
UR_OBJECT user;
{
UR_OBJECT u;
FILE *fp;
int new_mail,days,hours,mins,days2,hours2,mins2,timelen;
int len,i,a,b,c,d,e,tmp;
char filename[80],line[82];
char *str,*colour_com_strip();
char ir[ROOM_NAME_LEN+1],tempstr[USER_NAME_LEN+USER_DESC_LEN+20];
char plural[2][2]={" ","s"};

if (word_count<2) {
        strcpy(word[1],user->name);
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
/* Fool a reg. user if a wiz is invis... Hey!  That rhymes! */
if (!u->vis && user->level < u->level) {
        write_user(user,notloggedon);
        return;
        }
sprintf(filename,"%s/%s.M",USERFILES,u->name);
if (!(fp=fopen(filename,"r"))) new_mail=0;
else {
        fscanf(fp,"%d",&new_mail);
        fclose(fp);
        }
days=u->total_login/86400;
hours=(u->total_login%86400)/3600;
mins=(u->total_login%3600)/60;
timelen=(int)(time(0) - u->last_login);
days2=timelen/86400;
hours2=(timelen%86400)/3600;
mins2=(timelen%3600)/60;

/* Write Header (72 char)*/
write_user(user,".--- ");
sprintf(text,"%s%s %s~RS ",colors[CWHOUSER],u->name,u->desc);
write_user(user,text);
if (user->level < u->level) tmp=(59-strlen(u->name)-strlen(colour_com_strip(u->desc))-strlen(level_name[u->cloaklev]));
                       else tmp=(59-strlen(u->name)-strlen(colour_com_strip(u->desc))-strlen(level_name[u->level]));
for (i=0; i<tmp; i++) {
        write_user(user,"-");
        }
if (user->level < u->level) sprintf(text," ~FT%s~RS ---.\n",level_name[u->cloaklev]);
                       else sprintf(text," ~FT%s~RS ---.\n",level_name[u->level]);
write_user(user,text);
/* Header Done */

if (u->reverse) a=1; else a=0;
if (u->muzzled) b=1; else b=0;
if (u->arrested) c=1; else c=0;
if (new_mail>u->read_mail) d=1; else d=0;
if (u->level != u->orig_level) e=1; else e=0;
if (u->invite_room==NULL) strcpy(ir,"<nowhere>");
else if (!u->invite_room->personal) strcpy(ir,u->invite_room->name);
        else strcpy(ir,u->invite_room->owner);
write_user(user,"|                                                                      |\n");
   sprintf(text,"|    Ignore ALL: ~OL%3s~RS       Ignore TELLS: ~OL%3s~RS    Ignore SHOUTS: ~OL%3s~RS     |\n",noyes2[u->ignall],noyes2[u->igntell],noyes2[u->ignshout]);
   write_user(user,text);
   sprintf(text,"|     Possessed: ~OL%3s~RS            Muzzled: ~OL%3s~RS         Arrested: ~OL%3s~RS     |\n",noyes2[a],noyes2[b],noyes2[c]);
   write_user(user,text);
   sprintf(text,"|  Times Warned: ~OL%-2d~RS     Has Unread Mail: ~OL%3s~RS    AwayFromKeybd: ~OL%3s~RS     |\n",u->alert,noyes2[d],noyes2[u->afk]);
   write_user(user,text);
   sprintf(text,"|  Pueblo Enh'd: ~OL%3s~RS      Audio Prompts: ~OL%3s~RS    Pager Enabled: ~OL%3s~RS     |\n",noyes2[u->pueblo],noyes2[(u->pueblo_mm && u->pueblo)],noyes2[(u->pueblo_pg && u->pueblo)]);
   write_user(user,text);
if (user->level >= WIZ && user->level >= u->level) {
   if (u->cloaklev != u->level) sprintf(text,"|    On WizDuty: ~OL%3s~RS        Cloak Level: ~OL%-10s~RS  Temp-Lev: ~OL%3s~RS     |\n",noyes2[u->duty],level_name[u->cloaklev],noyes2[e]);
                           else sprintf(text,"|    On WizDuty: ~OL%3s~RS        Cloak Level: ~OL(off)~RS       Temp-Lev: ~OL%3s~RS     |\n",noyes2[u->duty],noyes2[e]);
   write_user(user,text);
   }
write_user(user,"|                                                                      |\n");
if ( !u->pstats || user->level>XUSR || u==user) {
   sprintf(tempstr,"%s",u->email);
   tmp=(58 + (colour_com_count(tempstr) * 3));
   sprintf(text,"|   E-Mail: ~OL%-*s~RS |\n",tmp,u->email);
   write_user(user,text);
   sprintf(tempstr,"%s",u->http);
   tmp=(58 + (colour_com_count(tempstr) * 3));
   sprintf(text,"|  Website: ~OL%-*s~RS |\n",tmp,u->http);
   write_user(user,text);
        }
else {
write_user(user,"|   E-Mail: (Information locked-out to public.)                        |\n");
write_user(user,"|  Website: (Information locked-out to public.)                        |\n");
        }
write_user(user,"|                                                                      |\n");
   sprintf(tempstr,"%s",u->in_phrase);
   tmp=(40 + (colour_com_count(tempstr) * 3));
   sprintf(text,"|     In-Phrase: %s%-*s~RS              |\n",colors[CWHOUSER],tmp,u->in_phrase);
   write_user(user,text);
   sprintf(tempstr,"%s",u->out_phrase);
   tmp=(40 + (colour_com_count(tempstr) * 3));
   sprintf(text,"|    Out-Phrase: %s%-*s~RS              |\n",colors[CWHOUSER],tmp,u->out_phrase);
   write_user(user,text);
   sprintf(text,"|    Invited to: %-20s                                  |\n",ir);
   write_user(user,text);
if (u->waitfor[0]!='\0') {
   sprintf(text,"|   Waiting for: ~OL%-12s~RS                                          |\n",u->waitfor);
   write_user(user,text);
   }
if (u->alias[0]!='\0') {
   sprintf(text,"|      Nickname: ~FG%-12s~RS                                          |\n",u->alias);
   write_user(user,text);
   }
write_user(user,"|                                                                      |\n");
   sprintf(text,"| Connected continuously for: %3d hour%s, and %2d minute%s                |\n",(timelen/3600),plural[((timelen/3600)!=1)],mins2,plural[((mins2)!=1)]);
   write_user(user,text);
   sprintf(text,"|           Total login time: %3d day%s, %2d hour%s, and %2d minute%s       |\n",days,plural[((days)!=1)],hours,plural[((hours)!=1)],mins,plural[((mins)!=1)]);
   write_user(user,text);
write_user(user,"`----------------------------------------------------------------------'\n");
return;
}



/*** Read your mail ***/
rmail(user)
UR_OBJECT user;
{
FILE *infp,*outfp;
int ret;
char c,filename[80],line[DNL+1];

sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(infp=fopen(filename,"r"))) {
        write_user(user,"You have no mail.\n");  return;
        }
/* Update last read / new mail received time at head of file */
if (outfp=fopen("tempfile","w")) {
        fprintf(outfp,"%d\r",(int)(time(0)));
        /* skip first line of mail file */
        fgets(line,DNL,infp);

        /* Copy rest of file */
        c=getc(infp);
        while(!feof(infp)) {  putc(c,outfp);  c=getc(infp);  }

        fclose(outfp);
        rename("tempfile",filename);
        }
user->read_mail=time(0);
fclose(infp);
write_user(user,"\n~BB*** Your mail ***\n\n");
ret=more(user,user->socket,filename);
if (ret==1) user->misc_op=2;
}



/*** Send mail message ***/
smail(user,inpstr,done_editing,memo)
UR_OBJECT user;
char *inpstr;
int done_editing,memo;
{
UR_OBJECT u;
FILE *fp;
int remote,has_account;
char *c,filename[80];

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (done_editing) {
        send_mail(user,user->mail_to,user->malloc_start);
        user->mail_to[0]='\0';
        return;
        }
if (word_count<2 && !memo) {
        write_user(user,"Smail who?\n");  return;
        }
/* See if its to another site */
remote=0;
has_account=0;
c=word[1];
while(*c) {
        if (*c=='@') {  
                if (c==word[1]) {
                        write_user(user,"User's name missing before @ sign.\n");  
                        return;
                        }
                remote=1;  break;  
                }
        ++c;
        }
word[1][0]=toupper(word[1][0]);
if (memo) strcpy(word[1],user->name);
/* See if user exists */
if (!remote) {
        u=NULL;
        if (!(u=get_user(word[1]))) {
                sprintf(filename,"%s/%s.D",USERFILES,word[1]);
                if (!(fp=fopen(filename,"r"))) {
                        if (!memo) { write_user(user,nosuchuser);  return; }
                        if (memo) { write_user(user,"You must establish an account before you can send a message to yourself.\n"); return; }
                        }
                has_account=1;
                fclose(fp);
                }
        if (u==user && !memo) {
                write_user(user,"Don't waste a stamp!  (use .memo to mail yourself)\n");
                return;
                }
        if (u!=NULL) strcpy(word[1],u->name); 
        if (!has_account) {
                /* See if user has local account */
                sprintf(filename,"%s/%s.D",USERFILES,word[1]);
                if (!(fp=fopen(filename,"r"))) {
                        sprintf(text,"%s is a remote user and does not have a local account.\n",u->name);
                        write_user(user,text);  
                        return;
                        }
                fclose(fp);
                }
        }
if (word_count>2 || (word_count>1 && memo)) {
        /* One line mail */
        strcat(inpstr,"\n"); 
        if (!memo) send_mail(user,word[1],remove_first(inpstr));
        if  (memo) send_mail(user,word[1],inpstr);
        return;
        }
if (user->type==REMOTE_TYPE) {
        write_user(user,"Sorry, due to software limitations remote users cannot use the line editor.\nUse the '.smail <user> <mesg>' method instead.\n");
        return;
        }
if (!memo) { sprintf(text,"\n%s%s-=- Writing Mail Message to %s -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],word[1]); }
if (memo) { sprintf(text,"\n%s%s-=- Writing Memo -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]); }
write_user(user,text);
if (memo) user->misc_op=10;
else user->misc_op=4;
strcpy(user->mail_to,word[1]);
editor(user,NULL);
}


/*** Delete some or all of your mail. A problem here is once we have deleted
     some mail from the file do we mark the file as read? If not we could
     have a situation where the user deletes all his mail but still gets
     the YOU HAVE UNREAD MAIL message on logging on if the idiot forgot to 
     read it first. ***/
dmail(user)
UR_OBJECT user;
{
FILE *infp,*outfp;
int num,valid,cnt;
char filename[80],w1[ARR_SIZE],line[ARR_SIZE],line2[ARR_SIZE];

if (word_count<2 || ((num=atoi(word[1]))<1 && strcmp(word[1],"all"))) {
        write_user(user,"Usage: dmail <number of messages>/all\n");  return;
        }
sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(infp=fopen(filename,"r"))) {
        write_user(user,"You have no mail to delete.\n");  return;
        }
if (!strcmp(word[1],"all")) {
        fclose(infp);
        unlink(filename);
        write_user(user,"All mail deleted.\n");
        return;
        }
if (!(outfp=fopen("tempfile","w"))) {
        sprintf(text,"%s: couldn't open tempfile.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open tempfile in dmail().\n",0,SYSLOG);
        fclose(infp);
        return;
        }
fprintf(outfp,"%d\r",(int)time(0));
user->read_mail=time(0);
cnt=0;  valid=1;
fgets(line,DNL,infp); /* Get header date */
fgets(line,ARR_SIZE-1,infp);

/* An error developed when counting messages because it looked for "~OLFrom:"
   but with the modular colorcodes, it wouldn't always be "~OL".  */
sprintf(text,"%sFrom:",colors[CMAILHEAD]);
sprintf(line2,"%s(MEMO)",colors[CMAILHEAD]);

while(!feof(infp)) {
        if (cnt<=num) {
                if (*line=='\n') valid=1;
                sscanf(line,"%s",w1);
                if (valid && (!strcmp(w1,line2) || !strcmp(w1,text) || !strcmp(w1,"From:"))) {
                        if (++cnt>num) fputs(line,outfp);
                        valid=0;
                        }
                }
        else fputs(line,outfp);
        fgets(line,ARR_SIZE-1,infp);
        }
fclose(infp);
fclose(outfp);
unlink(filename);
if (cnt<num) {
        unlink("tempfile");
        sprintf(text,"There were only %d messages in your mailbox, all now deleted.\n",cnt);
        write_user(user,text);
        return;
        }
if (cnt==num) {
        unlink("tempfile"); /* cos it'll be empty anyway */
        write_user(user,"All messages deleted.\n");
        }
else {
        rename("tempfile",filename);
        sprintf(text,"%d messages deleted.\n",num);
        write_user(user,text);
        }
}


/*** Show list of people your mail is from without seeing the whole lot ***/
mail_from(user)
UR_OBJECT user;
{
FILE *fp;
int valid,cnt,memo;
char w1[ARR_SIZE],line[ARR_SIZE],filename[80],line2[ARR_SIZE];
char plural[2][2]={"s",""};

sprintf(filename,"%s/%s.M",USERFILES,user->name);
if (!(fp=fopen(filename,"r"))) {
        write_user(user,"You have no mail.\n");  return;
        }
sprintf(text,"\n%s%s-=- Mail Contents -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
write_user(user,text);
valid=1;  cnt=0;  memo=0;
fgets(line,DNL,fp); 
fgets(line,ARR_SIZE-1,fp);

/* An error developed when counting messages because it looked for "~OLFrom:"
   but with the modular colorcodes, it wouldn't always be "~OL".  */
sprintf(text,"%sFrom:",colors[CMAILHEAD]);
sprintf(line2,"%s(MEMO)",colors[CMAILHEAD]);

while(!feof(fp)) {
        if (*line=='\n') valid=1;
        sscanf(line,"%s",w1);
        if (valid && (!strcmp(w1,line2) || !strcmp(w1,text) || !strcmp(w1,"From:"))) {
                if (!(!strcmp(w1,line2))) {
                        write_user(user,remove_first(line));
                        cnt++;  valid=0;
                        }
                else { memo++;  valid=0; }
                }
        fgets(line,ARR_SIZE-1,fp);
        }
fclose(fp);
sprintf(text,"\nThere are %d message%s & %d personal memo%s in your mailbox.\n\n",cnt,plural[(cnt==1)],memo,plural[(memo==1)]);
write_user(user,text);
}



/*** Enter user profile ***/
enter_profile(user,done_editing)
UR_OBJECT user;
int done_editing;
{
FILE *fp;
char *c,filename[80];

if (!done_editing) {
        sprintf(text,"\n%s%s-=- Writing User Profile -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        user->misc_op=5;
        editor(user,NULL);
        return;
        }
sprintf(filename,"%s/%s.P",USERFILES,user->name);
if (!(fp=fopen(filename,"w"))) {
        sprintf(text,"%s: couldn't save your profile.\n",syserror);
        write_user(user,text);
        sprintf("ERROR: Couldn't open file %s to write in enter_profile().\n",filename);
        write_syslog(text,0,SYSLOG);
        return;
        }
c=user->malloc_start;
while(c!=user->malloc_end) putc(*c++,fp);
fclose(fp);
write_user(user,"Profile stored.\n");
}


/*** Examine a user ***/
examine(user)
UR_OBJECT user;
{
UR_OBJECT u,u2;
FILE *fp;
char filename[80],line[82];
char *str,*colour_com_strip();
int new_mail,days,hours,mins,timelen,days2,hours2,mins2,idle,len,i,a,b,c,d,tmp;
char plural[2][2]={" ","s"};

if (word_count<2) {
        write_user(user,"Examine who?\n");  return;
        }
if (!(u=get_user(word[1])) || (!u->vis && u->level > user->level)) {
        if ((u=create_user())==NULL) {
                sprintf(text,"%s: unable to create temporary user object.\n",syserror);
                write_user(user,text);
                write_syslog("ERROR: Unable to create temporary user object in examine().\n",0,SYSLOG);
                return;
                }
        strcpy(u->name,word[1]);
        if (!load_user_details(u)) {
                write_user(user,nosuchuser);   
                destruct_user(u);
                destructed=0;
                return;
                }
        u2=NULL;
        }
else u2=u;

days=u->total_login/86400;
hours=(u->total_login%86400)/3600;
mins=(u->total_login%3600)/60;
timelen=(int)(time(0)) - u->last_login;
days2=timelen/86400;
hours2=(timelen%86400)/3600;
mins2=(timelen%3600)/60;

sprintf(filename,"%s/%s.M",USERFILES,u->name);
if (!(fp=fopen(filename,"r"))) new_mail=0;
else {
        fscanf(fp,"%d",&new_mail);
        fclose(fp);
        }

/* Write Header (72 char)*/
write_user(user,".--- ");
sprintf(text,"%s%s %s~RS ",colors[CWHOUSER],u->name,u->desc);
write_user(user,text);
tmp=(59-strlen(u->name)-strlen(colour_com_strip(u->desc))-strlen(level_name[u->cloaklev]));
for (i=0; i<tmp; i++) {
        write_user(user,"-");
        }
sprintf(text," ~FT%s~RS ---.\n\n",level_name[u->cloaklev]);
write_user(user,text);
/* Header Done */

if (u->reverse) a=1; else a=0;
if (u->muzzled) b=1; else b=0;
if (u->arrested) c=1; else c=0;
if (new_mail>u->read_mail) d=1; else d=0;
if (u2!=NULL) {
        idle=(int)(time(0) - u->last_input)/60;
           sprintf(text,"     Possessed: ~OL%3s~RS            Muzzled: ~OL%3s~RS         Arrested: ~OL%3s\n",noyes2[a],noyes2[b],noyes2[c]);
           write_user(user,text);         
           sprintf(text,"  Ignoring ALL: ~OL%3s~RS    Has Unread Mail: ~OL%3s~RS    AwayFromKeybd: ~OL%3s\n",noyes2[u->ignall],noyes2[d],noyes2[u->afk]);
           write_user(user,text);         
        if (u->age!=-1)
           sprintf(text,"           Age: ~OL%-3d~RS             Gender: ~OL%-6s\n",u->age,gender[u->gender]);
        else
           sprintf(text,"           Age: (unknown)       Gender: ~OL%-6s\n",gender[u->gender]);
           write_user(user,text);
        write_user(user,"\n");
           sprintf(text,"              Logged in on: %-24s",ctime((time_t *)&(u->last_login)));
           write_user(user,text);         
           sprintf(text,"                online for: %3d hour%s, and %2d minute%s\n",(timelen/3600),plural[((timelen/3600)!=1)],mins2,plural[((mins2)!=1)]);
           write_user(user,text);         
           sprintf(text,"         and has been idle: %3d minute%s.\n",idle,plural[((idle)!=1)]);
           write_user(user,text);         
        sprintf(text,"(%s examined you.)\n",user->name);
        if (u->level >= user->level && u!=user) write_user(u,text);
        }
else {
           sprintf(text,"       Muzzled: ~OL%3s~RS           Arrested: ~OL%3s~RS    Has Unread Mail: ~OL%3s\n",noyes2[b],noyes2[c],noyes2[d]);
           write_user(user,text);         
        if (u->age!=-1)
           sprintf(text,"           Age: ~OL%-3d~RS             Gender: ~OL%-6s\n",u->age,gender[u->gender]);
        else
           sprintf(text,"           Age: (unknown)       Gender: ~OL%-6s\n",gender[u->gender]);
           write_user(user,text);         
        write_user(user,"\n");
           sprintf(text,"     Was last logged in on: %-24s",ctime((time_t *)&(u->last_login)));
           write_user(user,text);         
           sprintf(text,"                            %3d day%s, %2d hour%s, and %2d minute%s ago.\n",days2,plural[((days2)!=1)],hours2,plural[((hours2)!=1)],mins2,plural[((mins2)!=1)]);
           write_user(user,text);
           sprintf(text,"        and was online for:  %2d hour%s, and %2d minute%s\n",u->last_login_len/3600,plural[((u->last_login_len/3600)!=1)],(u->last_login_len%3600)/60,plural[(((u->last_login_len%3600)/60)!=1)]);
           write_user(user,text);         
        }
   sprintf(text,"\n          Total login time: %3d day%s, %2d hour%s, and %2d minute%s\n",days,plural[((days)!=1)],hours,plural[((hours)!=1)],mins,plural[((mins)!=1)]);
   write_user(user,text);         
   sprintf(text," since user was created on: %-24s\n",ctime((time_t *)&u->created));
   write_user(user,text);                                       
sprintf(filename,"%s/%s.P",USERFILES,u->name);
if (u2==NULL) {
        destruct_user(u);
        destructed=0;
        }
if (!strcmp(word[2],"data")) { write_user(user,"------------------------------------------------------------------------\n\n"); return; }
        else write_user(user,"-----------------------------=< ~FT~ULUser Profile~RS >=-------------------------\n\n");
if (!(fp=fopen(filename,"r"))) write_user(user,"                        [ Profile does not exist. ]\n");
else {
        fgets(line,81,fp);
        while(!feof(fp)) {
                write_user(user,line);
                fgets(line,81,fp);
                }
        fclose(fp);
        }
write_user(user,"\n");
}


/*** Show talker rooms ***/
rooms(user,show_topics)
UR_OBJECT user;
int show_topics;
{
RM_OBJECT rm;
UR_OBJECT u;
NL_OBJECT nl;
char access[9],stat[9],serv[SERV_NAME_LEN+1];
char plural[2][2]={"","s"};
int cnt,numrooms,personal,foundjail,messages_num,messages_rm,rm_priv,rm_pub;

numrooms=0; personal=0; foundjail=0;  messages_num=0;  messages_rm=0;
rm_priv=0;  rm_pub=0;

if (show_topics) {
        sprintf(text,"\n%s%s-=- Rooms Data -=-\n\n~FTRoom name            : Access  Usr  Msg  Topic\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        }
else { sprintf(text,"\n%s%s-=- Rooms Data -=-\n\n~FTRoom name            : Access  Usr  Msg  LinkIN  LStat  Service\n\n",colors[CHIGHLIGHT],colors[CTEXT]); write_user(user,text); }
for(rm=room_first;rm!=NULL;rm=rm->next) {
        if (rm->personal) { personal++; continue; }
        else numrooms++;
        if (rm->access & PRIVATE) { strcpy(access," ~FRPRIV"); rm_priv++; }
                else { strcpy(access,"  ~FGPUB"); rm_pub++; }
        if (!rm->link_label[0][0]) { strcpy(access," ~FYJAIL"); foundjail=1; }
        if (rm->access & FIXED) access[0]='*';
        cnt=0;
        for(u=user_first;u!=NULL;u=u->next) 
                if (u->type!=CLONE_TYPE && u->room==rm) ++cnt;
        if (show_topics)
                sprintf(text,"%-20s : %9s~RS  %3d  %3d  %s\n",rm->name,access,cnt,rm->mesg_cnt,rm->topic);
        else {
                nl=rm->netlink;  serv[0]='\0';
                if (nl==NULL) {
                        if (rm->inlink) strcpy(stat,"~FRDOWN");
                        else strcpy(stat,"   -");
                        }
                else {
                        if (nl->type==UNCONNECTED) strcpy(stat,"~FRDOWN");
                                else if (nl->stage==UP) strcpy(stat,"  ~FGUP");
                                        else strcpy(stat," ~FYVER");
                        }
                if (nl!=NULL) strcpy(serv,nl->service);
                sprintf(text,"%-20s : %9s~RS  %3d  %3d     %s   %s~RS  %s\n",rm->name,access,cnt,rm->mesg_cnt,noyes1[rm->inlink],stat,serv);
                }
        write_user(user,text);
        if (rm->mesg_cnt > 0) { messages_num = messages_num + rm->mesg_cnt;
                                messages_rm++;
                                }
        }
sprintf(text,"\nTotal of %d permanent rooms configured.\nSystem has allocated space for %d personal room%s. (.prstat for details)\n\n",numrooms,personal,plural[(personal!=1)]);
write_user(user,text);
if (foundjail) write_user(user,"The jail system is available.\n");
        else   write_user(user,"The jail system is not available.  No room has been specified as 'JAIL.'\n");
sprintf(text,"%d room%s are public.  %d room%s are private.\n%d message%s found in %d room%s.\n\n",rm_pub,plural[(rm_pub!=0)],rm_priv,plural[(rm_priv!=0)],messages_num,plural[(messages_num!=1)],messages_rm,plural[(messages_rm!=1)]);
write_user(user,text);
}


/*** List defined netlinks and their status ***/
netstat(user)
UR_OBJECT user;
{
NL_OBJECT nl;
UR_OBJECT u;
char *allow[]={ "  ?","ALL"," IN","OUT" };
char *type[]={ "  -"," IN","OUT" };
char portstr[6],stat[9],vers[8];
int iu,ou,a;

if (nl_first==NULL) {
        write_user(user,"No remote connections configured.\n");  return;
        }
sprintf(text,"\n%s%s-=- Netlink data & status -=-\n\n~FTService name    : Allow Type Status IU OU Version  Site\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
write_user(user,text);
for(nl=nl_first;nl!=NULL;nl=nl->next) {
        iu=0;  ou=0;
        if (nl->stage==UP) {
                for(u=user_first;u!=NULL;u=u->next) {
                        if (u->netlink==nl) {
                                if (u->type==REMOTE_TYPE)  ++iu;
                                if (u->room==NULL) ++ou;
                                }
                        }
                }
        if (nl->port) sprintf(portstr,"%d",nl->port);  else portstr[0]='\0';
        if (nl->type==UNCONNECTED) {
                strcpy(stat,"~FRDOWN");  strcpy(vers,"-");
                }
        else {
                if (nl->stage==UP) strcpy(stat,"  ~FGUP");
                else strcpy(stat," ~FYVER");
                if (!nl->ver_major) strcpy(vers,"3.?.?"); /* Pre - 3.2 version */  
                else sprintf(vers,"%d.%d.%d",nl->ver_major,nl->ver_minor,nl->ver_patch);
                }
        /* If link is incoming and remoter vers < 3.2 we have no way of knowing 
           what the permissions on it are so set to blank */
        if (!nl->ver_major && nl->type==INCOMING && nl->allow!=IN) a=0; 
        else a=nl->allow+1;
        sprintf(text,"%-15s :   %s  %s   %s~RS %2d %2d %7s  %s %s\n",nl->service,allow[a],type[nl->type],stat,iu,ou,vers,nl->site,portstr);
        write_user(user,text);
        }
write_user(user,"\n");
}



/*** Show type of data being received down links (this is usefull when a
     link has hung) ***/
netdata(user)
UR_OBJECT user;
{
NL_OBJECT nl;
char from[80],name[USER_NAME_LEN+1];
int cnt;

cnt=0;
sprintf(text,"\n%s%s-=- Mail receiving status -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
write_user(user,text);
for(nl=nl_first;nl!=NULL;nl=nl->next) {
        if (nl->type==UNCONNECTED || nl->mailfile==NULL) continue;
        if (++cnt==1) write_user(user,"To              : From                       Last recv.\n\n");
        sprintf(from,"%s@%s",nl->mail_from,nl->service);
        sprintf(text,"%-15s : %-25s  %d seconds ago.\n",nl->mail_to,from,(int)(time(0)-nl->last_recvd));
        write_user(user,text);
        }
if (!cnt) write_user(user,"No mail being received.\n\n");
else write_user(user,"\n");

cnt=0;
sprintf(text,"\n%s%s-=- Message receiving status -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
write_user(user,text);
for(nl=nl_first;nl!=NULL;nl=nl->next) {
        if (nl->type==UNCONNECTED || nl->mesg_user==NULL) continue;
        if (++cnt==1) write_user(user,"To              : From             Last recv.\n\n");
        if (nl->mesg_user==(UR_OBJECT)-1) strcpy(name,"<unknown>");
        else strcpy(name,nl->mesg_user->name);
        sprintf(text,"%-15s : %-15s  %d seconds ago.\n",name,nl->service,(time(0)-nl->last_recvd));
        write_user(user,text);
        }
if (!cnt) write_user(user,"No messages being received.\n\n");
else write_user(user,"\n");
}


/*** Connect a netlink. Use the room as the key ***/
connect_netlink(user)
UR_OBJECT user;
{
RM_OBJECT rm;
NL_OBJECT nl;
int ret,tmperr;

if (word_count<2) {
        write_user(user,"Usage: connect <room service is linked to>\n");  return;
        }
if ((rm=get_room(word[1]))==NULL) {
        write_user(user,nosuchroom);  return;
        }
if ((nl=rm->netlink)==NULL) {
        write_user(user,"That room is not linked to a service.\n");
        return;
        }
if (nl->type!=UNCONNECTED) {
        write_user(user,"That rooms netlink is already up.\n");  return;
        }
write_user(user,"Attempting connect (this may cause a temporary hang)...\n");
sprintf(text,"NETLINK: Connection attempt to %s initiated by %s.\n",nl->service,user->name);
write_syslog(text,1,SYSLOG);
errno=0;
if (!(ret=connect_to_site(nl))) {
        write_user(user,"~FGInitial connection made...\n");
        sprintf(text,"NETLINK: Connected to %s (%s %d).\n",nl->service,nl->site,nl->port);
        write_syslog(text,1,SYSLOG);
        nl->connect_room=rm;
        return;
        }
tmperr=errno; /* On Linux errno seems to be reset between here and sprintf */
write_user(user,"~FRConnect failed: ");
write_syslog("NETLINK: Connection attempt failed: ",1,SYSLOG);
if (ret==1) {
        sprintf(text,"%s.\n",sys_errlist[tmperr]);
        write_user(user,text);
        write_syslog(text,0,SYSLOG);
        return;
        }
write_user(user,"Unknown hostname.\n");
write_syslog("Unknown hostname.\n",0,SYSLOG);
}



/*** Disconnect a link ***/
disconnect_netlink(user)
UR_OBJECT user;
{
RM_OBJECT rm;
NL_OBJECT nl;

if (word_count<2) {
        write_user(user,"Usage: disconnect <room service is linked to>\n");  return;
        }
if ((rm=get_room(word[1]))==NULL) {
        write_user(user,nosuchroom);  return;
        }
nl=rm->netlink;
if (nl==NULL) {
        write_user(user,"That room is not linked to a service.\n");
        return;
        }
if (nl->type==UNCONNECTED) {
        write_user(user,"That rooms netlink is not connected.\n");  return;
        }
/* If link has hung at verification stage don't bother announcing it */
if (nl->stage==UP) {
        sprintf(text,"~OLSYSTEM:~RS Disconnecting from %s in the %s.\n",nl->service,rm->name);
        write_room(NULL,text);
        sprintf(text,"NETLINK: Link to %s in the %s disconnected by %s.\n",nl->service,rm->name,user->name);
        write_syslog(text,1,SYSLOG);
        }
else {
        sprintf(text,"NETLINK: Link to %s disconnected by %s.\n",nl->service,user->name);
        write_syslog(text,1,SYSLOG);
        }
shutdown_netlink(nl);
write_user(user,"Disconnected.\n");
}


/*** Change users password. Only ARCHes and above can change another users 
        password and they do this by specifying the user at the end. When this is 
        done the old password given can be anything, the wiz doesnt have to know it
        in advance. ***/
change_pass(user)
UR_OBJECT user;
{
UR_OBJECT u;
char *name;

if (word_count<3) {
        if (user->level<GOD)
                write_user(user,"Usage: passwd <old password> <new password>\n");
        else write_user(user,"Usage: passwd <old password> <new password> [<user>]\n");
        return;
        }
if (strlen(word[2])<3) {
        write_user(user,"New password too short.\n");  return;
        }
if (strlen(word[2])>PASS_LEN) {
        write_user(user,"New password too long.\n");  return;
        }
/* Change own password */
if (word_count==3) {
        if (strcmp((char *)crypt(word[1],"NU"),user->pass)) {
                write_user(user,"Old password incorrect.\n");  return;
                }
        if (!strcmp(word[1],word[2])) {
                write_user(user,"Old and new passwords are the same.\n");  return;
                }
        strcpy(user->pass,(char *)crypt(word[2],"NU"));
        save_user_details(user,0);
        cls(user);
        write_user(user,"Password changed.\n");
        return;
        }
/* Change someone elses */
if (user->level<GOD) {
        write_user(user,"You are not a high enough level to use the user option.\n");  
        return;
        }
word[3][0]=toupper(word[3][0]);
if (!strcmp(word[3],user->name)) {
        /* security feature  - prevents someone coming to a wizes terminal and 
           changing his password since he wont have to know the old one */
        write_user(user,"You cannot change your own password using the <user> option.\n");
        return;
        }
if (u=get_user(word[3])) {
        if (u->type==REMOTE_TYPE) {
                write_user(user,"You cannot change the password of a user logged on remotely.\n");
                return;
                }
        if (u->level>=user->level) {
                write_user(user,"You cannot change the password of a user of equal or higher level than yourself.\n");
                return;
                }
        strcpy(u->pass,(char *)crypt(word[2],"NU"));
        cls(user);
        sprintf(text,"%s's password has been changed.\n",u->name);
        write_user(user,text);
        if (user->vis) name=user->name; else name=invisname;
        sprintf(text,"~FR~OLYour password has been changed by %s!\n",name);
        write_user(u,text);
        sprintf(text,"%s changed %s's password.\n",user->name,u->name);
        write_syslog(text,1,USERLOG);
        sprintf(text,"%s[ %s's PASSWORD CHANGED by %s. ]\n",colors[CSYSTEM],u->name,user->name);
        write_duty(user->level,text,NULL,user,0);
        return;
        }
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in change_pass().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[3]);
if (!load_user_details(u)) {
        write_user(user,nosuchuser);   
        destruct_user(u);
        destructed=0;
        return;
        }
if (u->level>=user->level) {
        write_user(user,"You cannot change the password of a user of equal or higher level than yourself.\n");
        destruct_user(u);
        destructed=0;
        return;
        }
strcpy(u->pass,(char *)crypt(word[2],"NU"));
save_user_details(u,0);
cls(user);
sprintf(text,"%s's password changed to \"%s\".\n",word[3],word[2]);
write_user(user,text);
sprintf(text,"%s changed %s's password.\n",user->name,u->name);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s's PASSWORD CHANGED by %s. ]\n",colors[CSYSTEM],u->name,user->name);
write_duty(user->level,text,NULL,user,0);
destruct_user(u);
destructed=0;
}


/*** Kill a user ***/
kill_user(user)
UR_OBJECT user;
{
UR_OBJECT victim;
RM_OBJECT rm;
char *name;

if (word_count<2) {
        write_user(user,"Usage: kill <user>\n");  return;
        }
if (!(victim=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (user==victim) {
        write_user(user,"No matter how bad it gets, suicide via .KILL is not an option here.\n");
        return;
        }
if (victim->level>=user->level) {
        write_user(user,"You cannot kill a user of equal or higher level than yourself.\n");
        sprintf(text,"%s tried to kill you!\n",user->name);
        write_user(victim,text);
        return;
        }
sprintf(text,"%s KILLED %s.\n",user->name,victim->name);
write_syslog(text,1,USERLOG);
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"%s%s>>>~RS %sYou are being forced to disconnect!  You have been KILLED.\n",colors[CBOLD],colors[CSYSBOLD],colors[CSYSTEM]);
write_user(victim,text);
sprintf(text,"%s[ %s KILLED %s ]\n",colors[CSYSTEM],user->name,victim->name);
write_duty(WIZ,text,NULL,victim,0);
disconnect_user(victim);
sprintf(text,"%s%s<!>~RS%s USER KILLED ~RS%s%s<!>\n",colors[CBOLD],colors[CSYSBOLD],colors[CSYSTEM],colors[CBOLD],colors[CSYSBOLD]);
write_room(NULL,text);
}


/*** Promote a user ***/
promote(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char text2[80],*name;
int alt_cloak; /* By changing the level, do we need to alter the cloaklev? */

if (word_count<2) {
        write_user(user,"Usage: promote <user>\n");  return;
        }
/* See if user is on atm */
if ((u=get_user(word[1]))!=NULL) {
        if (u->cloaklev==u->level) alt_cloak=1;
        else alt_cloak=0;
        if (u->orig_level>=user->orig_level || u->orig_level+1==user->orig_level) {
                write_user(user,"You cannot promote a user to a level higher than or equal to your own.\n");
                return;
                }
        if (user->vis) name=user->name; else name=invisname;
        u->orig_level++;
        u->level = u->orig_level;
        if (alt_cloak) {
                u->cloaklev=u->level;
                }
        if (!user->duty) {
                sprintf(text,"~FG~OLYou promote %s to level: ~RS~OL%s.\n",u->name,level_name[u->level]);
                write_user(user,text);
                }
        sprintf(text,"%s[ %s PROMOTED %s to level %s. ]\n",colors[CSYSTEM],user->name,u->name,level_name[u->level]);
        write_duty(WIZ,text,NULL,NULL,0);
        sprintf(text,"~FG~OL%s has promoted you to level: ~RS~OL%s!\n",name,level_name[u->level]);
        write_user(u,text);
        sprintf(text,"%s PROMOTED %s to level %s.\n",user->name,u->name,level_name[u->level]);
        write_syslog(text,1,USERLOG);
        save_user_details(u,1);
        return;
        }
/* Create a temp session, load details, alter , then save. This is inefficient
   but its simpler than the alternative */
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in promote().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
        write_user(user,nosuchuser);  
        destruct_user(u);
        destructed=0;
        return;
        }
if (u->orig_level>=user->orig_level) {
        write_user(user,"You cannot promote a user to a level higher than your own.\n");
        destruct_user(u);
        destructed=0;
        return;
        }
u->orig_level++;  
u->level=u->orig_level;
u->socket=-2;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"You promote %s to level: ~OL%s.\n",u->name,level_name[u->level]);
write_user(user,text);
sprintf(text2,"~FG~OLYou have been promoted to level: ~RS~OL%s.\n",level_name[u->level]);
send_mail(NULL,word[1],text2);
sprintf(text,"%s PROMOTED %s to level %s.\n",user->name,word[1],level_name[u->level]);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s PROMOTED %s (offline) to level %s. ]\n",colors[CSYSTEM],user->name,u->name,level_name[u->level]);
write_duty(WIZ,text,NULL,NULL,0);
destruct_user(u);
destructed=0;
}


/*** Demote a user ***/
demote(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char text2[80],*name;
int alt_cloak;

if (word_count<2) {
        write_user(user,"Usage: demote <user>\n");  return;
        }
/* See if user is on atm */
if ((u=get_user(word[1]))!=NULL) {
        if (u->cloaklev==u->level) alt_cloak=1;
        else alt_cloak=0;
        if (u->level==NEW) {
                write_user(user,"You cannot demote a user of level NEW.\n");
                return;
                }
        if (u->orig_level>=user->orig_level) {
                write_user(user,"You cannot demote a user of an equal or higher level than yourself.\n");
                return;
                }
        if (user->vis) name=user->name; else name=invisname;
        u->orig_level--;
        u->level = u->orig_level;
        if (alt_cloak) u->cloaklev=u->level;
        else if (u->cloaklev > u->level || u->level < com_level[CLOAK] ) u->cloaklev = u->level;
        if (!user->duty) {
                sprintf(text,"~FR~OLYou demote %s to level: ~RS~OL%s.\n",u->name,level_name[u->level]);
                write_user(user,text);
                }
        sprintf(text,"%s[ %s DEMOTED %s to level %s. ]\n",colors[CSYSTEM],user->name,u->name,level_name[u->level]);
        write_duty(WIZ,text,NULL,NULL,0);
        sprintf(text,"~FR~OL%s has demoted you to level: ~RS~OL%s!\n",name,level_name[u->level]);
        write_user(u,text);
        sprintf(text,"%s DEMOTED %s to level %s.\n",user->name,u->name,level_name[u->level]);
        write_syslog(text,1,USERLOG);
        save_user_details(u,1);
        return;
        }
/* User not logged on */
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in demote().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
        write_user(user,nosuchuser);  
        destruct_user(u);
        destructed=0;
        return;
        }
if (u->level==NEW) {
        write_user(user,"You cannot demote a user of level NEW.\n");
        destruct_user(u);
        destructed=0;
        return;
        }
if (u->orig_level>=user->orig_level) {
        write_user(user,"You cannot demote a user of an equal or higher level than yourself.\n");
        destruct_user(u);
        destructed=0;
        return;
        }
u->orig_level--;  u->level=u->orig_level;
u->socket=-2;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"You demote %s to level: ~OL%s.\n",u->name,level_name[u->level]);
write_user(user,text);
sprintf(text2,"~FR~OLYou have been demoted to level: ~RS~OL%s.\n",level_name[u->level]);
send_mail(NULL,word[1],text2);
sprintf(text,"%s DEMOTED %s to level %s.\n",user->name,word[1],level_name[u->level]);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s DEMOTED %s (offline) to level %s. ]\n",colors[CSYSTEM],user->name,u->name,level_name[u->level]);
write_duty(WIZ,text,NULL,NULL,0);
destruct_user(u);
destructed=0;
}


/*** List banned sites or users ***/
listbans(user)
UR_OBJECT user;
{
int i;
char filename[80];

if (!strncmp(word[1],"s",1)) {
        sprintf(text,"\n%s%s-=- Banned SITES/DOMAINS -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        sprintf(filename,"%s/%s",DATAFILES,SITEBAN);
        switch(more(user,user->socket,filename)) {
                case 0:
                write_user(user,"There are no banned sites/domains.\n\n");
                return;

                case 1: user->misc_op=2;
                }
        return;
        }
if (!strncmp(word[1],"u",1)) {
        sprintf(text,"\n%s%s-=- Banned USERS -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        sprintf(filename,"%s/%s",DATAFILES,USERBAN);
        switch(more(user,user->socket,filename)) {
                case 0:
                write_user(user,"There are no banned users.\n\n");
                return;

                case 1: user->misc_op=2;
                }
        return;
        }
if (!strncmp(word[1],"w",1)) {
        sprintf(text,"\n%s%s-=- Banned WORDS -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        i=0;
        while(swear_words[i][0]!='*') {
                write_user(user,swear_words[i]);
                write_user(user,"\n");
                ++i;
                }
        if (!i) write_user(user,"There are no banned words.\n");
        if (ban_swearing) write_user(user,"\n");
        else write_user(user,"\n(Word ban is currently ~FROFF~RS.)\n\n");
        return;
        }
if (!strncmp(word[1],"n",1)) {
        sprintf(text,"\n%s%s-=- Newbie Site Lockouts -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        sprintf(filename,"%s/%s",DATAFILES,NEWBIEBAN);
        switch(more(user,user->socket,filename)) {
                case 0:
                write_user(user,"No sites are banned from creating new accounts.\n\n");
                return;

                case 1: user->misc_op=2;
                }
        return;
        }

write_user(user,"Usage: listbans sites/users/new/words\n");
}


/*** Ban a site/domain or user ***/
ban(user)
UR_OBJECT user;
{
char *usage="Usage: ban site/user/new <site/user name>\n";

if (word_count<3) {
        write_user(user,usage);  return;
        }
if (!strncmp(word[1],"s",1)) {  ban_site(user);  return;  }
if (!strncmp(word[1],"u",1)) {  ban_user(user);  return;  }
if (!strncmp(word[1],"n",1))  {  ban_newsite(user);  return;  }
write_user(user,usage);
}


ban_site(user)
UR_OBJECT user;
{
FILE *fp;
char filename[80],host[81],site[80];

gethostname(host,80);
if (!strcmp(word[2],host)) {
        write_user(user,"You cannot ban the machine that this program is running on.\n");
        return;
        }
sprintf(filename,"%s/%s",DATAFILES,SITEBAN);

/* See if ban already set for given site */
if (fp=fopen(filename,"r")) {
        fscanf(fp,"%s",site);
        while(!feof(fp)) {
                if (!strcmp(site,word[2])) {
                        write_user(user,"That site/domain is already banned.\n");
                        fclose(fp);  return;
                        }
                fscanf(fp,"%s",site);
                }
        fclose(fp);
        }

/* Write new ban to file */
if (!(fp=fopen(filename,"a"))) {
        sprintf(text,"%s: Can't open file to append.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open file to append in ban_site().\n",0,SYSLOG);
        return;
        }
fprintf(fp,"%s\n",word[2]);
fclose(fp);
write_user(user,"Site/domain banned.\n");
sprintf(text,"%s BANNED site/domain %s.\n",user->name,word[2]);
write_syslog(text,1,BANLOG);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s BANNED site/domain: %s ]\n",colors[CSYSTEM],user->name,word[2]);
write_duty(WIZ,text,NULL,user,0);
}


ban_user(user)
UR_OBJECT user;
{
UR_OBJECT u;
FILE *fp;
char filename[80],filename2[80],p[20],name[USER_NAME_LEN+1];
int a,b,c,d,level;

word[2][0]=toupper(word[2][0]);
if (!strcmp(user->name,word[2])) {
        write_user(user,"You must be a talker addict trying to .QUIT if you're BANNING yourself.\n");
        return;
        }

/* See if ban already set for given user */
sprintf(filename,"%s/%s",DATAFILES,USERBAN);
if (fp=fopen(filename,"r")) {
        fscanf(fp,"%s",name);
        while(!feof(fp)) {
                if (!strcmp(name,word[2])) {
                        write_user(user,"That user is already banned.\n");
                        fclose(fp);  return;
                        }
                fscanf(fp,"%s",name);
                }
        fclose(fp);
        }

/* See if already on */
if ((u=get_user(word[2]))!=NULL) {
        if (u->level>=user->level) {
                write_user(user,"You cannot ban a user of equal or higher level than yourself.\n");
                return;
                }
        }
else {
        /* User not on so load up his data */
        sprintf(filename2,"%s/%s.D",USERFILES,word[2]);
        if (!(fp=fopen(filename2,"r"))) {
                write_user(user,nosuchuser);  return;
                }
        fscanf(fp,"%s\n%d %d %d %d %d",p,&a,&b,&c,&d,&level);
        fclose(fp);
        if (level>=user->level) {
                write_user(user,"You cannot ban a user of equal or higher level than yourself.\n");
                return;
                }
        }

/* Write new ban to file */
if (!(fp=fopen(filename,"a"))) {
        sprintf(text,"%s: Can't open file to append.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open file to append in ban_user().\n",0,SYSLOG);
        return;
        }
fprintf(fp,"%s\n",word[2]);
fclose(fp);
write_user(user,"User banned.\n");
sprintf(text,"%s BANNED user %s.\n",user->name,word[2]);
write_syslog(text,1,BANLOG);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s BANNED user: %s ]\n",colors[CSYSTEM],user->name,word[2]);
write_duty(WIZ,text,NULL,user,0);
if (u!=NULL) {
        sprintf(text,"\n\07%s%s~LIYou have been banned from here!\n\n",colors[CBOLD],colors[CSYSTEM]);
        write_user(u,text);
        disconnect_user(u);
        }
}


ban_newsite(user)
UR_OBJECT user;
{
FILE *fp;
char filename[80],host[81],site[80];

gethostname(host,80);
if (!strcmp(word[2],host)) {
        write_user(user,"You cannot ban the machine that this program is running on.\n");
        return;
        }
sprintf(filename,"%s/%s",DATAFILES,NEWBIEBAN);

/* See if ban already set for given site */
if (fp=fopen(filename,"r")) {
        fscanf(fp,"%s",site);
        while(!feof(fp)) {
                if (!strcmp(site,word[2])) {
                        write_user(user,"That site/domain is already banned from making new accounts.\n");
                        fclose(fp);  return;
                        }
                fscanf(fp,"%s",site);
                }
        fclose(fp);
        }

/* Write new ban to file */
if (!(fp=fopen(filename,"a"))) {
        sprintf(text,"%s: Can't open file to append.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open file to append in ban_newsite().\n",0,0);
        return;
        }
fprintf(fp,"%s\n",word[2]);
fclose(fp);
write_user(user,"Site/domain banned from creating new accounts.\n");
sprintf(text,"%s BANNED site/domain %s from creating accounts.\n",user->name,word[2]);
write_syslog(text,1,BANLOG);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s BANNED site/domain: %s from creating accounts. ]\n",colors[CSYSTEM],user->name,word[2]);
write_duty(WIZ,text,NULL,user,0);
}


/*** uban a site (or domain) or user ***/
unban(user)
UR_OBJECT user;
{
char *usage="Usage: unban site/user/new <site/user name>\n";

if (word_count<3) {
        write_user(user,usage);  return;
        }
if (!strncmp(word[1],"s",1)) {  unban_site(user);  return;  }
if (!strncmp(word[1],"u",1)) {  unban_user(user);  return;  }
if (!strncmp(word[1],"n",1)) {  unban_newsite(user);  return;  }
write_user(user,usage);
}


unban_site(user)
UR_OBJECT user;
{
FILE *infp,*outfp;
char filename[80],site[80];
int found,cnt;

sprintf(filename,"%s/%s",DATAFILES,SITEBAN);
if (!(infp=fopen(filename,"r"))) {
        write_user(user,"That site/domain is not currently banned.\n");
        return;
        }
if (!(outfp=fopen("tempfile","w"))) {
        sprintf(text,"%s: Couldn't open tempfile.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open tempfile to write in unban_site().\n",0,SYSLOG);
        fclose(infp);
        return;
        }
found=0;   cnt=0;
fscanf(infp,"%s",site);
while(!feof(infp)) {
        if (strcmp(word[2],site)) {  
                fprintf(outfp,"%s\n",site);  cnt++;  
                }
        else found=1;
        fscanf(infp,"%s",site);
        }
fclose(infp);
fclose(outfp);
if (!found) {
        write_user(user,"That site/domain is not currently banned.\n");
        unlink("tempfile");
        return;
        }
if (!cnt) {
        unlink(filename);  unlink("tempfile");
        }
else rename("tempfile",filename);
write_user(user,"Site ban removed.\n");
sprintf(text,"%s UNBANNED site %s.\n",user->name,word[2]);
write_syslog(text,1,BANLOG);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s UNBANNED site/domain: %s ]\n",colors[CSYSTEM],user->name,word[2]);
write_duty(WIZ,text,NULL,user,0);
}


unban_user(user)
UR_OBJECT user;
{
FILE *infp,*outfp;
char filename[80],name[USER_NAME_LEN+1];
int found,cnt;

sprintf(filename,"%s/%s",DATAFILES,USERBAN);
if (!(infp=fopen(filename,"r"))) {
        write_user(user,"That user is not currently banned.\n");
        return;
        }
if (!(outfp=fopen("tempfile","w"))) {
        sprintf(text,"%s: Couldn't open tempfile.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open tempfile to write in unban_user().\n",0,SYSLOG);
        fclose(infp);
        return;
        }
found=0;  cnt=0;
word[2][0]=toupper(word[2][0]);
fscanf(infp,"%s",name);
while(!feof(infp)) {
        if (strcmp(word[2],name)) {
                fprintf(outfp,"%s\n",name);  cnt++;
                }
        else found=1;
        fscanf(infp,"%s",name);
        }
fclose(infp);
fclose(outfp);
if (!found) {
        write_user(user,"That user is not currently banned.\n");
        unlink("tempfile");
        return;
        }
if (!cnt) {
        unlink(filename);  unlink("tempfile");
        }
else rename("tempfile",filename);
write_user(user,"User ban removed.\n");
sprintf(text,"%s UNBANNED user %s.\n",user->name,word[2]);
write_syslog(text,1,BANLOG);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s UNBANNED user: %s ]\n",colors[CSYSTEM],user->name,word[2]);
write_duty(WIZ,text,NULL,user,0);
}

unban_newsite(user)
UR_OBJECT user;
{
FILE *infp,*outfp;
char filename[80],site[80];
int found,cnt;

sprintf(filename,"%s/%s",DATAFILES,NEWBIEBAN);
if (!(infp=fopen(filename,"r"))) {
        write_user(user,"That site/domain is already allowed to create new accounts.\n");
        return;
        }
if (!(outfp=fopen("tempfile","w"))) {
        sprintf(text,"%s: Couldn't open tempfile.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Couldn't open tempfile to write in unban_newsite().\n",0,SYSLOG);
        fclose(infp);
        return;
        }
found=0;   cnt=0;
fscanf(infp,"%s",site);
while(!feof(infp)) {
        if (strcmp(word[2],site)) {  
                fprintf(outfp,"%s\n",site);  cnt++;  
                }
        else found=1;
        fscanf(infp,"%s",site);
        }
fclose(infp);
fclose(outfp);
if (!found) {
        write_user(user,"That site/domain is already allowed to create new accounts.\n");
        unlink("tempfile");
        return;
        }
if (!cnt) {
        unlink(filename);  unlink("tempfile");
        }
else rename("tempfile",filename);
write_user(user,"Newbie ban removed from site.\n");
sprintf(text,"%s UNBANNED site %s from making new accounts.\n",user->name,word[2]);
write_syslog(text,1,BANLOG);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s UNBANNED site/domain: %s from making new accounts. ]\n",colors[CSYSTEM],user->name,word[2]);
write_duty(WIZ,text,NULL,user,0);
}



/*** Set user visible or invisible ***/
hide(user)
UR_OBJECT user;
{
UR_OBJECT u;
if (word_count<2) {
        if (user->vis) {
                user->vis=0;
                write_user(user,"You are now hidden!\n");
                sprintf(text,"%s %s\n",user->name,goinvis);
                write_room_except(user->room,text,user);
                return;
                }
        else {
                user->vis=1;
                write_user(user,"You are now visible!\n");
                sprintf(text,"%s %s\n",user->name,govis);
                write_room_except(user->room,text,user);
                return;
                }
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
if (u==user) {
        if (user->vis) {
                user->vis=0;
                write_user(user,"You are now hidden!\n");
                sprintf(text,"%s %s\n",user->name,goinvis);
                write_room_except(user->room,text,user);
                return;
                }
        else {
                user->vis=1;
                write_user(user,"You are now visible!\n");
                sprintf(text,"%s %s\n",user->name,govis);
                write_room_except(user->room,text,user);
                return;
                }
        }
if (u->level >= user->level) {
        write_user(user,"You cannot hide/unhide a user of an equal or higher level.\n");
        return;
        }
if (u->vis) {
        u->vis=0;
                sprintf(text,"%s %s\n",u->name,goinvis);
                write_room_except(u->room,text,u);
        write_user(u,"You are being forced to flicker out of sight!\n");
        write_user(user,"User is now hidden.\n");
        sprintf(text,"%s[ %s has been HIDDEN by %s ]\n",colors[CSYSTEM],u->name,user->name);
        write_duty(user->level,text,NULL,user,0);
        return;
        }
else {
        u->vis=1;
                sprintf(text,"%s %s\n",u->name,govis);
                write_room_except(u->room,text,u);
        write_user(u,"You are being forced to return to full view!\n");
        write_user(user,"User is now visible.\n");
        sprintf(text,"%s[ %s has been REVEALED by %s ]\n",colors[CSYSTEM],u->name,user->name);
        write_duty(user->level,text,NULL,user,0);
        return;
        }
}

/*** Site a user ***/
site(user)
UR_OBJECT user;
{
UR_OBJECT u;

if (word_count<2) {
        write_user(user,"Usage: site <user>\n");  return;
        }
/* User currently logged in */
if (u=get_user(word[1])) {
        if (u->type==REMOTE_TYPE) sprintf(text,"%s is remotely connected from %s.\n",u->name,u->site);
        else sprintf(text,"%s is logged in from %s:%d.\n",u->name,u->site,u->site_port);
        write_user(user,text);
        return;
        }
/* User not logged in */
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in site().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
        write_user(user,nosuchuser);
        destruct_user(u);
        destructed=0;
        return;
        }
sprintf(text,"%s was last logged in from %s.\n",word[1],u->last_site);
write_user(user,text);
destruct_user(u);
destructed=0;
}


/*** Wake up some sleepy herbert ***/
wake(user)
UR_OBJECT user;
{
UR_OBJECT u;
char *name;

if (word_count<2) {
        write_user(user,"Usage: page <user>\n");  return;
        }
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (u==user) {
        write_user(user,"Now what would drive you to page yourself?  Hrmmm...\n");
        return;
        }
if (u->afk && user->level<WIZ) {
        write_user(user,"You cannot page someone who is AFK.\n");  return;
        }
if (!strcmp(u->ignuser,user->name)) {
        write_user(user,"Sorry, you cannot page that person right now.\n");
        return;
        }
if (user->vis || user->level <= u->level) name=user->name; else name=invisname;
sprintf(text,"\07\n%s-=- You have been PAGED by ~RS%s%s~RS%s. -=-\n\n",colors[CSYSBOLD],colors[CWARNING],name,colors[CSYSBOLD]);
write_user(u,text);
if (u->pueblo && !audioprompt(u,2,1)) write_user(u,"</xch_mudtext><img xch_alert><xch_mudtext>");
write_user(user,"Your page has been sent.\n");
}


/*** Shout something to other wizes and gods. If the level isnt given it
        defaults to WIZ level. ***/
wizshout(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int lev;

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<2) {
        write_user(user,"Usage: wizshout [<superuser level>] <message>\n"); 
        return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
/* strtoupper(word[1]); */
if ((lev=get_level(word[1]))==-1) lev=WIZ;
else {
        if (lev<WIZ || word_count<3) {
                write_user(user,"Usage: wizshout [<superuser level>] <message>\n");
                return;
                }
        if (lev>user->level) {
                write_user(user,"You cannot specifically shout to users of a higher level than yourself.\n");
                return;
                }
        inpstr=remove_first(inpstr);
        sprintf(text,"~OLYou wizshout to level %s:~RS %s\n",level_name[lev],inpstr);
        write_user(user,text);
        sprintf(text,"~OL%s wizshouts to level %s:~RS %s\n",user->name,level_name[lev],inpstr);
        write_level(lev,1,text,user);
        return;
        }
sprintf(text,"~OLYou wizshout:~RS %s\n",inpstr);
write_user(user,text);
sprintf(text,"~OL%s wizshouts:~RS %s\n",user->name,inpstr);
write_level(WIZ,1,text,user);
}


/*** Muzzle an annoying user so he cant speak, emote, echo, write, smail
        or bcast. Muzzles have levels from WIZ to GOD so for instance a wiz
     cannot remove a muzzle set by a god.  ***/
muzzle(user)
UR_OBJECT user;
{
UR_OBJECT u;

if (word_count<2) {
        write_user(user,"Usage: muzzle <user>\n");  return;
        }
if ((u=get_user(word[1]))!=NULL) {
        if (u==user) {
                write_user(user,"Instead of muzzling yourself, just shut up!\n");
                return;
                }
        if (u->level>=user->level) {
                write_user(user,"You cannot muzzle a user of equal or higher level than yourself.\n");
                return;
                }
        if (u->muzzled>=user->level) {
                sprintf(text,"%s is already muzzled.\n",u->name);
                write_user(user,text);  return;
                }
        if (!user->duty) {
                sprintf(text,"%s%s%s muzzled by level: ~RS%s%s.\n",colors[CBOLD],colors[CSYSBOLD],u->name,level_name[user->level],colors[CSYSTEM]);
                write_user(user,text);
                }
        sprintf(text,"%s[ %s MUZZLED %s with level: %s ]\n",colors[CSYSTEM],user->name,u->name,level_name[user->level]);
        write_duty(WIZ,text,NULL,NULL,0);
        sprintf(text,"%s%sYou have been muzzled!\n",colors[CBOLD],colors[CSYSBOLD]);
        write_user(u,text);
        sprintf(text,"%s muzzled %s.\n",user->name,u->name);
        write_syslog(text,1,USERLOG);
        u->muzzled=user->level;
        return;
        }
/* User not logged on */
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in muzzle().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
        write_user(user,nosuchuser);  
        destruct_user(u);
        destructed=0;
        return;
        }
if (u->level>=user->level) {
        write_user(user,"You cannot muzzle a user of equal or higher level than yourself.\n");
        destruct_user(u);
        destructed=0;
        return;
        }
if (u->muzzled>=user->level) {
        sprintf(text,"%s is already muzzled.\n",u->name);
        write_user(user,text); 
        destruct_user(u);
        destructed=0;
        return;
        }
u->socket=-2;
u->muzzled=user->level;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"%s%s%s muzzled by level: ~RS%s%s.\n",colors[CBOLD],colors[CSYSBOLD],u->name,level_name[user->level],colors[CSYSTEM]);
write_user(user,text);
send_mail(user,word[1],"~OLYou have been muzzled.\n");
sprintf(text,"%s muzzled %s.\n",user->name,u->name);
write_syslog(text,1,USERLOG);
destruct_user(u);
destructed=0;
}



/*** Umuzzle the bastard now he's apologised and grovelled enough via email ***/
unmuzzle(user)
UR_OBJECT user;
{
UR_OBJECT u;

if (word_count<2) {
        write_user(user,"Usage: unmuzzle <user>\n");  return;
        }
if ((u=get_user(word[1]))!=NULL) {
        if (u==user) {
                write_user(user,"Even TRYING to unmuzzle yourself is just plain STUPID.\n");
                return;
                }
        if (!u->muzzled) {
                sprintf(text,"%s is not muzzled.\n",u->name);  return;
                }
        if (u->muzzled>user->level) {
                sprintf(text,"%s's muzzle is set to level %s, you do not have the power to remove it.\n",u->name,level_name[u->muzzled]);
                write_user(user,text);  return;
                }
        if (!user->duty) {
                sprintf(text,"%sYou remove %s's muzzle.\n",colors[CSYSTEM],u->name);
                write_user(user,text);
                }
        sprintf(text,"%s[ %s UNMUZZLED %s. ]\n",colors[CSYSTEM],user->name,u->name);
        write_duty(WIZ,text,NULL,NULL,0);
        write_user(u,"~FGYou have been unmuzzled!\n");
        sprintf(text,"%s unmuzzled %s.\n",user->name,u->name);
        write_syslog(text,1,USERLOG);
        u->muzzled=0;
        return;
        }
/* User not logged on */
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in unmuzzle().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
        write_user(user,nosuchuser);  
        destruct_user(u);
        destructed=0;
        return;
        }
if (u->muzzled>user->level) {
        sprintf(text,"%s's muzzle is set to level %s, you do not have the power to remove it.\n",u->name,level_name[u->muzzled]);
        write_user(user,text);  
        destruct_user(u);
        destructed=0;
        return;
        }
u->socket=-2;
u->muzzled=0;
strcpy(u->site,u->last_site);
save_user_details(u,0);
sprintf(text,"%sYou remove %s's muzzle.\n",colors[CSYSTEM],u->name);
write_user(user,text);
send_mail(user,word[1],"~OLYou have been unmuzzled.\n");
sprintf(text,"%s unmuzzled %s.\n",user->name,u->name);
write_syslog(text,1,USERLOG);
destruct_user(u);
destructed=0;
}



/*** Switch system logging on and off ***/
logging(user)
UR_OBJECT user;
{
if (system_logging) {
        write_user(user,"System logging ~FROFF.\n");
        sprintf(text,"%s switched system logging OFF.\n",user->name);
        write_syslog(text,1,SYSLOG);
        write_syslog(text,1,USERLOG);
        write_syslog(text,1,ACCREQLOG);
        write_syslog(text,1,FIRELOG);
        write_syslog(text,1,BANLOG);
        system_logging=0;
        return;
        }
system_logging=1;
write_user(user,"System logging ~FGON.\n");
sprintf(text,"%s switched system logging ON.\n",user->name);
write_syslog(text,1,SYSLOG);
write_syslog(text,1,USERLOG);
write_syslog(text,1,ACCREQLOG);
write_syslog(text,1,FIRELOG);
write_syslog(text,1,BANLOG);
}


/*** Set minlogin level ***/
minlogin(user)
UR_OBJECT user;
{
UR_OBJECT u,next;
char *usage="Usage: minlogin NONE/<user level>\n";
char levstr[5],*name;
int lev,cnt;

if (word_count<2) {
        write_user(user,usage);  return;
        }
if ((lev=get_level(word[1]))==-1) {
        if (strcmp(word[1],"NONE")) {
                write_user(user,usage);  return;
                }
        lev=-1;
        strcpy(levstr,"NONE");
        }
else strcpy(levstr,level_name[lev]);
if (lev>user->level) {
        write_user(user,"You cannot set minlogin to a higher level than your own.\n");
        return;
        }
if (minlogin_level==lev) {
        write_user(user,"That is the current MinLogin setting.\n");  return;
        }
minlogin_level=lev;
if (!user->duty) {
        sprintf(text,"Minlogin level set to: ~OL%s.\n",levstr);
        write_user(user,text);
        }
sprintf(text,"%s[ %s set MINLOGIN to: %s ]\n",colors[CSYSTEM],user->name,levstr);
write_duty(WIZ,text,NULL,NULL,0);
sprintf(text,"SYSTEM:  Minimum login level is now set at: ~OL%s.\n",levstr);
write_room_except(NULL,text,user);
sprintf(text,"%s set the minlogin level to %s.\n",user->name,levstr);
write_syslog(text,1,USERLOG);

/* Now boot off anyone below that level */
cnt=0;
u=user_first;
while(u) {
        next=u->next;
        if (!u->login && u->type!=CLONE_TYPE && u->level<lev) {
                write_user(u,"\n~FY~OLYour level is now below the minlogin level, disconnecting you...\n");
                disconnect_user(u);
                ++cnt;
                }
        u=next;
        }
sprintf(text,"Total of %d users were disconnected.\n",cnt);
destructed=0;
write_user(user,text);
}



/*** Show talker system parameters etc ***/
system_details(user)
UR_OBJECT user;
{
NL_OBJECT nl;
RM_OBJECT rm;
UR_OBJECT u;
PL_OBJECT plugin;
CM_OBJECT plcmd;
char bstr[40],minlogin[5];
char *ca[]={ "NONE  ","IGNORE","REBOOT" };
char plural[2][2]={"s",""};
int days,hours,mins,secs;
int netlinks,live,inc,outg;
int rms,inlinks,num_clones,mem,size;

sprintf(text,"\n%s%s-=- TalkerOS ver %s - System Status -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],TOS_VER);
write_user(user,text);

/* Get some values */
strcpy(bstr,ctime(&boot_time));
secs=(int)(time(0)-boot_time);
days=secs/86400;
hours=(secs%86400)/3600;
mins=(secs%3600)/60;
secs=secs%60;
num_clones=0;
mem=0;
size=sizeof(struct user_struct);
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE) num_clones++;
        mem+=size;
        }

rms=0;  
inlinks=0;
size=sizeof(struct room_struct);
for(rm=room_first;rm!=NULL;rm=rm->next) {
        if (rm->inlink) ++inlinks;
        ++rms;  mem+=size;
        }

netlinks=0;  
live=0;
inc=0; 
outg=0;
size=sizeof(struct netlink_struct);
for(nl=nl_first;nl!=NULL;nl=nl->next) {
        if (nl->type!=UNCONNECTED && nl->stage==UP) live++;
        if (nl->type==INCOMING) ++inc;
        if (nl->type==OUTGOING) ++outg;
        ++netlinks;  mem+=size;
        }
size=sizeof(struct plugin_struct);
for(plugin=plugin_first; plugin!=NULL; plugin=plugin->next) mem+=size;
size=sizeof(struct plugin_cmd);
for(plcmd=cmds_first; plcmd!=NULL; plcmd=plcmd->next) mem+=size;

if (minlogin_level==-1) strcpy(minlogin,"NONE");
else strcpy(minlogin,level_name[minlogin_level]);

/* Show header parameters */
sprintf(text,"~FTProcess ID   : ~FG%d\n~FTTalker booted: ~FG%s~FTUptime       : ~FG%d day%s, %d hour%s, %d minute%s, %d second%s\n",getpid(),bstr,days,plural[(days==1)],hours,plural[(hours==1)],mins,plural[(mins==1)],secs,plural[(secs==1)]);
write_user(user,text);
sprintf(text,"~FTSystem Ports : ~FGMAIN %d,  WIZ %d,  LINK %d\n\n",port[0],port[1],port[2]);
write_user(user,text);

/* Show others */
sprintf(text,"Maximum USERS          : %-3d          Current num. of USERS  : %d\n",max_users,num_of_users);
write_user(user,text);
sprintf(text,"Maximum CLONES         : %-2d           Current num. of CLONES : %d\n",max_clones,num_clones);
write_user(user,text);
sprintf(text,"Current MinLogin level : %-10s   Login idle time out    : %d secs.\n",minlogin,login_idle_time);
write_user(user,text);
sprintf(text,"User idle time out     : %-4d secs.   Heartbeat              : %d\n",user_idle_time,heartbeat);
write_user(user,text);
sprintf(text,"Remote user maxlevel   : %-10s   Remote user deflevel   : %s\n",level_name[rem_user_maxlevel],level_name[rem_user_deflevel]);
write_user(user,text);
sprintf(text,"Wizport min login level: %-10s   Gatecrash level        : %s\n",level_name[wizport_level],level_name[gatecrash_level]);
write_user(user,text);
sprintf(text,"Time out maxlevel      : %-10s   Private room min count : %d\n",level_name[time_out_maxlevel],min_private_users);
write_user(user,text);
sprintf(text,"Message lifetime       : %-2d days      Message check time     : %02d:%02d\n",mesg_life,mesg_check_hour,mesg_check_min);
write_user(user,text);
sprintf(text,"Net idle time out      : %-4d secs.   Number of rooms        : %d\n",net_idle_time,rms);
write_user(user,text);
sprintf(text,"Num. accepting connects: %-2d           Total netlinks         : %d\n",inlinks,netlinks);
write_user(user,text);
sprintf(text,"Number which are live  : %-2d           Number incoming        : %d\n",live,inc);
write_user(user,text);
sprintf(text,"Number outgoing        : %-2d           Ignoring sigterm       : %s\n",outg,noyes2[ignore_sigterm]);
write_user(user,text);
sprintf(text,"Echoing passwords      : %s          Swearing banned        : %s\n",noyes2[password_echo],noyes2[ban_swearing]);
write_user(user,text);
sprintf(text,"Time out afks          : %s          Allowing caps in name  : %s\n",noyes2[time_out_afks],noyes2[allow_caps_in_name]);
write_user(user,text);
sprintf(text,"New user prompt default: %s          New user color default : %s\n",offon[prompt_def],offon[colour_def]);
write_user(user,text);
sprintf(text,"New user charecho def. : %s          System logging         : %s\n",offon[charecho_def],offon[system_logging]);
write_user(user,text);
sprintf(text,"Crash action           : %s       Object memory allocated: %d\n",ca[crash_action],mem);
write_user(user,text);
sprintf(text,"Rejecting NEW Accounts : %s          Saving NEW Accounts    : %s\n",noyes2[nonewbies],noyes2[saveaccts]);
write_user(user,text);
sprintf(text,"Pueblo-Enhanced Mode   : %s          Personal Rooms Active  : %s\n",noyes2[pueblo_enh],noyes2[allowpr]);
write_user(user,text);
sprintf(text,"Num Logins Since Reboot: %-5d        # Newbies Autopromoted : %d\n",totlogins,totnewbies);
write_user(user,text);
write_user(user,"\n");
}


/*** Set the character mode echo on or off. This is only for users logging in
     via a character mode client, those using a line mode client (eg unix
     telnet) will see no effect. ***/
toggle_charecho(user)
UR_OBJECT user;
{
if (!user->charmode_echo) {
        write_user(user,"Echoing for character mode clients ~FGON.\n");
        user->charmode_echo=1;
        }
else {
        write_user(user,"Echoing for character mode clients ~FROFF.\n");
        user->charmode_echo=0;
        }
if (user->room==NULL) prompt(user);
}


/*** Free a hung socket ***/
clearline(user)
UR_OBJECT user;
{
UR_OBJECT u;
int sock;

if (word_count<2 || !isnumber(word[1])) {
        write_user(user,"Usage: clearline <line>\n");  return;
        }
sock=atoi(word[1]);

/* Find line amongst users */
for(u=user_first;u!=NULL;u=u->next) 
        if (u->type!=CLONE_TYPE && u->socket==sock) goto FOUND;
write_user(user,"That line is not currently active.\n");
return;

FOUND:
if (!u->login) {
        write_user(user,"You cannot clear the line of a logged in user.\n");
        return;
        }
write_user(u,"\n\n<!> This network connection is being cleared! - Sorry. <!>\n\n");
disconnect_user(u); 
sprintf(text,"%s cleared line %d.\n",user->name,sock);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s CLEARED LINE %d ]\n",colors[CSYSTEM],user->name,sock);
write_duty(WIZ,text,NULL,user,0);
sprintf(text,"Line %d cleared.\n",sock);
write_user(user,text);
destructed=0;
no_prompt=0;
}


/*** Change whether a rooms access is fixed or not ***/
change_room_fix(user,fix)
UR_OBJECT user;
int fix;
{
RM_OBJECT rm;
char *name;

if (word_count<2) rm=user->room;
else {
        if ((rm=get_room(word[1]))==NULL) {
                write_user(user,nosuchroom);  return;
                }
        }
if (rm->personal) { write_user(user,"You cannot change the access of a personal room.\n"); return; }
if (user->vis) name=user->name; else name=invisname;
if (fix) {      
        if (rm->access & FIXED) {
                if (rm==user->room) 
                        write_user(user,"This room's access is already fixed.\n");
                else write_user(user,"That room's access is already fixed.\n");
                return;
                }
        sprintf(text,"Access for room %s is now ~FRFIXED.\n",rm->name);
        write_user(user,text);
        if (user->room==rm) {
                sprintf(text,"%s has ~FRFIXED~RS access for this room.\n",name);
                write_room_except(rm,text,user);
                }
        else {
                sprintf(text,"This room's access has been ~FRFIXED.\n");
                write_room(rm,text);
                }
        sprintf(text,"%s FIXED access to room %s.\n",user->name,rm->name);
        write_syslog(text,1,SYSLOG);
        sprintf(text,"%s[ %s FIXED access to room %s. ]\n",colors[CSYSTEM],user->name,rm->name);
        write_duty(WIZ,text,NULL,user,0);
        rm->access+=2;
        return;
        }
if (!(rm->access & FIXED)) {
        if (rm==user->room) 
                write_user(user,"This room's access is already unfixed.\n");
        else write_user(user,"That room's access is already unfixed.\n");
        return;
        }
sprintf(text,"Access for room %s is now ~FGUNFIXED.\n",rm->name);
write_user(user,text);
if (user->room==rm) {
        sprintf(text,"%s has ~FGUNFIXED~RS access for this room.\n",name);
        write_room_except(rm,text,user);
        }
else {
        sprintf(text,"This room's access has been ~FGUNFIXED.\n");
        write_room(rm,text);
        }
sprintf(text,"%s UNFIXED access to room %s.\n",user->name,rm->name);
write_syslog(text,1,SYSLOG);
sprintf(text,"%s[ %s UNFIXED access to room %s. ]\n",colors[CSYSTEM],user->name,rm->name);
write_duty(WIZ,text,NULL,user,0);
rm->access-=2;
reset_access(rm);
}



/*** View the system log ***/
viewlog(user)
UR_OBJECT user;
{
FILE *fp;
char c,*emp="The log is empty.\n";
int lines,cnt,cnt2,log;

if (word_count<2) {
        write_user(user,"Usage:  viewlog <sys/user/accreq/firewall/ban> [<lines from end>]\n");
        return;
        }
if (!strncmp(word[1],"s",1)) log=0;
        else if (!strncmp(word[1],"u",1)) log=1;
                else if (!strncmp(word[1],"a",1)) log=2;
                        else if (!strncmp(word[1],"f",1)) log=3;
                                else if (!strncmp(word[1],"b",1)) log=4;
                                        else {
                                                write_user(user,"Usage:  viewlog <sys/user/accreq/firewall/ban> [<lines from end>]\n");
                                                return;
                                                }

if (word_count==2) {
        if (log==0) {
                sprintf(text,"\n%s%s-=- System Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
                write_user(user,text);
                switch(more(user,user->socket,SYSLOG)) {
                        case 0: write_user(user,emp);  return;
                        case 1: user->misc_op=2; 
                        }
                return;
                }
        if (log==1) {
                sprintf(text,"\n%s%s-=- User Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
                write_user(user,text);
                switch(more(user,user->socket,USERLOG)) {
                        case 0: write_user(user,emp);  return;
                        case 1: user->misc_op=2; 
                        }
                return;
                }
        if (log==2) {
                sprintf(text,"\n%s%s-=- Account Request Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
                write_user(user,text);
                switch(more(user,user->socket,ACCREQLOG)) {
                        case 0: write_user(user,emp);  return;
                        case 1: user->misc_op=2; 
                        }
                return;
                }
        if (log==3) {
                sprintf(text,"\n%s%s-=- Talker Firewall Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
                write_user(user,text);
                switch(more(user,user->socket,FIRELOG)) {
                        case 0: write_user(user,emp);  return;
                        case 1: user->misc_op=2; 
                        }
                return;
                }
        if (log==4) {
                sprintf(text,"\n%s%s-=- System Ban Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
                write_user(user,text);
                switch(more(user,user->socket,BANLOG)) {
                        case 0: write_user(user,emp);  return;
                        case 1: user->misc_op=2; 
                        }
                return;
                }
        }
if ((lines=atoi(word[2]))<1) {
        write_user(user,"Usage: viewlog <sys/user/accreq/firewall/ban> [<lines from the end>]\n");  return;
        }
/* Count total lines */
if (log==0) if (!(fp=fopen(SYSLOG,"r"))) {  write_user(user,emp);  return;  }
if (log==1) if (!(fp=fopen(USERLOG,"r"))) {  write_user(user,emp);  return;  }
if (log==2) if (!(fp=fopen(ACCREQLOG,"r"))) {  write_user(user,emp);  return;  }
if (log==3) if (!(fp=fopen(FIRELOG,"r"))) {  write_user(user,emp);  return;  }
if (log==4) if (!(fp=fopen(BANLOG,"r"))) { write_user(user,emp);  return; }
cnt=0;

c=getc(fp);
while(!feof(fp)) {
        if (c=='\n') ++cnt;
        c=getc(fp);
        }
if (cnt<lines) {
        sprintf(text,"There are only %d lines in the log.\n",cnt);
        write_user(user,text);
        fclose(fp);
        return;
        }
if (cnt==lines && log==0) {
        sprintf(text,"\n%s%s-=- System Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        fclose(fp);  more(user,user->socket,SYSLOG);  return;
        }
if (cnt==lines && log==1) {
        sprintf(text,"\n%s%s-=- User Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        fclose(fp);  more(user,user->socket,USERLOG);  return;
        }
if (cnt==lines && log==2) {
        sprintf(text,"\n%s%s-=- Account Request Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        fclose(fp);  more(user,user->socket,ACCREQLOG);  return;
        }
if (cnt==lines && log==3) {
        sprintf(text,"\n%s%s-=- Talker Firewall Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        fclose(fp);  more(user,user->socket,FIRELOG);  return;
        }
if (cnt==lines && log==4) {
        sprintf(text,"\n%s%s-=- System Ban Log -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        fclose(fp);  more(user,user->socket,BANLOG);  return;
        }

/* Find line to start on */
fseek(fp,0,0);
cnt2=0;
c=getc(fp);
while(!feof(fp)) {
        if (c=='\n') ++cnt2;
        c=getc(fp);
        if (cnt2==cnt-lines) {
                if (log==0) sprintf(text,"\n%s%s-=- System Log (last %d lines) -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],lines);
                if (log==1) sprintf(text,"\n%s%s-=- User Log (last %d lines) -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],lines);
                if (log==2) sprintf(text,"\n%s%s-=- Account Request Log (last %d lines) -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],lines);
                if (log==3) sprintf(text,"\n%s%s-=- Firewall Log (last %d lines) -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],lines);
                if (log==4) sprintf(text,"\n%s%s-=- System Ban Log (last %d lines) -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT],lines);
                write_user(user,text);
                user->filepos=ftell(fp)-1;
                fclose(fp);
                if (log==0) {
                       if (more(user,user->socket,SYSLOG)!=1) user->filepos=0;
                       else user->misc_op=2;
                       return;
                       }
                if (log==1) {
                       if (more(user,user->socket,USERLOG)!=1) user->filepos=0;
                       else user->misc_op=2;
                       return;
                       }
                if (log==2) {
                       if (more(user,user->socket,ACCREQLOG)!=1) user->filepos=0;
                       else user->misc_op=2;
                       return;
                       }
                if (log==3) {
                       if (more(user,user->socket,FIRELOG)!=1) user->filepos=0;
                       else user->misc_op=2;
                       return;
                       }
                if (log==4) {
                       if (more(user,user->socket,BANLOG)!=1) user->filepos=0;
                       else user->misc_op=2;
                       return;
                       }
                }
        }
fclose(fp);
sprintf(text,"%s: Line count error.\n",syserror);
write_user(user,text);
write_syslog("ERROR: Line count error in viewlog().\n",0,SYSLOG);
}


/*** A newbie is requesting an account. Get his email address off him so we
     can validate who he is before we promote him and let him loose as a 
     proper user. ***/
account_request(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (user->level>NEW) {
        write_user(user,"This command is for new users only, you already have a full account.\n");
        return;
        }
/* This is so some pillock doesnt keep doing it just to fill up the syslog */
if (user->accreq) {
        write_user(user,"You have already requested an account.\n");
        return;
        }
if (!user->readrules && req_read_rules) {
        write_user(user,"You must read the .rules before requesting an account.\n");
        return;
        }
if (word_count<2) {
        write_user(user,"Usage: accreq <an email address we can contact you on + any relevent info>\n");
        return;
        }
/* Could check validity of email address I guess but its a waste of time.
   If they give a duff address they don't get an account, simple. ***/
sprintf(text,"ACCOUNT REQUEST from %-12s : %s\n",user->name,inpstr);
write_syslog(text,1,ACCREQLOG);
sprintf(text,"%s[SYSTEM:  %s has made a request for an account.]\n",colors[CSYSTEM],user->name);
write_duty(WIZ,text,NULL,NULL,0);
write_user(user,"Account request logged.\n");
if (autopromote) {
        user->accreq=1;
        user->level++;
        user->orig_level++;
        user->cloaklev++;
        sprintf(text,"%s[SYSTEM:  %s has been AUTO-PROMOTED.]\n",colors[CSYSTEM],user->name);
        write_duty(WIZ,text,NULL,NULL,0);
        write_user(user,"You have been AUTOPROMOTED to level: ");
        write_user(user,level_name[user->level]);
        write_user(user,"\n");
        totnewbies++;
        }
if (user->orig_level>0 || saveaccts) save_user_details(user,1);
write_user(user,"NOTE:  By requesting an account, you are acknowledging the fact that you\n       have read and will abide by the ~FG.rules~RS of this talker.  You are\n       also acknowledging the fact that you will be held responsible for\n       ANY actions done using your account.  If you have a problem with any\n       of this, talk to an admin or ~FG.suicide~RS your account now.\n");
}


/*** Clear the review buffer ***/
revclr(user)
UR_OBJECT user;
{
char *name;

clear_revbuff(user->room); 
write_user(user,"Review buffer cleared.\n");
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"%s has cleared the review buffer.\n",name);
write_room_except(user->room,text,user);
}


/*** Clone a user in another room ***/
create_clone(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;
int cnt;

/* Check room */
if (word_count<2) rm=user->room;
else {
        if ((rm=get_room(word[1]))==NULL) {
                write_user(user,nosuchroom);  return;
                }
        }       
/* If room is private then nocando */
if (!has_room_access(user,rm)) {
        write_user(user,"That room is currently private, you cannot create a clone there.\n");  
        return;
        }
/* Count clones and see if user already has a copy there , no point having 
   2 in the same room */
cnt=0;
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE && u->owner==user) {
                if (u->room==rm) {
                        sprintf(text,"You already have a clone in the %s.\n",rm->name);
                        write_user(user,text);
                        return;
                        }       
                if (++cnt==max_clones) {
                        write_user(user,"You already have the maximum number of clones allowed.\n");
                        return;
                        }
                }
        }
/* Create clone */
if ((u=create_user())==NULL) {          
        sprintf(text,"%s: Unable to create copy.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create user copy in clone().\n",0,SYSLOG);
        return;
        }
u->type=CLONE_TYPE;
u->socket=user->socket;
u->room=rm;
u->owner=user;
strcpy(u->name,user->name);
strcpy(u->desc,"~BR(CLONE)");

if (rm==user->room)
        write_user(user,"( You have created a clone in this room. )\n");
else {
        sprintf(text,"( You have created a clone in the %s. )\n",rm->name);
        write_user(user,text);
        }
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"%sA clone of %s appears!\n",colors[CWARNING],user->name);
write_room_except(rm,text,user);
}


/*** Destroy user clone ***/
destroy_clone(user)
UR_OBJECT user;
{
UR_OBJECT u,u2;
RM_OBJECT rm;
char *name;

/* Check room and user */
if (word_count<2) rm=user->room;
else {
        if ((rm=get_room(word[1]))==NULL) {
                write_user(user,nosuchroom);  return;
                }
        }
if (word_count>2) {
        if ((u2=get_user(word[2]))==NULL) {
                write_user(user,notloggedon);  return;
                }
        if (u2->level>=user->level) {
                write_user(user,"You cannot destroy the clone of a user of an equal or higher level.\n");
                return;
                }
        }
else u2=user;
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE && u->room==rm && u->owner==u2) {
                destruct_user(u);
                reset_access(rm);
                write_user(user,"( You have destroyed the clone. )\n");
                if (user->vis) name=user->name; else name=invisname;
                sprintf(text,"%sThe clone of %s vanishes.\n",colors[CWARNING],u2->name);
                write_room(rm,text);
                if (u2!=user) {
                        sprintf(text,"~OLSYSTEM: ~FR%s has destroyed your clone in the %s.\n",user->name,rm->name);
                        write_user(u2,text);
                        }
                destructed=0;
                return;
                }
        }
if (u2==user) sprintf(text,"You do not have a clone in the %s.\n",rm->name);
else sprintf(text,"%s does not have a clone the %s.\n",u2->name,rm->name);
write_user(user,text);
}


/*** Show users own clones ***/
myclones(user)
UR_OBJECT user;
{
UR_OBJECT u;
int cnt;

cnt=0;
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type!=CLONE_TYPE || u->owner!=user) continue;
        if (++cnt==1) 
                sprintf(text,"\n%s-=- Your Clone Locations -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
                write_user(user,text);
        sprintf(text,"  %s\n",u->room);
        write_user(user,text);
        }
if (!cnt) write_user(user,"You have no clones.\n");
else {
        sprintf(text,"\nTotal of %d clones.\n\n",cnt);
        write_user(user,text);
        }
}


/*** Show all clones on the system ***/
allclones(user)
UR_OBJECT user;
{
UR_OBJECT u;
int cnt;

cnt=0;
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type!=CLONE_TYPE) continue;
        if (++cnt==1) {
                sprintf(text,"\n%s%s-=- All Clones -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
                write_user(user,text);
                }
        sprintf(text,"%-15s : %s\n",u->name,u->room);
        write_user(user,text);
        }
if (!cnt) write_user(user,"There are no clones on the system.\n");
else {
        sprintf(text,"\nTotal of %d clones.\n\n",cnt);
        write_user(user,text);
        }
}


/*** User swaps places with his own clone. All we do is swap the rooms the
        objects are in. ***/
clone_switch(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;

if (word_count<2) {
        write_user(user,"Usage: switch <room clone is in>\n");  return;
        }
if ((rm=get_room(word[1]))==NULL) {
        write_user(user,nosuchroom);  return;
        }
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE && u->room==rm && u->owner==user) {
                write_user(user,"\n~FB~OLYou experience a strange sensation...\n");
                u->room=user->room;
                user->room=rm;
                sprintf(text,"The clone of %s comes alive!\n",u->name);
                write_room_except(user->room,text,user);
                sprintf(text,"%s turns into a clone!\n",u->name);
                write_room_except(u->room,text,u);
                look(user);
                return;
                }
        }
write_user(user,"You do not have a clone in that room.\n");
}


/*** Make a clone speak ***/
clone_say(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
RM_OBJECT rm;
UR_OBJECT u;

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");
        return;
        }
if (word_count<3) {
        write_user(user,"Usage: csay <room clone is in> <message>\n");
        return;
        }
if ((rm=get_room(word[1]))==NULL) {
        write_user(user,nosuchroom);  return;
        }
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE && u->room==rm && u->owner==user) {
                say(u,remove_first(inpstr));  return;
                }
        }
write_user(user,"You do not have a clone in that room.\n");
}


/*** Set what a clone will hear, either all speach , just bad language
        or nothing. ***/
clone_hear(user)
UR_OBJECT user;
{
RM_OBJECT rm;
UR_OBJECT u;

if (word_count<3  
    || (strncmp(word[2],"a",1) 
            && strncmp(word[2],"s",1) 
            && strncmp(word[2],"n",1))) {
        write_user(user,"Usage: chear <room clone is in> all/swears/nothing\n");
        return;
        }
if ((rm=get_room(word[1]))==NULL) {
        write_user(user,nosuchroom);  return;
        }
for(u=user_first;u!=NULL;u=u->next) {
        if (u->type==CLONE_TYPE && u->room==rm && u->owner==user) break;
        }
if (u==NULL) {
        write_user(user,"You do not have a clone in that room.\n");
        return;
        }
if (!strncmp(word[2],"a",1)) {
        u->clone_hear=CLONE_HEAR_ALL;
        write_user(user,"Clone will now hear everything.\n");
        return;
        }
if (!strncmp(word[2],"s",1)) {
        u->clone_hear=CLONE_HEAR_SWEARS;
        write_user(user,"Clone will now only hear swearing.\n");
        return;
        }
u->clone_hear=CLONE_HEAR_NOTHING;
write_user(user,"Clone will now hear nothing.\n");
}


/*** Stat a remote system ***/
remote_stat(user)
UR_OBJECT user;
{
NL_OBJECT nl;
RM_OBJECT rm;

if (word_count<2) {
        write_user(user,"Usage: rstat <room service is linked to>\n");  return;
        }
if ((rm=get_room(word[1]))==NULL) {
        write_user(user,nosuchroom);  return;
        }
if ((nl=rm->netlink)==NULL) {
        write_user(user,"That room is not linked to a service.\n");
        return;
        }
if (nl->stage!=2) {
        write_user(user,"Not (fully) connected to service.\n");
        return;
        }
if (nl->ver_major<=3 && nl->ver_minor<1) {
        write_user(user,"The NUTS version running that service does not support this facility.\n");
        return;
        }
sprintf(text,"RSTAT %s\n",user->name);
write_sock(nl->socket,text);
write_user(user,"Request sent.\n");
}


/*** Switch swearing ban on and off ***/
swban(user)
UR_OBJECT user;
{
if (!ban_swearing) {
        write_user(user,"Swearing ban ~FGON.\n");
        sprintf(text,"%s switched swearing ban ON.\n",user->name);
        write_syslog(text,1,USERLOG);
        ban_swearing=1;  return;
        }
write_user(user,"Swearing ban ~FROFF.\n");
sprintf(text,"%s switched swearing ban OFF.\n",user->name);
write_syslog(text,1,USERLOG);
ban_swearing=0;
}


/*** Do AFK ***/
afk(user,inpstr,automatic)
UR_OBJECT user;
char *inpstr;
int automatic;
{
if (word_count>1 && !automatic) {
        if (!strcmp(word[1],"lock")) {
                if (user->type==REMOTE_TYPE) {
                        /* This is because they might not have a local account and hence
                           they have no password to use. */
                        write_user(user,"Sorry, due to software limitations remote users cannot use the lock option.\n");
                        return;
                        }
                inpstr=remove_first(inpstr);
                if (strlen(inpstr)>AFK_MESG_LEN) {
                        write_user(user,"AFK message too long.\n");  return;
                        }
                write_user(user,"You are now AFK with the session locked, enter your password to unlock it.\n");
                echo_off(user);  /* turn off password echo */
                if (inpstr[0]) {
                        strcpy(user->afk_mesg,inpstr);
                        write_user(user,"AFK message set.\n");
                        }
                user->afk=2;
                }
        else {
                if (strlen(inpstr)>AFK_MESG_LEN) {
                        write_user(user,"AFK message too long.\n");  return;
                        }
                write_user(user,"You are now AFK, press <return> to reset.\n");
                if (inpstr[0]) {
                        strcpy(user->afk_mesg,inpstr);
                        write_user(user,"AFK message set.\n");
                        }
                user->afk=1;
                }
        }
else {
        if (!automatic) write_user(user,"You are now AFK, press <return> to reset.\n");
        if (automatic) write_user(user,"You have automatically been placed AFK.  Press <return> to reset.\n");
        user->afk=1;
        }
if (user->afk_mesg[0]) sprintf(text,"%s goes AFK: %s\n",user->name,user->afk_mesg);
        else {  if (!automatic) sprintf(text,"%s goes AFK...\n",user->name);
                if  (automatic) sprintf(text,"%s automatically goes AFK...\n",user->name);
                }
if (user->vis) write_room_except(user->room,text,user);
        else write_duty(user->level,text,user->room,user,0);
}


/*** Toggle user colour on and off ***/
toggle_colour(user)
UR_OBJECT user;
{
int col;

if (user->colour) {
        user->colour=0;
        write_user(user,"Color OFF.\n");  
        }
else {
        user->colour=1;  
        write_user(user,"Color ~FGON.\n");
        }
if (user->room==NULL) prompt(user);
}


toggle_ignshout(user)
UR_OBJECT user;
{
if (user->ignshout) {
        write_user(user,"You are no longer ignoring shouts and shout emotes.\n");  
        user->ignshout=0;
        return;
        }
write_user(user,"You are now ignoring shouts and shout emotes.\n");
user->ignshout=1;
}


toggle_igntell(user)
UR_OBJECT user;
{
if (user->igntell) {
        write_user(user,"You are no longer ignoring tells and private emotes.\n");  
        user->igntell=0;
        return;
        }
write_user(user,"You are now ignoring tells and private emotes.\n");
user->igntell=1;
}


suicide(user)
UR_OBJECT user;
{
if (word_count<2) {
        write_user(user,"Usage: suicide <your password>\n");  return;
        }
if (strcmp((char *)crypt(word[1],"NU"),user->pass)) {
        write_user(user,"Password incorrect.\n");  return;
        }
audioprompt(user,4,0);  /* Audio warning */
write_user(user,"\n\07~FR~OL~LI*** WARNING - This will delete your account! ***\n\nAre you sure about this (y/n)? ");
user->misc_op=6;
no_prompt=1;
}


/*** Delete a user ***/
delete_user(user,this_user)
UR_OBJECT user;
int this_user;
{
UR_OBJECT u;
char filename[80],name[USER_NAME_LEN+1];
FILE *fp;

if (this_user) {
        /* User structure gets destructed in disconnect_user(), need to keep a
           copy of the name */
        strcpy(name,user->name); 
        write_user(user,"\n~FR~LI~OLACCOUNT DELETED!\n");
        sprintf(text,"~OL~LI%s commits suicide!\n",user->name);
        write_room_except(user->room,text,user);
        sprintf(text,"%s SUICIDED.\n",name);
        write_syslog(text,1,USERLOG);
        disconnect_user(user);
        sprintf(filename,"%s/%s.D",USERFILES,name);
        unlink(filename);
        sprintf(filename,"%s/%s.M",USERFILES,name);
        unlink(filename);
        sprintf(filename,"%s/%s.P",USERFILES,name);
        unlink(filename);
        sprintf(filename,"%s/%s.R",USERFILES,name);
        unlink(filename);
        return;
        }
if (word_count<2) {
        write_user(user,"Usage: delete <user>\n");  return;
        }
word[1][0]=toupper(word[1][0]);
if (!strcmp(word[1],user->name)) {
        write_user(user,"If you deleted yourself, you'd implode.  Do you really want that?!\n");
        return;
        }
if (get_user(word[1])!=NULL) {
        /* Safety measure just in case. Will have to .kill them first */
        write_user(user,"You cannot delete a user who is currently logged on.\n");
        return;
        }
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in delete_user().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
        write_user(user,nosuchuser);  
        destruct_user(u);
        destructed=0;
        return;
        }
if (u->level>=user->level) {
        write_user(user,"You cannot delete a user of an equal or higher level than yourself.\n");
        destruct_user(u);
        destructed=0;
        return;
        }
sprintf(filename,"%s/%s.bot",BOTFILES,u->name);
destruct_user(u);
destructed=0;
if (fp=fopen(filename,"r")) {
        fclose(fp);
        write_user(user,"You cannot use this command to delete a bot.  You must use .bot delete\n");
        return;
        }
sprintf(filename,"%s/%s.D",USERFILES,word[1]);
unlink(filename);
sprintf(filename,"%s/%s.M",USERFILES,word[1]);
unlink(filename);
sprintf(filename,"%s/%s.P",USERFILES,word[1]);
unlink(filename);
sprintf(filename,"%s/%s.R",USERFILES,word[1]);
unlink(filename);
sprintf(text,"\07~FR~OL~LIUser %s deleted!\n",word[1]);
write_user(user,text);
sprintf(text,"%s DELETED %s.\n",user->name,word[1]);
write_syslog(text,1,SYSLOG);
sprintf(text,"%s[ %s DELETED user %s! ]\n",colors[CSYSTEM],user->name,word[1]);
write_duty(user->level,text,NULL,user,0);
}


/*** Shutdown talker interface func. Countdown time is entered in seconds so
        we can specify less than a minute till reboot. ***/
shutdown_com(user)
UR_OBJECT user;
{
if (rs_which==1) {
        write_user(user,"The reboot countdown is currently active, you must cancel it first.\n");
        return;
        }
if (!strcmp(word[1],"cancel")) {
        if (!rs_countdown || rs_which!=0) {
                write_user(user,"The shutdown countdown is not currently active.\n");
                return;
                }
        if (rs_countdown && !rs_which && rs_user==NULL) {
                write_user(user,"Someone else is currently setting the shutdown countdown.\n");
                return;
                }
        write_room(NULL,"~OLSYSTEM:~RS~FG Shutdown cancelled.\n");
        sprintf(text,"%s cancelled the shutdown countdown.\n",user->name);
        write_syslog(text,1,USERLOG);
        rs_countdown=0;
        rs_announce=0;
        rs_which=-1;
        rs_user=NULL;
        return;
        }
if (word_count>1 && !isnumber(word[1])) {
        write_user(user,"Usage: shutdown [<secs>/cancel]\n");  return;
        }
if (rs_countdown && !rs_which) {
        write_user(user,"The shutdown countdown is currently active, you must cancel it first.\n");
        return;
        }
if (word_count<2) {
        rs_countdown=0;  
        rs_announce=0;
        rs_which=-1; 
        rs_user=NULL;
        }
else {
        rs_countdown=atoi(word[1]);
        rs_which=0;
        }
audioprompt(user,4,0);  /* Audio warning */
write_user(user,"\n\07~FR~OL~LI*** WARNING - This will shutdown the talker! ***\n\nAre you sure about this (y/n)? ");
user->misc_op=1;  
no_prompt=1;  
}


/*** Reboot talker interface func. ***/
reboot_com(user)
UR_OBJECT user;
{
if (!rs_which) {
        write_user(user,"The shutdown countdown is currently active, you must cancel it first.\n");
        return;
        }
if (!strcmp(word[1],"cancel")) {
        if (!rs_countdown) {
                write_user(user,"The reboot countdown is not currently active.\n");
                return;
                }
        if (rs_countdown && rs_user==NULL) {
                write_user(user,"Someone else is currently setting the reboot countdown.\n");
                return;
                }
        write_room(NULL,"~OLSYSTEM:~RS~FG Reboot cancelled.\n");
        sprintf(text,"%s cancelled the reboot countdown.\n",user->name);
        write_syslog(text,1,USERLOG);
        rs_countdown=0;
        rs_announce=0;
        rs_which=-1;
        rs_user=NULL;
        return;
        }
if (word_count>1 && !isnumber(word[1])) {
        write_user(user,"Usage: reboot [<secs>/cancel]\n");  return;
        }
if (rs_countdown) {
        write_user(user,"The reboot countdown is currently active, you must cancel it first.\n");
        return;
        }
if (word_count<2) {
        rs_countdown=0;  
        rs_announce=0;
        rs_which=-1; 
        rs_user=NULL;
        }
else {
        rs_countdown=atoi(word[1]);
        rs_which=1;
        }
audioprompt(user,4,0);  /* Audio warning */
write_user(user,"\n\07~FY~OL~LI*** WARNING - This will reboot the talker! ***\n\nAre you sure about this (y/n)? ");
user->misc_op=7;  
no_prompt=1;  
}



/*** Show recorded tells and pemotes ***/
revtell(user)
UR_OBJECT user;
{
int i,cnt,line;

cnt=0;
for(i=0;i<REVTELL_LINES;++i) {
        line=(user->revline+i)%REVTELL_LINES;
        if (user->revbuff[line][0]) {
                cnt++;
                if (cnt==1) write_user(user,"\n~BB~FG*** Private Message Review Buffer ***\n\n");
                write_user(user,user->revbuff[line]); 
                }
        }
if (!cnt) write_user(user,"Private buffer is empty.\n");
else write_user(user,"\n~BB~FG*** End ***\n\n");
user->primsg=0;
}



/**************************** EVENT FUNCTIONS ******************************/

void do_events()
{
set_date_time();
check_reboot_shutdown();
check_idle_and_timeout();
check_nethangs_send_keepalives(); 
check_messages(NULL,0);
plugin_triggers(NULL,"");
reset_alarm();
}


reset_alarm()
{
signal(SIGALRM,do_events);
alarm(heartbeat);
}



/*** See if timed reboot or shutdown is underway ***/
check_reboot_shutdown()
{
int secs;
char *w[]={ "~FRShutdown","~FYRebooting" };
char plural[2][2]={"s",""};

if (rs_user==NULL) return;
rs_countdown-=heartbeat;
if (rs_countdown<=0) talker_shutdown(rs_user,NULL,rs_which);

/* Print countdown message every minute unless we have less than 1 minute
   to go when we print every 10 secs */
secs=(int)(time(0)-rs_announce);
if (rs_countdown>=60 && secs>=60) {
        sprintf(text,"~OLSYSTEM: %s in %d minute%s, %d second%s.\n",w[rs_which],rs_countdown/60,plural[((rs_countdown/60)==1)],rs_countdown%60,plural[((rs_countdown%60)==1)]);
        write_room(NULL,text);
        rs_announce=time(0);
        }
if (rs_countdown<60 && secs>=10) {
        sprintf(text,"~OLSYSTEM: %s in %d second%s.\n",w[rs_which],rs_countdown,plural[(rs_countdown==1)]);
        write_room(NULL,text);
        rs_announce=time(0);
        }
}



/*** login_time_out is the length of time someone can idle at login, 
     user_idle_time is the length of time they can idle once logged in. 
     Also ups users total login time. ***/
check_idle_and_timeout()
{
UR_OBJECT user,next;
int tm;

/* Use while loop here instead of for loop for when user structure gets
   destructed, we may lose ->next link and crash the program */
user=user_first;
while(user) {
        next=user->next;
        if (user->type==CLONE_TYPE || user->type==BOT_TYPE) {
                if (user->type==BOT_TYPE) user->total_login+=heartbeat;
                user=next;  continue;
                }
        user->total_login+=heartbeat; 

        tm=(int)(time(0) - user->last_input);
                
        /* If the AutoAFK feature is on and the user has idled for 1/3
           of the idle timeout, the system will automatically place them
           AFK. */
        if (!user->waitfor[0] && auto_afk && !user->afk && tm>=(user_idle_time/3))      afk(user,"",1);

        if (user->level>time_out_maxlevel) {  user=next;  continue;  }

        if (user->login && tm>=login_idle_time) {
                write_user(user,"\n\n*** Time out ***\n\n");
                sprintf(text,"%s<>~RS %sLogin timeout. ~RS%s(%s:%d)\n",colors[CWARNING],colors[CSYSBOLD],colors[CSYSTEM],user->site,user->site_port);
                write_duty(ARCH,text,NULL,NULL,2);
                disconnect_user(user);
                user=next;
                continue;
                }
        if (user->warned) {
                if (tm<user_idle_time-60) {  user->warned=0;  continue;  }
                if (tm>=user_idle_time) {
                        write_user(user,"\n\n\07~FR~OL~LI*** You have been timed out. ***\n\n");
                        disconnect_user(user);
                        user=next;
                        continue;
                        }
                }
        if ((!user->afk || (user->afk && time_out_afks)) 
            && !user->login 
            && !user->warned
            && tm>=user_idle_time-60) {
                audioprompt(user,4,0);  /* Audio warning */
                write_user(user,"\n\07~FY~OL~LI*** WARNING - Input within 1 minute or you will be disconnected. ***\n\n");
                user->warned=1;
                }

        user=next;
        }
}
        


/*** See if any net connections are dragging their feet. If they have been idle
     longer than net_idle_time the drop them. Also send keepalive signals down
     links, this saves having another function and loop to do it. ***/
check_nethangs_send_keepalives()
{
NL_OBJECT nl;
int secs;

for(nl=nl_first;nl!=NULL;nl=nl->next) {
        if (nl->type==UNCONNECTED) {
                nl->warned=0;  continue;
                }

        /* Send keepalives */
        nl->keepalive_cnt+=heartbeat;
        if (nl->keepalive_cnt>=keepalive_interval) {
                write_sock(nl->socket,"KA\n");
                nl->keepalive_cnt=0;
                }

        /* Check time outs */
        secs=(int)(time(0) - nl->last_recvd);
        if (nl->warned) {
                if (secs<net_idle_time-60) nl->warned=0;
                else {
                        if (secs<net_idle_time) continue;
                        sprintf(text,"~OLSYSTEM:~RS Disconnecting hung netlink to %s in the %s.\n",nl->service,nl->connect_room->name);
                        write_room(NULL,text);
                        shutdown_netlink(nl);
                        nl->warned=0;
                        }
                continue;
                }
        if (secs>net_idle_time-60) {
                sprintf(text,"~OLSYSTEM:~RS Netlink to %s in the %s has been hung for %d seconds.\n",nl->service,nl->connect_room->name,secs);
                write_duty(ARCH,text,NULL,NULL,0);
                nl->warned=1;
                }
        }
destructed=0;
}



/*** Remove any expired messages from boards unless force = 2 in which case
        just do a recount. ***/
check_messages(user,force)
UR_OBJECT user;
int force;
{
RM_OBJECT rm;
FILE *infp,*outfp;
char id[82],filename[80],line[82];
int valid,pt,write_rest;
int board_cnt,old_cnt,bad_cnt,tmp;
static int done=0;

switch(force) {
        case 0:
        if (mesg_check_hour==thour && mesg_check_min==tmin) {
                if (done) return;
                }
        else {  done=0;  return;  }
        break;

        case 1:
        printf("Checking boards...\n");
        }
done=1;
board_cnt=0;
old_cnt=0;
bad_cnt=0;

for(rm=room_first;rm!=NULL;rm=rm->next) {
        tmp=rm->mesg_cnt;  
        rm->mesg_cnt=0;
        sprintf(filename,"%s/%s.B",DATAFILES,rm->name);
        if (!(infp=fopen(filename,"r"))) continue;
        if (force<2) {
                if (!(outfp=fopen("tempfile","w"))) {
                        if (force) fprintf(stderr,"NUTS: Couldn't open tempfile.\n");
                        write_syslog("ERROR: Couldn't open tempfile in check_messages().\n",0,SYSLOG);
                        fclose(infp);
                        return;
                        }
                }
        board_cnt++;
        /* We assume that once 1 in date message is encountered all the others
           will be in date too , hence write_rest once set to 1 is never set to
           0 again */
        valid=1; write_rest=0;
        fgets(line,82,infp); /* max of 80+newline+terminator = 82 */
        while(!feof(infp)) {
                if (*line=='\n') valid=1;
                sscanf(line,"%s %d",id,&pt);
                if (!write_rest) {
                        if (valid && !strcmp(id,"PT:")) {
                                if (force==2) rm->mesg_cnt++;
                                else {
                                        /* 86400 = num. of secs in a day */
                                        if ((int)time(0) - pt < mesg_life*86400) {
                                                fputs(line,outfp);
                                                rm->mesg_cnt++;
                                                write_rest=1;
                                                }
                                        else old_cnt++;
                                        }
                                valid=0;
                                }
                        }
                else {
                        fputs(line,outfp);
                        if (valid && !strcmp(id,"PT:")) {
                                rm->mesg_cnt++;  valid=0;
                                }
                        }
                fgets(line,82,infp);
                }
        fclose(infp);
        if (force<2) {
                fclose(outfp);
                unlink(filename);
                if (!write_rest) unlink("tempfile");
                else rename("tempfile",filename);
                }
        if (rm->mesg_cnt!=tmp) bad_cnt++;
        }
switch(force) {
        case 0:
        if (bad_cnt) 
                sprintf(text,"CHECK_MESSAGES: %d files checked, %d had an incorrect message count, %d messages deleted.\n",board_cnt,bad_cnt,old_cnt);
        else sprintf(text,"CHECK_MESSAGES: %d files checked, %d messages deleted.\n",board_cnt,old_cnt);
        write_syslog(text,1,SYSLOG);
        break;

        case 1:
        printf("  %d board files checked, %d out of date messages found.\n",board_cnt,old_cnt);
        break;

        case 2:
        sprintf(text,"%d board files checked, %d had an incorrect message count.\n",board_cnt,bad_cnt);
        write_user(user,text);
        sprintf(text,"%s forced a recount of the message boards.\n",user->name);
        write_syslog(text,1,SYSLOG);
        }
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
   T a l k e r O S    S y s t e m   C o d e
   ----------------------------------------
   This is the heart of the TalkerOS modular design.  Please do not
   make any changes to this section other than adding plugin initialization
   calls and command calls.  Changing anything might result in the talker
   producing undesired output or not even running at all.
*/

tos4_load()
{
int cmd,tmp;
cmd=-1;
if (tmp=init_TOSmain()) cmd=0;  /* Initialize main system */
if (cmd==-1) { printf("TalkerOS:  Main system did not initialize in tos4_load()!!\n           BOOT ABORTED.\n\n"); exit(0); }
/* --------------------------------------------------
   Place third-party plugin initialization calls HERE
   Ex:  if (tmp=plugin_00x000_init(cmd)) cmd=cmd+tmp;
   -------------------------------------------------- */
if (tmp=plugin_01x000_init(cmd)) cmd=cmd+tmp;
if (tmp=plugin_00x000_init(cmd)) cmd=cmd+tmp;
/* ------------------------------------------------
   End third-party plugin initialization calls HERE
   ------------------------------------------------ */
printf("Verifying plugins... ");
tos_versionVerify();
printf("System plugin registry initialized.\n");
return 1;
}

tos_versionVerify()
{
PL_OBJECT plugin,p;
CM_OBJECT com;
for (plugin=plugin_first; plugin!=NULL; plugin=p) {
        p=plugin->next;
        if (atoi(RUN_VER) < atoi(plugin->req_ver)) {
                printf("TOS: Plugin '%s' requires a newer version of TalkerOS.\n",plugin->name);
                sprintf(text,"TalkerOS: Plugin '%s' requires version %s of TalkerOS.\n",plugin->name,plugin->req_ver);
                write_syslog(text,0,SYSLOG);
                for (com=cmds_first; com!=NULL; com=com->next) if (com->plugin==plugin) destroy_pl_cmd(com);
                destroy_plugin(plugin);
                }
        }
}

init_TOSmain()
{
printf("\nVerifying system components and variables. . .\n");
/* check modular colorcode index checksum */
if ((CDEFAULT+CHIGHLIGHT+CTEXT+CBOLD+CSYSTEM+CSYSBOLD+CWARNING+CWHOUSER+CWHOINFO+
        CPEOPLEHI+CPEOPLE+CUSER+CSELF+CEMOTE+CSEMOTE+CPEMOTE+CTHINK+CTELLUSER+
        CTELLSELF+CTELL+CSHOUT+CMAILHEAD+CMAILDATE+CBOARDHEAD+CBOARDDATE)!=300)
        { printf("TalkerOS:  Modular colorcode index checksum FAILED.\n"); return 0; }

/* check system registry index checksum */
if ((TALKERNAME+SERIALNUM+REGUSER+SERVERDNS+SERVERIP+TALKERMAIL+TALKERHTTP+SYSOPNAME+SYSOPUNAME+PUEBLOWEB+PUEBLOPIC)!=66)
        {printf("TalkerOS:  System information registry index checksum FAILED.\n"); return 0; }

/* check system registry master entry */
if (reg_sysinfo[0][0]!='*') { printf("TalkerOS:  System registry master entry (0) must be '*'.\n   -- Temporarily fixed.");
        reg_sysinfo[0][0]='*';  reg_sysinfo[0][1]='\0'; }

printf("TalkerOS version %s initialized.\n\n",TOS_VER);
return 1;
}

tos_run_plugins(user,str,comword,len)
UR_OBJECT user;
char *str,*comword;
int len;
{
PL_OBJECT plugin;
CM_OBJECT com;
for (com=cmds_first; com!=NULL; com=com->next) {
        if (!(!strncmp(comword,com->command,len))) continue;
        if (user->level < com->req_lev) return 0;
        if (!call_plugin_exec(user,str,com->plugin,com->comnum)) return 0;
        return 1;
        }
return 0;
}

call_plugin_exec(user,str,plugin,comnum)
UR_OBJECT user;
char *str;
PL_OBJECT plugin;
int comnum;
{
/* ---------------------------------------------------
   Put third-party plugin command calls here!
   example:  if (!strcmp(plugin->registration,"00-000")) { plugin_00x000_main(user,str,comnum); return 1; }
   --------------------------------------------------- */
if (!strcmp(plugin->registration,"00-000")) { plugin_00x000_main(user,str,comnum); return 1; }
if (!strcmp(plugin->registration,"01-000")) { plugin_01x000_main(user,str,comnum); return 1; }
/* ---------------------------------------------------
   End third-party plugin comand calls here.
   --------------------------------------------------- */
return 0;
}

save_plugin_data(user)
UR_OBJECT user;
{
PL_OBJECT plugin;
int i;
i=0;
for (plugin=plugin_first; plugin!=NULL; plugin=plugin->next) {
        if (!i && plugin->req_userfile) { call_plugin_exec(user,"",plugin,-7); i++; }
        else if (plugin->req_userfile) { call_plugin_exec(user,"",plugin,-8); i++; }
        }
}

load_plugin_data(user)
UR_OBJECT user;
{
PL_OBJECT plugin;
for (plugin=plugin_first; plugin!=NULL; plugin=plugin->next)
        if (plugin->req_userfile) call_plugin_exec(user,"",plugin,-9);
}


disp_plugin_registry(user)
UR_OBJECT user;
{
PL_OBJECT plugin;
CM_OBJECT com;
int cm,total;

cm=0;  total=0;
/* write data for each loaded plugin */
for (plugin=plugin_first; plugin!=NULL; plugin=plugin->next) {
        if (!total) {
                sprintf(text,"\n%s%s                   TalkerOS  Plugin  Module  Registry                   \n\n",colors[CHIGHLIGHT],colors[CTEXT]);
                write_user(user,text);
                write_user(user,"~FTPos: ID-Auth : Name                       : Cmds : Ver : Author\n");
                }
        /* search for associated commands */
        cm=0; for (com=cmds_first; com!=NULL; com=com->next) if (com->plugin==plugin) cm++;
        /* write data to user */
        sprintf(text,"%3d: %-7s : %-26s :  %2d  : %-3s : %-21s\n",total,plugin->registration,plugin->name,cm,plugin->ver,plugin->author);
        write_user(user,text);
        total++;
        }
if (!total) write_user(user,"No plugins are loaded on this system.\n");
else write_user(user,"\n");
}

plugin_triggers(user,str)
UR_OBJECT user;
char *str;
{
PL_OBJECT plugin;
for (plugin=plugin_first; plugin!=NULL; plugin=plugin->next) {
        if (plugin->triggerable) call_plugin_exec(user,str,plugin,-5);
        }
}

/*** Construct plugin object ***/
PL_OBJECT create_plugin()
{
PL_OBJECT plugin;
int i;

if ((plugin=(PL_OBJECT)malloc(sizeof(struct plugin_struct)))==NULL) {
        write_syslog("ERROR: Memory allocation failure in create_plugin().\n",0,SYSLOG);
        return NULL;
        }

/* Append object into linked list. */
if (plugin_first==NULL) {  
        plugin_first=plugin;  plugin->prev=NULL;  
        }
else {  
        plugin_last->next=plugin;  plugin->prev=plugin_last;  
        }
plugin->next=NULL;
plugin_last=plugin;

/* initialise plugin location structure */
plugin->name[0]='\0';
plugin->author[0]='\0';
plugin->ver[0]='\0';
plugin->req_ver[0]='\0';
plugin->registration[0]='\0';
plugin->id=-1;
return plugin;
}

/*** Construct plugin command object ***/
CM_OBJECT create_cmd()
{
CM_OBJECT cmd;
int i;

if ((cmd=(CM_OBJECT)malloc(sizeof(struct plugin_cmd)))==NULL) {
        write_syslog("ERROR: Memory allocation failure in create_cmd().\n",0,SYSLOG);
        return NULL;
        }

/* Append object into linked list. */
if (cmds_first==NULL) {  
        cmds_first=cmd;  cmd->prev=NULL;  
        }
else {  
        cmds_last->next=cmd;  cmd->prev=cmds_last;  
        }
cmd->next=NULL;
cmds_last=cmd;

/* initialise plugin command structure */
cmd->command[0]='\0';
cmd->id=-1;
cmd->comnum=-1;
cmd->req_lev=99;
cmd->plugin=NULL;
return cmd;
}

tos_plugin_dump()
{
PL_OBJECT p,p2;
CM_OBJECT c,c2;
for (c=cmds_first; c!=NULL; c=c2) { c2=c->next; destroy_pl_cmd(c); }
for (p=plugin_first; p!=NULL; p=p2) { p2=p->next; call_plugin_exec(NULL,"",p,-6); destroy_plugin(p); }
return 1;
}

destroy_pl_cmd(c)
CM_OBJECT c;
{
/* Remove from linked list */
if (c==cmds_first) {
        cmds_first=c->next;
        if (c==cmds_last) cmds_last=NULL;
        else cmds_first->prev=NULL;
        }
else {
        c->prev->next=c->next;
        if (c==cmds_last) { 
                cmds_last=c->prev;  cmds_last->next=NULL; 
                }
        else c->next->prev=c->prev;
        }
free(c);
}

destroy_plugin(p)
PL_OBJECT p;
{
/* Remove from linked list */
if (p==plugin_first) {
        plugin_first=p->next;
        if (p==plugin_last) plugin_last=NULL;
        else plugin_first->prev=NULL;
        }
else {
        p->prev->next=p->next;
        if (p==plugin_last) { 
                plugin_last=p->prev;  plugin_last->next=NULL; 
                }
        else p->next->prev=p->prev;
        }
free(p);
}

/* -------------------
    TalkerOS Debugger
   ------------------- */
tos_debugger(user)
UR_OBJECT user;
{
if (word_count<2) { write_user(user,"TalkerOS DEBUGGER:  Please specify debug module/option.\n"); return; }
if (!strncmp(word[1],"com",3)) { tos_debug_commands(user); return; }
if (tos_highlev_debug && !strncmp(word[1],"inp",3)) { tos_debug_allinput(user); return; }
if (!strncmp(word[1],"plug",4)) { tos_debug_plugindata(user); return; }
}

tos_debug_alert(user)
UR_OBJECT user;
{
if (highlev_debug_on) {
        write_user(user,"~OL~FYATTENTION!!!     ATTENTION!!!   ATTENTION!!!\n\n");
        write_user(user,"Higher-level debug features are currently active on this\n");
        write_user(user,"system.  You are being informed for privacy reasons.  Do\n");
        write_user(user,"NOT send private information or change your password until\n");
        write_user(user,"this message no longer appears when you log in.  See the\n");
        write_user(user,"~OL.tinfo~RS command for status information.\n\n");
        }
}

tos_debug_commands(user)
UR_OBJECT user;
{
PL_OBJECT plugin;
CM_OBJECT com;
int i,pos,num,sysN,sysT,plc,pl,total;
char cmdType[2][6]={":NUTS",":TOS "};

i=0; pos=0; num=0; sysN=0; sysT=0; plc=0; pl=0; total=0;

for (i=0; command[i][0]!='*'; i++) {
        if (!total) write_user(user,"\n        TalkerOS DEBUGGER:  Command Listing by Memory Address\n\n");
        if (!pos) write_user(user,"  ");
        total++;  sysN++;  pos++;
        sprintf(text,"%10s - %3d%s   ",command[i],sysN,cmdType[0]);
        write_user(user,text);
        if (pos==3) { pos=0; write_user(user,"\n"); }
        }
for (i=0; toscom[i][0]!='*'; i++) {
        if (!total) write_user(user,"\n        TalkerOS DEBUGGER:  Command Listing by Memory Address\n\n");
        if (!pos) write_user(user,"  ");
        total++;  sysT++;  pos++;
        sprintf(text,"%10s - %3d%s   ",toscom[i],sysT,cmdType[1]);
        write_user(user,text);
        if (pos==3) { pos=0; write_user(user,"\n"); }
        }
for (plugin=plugin_first; plugin!=NULL; plugin=plugin->next) {
        if (!total) write_user(user,"\n        TalkerOS DEBUGGER:  Command Listing by Addressable Location\n\n");
        for (com=cmds_first; com!=NULL; com=com->next) {
                if (com->plugin != plugin) continue;
                if (!pos) write_user(user,"  ");
                total++;  plc++;  pos++;
                sprintf(text,"%10s - %3dx%-4d   ",com->command,com->comnum,plugin->id);
                write_user(user,text);
                if (pos==3) { pos=0; write_user(user,"\n"); }
                }
        pl++;
        }
if (pos!=4) write_user(user,"\n");
sprintf(text,"\nDebugger:  Addressable command search finished.\n           A total of %d commands were found.\n           %d NUTS  /  %d TalkerOS  /  %d from %d Plugins.\n\n",total,sysN,sysT,plc,pl);
write_user(user,text);
}

tos_debug_allinput(user)
UR_OBJECT user;
{
if (debug_input) {
        debug_input=0;  highlev_debug_on--;
        sprintf(text,"%s deactivated the System Input Debugger.\n",user->name);
        write_syslog(text,1,SYSLOG);
        write_syslog(text,1,USERLOG);
        sprintf(text,"\n%s%sSYSTEM Debugger:  Input logging has been deactivated.\n\n",colors[CBOLD],colors[CSYSBOLD]);
        write_room_except(NULL,text,user);
        write_user(user,"Debugger:  Input logging has been deactivated.\n");
        }
else {
        debug_input=1;  highlev_debug_on++;
        sprintf(text,"%s activated the System Input Debugger.\n",user->name);
        write_syslog(text,1,SYSLOG);
        write_syslog(text,1,USERLOG);
        sprintf(text,"\n%s%sSYSTEM Debugger:  Input logging has been activated!\n                  NOTE:  *ALL* commands, speech, and text are being\n                         logged for debugging and testing purposes.\n\n",colors[CBOLD],colors[CSYSBOLD]);
        write_room_except(NULL,text,user);
        write_user(user,"Debugger:  Input logging has been activated!\n           ~OLNOTE: If left on, this command will *rapidly* fill up the system log.\n");
        }
}

tos_debug_plugindata(user)
UR_OBJECT user;
{
PL_OBJECT plugin;
CM_OBJECT cmd;
int num,cnt,found;

num = -1;       cnt = 0;       found = 0;       plugin = plugin_first;
if (word_count<3) { write_user(user,"DEBUG PluginData requires the plugin position to retrieve data.\n"); return; }
if ((num=atoi(word[2]))==-1) { write_user(user,"DEBUG PluginData requires the NUMERICAL position for data retrieval.\n"); return; }
write_user(user,"TalkerOS DEBUGGER:  Retrieving addressable plugin information by location...\n\n");
if (plugin_first!=NULL) {
        for (plugin=plugin_first; plugin!=NULL; plugin=plugin->next) {
                if (cnt==num) { found++; break; }
                cnt++;
                }
        }        
if (!found) { write_user(user,"No addressable plugins found in system memory.  Check .plugreg for details.\n"); return; }
sprintf(text,"Plugin and Associated Commands data for registry position: %d\n\n",cnt);
write_user(user,text);
sprintf(text,"Registered ID: %-7s                            Local ID      : %3d\n",plugin->registration,plugin->id);
write_user(user,text);
sprintf(text,"Author Name  : %-22s             Plugin Version: %3s\n",plugin->author,plugin->ver);
write_user(user,text);
sprintf(text,"Description  : %-27s        Int Ver. Req'd: %3s\n",plugin->name,plugin->req_ver);
write_user(user,text);
sprintf(text,"Uses Userdata: %-3s                                Triggerable   : %-3s\n\n",noyes2[plugin->req_userfile],noyes2[plugin->triggerable]);
write_user(user,text);
cnt=0;
for (cmd=cmds_first; cmd!=NULL; cmd=cmd->next) {
        if (cmd->plugin != plugin) continue;
        cnt++;
        write_user(user,"   Associated Command\n   ------------------\n");
        sprintf(text,"   Command ID: %3d                        Full Address: %3dx%d\n",cmd->comnum,cmd->comnum,cmd->id);
        write_user(user,text);
        sprintf(text,"   Name      : %-20s       Level Req'd : %s\n\n",cmd->command,level_name[cmd->req_lev]);
        write_user(user,text);
        }
if (!cnt) write_user(user,"   (No commands associated with this plugin.)\n");
write_user(user,"Debugger:  Search finished.\n");
}

/* END OF SESSION DEBUGGER */




/* Any checks, etc. that need to be performed on login should be placed here. */
do_login_event(user)
UR_OBJECT user;
{
int pt;
char portNames[4][7]={"Main","Wiz","Link","PortErr"};

pt=3; /* default to Port Error */
if (user->port==port[0]) pt=0;                          /* on MAIN port */
        else if (user->port==port[1]) pt=1;             /* on WIZ  port */
                else if (user->port==port[2]) pt=2;     /* shouldn't get this */

totlogins++;
sprintf(text,"%s logged in. (%s Port - %s:%d)\n",user->name,portNames[pt],user->site,user->site_port);
write_syslog(text,1,USERLOG);
num_of_users++;
num_of_logins--;
user->flc=0;
if (user->pueblo==-1) user->pueblo=1;
load_plugin_data(user);
look(user);
chk_stats(user);
if (!(chk_firewall(user))) return;
audioprompt(user,0,0);  /* Audio greeting */
if (!user->vis) write_user(user,"[ You are currently invisible. ]\n");
chk_waitfor(user);
if (has_unread_mail(user)) write_user(user,"\07~FT~OL~LI-=- YOU HAVE UNREAD MAIL -=-\n");
prompt(user);
/* Encoded to get the bot to greet the user no matter what the bot's name is. */
bot_trigger(user,"login!@#$%^&*()greet)(*&^%$#@!nigol");
tos_debug_alert(user);
}

chk_stats(user)
UR_OBJECT user;
{
int desc,accreq,gender,email;
desc=0; accreq=0; gender=0; email=0;
/* seem to pick up a waitfor for themeselves somewhere... need to clear it. */
user->waitfor[0]='\0';
if (!strcmp(user->desc,newbiedesc)) desc=1;
if (!user->accreq && user->orig_level==NEW) accreq=1;
if (!user->gender) gender=1;
if (user->email[0]=='\0') email=1;
if (!(desc+accreq+gender+email)) return;
write_user(user,"\nNOTE:  You still need to set the following...\n\n");
if (accreq) write_user(user,"  -->  Request an account.         (see ~FG.help new~RS)\n");
if   (desc) write_user(user,"  -->  A description of yourself.  (use ~FG.desc~RS)\n");
if (gender) write_user(user,"  -->  Your gender.                (use ~FG.set~RS)\n");
if  (email) write_user(user,"  -->  Your email address.         (use ~FG.set~RS)\n");
write_user(user,"\nFor help with any of these settings, see ~FG.help~RS or ask an admin.\n");
}


/* =======================================================================
   TalkerOS v4.03 - Security update from previous versions.

   Previous versions of TalkerOS were prone to the NUTS 3.3.3 bug present
   in all non-bugfixed 3.3.3 codes.  For information as to the exploits
   of this bug, please contact the TalkerOS Group at the email address
   listed below.  For obvious reasons, the exact details will not be
   plainly released to avoid additional and intentional exploits of this
   security problem.

   NOTE:  TalkerOS systems with the security 'firewall' properly operating
          and set up are at minimal risk.  Exploits are still available,
          but are not critical risks to your system.  This, however, may
          not be the case on systems that do not use the included firewall,
          or on NUTS 3.3.3 systems without the TalkerOS firewall enhancement.
   ========================================================================== */

tos403_bugfix_01(user)
UR_OBJECT user;
{
        user->desc[0]='\0';
        user->pass[0]='\0';
        user->last_site[0]='\0';
        user->room_topic[0]='\0';
        user->http[0]='\0';
        user->email[0]='\0';
        user->vis=1;
        user->level=0;
        user->orig_level=0;
        user->accreq=0;
        user->duty=0;
        user->cloaklev=user->level;
        user->can_edit_rooms=0;
        user->readrules=0;
        user->created=0;
        user->arrested=0;
        user->muzzled=0;
        user->gender=0;
        user->age=-1;
        user->last_login=time(0);
        user->last_login_len=0;
        user->total_login=0;
        }

// End of Login Bugfix.                 Updated 21 Apr 1999

/* ------------------------------------------------------------------------
   ------------------------------------------------------------------------
                   END OF MAIN SYSTEM SOFTWARE CODE
   ------------------------------------------------------------------------
   ------------------------------------------------------------------------ */

/* -=-=-=-=-=-=- Begin TalkerOS Feature Additions... -=-=-=-=-=-=-=- */

/* Writes STR to users of LEVEL and higher who are in ROOM except for USER */
write_duty(level,str,room,user,cr)
int level;
char *str;
RM_OBJECT room;
UR_OBJECT user;
int cr; /* Not used anymore. */
{
UR_OBJECT u;
if (level<WIZ) level=WIZ;
for (u=user_first;u!=NULL;u=u->next) {
        if (u->login) continue;
        if (u->type==CLONE_TYPE) continue;
        if (!u->duty) continue;
        if (u->level<level || u->level >= BOTLEV) continue;
        if (u==user) continue;
        if (u->room==NULL) continue;
        if (u->room!=room && room!=NULL) continue;
        if (user!=NULL && !strcmp(u->ignuser,user->name)) continue;
        if (u->ignall) continue;
        write_user(u,str);
    }    
}


duty_toggle(user)
UR_OBJECT user;
{
if (user->duty) {
        user->duty=0;
        sprintf(text,"%s[ You are now OFF DUTY. ]\n",colors[CSYSTEM]);
        write_user(user,text);
        sprintf(text,"%s[ %s is OFF DUTY. ]\n",colors[CSYSTEM],user->name);
        write_duty(WIZ,text,NULL,user,0);
        return;
        }
else {
        user->duty=1;
        sprintf(text,"%s[ You are now ON DUTY. ]\n",colors[CSYSTEM]);
        write_user(user,text);
        sprintf(text,"%s[ %s is ON DUTY. ]\n",colors[CSYSTEM],user->name);
        write_duty(WIZ,text,NULL,user,0);
        }
}


/* Arrest/Release Functions */
arrest(user)
UR_OBJECT user;
{
UR_OBJECT u;
if (word_count<2) {
        write_user(user,"Usage: arrest <user>\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
if (u==user) {
        write_user(user,"Trying to arrest yourself isn't logical... Covering up for the real killer?!\n");
        return;
        }
if (u->arrested) { write_user(user,"That user is already under arrest!\n"); return; }
else {
        if (u->level >= user->level) { write_user(user,"You cannot arrest a user of equal or higher rank.\n"); return; }
        u->arrested=user->level;
        toggle_jail(user,u);
        }
}

release(user)
UR_OBJECT user;
{
UR_OBJECT u;
if (word_count<2) {
        write_user(user,"Usage: release <user>\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
if (!u->arrested) { write_user(user,"That user is not arrested!\n"); return; }
else {
        if (u->arrested > user->level) { write_user(user,"You cannot release a user that was arrested by a higher rank.\n"); return; }
        u->arrested=0;
        toggle_jail(user,u);
        }
}

toggle_jail(user,u)
UR_OBJECT user,u;
{
RM_OBJECT rm;

if (u->arrested) {
        sprintf(text,"%s%s%s has been arrested!\n",colors[CBOLD],colors[CSYSTEM],u->name);
        write_room_except(u->room,text,u);
        for(rm=room_first;rm!=NULL;rm=rm->next)
                if (rm->link[0]==NULL) break;
        if (rm==NULL) { write_user(user,"No rooms with JAIL access in config file!\nCannot arrest user.\n"); u->arrested=0; return; }
        move_user(u,rm,2);
        prompt(u);
        write_room_except(u->room,text,u);
        sprintf(text,"%s[ %s has been ARRESTED by %s (%s). ]\n",colors[CSYSTEM],u->name,user->name,level_name[user->level]);
        write_duty(WIZ,text,NULL,u,0);
        sprintf(text,"%s (%s) ARRESTED %s.\n",user->name,level_name[user->level],u->name);
        write_syslog(text,1,USERLOG);
        if (!user->duty) write_user(user,"> User Arrested! <\n");
        write_user(u,"You have been arrested!\n");
        return;
        }
else {
        sprintf(text,"%s%s%s has been released!\n",colors[CBOLD],colors[CSYSTEM],u->name);
        write_room_except(u->room,text,u);
        rm=room_first;
        move_user(u,rm,2);
        prompt(u);
        sprintf(text,"%s[ %s has been RELEASED by %s. ]\n",colors[CSYSTEM],u->name,user->name);
        write_duty(WIZ,text,NULL,u,0);
        sprintf(text,"%s RELEASED %s.\n",user->name,u->name);
        write_syslog(text,1,USERLOG);
        if (!user->duty) write_user(user,"> User Released! <\n");
        write_user(u,"You have been released!\n");
        sprintf(text,"%s%s%s has been released!\n",colors[CBOLD],colors[CSYSTEM],u->name);
        write_room_except(u->room,text,u);
        }
}

/* Reverses a users text if made to do so by a wiz.  Tee-hee-hee */
possess_toggle(user)
UR_OBJECT user;
{
UR_OBJECT u;
if (word_count<2) { write_user(user,"Usage:  .possess <user>\n"); return; }
if (!(u=get_user(word[1]))) { write_user(user,notloggedon); return; }
if (u==user) { write_user(user,"What is the point of possessing yourself?\n"); return; }
if (u->level>=user->level) { write_user(user,"You cannot possess a user of equal or higher level than yourself.\n"); return; }
if (u->reverse) {
        u->reverse=0;
        sprintf(text,"%s%s repairs %s's connection...\n",colors[CWARNING],user->name,u->name);
        write_room(u->room,text);
        sprintf(text,"%s[ %s fixes %s's connection. ]\n",colors[CSYSTEM],user->name,u->name);
        write_duty(WIZ,text,NULL,u,0);
        }
else {
        u->reverse=1;
        sprintf(text,"%sSomeone tinkers with %s's net connection...\n",colors[CWARNING],u->name);
        write_room_except(u->room,text,u);
        sprintf(text,"%sYour network connection begins acting weirdly...\n",colors[CWARNING]);
        write_user(u,text);
        sprintf(text,"%s[ %s tinkers with %s's connection. ]\n",colors[CSYSTEM],user->name,u->name);
        write_duty(WIZ,text,NULL,u,0);
        }
}

possess_chk(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int len,len2,i;
char c;
if (!user->reverse) return 0;
else {
        len=strlen(inpstr)-1;
        len2=len/2;
        for(i=0;i<=len2;i++){
                c=inpstr[i];
                inpstr[i]=inpstr[len-i];
                inpstr[len-i]=c;
                }
        return 1;
        }
}

warnuser(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
char type[5],*name;

if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  
        return;
        }
if (word_count<2) {
        write_user(user,"Warn who?\n");  return;
        }
if (word_count<3) {
        write_user(user,"Usage:  .warn <user> <warning text>\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (u==user) {
        write_user(user,"Warning yourself is as easy as a mental note... This is riddiculous!\n");
        return;
        }
if (u->level>=user->level) {
        write_user(user,"You cannot warn a user of an equal or higher level.\n");
        return;
        }
if (u->alert>1) {
        sprintf(text,"~FT%s has been warned %d times already.\n",u->name,u->alert);
        write_user(user,text);
        }
inpstr=remove_first(inpstr);
sprintf(text,"~OL~FW~BR >>> ~RS~FW~OL You warn %s: ~RS %s\n",u->name,inpstr);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
sprintf(text,"~OL~FW~BR >>> %s warns you:~RS~FW  %s\n",name,inpstr);
write_user(u,text);
record_tell(u,text);
sprintf(text,"%s[ %s WARNED %s: %s ]\n",colors[CSYSTEM],user->name,u->name,inpstr);
write_duty(WIZ,text,NULL,user,0);
u->alert++;
sprintf(text,"~FY%s WARNED %s. (%d)\n",user->name,u->name,u->alert);
write_syslog(text,1,USERLOG);
}

/* WizChannel is for communication across the talker by wizzes. */
wiz_ch(user,inpstr)
UR_OBJECT user;
{
if (word_count<2) return;
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
if (possess_chk(user,inpstr)) { write_duty(WIZ,text,NULL,user,0); }
sprintf(text,"~FY[WizChannel]->~FY You say:~OL %s\n",inpstr);
write_user(user,text);
sprintf(text,"~FR[WizChannel]->~FR %s says:~OL %s\n",user->name,inpstr);
write_duty(WIZ,text,NULL,user,2);
}

wiz_ch_emote(user,inpstr)
UR_OBJECT user;
{
if (word_count<2) return;
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<2) {
        write_user(user,"WizChannel emote what?\n");  return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
if (possess_chk(user,inpstr)) { write_duty(WIZ,text,NULL,user,0); }
sprintf(text,"~FY[WizChannel]->~FY~OL %s %s\n",user->name,inpstr);
write_user(user,text);
sprintf(text,"~FR[WizChannel]->~FR~OL %s %s\n",user->name,inpstr);
write_duty(WIZ,text,NULL,user,2);
}

secho(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
if (word_count<2) {
        write_user(user,"System-echo what?\n");  return;
        }
sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
if (possess_chk(user,inpstr)) { write_duty(user->level,text,user->room,user,0); }
if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,NULL,1); }
sprintf(text,"~RS%s\n",inpstr);
write_room(user->room,text);
record(user->room,text);
}


think(user,inpstr)
UR_OBJECT user;
{
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<2) {
        write_user(user,"Think what?\n");  return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
if (possess_chk(user,inpstr)) { write_duty(user->level,text,user->room,user,0); }
sprintf(text,"%sYou think . o O ( %s )\n",colors[CTHINK],inpstr);
write_user(user,text);
if (user->vis) {
        sprintf(text,"%s%s thinks . o O ( %s )\n",colors[CTHINK],user->name,inpstr);
        }
else {
        sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name);
        write_duty(user->level,text,user->room,user,0);
        sprintf(text,"%s%s thinks . o O ( %s )\n",colors[CTHINK],invisname,inpstr);
        }
write_room_except(user->room,text,user);
record(user->room,text);
bot_trigger(user,inpstr);
plugin_triggers(user,inpstr);
}

/* Shift and Shout-Shift: mAkEs tExT LoOk lIkE ThIs */
char_shift(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int i,j;
i=0;
j=0;
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<2) {
        write_user(user,"Shift what?\n");  return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
for (i=0; i < strlen(inpstr); i++) {
        if (j++ % 2) { inpstr[i]=tolower(inpstr[i]); }
        else { inpstr[i]=toupper(inpstr[i]); }
        }
say(user,inpstr);
}
 
char_shift_shout(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
int i,j;
i=0;
j=0;
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (word_count<2) {
        write_user(user,"Shout-Shift what?\n");  return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
for (i=0; i < strlen(inpstr); i++) {
        if (j++ % 2) { inpstr[i]=tolower(inpstr[i]); }
        else { inpstr[i]=toupper(inpstr[i]); }
        }
shout(user,inpstr);
}

sing(user,inpstr)
UR_OBJECT user;
{
char *name;
if (word_count<2) { write_user(user,"Usage: sing <text>\n"); return; }
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
if (possess_chk(user,inpstr)) { write_duty(WIZ,text,user->room,user,0); }
if (user->vis) name=user->name; else name=invisname;
if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,user,1); }
sprintf(text,"%s sings: o/~ %s o/~\n",name,inpstr);
write_room_except(user->room,text,user);
write_user(user,text);
bot_trigger(user,inpstr);
plugin_triggers(user,inpstr);
}

set_user_total_time(user)
UR_OBJECT user;
{
UR_OBJECT u;
int days,hours,mins,total;
int days2,hours2,mins2;
char plural[2][2]={"s",""};

if (word_count<5) { write_user(user,"Usage:  .settime <user> <days> <hours> <minutes>\n"); return; }
if (get_user(word[1])) { write_user(user,"-=- You cannot alter the total time of a user currently logged in. -=-\n"); return; }
if (!isnumber(word[2])) { write_user(user,"The number of days must be a number.\n"); return; }
if (!isnumber(word[3])) { write_user(user,"The number of hours must be a number.\n"); return; }
if (!isnumber(word[4])) { write_user(user,"The number of minutes must be a number.\n"); return; }
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in set_user_total_time().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[1]);
if (!(load_user_details(u))) { write_user(user,nosuchuser); destruct_user(u); destructed=0; return; }
if (u->level>=user->level) { write_user(user,"You cannot change the total time of a user equal to or higher than you in rank.\n"); destruct_user(u); destructed=0; return; }

days=atoi(word[2]);
hours=atoi(word[3]);
mins=atoi(word[4]);
days2=u->total_login/86400;
hours2=(u->total_login%86400)/3600;
mins2=(u->total_login%3600)/60;

sprintf(text," Time Entered:  %d day%s, %d hour%s, %d minute%s.\n",days,plural[(days==1)],hours,plural[(hours==1)],mins,plural[(mins==1)]);
write_user(user,text);
total=days*86400;
total=total+(hours*3600);
total=total+(mins*60);
u->total_login=total;
sprintf(text,"Result Stored:  %d\n",total);
write_user(user,text);
days=u->total_login/86400;
hours=(u->total_login%86400)/3600;
mins=(u->total_login%3600)/60;
sprintf(text,"     New Time:  %d day%s, %d hour%s, %d minute%s.\n",days,plural[(days==1)],hours,plural[(hours==1)],mins,plural[(mins==1)]);
write_user(user,text);
save_user_details(u,0);
sprintf(text,"%s altered %s's total time.\n",user->name,u->name);
write_syslog(text,1,USERLOG);
sprintf(text,"\nNOTICE:\nYour total time has been changed to:\n     %d day%s, %d hour%s, and %d minute%s.\n\nIt was originally:\n     %d day%s, %d hour%s, and %d minute%s.\n",days,plural[(days==1)],hours,plural[(hours==1)],mins,plural[(mins==1)],days2,plural[(days2==1)],hours2,plural[(hours2==1)],mins2,plural[(mins2==1)]);
send_mail(NULL,u->name,text);
destruct_user(u);
destructed=0;
write_user(user,"-=- Saved. -=-\n");
}

waitfor(user)
UR_OBJECT user;
{
UR_OBJECT u;
if (word_count<2) {
        write_user(user,"Usage:  waitfor off/<user>\n");
        if (user->waitfor[0]) {sprintf(text,"You are currently waiting for: %s\n",user->waitfor); write_user(user,text);}
        return;
        }
if (!strcmp(word[1],"off")) {
        if (!user->waitfor[0]) { write_user(user,"You are not waiting for anyone.\n"); return; }
        user->waitfor[0]='\0';
        write_user(user,"You are no longer waiting for anyone.\n");
        return;
        }
if (!(u=get_user(word[1]))) {
        if ((u=create_user())==NULL) {
                sprintf(text,"%s: unable to create temporary user object.\n",syserror);
                write_user(user,text);
                write_syslog("ERROR: Unable to create temporary user object in waitfor().\n",0,SYSLOG);
                return;
                }
        strcpy(u->name,word[1]);
        if (!(load_user_details(u))) {
                write_user(user,nosuchuser);
                destruct_user(u);
                destructed=0;
                return;
                }
        strcpy(user->waitfor,u->name);
        destruct_user(u);
        destructed=0;
        sprintf(text,"-=- You are now waiting for: %s -=-\n",user->waitfor);
        write_user(user,text);
        }
else {
        if (u==user) { write_user(user,"You cannot wait for yourself!\n"); return; }
        write_user(user,"-=- User is already online! -=-\n");
        }
}

chk_waitfor(user)
UR_OBJECT user;
{
UR_OBJECT u;
for(u=user_first;u!=NULL;u=u->next) {
        if (u->login) continue;
        if (u->type!=USER_TYPE) continue;
        if (u==user) continue;
        if (!strcmp(u->waitfor,user->name)) {
                strcpy(u->waitfor,"");
                sprintf(text,"\n~BY~FY~OL!!!~RS  %s has been waiting for you.  ~BY~FY~OL!!!\n",u->name);
                write_user(user,text);
                write_user(u,"\07\07");
                sprintf(text,"\n~FY~BY~OL(*)~RS  %s has arrived.  ~BY~FY~OL(*)\n\n",user->name);
                write_user(u,text);
                if (u->pueblo && u->pueblo_pg) write_user(u,"</xch_mudtext><img xch_alert><xch_mudtext>");
                }
        }
}

clear_rm_topic(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
if (word_count<2) { write_user(user,"Usage:  .clrtopic <room>\n"); return; }
if ((rm=get_room(word[1]))==NULL) { write_user(user,nosuchroom);  return; }
rm->topic[0]='\0';
write_user(user,"-=- Okay. -=-\n");
sprintf(text,"-=- Room topic has been cleared. -=-\n");
write_room_except(rm,text,user);
if (rm->personal) {
        if (!(u=get_user(rm->owner))) return;
        else u->room_topic[0]='\0';
        }
}

set_nonewbies(user)
UR_OBJECT user;
{
if (!nonewbies) {
        write_user(user,"<*> New Accounts are now RESTRICTED. <*>\n");
        sprintf(text,"%s[ %s has RESTRICTED New Accounts. ]\n",colors[CSYSTEM],user->name);
        write_duty(WIZ,text,NULL,user,0);
        nonewbies=1;
        sprintf(text,"%s restricted NEW accounts.\n",user->name);
        write_syslog(text,1,USERLOG);
        }
else {
        write_user(user,"<*> New Accounts are now ALLOWED. <*>\n");
        sprintf(text,"%s[ %s has ALLOWED New Accounts. ]\n",colors[CSYSTEM],user->name);
        write_duty(WIZ,text,NULL,user,0);
        nonewbies=0;
        sprintf(text,"%s allowed NEW accounts.\n",user->name);
        write_syslog(text,1,USERLOG);
        }
}

set_user_prefs(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
char promptgender[2][7]={"Female","Male"};
if (word_count<2) {
        write_user(user,"You may make changes to any of the following...\n\n");
        write_user(user,"     ~FYFunction       Current Setting\n");
           sprintf(text,"     ~FG.set http    ~RS: ~FT%s\n",user->http);
                write_user(user,text);
        write_user(user,"                    Web Homepage Address\n");
           sprintf(text,"     ~FG.set email   ~RS: ~FT%s\n",user->email);
                write_user(user,text);
        write_user(user,"                    Electronic Mail Address\n");
           sprintf(text,"     ~FG.set gender  ~RS: ~FT%s\n",gender[user->gender]);
                write_user(user,text);
        write_user(user,"                    Your Gender - none/male/female\n");
           if (user->age!=-1) { sprintf(text,"     ~FG.set age     ~RS: ~FT%d\n",user->age); write_user(user,text); }
                else write_user(user,"     ~FG.set age     ~RS: ~FT(unknown)\n");
        write_user(user,"                    Your age - 1 to 999\n");
           sprintf(text,"     ~FG.set private ~RS: ~FT%s\n",noyes2[user->pstats]);
                write_user(user,text);
        write_user(user,"                    E-Mail & Web Address Privacy (toggle)\n");
           sprintf(text,"     ~FG.set audio   ~RS: ~FT%s\n",noyes2[user->pueblo_mm]);
                write_user(user,text);
        write_user(user,"                    Pueblo Audio Prompting (toggle)\n");
           sprintf(text,"     ~FG.set pager   ~RS: ~FT%s\n",noyes2[user->pueblo_pg]);
                write_user(user,text);
        write_user(user,"                    Pueblo Pager Audio (toggle)\n");
           sprintf(text,"     ~FG.set voice   ~RS: ~FT%s\n",promptgender[user->voiceprompt]);
                write_user(user,text);
        write_user(user,"                    Audio Prompt Gender - male/female (toggle)\n");

        write_user(user,"\n");        return;
        }
inpstr=remove_first(inpstr);
if (!strcmp(word[1],"http")) {
        if (strlen(inpstr)>58) {
                write_user(user,"HTTP address too long.  Must be less than 58 characters long.\n");
                return;
                }
        if (strlen(inpstr)<3) {
                write_user(user,"HTTP address too short.  HTTP setting has not been altered.\n");
                return;
                }
        strcpy(user->http,inpstr);
        sprintf(text,"HTTP set to: %s\n",user->http);
        write_user(user,text);
        return;
        }
if (!strcmp(word[1],"email")) {
        if (strlen(inpstr)>58) {
                write_user(user,"E-Mail address too long.  Must be less than 58 characters long.\n");
                return;
                }
        if (strlen(inpstr)<3) {
                write_user(user,"E-Mail address too short.  E-Mail setting has not been altered.\n");
                return;
                }
        strcpy(user->email,inpstr);
        sprintf(text,"E-Mail set to: %s\n",user->email);
        write_user(user,text);
        return;
        }
if (!strcmp(word[1],"gender")) {
        if (!strcmp(word[2],"none")) {
                user->gender=0;
                write_user(user,"Your gender has been set to: NONE\n");
                return;
                }
        if (!strcmp(word[2],"male")) {
                user->gender=1;
                write_user(user,"Your gender has been set to: MALE\n");
                return;
                }
        if (!strcmp(word[2],"female")) {
                user->gender=2;
                write_user(user,"Your gender has been set to: FEMALE\n");
                return;
                }
        write_user(user,"Unknown gender!\nMust be: none/male/female\n");
        return;
        }
if (!strcmp(word[1],"age")) {
        if (!(isnumber(word[2]))) {
                write_user(user,"Your age must be a NUMBER (1-999).\n");
                return;
                }
        if (atoi(word[2])<1 || atoi(word[2])>999) {
                write_user(user,"Age is out of valid range. (1-999)\n");
                return;
                }
        user->age = atoi(word[2]);
        sprintf(text,"Your age has been set to: %d\n",user->age);
        write_user(user,text);
        return;
        }
if (!strcmp(word[1],"private")) {
        if (user->pstats) {
                user->pstats=0;
                write_user(user,"You have made your E-Mail and Website PUBLIC.\n");
                save_user_details(user,1);
                return;
                }
        else {
                user->pstats=1;
                write_user(user,"You have made your E-Mail and Website PRIVATE.\n");
                save_user_details(user,1);
                return;
                }
        }
if (!strcmp(word[1],"audio")) {
        if (user->pueblo_mm) {
                user->pueblo_mm=0;
                write_user(user,"You have turned OFF the Pueblo audio prompting.\n");
                if (!user->pueblo) write_user(user,"This function only works when connected using the Pueblo telnet client.\n");
                save_user_details(user,1);
                return;
                }
        else {
                user->pueblo_mm=1;
                write_user(user,"You have turned ON the Pueblo audio prompting.\n");
                if (!user->pueblo) write_user(user,"This function only works when connected using the Pueblo telnet client.\n");
                save_user_details(user,1);
                return;
                }
        }
if (!strcmp(word[1],"pager")) {
        if (user->pueblo_pg) {
                user->pueblo_pg=0;
                write_user(user,"You have turned OFF pager audio for Pueblo.\n");
                write_user(user,"  (The talker will now activate the pueblo ALERT sound instead.)\n");
                if (!user->pueblo) write_user(user,"This function only works when connected using the Pueblo telnet client.\n");
                save_user_details(user,1);
                return;
                }
        else {
                user->pueblo_pg=1;
                write_user(user,"You have turned ON pager audio for Pueblo.\n");
                if (!user->pueblo) write_user(user,"This function only works when connected using the Pueblo telnet client.\n");
                save_user_details(user,1);
                return;
                }
        }
if (!strcmp(word[1],"voice")) {
        if (user->voiceprompt) user->voiceprompt=0;
                          else user->voiceprompt=1;
        save_user_details(user,1);
        sprintf(text,"You have set the audio prompt voice gender to: %s\n",promptgender[user->voiceprompt]);
        write_user(user,text);
        if (!user->pueblo) write_user(user,"This function only works when connected using the Pueblo telnet client.\n");
        return;
        }

write_user(user,"Unknown SET option.  Type ~FG.set~RS to see the available options.\n");
}

force_lowercase(user)
UR_OBJECT user;
{
UR_OBJECT u;
if (word_count<2) { write_user(user,"Usage:  .flc <user>\n"); return; }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (u==user) {
        write_user(user,"If you don't want to type any CAPS... turn caps lock off.\n");
        return;
        }
if (u->level>=user->level) {
        write_user(user,"You cannot force a user of an equal or higher level to type lowercase.\n");
        return;
        }
if (u->flc) {
        u->flc=0;
        write_user(u,"CAPSLOCK restriction removed!\n");
        sprintf(text,"%s[ %s ALLOWED %s's use of capital letters. ]\n",colors[CSYSTEM],user->name,u->name);
        write_duty(WIZ,text,NULL,u,0);
        if (!user->duty) {
                write_user(user,"User's CAPSLOCK restriction removed!\n");
                }
        sprintf(text,"%s allowed %s to use CAPS.\n",user->name,u->name);
        write_syslog(text,1,USERLOG);
        return;
        }
else {
        u->flc=1;
        write_user(u,"You have been placed under CAPSLOCK restriction!\n");
        sprintf(text,"%s[ %s RESTRICTED %s's use of capital letters. ]\n",colors[CSYSTEM],user->name,u->name);
        write_duty(WIZ,text,NULL,u,0);
        if (!user->duty) {
                write_user(user,"User's CAPSLOCK restriction activated!\n");
                }
        sprintf(text,"%s forced %s to use LOWERCASE.\n",user->name,u->name);
        write_syslog(text,1,USERLOG);
        return;
        }
}

temp_level(user,promote)
UR_OBJECT user;
int promote;
{
UR_OBJECT u;
int alt_cloak;

alt_cloak=0;
if (word_count<2) { write_user(user,"Usage:  tpromote/tdemote <user>\n"); return; }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);  return;
        }
if (u==user) {
        write_user(user,"You cannot change your own temp-level.\n");
        return;
        }
if (u->orig_level>=user->orig_level) {
        write_user(user,"You cannot change the temp-level of that user.\n");
        return;
        }
if (u->cloaklev==u->level) alt_cloak=1;
else alt_cloak=0;
if (promote) {
        if ((u->level+1)==user->level) {
                write_user(user,"You cannot temp-promote that user to that level.\n");
                return;
                }
        u->level++;
        sprintf(text,"[ You have been temporarily promoted to level: %s ]\n  It will reset to %s when you log out.\n",level_name[u->level],level_name[u->orig_level]);
        write_user(u,text);
        if (!user->duty) write_user(user,"[ User has been temp-promoted. ]\n");
        sprintf(text,"%s[ %s TEMP-PROMOTED %s to level: %s ]\n",colors[CSYSTEM],user->name,u->name,level_name[u->level]);
        write_duty(u->orig_level,text,NULL,u,0);
        sprintf(text,"%s TEMP-PROMOTED %s to: %s (This session only.)\n",user->name,u->name,level_name[u->level]);
        write_syslog(text,1,USERLOG);
        if (alt_cloak) {
                u->cloaklev=u->level;
                }
        return;
        }
else {
        if ((u->level-1)<=NEW) {
                write_user(user,"You cannot temp-demote that user to that level.\n");
                return;
                }
        u->level=u->level-1;
        sprintf(text,"[ You have been temporarily demoted to level: %s ]\n  It will reset to %s when you log out.\n",level_name[u->level],level_name[u->orig_level]);
        write_user(u,text);
        if (!user->duty) write_user(user,"[ User has been temp-demoted. ]\n");
        sprintf(text,"%s[ %s TEMP-DEMOTED %s to level: %s ]\n",colors[CSYSTEM],user->name,u->name,level_name[u->level]);
        write_duty(u->orig_level,text,NULL,u,0);
        sprintf(text,"%s TEMP-DEMOTED %s to: %s (This session only.)\n",user->name,u->name,level_name[u->level]);
        write_syslog(text,1,USERLOG);
        if (alt_cloak) u->cloaklev=u->level;
        else if (u->level < com_level[CLOAK] || u->cloaklev > u->level) u->cloaklev=u->level;
        }
}

set_cloak(user)
UR_OBJECT user;
{
int i,lev;
UR_OBJECT u;
int thisuser;
char *name;

i=0;  lev=0;  thisuser=1;
if (word_count<2) {
        write_user(user,"Usage:  cloak none/<levelname> [<user>]\n");
        if (user->cloaklev!=user->level) sprintf(text,"You are currently cloaked at level: %s\n\n",level_name[user->cloaklev]);
        else sprintf(text,"You currently are not cloaked.\n\n");
        write_user(user,text);
        return;
        }
if (word[2][0]) {  /* Check for user first, since the level name check
                      depends on whether it's this or another user. */
        if (!(u=get_user(word[2]))) { write_user(user,notloggedon); return; }
        if (u==user) thisuser=1;
        else if (u->level >= user->level) { write_user(user,"You may not alter the cloak of a user on or above your level.\n"); return; }
                else thisuser=0;
        }
lev=get_level(word[1]);
if (lev==-1) {
        strtolower(word[1]);
        if (!strcmp(word[1],"none") || !strcmp(word[1],"off")) { if (thisuser) lev=user->level;  else lev=u->level; }
        else { write_user(user,"-=- Unknown level name. -=-\n"); return; }
        }
if (thisuser) {
        if (lev > user->level) { write_user(user,"You cannot cloak above your actual level.\n"); return; }
        if (lev==user->cloaklev) {
                if (lev==user->level) write_user(user,"Your cloak is already off.\n");
                else write_user(user,"Your cloak is already set to that.\n");
                return;
                }
        user->cloaklev=lev;
        if (lev==user->level) { write_user(user,"Your cloak has been turned off.\n");
                                sprintf(text,"%s[ %s DEACTIVATED %s cloak. ]\n",colors[CSYSTEM],user->name,posgen[user->gender]);
                                write_duty(user->level,text,NULL,user,0);
                                return;
                                }
        sprintf(text,"You now have a cloaking level of: %s\n",level_name[user->cloaklev]);
        write_user(user,text);
        sprintf(text,"%s[ %s ACTIVATED %s cloak at level: %s ]\n",colors[CSYSTEM],user->name,posgen[user->gender],level_name[user->cloaklev]);
        write_duty(user->level,text,NULL,user,0);
        }
else { /* If it is not this user, we are cloaking someone else. */
        if (lev > u->level) { write_user(user,"You cannot cloak that person above his/her actual level.\n"); return; }
        if (lev==u->cloaklev) {
                if (lev==u->level) write_user(user,"That person's cloak is already off.\n");
                else write_user(user,"That person's cloak is already set to that.\n");
                return;
                }
        if (user->vis) name=user->name;  else name=invisname;
        u->cloaklev=lev;
        if (lev==u->level) {
                                sprintf(text,"You have turned %s's cloak off.\n",u->name);
                                write_user(user,text);
                                sprintf(text,"%s has revealed your cloak!!!\n",name);
                                write_user(u,text);
                                sprintf(text,"%s[ %s REVEALED %s's cloak. ]\n",colors[CSYSTEM],user->name,u->name);
                                write_duty(user->level,text,NULL,user,0);
                                return;
                                }
        sprintf(text,"You have cloaked %s to level: %s\n",u->name,level_name[u->cloaklev]);
        write_user(user,text);
        sprintf(text,"%s cloaked you to level: %s\n",name,level_name[u->cloaklev]);
        write_user(u,text);
        sprintf(text,"%s[ %s DISGUISED %s to level: %s ]\n",colors[CSYSTEM],user->name,u->name,level_name[u->cloaklev]);
        write_duty(user->level,text,NULL,user,0);
        }
}

directed_say(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
char type[11],type2[11],*name,*name2;

if (word_count<3) {
        write_user(user,"Usage:  to <user> <text>\n");
        return;
        }
if (user->muzzled) {
        write_user(user,"-=- You are muzzled... Action denied. -=-\n");  return;
        }
if (user->room==NULL) {
        sprintf(text,"ACT %s say %s\n",user->name,inpstr);
        write_sock(user->netlink->socket,text);
        no_prompt=1;
        return;
        }
if (word_count<3 && user->command_mode) {
        write_user(user,"Say what to who?\n");  return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
if (u==user) {
        write_user(user,"Talking to yourself again eh?  But OUT LOUD this time?!\n");
        return;
        }
if (u->room!=user->room) { write_user(user,"You cannot talk to a person in another room using this command.\n");
                           return; }
inpstr=remove_first(inpstr);
switch(inpstr[strlen(inpstr)-1]) {
     case '?': strcpy(type,"asks");  strcpy(type2,"ask");  break;
     case '!': strcpy(type,"exclaims to");  strcpy(type2,"exclaim to");  break;
     default : strcpy(type,"says to");  strcpy(type2,"say to");  break;
     }
if (ban_swearing && contains_swearing(inpstr)) {
        write_user(user,noswearing);  return;
        }
sprintf(text,"%s[%s: %s]\n",colors[CSYSTEM],user->name,inpstr);
if (possess_chk(user,inpstr)) { write_duty(user->level,text,user->room,user,0); }
sprintf(text,"%s[You %s %s]%s: %s\n",colors[CSELF],type2,u->name,colors[CTEXT],inpstr);
write_user(user,text);
if (user->vis) name=user->name; else name=invisname;
if (u->vis) name2=u->name; else name2=invisname;
if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,user,1); }
sprintf(text,"%s[%s %s %s]%s: %s\n",colors[CUSER],name,type,name2,colors[CTEXT],inpstr);
write_room_except(user->room,text,user);
record(user->room,text);
bot_trigger(user,inpstr);
plugin_triggers(user,inpstr);
}

disp_map(user)
UR_OBJECT user;
{
char filename[80];
int mapnum;

if (word_count<2 || totalmaps==1 || totalmaps==0) {
        sprintf(filename,"%s/%s",DATAFILES,MAPFILE);
        switch(more(user,user->socket,filename)) {
                case 0: if (totalmaps==1 || totalmaps==0) write_user(user,"-=- The map index is not currently available. -=-\n");
                        else write_user(user,"-=- The map is not currently available. -=-\n");
                        break;
                case 1: while(more(user,user->socket,filename)==1) {
                        more(user,user->socket,filename);
                        }
                        break;
                case 2:
                }
        return;
        }
if (!isnumber(word[1])) {
        write_user(user,"Usage:  map [<mapnumber>]\nMAPNUMBER must be a number.\n");
        return;
        }
mapnum=atoi(word[1]);
if (mapnum<0 || mapnum>totalmaps) { sprintf(text,"MAPNUMBER:  Must be between 0 and %d.\n",totalmaps); write_user(user,text); return; }
if (!mapnum) sprintf(filename,"%s/%s",DATAFILES,MAPFILE);
else sprintf(filename,"%s/%s.%d",DATAFILES,MAPFILE,mapnum);
switch(more(user,user->socket,filename)) {
        case 0: write_user(user,"-=- That map is not currently available. -=-\n");  break;
        case 1: while(more(user,user->socket,filename)==1) {
                        more(user,user->socket,filename);
                        }
                break;
        case 2:
        }
}


/**--------------------- TalkerOS Personal Rooms (v2.0) -------------------**/
/* TalkerOS v4.0x
 * ==============================================================================================
 * This file containes updated functions relating to the personal room systems in all versions
 * of TalkerOS 4.  These functions should be cross-compatible for all versions of TalkerOS 4,
 * which includes runtime versions 401, 402, and 403.
 *
 * Simply delete the OLD functions:  "prstat", "loadroom", and "myroom"
 * Then paste this file into "tos40x.c" (where 'x' is replaced by your particular version number)
 * Recompile and reboot.
 * ==============================================================================================
 * Released October 9, 2000
 *
 * >>>>> THIS FILE HAS ALREADY BEEN PATCHED <<<<<
 */

prstat(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
int total,ppl,tppl,free,use,rstats,i;
char *prstat[]={"~FRCLOSED","~FG OPEN ","~FYIN-USE"};
char rmname[USER_NAME_LEN+1];

total=0; ppl=0; tppl=0; free=0; use=0; rstats=0;

sprintf(text,"%s%s-=-  TalkerOS Dynamic Room Structure : Information  -=-\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
write_user(user,text);
for(rm=room_first;rm!=NULL;rm=rm->next) {
        if (!rm->personal) continue;
        total++;
        if (total==1) write_user(user,"Room : Status : Users : Owner");
        if (user->level > WIZ && total==1) write_user(user,"        : Topic\n\n");
        else if (total==1) write_user(user,"\n\n");
        ppl=0;
        for(u=user_first;u!=NULL;u=u->next) {
                if (u->room!=rm) continue;
                if (!u->vis && u->level > user->level) continue;
                if (u->type==CLONE_TYPE && !u->owner->vis && u->owner->level > user->level) continue;
                ppl++;  tppl++;
                }
        if (allowpr) rstats=1;
                else rstats=0;
        if (rm->owner[0]!='\0') { rstats=2; use++; }
                else free++; 

        /* Hide room owner if all ppl inside are hidden. */
        if (rstats==2 && !ppl) strcpy(rmname,"?");
                else strcpy(rmname,rm->owner);

        sprintf(text," %2d  ~FT: %s ~FT:~RS   %2d  ~FT:~FW %-12s",total,prstat[rstats],ppl,rmname);
        write_user(user,text);
        if (user->level > WIZ) { sprintf(text," ~FT:~RS %s",rm->topic); write_user(user,text); }
        write_user(user,"\n");
        }
write_user(user,"\n");
sprintf(text,"Total of %d users in personal rooms.\n",tppl);
write_user(user,text);
if (use && !allowpr) write_user(user,"~FR*");
if (allowpr) i=free;
else i=0;
sprintf(text,"Personal rooms are currently %s\nResources: %d free / %d used / %d available.\n\n",prstat[allowpr],free,use,i);
write_user(user,text);
if (use && !allowpr) write_user(user,"~FR*~RS Rooms currently in-use will be CLOSED when empty.\n\n");
}

myroom(user)
UR_OBJECT user;
{
RM_OBJECT room,rm;
int create;
room=NULL; create=1;
for(rm=room_first;rm!=NULL;rm=rm->next) 
	if (rm->personal && !strcmp(rm->owner,user->name)) { room=rm; create=0; }
for(rm=room_first;rm!=NULL && room==NULL;rm=rm->next) {
        if (rm->personal && rm->owner[0]=='\0' && room==NULL ) { room=rm; create=1; }
        }
if (room==NULL) { write_user(user,"Sorry - All personal rooms are currently in-use.  See .prstat for details\n");
                  return; }
if (create) {
        if (!allowpr) { write_user(user,"Sorry - Personal rooms currently are not allowed.\n"); return; }
        strcpy(room->owner,user->name); strcpy(room->topic,user->room_topic); }
clear_revbuff(room);
move_user(user,room,0);
}

loadroom(user)
UR_OBJECT user;
{
RM_OBJECT room,rm;
int create;
if (!allowpr) { write_user(user,"Sorry! - Personal/Dynamic rooms are disabled.\n"); return; }
if (word_count<2) { write_user(user,"Usage:  loadroom <user>\n"); return; }
if (get_user(word[1])) { write_user(user,"-=- User is connected. -=-\nYou may not override a user's control in this situation.\n"); return; }
if (get_room(word[1])) { write_user(user,"-=- Room is already loaded.\n"); return; }
if (strlen(word[1])>USER_NAME_LEN) { write_user(user,"-=- Room name is too long! -=-\n"); return; }
room=NULL; create=1;
for(rm=room_first;rm!=NULL;rm=rm->next) if (rm->personal && !strcmp(rm->owner,word[1])) { room=rm; create=0; }
for(rm=room_first;rm!=NULL && room==NULL;rm=rm->next) {
        if (rm->personal && rm->owner[0]=='\0' && room==NULL ) room=rm;
        }
if (room==NULL) { write_user(user,"Sorry - All personal rooms are currently in-use.  See .prstat for details\n");
                  return; }
if (create) { strcpy(room->owner,word[1]); move_user(user,room,2); write_user(user,"ATTENTION:  This room is currently occupying space in the TalkerOS\n            Personal Room Structure and will be destroyed when empty.\n\n"); }
else { write_user(user,"Room already exists.  Attempting to unload.\n"); reset_access(room); write_room(room,"ATTENTION:  This room has been destructed!\n"); return; }
}



getout(user)
UR_OBJECT user;
{
UR_OBJECT u;
if (word_count<2) { write_user(user,"Usage:  getout <user>\n"); return; }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
if (u==user) { write_user(user,"Why would you want to force yourself out of this room?!\n"); return; }
if (!u->room->personal) { write_user(user,"You cannot force a user out of a normal room.\n"); return; }
if (!strcmp(u->room->owner,user->name)) {
        sprintf(text,"%s has told you to GET OUT!\n",user->name);
        write_user(u,text);
        sprintf(text,"%s yells GET OUT into %s's ear!\n",user->name,u->name);
        write_room_except(u->room,text,u);
        move_user(u,room_first,2);
        write_user(user,"-=- Ok. -=-\n");
        }
else {
        write_user(user,"You cannot force a user out of any room but your own.\n");
        return;
        }
}

/* The following two functions edit the room descriptions.  The first edits
   personal rooms, which are stored in the userfile directory under the name
   of Username.R  Any dynamic rooms you make to use with .loadroom will need
   to have their room descriptions stored in this fashon from the shell.

   The second function will allow certain users to edit MAIN room descs.
   This is allowed on two conditions:
        1) The user has access to the .rmdesc command.
        2) The user has the can_edit_rooms bit set to 1 in their userfile.
   The second condition cannot be set from the talker itself, the user will
   have to NOT be logged in and have this set from the shell.  This is for
   security purposes.

   To use the personal editor, the user can use .rmdesc anywhere to edit
   THEIR personal room.  To use the main editor, you MUST be in the room
   you wish to edit, then type .rmdesc <roomname> */

rmdesc(user,done_editing)  /* Personal Rooms */
UR_OBJECT user;
int done_editing;
{
FILE *fp;
char *c,filename[80];

/* Check to see if a roomname is given, if so, check to see if user can
   edit room descriptions of other rooms. */
if (user->type==REMOTE_TYPE) {
        write_user(user,"Sorry, due to software limitations remote users cannot use the line editor.\nYou will have to logon locally to perform this function.\n");
        return;
        }
if (user->orig_level<XUSR) { write_user(user,"You are not fully promoted and cannot do this due to storage space limits.\n"); return; }
if (word_count>1 && user->can_edit_rooms) { editroom(user,0); return; }
if (!allowpr) { write_user(user,"Sorry! - Personal rooms are currently not allowed.\n"); return; }
if (!done_editing) {
        sprintf(text,"\n%s%s-=- Writing Personal Room Description -=-\nNOTE:  Maximum size is 80 characters by 10 lines.\n\n",colors[CHIGHLIGHT],colors[CTEXT]);
        write_user(user,text);
        user->misc_op=8;
        editor(user,NULL);
        return;
        }
sprintf(filename,"%s/%s.R",USERFILES,user->name);
if (!(fp=fopen(filename,"w"))) {
        sprintf(text,"%s: couldn't save room description.\n",syserror);
        write_user(user,text);
        sprintf("ERROR: Couldn't open file %s to write in rmdesc().\n",filename);
        write_syslog(text,0,SYSLOG);
        return;
        }
c=user->malloc_start;
while(c!=user->malloc_end) putc(*c++,fp);
fclose(fp);
write_user(user,"-=- Personal Room Description SAVED. -=-\n");
}

editroom(user,done_editing)  /* Regular Rooms */
UR_OBJECT user;
int done_editing;
{
RM_OBJECT rm1;
FILE *fp;
char *c,c2,filename[80];
int i,notopen,toolong;

if (!(get_room(word[1])) && !done_editing) {
        write_user(user,"Room NOT FOUND : Cannot edit description.\n");
        return;
        }
if (user->room!=get_room(word[1]) && !done_editing) {
        write_user(user,"Must be in the room to edit it's description! : ROOM EDIT ABORTED\n");
        return;
        }
if (!done_editing) {
        sprintf(text,"\n%s%s-=- Writing Description for Room: %s -=-\nNOTE:  Maximum size is 80 characters by 10 lines.\n\n",colors[CHIGHLIGHT],colors[CTEXT],user->room->name);
        write_user(user,text);
        user->misc_op=9;
        editor(user,NULL);
        return;
        }
sprintf(filename,"%s/%s.R",DATAFILES,user->room->name);
if (!(fp=fopen(filename,"w"))) {
        sprintf(text,"%s: couldn't save room description.\n",syserror);
        write_user(user,text);
        sprintf("ERROR: Couldn't open file %s to write in editroom().\n",filename);
        write_syslog(text,0,SYSLOG);
        return;
        }
c=user->malloc_start;
while(c!=user->malloc_end) putc(*c++,fp);
fclose(fp);
write_user(user,"-=- Room Description SAVED. -=-\n>>> RELOADING ROOM DESCRIPTIONS <<<\n");
sprintf(text,"%s saved NEW ROOM DESCRIPTION for room: %s\n",user->name,user->room->name);
write_syslog(text,1,SYSLOG);
write_syslog(text,1,USERLOG);
notopen=0; toolong=0;
/* Load room descriptions */
for(rm1=room_first;rm1!=NULL;rm1=rm1->next) {
        if (rm1->personal) continue;
        sprintf(filename,"%s/%s.R",DATAFILES,rm1->name);
        if (!(fp=fopen(filename,"r"))) {
                notopen++;
                continue;
                }
        i=0;
        c2=getc(fp);
        while(!feof(fp)) {
                if (i==ROOM_DESC_LEN) {
                        toolong++;
                        break;
                        }
                rm1->desc[i]=c2;  
                c2=getc(fp);  ++i;
                }
        rm1->desc[i]='\0';
        fclose(fp);
        }
look(user);
if (!notopen && !toolong) write_user(user,"\nRoom descriptions re-loaded successfully.\n");
else {
        sprintf(text,"Room descriptions re-loaded.\nCould not open files for %d rooms.\nDescriptions too long for %d rooms.\n\n",notopen,toolong);
        write_user(user,text);
        }
}


toggle_rooms(user)
UR_OBJECT user;
{
if (allowpr) {
        allowpr=0;
        write_user(user,"Personal/Dynamic rooms are no longer open.\n");
        write_room_except(NULL,"SYSTEM: Personal/Dynamic rooms are no longer available!\n",user);
        sprintf(text,"%s[ %s DISABLED Personal/Dynamic Rooms ]\n",colors[CSYSTEM],user->name);
        write_duty(user->level,text,NULL,user,0);
        sprintf(text,"%s CLOSED personal/dynamic rooms.\n",user->name);
        write_syslog(text,1,USERLOG);
        return;
        }
else {
        allowpr=1;
        write_user(user,"Personal/Dynamic rooms are now open.\n");
        write_room_except(NULL,"SYSTEM: Personal/Dynamic rooms are now available!\n",user);
        sprintf(text,"%s[ %s ENABLED Personal/Dynamic Rooms ]\n",colors[CSYSTEM],user->name);
        write_duty(user->level,text,NULL,user,0);
        sprintf(text,"%s OPENED personal/dynamic rooms.\n",user->name);
        write_syslog(text,1,USERLOG);
        return;
        }
}


set_alias(user)
UR_OBJECT user;
{
UR_OBJECT u;
int i;

if (word_count<2 && user->alias[0]=='\0') {
        write_user(user,"Usage:  alias off/<nickname>\n"); return;
        }
if (word_count<2 && user->alias[0]!='\0') {
        sprintf(text,"\nYour current alias: %s\n( To disable your alias, use ~FG.alias off~RS )\n\n",user->alias);
        write_user(user,text);
        return;
        }
if (strlen(word[1])>USER_NAME_LEN) {
        write_user(user,"Alias nickname length is too long.  Maximum of 12 characters.\n");
        return;
        }
/* check validity of new name and do CAPS stuff */
for (i=0;i<strlen(word[1]);++i) {
        if (!isalpha(word[1][i])) {
                write_user(user,"\n-=- Only letters are allowed in an alias. -=-\n\n");
                return;
                }
        word[1][0] = toupper(word[1][0]);
        }
if (!strcmp(word[1],"Off")) {
        write_user(user,"Your alias (nickname) has been deactivated.\n");
        user->alias[0]='\0';
        return;
        }
if (u=get_user(word[1])) {
        write_user(user,"That alias and/or username is currently in-use.  Please try again.\n");
        return;
        }
if (contains_swearing(word[1])) {
        write_user(user,"You may not use a swear word in your nickname.\n");
        return;
        }
if ((u=create_user())==NULL) {
        sprintf(text,"%s: unable to create temporary user object.\n",syserror);
        write_user(user,text);
        write_syslog("ERROR: Unable to create temporary user object in set_alias().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[1]);
if (load_user_details(u)) {
        write_user(user,"That name is currently held by a registered user.  Please try again.\n");
        destruct_user(u); destructed=0;
        return;
        }
destruct_user(u); destructed=0;
strcpy(user->alias,word[1]);
write_user(user,"Your alias (nickname) has been set.  It is valid for this session ONLY.\n");
}

list_alias(user)
UR_OBJECT user;
{
UR_OBJECT u;
int i;
i=0;
for (u=user_first; u!=NULL; u=u->next) {
        if (u->alias[0]!='\0') {
                if (u->level>user->level && !u->vis) continue;
                i++;
                if (i==1) write_user(user,"\nList of all aliases currently in use on this system:\n");
                sprintf(text,"     ~FT%-12s~RS aka \"~FT%s~RS\"\n",u->name,u->alias);
                write_user(user,text);
                }
        }
if (i) sprintf(text,"~FGTotal of %d users with aliases currently on this system.\n\n",i);
else sprintf(text,"There are no aliases currently in use on this system.\n");
write_user(user,text);
}

list_admins(user)
UR_OBJECT user;
{
UR_OBJECT u;
int i,lev;
i=0;
for (u=user_first; u!=NULL; u=u->next) {
        if (u->level<WIZ || (!u->vis && u->level>user->level)) continue;
        if (u->level > user->level && u->cloaklev!=u->level) lev=u->cloaklev;
        else lev=u->level;
        if (lev<WIZ) continue;
        i++;
        if (i==1) {
                write_user(user,"\nWizards/Administrators Currently Online\n");
                write_user(user,"---------------------------------------\n");
                }
        sprintf(text,"    %-12s [~FR%-10s~RS]\n",u->name,level_name[lev]);
        write_user(user,text);
        }
if (i) sprintf(text,"Total of %d wizards/administrators currently online.\n\n",i);
else sprintf(text,"Sorry, there are no administrators currently available.\n");
write_user(user,text);
}


tos_info(user)
UR_OBJECT user;
{
char debugstat[3][27]={"DISABLED (never on)","ENABLED (currently OFF)","~OL*ACTIVE* (currently ON)"};
int tempstat=0;

if (tos_highlev_debug) {
        if (highlev_debug_on) tempstat=2;
                else tempstat=1;
        }

write_user(user,"\n               TalkerOS System Information\n");
write_user(user,"--------------------------------------------------------\n");
   sprintf(text,"Version (system/runtime): %s / %s\n",TOS_VER,RUN_VER);
write_user(user,text);
   sprintf(text,"Talker Name             : %s\n",reg_sysinfo[TALKERNAME]);
write_user(user,text);
   sprintf(text,"Serial Number           : %s\n",reg_sysinfo[SERIALNUM]);
write_user(user,text);
   sprintf(text,"Server Domain           : %s\n",reg_sysinfo[SERVERDNS]);
write_user(user,text);
   sprintf(text,"       IP               : %s\n",reg_sysinfo[SERVERIP]);
write_user(user,text);
   sprintf(text,"       Port             : %d\n",port[0]);
write_user(user,text);
   sprintf(text,"Talker E-Mail           : %s\n",reg_sysinfo[TALKERMAIL]);
write_user(user,text);
   sprintf(text,"Talker Website          : %s\n",reg_sysinfo[TALKERHTTP]);
write_user(user,text);
   sprintf(text,"Sysop Name              : %s\n",reg_sysinfo[SYSOPNAME]);
if (!(!strcmp(reg_sysinfo[SYSOPNAME],reg_sysinfo[SYSOPUNAME]))) write_user(user,text);
   sprintf(text,"Sysop Username          : %s\n",reg_sysinfo[SYSOPUNAME]);
write_user(user,text);
   sprintf(text,"Pueblo-Enhanced         : %s\n",noyes2[pueblo_enh]);
write_user(user,text);
   sprintf(text,"High-Level Debug System : %s\n\n",debugstat[tempstat]);
write_user(user,text);
}





/*----------------------- TalkerOS Security Firewall -----------------v2.0--*/
fw_signoff(user)
UR_OBJECT user;
{
sprintf(text,"~FR~OLSIGN OFF:~RS %s has been disconnected by the system firewall.\n",user->name);
write_room_except(NULL,text,user);
close(user->socket);  
destruct_user(user);
destructed=0;
}

chk_firewall(user)
UR_OBJECT user;
{
char *site_search,*fw_search;
int i,result,tmp;
i = 0;  tmp=0;
if (allow_namefail && allow_sitefail) return;
if (!allow_local && !strcmp(user->site,"localhost")) {
        write_user(user,"FIREWALL:  Logins from 'localhost' are currently restricted!\n           DISCONNECTING YOU...\n\n");
        fw_signoff(user);
        sprintf(text,"%s[ FIREWALL - Localhost login denied! ]\n",colors[CSYSTEM]);
        write_duty(check_at_lev,text,NULL,NULL,0);
        return 0;
        }
if (user->level < check_at_lev) return;
if (!allow_wiz_on_main && user->port==port[0]) {
        write_user(user,"FIREWALL:  Wizards are not allowed to login on this port!\n           DISCONNECTING YOU...\n\n");
        fw_signoff(user);
        sprintf(text,"%s[ FIREWALL - Wizard login on MainPort denied! ]\n",colors[CSYSTEM]);
        write_duty(check_at_lev,text,NULL,NULL,0);
        return 0;
        }
if ((site_search=(char *)malloc(81))==NULL) {
        write_syslog("ERROR: Failed to allocate memory in chk_firewall() for *site_search.\n",0,SYSLOG);
        write_syslog("ERROR: Failed to allocate memory in chk_firewall() for *site_search.\n",0,FIRELOG);
        return 0;
        }
if ((fw_search=(char *)malloc(81))==NULL) {
        write_syslog("ERROR: Failed to allocate memory in chk_firewall() for *fw_search.\n",0,SYSLOG);
        write_syslog("ERROR: Failed to allocate memory in chk_firewall() for *fw_search.\n",0,FIRELOG);
        free(site_search);
        return 0;
        }
result=-1;
for (i=0; i < total_fwusers; i++) {
        if (!strcmp(user->name,fire_name[i])) result=i;
        }
if (result==-1 && !allow_namefail) {
        sprintf(text,"User ID \"%s\" not authorized for level %d!\n",user->name,user->level);
        write_syslog(text,1,FIRELOG);
        sprintf(text,"~FR[ FIREWALL VIOLATION - Name:Level ]\n");
        write_level(check_at_lev,1,text,user);
        if (namefail_temp) {
                user->level=revert_level;
                user->cloaklev=revert_level;
                }
        else {
                user->orig_level=revert_level;
                user->level=revert_level;
                user->cloaklev=revert_level;
                }
        save_user_details(user,1);
        free(site_search);        free(fw_search);
        if (namefail_kill) {
                fw_signoff(user);
                return 0;
                }
        sprintf(text,"%s>> ATTENTION - System Firewall Violation Detected! <<\n",colors[CSYSTEM]);
        write_user(user,text);
        return 1;
        }
if (allow_sitefail) { free(site_search);        free(fw_search);  return 1; }
strcpy(site_search,user->site);
if (!allow_namefail) {
        strcpy(fw_search,fire_main[result]);
        if (strstr(site_search,fw_search)) { free(site_search);  free(fw_search);  return 1; }
        }
else {
        for (i=0; i<total_fwusers; i++) {
                strcpy(fw_search,fire_main[i]);
                if (strstr(site_search,fw_search)) { free(site_search); free(fw_search); return 1; }
                }
        }
if (allow_altsite) {
        if (!allow_namefail) {
                strcpy(fw_search,fire_alt[result]);
                if (strstr(site_search,fw_search)) {
                        free(site_search);
                        free(fw_search);
                        return 1;
                        }
                }
        else {
                for (i=0; i<total_fwusers; i++) {
                        strcpy(fw_search,fire_alt[i]);
                        if (strstr(site_search,fw_search)) {
                                free(site_search);
                                free(fw_search);
                                return 1;
                                }
                        }
                }
        }
sprintf(text,"User ID \"%s\" not authorized for site %s!\n",user->name,user->site);
write_syslog(text,1,FIRELOG);
sprintf(text,"~FR[ FIREWALL VIOLATION - Site/IP ]\n");
write_level(check_at_lev,1,text,user);
if (sitefail_temp) {
        user->level=revert_site;
        user->cloaklev=revert_site;
        }
else {
        user->orig_level=revert_site;
        user->level=revert_site;
        user->cloaklev=revert_site;
        }
save_user_details(user,1);
free(site_search);
free(fw_search);
if (sitefail_kill) {
        fw_signoff(user);
        return 0;
        }
sprintf(text,"%s>> ATTENTION - System Firewall Violation Detected! <<\n",colors[CSYSTEM]);
write_user(user,text);
return 1;
}

list_firewall(user)
UR_OBJECT user;
{
int i,stat;
char revertstat[4][11]={ "OFF","Check NAME","Check SITE","Check ALL" };
char onFail[2][12]={ "DEMOTE","TEMP-DEMOTE" };
i=0; stat=0;

if (!allow_namefail) stat++;
if (!allow_sitefail) stat=stat+2;
if (user->orig_level<check_at_lev) { write_user(user,"Firewall access denied.\n");
                                     sprintf(text,"%s[ %s attempted to view the firewall information. ]\n",colors[CSYSTEM],user->name);
                                     write_duty(check_at_lev,text,NULL,user,0); return; }
write_user(user,"\n~BB~FW>>>                    ~OLTalkerOS:  Firewall Information                       ~RS~BB~FW<<<\n\n");
write_user(user,"~FY USER________  MAIN SITE~FY_____________________");
if (allow_altsite) write_user(user,"~FY  ALT SITE______________________\n");
else write_user(user,"\n");
for (i=0; i < total_fwusers; i++) {
        if (allow_altsite) sprintf(text," %-12s  %-30s  %s\n",fire_name[i],fire_main[i],fire_alt[i]);
        else sprintf(text," %-12s  %s\n",fire_name[i],fire_main[i]);
        write_user(user,text);
        }
/* Firewall Setup Information */
write_user(user,"\n");
sprintf(text," Total users authorized   : %-3d           Begin checking at level  : %s\n",total_fwusers,level_name[check_at_lev]); write_user(user,text);
sprintf(text," Allowing wizards on MAIN : %-3s           Allowing localhost logins: %-3s\n",noyes2[allow_wiz_on_main],noyes2[allow_local]); write_user(user,text);
sprintf(text," Alternate site enabled   : %-3s           Firewall mode            : %s\n",noyes2[allow_altsite],revertstat[stat]); write_user(user,text);
sprintf(text," When NAME/LEVEL fails    : %-11s   When SITE/IP fails       : %s\n",onFail[namefail_temp],onFail[sitefail_temp]); write_user(user,text);
sprintf(text," Revert level for bad NAME: %-10s    Revert level for bad SITE: %s\n",level_name[revert_level],level_name[revert_site]); write_user(user,text);
sprintf(text," KILL on NAME mismatch    : %-3s           KILL on SITE mismatch    : %-3s\n",noyes2[namefail_kill],noyes2[sitefail_kill]); write_user(user,text);
write_user(user,"\n");
}
/*--------------------------------------------------------------------------*/

/* PUEBLO-Enhancement : Main Features */

/* Online init. of pueblo.  (In case the login doesn't do it.) */
chk_pblo(user,str)
UR_OBJECT user;
char *str;
{
if (user->pueblo==1) {
        user->pblodetect=0;
        if (!strncmp(str,"PUEBLOCLIENT",12)) return 1;
                else return 0;
        }
if (!user->pblodetect) {
        if (!strncmp(str,"PUEBLOCLIENT",12)) return 1;
                else return 0;
        }
if (!strncmp(str,"PUEBLOCLIENT",12)) {
                user->pueblo=1;
                user->pblodetect=0;
                write_user(user,"</xch_mudtext><font size=+2 color=\"#FFFFFF\">Pueblo has been detected!</font><xch_mudtext>\n");
                return 1;
                }
user->pueblo=0;
user->pblodetect=0;
return 0;
}

/* Executes all the main features. */
pblo_exec(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
inpstr=remove_first(inpstr);
if (!user->pueblo) return;
if (!strcmp(word[1],"audioSTOP")) {
        write_user(user,"</xch_mudtext><img xch_sound=stop><xch_mudtext>");
        return;
        }
if (!strcmp(word[1],"rmAccess")) {
        click_rm_access(user);
        return;
        }
if (!strcmp(word[1],"RoomConfig_setOpt")) {
        if (user->room->personal) { write_user(user,"Personal rooms do not have access controls.\n"); return; }
        if ( user->level < com_level[FIX]
                && user->level < com_level[UNFIX]
                && user->level < toscom_level[CLRTOPIC]
                && user->level < com_level[REVCLR]
                && user->level < toscom_level[RMDESC]) {
                        if ( user->level >= com_level[PUBCOM]
                             && user->level >= com_level[PRIVCOM] ) {
                                clear_words();
                                if (user->room->access==PUBLIC) { com_num=PRIVCOM; set_room_access(user); return; }
                                if (user->room->access==PRIVATE) { com_num=PUBCOM;  set_room_access(user); return; }
                                }
                        return;
                        }
        sprintf(text,"\n ---- Room Configuration Options : %s ----\n\n",user->room->name);  write_user(user,text);
        if (user->level >= com_level[FIX] && user->level >= com_level[UNFIX]) {
                sprintf(text,"        Room Type:  </xch_mudtext><b><a xch_cmd=\".fix %s\" xch_hint=\"Set this room's type to FIXED access.\">FIXED</a> / <a xch_cmd=\".unfix %s\" xch_hint=\"Set this room's type to VARIABLE access.\">VARIABLE</a></b><xch_mudtext>\n",user->room->name,user->room->name);
                write_user(user,text);
                }
        if (user->level >= com_level[PUBCOM] && user->level >= com_level[PRIVCOM]) {
                sprintf(text,"           Access:  </xch_mudtext><b><a xch_cmd=\".pbloenh rmAccess PUBLIC %s\" xch_hint=\"Set this room's access to PUBLIC.\">PUBLIC</a> / <a xch_cmd=\".pbloenh rmAccess PRIVATE %s\" xch_hint=\"Set this room's access to PRIVATE.\">PRIVATE</a></b><xch_mudtext>\n",user->room->name,user->room->name);
                write_user(user,text);
                }
        if (user->level >= com_level[REVCLR]) {
                sprintf(text,"    Review Buffer:  </xch_mudtext><b><a xch_cmd=\".revclr %s\" xch_hint=\"Clear this room's speech review buffer.\">CLEAR REVIEW BUFFER NOW</a></b><xch_mudtext>\n",user->room->name);
                write_user(user,text);
                }                
        if (user->level >= toscom_level[CLRTOPIC]) {
                sprintf(text,"       Room Topic:  </xch_mudtext><b><a xch_cmd=\".clrtopic %s\" xch_hint=\"Clear this room's topic.\">CLEAR TOPIC NOW</a></b><xch_mudtext>\n",user->room->name);
                write_user(user,text);
                }
        if (user->level >= toscom_level[RMDESC] && user->can_edit_rooms && user->room->personal!=1) {
                sprintf(text," Room Description:  </xch_mudtext><b><a xch_cmd=\".rmdesc %s\" xch_hint=\"EDIT this room's description.\">EDIT DESCRIPTION</a></b><xch_mudtext>\n",user->room->name);
                write_user(user,text);
                }
        if (user->level >= toscom_level[RMDESC] && user->room->personal==1 && !strcmp(user->name,user->room->owner)) {
                sprintf(text," Room Description:  </xch_mudtext><b><a xch_cmd=\".rmdesc\" xch_hint=\"Edit your PERSONAL ROOM description.\">EDIT DESCRIPTION</a></b><xch_mudtext>\n");
                write_user(user,text);
                }
        write_user(user,"\n");
        return;
        }
if (word_count<3) return;
if (!strcmp(word[1],"vpic")) {
        sprintf(text,"</xch_mudtext><br><img src=\"%s\"><br><xch_mudtext>",inpstr);
        write_user(user,text);
        return;
        }
if (!strcmp(word[1],"audioPLAY")) {
        sprintf(text,"</xch_mudtext><img xch_sound=play href=\"%s\"><xch_mudtext>",inpstr);        write_user(user,text);
        return;
        }
if (!strcmp(word[1],"audioVOL")) {
        if (!strncmp(word[2],"muteVOL")) write_user(user,"</xch_mudtext><img xch_volume=0><xch_mudtext>");
        if (!strncmp(word[2],"minVOL")) write_user(user,"</xch_mudtext><img xch_volume=33><xch_mudtext>");
        if (!strncmp(word[2],"medVOL")) write_user(user,"</xch_mudtext><img xch_volume=66><xch_mudtext>");
        if (!strncmp(word[2],"maxVOL")) write_user(user,"</xch_mudtext><img xch_volume=100><xch_mudtext>");
        return;
        }
}

click_rm_access(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
char *name;
int cnt;

        if ((rm=get_room(word[3]))==NULL) {
                write_user(user,nosuchroom);  return;
                }
        if (user->level<gatecrash_level && rm!=user->room) {
                write_user(user,"You have left that room.  Cannot perform access change.\n");
                return;
                }
        if (rm->personal) {
                write_user(user,"You cannot change the access of a personal room.\n");
                return;
                }
if (user->vis) name=user->name; else name=invisname;
if (rm->access>PRIVATE) {
        if (rm==user->room) 
                write_user(user,"This room's access is fixed.\n"); 
        else write_user(user,"That room's access is fixed.\n");
        return;
        }
if (!strcmp(word[2],"PUBLIC") && rm->access==PUBLIC) {
        if (rm==user->room) 
                write_user(user,"This room is already public.\n");  
        else write_user(user,"That room is already public.\n"); 
        return;
        }
if (user->vis) name=user->name; else name=invisname;
if (!strcmp(word[2],"PRIVATE")) {
        if (rm->access==PRIVATE) {
                if (rm==user->room) 
                        write_user(user,"This room is already private.\n");  
                else write_user(user,"That room is already private.\n"); 
                return;
                }
        cnt=0;
        for(u=user_first;u!=NULL;u=u->next) if (u->room==rm) ++cnt;
        if (cnt<min_private_users && user->level<ignore_mp_level) {
                sprintf(text,"You need at least %d users/clones in a room before it can be made private.\n",min_private_users);
                write_user(user,text);
                return;
                }
        write_user(user,"Room set to ~FRPRIVATE.\n");
        if (rm==user->room) {
                sprintf(text,"%s has set the room to ~FRPRIVATE.\n",name);
                write_room_except(rm,text,user);
                }
        else write_room(rm,"This room has been set to ~FRPRIVATE.\n");
        rm->access=PRIVATE;
        return;
        }
write_user(user,"Room set to ~FGPUBLIC.\n");
if (rm==user->room) {
        sprintf(text,"%s has set the room to ~FGPUBLIC.\n",name);
        write_room_except(rm,text,user);
        }
else write_room(rm,"This room has been set to ~FGPUBLIC.\n");
rm->access=PUBLIC;

/* Reset any invites into the room & clear review buffer */
for(u=user_first;u!=NULL;u=u->next) {
        if (u->invite_room==rm) u->invite_room=NULL;
        }
clear_revbuff(rm);
}


query_img(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
char *name;

if (word_count<2) { write_user(user,"Usage:  ppic <picture URL>\n"); return; }
if (contains_swearing(inpstr)) { write_user(user,noswearing); return; }
if (!strncmp(word[1],"http://",7)) {
        if (!(contains_extension(inpstr,0))) {
                write_user(user,"URL must end in either .jpg or .gif for a picture.\n");
                return;
                }
        if (user->vis) name=user->name;  else name=invisname;
        if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,user,0); }
        sprintf(text,"%s suggests picture: %s\n",name,inpstr);
        write_room(user->room,text);
        sprintf(text,"-->  [ </xch_mudtext><a xch_cmd=\".pbloenh vpic %s\" xch_hint=\"View picture.\"><b>CLICK to view picture</b></a><xch_mudtext> ]\n",inpstr);
        for (u=user_first; u!=NULL; u=u->next) {
                if (!u->pueblo || u->login || u->room!=user->room) continue;
                write_user(u,text);
                }
        }
else { write_user(user,"URL must begin with:  http://\n"); return; }
}

query_aud(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT u;
char *name;

if (word_count<2) { write_user(user,"Usage:  paudio <soundfile URL>\n"); return; }
if (contains_swearing(inpstr)) { write_user(user,noswearing); return; }
if (!strncmp(word[1],"http://",7)) {
        if (!(contains_extension(inpstr,1))) {
                write_user(user,"URL must end in either .wav or .mid for an audio clip.\n");
                return;
                } 
        if (user->vis) name=user->name;  else name=invisname;
        if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,user,0); }
        sprintf(text,"%s suggests audio: %s\n",name,inpstr);
        write_room(user->room,text);
        sprintf(text,"~FMo/~ ~FYAudioPlayer~FM o/~~RS </xch_mudtext><b>[ <a xch_cmd=\".pbloenh audioSTOP\" xch_hint=\"Stop all audio.\">STOP</a> | <a xch_cmd=\".pbloenh audioPLAY %s\" xch_hint=\"Play this file.\">PLAY</a> |</b> Volume <b><a xch_cmd=\".pbloenh audioVOL muteVOL\" xch_hint=\"Mute\">Mute</a> : <a xch_cmd=\".pbloenh audioVOL minVOL\" xch_hint=\"Low Volume\">Soft</a> : <a xch_cmd=\".pbloenh audioVOL medVOL\" xch_hint=\"Normal Volume\">Normal</a> : <a xch_cmd=\".pbloenh audioVOL maxVOL\" xch_hint=\"High Volume\">Loud</a> ]</b><br><xch_mudtext>",inpstr);
        for (u=user_first; u!=NULL; u=u->next) {
                if (!u->pueblo || u->login || u->room!=user->room) continue;
                write_user(u,text);
                }
        }
else { write_user(user,"URL must begin with:  http://\n"); return; }
}

pblo_jukebox(user)
UR_OBJECT user;
{
int i,cnt,pos;
i=0; cnt=0; pos=0;
if (word_count<2) {
        for (i=0; jb_titles[i][0]!='*'; i++) {
                cnt++;
                if (i==0) write_user(user,"Jukebox Song List\n\n");
                sprintf(text,"%3d: %-33s ",cnt,jb_titles[i]);
                write_user(user,text);
                if (pos) { write_user(user,"\n"); pos=0; }
                else pos=1;
                }
        if (pos) write_user(user,"\n");
        if (cnt) write_user(user,"\nUsage:  .jukebox <songnumber>\n");
        else write_user(user,"There are no song titles loaded in the jukebox.\n");
        return;
        }
if (isnumber(word[1])) cnt=atoi(word[1]);
else { write_user(user,"Usage:  .jukebox <songnumber>\n"); return; }
for (i=0; jb_titles[i][0]!='*'; i++) {
        if (i==cnt-1) { disp_song(user,i); break; }
        }
if (i!=cnt-1) write_user(user,"Song not found.\n");
}

disp_song(user,num)
UR_OBJECT user;
int num;
{
UR_OBJECT u;
char *name;

if (user->vis) name=user->name;  else name=invisname;
if (!user->vis) { sprintf(text,"%s[%s] ",colors[CSYSTEM],user->name); write_duty(user->level,text,user->room,user,0); }
sprintf(text,"o/~ %s plays \"%s\" on the Jukebox. o/~\n",name,jb_titles[num]);
write_room(user->room,text);
sprintf(text,"~FMo/~ ~FYAudioPlayer~FM o/~~RS </xch_mudtext><b>[ <a xch_cmd=\".pbloenh audioSTOP\" xch_hint=\"Stop all audio.\">STOP</a> | <a xch_cmd=\".pbloenh audioPLAY %s%s%s\" xch_hint=\"Play this file.\">PLAY</a> |</b> Volume <b><a xch_cmd=\".pbloenh audioVOL muteVOL\" xch_hint=\"Mute\">Mute</a> : <a xch_cmd=\".pbloenh audioVOL minVOL\" xch_hint=\"Low Volume\">Soft</a> : <a xch_cmd=\".pbloenh audioVOL medVOL\" xch_hint=\"Normal Volume\">Normal</a> : <a xch_cmd=\".pbloenh audioVOL maxVOL\" xch_hint=\"High Volume\">Loud</a> ]</b><br><xch_mudtext>",reg_sysinfo[TALKERHTTP],reg_sysinfo[PUEBLOWEB],jb_files[num]);
for (u=user_first; u!=NULL; u=u->next) {
        if (u->login || u->room!=user->room) continue;
        if (u->pueblo) write_user(u,text);
        }
}


/* Relist EXITS ... This will work for anyone, but users with Pueblo can
                    click on the non-private rooms to go there.  Netlinks
                    are not clickable, because they sometimes require pwds. */
pblo_listexits(user)
UR_OBJECT user;
{
RM_OBJECT rm;
char temp[125];
int i,exits;

rm=user->room;

if (!rm->personal) sprintf(text,"The exits for this room (%s) are:",rm->name);
              else sprintf(text,"The exits for this personal room (%s) are:",rm->owner);
write_user(user,text);
strcpy(text,"\n");
for(i=0;i<MAX_LINKS;++i) {
        if (rm->link[i]==NULL) break;
        if (rm->link[i]->access & PRIVATE)
                if (user->pueblo) sprintf(temp,"  ~FR%s",rm->link[i]->name);
                else sprintf(temp,"  ~FR%s",rm->link[i]->name);
        else {
                if (user->pueblo) sprintf(temp,"  </xch_mudtext><b><a xch_cmd=\".go %s\" xch_hint=\"Go to this room.\">%s</a></b><xch_mudtext>",rm->link[i]->name,rm->link[i]->name);
                else sprintf(temp,"  ~FG%s",rm->link[i]->name);
                }
        strcat(text,temp);
        ++exits;
        }
if (rm->netlink!=NULL && rm->netlink->stage==UP) {
        if (rm->netlink->allow==IN) sprintf(temp,"  ~FR%s*",rm->netlink->service);
        else sprintf(temp,"  ~FG%s*",rm->netlink->service);
        strcat(text,temp);
        }
else if (!exits) strcpy(text,"\n~FY(!) ~FRThere are no exits. ~FY(!)");
strcat(text,"\n");
write_user(user,text);
write_user(user," * indicates a netlink.\n\n");
}



/*---------------------------------------------------------------------------
  TalkerBOT v2.30                                                           */

/* Version 2.3 of the TalkerBOT code supports custom responses for EACH
   individual bot, if you choose.  If a custom response file does not exist,
   the default file is used.  If a default file does not exist, your bot will
   just sit there.

                 Custom response files are stored in:
                 talkerDIR/botfiles/Botname/filename.T

   You will have to make the dir & custom files if you want them.  Simply copy
   the default trigger files that you made in the /botfiles directory into
   the specific bot's directory.  Leave out those that don't need custom
   responses, like hello.T for instance.
*/

/* bot command control function */
bot_ctrl(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT bot;
int botcom,i,n;
char *botcmds[]={
"help","create","load","unload","delete","say","go","home","shout","tell",
"emote","semote","desc","inphr","outphr","think","set","shift","sshift",
"sing","alias","ltrig","*"};
if (word_count<2) {
        i=0; n=0;
        while(botcmds[i][0]!='*') {
                if (i==0) write_user(user,"TalkerBOT v2.3 - Commands\n\n");
                if (!n) write_user(user,"   ");
                sprintf(text,"%-10s  ",botcmds[i]);
                write_user(user,text);
                i++;
                n++;
                if (n>=6) { n=0; write_user(user,"\n"); }
                }
        if (n!=0) write_user(user,"\n");
        write_user(user,"\n");
        i=0;
        for (bot=user_first; bot!=NULL; bot=bot->next) {
                if (bot->type==BOT_TYPE) { i=1; break; }
                }
        if (!i) write_user(user,"The bot system is currently not active.\n");
        else {
                sprintf(text,"The bot \"%s\" is currently active.\n",bot->name);
                write_user(user,text);
                }
        return;
        }
i=0; botcom=-1;
while(botcmds[i][0]!='*') {
        if (!strcmp(word[1],botcmds[i])) botcom=i;
        i++;
        }
if (botcom==-1) {
        write_user(user,"Invalid bot command.\n");
        return;
        }
if (botcom>4) {
        i=0;
        for (bot=user_first; bot!=NULL; bot=bot->next) {
                if (bot->type==BOT_TYPE) { i=1; break; }
                }
        if (botcom!=21 && !i) { write_user(user,"The bot is not currently loaded.\n"); return; }
        if (botcom!=21 && botcom!=7 && word_count<3) { write_user(user,"Bot command syntax error.\n"); return; }
        inpstr=remove_first(inpstr);
        }
switch(botcom) {
        case  0:  bot_help(user);  break;               /* help */
        case  1:  bot_file(user,0);  break;             /* create */
        case  2:  bot_load(user,0);  break;             /* load */
        case  3:  bot_load(user,1);  break;             /* unload */
        case  4:  bot_file(user,1);  break;             /* delete */
        case  5:  say(bot,inpstr);  bot->last_input=time(0);  break;              /* say */
        case  6:  bot_go(bot);  bot->last_input=time(0);  break;                  /* go */
        case  7:  home(bot);  bot->last_input=time(0);  break;                    /* home */
        case  8:  shout(bot,inpstr);  bot->last_input=time(0);  break;            /* shout */
        case  9:  bot_tell(bot,inpstr);  bot->last_input=time(0);  break;         /* tell */
        case 10:  emote(bot,inpstr);  bot->last_input=time(0);  break;            /* emote */
        case 11:  semote(bot,inpstr);  bot->last_input=time(0);  break;           /* semote */
        case 12:  bot_desc(bot,inpstr);  break;         /* description */
        case 13:  set_bot_iophrase(bot,0,inpstr); break;/* in/out phrase */
        case 14:  set_bot_iophrase(bot,1,inpstr); break;/* in/out phrase */
        case 15:  think(bot,inpstr);  bot->last_input=time(0);  break;            /* think */
        case 16:  set_bot_stats(bot,inpstr);  break;    /* set */
        case 17:  char_shift(bot,inpstr);  bot->last_input=time(0);  break;       /* shift */
        case 18:  char_shift_shout(bot,inpstr);  bot->last_input=time(0);  break; /* shout-shift */
        case 19:  sing(bot,inpstr);  bot->last_input=time(0);  break;             /* sing */
        case 20:  set_bot_alias(bot);  break;           /* alias */
        case 21:  bot_list_triggers(user);  break;      /* list triggers */
        default:  write_user(user,"BotCommand not executed in bot_ctrl().\n");
                  break;
        }
}


bot_help(user)
UR_OBJECT user;
{
write_user(user,"Bot Help\n-------\n");
write_user(user,"To activate the default bot, type        :  ~FG.bot load\nTo activate a specific bot, type         :  ~FG.bot load <botname>\n");
write_user(user,"To deactivate the current bot, type      :  ~FG.bot unload\n");
write_user(user,"To create AND load a new bot, type       :  ~FG.bot create <newname>\n");
write_user(user,"To delete an existing bot, type          :  ~FG.bot delete <botname>\n\n");
write_user(user,"To perform any other bot function, type  :  ~FG.bot <command> <parameters>\n");
write_user(user,"To see a list of available commands, type:  ~FG.bot\n\n");
}

bot_file(user,op)
UR_OBJECT user;
int op;
{
UR_OBJECT bot;
FILE *fp;
char filename[80];
int saveOK,i;

if (word_count<3) { write_user(user,"Usage:  bot <create/delete> <botname>\n"); return; }
/* check validity of new name and do CAPS stuff */
for (i=0;i<strlen(word[2]);i++) {
        if (!isalpha(word[2][i])) {
                write_user(user,"\n-=- Only letters are allowed in the bot's name. -=-\n\n");
                return;
                }
        if (!i) word[2][0] = toupper(word[2][0]);
        else if (!allow_caps_in_name) word[2][i] = tolower(word[2][i]);
        }
sprintf(filename,"%s/%s.bot",BOTFILES,word[2]);
if (!op) { /* Create new bot. */
        saveOK=1;
        for (bot=user_first; bot!=NULL; bot=bot->next) {
                if (bot->type==BOT_TYPE) saveOK=0;
                }
        if (fp=fopen(filename,"r")) {
                fclose(fp);
                write_user(user,"Bot already exists!  -  Cannot create new file.\n");
                return;
                }
        if (get_user(word[2])) {
                write_user(user,"A user or bot is currently logged in with that name!\n");
                return;
                }
        if ((bot=create_user())==NULL) {
                write_user(user,"ERROR:  Could not create new user object for bot!\n");
                write_syslog("ERROR:  Couldn't create new user object in bot_file().\n",0,SYSLOG);
                return;
                }
        strcpy(bot->name,word[2]);
        if (load_user_details(bot)) {
                write_user(user,"Existing user or bot has already registered that name.\n");
                destruct_user(bot);
                destructed=0;
                return;
                }
        if (!(fp=fopen(filename,"w"))) {
                write_user(user,"Could not create file for new bot!\n");
                sprintf(text,"Failed to create new botfile \"%s\" as executed by %s.\n",filename,user->name);
                write_syslog(text,0,SYSLOG);
                return;
                }
        /* write information to bot directory */
        fprintf(fp,"Created: %d\n     By: %s\nVersion: 2.3",(int)(time(0)),user->name);
        fclose(fp);
        sprintf(text,"Bot file created for NEW BOT: %s\n",word[2]);
        write_syslog(text,0,SYSLOG);

        /* set up bot's user structure */
        bot->type=BOT_TYPE;
        strcpy(bot->pass,"MU2Yw4yECNO.9k");
        bot->level=BOTLEV;
        bot->orig_level=bot->level;
        bot->cloaklev = bot->orig_level;
        strcpy(bot->desc,"- Talker~FTBOT~FY v2.3");
        strcpy(bot->in_phrase,"walks in"); 
        strcpy(bot->out_phrase,"walks out"); 
        strcpy(bot->last_site,"localhost");
        bot->muzzled=0;
        bot->command_mode=0;
        bot->prompt=0;
        bot->colour=0;
        bot->charmode_echo=0;
        bot->arrested=0;
        bot->duty=0;
        bot->waitfor[0]='\0';
        bot->alert=0;
        bot->age=-1;
        bot->gender=0;
        bot->pstats=1;
        bot->cloaklev = bot->level;
        bot->orig_level = bot->level;
        bot->reverse = 0;
                /* Set create time. */
                bot->created=(int)(time(0));
        save_user_details(bot,0);
        destruct_user(bot);  destructed=0; 
        write_user(user,"New bot information saved.\n");
        sprintf(text,"%s created a NEW BOT named '%s'.\n",user->name,word[2]);
        write_syslog(text,0,USERLOG);
        if (!saveOK) write_user(user,"A bot is currently in use.\nYou must unload it before the new bot may be loaded.\n");
        else bot_load(user,0);
        return;
        }
else {  /* delete existing bot */

        if (get_user(word[2])) {
                write_user(user,"A user or bot is currently logged in with that name!\n");
                return;
                }
        if (!(fp=fopen(filename,"r"))) {
                write_user(user,"Bot does not exist!\n");
                return;
                }
        fclose(fp);
        unlink(filename);
        sprintf(filename,"%s/%s.D",USERFILES,word[2]);
        unlink(filename);
        sprintf(filename,"%s/%s.M",USERFILES,word[2]);
        unlink(filename);
        sprintf(filename,"%s/%s.P",USERFILES,word[2]);
        unlink(filename);
        sprintf(filename,"%s/%s.R",USERFILES,word[2]);
        unlink(filename);
        write_user(user,"Bot deleted!\n");
        sprintf(text,"%s DELETED BOT '%s'\n",user->name,word[2]);
        write_syslog(text,0,SYSLOG);
        write_syslog(text,0,USERLOG);
        }
}

bot_load(user,unload)
UR_OBJECT user;
int unload;
{
UR_OBJECT bot;
FILE *fp;
char filename[80],line[81];
int i;

if (word_count<3) sprintf(filename,"%s/%s.bot",BOTFILES,BOT_def_name);
else {
        word[2][0]=toupper(word[2][0]);
        sprintf(filename,"%s/%s.bot",BOTFILES,word[2]);
        }
if (unload) {
        i=0;
        for (bot=user_first;bot!=NULL;bot=bot->next) {
                if (bot->type==BOT_TYPE) { i=1;  break; }
                }
        if (!i) { write_user(user,"There is no bot loaded!\n"); return; }
        sprintf(text,"%s flickers out of sight!\n",bot->name);
        write_room(bot->room,text);
        write_user(user,"Bot unloaded.\n");
        sprintf(text,"%s[ %s UNLOADED the bot. ]\n",colors[CSYSTEM],user->name);
        write_duty(user->level,text,NULL,user,0);
        sprintf(text,"%s unloaded the bot \"%s\".\n",user->name,bot->name);
        write_syslog(text,0,USERLOG);
        save_user_details(bot,1);
        destruct_user(bot);
        destructed=0;
        return;
        }
else {
        i=0;
        for (bot=user_first;bot!=NULL;bot=bot->next) {
                if (bot->type==BOT_TYPE) { i=1;  break; }
                }
        if (i) { write_user(user,"There is a bot already loaded!\n"); return; }
                
        if (!(fp=fopen(filename,"r"))) {
                write_user(user,"Bot information file not found!  -  Cannot load.\n");
                return;
                }
        fclose(fp);

        if ((bot=create_user())==NULL) {
                write_user(user,"ERROR:  Could not create and load user object for bot!\n");
                write_syslog("ERROR:  Couldn't create new user object in bot_load().\n",0,SYSLOG);
                return;
                }
        bot->type=BOT_TYPE;
        if (word_count<3) strcpy(bot->name,BOT_def_name);
        else strcpy(bot->name,word[2]);
        bot->last_login_len=0;
        if (!(load_user_details(bot))) {
                write_user(user,"ERROR:  Couldn't find userfile for bot!\n");
                destruct_user(bot);
                destructed=0;
                return;
                }
        bot->room=room_first;
        strcpy(bot->last_site,"localhost\0");
        strcpy(bot->site,"localhost\0");
        bot->last_input=time(0);
//        bot->last_login=time(0);
        bot->muzzled=0;
        bot->command_mode=0;
        bot->prompt=0;
        bot->colour=0;
        bot->charmode_echo=0;
        bot->arrested=0;
        bot->duty=0;
        bot->waitfor[0]='\0';
        bot->alert=0;
        bot->reverse = 0;
        sprintf(text,"%s fades into view!\n",bot->name);
        write_room(bot->room,text);
        write_user(user,"Bot loaded.\n");
        sprintf(text,"%s loaded bot \"%s\".\n",user->name,bot->name);
        write_syslog(text,0,USERLOG);
        sprintf(text,"%s[ %s LOADED the bot: \"%s\" ]\n",colors[CSYSTEM],user->name,bot->name);
        write_duty(user->level,text,NULL,user,0);
        }
}

set_bot_iophrase(bot,mode,inpstr)
UR_OBJECT bot;
int mode;
char *inpstr;
{
if (strlen(inpstr)>PHRASE_LEN) return;
if (!mode) {
        if (word_count<3) return;
        strcpy(bot->in_phrase,inpstr);
        return;
        }
if (word_count<3) return;
strcpy(bot->out_phrase,inpstr);
}

set_bot_stats(bot,inpstr)
UR_OBJECT bot;
char *inpstr;
{
if (word_count<3) return;
if (!strcmp(word[2],"http")) {
        if (strlen(word[3])>58) return;
        strcpy(bot->http,word[3]);
        return;
        }
if (!strcmp(word[2],"email")) {
        if (strlen(word[3])>58) return;
        strcpy(bot->email,word[3]);
        return;
        }
if (!strcmp(word[2],"gender")) {
        if (!strcmp(word[3],"none")) {
                bot->gender=0;
                return;
                }
        if (!strcmp(word[3],"male")) {
                bot->gender=1;
                }
        if (!strcmp(word[3],"female")) {
                bot->gender=2;
                return;
                }
        return;
        }
if (!strcmp(word[2],"age")) {
        if (!(isnumber(word[3]))) return;
        if (atoi(word[3])<1 || atoi(word[3])>999) return;
        bot->age = atoi(word[3]);
        return;
        }
if (!strcmp(word[2],"private")) {
        if (bot->pstats) {
                bot->pstats=0;
                save_user_details(bot,1);
                return;
                }
        else {
                bot->pstats=1;
                save_user_details(bot,1);
                return;
                }
        }
}

set_bot_alias(bot)
UR_OBJECT bot;
{
UR_OBJECT u;
if (word_count<3) return;
if (strlen(word[2])>USER_NAME_LEN) return;
if (!strcmp(word[2],"off")) {
        bot->alias[0]='\0';
        return;
        }
if (u=get_user(word[2])) return;
if (contains_swearing(word[2])) return;
if ((u=create_user())==NULL) {
        write_syslog("ERROR: Unable to create temporary user object in set_alias().\n",0,SYSLOG);
        return;
        }
strcpy(u->name,word[2]);
if (load_user_details(u)) {
        destruct_user(u); destructed=0;
        return;
        }
destruct_user(u); destructed=0;
strcpy(bot->alias,word[2]);
}

bot_list_triggers(user)
UR_OBJECT user;
{
int i,n;
        i=0; n=0;
        while(BOT_triggers[i][0]!='*') {
                if (i==0) write_user(user,"TalkerBOT v2.3 - Installed Trigger Words\n\n");
                if (!n) write_user(user,"   ");
                sprintf(text,"%-20s  ",BOT_triggers[i]);
                write_user(user,text);
                i++;
                n++;
                if (n>=3) { n=0; write_user(user,"\n"); }
                }
        if (n!=0) write_user(user,"\n");
        write_user(user,"\n");
}

bot_tell(bot,inpstr)
UR_OBJECT bot;
char *inpstr;
{
UR_OBJECT u;
char type[5],*name;

if (bot->muzzled) return;
if (!(u=get_user(word[2]))) return;
if (u==bot) return;
if (bot->vis) name=bot->name; else name=invisname;
if (u->afk) {
        inpstr=remove_first(inpstr);
        if (inpstr[strlen(inpstr)-1]=='?') strcpy(type,"ask");
        else strcpy(type,"tell");
        sprintf(text,"(AFK) ~RS~OL%s%s %ss you:~RS%s %s\n",colors[CTELLUSER],name,type,colors[CTELL],inpstr);
        record_tell(u,text);
        u->primsg++;
        return;
        }
if ((u->misc_op >= 3 && u->misc_op <= 5) || (u->misc_op >= 8 && u->misc_op <= 10)) {
        if (inpstr[strlen(inpstr)-1]=='?') strcpy(type,"ask");
        else strcpy(type,"tell");
        sprintf(text,"(EDIT) ~RS~OL%s%s %ss you:~RS%s %s\n",colors[CTELLUSER],name,type,colors[CTELL],inpstr);
        record_tell(u,text);
        u->primsg++;
        return;
        }
if (u->ignall && (bot->level<WIZ || u->level>bot->level)) return;
if (u->igntell && (bot->level<WIZ || u->level>bot->level)) return;
if (u->room==NULL) return;
inpstr=remove_first(inpstr);
if (inpstr[strlen(inpstr)-1]=='?') strcpy(type,"ask");
else strcpy(type,"tell");
if (!bot->vis && u->level >= bot->level) { sprintf(text,"%s[ %s ] ",colors[CSYSTEM],bot->name); write_user(u,text); }
sprintf(text,"~RS~OL%s%s %ss you:~RS%s %s\n",colors[CTELLUSER],name,type,colors[CTELL],inpstr);
write_user(u,text);
record_tell(u,text);
bot->last_input=time(0);
}

bot_desc(bot,inpstr)
UR_OBJECT bot;
char *inpstr;
{
if (word_count<3) return;
if (strstr(word[2],"(CLONE)")) return;
if (strlen(inpstr)>USER_DESC_LEN) return;
strcpy(bot->desc,inpstr);
}


bot_trigger(user,inpstr)
UR_OBJECT user;
char *inpstr;
{
UR_OBJECT bot,b2;
char *s,*name_search,filename[80],line[300],text2[300],*name,*str;
int bot_active,trig,i,total,sel,com,num,pos,ast_found,un_found;
FILE *fp;

bot_active=0;  trig=-1;  i=0;  total=0;  sel=0;  com=0;  num=0;  pos=0;
ast_found=0;   un_found=0;
filename[0]='\0';  line[0]='\0';  text2[0]='\0';

/* Check to see if the user has been disconnected suddenly, if so, don't respond. */
if (!get_user(user->name)) return;
/* Check to see if the bot is loaded */
for (b2=user_first;b2!=NULL;b2=b2->next) {
        if (b2->type==BOT_TYPE) { bot_active=1; bot=b2; break; }
        }
if (!bot_active) return;
if (bot==user) return;  /* Don't want the bot triggering itself */
if ((s=(char *)malloc(strlen(inpstr)+1))==NULL) {
        write_syslog("ERROR: Failed to allocate memory in bot_trigger().\n",SYSLOG);
        return;
        }
if ((name_search=(char *)malloc(strlen(bot->name)+1))==NULL) {
        write_syslog("ERROR: Failed to allocate memory in bot_trigger().\n",SYSLOG);
        free(s);
        return;
        }
if (user->vis) { name=user->name; }
else { name=invisname; }
if (!is_using_old_inpstr) strcpy(s,inpstr);
        else { free(s);  free(name_search);  return; }
strcpy(name_search,bot->name);
strtolower(s); 
strtolower(name_search);
if (bot->room!=user->room) return;
if (strstr(s,name_search) || strstr(s,"login!@#$%^&*()greet)(*&^%$#@!nigol")) {
        while(BOT_triggers[i][0]!='*') {
                if (strstr(s,BOT_triggers[i])) { trig=i;  break; }
                i++;
                }

        /* If the bot's name is found, but no trigger in text,
           do the default file. */
        if (trig==-1) {
                i=0;  trig=-1;  ast_found=0;
                while(!ast_found && BOT_triggers[i]!=NULL) {
                        if (BOT_triggers[i][0]=='*') { ast_found=1; trig=i; }
                        i++;
                        }
                if (BOT_triggers[i]==NULL) 
                        write_syslog("ERROR:  NULL value found in BOT_triggers[]!\n        Make sure '*' is the last entry in the trigger array!\n",0,SYSLOG);
                if (trig==-1) { free(s); free(name_search); return; }
                }

        /* Do the custom file search */
        sprintf(filename,"%s/%s/%s.T",BOTFILES,bot->name,BOT_trigger_files[trig]);
        if (!(fp=fopen(filename,"r"))) {
                sprintf(filename,"%s/%s.T",BOTFILES,BOT_trigger_files[trig]);
                if (!(fp=fopen(filename,"r"))) { free(s); free(name_search); return; }
                }

        fscanf(fp,"%d\n",&total); /* total file entries */
        sel = rand() % total;
        sel++;  /* Have to increase because the rand might leave it 0. */
        /* Seek to selected line in file */
        for(num=0; (num<sel && !feof(fp)); num++) {
                fgets(line,300,fp);     /* Read the desired line */
                }
        if (feof(fp)) { fclose(fp); return; }
        fclose(fp);
        line[strlen(line)-1]='\0';
        terminate(line);
        if (line[0]=='.') com=1;                                /* say   */
                else if (line[0]==';') com=2;                   /* emote */
                        else if (line[0]=='!') com=4;           /* shout */
                                else if (line[0]==',') com=3;   /* think */
                                        else { free(s); free(name_search); return; }
        for (i=0; i<strlen(line); i++) line[i]=line[i+1];
        line[i]='\0';
        if (com==1) sprintf(text,"%s[%s says]%s: ",colors[CUSER],bot->name,colors[CTEXT]);
        if (com==2) sprintf(text,"%s%s ",colors[CEMOTE],bot->name);
        if (com==3) sprintf(text,"%s%s thinks . o O ( ",colors[CTHINK],bot->name);
        if (com==4) sprintf(text,"%s%s%s shouts:%s ",colors[CBOLD],colors[CSHOUT],bot->name,colors[CTEXT]);
        pos=0;  i=0;
        for (pos=0; pos<strlen(line); pos++) {
                if (line[pos]=='%' && line[pos+1]=='s') {
                        pos=pos+2;
                        un_found=1;  /* if the %s was found, then put the
                                        username in, else, don't. */
                        break;
                        }
                else {
                        text2[i]=line[pos];
                        i++;
                        }
                }
        text2[i]='\0';
        if (!un_found) name="";  /* get rid of name if not found */
        str=text2;
        sprintf(text2,"%s%s",str,name);
        strcat(text,text2);
        i=0;
        for (i=0; pos<strlen(line); pos++) {
                if (line[pos]=='%' && line[pos+1]=='s') {
                        pos=pos+2;
                        }
                else {
                        text2[i]=line[pos];
                        i++;
                        }
                }
        text2[i]='\0';
        str=text2;
        if (com!=3) sprintf(text2,"%s\n",str);
                else sprintf(text2,"%s %s)\n",str,colors[CTHINK]);
        strcat(text,text2);
        if (com!=4) write_room_except(bot->room,text,bot);
                else write_room_except(NULL,text,bot);
        record(bot->room,text);

        bot->last_input=time(0);
        if (!strcmp(BOT_triggers[trig],"kill")) {  /* this if statement might
                                                      kill a user if they entered
                                                      the "kill" trigger. hehe */
                if ((rand() % 20)>9) { disconnect_user(user); }
                }
        free(s);  free(name_search);
        return;
        }
}

bot_go(bot) /* Bots do not have netlink privledges - Errors occur using .go */
UR_OBJECT bot;
{
RM_OBJECT rm;
int i;

if ((rm=get_room(word[2]))==NULL) return;
if (rm==bot->room) return;

bot->last_input=time(0);
/* See if link from current room */
if (!rm->personal) {
        for(i=0;i<MAX_LINKS;++i) {
                if (bot->room->link[i]==rm) {
                        move_user(bot,rm,0);  return;
                        }
                }
        if (bot->level<WIZ) {
                return;
                }
        } else { move_user(bot,rm,0); return; }
move_user(bot,rm,1);
}

/***************** User Idle Time Information ********************/
disp_idletime(user)
UR_OBJECT user;
{
UR_OBJECT u;
int idle,mins,stat,temp;
char attn[3][4]={"~FW","~FY","~FR"};
char attn2[3][2]={"-","/","!"};

if (word_count<2) {
        write_user(user,"Usage:  idle ALL/<user>\n");
        return;
        }
if (!strcmp(word[1],"ALL") || !strcmp(word[1],"all")) {
        sprintf(text,"\n -=-=-=-=-=-=-=-=-=- User idle times at %s -=-=-=-=-=-=-=-=-=-\n\n",reg_sysinfo[TALKERNAME]);
        write_user(user,text);
        for (u=user_first;u!=NULL;u=u->next) {
                if (u->login || u->type!=USER_TYPE) continue;
                if (user->level<u->level && !u->vis) continue;
                mins=(int)(time(0) - u->last_login)/60;
                temp=(int)(time(0) - u->last_input)/60;
                idle = u->idle + temp;
                stat=0;
                if ((mins/2)<idle) stat=1;  /* Idled 50% of total time on. */
                if (((mins/4)*3)<idle) stat=2;  /* Idled 75% of total time on. */
                if (idle<(user_idle_time/60)) stat=0;
                sprintf(text,"~BK~FK%s~RS%s %-12s has idled %3d of %4d total minutes connected.\n",attn2[stat],attn[stat],u->name,idle,mins);
                write_user(user,text);
                }
        write_user(user,"\n      ~BW~FW -~RS = Okay   ~BY~FY /~RS = Idled over 50%   ~BR~FR !~RS = Idled over 75%\n\n");
        return;
        }
if (!(u=get_user(word[1]))) { write_user(user,notloggedon); return; }
if (user->level<u->level && !u->vis) { write_user(user,notloggedon); return; }
mins=(int)(time(0) - u->last_login)/60;
idle=u->idle;
sprintf(text," %-12s has idled %3d of %4d total minutes connected.\n",u->name,idle,mins);
write_user(user,text);
}

/*------------------------ The TalkerOS Multi-Move ------------------------*/
multi_move(user)
UR_OBJECT user;
{
RM_OBJECT rm,final_rm,old_room;
int num_moves,num_ok,pos,err,i,has_link,has_access,first_err;

num_moves = word_count - 1;
num_ok=0; pos=1;  err=0;  first_err=0;

if (num_moves > 8) num_moves = 8;       /* Make sure we don't exceed the
                                           word[] array dimensions */
/* check that all rooms are valid */
for (pos=1; pos<num_moves+1; pos++) {
        if (err) continue;
        if ((rm=get_room(word[pos]))==NULL) {
                sprintf(text,"multiMOVE:  Room #%d (%s) does not exist.\n",pos,word[pos]);
                write_user(user,text);
                err=pos;
                continue;
                }
        if (rm->personal) {
                write_user(user,"multiMOVE:  Room hopping can not involve personal rooms.\n");
                err=pos;
                continue;
                }
        num_ok++;
        }
if (num_ok==0) { write_user(user,"An error occurred on your first room choice.\nThe operation could not be completed.\n"); return; }

/* store first_err because err will be reused */
first_err=err;
err=0;

/* store original room for reset_access */
old_room=user->room;

/* set the final_rm even though the user still might not be able to get there */
final_rm=get_room(word[num_ok]);

/* check that user can jump to that room, and if so, go there. */
for (pos=1; pos<=num_ok; pos++) {
        if (err) continue;
        rm=get_room(word[pos]);

        /* See if link from current room */
        has_link=0;
        for(i=0;i<MAX_LINKS;++i) if (user->room->link[i]==rm) has_link=1;
        if (user->level>=WIZ) has_link=1;

        /* set error bit if there is no link so it stops here */
        if (!has_link) {
                sprintf(text,"multiMOVE:  Room #%d is not linked to room #%d.\n",pos-1,pos);
                write_user(user,text);
                err=pos; continue;
                }

        /* See if user has access to the new room */
        has_access=0;
        if (has_room_access(user,rm)) has_access=1;

        /* set error bit if access is denied */
        if (!has_access) {
                sprintf(text,"multiMOVE:  You do not have access to room #%d (%s).\n",pos,rm->name);
                write_user(user,text);
                err=pos; continue;
                }

/* ------  if we get this far, then we can safely let the user go
           to this room                                                 */

        /* If this room is an invited room, clear the invite. */
        if (user->invite_room==rm) user->invite_room=NULL;
        
        /* If the user is invis, do the invis stuff.  'nuff said. */
        if (!user->vis) {
                write_room(rm,invisenter);
                write_room_except(user->room,invisleave,user);
                }
        else {
                /* do the inphr/outphr stuff */
                sprintf(text,"%s%s %s.\n",colors[CWHOUSER],user->name,user->in_phrase);
                write_room(rm,text);
                if (final_rm!=rm) sprintf(text,"%s%s %s to the %s on %s way to the %s.\n",colors[CWHOUSER],user->name,user->out_phrase,rm->name,posgen[user->gender],final_rm->name);
                else sprintf(text,"%s%s %s to the %s.\n",colors[CWHOUSER],user->name,user->out_phrase,rm->name);
                write_room_except(user->room,text,user);
                }
        user->room=rm;
        }
if (first_err || err) {
        look(user);
        reset_access(old_room);
        write_user(user,"The multi-move did not complete entirely because...\n");
        if (first_err) {
                sprintf(text,"   ...your room choice #%d did not exist or was invalid.",first_err);
                write_user(user,text);
                }
        if (first_err && err) write_user(user,"..\n   ...and "); else write_user(user,"\n");
        if (!first_err && err) write_user(user,"   ...");
        if (err) {
                sprintf(text,"your room choice #%d was out of sequence or set to private.\n",err);
                write_user(user,text);
                }
        }
else { look(user); write_user(user,"Multi-Move completed successfully!\n"); reset_access(old_room); }
}

/*---------- TalkerOS:  Function for ignoring a specific user. ----------*/
ign_user(user)
UR_OBJECT user;
{
UR_OBJECT u;

if (word_count<2) {
        if (user->ignuser[0]!='\0') { sprintf(text,"\nYou are currently ignoring: %s\n",user->ignuser); write_user(user,text); }
        write_user(user,"Usage:  ignuser [off/<user>]\n"); return;
        }
strtolower(word[1]);
if (!strcmp(word[1],"off")) {
        if (user->ignuser[0]=='\0') write_user(user,"You were not ignoring anyone.\n");
        else { sprintf(text,"You are no longer ignoring %s.\n",user->ignuser); write_user(user,text); }
        user->ignuser[0]='\0';
        return;
        }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
if (u==user) { write_user(user,"You may not ignore yourself.\n"); return; }
if (u->level > XUSR && u->level > user->level) {
        write_user(user,"You may not ignore that user.\n");
        sprintf(text,"%s attempted to ignore you.\n",user->name);
        write_user(u,text);
        return;
        }
if (user->ignuser[0]!='\0') sprintf(text,"You are now ignoring %s instead of %s.\n",u->name,user->ignuser);
else sprintf(text,"You are now ignoring all posts from %s!\n",u->name);
write_user(user,text);
strcpy(user->ignuser,u->name);
}

reset_ignuser(user)
UR_OBJECT user;
{
UR_OBJECT u;
for (u=user_first;u!=NULL;u=u->next) {
        if (!strcmp(u->ignuser,user->name)) {
                u->ignuser[0]='\0';
                write_user(u,"You are no longer ignoring anyone.\n");
                }
        }
}

/* ------------------- TalkerOS User Rename Utility ------------------- */

/*  This will rename a user to a new name.  Isn't that nice?  The user
    must be logged in at the time, because if you changed it while they
    were not logged in, how would they know that you changed it?!  This
    simplifies having to go into the shell to rename all the files, and
    it also lets you give your high-ranking wizards the ability to do
    this without them having to bug you, or having to give them shell
    access. */

user_rename_files(user,str)
UR_OBJECT user;
char *str;
{
UR_OBJECT u,u2;
char oldfile[81],newfile[81];
int i,len;

if (word_count<3) { write_user(user,"Usage:  .urename <user> <newName>\n"); return; }
/* make sure we aren't renaming someone to a swear word... of course
   if the swear ban is off, this won't matter. */
if (contains_swearing(str)) {
        write_user(user,"Swear words can not be used in the user name.\n");
        return;
        }
/* check to see if the user is logged in or not (see above) */
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
/* user is logged in */
else {
        /* make sure the user isn't temp-promoted, and that he/she isn't
           a lower rank than the person he/she is changing. */
        if (u->level >= user->orig_level || user->level > user->orig_level) {
                write_user(user,"File access denied.\n");
                sprintf(text,"%s[ %s attempted to RENAME user: %s ]\n",colors[CSYSTEM],user->name,u->name);
                write_duty(user->level,text,NULL,user,0);
                return;
                }                
        /* make sure the user to be renamed is renameable  :)  */
        if (u->type!=USER_TYPE) { write_user(user,"ERROR:  User is not the correct type and cannot be renamed.\n"); return; }
        /* make new user struct */
        if ((u2=create_user())==NULL) {
                write_user(user,"ERROR:  Could not create new user object in user_rename_files()!\n");
                write_syslog("ERROR:  Couldn't create new user object in user_rename_files().\n",0,SYSLOG);
                return;
                }
        /* check validity of new name and do CAPS stuff */
        for (i=0;i<strlen(word[2]);++i) {
                if (!isalpha(word[2][i])) {
                        write_user(user,"\n-=- Only letters are allowed in a username. -=-\n\n");
                        destruct_user(u2); destructed=0;
                        return;
                        }
                if (!i) word[2][0] = toupper(word[2][0]);
                else if (!allow_caps_in_name) word[2][i] = tolower(word[2][i]);
                }
        /* check to see if user exists with new name */
        strcpy(u2->name,word[2]);
        if (load_user_details(u2)) {
                write_user(user,"ERROR:  Cannot rename, user exists with new name.\n");
                destruct_user(u2); destructed=0;
                return;
                }
        /* Do file stuff */
        sprintf(oldfile,"%s/%s.D",USERFILES,u->name);
        sprintf(newfile,"%s/%s.D",USERFILES,u2->name);
        rename(oldfile,newfile);
        unlink(oldfile);
        sprintf(oldfile,"%s/%s.P",USERFILES,u->name);
        sprintf(newfile,"%s/%s.P",USERFILES,u2->name);
        rename(oldfile,newfile);
        unlink(oldfile);
        sprintf(oldfile,"%s/%s.M",USERFILES,u->name);
        sprintf(newfile,"%s/%s.M",USERFILES,u2->name);
        rename(oldfile,newfile);
        unlink(oldfile);
        sprintf(oldfile,"%s/%s.R",USERFILES,u->name);
        sprintf(newfile,"%s/%s.R",USERFILES,u2->name);
        rename(oldfile,newfile);
        unlink(oldfile);
        save_user_details(u2,1);
        /* End file stuff */

        /* Do announcements */
        sprintf(text,"~OLSYSTEM:~RS  %s has been renamed to: %s!\n",u->name,u2->name);
        write_room_except(NULL,text,u);
        sprintf(text,"%s[ %s RENAMED user '%s' to '%s' ]\n",colors[CSYSTEM],user->name,u->name,u2->name);
        write_duty(user->level,text,NULL,user,0);
        sprintf(text,"%s RENAMED user '%s' to '%s'\n",user->name,u->name,u2->name);
        write_syslog(text,1,USERLOG);
        sprintf(text,"\n~FR~OLATTENTION:  You have been renamed to: %s\n            This is now your NEW username.  The password remains unchanged.\n\n",u2->name);
        write_user(u,text);

        /* Change the actual user name */
        strcpy(u->name,u2->name);

        /* clean up */
        destruct_user(u2);  destructed=0;

        return;
        }

/* All that just to rename a user... I would have to rename 60 people in
the shell to equal that much typing.  Oh well, i guess its convenient for
you. */
}

/** ----------------------------------------------------------------------
     TalkerOS  AUDIO PROMPTER for Pueblo
    ---------------------------------------------------------------------- **/
audioprompt(user,prmpt,pager)
UR_OBJECT user;
int prmpt;
int pager;
{
UR_OBJECT u;
char audiofiles[8][30]={
        "ap_f-welcome.wav",     /* 00: Login greeting        (female)  */
        "ap_m-welcome.wav",     /* 01: Login greeting        (male)    */
        "ap_f-pager.wav",       /* 02: Pager sound           (female)  */
        "ap_m-pager.wav",       /* 03: Pager sound           (male)    */
        "ap_f-warning.wav",     /* 04: Warning sound         (female)  */
        "ap_m-warning.wav",     /* 05: Warning sound         (male)    */
        "ap_f-shutdown.wav",    /* 06: Shutdown/Reboot alert (female)  */
        "ap_m-shutdown.wav"     /* 07: Shutdown/Reboot alert (male)    */
        };
if (user!=NULL) {
        /* Check user prefs. */
        if (!user->pueblo) return;
        if (!pager && !user->pueblo_mm) return 0;
        if (pager  && !user->pueblo_pg) return 0;

        /* Set to male voice if that is the user's preference. */
        if (user->voiceprompt) prmpt++;
        
        /* Send playback command */
        sprintf(text,"</xch_mudtext><img xch_sound=play xch_device=wav href=\"%s%s%s\"><xch_mudtext>",reg_sysinfo[TALKERHTTP],reg_sysinfo[PUEBLOWEB],audiofiles[prmpt]);
        write_user(user,text);
        return 1;
        }
/* If we get here then the announcement is to everyone. */
for (u=user_first; u!=NULL; u=u->next) {
        /* Check user prefs. */
        if (!u->pueblo || !u->pueblo_mm) continue;

        /* Send playback command */
        sprintf(text,"</xch_mudtext><img xch_sound=play xch_device=wav href=\"%s%s%s\"><xch_mudtext>",reg_sysinfo[TALKERHTTP],reg_sysinfo[PUEBLOWEB],audiofiles[prmpt+u->voiceprompt]);
        write_user(u,text);
        }
}

wizremove(user)
UR_OBJECT user;
{
UR_OBJECT u;
int temp,idle,mins;

if (word_count<2) { write_user(user,"Remove who?\n"); return; }
if (!(u=get_user(word[1]))) {
        write_user(user,notloggedon);
        return;
        }
if (u==user) { write_user(user,"Use .quit to disconnect.\n"); return; }
if (u->level < WIZ) { write_user(user,"This command will remove idle administrators only.\n"); return; }
if (u->level > user->level) { write_user(user,"You must be at or above the user's level to WizRemove them.\n");
                              sprintf(text,"(%s attempted to remove you!)\n",user->name);
                              write_user(u,text);
                              return; }
mins=(int)(time(0) - u->last_login)/60;
temp=(int)(time(0) - u->last_input)/60;
idle = u->idle + temp;
if ((int)(time(0) - u->last_input) < user_idle_time) {
        sprintf(text,"That person has not idled long enough for you to remove %s.\n",objgen[u->gender]);
        write_user(user,text);
        sprintf(text,"(%s attempted to remove you!)\n",user->name);
        write_user(u,text);
        return;
        }
/* If we get this far then it is okay to remove the user. */
sprintf(text,"%s REMOVED %s after %s idled %d mins. (%d total of %d minutes.)\n",user->name,u->name,heshe[u->gender],temp,idle,mins);
write_syslog(text,1,USERLOG);
sprintf(text,"%s[ %s WizREMOVED %s. ]\n",colors[CSYSTEM],user->name,u->name);
write_duty(u->level,text,NULL,u,0);
disconnect_user(u);  destructed=0;
write_user(user,"Okay.\n");
}

disp_memory_summary(user)
UR_OBJECT user;
{
UR_OBJECT u;
RM_OBJECT rm;
NL_OBJECT nl;
PL_OBJECT pl;
CM_OBJECT plcmd;
char datasize[4][6]={"Bytes","KB","MB","GB"};
char dataname[5][16]={"User Objects","Room Objects","NetLink Objects","Plugin Modules","Plugin Commands"};
int i,val,digit,scale,cr;
int size,total,system,object,usr,room,netlink,plugin,cmds;
usr=0; room=0; netlink=0; plugin=0; cmds=0; total=0; object=0;

/* get object sizes */
size=sizeof(struct user_struct);
for(u=user_first;u!=NULL;u=u->next) { usr+=size;  object++; }
size=sizeof(struct room_struct);
for(rm=room_first;rm!=NULL;rm=rm->next) { room+=size;  object++; }
size=sizeof(struct netlink_struct);
for(nl=nl_first;nl!=NULL;nl=nl->next) { netlink+=size;  object++; }
size=sizeof(struct plugin_struct);
for(pl=plugin_first; pl!=NULL; pl=pl->next) { plugin+=size;  object++; }
size=sizeof(struct plugin_cmd);
for(plcmd=cmds_first; plcmd!=NULL; plcmd=plcmd->next) { cmds+=size;  object++; }

write_user(user,"____________________ TalkerOS Memory Usage ____________________\n\n");

for (i=0; i<5; i++) {
        /* transfer memory value */
        if (i==0) {val=usr; cr=0;}
        if (i==1) {val=room; cr=1;}
        if (i==2) {val=netlink; cr=0;}
        if (i==3) {val=plugin; cr=1;}
        if (i==4) {val=cmds; cr=1;}
        
        /* set data scale -- bytes, kb, mb, gb */
        scale=0;
        if (val>=1024) scale=1;
        if (val>=1048576) scale=2;
        if (val>=1073741824) scale=3;

        /* interpret scale */
        digit=val;
        if (scale==3) digit=(int)(val/1073741824);
        if (scale==2) digit=(int)(val/1048576);
        if (scale==1) digit=(int)(val/1024);
                                
        /* Write data to screen */
        sprintf(text,"%-15s: %4d %-5s      ",dataname[i],digit,datasize[scale]);
        write_user(user,text);
        if (cr) write_user(user,"\n");
        }
total=(usr+room+netlink+plugin+cmds);
sprintf(text,"\n%d bytes of memory used for %d objects.\n\n",total,object);
write_user(user,text);
}
