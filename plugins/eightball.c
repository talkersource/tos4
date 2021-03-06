/* The TalkerOS Eightball Plugin 
   -------------------------------------------------------------
   The initialization code for this plugin is:

      if (tmp=plugin_00x000_init(cmd)) cmd=cmd+tmp;

   The command call for this plugin is:

      if (!strcmp(plugin->registration,"01-000")) { plugin_01x000_main(user,str,comnum); return 1; }

   No further modifications are necessary.
   ------------------------------------------------------------- */

   extern UR_OBJECT get_user(char[]);
   extern CM_OBJECT create_cmd();
   extern PL_OBJECT create_plugin();

/* ------------------------------------------------------------- */

plugin_00x000_init(cm)
int cm;
{
PL_OBJECT plugin;
CM_OBJECT com;
int i;
i=0;
/* create plugin */
if ((plugin=create_plugin())==NULL) {
        write_syslog("ERROR: Unable to create new registry entry!\n",0,SYSLOG);
        return 0;
	}
strcpy(plugin->name,"TalkerMagicEightBall");    /* Plugin Description   */
strcpy(plugin->author,"William Price");         /* Author's name        */
strcpy(plugin->registration,"00-000");          /* Plugin/Author ID     */
strcpy(plugin->ver,"1.0");                      /* Plugin version       */
strcpy(plugin->req_ver,"400");                  /* Runtime ver required */
plugin->id = cm;                                /* ID used as reference */
plugin->req_userfile = 0;                       /* Requires user data?  */
plugin->triggerable = 0;                        /* Can be triggered by
                                                   regular speech? */

/* create associated command */
if ((com=create_cmd())==NULL) {
        write_syslog("ERROR: Unable to add command to registry!\n",0,SYSLOG);
        return 0;
	}
i++;                                            /* Keep track of number created */
strcpy(com->command,"8ball");                   /* Name of command */
com->id = plugin->id;                           /* Command reference ID */
com->req_lev = XUSR;                            /* Required level for cmd. */
com->comnum = i;                                /* Per-plugin command ID */
com->plugin = plugin;                           /* Link to parent plugin */
/* end creating command - repeat as needed for more commands */
return i;
}

plugin_00x000_main(user,str,comid)
UR_OBJECT user;
char *str;
int comid;
{
        /* put case calls here for each command needed.
           remember that the commands will be in order, with the first
           command always being 1
        */
switch (comid) {
        case  1: plugin_00x000_respond_eightball(user,str); return 1;
        case -5: /* Trigger Heartbeat */
        case -6: /* Talker shutdown */
        case -7: /* 1st to save */
        case -8: /* save normal */
        case -9: /* load user data */
        default: return 0;
        }
}

plugin_00x000_respond_eightball(user,str)
UR_OBJECT user;
char *str;
{
char filename[80],line[150],*name,text2[180];
int i,num,total;
FILE *fp;

i=0;  num=0;  total=0;
filename[0]='\0';  line[0]='\0';  text2[0]='\0';

if (word_count<2) { write_user(user,"The Magic EightBall will only respond to a question.\n"); return; }
if (user->vis) name=user->name;  else name=invisname;

        sprintf(filename,"%s/eightball.8",PLUGINFILES);
        if (!(fp=fopen(filename,"r"))) {
                write_user(user,"EightBall:  Sorry!  Response file was not found.\n");
                return;
                }
        fscanf(fp,"%d\n",&total); /* total file entries */
        num = rand() % total;
        num++;  /* Have to increase because the rand() might leave it 0. */

        /* get the line from the file that contains the random response */
        for(i=0; (i<num && !feof(fp)); i++) fgets(line,161,fp);
        fclose(fp);
        line[strlen(line)-1]='\0';
        terminate(line);

        /* write question to users */
        sprintf(text,"%s[You ask the EightBall]%s: %s\n",colors[CSELF],colors[CTEXT],str);
        write_user(user,text);        
        sprintf(text,"%s[%s asks the EightBall]%s: %s\n",colors[CUSER],name,colors[CTEXT],str);
        write_room_except(user->room,text,user);
        record(user->room,text);

        /* write response to room */
        sprintf(text,"~FG[The Magic EightBall]%s: %s\n",colors[CTEXT],line);
        write_room(user->room,text);
        record(user->room,text);
}
