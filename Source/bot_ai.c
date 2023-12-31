/*****************************************************************

	Eraser Bot source code - by Ryan Feltrin, Added to by Acrid-

	..............................................................

	This file is Copyright(c) 1998, Ryan Feltrin, All Rights Reserved.

	..............................................................

	All other files are Copyright(c) Id Software, Inc.

	Please see liscense.txt in the source directory for the copyright
	information regarding those files belonging to Id Software, Inc.

	..............................................................
	
	Should you decide to release a modified version of Eraser, you MUST
	include the following text (minus the BEGIN and END lines) in the 
	documentation for your modification.

	--- BEGIN ---

	The Eraser Bot is a product of Ryan Feltrin, and is available from
	the Eraser Bot homepage, at http://impact.frag.com.

	This program is a modification of the Eraser Bot, and is therefore
	in NO WAY supported by Ryan Feltrin.

	This program MUST NOT be sold in ANY form. If you have paid for 
	this product, you should contact Ryan Feltrin immediately, via
	the Eraser Bot homepage.

	--- END ---

	..............................................................

	You will find p_trail.c has not been included with the Eraser
	source code release. This is NOT an error. I am unable to 
	distribute this file because it contains code that is bound by
	legal documents, and signed by myself, never to be released
	to the public. Sorry guys, but law is law.

	I have therefore include the compiled version of these files
	in .obj form in the src\Release and src\Debug directories.
	So while you cannot edit and debug code within these files,
	you can still compile this source as-is. Although these will only
	work in MSVC v5.0, linux versions can be made available upon
	request.

	NOTE: When compiling this source, you will get a warning
	message from the compiler, regarding the missing p_trail.c
	file. Just ignore it, it will still compile fine.

	..............................................................

	I, Ryan Feltrin/Acrid-, hold no responsibility for any harm caused by the
	use of this source code. I also am NOT willing to provide any form
	of help or support for this source code. It is provided as-is,
	as a service by me, with no documentation, other then the comments
	contained within the code. If you have any queries, I suggest you
	visit the "official" Eraser source web-board, at
	http://www.telefragged.com/epidemic/. I will stop by there from
	time to time, to answer questions and help with any problems that
	may arise.

	Otherwise, have fun, and I look forward to seeing what can be done
	with this.

	-Ryan Feltrin
	-Acrid-

 *****************************************************************/
// bot_ai.c
#include "g_local.h"
#include "m_player.h"
#include "g_items.h"

#include "p_trail.h"
  
#include "bot_procs.h"

mmove_t bot_move_attack;

float	last_roam_time;

void CTFDropFlagTouch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);
void CTFFlagThink(edict_t *ent);
void CTFGrappleFire (edict_t *ent, vec3_t g_offset, int damage, int effect);

/*
=======================
bot_roam

  This is the main searching routine, whichs looks around for
  enemies or items to head for.

  set "no_paths" to disable all path checking (faster)
=======================
*/
void bot_roam (edict_t *self, int no_paths)
{
	edict_t		*search, *closest, /**enemy,*/ *closest_nonvisible=NULL, *save_goal=NULL, *save_movetarget=NULL;
	float		closest_dist, this_dist, nonvis_dist=999999;
	int			player_index = 0;
	float		save_suicide_time;
	int			i;
	int n, carrying_flag;
	edict_t *flag, *enemy_flag;
	int			flaggoal=false;

//	vec3_t		vec;
//testing
//	edict_t *blip;
//	float dist;
//	vec3_t v;
//	trace_t		tr;
//	blip = NULL;
//testing

	if (no_paths)
	{
		if (self->last_nopaths_roam > (level.time - (0.05*num_players + 0.1)))
			return;
		self->last_nopaths_roam = level.time;
	}
	else
	{
		if (self->last_roam_time > (level.time - (0.05*num_players*3 + 1)))
			return;
	}

	save_suicide_time = self->bored_suicide_time;
	self->bored_suicide_time = -1;		// restored only if not found anything

	// look for a visible enemy
	closest = closest_nonvisible = NULL;
	closest_dist = 800;

	search = players[player_index];

// enable the following line to disable bots attacking others
//if (false)
	if (no_paths && (!ctf->value || !self->enemy || !CarryingFlag(self->enemy)))
	{	// don't attack someone else if current enemy has our flag!
		carrying_flag = false;

		while (player_index < num_players)
		{
			if (	!(search->flags & FL_NOTARGET) && 
(search != self) && (self->enemy != search) && (!search->disguised)&&
(search->health > 0) && (search->bot_client || (search->light_level > 5)) 
&& search->solid && !SameTeam(self, search)
/*|| SameTeam(self, search) && (search->disease)&&
(self->client->player_class == 2)&&
(self->client->pers.inventory[ITEM_INDEX(item_shells)] != 0))*///Acrid Nurse
					// we can see this person, is it worth attacking them?
				&&	(!(search->client->invincible_framenum > level.framenum)) 

				&&	(	((self->client->quad_framenum > level.framenum) 
				|| self->client->invincible_framenum > level.framenum)
					 || (ctf->value && (carrying_flag = CarryingFlag(search)))
					 || !self->enemy 
					 || (self->enemy->health > search->health) 
					 || (search->client->pers.weapon == item_blaster)))	// if Quad, or enemy is weaker than self
			{
				if (((carrying_flag) || ((this_dist = entdist(self, search)) < closest_dist)) 
                && visible(self, search) && CanSee(self, search))
				{	// go for em!//fixme testing
					botDebugPrint("Attacking %s\n",search->classname);// search->client->pers.netname);

					self->enemy = search;
					closest_dist = this_dist;
       //I think don't go for any other goal but enemy
if (	(carrying_flag) ||	(self->movetarget && 
	   ((entdist(self, self->movetarget) > 256) && 
	   ((search->health < 15) || 
		(search->client->pers.weapon == item_blaster)) && 
		((self->bot_fire != botBlaster) && 
		(self->bot_fire != botShotgun))) ||	
		((self->bot_stats->aggr/5)*0.2 > random())))
					{
						botDebugPrint(" - ABORTING MOVETARGET !!");
						self->movetarget = NULL;

						if (carrying_flag)	// go for flag carrier!
							break;
					}

					botDebugPrint("\n");
				}
				else if (this_dist < nonvis_dist)
				{
					closest_nonvisible = search;
					nonvis_dist = this_dist;
				}

			}

			search = players[++player_index];
		}
	}
/////////////////////////////////////testing
/*		if(!self->enemy)
		{

			//Reduce range 
			while (blip = findradius (blip, self->s.origin, 550))
			{

				if (blip->solid == SOLID_NOT)
					continue;	//don't see observers

				if (blip == self)
					continue;
				if (blip->wf_team == self->wf_team)
					continue;
				if (!blip->takedamage)
					continue;
				if (blip->disguised)
					continue;

/*				tr = gi.trace (self->s.origin, NULL, NULL, blip->s.origin, self, MASK_SOLID);
				if (tr.fraction != 1.0)
					continue;
				VectorSubtract (self->s.origin, blip->s.origin, v);
			
				dist = VectorLength(v);

				if (!visible(self, blip) && dist > 300)
					continue;*/
 /*               botDebugPrint("Attacking %s\n",blip->classname);			
				self->enemy = blip;
				closest_dist = this_dist;

						self->movetarget = NULL;
                if (this_dist < nonvis_dist)
				{
					closest_nonvisible = blip;
					nonvis_dist = this_dist;
				}
			}
		}*/
/////////////////////////////////////////////////testing
	// only go beyond this point a few times per server frame
	if (roam_calls_this_frame > 10)	
		return;

	if (no_paths)
	{
		roam_calls_this_frame++;
	}
	else	// checking paths
	{
		self->last_roam_time = level.time;
		roam_calls_this_frame += 3;
	}

	if (ctf->value && CarryingFlag(self))
	{
		// don't go for items if carrying flag, at enemy base, and there is an enemy around
if (CarryingFlag(self) && (((int)wfflags->value & WF_ZOID_FLAGCAP) == 0)//$
	&& !(flagreturn1_ent == NULL || flagreturn2_ent == NULL))//$
{//$
botDebugPrint("zoid1\n");
		if (self->client->resp.ctf_team == CTF_TEAM1)//$
		{//$
			flag = flagreturn1_ent;//$
			enemy_flag = flag2_ent;//$
		}//$
		else//$
		{//$
			flag = flagreturn2_ent;//$
			enemy_flag = flag1_ent;//$
		}//$
}//$
else//$
{//$
botDebugPrint("normal1\n");
			if (self->client->resp.ctf_team == CTF_TEAM1)
		{
			flag = flag1_ent;
			enemy_flag = flag2_ent;
		}
		else
		{
			flag = flag2_ent;
			enemy_flag = flag1_ent;
		}
}//$
        // we don't really need anything, and enemy is close
		if (self->enemy && (entdist(self, self->enemy) < 384) && (self->health > 25))//acrid fixme && (self->bot_fire != botBlaster))
			return;		

		// don't check paths if we have flag, unless at base
		if ((entdist(self, flag) > 800) || (flag->solid))
			flaggoal = true;
	}

////////////// look for an item to get//////////////FIXME LOOK HERE ACRID

	if (	(!self->movetarget || (self->movetarget->routes) || 
		(no_paths && (entdist(self->movetarget, self) > 512)))
		// if going for enemy flag, don't look for a new target
		&&	!(self->movetarget && self->movetarget->solid && 
		self->movetarget->item && 
		(self->movetarget->item->pickup == CTFPickup_Flag) && 
		(self->movetarget->count != self->client->resp.ctf_team) && 
		(entdist(self->movetarget, self) < 600))
		&&	((self->bot_fire == botBlaster) || 
		(((0.3 * self->bot_stats->aggr) / 5.0) < random())))	// if really aggressive, don't always look for items
	{
		closest_dist = 999999; 
//aco
//     botDebugPrint("%s looking for an item.. ", self->client->pers.netname);
//    if (no_paths)
//	 botDebugPrint("(no paths)\n");
//     else
//	 botDebugPrint("(checking paths)\n");

		self->save_movetarget = self->movetarget;	// used in RoamFindBestWeapon()
		self->save_goalentity = self->goalentity;

		save_goal = self->goalentity;
		save_movetarget = self->movetarget;

		// look for health
		this_dist = RoamFindBestItem(self, health_head, !no_paths);
		if (this_dist > -1)
		{
			closest_dist = this_dist;
            // go for the item
			if (self->health < 10)	
			{
				goto gotgoal;
			}
			// we kinda need health
			else if (self->health < 50)	
			{
				this_dist = this_dist / 3;
			}

			save_goal = self->goalentity;
			save_movetarget = self->movetarget;

			if (no_paths && (this_dist < 128))
			{	// this will do
				goto gotgoal;
			}
		}

		// look for weapons//look here might be able to use this for depots/healing stations
		this_dist = RoamFindBestItem(self, weapons_head, !no_paths);
		if ((this_dist > -1) && (this_dist < closest_dist))
		{
			closest_dist = this_dist;
			save_goal = self->goalentity;
			save_movetarget = self->movetarget;

			if (no_paths && (this_dist < 128))
			{	// this will do
				goto gotgoal;
			}
		}
		else	// restore goals
		{
			self->goalentity = save_goal;
			self->movetarget = save_movetarget;
		}

		// look for bonuses
		this_dist = RoamFindBestItem(self, bonus_head, !no_paths);
		if ((this_dist > -1) && (this_dist < closest_dist))/* && //acrid
			(self->goalentity->wf_team == self->wf_team)
			&&(self->goalentity != flag))*///acrid added fixme fixme fixme for armor
		{	// go for this item instead

			closest_dist = this_dist;
			save_goal = self->goalentity;
			save_movetarget = self->movetarget;

			if (no_paths && (this_dist < 128))
			{	// this will do
				goto gotgoal;
			}//REF goal->item->pickup == CTFPickup_Flag
		}//whole acrid section//FIXME LOOK HERE ACRID
/*		else if  ((this_dist > -1) && (this_dist < closest_dist) && 
			(self->goalentity == flag))//acrid added fixme
		{	// go for this item instead
			closest_dist = this_dist;
			save_goal = self->goalentity;
			save_movetarget = self->movetarget;
           botDebugPrint(" flagger\n");
			if (no_paths && (this_dist < 128))
			{	// this will do
				goto gotgoal;
			}
		}*/
		else	// restore goals
		{
			self->goalentity = save_goal;
			self->movetarget = save_movetarget;
		}

		this_dist = RoamFindBestItem(self, ammo_head, !no_paths);
		if (this_dist > closest_dist)	// revert back to ammo targets
		{
			self->goalentity = save_goal;
			self->movetarget = save_movetarget;
		}
		else
		{
			closest_dist = this_dist;
		}

gotgoal:

		if (self->movetarget)
		{
			if (closest_dist < 999999)
			{
				if (flaggoal)
				{
					if (entdist(self->movetarget, self) > 128)
					{	// restore goals
						self->movetarget = self->save_movetarget;
						self->goalentity = self->save_goalentity;
					}
				}
				else
				{
//					botDebugPrint("%s going for %s ", self->client->pers.netname, self->movetarget->classname);
//					botDebugPrint("(%i)\n", (int)closest_dist);
				}
			}
		}
	}
//////////////////////////////////////////////////////////////////////////
	if (no_paths)	// don't go passed here
		return;

	if (self->movetarget || self->enemy)
	{//botDebugPrint("RETURN if self-enemy\n");
		return;
	}

	if (ClosestNodeToEnt(self, false, true) == -1)
	{	// can't get to a node
		goto roam_no_node;
	}
	else if (self->target_ent)
	{	// no need to roam, just go for the target_ent
		return;
	}

	// walk around aimlessly, if possible

		// pick a random node and go for it, hopefully once we get there, it'll lead us to sometihng else
		for (n=0; n<5; n++)	// try 10 different nodes
		{
			i = (int) ((trail_head-1) * random()) + 1;

			if (!trail[i]->timestamp)
				continue;

			if (entdist(self, trail[i]) < 800)
				continue;

			if (PathToEnt(self, trail[i], false, false) == -1)
				continue;

			if ((PathToEnt_Node->enemy == self) && (PathToEnt_Node->ignore_time > level.time))
				continue;

			botDebugPrint("Roaming towards node #%i\n", i);

			// found a distant node
			self->movetarget = trail[i];
			self->goalentity = PathToEnt_Node;
			return;
		}

roam_no_node:

	self->last_roam_time = level.time + 2;	// don't roam again for a few seconds
	self->movetogoal_time = level.time + 2;
	botRoamFindBestDirection(self);

	// nothing to do, if we've been lost for long enough, suicide
//	self->enemy = self->goalentity = self->movetarget = NULL;
	
	self->bored_suicide_time = save_suicide_time;
}


/*
=============
bot_run

  This is the core thinking routine
=============
*/
void G_SetClientEffects (edict_t *ent);

void bot_run (edict_t *self)
{
	float	dist;
	vec3_t	vec;
	edict_t	*old_goal;
botDebugPrint("frozen #%i\n",self->frozen);
botDebugPrint("frozen time #%i\n",self->frozentime);
	if (self->frozen)//botfreeze
		return;

	if (self->health <= 0)
	{
		self->s.modelindex2 = 0;
		self->s.modelindex3 = 0;
		return;
	}

	if (self->waterlevel > 2)
		self->flags |= FL_SWIM;		// so walkmove() works
	else
		self->flags &= ~FL_SWIM;

	// fix: somehow non-client's are becoming enemies, which causes crashes
	if (self->enemy && !self->enemy->client && 
	  ((strcmp(self->enemy->classname, "SentryGun") != 0)
	  || (strcmp(self->enemy->classname, "turret") != 0)))
		self->enemy = NULL;//Acrid 

	CTFEffects(self);

	CTFApplyRegeneration(self);

/*
if ((self->client->resp.ctf_flagsince > 0) && (self->client->resp.ctf_flagsince > (level.time - 10)))
dist = 0;
*/
	// do Quad Damage AI
	if ((self->client->quad_framenum > level.framenum) || self->client->invincible_framenum > level.framenum)
	{
		bot_roam(self, true);	// keep scanning for visible enemies, even though we may be chasing down a current enemy
	}
	else
	{
//Acrid removed old Co code
			bot_roam(self, true);
//Acrid removed old Co code
	}

	G_SetClientEffects(self);

	// support infinite AMMO
	if ((int)dmflags->value & DF_INFINITE_AMMO)
	{
		if (self->client->ammo_index)
			self->client->pers.inventory[self->client->ammo_index] = 999;
	}

	// check for taunting
	if ((self->s.frame <= FRAME_point12) && (self->s.frame >= FRAME_salute01))
	{
		if ((skill->value >= 3) && self->enemy && gi.inPVS(self->s.origin, self->enemy->s.origin))	// abort taunt
		{
			self->s.frame = FRAME_run1;
		}
		else
		{
			bot_ChangeYaw(self);
			bot_ChangeYaw(self);
			return;
		}
	}

	// this is the default moving distance per frame
	dist = (float) BOT_RUN_SPEED * bot_frametime;

	// ducking
	if (self->goalentity && (self->goalentity->maxs[2] < 32) && (self->goalentity->routes))
	{
		if (self->maxs[2] != self->goalentity->maxs[2])
		{
			if (self->maxs[2] < 32)
			{	// try to stand
				if (CanStand(self))
					self->maxs[2] = self->goalentity->maxs[2];
			}
			else	// duck
			{
				self->maxs[2] = self->goalentity->maxs[2];
			}
		}
	}

	// move slower if ducking
	if (self->maxs[2] == 4)
	{
		dist *= 0.5;
		self->viewheight = -2;
	}
	else {
		self->viewheight = 22;
/*
		if ((self->waterlevel > 2) || (!self->groundentity && (self->waterlevel > 0)))
		{
			dist *= 0.75;
		}
*/
	}
//////////////////////////////////////////////////////////////////////////
	// GRAPPLE
	if (self->client->ctf_grapple)
	{
		vec3_t oldorg;

		// so we don't fire immediately after switching from grapple
		self->last_fire = level.time + BOT_CHANGEWEAPON_DELAY;
	
		VectorCopy(self->s.origin, oldorg);

		CTFGrapplePull(self->client->ctf_grapple);
	    // set visible model vwep with 3.20 code
		ShowGun(self);		
        // it could have been killed in CTFGrapplePull()
		if (self->client->ctf_grapple)	
		{
			// face in direction of grapple
			if (self->groundentity)
			{
				VectorSubtract( self->client->ctf_grapple->s.origin, self->s.origin, vec);
				VectorNormalize(vec);
				self->s.angles[YAW] = vectoyaw(vec);
			}

			if (self->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY)
			{   // gravity will be applied in gi.Pmove()
				self->velocity[2] += sv_gravity->value*FRAMETIME;		
				VectorCopy(self->velocity, self->jump_velocity);
                // so we move really fast, and don't do any course adjustments
				self->groundentity = NULL;	
				if (CanJump(self))
				{
					self->s.origin[2] += 1;
					gi.linkentity(self);
				}
				bot_move(self, dist);

				if (self->client->ctf_grapple)
				{
					static vec3_t	goal_dir, grapple_dir, diff_dir;
					static edict_t	*nextgoal;
					qboolean	abort_grapple=false;

					// if grapple is not in direction of goal, abort
					if (self->goalentity && (self->groundentity || self->waterlevel > 1) && self->last_movegoal && (self->goalentity->node_type != NODE_LANDING))
					{
						VectorSubtract(self->client->ctf_grapple->s.origin, self->s.origin, grapple_dir);
						VectorNormalize(grapple_dir);

						VectorSubtract(self->goalentity->s.origin, self->s.origin, goal_dir);
						VectorNormalize(goal_dir);

						VectorSubtract(grapple_dir, goal_dir, diff_dir);

						if (VectorLength(diff_dir) > 0.25)
						{	// see if the next goal is ahead

							abort_grapple = true;

							if (PathToEnt(self->goalentity, self->last_movegoal, false, false) > -1)
							{
								VectorSubtract(PathToEnt_Node->s.origin, self->s.origin, goal_dir);
								VectorNormalize(goal_dir);

								VectorSubtract(grapple_dir, goal_dir, diff_dir);

								if (VectorLength(diff_dir) < 0.25)
								{
									self->goalentity = PathToEnt_Node;
									abort_grapple = false;
								}
							}
						}
					}

					if (	(abort_grapple)
						||	((fabs(self->client->ctf_grapple->s.origin[2] - (self->s.origin[2])) < 40) && (entdist(self->client->ctf_grapple, self) < (44*(((!self->goalentity) || (self->goalentity->node_type != NODE_LANDING)) + 1))))
						||	(self->client->ctf_grapplestart < (level.time - 3)))
					{	// give up, or drop to floor
						CTFResetGrapple(self->client->ctf_grapple);
                        // set visible model vwep with 3.20 code 
						ShowGun(self);	// update view model

						if (self->goalentity && (PathToEnt(self->goalentity, self->last_movegoal, false, false) > -1))
						{
							VectorSubtract(PathToEnt_Node->s.origin, self->s.origin, self->velocity);
							VectorNormalize(self->velocity);
							VectorScale(self->velocity, 240, self->velocity);

							self->goalentity = PathToEnt_Node;
						}
						else
						{
							AngleVectors(self->s.angles, vec, NULL, NULL);
							VectorScale(vec, 80, self->velocity);
							VectorCopy(self->velocity, self->jump_velocity);
						}

						self->goalentity = NULL;
					}
				}

				return;
			}
			else	// only allow the bot to move while firing grapple under certain conditions
			{
				if (	(self->waterlevel > 2)
					||	(self->goalentity && (fabs(self->s.origin[2] - self->client->ctf_grapple->s.origin[2]) < 128)))
				{
					bot_move(self, dist);
					return;
				}

				if (self->goalentity && self->goalentity->s.origin[2] > self->s.origin[2])
					return;
			}
		}

	}
	// END GRAPPLE

	// are these still required??
	if (self->enemy == world)
		self->enemy = NULL;
	if (self->enemy == self)
		self->enemy = NULL;

	// teamplay, prevent attacking someone on our team
	if (self->enemy && self->enemy->client && SameTeam(self, self->enemy)
		&& ((strcmp(self->enemy->classname, "SentryGun") != 0)
		|| (strcmp(self->enemy->classname, "turret") != 0)))
		self->enemy = NULL;

// ACRID NURSE HEALING
/*if((self->client->player_class == 2) &&
   (self->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))] > 0))*/

	// this stuff checks our current movetarget, to see if we should still be going for it
	if (self->movetarget && (self->movetarget->item))
	{	// check that the movetarget is still reachable

		if (	(self->movetarget->solid != SOLID_TRIGGER) // can't get it anymore
				// if this item is REALLY desirable, hover around it, waiting for it to respawn
			&&	(	(self->movetarget_want < WANT_SHITYEAH) 
	||	(	(self->movetarget->nextthink > (level.time + 4))
	&&	(	(self->movetarget->item->pickup != CTFPickup_Flag)
	||	!CarryingFlag(self)))))//ACRID LOOK HERE FOR FLAG GUARDING
		{
			self->last_roam_time = 0;
			self->movetarget = NULL;
		}
		else if (self->movetarget->item->pickup == Pickup_Health)
		{	// check that we still need this health
			if (!(self->movetarget->style & HEALTH_IGNORE_MAX))
				if (self->health >= self->max_health)
					self->movetarget = NULL;
		}//flagref
		else if (self->movetarget->item->pickup == CTFPickup_Flag)
		{
			if (self->movetarget == self->target_ent)
			{
				if (entdist(self->movetarget, self) < 384)
					self->movetarget = NULL;
			}
			else if (self->movetarget->solid != SOLID_TRIGGER)
			{
				if (entdist(self->movetarget, self) < 256)
					self->movetarget = NULL;
			}
		}
	}

	// check for dead enemy
	if (self->enemy && (self->enemy->health <= 0) && (self->groundentity) 
		&& self->enemy->client)
	{	// enemy is dead, taunt if no enemies around

		self->goalentity = NULL;

		// check for the enemy dropping a weapon
		RoamFindBestItem(self, weapons_head, false);

		if ((self->bot_fire != botBlaster) && (entdist(self, self->enemy) < 384) && (random() < 0.5))
		{
			vec3_t	vec;
			int		in_range=false;

			VectorSubtract(self->enemy->s.origin, self->s.origin, vec);

			if (!in_range)
			{	// taunt
				float	rnd;
				edict_t	*chat;

				chat = G_Spawn();
				chat->owner = self;
				chat->enemy = self->enemy;
				chat->think = BotInsultStart;
				chat->nextthink = level.time + 1.5 + random();

				// face towards player
				self->ideal_yaw = vectoyaw(vec);

				rnd = random() * 3;
				if (rnd < 1)
				{
					self->s.frame = FRAME_taunt01;
					self->radius_dmg = FRAME_taunt17;
				}
				else if (rnd < 2)
				{
					self->s.frame = FRAME_salute01;
					self->radius_dmg = FRAME_salute11;
				}
				else
				{
					self->s.frame = FRAME_point01;
					self->radius_dmg = FRAME_point12;
				}

				return;
			}

		}

		self->enemy = NULL;

		if (!self->movetarget)
		{
			self->last_roam_time = 0;	// force check for new enemy
			self->last_nopaths_roam = 0;
			self->search_time = 0;
			bot_roam(self, false);
		}

	}
//////////////////////////////////check for dead end//////////////////////
	if (!self->enemy && !self->movetarget && !self->target_ent)
		self->goalentity = NULL;

	old_goal = self->goalentity;

	// if roaming aimlessly, and we have found an enemy, aborting roaming
	if (self->movetarget && self->movetarget->routes && self->enemy)	// we have an enemy!
	{
		if (PathToEnt(self, self->enemy, false, false) > -1)	// make sure we can reach it still
		{
			self->goalentity = PathToEnt_Node;
			self->movetarget = NULL;
		}
		else	// no path, so abort enemy
		{
			self->enemy = NULL;
		}
	}
//fixme acrid remove bot blaster dumb lines called alot might be a prob
	if ((!self->movetarget || self->movetarget->routes) 
		&& (!self->enemy || (self->bot_fire == botBlaster)))
	{
		edict_t	*oldgoalentity = self->goalentity;
		bot_roam(self, false);
//aco		if (self->target_ent)
			self->goalentity = oldgoalentity;
	}

	if (!self->enemy && !self->movetarget && !self->target_ent)
	{
		botRoamFindBestDirection(self);
		bot_move(self, dist);
	}
	else // we have a goal, so go for it, and attack if necessary
	{
		// set ideal yaw
		if ((self->last_ladder_touch > (level.time - 0.2)) && self->goalentity)
		{botDebugPrint("else 1\n");
			VectorSubtract(self->goalentity->s.origin, self->s.origin, vec);
			self->ideal_yaw = vectoyaw(vec);
		}
		else if (!self->groundentity && self->goalentity && ((self->goalentity->s.origin[2] - 80) > self->s.origin[2]))
		{botDebugPrint("else 2\n");
			VectorSubtract(self->goalentity->s.origin, self->s.origin, vec);
			self->ideal_yaw = vectoyaw(vec);
		}
		else if (self->goalentity && self->goalentity->goalentity && self->waterlevel && (self->waterlevel < 3) && (!self->enemy || (entdist(self, self->enemy) > 256)))
		{botDebugPrint("else 3\n");
			VectorSubtract(self->goalentity->goalentity->s.origin, self->goalentity->s.origin, vec);
			self->ideal_yaw = vectoyaw(vec);
		}
		else if (self->enemy && ((entdist(self, self->enemy) < 600) || (gi.inPVS(self->s.origin, self->enemy->s.origin))))
		{
			VectorSubtract(self->enemy->s.origin, self->s.origin, vec);
			self->ideal_yaw = vectoyaw(vec);
		}
		else if (self->target_ent && (self->s.frame <= FRAME_stand40) && (entdist(self->target_ent, self) < 600))
		{//self standing still acrid
			VectorSubtract(self->s.origin, self->target_ent->s.origin, vec);
			self->ideal_yaw = vectoyaw(vec);
		}//called alot must be >2
		else if (self->goalentity && (entdist(self, self->goalentity) > 2))
		{
			VectorSubtract(self->goalentity->s.origin, self->s.origin, vec);
			self->ideal_yaw = vectoyaw(vec);
		}

		bot_ChangeYaw(self);

		bot_MoveAI(self, dist);

		bot_SuicideIfStuck(self);

		if (self->enemy)
		{
			bot_Attack(self);
		}
		else
		{
			if ((self->health > 0) && (self->s.frame >= FRAME_attack1) && (self->s.frame <= FRAME_attack8))
			{	// get out of attack frames
				self->s.frame = FRAME_run1;
			}
		}

	}

	if (self->goalentity && (self->goalentity->node_type == NODE_PLAT))
	{	// check for platform pausing, while going upwards
		if (self->groundentity && (self->groundentity->use == Use_Plat) &&
			(self->groundentity->moveinfo.state == STATE_UP))
		{	// wait for plat to rise/fall
			self->bot_plat_pausetime = level.time + 0.3;
			goto noabortcheck;
		}
	}

	// keep track of each time our goalentity changes
	if (self->goalentity != self->giveup_lastgoal)
	{
		self->giveup_lastgoal = self->goalentity;
		self->last_reached_trail = level.time;
	}

	if (!self->client->ctf_grapple && self->goalentity /*&& self->goalentity->routes*/
		&& ((self->last_movegoal != self->enemy) || 
		(self->last_enemy_sight < (level.time - 0.5))))	// if we've been going for this node for too long, give up
	{
		if (	((self->last_reached_trail < (level.time - 2)) && 
			(entdist(self, self->goalentity) > 128))
			||	(self->last_reached_trail < (level.time - 4)))
		{
//			edict_t	*goal;

			if (self->movetarget)
			{
//acrid co botDebugPrint("Giving up search for %s\n", self->movetarget->classname);
				self->movetarget->ignore_time = level.time + 3;
				self->movetarget->enemy = self;
				self->movetarget = NULL;
			}

			if (self->enemy)
			{
//acrid co botDebugPrint("Giving up search for %s\n", self->enemy->client->pers.netname);
				self->enemy->ignore_time = level.time + 1;
				self->enemy->enemy = self;
				self->enemy = NULL;
			}

			self->goalentity->ignore_time = level.time + 0.5;
			self->goalentity->enemy = self;
			self->goalentity = NULL;

			bot_roam(self, false);

			// go for the new target for at least a few seconds
			self->last_reached_trail = level.time;
		}
	}

noabortcheck:

	if (self->movetarget && !self->movetarget->item && (self->last_roam_time < (level.time - 5)))
	{	// look for new targets, since we're roaming
		bot_roam(self, false);
	}

}

void	bot_ChangeYaw(edict_t *self)
{	// this used to do special handling
	M_ChangeYaw(self);
};

void	BotLadderEnd(edict_t	*self)
{	// end of ladder, jump forward
	vec3_t	dir;

	self->last_ladder_touch = -1;

	AngleVectors(self->s.angles, dir, NULL, NULL);
	VectorScale(dir, 240, self->velocity);
	self->velocity[2] = 300;
	VectorCopy(self->velocity, self->jump_velocity);
};

qboolean	botTouchingLadder(edict_t *self)
{
	vec3_t org;

	VectorCopy(self->s.origin, org);

	org[2] += 48;

	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[0] += 8; org[1] += 8;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[0] += -16;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[1] += -16;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[0] += 16;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	org[0] -= 8; org[1] += 8;
	org[2] -= 48;
	if (gi.pointcontents(org) & CONTENTS_LADDER)
		return true;

	return false;
};

void	bot_MoveAI(edict_t *self, int dist)
{
	vec3_t	dir, angle;
	edict_t	*goal=NULL;
	int		goal_in_range = false;
	float	goal_dist;
	int		done_move=false;

//gi.dprintf("%s moving (%i)\n", self->map, (int) entdist(self, the_client));

	// check platform riding
	if (self->groundentity && 
		(self->goalentity) && (self->goalentity->node_type == NODE_PLAT))
	{
		if (self->groundentity->use == Use_Plat)
		{
			if	(self->groundentity->moveinfo.state == STATE_UP)
			{	// make sure we keep waiting
				vec3_t		center;
				float length;

				self->bot_plat_pausetime = level.time + 0.3;

				// walk towards the center
				VectorCopy(self->goalentity->s.origin, center);

				center[2] = self->s.origin[2];

				VectorSubtract(center, self->s.origin, dir);

				if ((length = VectorLength(dir)) > 12)
				{
					VectorNormalize(dir);
					M_walkmove(self, vectoyaw(dir), 10);
				}
				else
				{
					VectorNormalize(dir);
					M_walkmove(self, vectoyaw(dir), length);
				}
			}
			else if (self->groundentity->moveinfo.state == STATE_BOTTOM)
			{	// move towards the center of the platform
				qboolean	activator_set=false;
				vec3_t		center;

				if (!self->activator)	// HACK: to walk in ideal_yaw direction, bot_move() likes to see a self->activator
				{
					self->activator = self->goalentity;
					activator_set = true;
				}

				// find center of plat, at current Z
				VectorSubtract(self->groundentity->maxs, self->groundentity->mins, dir);
				VectorMA(self->groundentity->mins, 0.5, dir, center);
				center[2] = self->s.origin[2];

				VectorSubtract(center, self->s.origin, dir);
				vectoangles(dir, angle);
				self->ideal_yaw = angle[1];

				bot_move(self, dist);

				if (activator_set)
					self->activator = NULL;
			}
		}
		// if it's not ready, wait here for a bit
		else if (self->goalentity->target_ent->moveinfo.state != STATE_BOTTOM)
		{
			self->bot_plat_pausetime = level.time + 0.3;

			if (entdist(self, self->goalentity) < 200)
			{
				if (self->last_goal && (self->last_goal != self->goalentity))
				{	// face towards old goal
//					if (entdist(self, self->last_goal) > 10)
//					{
						VectorSubtract(self->last_goal->s.origin, self->goalentity->s.origin, dir);
						VectorNormalize2(dir, dir);
						vectoangles(dir, angle);

						self->s.angles[1] = angle[1];
						if (!M_walkmove(self, self->s.angles[1], 10))
							if (!M_walkmove(self, self->s.angles[1]+45, 10))
								if (!M_walkmove(self, self->s.angles[1]-45, 10))
									if (!M_walkmove(self, self->s.angles[1]+100, 10))
										M_walkmove(self, self->s.angles[1]-100, 10);
//					}
				}
				else
				{
					// move backwards
					M_walkmove(self, self->s.angles[1] + 180, 10);
				}
			}
		}

		if (self->bot_plat_pausetime > level.time)
			return;
	}
	else if (self->bot_plat_pausetime > -1)
	{	// set new goalentity, since we're at the top of the platform
/*
		if (self->movetarget)
			goal = self->movetarget;
		else if (self->enemy)
			goal = self->enemy;
		else
			goal = NULL;

		if (goal && (PathToEnt(self, goal, false, true) > -1))
		{
			self->goalentity = PathToEnt_Node;
		}
*/
//		self->goalentity = NULL;
		self->bot_plat_pausetime = -1;
	}


	// make sure we crouch when we have to
	if (self->goalentity && self->goalentity->routes && (self->goalentity->maxs[2] < 32))
	{					//  ^^ don't set maxs to that of an item
		if (self->maxs[2] != self->goalentity->maxs[2])
		{
			if (self->maxs[2] < 32)
			{	// try to stand
				if (CanStand(self))
					self->maxs[2] = self->goalentity->maxs[2];
			}
			else	// duck
			{
				self->maxs[2] = self->goalentity->maxs[2];
			}
		}
	}
	else if (self->movetarget && (self->goalentity == self->movetarget) && self->movetarget->movetarget
			 && (self->movetarget->movetarget->maxs[2] < 32))
	{	// duck
		self->maxs[2] = 4;
	}
	else if (self->maxs[2] < 32 && (self->crouch_attack_time < level.time))
	{
		if (CanStand(self))
			self->maxs[2] = 32;
	}

	if (!self->groundentity && !self->waterlevel)
	{	// in-air, also check for ladder movement
		vec3_t	dir;

		if ((self->goalentity) && ((self->goalentity->s.origin[2]+8) > self->s.origin[2])
			&& botTouchingLadder(self))
		{
			AngleVectors(self->goalentity->s.angles, dir, NULL, NULL);
			VectorScale(dir, 48, self->velocity);
			self->velocity[2] = 310;

			self->last_ladder_touch = level.time;
		}
		else if (self->last_ladder_touch > (level.time - 0.3))
		{
			BotLadderEnd(self);
		}

		bot_move(self, dist);
		return;
	}
	else if (self->last_ladder_touch > (level.time - 0.3))
	{
		BotLadderEnd(self);

		bot_move(self, dist);
		return;
	}

	if (self->activator)
	{	// going for a button

		if (self->activator_time < (level.time - 5))
		{	// abort it
			self->activator = self->goalentity = NULL;
		}
/*
		else if ((self->activator->moveinfo.state == STATE_BOTTOM) && !CarryingFlag(self))
		{
			float dist1;
			vec3_t	postvec;

			VectorSubtract(self->activator->absmin, self->s.origin, dir);
			dist1 = VectorLength(dir);
			VectorNormalize2(dir, dir);

			self->ideal_yaw = vectoyaw(dir);

			bot_move(self, dist);

			// did we move towards the button?
			VectorSubtract(self->activator->absmin, self->s.origin, postvec);
			if (VectorLength(postvec) >= dist1)
				self->activator = NULL;

			return;
		}
		else
		{
			self->activator = self->goalentity = NULL;
		}
*/
		// get to the button
		goal = self->activator;

		goto got_goal;
	}

	// We're kinda lost, so walk in the ideal_yaw direction, and randomly jump if stuck
	if (self->movetogoal_time > level.time)
	{
		vec3_t	oldorg;

		VectorCopy(self->s.origin, oldorg);

		bot_move(self, dist);

		VectorSubtract(self->s.origin, oldorg, oldorg);

		// jump randomly, so as not to get hung up
		if (VectorLength(oldorg) < 3)
		{
			self->movetogoal_time = 0;
/*
			if ((random() < 0.5) && (CanJump(self)))
			{
botDebugPrint("MoveToGoal: random jump\n");
				botRandomJump(self);
			}
*/
		}

		return;
	}

	// HACK, make sure we head for home if we have the flag!
	if (ctf->value && CarryingFlag(self))// && 
//		(!self->movetarget || !self->movetarget->item || (self->movetarget->item->pickup != CTFPickup_Flag)))
	{
		edict_t *flag, *enemy_flag, *plyr;
		int	i=0, count=0, ideal;
		static float	last_checkhelp=0;
if (CarryingFlag(self) && (((int)wfflags->value & WF_ZOID_FLAGCAP) == 0)//$
	&& !(flagreturn1_ent == NULL || flagreturn2_ent == NULL))//$
{//$
	botDebugPrint("zoid2\n");
		if (self->client->resp.ctf_team == CTF_TEAM1)//$
		{//$
			flag = flagreturn1_ent;//$
			enemy_flag = flag2_ent;//$
		}//$
		else//$
		{//$
			flag = flagreturn2_ent;//$
			enemy_flag = flag1_ent;//$
		}//$
}//$
else//$
{//$
	botDebugPrint("normal2\n");
		if (self->client->resp.ctf_team == CTF_TEAM1)
		{
			flag = flag1_ent;
			enemy_flag = flag2_ent;
		}
		else
		{
			flag = flag2_ent;
			enemy_flag = flag1_ent;
		}
}//$
		// look for some helpers
//		if (last_checkhelp < (level.time - 0.5))
		{
			for (i=0; i<num_players; i++)
			{
				plyr = players[i];

				if (plyr->client->resp.ctf_team != self->client->resp.ctf_team)
				{
					if (	(plyr->enemy != self) 
//						&&	(!plyr->target_ent || (plyr->target_ent->think != CTFFlagThink) || (entdist(plyr, plyr->target_ent) > 1000))
						&&	(entdist(plyr, self) < 2000))
					{	// send this enemy to us
						plyr->enemy = self;
					}
//					continue;
				}
				else if ((plyr != self) && (plyr->target_ent == self))
					count++;
			}

			ideal = ((int)ceil((1.0*(float)num_players)/4.0));

			if (count < ideal)
			{
				for (i=0; (i<num_players && count<ideal); i++)
				{
					plyr = players[i];

					if (plyr->client->resp.ctf_team != self->client->resp.ctf_team)
						continue;

					if (plyr->target_ent == self)
						continue;

					if (entdist(plyr, self) > 700)
						continue;

					if (!gi.inPVS(plyr->s.origin, self->s.origin))
						continue;

					plyr->target_ent = self;
					if (++count >= ideal)
						break;
				}
			}
			else if (count > ideal)	// release a defender
			{
				for (i=0; (i<num_players && count<ideal); i++)
				{
					plyr = players[i];

					if (plyr->client->resp.ctf_team != self->client->resp.ctf_team)
						continue;

					if (plyr->target_ent != self)
						continue;

					plyr->target_ent = NULL;
					break;
				}
			}

			last_checkhelp = level.time + random()*0.5;
		}
        //flagref
		if ((flag->solid == SOLID_TRIGGER) || (entdist(self, flag) > 700) 
			|| CarryingFlag(self))// FLAG CAPPING FIX ACRID 
		{
			if (	(!self->movetarget)
				||	(	(self->movetarget != flag)
					 &&	(	(entdist(self->movetarget, self) > 128)
						 ||	(entdist(self, enemy_flag) < 1000))))
			{

				self->movetarget = flag;
			}

			if (self->movetarget == flag)
			{
				if ((self->target_ent == flag) && (flag->solid == SOLID_TRIGGER))
					self->target_ent = NULL;
				botDebugPrint("movetarg flag2\n");
				goal = flag;
				goto got_goal;
			}
		}
		else if (self->movetarget && self->movetarget->item && 
				 (self->movetarget->item->pickup == CTFPickup_Flag))	// do something else?
		{
			self->movetarget = NULL;
		}

		self->target_ent = NULL;
	}

	// ------------------------------------------------------------
	// OK, now choose which goal to head for (enemy or movetarget)
	// and go for it.

	goal = NULL;

	// always head for flag carrier //fixme acrid pain bots getting attacked by other players
	if (self->enemy && !CarryingFlag(self) && CarryingFlag(self->enemy))
	{
		goal = self->enemy;
		self->last_seek_enemy = level.time;

		goto got_goal;
	}

	// start off with the movetarget, then overwrite if necessary
	if (self->movetarget)
	{
		goal = self->movetarget;

		if (goal->touch == CTFDropFlagTouch)
			goto got_goal;
        else if (((strcmp(goal->classname, "item_flagreturn_team1") == 0) ||//$
	              (strcmp(goal->classname, "item_flagreturn_team2") == 0)) &&//$
				  CarryingFlag(self) && !(flagreturn1_ent == NULL || flagreturn2_ent == NULL))//$
	 	    goto got_goal;//$
		else if (goal->item && (goal->item->pickup == CTFPickup_Flag) && goal->solid && (goal->count != self->client->resp.ctf_team) && (entdist(goal, self) < 1000))
			goto got_goal;
	}

	if (self->enemy && (!CarryingFlag(self) || !goal)
		 && ((!ctf->value || !self->movetarget || !self->movetarget->item || (self->movetarget->item->pickup != CTFPickup_Flag))))	// attacking enemy, may be going for an item also, decide which to go for
	{
		if	(!	(	(goal)
				 && (	(self->movetarget_want >= WANT_SHITYEAH)
					 || (self->bot_fire == botBlaster)
					 || (self->bot_fire == botShotgun)
					 || ((goal_dist = entdist(self->movetarget, self)) < 512) 
					 || (self->enemy->health > self->health)
					 ||	(goal_dist < entdist(self, self->enemy))
					)
				)
			 ||	(	ctf->value 
				 && (HomeFlagDist(self) < 1500))	// defend base!
			)
		{	// enemy is best goal
			goal = self->enemy;
			self->last_seek_enemy = level.time;
		}
	}
	// check for team goal
	if (	(self->target_ent && !CarryingFlag(self))
		&&	(!self->enemy || !CarryingFlag(self->enemy))
		&&	(	!goal 
			 ||	(self->target_ent->client)
			 ||	(	(goal == self->movetarget)
				 &&	((entdist(goal, self->target_ent) / self->movetarget_want) > BOT_GUARDING_RANGE))))
	{

		if (self->target_ent->client)
		{
			if (self->target_ent->health <= 0)
			{	// leader is dead, resume normal roaming
				self->target_ent = NULL;
			}
			else if (!self->movetarget || (entdist(self->target_ent, self->movetarget) > 400))	// must follow leader!
			{
				if (self->movetarget)
				{
					self->movetarget->ignore_time = level.time + 2;
					self->movetarget = NULL;
				}

				goal = self->target_ent;
			}
		}
		else if (self->target_ent->item)	// defending an item (CTF flag, etc)
		{
			if (self->bot_fire == botBlaster)
			{
			}
			if ((entdist(self, self->target_ent) > BOT_GUARDING_RANGE) ||
				!gi.inPVS(self->s.origin, self->target_ent->s.origin))
			{	// go to our target_ent
				goal = self->target_ent;
				self->movetarget = NULL;
			}
			else	// hang around for a bit
			{
				self->group_pausetime = level.time + 1;
				self->checkstuck_time = level.time;

				if (self->movetarget == self->target_ent)
					self->movetarget = NULL;
			}
		}

	}
got_goal:

	// flagpath stuff
	if (self->flagpath_goal)
	{botDebugPrint("flaggoalpath\n");
		if (CarryingFlag(self))
			goal = self->flagpath_goal;
		else
			self->flagpath_goal = NULL;
	}
	// end: flagpath stuff

	self->last_movegoal = goal;

	// if enemy has moved out of view, look for a new goal
	if (	(goal)
		&&	(goal->client)//fixme?
		&&	(self->goalentity == goal) && !visible_box(self, goal))
	{
				botDebugPrint("goal->client\n");
		self->goalentity = NULL;
	}

	// ------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
	// check for a node jump
	if (goal && self->goalentity && self->goalentity->routes 
		&& self->goalentity->goalentity) 
	if (entdist(self->goalentity, self) < 32)//fix this?
	if (CanJump(self))
	{
		if ((PathToEnt(self->goalentity, goal, false, false) > -1) && 
			(PathToEnt_Node == self->goalentity->goalentity))
		{	// jumping from here, will get us closer to the goal
			vec3_t	jump_vec, org;
			trace_t trace;
			int		i;

			VectorCopy(self->goalentity->velocity, jump_vec);
			jump_vec[2] = 0;

			// grapple?//Acrid FIXME this is the prob
			if ((self->goalentity->node_type == NODE_NORMAL)//FIXME ACRID WAS NODE_GRAPPLE 
&& (self->goalentity->s.origin[2] <= PathToEnt_Node->s.origin[2]))
//&& ((self->client->player_special & SPECIAL_GRAPPLE) != 0))//future fix
			{
				// find the target point, and face towards it
				VectorScale(self->goalentity->velocity, 8000, org);
				VectorAdd(self->goalentity->s.origin, org, org);
				trace = gi.trace(self->goalentity->s.origin, NULL, NULL, org, NULL, MASK_SOLID);

				VectorSubtract(trace.endpos, self->s.origin, jump_vec);
				VectorNormalize(jump_vec);

				vectoangles(jump_vec, self->client->v_angle);
				CTFGrappleFire (self, vec3_origin, 10, 0);
                // set visible model vwep with 3.20 code
				ShowGun(self);

				self->goalentity = PathToEnt_Node;
				return;
			}

			if (self->movetarget && (entdist(self, self->movetarget) < 64))
			{	// movetarget is in range, so jump towards it
				VectorSubtract(self->movetarget->s.origin, self->goalentity->s.origin, dir);
				dir[2] = 0;
				VectorScale(dir, 2, dir);
			}
			else if (VectorLength(jump_vec) > 80)
			{	// the velocity of the jump node is good enough
				VectorCopy(self->goalentity->velocity, dir);
			}
			else 
			{	// just jump in the direction we're going
//Acrid removed old co code

					if ((self->goalentity->goalentity->s.origin[2] - 64) < self->goalentity->s.origin[2])
					{
						if (self->goalentity->waterlevel && !self->goalentity->goalentity->waterlevel)
						{	// coming out of water, jump in the direction the goal is facing
							AngleVectors(self->goalentity->s.angles, dir, NULL, NULL);
							VectorScale(dir, 240, dir);
							dir[2] = 310;
							goto gotvel;
						}
						else
						{
							VectorSubtract(self->goalentity->goalentity->s.origin, self->goalentity->s.origin, jump_vec);
						}
					}
					else	// must be a ladder
					{
						AngleVectors(self->goalentity->s.angles, jump_vec, NULL, NULL);
						VectorScale(jump_vec, 10, jump_vec);
					}

					jump_vec[2] = 0;

					if (VectorLength(jump_vec) > 8)
						VectorCopy(jump_vec, dir);
					else
					{
						AngleVectors(self->s.angles, dir, NULL, NULL);
					}

					dir[2] = 0;
					VectorNormalize2(dir, dir);
					VectorScale(dir, 50, dir);
//				}
			}

			dir[2] = 0;
			if (VectorLength(dir) > 300)
			{
				VectorNormalize2(dir, dir);
				VectorScale(dir, 300, dir);
			}
			else if (VectorLength(dir) < 20)
			{
				VectorNormalize2(dir, dir);
				VectorScale(dir, 20, dir);
			}

			dir[2] = self->goalentity->velocity[2];

gotvel:

			if (dir[2] < 80)
			{	// if directly ahead is blocked we should jump higher
				static vec3_t joffs = {0,0,8};
				vec3_t	vec, org;

				VectorAdd(self->s.origin, joffs, org);
				VectorCopy(dir, vec);
				vec[2] = 0;
				VectorNormalize2(vec, vec);
				VectorScale(vec, 32, vec);
				VectorAdd(org, vec, vec);
				trace = gi.trace(org, VEC_ORIGIN, VEC_ORIGIN, vec, self, MASK_SOLID);

				if (trace.fraction < 1)
					dir[2] = 310;
				else if ((self->goalentity->goalentity->s.origin[2] + 96) < self->goalentity->s.origin[2])
				{
					VectorSubtract(self->goalentity->goalentity->s.origin, self->goalentity->s.origin, jump_vec);
					jump_vec[2] = 0;

					if (VectorLength(jump_vec) < 96)
					{
						VectorNormalize2(jump_vec, dir);
						VectorScale(dir, 150, dir);
						dir[2] = 160;
					}
				}
			}
			else
			{
				dir[2] += 80;
			}

			VectorCopy(dir, self->velocity);

			if (self->velocity[2] > 191)
			{
				self->velocity[2] = 300;
			}
			else	// ladder?
			{
				if (botTouchingLadder(self) && (self->goalentity->goalentity->s.origin[2] > self->goalentity->s.origin[2]))
				{
					self->velocity[2] = 300;
				}
				else if (self->velocity[2] < 40)
				{
					self->velocity[2] = 40;
				}
			}

			VectorCopy(self->goalentity->s.origin, org);
//			org[2] += 1;
			trace = gi.trace(org, self->mins, self->maxs, org, self, MASK_PLAYERSOLID);

			if (trace.fraction == 1)
				VectorCopy(org, self->s.origin);

			self->groundentity = NULL;

			gi.linkentity(self);

			VectorCopy(self->velocity, self->jump_velocity);

			// make sure once we land, we go for the next node
			if (((i = PathToEnt(self->goalentity->goalentity, goal, false, false)) > -1) && (PathToEnt_Node->s.origin[2] > self->s.origin[2]))
				self->goalentity = PathToEnt_Node;
			else	/// just head for the landing node
				self->goalentity = self->goalentity->goalentity;

			if (self->velocity[2] > 200)
			{ 
				gi.sound(self, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, 2, 0);
			}
//botDebugPrint("Trail jump: %s\n", vtos(self->velocity));

			return;
		}
		else	// there is a better route than jumping
		{
			self->goalentity = PathToEnt_Node;
		}
	}
	// if we've found a valid goal, make sure we have a visible destination (goalentity)
	if (goal)
	{
		if (!self->enemy && (self->target_ent == self))
		{
			if (self->group_pausetime < level.time)
			{	// start this leader on a path of destruction
				self->target_ent = NULL;
			}
			else	// hang around for a bit
			{
				return;
			}
		}
//ref(self->client->pers.weapon != item_blaster)
		// Squadron following
		if (goal == self->target_ent)
		{	
//Acrid removed old co code here flag thinking
			if ((goal_dist = entdist(goal, self)) < (BOT_IDEAL_DIST_FROM_ENEMY / 2) 
				&& (self->client->pers.weapon != item_knife))
			{	
				// too close, move back
				vec3_t vec;
//fixme acrid look here for attacking sentrys distance
				self->goalentity = NULL;
                botDebugPrint("ideal dis 1\n");
				VectorSubtract(self->s.origin, goal->s.origin, vec);
				VectorNormalize2(vec, vec);
				self->ideal_yaw = vectoyaw(vec);

				bot_move(self, dist);
			}

			// if in proximity, then hang around for a bit longer
			if (goal_dist < (BOT_IDEAL_DIST_FROM_ENEMY * (1+((goal->think == CTFFlagThink)*2)))
				 && (self->client->pers.weapon != item_knife))
			{      botDebugPrint("ideal dis 3\n");
				if (goal->client && self->movetarget)
					self->movetarget = NULL;
				self->group_pausetime = level.time + 0.2;	// don't check_stuck
				return;
			}

			// otherwise, move towards leader as you would any other item
		}

		if (!self->goalentity)
		{	// find a path damnit!
			if (PathToEnt(self, goal, false, false) > -1)
			{
				self->goalentity = PathToEnt_Node;
			}
			else	// no path, so stop looking for this "thing"
			{
				if (goal == self->movetarget)
					self->movetarget = NULL;
				else if (goal == self->enemy)
					self->enemy = NULL;
				else if (goal == self->target_ent)
					self->target_ent = NULL;
			}
		}
		else if ((self->goalentity == self->enemy) &&
				 ((goal_dist = entdist(self->enemy, self)) < BOT_IDEAL_DIST_FROM_ENEMY)
				 && (self->client->pers.weapon != item_knife))
		{	
			// in range
			if (goal_dist < (BOT_IDEAL_DIST_FROM_ENEMY / 2))
			{	
			    // too close, move back
				vec3_t vec;
                botDebugPrint("ideal dis 4\n");
				self->goalentity = NULL;

				VectorSubtract(self->s.origin, self->enemy->s.origin, vec);
				VectorNormalize2(vec, vec);
				self->ideal_yaw = vectoyaw(vec);

				bot_move(self, dist);
			}

			return;
		}
	}
	else	// no goal, so clear goalentity and get out of here!
	{
		self->goalentity = NULL;
		return;
	}
//////////////////////////////////////////////////////////////////////////
	// *********************************************************
	//
	// This is the MAIN movement AI
	// 
	// If we have a goalentity, walk towards it, when we reach it,
	// find the next goal for the next frame

	if (	(self->goalentity)								// if we don't have a goalentity, then no point trying to go anywhere
		&&	(	(	(self->last_inair < level.time)			// if just landed check for a new goalentity from landing position
				 ||	(self->goalentity->s.origin[2] <= self->s.origin[2])
				 ||	(!visible_box(self, self->goalentity))
				 || (	(PathToEnt(self->goalentity, goal, false, false) > -1)
					 &&	(visible_box(self, PathToEnt_Node))
					 &&	(self->goalentity = PathToEnt_Node)))

			 // we need a new goalentity, since we've just landed
			 ||	(	(PathToEnt(self, goal, false, false) > -1)
				 &&	(self->goalentity = PathToEnt_Node))
			 ||	((self->goalentity = NULL) && false)))		// couldn't find a path, so clear goalentity
	{	// going for an item, so follow the set path

		VectorSubtract(self->goalentity->s.origin, self->s.origin, dir);
		if (self->goalentity->node_type == NODE_PLAT)	// could be riding the plat
			dir[2] = 0;
		vectoangles(dir, angle);

		if (VectorLength(dir) <= (dist))
		{
			dist = VectorLength(dir);
			goal_in_range = true;
		}

		// do the actual movement
		bot_move(self, dist);

		if (self->goalentity)	// goalentity could have been cleared during bot_move()
		{
			if (goal_in_range)
			{	// see if we've reached the marker
				if (bot_ReachedTrail(self))
				{
//f					botDebugPrint("reached goal (%i)\n", self->goalentity->trail_index);
					if ( !( (PathToEnt(self->goalentity, goal, false, false) > -1) && (self->goalentity = PathToEnt_Node)))
					{
						self->goalentity = NULL;
					//f	botDebugPrint("Couldn't find next goal, roaming\n");
						bot_roam(self, false);
					}
					else
					{
//f						botDebugPrint("heading for goal (%i)\n", self->goalentity->trail_index);

						// if goal is plat, and it's no ready, wait here for a bit
						if (	(self->goalentity->node_type == NODE_PLAT)
							&&	(self->goalentity->target_ent->moveinfo.state != STATE_BOTTOM))
						{
							// stay here for a bit
							self->bot_plat_pausetime = level.time + 0.5;

							// move backwards
							M_walkmove(self, self->s.angles[1] + 180, 10);
						}

						// use grapple to get there faster?//FIXME ACRID

						else if (		(bot_tarzan->value)/*(grapple->value || ctf->value)*///acrid coed and added tarzan
									&&	(bot_tarzan->value || (goal->item && goal->item->pickup == CTFPickup_Flag) /*CarryingFlag(self)*/)
									&&	((self->waterlevel > 2) || (self->last_enemy_sight < (level.time - 1)) || !self->enemy || (entdist(self, self->enemy) > 512))
									&&	(PathToEnt(self->goalentity, goal, false, false) > -1))
						{
							static vec3_t goaldir, nextgoaldir, diffdir;


							VectorSubtract(self->goalentity->s.origin, self->s.origin, goaldir);
							VectorSubtract(PathToEnt_Node->s.origin, self->s.origin, nextgoaldir);

							if (VectorLength(nextgoaldir) > 300)
							{
								VectorNormalize(goaldir);
								VectorNormalize(nextgoaldir);

								VectorSubtract(nextgoaldir, goaldir, diffdir);

								if (VectorLength(diffdir) < 0.4)
								{
									trace_t tr;

									VectorScale(nextgoaldir, 1024, diffdir);
									VectorAdd(self->s.origin, diffdir, diffdir);

									tr = gi.trace(self->s.origin, NULL, NULL, diffdir, self, MASK_PLAYERSOLID);

									if ((tr.fraction < 1) && (tr.fraction > 0.4) && (tr.ent == world))
									{	// fire away
										vectoangles(nextgoaldir, self->client->v_angle);
										CTFGrappleFire (self, vec3_origin, 10, 0);
										// set visible model vwep with 3.20 code
										ShowGun(self);
									}
								}
							}
						}
					}
				}
			}
		}
		else	// goalentity has been whiped
		{
			return;
		}

		if (self->movetarget && !self->movetarget->item && (self->last_roam_time < (level.time - 3)))
		{	// walking around aimlessly, check for new targets every 3 seconds
			self->movetarget = NULL;
			bot_roam(self, false);
		}
	}
	else
	{
		bot_move(self, dist);	// just go anywhere
	}

	// *********************************************************

	if ((self->flags & FL_SHOWPATH) && (goal))
	{	// show path to goal
		Debug_ShowPathToGoal(self, goal);
	}
};

float	bot_ReachedTrail(edict_t *self)
{
	vec3_t	vec;
	int		retval = false;

	VectorSubtract(self->s.origin, self->goalentity->s.origin, vec);

	if (retval = (abs(vec[2]) < 16))
	{
		vec[2] = 0;
		retval = (VectorLength(vec) < 12);
	}

	// check if this is a plat node, and the plat is not responding
	if (self->goalentity->node_type == NODE_PLAT)
	{
		if (	(!self->goalentity->target_ent)
			||	(	(self->groundentity != self->goalentity->target_ent)			// not standing on the platform
				 &&	(self->goalentity->target_ent->moveinfo.state != STATE_BOTTOM)))	// and it's not waiting
		{	// give up searching for this ent
			edict_t	*goal = NULL;

			botDebugPrint("Aborting plat node\n");

			if (self->movetarget)
				goal = self->movetarget;
			else if (self->enemy)
				goal = self->enemy;

			// make sure nothing else searches for this ent for a while
			if (goal)
			{
				goal->ignore_time = level.time + 0.5;
				goal->enemy = self;
			}

			self->goalentity->ignore_time = level.time + 0.2;
			self->movetarget = self->enemy = NULL;

			bot_roam(self, false);

			return false;
		}
		else if (	(self->groundentity == self->goalentity->target_ent)
				 &&	(self->goalentity->target_ent->moveinfo.state == STATE_TOP))
		{	// standing on the platform, and it's reached the top
			return true;
		}

	}

	if (retval || (self->client->reached_goal_time == level.time))
	{	// ignore the current node for a bit
		if ((self->goalentity->enemy != self) || (self->goalentity->ignore_time < (level.time - 3)))
			self->last_reached_trail = level.time;

		self->goalentity->enemy = self;
		self->goalentity->ignore_time = level.time + 0.2;

		// standing/crouching
		if (self->goalentity->routes)
		{
			if (self->maxs[2] != self->goalentity->maxs[2])
			{
				if (self->maxs[2] < 32)
				{	// try to stand
					if (CanStand(self))
						self->maxs[2] = self->goalentity->maxs[2];
				}
				else	// duck
				{
					self->maxs[2] = self->goalentity->maxs[2];
				}
			}
		}

		self->client->reached_goal_time = 0;
		retval = true;
	}

	return retval;
}

extern qboolean touched_player;

// Simulated movement physics, adapted from player-like movement
int		bot_move(edict_t *self, float dist)
{
	trace_t	trace;
	vec3_t	dir, vec1/*, vec2*/, dest, dropdest; //, save_vel, save2;
	usercmd_t	ucmd;		// fake ucmd for client emulation
	vec3_t		angles, oldorg, move;
	int			ret, got_dir=false;
	qboolean	going_for_goalentity=false;
	edict_t		*last_groundentity;
	float		save_nocloser;
	int			start_waterlevel;


	//botfreeze 3/99
	if (level.time < self->frozentime)
		return false;


	// only restore last_move_nocloser if we don't move any closer
	save_nocloser = self->last_move_nocloser;
	self->last_move_nocloser = level.time;
/*
	// make sure we crouch when we have to
	if (self->goalentity && !self->goalentity->item && (self->goalentity->maxs[2] < 32))
	{						// don't set maxs to that of an item
		self->maxs[2] = self->goalentity->maxs[2];
	}
	else if (self->movetarget && (self->goalentity == self->movetarget) && self->movetarget->movetarget
			 && (self->movetarget->movetarget->maxs[2] < 32))
	{
		self->maxs[2] = self->movetarget->movetarget->maxs[2];
	}
*/

// BEGIN, SABIN code: prepare the ucmd for client simulation, but we have to clear the structure manually, since we don't have actual client's do it for us
	// delta_angles must be manually cleared
	VectorClear (self->client->ps.pmove.delta_angles);	// otherwise the actual facing direction may be modified prior to movement

	memset (&ucmd, 0, sizeof (ucmd));
// END, SABIN code

	ucmd.msec = 100;		// bot's think every 100 ms
//acrid fixme look here for speed
	if (dist < BOT_RUN_SPEED*FRAMETIME)
		ucmd.forwardmove = sv_maxvelocity->value * 0.5;	// walking speed
	else
	{
		if ((self->waterlevel > 2) || (!self->groundentity && 
			(self->waterlevel > 0)))
			ucmd.forwardmove = sv_maxvelocity->value * 0.6;	// go slower if swimming
		else
			ucmd.forwardmove = sv_maxvelocity->value;
	}

	if (self->waterlevel && ((self->air_finished - 2.5) < level.time)
		&&	(	!self->movetarget || !self->movetarget->item
||	(entdist(self, self->movetarget) > (384 * ((self->movetarget->item->pickup == CTFPickup_Flag) + 1)))))	// go for air!
	{
		// can we get to air?
		VectorCopy(self->s.origin, dest);
		dest[2] += 512;

		trace = gi.trace(self->s.origin, NULL, NULL, dest, self, MASK_PLAYERSOLID);

		VectorCopy(trace.endpos, dest);
		dest[2] -= 1;

		if (botTouchingLadder(self) || !(gi.pointcontents(dest) & MASK_WATER))
			ucmd.upmove = sv_maxvelocity->value;
	}
	else if (!self->waterlevel && self->goalentity && 
(self->goalentity->node_type == NODE_LANDING) && 
(self->goalentity->s.origin[2] > self->s.origin[2]+64))
	{//the jumping fixme
		ucmd.upmove = sv_maxvelocity->value;
	}
	else if (!self->groundentity && (!self->waterlevel))// && self->goalentity && ((self->goalentity->s.origin[2]+32) > self->s.origin[2]))
	{
		ucmd.upmove = sv_maxvelocity->value;
	}
	else if (!self->groundentity && !self->waterlevel && self->goalentity)
	{	// ladder?
		vec3_t vec;

		VectorSubtract(self->s.origin, self->goalentity->s.origin, vec);
		vec[2] = 0;

		if ((self->goalentity->s.origin[2] > self->s.origin[2]) && (VectorLength(vec) < 128))
			ucmd.upmove = sv_maxvelocity->value;
	}


	VectorCopy(self->s.origin, oldorg);
	VectorCopy(self->s.angles, angles);

	if (self->avoid_ent && !self->avoid_ent->inuse)
		self->avoid_ent = NULL;

	if (self->groundentity || self->waterlevel)
	{
		// find walking direction
		if (self->avoid_ent)
		{
			if (self->avoid_dir_time > level.time)
			{
				VectorCopy(self->avoid_dir, dir);
				got_dir = true;
			}	
			// find a direction away from danger
			else if (!(ret = botJumpAvoidEnt(self, self->avoid_ent)))
			{
				// move away from danger
				VectorSubtract(self->s.origin, self->avoid_ent->s.origin, dir);
				self->avoid_dir_time = level.time + 0.5;
				got_dir = true;
			}
			else if (ret == 1)	// successful jump from danger
			{
				if (self->groundentity)
				{
					VectorCopy(self->avoid_dir, dir);
					got_dir = true;
				}
				return true;
			}
		}

		if (!got_dir)
		{
			if (self->goalentity && !self->activator)
			{
				VectorSubtract(self->goalentity->s.origin, self->s.origin, dir);

				if (VectorLength(dir) < 32)
				{	// move straight to the goalentity
					VectorCopy(dir, self->velocity);

					// convert to "per second"
					VectorScale(self->velocity, 10, self->velocity);
				}

				VectorNormalize2(dir, vec1);
				going_for_goalentity = true;
			}
			else
			{
				VectorClear(dest);
				dest[1] = self->ideal_yaw;
				AngleVectors(dest, dir, NULL, NULL);
			}
		}

//		dir[2] = 0;

		VectorNormalize2(dir, dir);
		vectoangles(dir, angles);
	}
	else if (!self->client->ctf_grapple)
	{
		if (self->velocity[2] > 310)
			self->velocity[2] = 310;

		// have we gone passed the goalentity? if so slow down velocity
		if (self->goalentity && (fabs(self->s.origin[2] - self->goalentity->s.origin[2]) > 16))
		{
			VectorCopy(self->velocity, dir);
			dir[2] = 0;

			if (VectorLength(dir) > 8)
			{
				VectorNormalize(dir);
				VectorSubtract(self->goalentity->s.origin, self->s.origin, dest);
				dest[2]=0;

				VectorNormalize2(dest, vec1);

				VectorSubtract(dir, vec1, dir);

				if ((VectorLength(dest) < 64) && (VectorLength(dir) > 1.5))
				{	// slow down!
					self->velocity[0] = self->jump_velocity[0] = 0;
					self->velocity[1] = self->jump_velocity[1] = 0;
				}
			}
		}

	}

// BEGIN, SABIN code: convert angles to 16-bit int's, and pass them into the ucmd for simulation
	ucmd.angles[PITCH] = ANGLE2SHORT(angles[PITCH]);
	ucmd.angles[YAW] = ANGLE2SHORT(angles[YAW]);
	ucmd.angles[ROLL] = ANGLE2SHORT(angles[ROLL]);
// END, SABIN code

	last_groundentity = self->groundentity;
	start_waterlevel  = self->waterlevel;

	BotMoveThink(self, &ucmd);

	VectorSubtract(self->s.origin, oldorg, move);

	if ((!start_waterlevel) && !self->groundentity && last_groundentity)
	{
		float drop_dist=400;

		VectorCopy(self->velocity, self->jump_velocity);	// just to be safe

// if our goal is close by, and is slightly above, jump//fixme???
		if (self->goalentity /*&& (entdist(self, self->goalentity) < 256)*/ 
			&& (self->s.origin[2] < (self->goalentity->s.origin[2] /*- 48*/)))
		{
			if (self->s.origin[2] < (self->goalentity->s.origin[2]+32))	// only jump if goal is above us
			{
				// don't jump if ground is closeby
				trace_t tr;

				VectorCopy(self->s.origin, dropdest);
				dropdest[2] -= 20;

				tr = gi.trace(self->s.origin, self->mins, self->maxs, dropdest, self, MASK_PLAYERSOLID | CONTENTS_LAVA | CONTENTS_SLIME);
//lavacode
				if ((tr.fraction == 1) || (tr.plane.normal[2] < 0.5) || (tr.contents && (CONTENTS_LAVA | CONTENTS_SLIME)))
				{	// jump to be safe

					VectorSubtract(self->goalentity->s.origin, self->s.origin, self->velocity);
					VectorScale( self->velocity, 1.5, self->velocity);
					self->velocity[2] = 0;

					if (VectorLength(self->velocity) > 300)
					{
						VectorNormalize(self->velocity);
						VectorScale(self->velocity, 300, self->velocity);
					}

					self->velocity[2] = 310;
					VectorCopy(self->velocity, self->jump_velocity);

					gi.sound(self, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, 2, 0);
				}
			}
		}
		else
		{

			if (self->goalentity)
			{
				if (self->goalentity->s.origin[2]+64 < self->s.origin[2])
					drop_dist= 200 + (-1 * (self->goalentity->s.origin[2] - self->s.origin[2]));
				else if (self->goalentity->s.origin[2] > self->s.origin[2])
					drop_dist = 22;
			}
//lavacode
		// if monster had the ground pulled out, go ahead and fall, but only if safe (No lava or slime)
			// trace downwards, to see if it's dangerous
			VectorSubtract(self->s.origin, tv(0,0,drop_dist), dropdest);
			VectorMA(dropdest, 0.3 * (drop_dist/400), self->velocity, dropdest);
			trace = gi.trace(self->s.origin, VEC_ORIGIN, VEC_ORIGIN, dropdest, self, MASK_SOLID | MASK_WATER);
			
			VectorCopy(trace.endpos, dest);
			dest[2] += 1;
//lavacode
			if (	(	(	(trace.fraction == 1) 
						 ||	(self->goalentity && (self->goalentity->s.origin[2]-64 > trace.endpos[2]))) 
					 &&	(!self->enemy || (self->enemy->s.origin[2] < self->s.origin[2]))) // don't follow a player too far down
				|| (trace.contents) & (CONTENTS_LAVA | CONTENTS_SLIME))	// falling into lava
	//			|| (self->goalentity && (trace.fraction > 0.4) && (fabs(self->s.origin[2] - self->goalentity->s.origin[2]) < 8)) )	// we shouldn't be falling here
			{	// lava below, see if we can safely proceed
				VectorCopy(self->velocity, dropdest);
				dropdest[2] = -64;
				VectorAdd(self->s.origin, dropdest, dropdest);

				trace = gi.trace(self->s.origin, self->mins, self->maxs, dropdest, self, MASK_SOLID | CONTENTS_LAVA | CONTENTS_SLIME);

				if (	(drop_dist < 32)
					||	(trace.fraction == 1)
					||	(trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
					||	(trace.plane.normal[2] < 0.5))	// too steep
				{

					// abort the current path, so we can look for an alternate path next frame
					if (self->goalentity)
						self->goalentity->ignore_time = level.time + 0.5;
					self->goalentity = NULL;
					self->groundentity = last_groundentity;
					VectorCopy(oldorg, self->s.origin);
					gi.linkentity(self);

				}
			}

			else if ((self->velocity[2] <= 0) && self->goalentity && (self->goalentity->s.origin[2] < self->s.origin[2]))
			{	// keep on ground when going down stairs or ramp
				vec3_t	dest;

				VectorCopy(self->s.origin, dest);
				dest[2] -= 32;

				trace = gi.trace(self->s.origin, self->mins, self->maxs, dest, self, MASK_PLAYERSOLID);

				// only really inair if way above ground
				if (trace.fraction < 1)
				{
					VectorCopy(trace.endpos, self->s.origin);
					self->groundentity = trace.ent;
					gi.linkentity(self);
				}
			}

		}

	}
	else
	{
		if (self->groundentity && !last_groundentity)
		{	// just landed, let ClosestNodeToEnt check for a better node
			self->last_closest_time = 0;
		}
		else if (!self->groundentity && !last_groundentity && (self->velocity[2] > 0) /*&& (!self->goalentity || (self->goalentity->s.origin[2] > self->s.origin[2]))*/)
		{	// prevent not jumping high enough when scraping a wall/ledge
			float	f;

			f = move[2];
			move[2] = 0;

			if (VectorLength(move) < 5)
			{
				self->velocity[2] += 13;
			}

			move[2] = f;
		}

		if (self->groundentity && (VectorLength(move) < dist*0.1))
		{	// try walkmove()

			if (!M_walkmove(self, angles[YAW], dist))
			{
//				if (!M_walkmove(self, angles[YAW], dist*0.3))
//					if (!M_walkmove(self, angles[YAW+100], dist*0.5))
//						M_walkmove(self, angles[YAW-100], dist*0.5);
			}

			VectorSubtract(self->s.origin, oldorg, move);
		}

		if ((self->groundentity) && (VectorLength(move) < dist*0.1))
		{

			if (self->goalentity /*&& self->groundentity*/)
			{
//				if ((random() < 0.7)) // && (self->s.origin[2] + 16 < self->goalentity->s.origin[2]))
				{
					vec3_t start, dir, org, mins;

					VectorCopy(self->s.origin, start);
//					start[2] -= 12;

					AngleVectors(angles, dir, NULL, NULL);
					VectorScale(dir, 24, dir);
					VectorAdd(start, dir, org);

					VectorCopy(self->mins, mins);
					mins[2] += 16;

					trace = gi.trace(start, mins, self->maxs, org, self, MASK_SOLID);

					if ((trace.fraction < 0.3) && (fabs(trace.plane.normal[2]) < 0.3))
					{
						// set ideal velocity
//						VectorScale(trace.plane.normal, -40, dir);
						dir[2] = 310;

						// see if clear at head
						start[2] += 32;
						org[2] += 32;

						trace = gi.trace(start, VEC_ORIGIN, VEC_ORIGIN, org, self, MASK_SOLID);

						if (trace.fraction == 1)
						{
							if (CanJump(self))
							{	// safe to jump
//								self->s.origin[2] += 1;

								self->groundentity = NULL;

								gi.linkentity(self);

								VectorCopy(dir, self->velocity);
								VectorCopy(self->velocity, self->jump_velocity);

								gi.sound(self, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, 2, 0);
							}
						}
						else	// blocked at head also, abort goal
						{
							self->goalentity = NULL;
						}
					}
				}
			}

//			if (touched_player)
			if (self->groundentity)
			{	// stuck, try and strafe
				int dir=1;

				if ((((int)(level.time)) % 6) < 3)
					dir = -1;

				if (!M_walkmove(self, 
								self->s.angles[YAW] - 110 * dir,
								BOT_STRAFE_SPEED * FRAMETIME * 0.5))
				{	// try the other way
					M_walkmove(	self, 
								self->s.angles[YAW] + 110 * dir,
								BOT_STRAFE_SPEED * FRAMETIME * 0.5);
				}

				self->client->slide_time = level.time + 0.5;
			}
		}
		else if (self->client->slide_time > level.time)
		{
			int dir=1;

			if ((((int)(level.time)) % 6) < 3)
				dir = -1;

			M_walkmove(	self, 
						self->s.angles[YAW] - 110 * dir,
						BOT_STRAFE_SPEED * FRAMETIME );

		}

		if (self->groundentity && (VectorLength(move) < dist*0.1))
		{	// try walkmove()

			M_walkmove(self, angles[YAW] + 180 * (random()*2 - 1), dist*0.5);

			VectorSubtract(self->s.origin, oldorg, move);
		}
	}

	if (going_for_goalentity && self->goalentity /*&& !self->goalentity->item*/)
	{
		vec3_t	goalvec, oldgoalvec;
		vec3_t	u_goalvec, u_oldgoalvec;
		float	vdist;

		VectorSubtract(self->s.origin, self->goalentity->s.origin, goalvec);
		VectorSubtract(oldorg,         self->goalentity->s.origin, oldgoalvec);

		if ((vdist = VectorLength(goalvec)) < 40)
		{	// see if we can move to the goal

			if (vdist > 12)
			{
				VectorNormalize2(goalvec, u_goalvec);
				VectorNormalize2(oldgoalvec, u_oldgoalvec);

				VectorSubtract(u_goalvec, u_oldgoalvec, dir);
			}

			if ((vdist <= 12) || (VectorLength(dir) > 0.4))
			{	// close enough, and we've passed the goal, so try to move to it
//				trace_t	trace;

				self->client->reached_goal_time = level.time;

				// if this is a jump node, move to it
				if (self->goalentity->goalentity)
				{
					trace = gi.trace (self->goalentity->s.origin, self->goalentity->mins, self->goalentity->maxs, self->goalentity->s.origin, self, MASK_PLAYERSOLID);

					if (!trace.startsolid)
					{
						VectorCopy(self->goalentity->s.origin, self->s.origin);
						gi.linkentity(self);
					}
				}

			}
		}
		else if (self->groundentity || self->waterlevel)	// make sure we keep heading towards the goal
		{
			if (VectorLength(goalvec) >= VectorLength(oldgoalvec))
			{	// we haven't moved any closer, something's wrong
				self->last_move_nocloser = save_nocloser;

				if (self->last_move_nocloser < (level.time - 0.3))
				{
					botDebugPrint("%s didn't move closer to goalentity\n", self->client->pers.netname);

					self->goalentity->ignore_time = level.time + 1;
					self->goalentity->enemy = self;
					self->goalentity = NULL;
					self->last_move_nocloser = level.time;
				}
			}
		}

	}

	return true;
}

int bot_BetterTarget (edict_t *self, edict_t *other)
{ 
	botDebugPrint(" class other %s\n",other->classname);
	botDebugPrint(" self team %d\n",self->wf_team);
	botDebugPrint(" other team %d\n",other->wf_team);
	//if attacker is not a sentry or turret grenade dont attack
/*	if (!other->client && ((strcmp(other->classname, "SentryGun") != 0)
		||(strcmp(other->classname, "turret") != 0)))
	{	botDebugPrint("false not sentry or turret\n",other->classname);
		return false;
	}*/
return false;
	// attack sentry if not same team
	if ((strcmp(other->classname, "SentryGun") == 0) &&
		(other->wf_team != self->wf_team))
	{	botDebugPrint(" true\n",other->classname);
		return true;
	}
    // don't attack same team sentry if it accidently hits bot
	if ((strcmp(other->classname, "SentryGun") == 0) && 
		(other->wf_team == self->wf_team))
	{
		return false;
	}

	if ((strcmp(other->classname, "turret") == 0)&&
	    (other->wf_team != self->wf_team))
		return true;

	if ((strcmp(other->classname, "turret") == 0)&&
	    (other->wf_team == self->wf_team))
		return false;
/*if((self->client->player_class == 2) &&
(self->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))] > 0))
	{	if (SameTeam(other, self) && (other->disease))
	        return true;
	}*/

	if (SameTeam(other, self))
		return false;

	if (ctf->value)
	{
		if (self->enemy && CarryingFlag(self->enemy))
			return false;
		if (CarryingFlag(other))
			return true;
	}

	// FIXME: weigh up other's weapon against their health?

	if ((self->enemy) && (entdist(self, self->enemy) < 512))
		return (other->health < self->enemy->health);
	else
		return (other->health > 0);
}

void bot_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	int	r, l;
//	botDebugPrint(" class 2222 %s\n",other->classname);
//	botDebugPrint(" name 2222 %1\n",other->health);

	self->last_pain = level.time;
//lavacode
	if (	(!ctf->value)
		&&	(self->client->team)
		&&	(other->client)		// ignore if hurt by door or lava
		&&	(self->client->team->last_grouping < (level.time - 5))
		&&	(last_bot_chat[CHAT_TEAMPLAY_HELP] < (level.time - 5))
		&&	(self->client->team != other->client->team)
		&&	(!self->target_ent)
		&&	((self->bot_fire == botBlaster) || (self->bot_fire == botShotgun))
		&&	(self->health < other->health))
	{	// signal for help!
		botDebugPrint("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n");
		TeamGroup(self);
	}

	if (bot_BetterTarget(self,other))
	{
		// get mad at this person
		self->enemy = other;
	}
/*
	if ((self->health < 20) && (self->bot_stats->aggr < random() * 5))
	{	// health required!
//		self->movetarget = NULL;

		bot_roam(self, false);
	}
*/
	// play an apropriate pain sound
	if (level.time > self->pain_debounce_time)
	{
		int i=0;

		r = 1 + (rand()&1);
		self->pain_debounce_time = level.time + 0.7;
		if (self->health < 25)
			l = 25;
		else if (self->health < 50)
			l = 50;
		else if (self->health < 75)
			l = 75;
		else
			l = 100;

		gi.sound (self, CHAN_VOICE, gi.soundindex(va("*pain%i_%i.wav", l, r)), 1, ATTN_NORM, 0);
        if(!self->frozen)//botfreeze 
		if (self->client->anim_priority < ANIM_PAIN)
		{
			self->client->anim_priority = ANIM_PAIN;

			if (self->maxs[2] == 4)
			{
				self->s.frame = FRAME_crpain1;
				self->client->anim_end = FRAME_crpain4;
			}
			else
			{
				i = (i+1)%3;
				switch (i)
				{
				case 0:
					self->s.frame = FRAME_pain101;
					self->client->anim_end = FRAME_pain104;
					break;
				case 1:
					self->s.frame = FRAME_pain201;
					self->client->anim_end = FRAME_pain204;
					break;
				case 2:
					self->s.frame = FRAME_pain301;
					self->client->anim_end = FRAME_pain304;
					break;
				}
			}
		}
	}

}

//========================================================================================
//
// Miscellaneous routines
//

// Checks if the entity can be moved upwards for jumping
int	CanJump(edict_t *ent)
{
	trace_t	trace;
	vec3_t	dest;

	VectorCopy(ent->s.origin, dest);
	dest[2] += 1;

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_PLAYERSOLID);

	return (!trace.startsolid && (trace.fraction == 1));
}

// check that the targ is visible from self (restricted to FOV)
int		CanSee(edict_t *self, edict_t *targ)
{
	vec3_t	dir, forward, vec;

	VectorSubtract(targ->s.origin, self->s.origin, dir);

	if (VectorLength(dir) < 256)	// close enough
		return true;

//	dir[2] = 0;
	VectorNormalize2(dir, dir);

	AngleVectors(self->s.angles, forward, NULL, NULL);

	VectorSubtract(forward, dir, vec);

	return (VectorLength(vec) < (1 + (self->bot_stats->combat / 5)));
}

// check that the path from self to targ is valid (doesn't walk on air)
int		CanReach(edict_t *self, edict_t *targ)
{
	vec3_t	dir, midpos, end_trace, start;
	vec3_t	midpos2, mins;
	trace_t	trace;
	float	dist, progressive_dist;
	int		inc=32;

	// special water case (since tracing for WATER doesn't work
	if (	(self->waterlevel && targ->waterlevel)
		||	(	(targ->waterlevel && (!targ->groundentity || (targ->waterlevel > 2))) 
			 &&	((self->s.origin[2]+16) > targ->s.origin[2])))
		return true;

	VectorAdd(self->mins, tv(0,0,12), mins);
	VectorCopy(self->s.origin, start);

	VectorSubtract(targ->s.origin, self->s.origin, dir);
	VectorMA(start, 0.5, dir, midpos);
	VectorSubtract(midpos, tv(0,0,40), end_trace);

	// check that the midpos is onground (down 28 units)
	trace = gi.trace(midpos, mins, self->maxs, end_trace, self, MASK_SOLID);

	if (trace.fraction == 1)
		return false;

	if ((dist = VectorLength(dir)) < 32)
		return true;

	VectorNormalize2(dir, dir);

	if (!bot_calc_nodes->value || (self->bot_client))	// only do thorough checking when laying nodes
	{
		return true;
//		if (dist > 66)
//			dist = 66;
	}
	else
	{
		inc = 12;		// do more thourough checking
	}

	for (progressive_dist = 32; progressive_dist < (dist - 16) ; progressive_dist += inc)
	{
		VectorMA(start, progressive_dist, dir, midpos2);
		VectorSubtract(midpos2, tv(0,0,28), end_trace);

		trace = gi.trace(midpos2, mins, self->maxs, end_trace, self, MASK_SOLID);

		if (trace.fraction == 1)
			return false;
	}

	return true;
}

int CanStand(edict_t	*self)
{
	static vec3_t	maxs = {16, 16, 32};
	trace_t trace;

	trace = gi.trace(self->s.origin, self->mins, maxs, self->s.origin, self, MASK_PLAYERSOLID);

	return (!trace.startsolid || ((trace.ent->svflags & SVF_MONSTER) && (trace.ent->health <= 0)));
}

//========================================================================================

void bot_SuicideIfStuck(edict_t *self)
{
	if ((self->checkstuck_time < (level.time - 1)) && (self->group_pausetime < (level.time - 1)) && (self->bot_plat_pausetime < (level.time - 1)))
	{
		vec3_t	move;

		VectorSubtract(self->s.origin, self->checkstuck_origin, move);

		if (VectorLength(move) < 4)
		{	// bot is stuck
			if (self->checkstuck_time < (level.time - 5))
			{	// suicide after long enough
				botDebugPrint("Bot suicide (stuck)\n");
				T_Damage(self, self, self, tv(0,0,0), self->s.origin, tv(0,0,0), self->health + 1, 0, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
			}
			else
			{	// jump randomly
				botDebugPrint("BotStuck: Random jump\n");
				botRandomJump(self);
			}

			self->checkstuck_time = level.time;
			return;
		}
		else
		{
			VectorCopy(self->s.origin, self->checkstuck_origin);
			self->checkstuck_time = level.time;
		}
	}
}

void botButtonThink(edict_t *ent)
{
	static vec3_t	mins = {-16, -16, -8};
	int i, count=0;
//	trace_t tr;
	edict_t	*plyr;
	float	dist;

	if (ent->skill_level > num_players)
		ent->skill_level = 0;

	for (; ent->skill_level<num_players; ent->skill_level++)
	{
		i = ent->skill_level;
		plyr = players[i];

		if (count++ > 6)
			break;

		if (!plyr->bot_client || plyr->activator)
			continue;

		if (plyr->activator_time > level.time)
			continue;

		if (entdist(ent, plyr) > 900)
			continue;

		count += 2;	// so we don't do too many PathToEnt()'s per think

		if ((dist = PathToEnt(ent->owner, ent, false, false)) == -1)
			continue;

		if (dist > 1200)
			continue;

		// push me!
		plyr->activator = ent;
		plyr->goalentity = PathToEnt_Node;
		plyr->activator_time = level.time;
		plyr->last_reached_trail = level.time;

		ent->nextthink = level.time + 4;	// wait a bit longer than usual

		break;
	}

	ent->nextthink = level.time + 0.2 + random()*0.3;		// so multiple buttons don't all go at once
}
