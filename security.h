/*========================================================================
========  TalkerOS ver4.03 - Internal Proprietary Firewall Info.  ========
========================================================================*/

/* NOTE:  Changing any information in this file will require you to
          recompile and reboot your talker before the changes will
          take effect.  */

int check_at_lev =   4;   /* What level to begin requiring firewall? */
int revert_level =   0;   /* Revert level if name match fails. */
int revert_site =    2;   /* Revert level if site match fails. */
int allow_local =    1;   /* Allow logins from localhost? 1=YES 0=NO */
int allow_altsite =  1;   /* Allow two sites per person?  1=YES 0=NO */
int allow_namefail = 1;   /* 1=Allow if name check fails. 0=Revert on fail. */
int allow_sitefail = 1;   /* 1=Allow if site check fails. 0=Revert on fail. */
int namefail_temp =  0;   /* 1=Temporarily demote if fail.  0=Permanent. */
int sitefail_temp =  1;   /* 1=Temporarily demote if fail.  0=Permanent. */
int namefail_kill =  1;   /* 1=Kill user if name fails.     0=Don't kill. */
int sitefail_kill =  0;   /* 1=Kill user if site fails.     0=Don't kill. */

int allow_wiz_on_main = 1;  /* 1=Allow wizards to login on Main Port
                               0=Wizards can only login on Wiz Port  */

/*  NOTE: If allow_namefail and allow_sitefail are both set to 1, then
          the firewall will be useless as it will allow you to not match
          information.  For best results, set name_fail to 0 and site_fail
          to 0.  This will kill users who attempt to hack their own username
          into becoming a wiz, and allow wizards not matching the site to
          still log-in, but with a reduced level.
*/

int total_fwusers=2;  // <---- Make sure this is set properly!!!
char *fire_name[]={
"Admin", "Second-User"
};

char *fire_main[]={
".host.dns", "207.0.23.0"
};

char *fire_alt[]={
"localhost", "(none)"
};

