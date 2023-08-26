// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

/**
 * Custom Atcommands
 * Place the body of custom atcommands in this file.
 * Format:
 *	ACMD_FUNC(command_name)
 *	{
 *		<code>;
 *	}
 **/

//ACMD_FUNC(newcommand)
//{
//	clif_displaymessage(fd, "It works!");
//	clif_specialeffect(&sd->bl, EF_HEARTCASTING, AREA);
//	return 0;
//}




// Battleground Extended by Vykimo
ACMD_FUNC(bgstart) {
	
	nullpo_retr(-1, sd);
	
	if (bg_flag) {
		clif_displaymessage(fd, "Battleground is currently in progress.");
		return -1;
	}

	bg_flag = true;
	bg_start();
	clif_displaymessage(fd, "Battleground has been initiated");

	return 0;
}

ACMD_FUNC(bgend) {
	
	nullpo_retr(-1, sd);
	
	if (!bg_flag) {
		clif_displaymessage(fd, "Battleground is currently not in progress.");
		return -1;
	}

	bg_flag = false;
	bg_end();
	clif_displaymessage(fd, "Battleground has been ended.");

	return 0;
}

ACMD_FUNC(listenbg)
{
	sd->state.bg_listen = (sd->state.bg_listen == 0);
	
	if( sd->state.bg_listen )
		clif_displaymessage(fd, "You will receive Battleground announcements.");
	else
		clif_displaymessage(fd, "You will not receive Battleground announcements.");

	return 0;
}

ACMD_FUNC(order)
{
	nullpo_retr(-1,sd);
	if( !message || !*message )
	{
		clif_displaymessage(fd, "Please, enter a message (usage: @order <message>).");
		return -1;
	}

	if( map_getmapflag(sd->bl.m, MF_BATTLEGROUND) )
	{
		if( !sd->bmaster_flag )
		{
			clif_displaymessage(fd, "This command is reserved for Team Leaders Only.");
			return -1;
		}
		clif_broadcast2(&sd->bl, message, (int)strlen(message)+1, sd->bmaster_flag->color, 0x190, 20, 0, 0, BG);
	}
	else
	{
		if( !sd->state.gmaster_flag )
		{
			clif_displaymessage(fd, "This command is reserved for Guild Leaders Only.");
			return -1;
		}
		clif_broadcast2(&sd->bl, message, (int)strlen(message)+1, 0xFF0000, 0x190, 20, 0, 0, GUILD);
	}

	return 0;
}
ACMD_FUNC(leader)
{
	struct map_session_data *pl_sd;
	nullpo_retr(-1,sd);
	if( !sd->bmaster_flag )
		clif_displaymessage(fd, "This command is reserved for Team Leaders Only.");
	else if( sd->ud.skilltimer != INVALID_TIMER )
		clif_displaymessage(fd, "Command not allow while casting a skill.");
	else if( !message || !*message )
		clif_displaymessage(fd, "Please, enter the new Leader name (usage: @leader <name>).");
	else if( (pl_sd = map_nick2sd((char *)message,false)) == NULL )
		clif_displaymessage(fd, msg_txt(sd,3)); // Character not found.
	else if( sd->bg_id != pl_sd->bg_id )
		clif_displaymessage(fd, "Destination Player is not in your Team.");
	else if( sd == pl_sd )
		clif_displaymessage(fd, "You are already the Team Leader.");
	else
	{ // Everytest OK!
		sprintf(atcmd_output, "Team Leader transfered to [%s]", pl_sd->status.name);
		clif_broadcast2(&sd->bl, atcmd_output, (int)strlen(atcmd_output)+1, sd->bmaster_flag->color, 0x190, 20, 0, 0, BG);

		sd->bmaster_flag->leader_char_id = pl_sd->status.char_id;
		pl_sd->bmaster_flag = sd->bmaster_flag;
		sd->bmaster_flag = NULL;

		clif_name_area(&sd->bl);
		clif_name_area(&pl_sd->bl);
		return 0;
	}
	return -1;
}

ACMD_FUNC(reportafk)
{
	struct map_session_data *pl_sd;
	nullpo_retr(-1,sd);
	if( !sd->bg_id )
		clif_displaymessage(fd, "This command is reserved for Battleground Only.");
	else if( !sd->bmaster_flag && battle_config.bg_reportafk_leaderonly )
		clif_displaymessage(fd, "This command is reserved for Team Leaders Only.");
	else if( !message || !*message )
		clif_displaymessage(fd, "Please, enter the character name (usage: @reportafk/votekick <name>).");
	else if( (pl_sd = map_nick2sd((char *)message,false)) == NULL )
		clif_displaymessage(fd, msg_txt(sd,3)); // Character not found.
	else if( sd->bg_id != pl_sd->bg_id )
		clif_displaymessage(fd, "Destination Player is not in your Team.");
	else if( sd == pl_sd )
		clif_displaymessage(fd, "You cannot kick yourself.");
	else if( pl_sd->state.bg_afk == 0 )
		clif_displaymessage(fd, "The player is not AFK on this Battleground.");
	else
	{ // Everytest OK!
		struct battleground_data *bg;
		if( (bg = bg_team_search(sd->bg_id)) == NULL )
			return -1;

		bg_team_leave(pl_sd,2);
		clif_displaymessage(pl_sd->fd, "You have been kicked from Battleground because of your AFK status.");
		pc_setpos(pl_sd,pl_sd->status.save_point.map,pl_sd->status.save_point.x,pl_sd->status.save_point.y,CLR_OUTSIGHT);
		clif_refresh(pl_sd);

		sprintf(atcmd_output, "- AFK [%s] Kicked -", pl_sd->status.name);
		clif_broadcast2(&sd->bl, atcmd_output, (int)strlen(atcmd_output)+1, bg->color, 0x190, 20, 0, 0, BG);
		return 0;
	}
	return -1;
}
/*==========================================
 * Guild Skill Usage for Guild Masters
 *------------------------------------------*/
ACMD_FUNC(guildskill)
{
	int i, skillnum = 0, skilllv = 0;
	t_tick tick = gettick();

	const struct { char skillstr[3]; int id; } skills[] = {
		{ "BO",	10010 },
		{ "RG",	10011 },
		{ "RS",	10012 },
		{ "EC",	10013 },
	};

	// Check for Skill ID
	for( i = 0; i < ARRAYLENGTH(skills); i++ )
	{
		if( strncmpi(message, skills[i].skillstr, 3) == 0 )
		{
			skillnum = skills[i].id;
			break;
		}
	}
	if( !skillnum )
	{
		clif_displaymessage(fd, "Invalid Skill string. Use @guildskill EC/RS/RG/BO");
		return -1;
	}

	if( !map_getmapflag(sd->bl.m, MF_BATTLEGROUND) ) {
		if( sd->state.gmaster_flag )
			skilllv = guild_checkskill(sd->guild, skillnum);
		else {
			clif_displaymessage(fd, "This command is reserved for Guild Leaders Only.");
			return -1;
		}
	} else {
		struct battleground_data *bg;
		if( (bg = sd->bmaster_flag) != NULL )
			skilllv = bg_checkskill(bg, skillnum);
		else {
			clif_displaymessage(fd, "This command is reserved for Team Leaders Only.");
			return -1;
		}
	}

	if( pc_cant_act(sd) || pc_issit(sd) || skill_isNotOk(skillnum, sd) || sd->ud.skilltimer != -1 || sd->sc.option&(OPTION_WEDDING|OPTION_XMAS|OPTION_SUMMER) || sd->state.only_walk || sd->sc.data[SC_BASILICA] )
		return -1;

	if( DIFF_TICK(tick, sd->ud.canact_tick) < 0 )
		return -1;

	if( sd->menuskill_id )
	{
		if( sd->menuskill_id == SA_TAMINGMONSTER )
			sd->menuskill_id = sd->menuskill_val = 0; //Cancel pet capture.
		else if( sd->menuskill_id != SA_AUTOSPELL )
			return -1; //Can't use skills while a menu is open.
	}

	sd->skillitem = sd->skillitemlv = 0;
	if( skilllv ) unit_skilluse_id(&sd->bl, sd->bl.id, skillnum, skilllv);
	return 0;
}

ACMD_FUNC(partybuff)
{ // [Vykimo]
	struct party_data* p = NULL;
	nullpo_retr(-1, sd);

	if( !sd->status.party_id ) {
		clif_displaymessage(fd, msg_txt(sd,1071)); // You're not in a party.
		return -1;
	}

	p = party_search(sd->status.party_id);

	if( sd->state.spb ) {
		sd->state.spb = 0;
		clif_displaymessage(fd, msg_txt(sd,1072)); // Displaying party member's buffs disabled.
	} else {
		sd->state.spb = 1;
		clif_displaymessage(fd, msg_txt(sd,1073)); // Displaying party member's buffs enabled.
	}

	clif_party_info(p,sd);
	return 0;
}

// [Vykimo] No recall command
ACMD_FUNC(norecall)
{
	nullpo_retr(-1, sd);
	
	sd->state.norecall = (sd->state.norecall==0);
	if(sd->state.norecall)	clif_displaymessage(fd, "@norecall : You'll be not recalled when Emergency Call skill will be casted.");
	else clif_displaymessage(fd, "@norecall : You'll be now recalled when Emergency Call skill will be casted.");
	
	return 0;
}

/*==========================================
 * Hate (SG designated monsters) Reset
 *------------------------------------------*/
ACMD_FUNC(hatereset)
{
	if ((sd->class_&MAPID_UPPERMASK) != MAPID_STAR_GLADIATOR) {
		clif_displaymessage(sd->fd,msg_txt(sd,35));	// You can't use this command with this class.
		return -1;
	}

	pc_resethate(sd);
	clif_displaymessage(fd, msg_txt(sd,1515)); // Reset 'Hatred' monsters.

	return 0;
}
/*
ACMD_FUNC(battleinfo)
{
	if( sd->state.battleinfo )
	{
		clif_displaymessage(fd, "- Battle Information Display OFF - Kill/Death -");
		sd->state.battleinfo = 0;
	}
	else
	{
		clif_displaymessage(fd, "- Battle Information Display ON - Kill/Death -");
		sd->state.battleinfo = 1;
	}
	return 0;
}
*/

/*==========================================
 * Packet Filter
 *------------------------------------------*/
ACMD_FUNC(packetfilter)
{
	nullpo_retr(-1,sd);
	if( !message || !*message )
	{
		clif_displaymessage(fd,"<<----- Packet Filtering Usage ----->>");
		clif_displaymessage(fd,".   @packetfilter <options>");
		clif_displaymessage(fd,".   C : To filter global chat messages.");
		clif_displaymessage(fd,".   I : To filter item usage.");
		clif_displaymessage(fd,".   S : To filter soul linker skills.");
		clif_displaymessage(fd,".   - Samples");
		clif_displaymessage(fd,".   @packetfilter CI : To filter the 2 options.");
		clif_displaymessage(fd,".   @packetfilter off : To turn packet filter off.");
	}
	else if( !strcmpi(message,"off") )
	{
		sd->state.packet_filter = 0;
		clif_displaymessage(fd,"<< Packet Filtering Off >>");
	}
	else
	{
		if( strstr(message,"C") )
			sd->state.packet_filter |= 1;
		if( strstr(message,"I") )
			sd->state.packet_filter |= 2;
		if( strstr(message,"S") )
			sd->state.packet_filter |= 8;
		clif_refresh(sd);
		sprintf(atcmd_output,"<< Packet Filtering | Chat %s | Items %s | Soul linker skills %s >>", (sd->state.packet_filter&1) ? "ON" : "OFF", (sd->state.packet_filter&2) ? "ON" : "OFF", (sd->state.packet_filter&8) ? "ON" : "OFF");
		clif_displaymessage(fd,atcmd_output);
	}

	return 0;
}




/*==========================================
 * disable viewdata's
 *------------------------------------------*/
ACMD_FUNC(hideview)
{
	int val = 0, view = 0;
	char type[CHAT_SIZE_MAX];
	
	nullpo_retr(-1, sd);
	memset(atcmd_output, '\0', sizeof(atcmd_output));
	
	parent_cmd = atcommand_checkalias(command+1);
	
	if( strcmp(parent_cmd, "hideview") == 0 ){
		if((sscanf(message, "%s %11d", &type, &view)) < 1 || (view > 15 || view < 1)){
			clif_displaymessage(fd, "Usage: @hideview {<type>} {<position>}");
			clif_displaymessage(fd, "type: all - all players {<position>} view | other - other players {<position>} view");
			clif_displaymessage(fd, "Position: 1 - top | 2 - mid | 4 - bottom | 8 - robe");
			clif_displaymessage(fd, "Sample: \"@hideview other\" 7 ( 1 + 2 + 4 = 7 will hide other players top mid bot equipments)");
			return -1;
		}
		if((strncmp(type, "all", 3) == 0))
			val += 16;

		if(view&1) val += 1;
		if(view&2) val += 2;
		if(view&4) val += 4;
		if(view&8) val += 8;
		
		sd->state.hideview = val;
		clif_refresh(sd);
		if((strncmp(message, "others", 3) == 0) || (val&16)){
			clif_refreshlook(&sd->bl,sd->bl.id,LOOK_HEAD_TOP,sd->status.head_top,SELF);
			clif_refreshlook(&sd->bl,sd->bl.id,LOOK_HEAD_MID,sd->status.head_mid,SELF);
			clif_refreshlook(&sd->bl,sd->bl.id,LOOK_HEAD_BOTTOM,sd->status.head_bottom,SELF);
			clif_refreshlook(&sd->bl,sd->bl.id,LOOK_ROBE,sd->status.robe,SELF);
		}
		if(val&16){
			if(val&1) clif_refreshlook(&sd->bl,sd->bl.id,LOOK_HEAD_TOP,0,SELF);
			if(val&2) clif_refreshlook(&sd->bl,sd->bl.id,LOOK_HEAD_MID,0,SELF);
			if(val&4) clif_refreshlook(&sd->bl,sd->bl.id,LOOK_HEAD_BOTTOM,0,SELF);
			if(val&8) clif_refreshlook(&sd->bl,sd->bl.id,LOOK_ROBE,0,SELF);
		}
		sprintf(atcmd_output, "%s players equip view are now hidden",(strncmp(type, "others", 3)) ? "Other":"All");
		clif_displaymessage(fd, atcmd_output);
		clif_displaymessage(fd, "Use @showview to show all players equip view");
	} else if(strcmp(parent_cmd, "showview") == 0){
		sd->state.hideview = 0;
		clif_displaymessage(fd, "All players view are now shown.)");
		clif_refresh(sd);
		clif_refreshlook(&sd->bl,sd->bl.id,LOOK_HEAD_TOP,sd->status.head_top,SELF);
		clif_refreshlook(&sd->bl,sd->bl.id,LOOK_HEAD_MID,sd->status.head_mid,SELF);
		clif_refreshlook(&sd->bl,sd->bl.id,LOOK_HEAD_BOTTOM,sd->status.head_bottom,SELF);
		clif_refreshlook(&sd->bl,sd->bl.id,LOOK_ROBE,sd->status.robe,SELF);
	}
	
	return 0;
}
