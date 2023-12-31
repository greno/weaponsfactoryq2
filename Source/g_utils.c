// g_utils.c -- misc utility functions for game module

#include "g_local.h"
void laserball_cleanup (edict_t *ent);//WF34


void G_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}


/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
edict_t *G_Find (edict_t *from, int fieldofs, char *match)
{
	char	*s;

	if (!from)
		from = g_edicts;
	else
		from++;

	for ( ; from < &g_edicts[globals.num_edicts] ; from++)
	{
		if (!from->inuse)
			continue;
		s = *(char **) ((byte *)from + fieldofs);
		if (!s)
			continue;
		if (!Q_stricmp (s, match))
			return from;
	}

	return NULL;
}


/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
edict_t *findradius (edict_t *from, vec3_t org, float rad)
{
	vec3_t	eorg;
	int		j;
//WF34 S ++TeT
	float	vecLength;
	float	radSquared = rad * rad;
//WF34 E --TeT

	if (!from)
		from = g_edicts;
	else
		from++;
	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j])*0.5);
//WF34 S ++TeT - inlined vectorLength func which returned sqrt of vecLength,
//         instead we compare it to the square of rad
		vecLength = 0;
		vecLength += eorg[0]*eorg[0];
		vecLength += eorg[1]*eorg[1];
		vecLength += eorg[2]*eorg[2];
		if (vecLength > radSquared)
			continue;
// --TeT
		return from;
	}

	return NULL;
}

/*
=================
findEnemyWithinRadius

Returns entities that have origins within a spherical area

findEnemyWithinRadius (origin, radius)
=================
*/
edict_t *findEnemyWithinRadius (edict_t *self, edict_t *from, vec3_t org, float rad)
{
	vec3_t	eorg;
	int		j;
//WF34 S ++TeT
	float	vecLength;
	float	radSquared = rad * rad;
//WF34 E --TeT

	if (!from)
		from = g_edicts;
	else
		from++;
	for ( ; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
        if (!(from->svflags & SVF_MONSTER) && !from->client)
            continue;
 	    //dont attack same team
		if (from->wf_team == self->wf_team)
		    continue;
		if (from->disguised)
			continue;
        if (!from->takedamage)
            continue;
        if (from->health <= 0)
            continue;
        if (!visible(self, from))
            continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j])*0.5);
//WF34 S ++TeT - inlined vectorLength func which returned sqrt of vecLength,
//         instead we compare it to the square of rad
		vecLength = 0;
		vecLength += eorg[0]*eorg[0];
		vecLength += eorg[1]*eorg[1];
		vecLength += eorg[2]*eorg[2];
		if (vecLength > radSquared)
			continue;
// --TeT
		return from;
	}

	return NULL;
}

/*
=============
G_PickTarget

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the edict after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
#define MAXCHOICES	8

edict_t *G_PickTarget (char *targetname)
{
	edict_t	*ent = NULL;
	int		num_choices = 0;
	edict_t	*choice[MAXCHOICES];

	if (!targetname)
	{
		gi.dprintf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while(1)
	{
		ent = G_Find (ent, FOFS(targetname), targetname);
		if (!ent)
			break;
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices)
	{
		gi.dprintf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}



void Think_Delay (edict_t *ent)
{
	G_UseTargets (ent, ent->activator);
	G_FreeEdict (ent);
}

/*
==============================
G_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Centerprints any self.message to the activator.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets (edict_t *ent, edict_t *activator)
{
	edict_t		*t;

//
// check for a delay
//WF34 S
//if (wfdebug) gi.dprintf("G_UseTargets:1. Ent = %s, Activator=%s\n",
//						ent->classname, activator->classname);
//WF34 E
	if (ent->delay)
	{
	// create a temp object to fire at a later time
		t = G_Spawn();
		t->classname = "DelayedUse";
		t->nextthink = level.time + ent->delay;
		t->think = Think_Delay;
		t->activator = activator;
		if (!activator)
			gi.dprintf ("Think_Delay with no activator\n");
		t->message = ent->message;
		t->target = ent->target;
		t->killtarget = ent->killtarget;
		return;
	}


//
// print the message
//
	if ((ent->message) && !(activator->svflags & SVF_MONSTER))

	{
		gi.centerprintf (activator, "%s", ent->message);

		if (ent->noise_index)

			gi.sound (activator, CHAN_AUTO, ent->noise_index, 1, ATTN_NORM, 0);

		else

			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);

	}

//
// kill killtargets
//

	if (ent->killtarget)
	{
		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), ent->killtarget)))

		{

			G_FreeEdict (t);

			if (!ent->inuse)

			{

				gi.dprintf("entity was removed while using killtargets\n");

				return;

			}

		}
	}

//	gi.dprintf("TARGET: activating %s\n", ent->target);



//
// fire targets
//
//if (wfdebug) gi.dprintf("G_UseTargets:4\n");

	if (ent->target)
	{
		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), ent->target)))
		{
			// doors fire area portals in a specific way
			if (!Q_stricmp(t->classname, "func_areaportal") &&
				(!Q_stricmp(ent->classname, "func_door") || !Q_stricmp(ent->classname, "func_door_rotating")))
				continue;

			if (t == ent)
			{
				gi.dprintf ("WARNING: Entity used itself.\n");
			}
			else
			{//WF34S
//if (wfdebug) gi.dprintf("G_UseTargets: 4a-use. Target=%s\n", t->classname);
				if (t->use)
					t->use (t, ent, activator);
			}
			if (!ent->inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;

			}

		}
	}
//if (wfdebug) gi.dprintf("G_UseTargets:5\n");//WF34
}


/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float	*tv (float x, float y, float z)
{
	static	int		index;
	static	vec3_t	vecs[8];
	float	*v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1)&7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}


/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char	*vtos (vec3_t v)
{
	static	int		index;
	static	char	str[8][32];
	char	*s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = (index + 1)&7;

	Com_sprintf (s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2]);

	return s;
}


vec3_t VEC_UP		= {0, -1, 0};
vec3_t MOVEDIR_UP	= {0, 0, 1};
vec3_t VEC_DOWN		= {0, -2, 0};
vec3_t MOVEDIR_DOWN	= {0, 0, -1};

void G_SetMovedir (vec3_t angles, vec3_t movedir)
{
	if (VectorCompare (angles, VEC_UP))
	{
		VectorCopy (MOVEDIR_UP, movedir);
	}
	else if (VectorCompare (angles, VEC_DOWN))
	{
		VectorCopy (MOVEDIR_DOWN, movedir);
	}
	else
	{
		AngleVectors (angles, movedir, NULL, NULL);
	}

	VectorClear (angles);
}


float vectoyaw (vec3_t vec)
{
	float	yaw;

	if (/*vec[YAW] == 0 &&*/ vec[PITCH] == 0)
	{
		yaw = 0;
		if (vec[YAW] > 0)
			yaw = 90;
		else if (vec[YAW] < 0)
			yaw = -90;
	}
	else
	{
		yaw = (int) (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	return yaw;
}


void vectoangles (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		if (value1[0])
			yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = -90;
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (int) (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

char *G_CopyString (char *in)
{
	char	*out;

	out = gi.TagMalloc (strlen(in)+1, TAG_LEVEL);
	strcpy (out, in);
	return out;
}


void G_InitEdict (edict_t *e)
{
	e->inuse = true;
	e->classname = "noclass";
	e->gravity = 1.0;
	e->s.number = e - g_edicts;
//ERASER START
	e->trail_index = 0;
	e->closest_trail = 0;
	e->ignore_time = 0;
	e->last_reached_trail = 0;
//ERASER END
}

/*
=================
G_Spawn

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *G_Spawn (void)
{
	int			i;
	edict_t		*e;

	e = &g_edicts[(int)maxclients->value+1];
	for ( i=maxclients->value+1 ; i<globals.num_edicts ; i++, e++)
	{
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e->inuse && ( e->freetime < 2 || level.time - e->freetime > 0.5 ) )
		{
			G_InitEdict (e);
			return e;
		}
	}

	if (i == game.maxentities)
		gi.error ("ED_Alloc: no free edicts");

	globals.num_edicts++;
	G_InitEdict (e);
	return e;
}

/*
=================
G_FreeEdict

Marks the edict as free
=================
*/
void G_FreeEdict (edict_t *ed)
{

if (wfdebug)
 {
//	gi.dprintf("G_FreeEdict: %s\n", ed->classname);
 }
//WF34 S Check for entity limits
	if ((ed->grenade_index) && (ed->owner) && (ed->owner->client))
	{
		--ed->owner->client->pers.active_grenades[ed->grenade_index];
	}

	if (ed->special_index)	//if set, this is an index into the active_special array
	{
		--ed->owner->client->pers.active_special[ed->special_index];
	}

	//Special handling for other WF items

	//Sentry gun
	if (!strcmp(ed->classname, "SentryGun") )
	{
		if (ed->sentry) ed->sentry->health = 0;
		if (ed->creator) ed->creator->sentry = NULL;
	}

	//Supply depot
	else if (!strcmp(ed->classname, "depot") )
	{
		if (ed->owner) ed->owner->supply = NULL;
	}

	//Healing depot
	else if (!strcmp(ed->classname, "healingdepot") )
	{
		if (ed->owner) ed->owner->supply = NULL;
	}

	//Laserball
	else if (!strcmp(ed->classname, "laserball") )
	{
		laserball_cleanup(ed);
	}
//WF34 E
	gi.unlinkentity (ed);		// unlink from world

	if ((ed - g_edicts) <= (maxclients->value + BODY_QUEUE_SIZE))

	{

//		gi.dprintf("tried to free special edict\n");

		return;

	}



	memset (ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = false;
}


/*
============
G_TouchTriggers

============
*/
void	G_TouchTriggers (edict_t *ent)
{
	int			i, num;
	edict_t		*touch[MAX_EDICTS], *hit;


	// dead things don't activate triggers!

	if ((ent->client || (ent->svflags & SVF_MONSTER)) && (ent->health <= 0))

		return;


	num = gi.BoxEdicts (ent->absmin, ent->absmax, touch
		, MAX_EDICTS, AREA_TRIGGERS);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (!hit->touch)
			continue;
		hit->touch (hit, ent, NULL, NULL);
	}
}

/*
============
G_TouchSolids

Call after linking a new trigger in during gameplay
to force all entities it covers to immediately touch it
============
*/
void	G_TouchSolids (edict_t *ent)
{
	int			i, num;
	edict_t		*touch[MAX_EDICTS], *hit;

	num = gi.BoxEdicts (ent->absmin, ent->absmax, touch
		, MAX_EDICTS, AREA_SOLID);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (ent->touch)
			ent->touch (hit, ent, NULL, NULL);
		if (!ent->inuse)
			break;
	}
}


/*=================
 * KillBox
 *
 * Kills all entities that would touch the proposed new positioning
 * of ent.  Ent should be unlinked before calling this!
 * =================*/
qboolean KillBox (edict_t *ent)
{
	trace_t		tr;
	int			i;//ERASER
	edict_t		*trav;//ERASER

	gi.unlinkentity(ent);//ERASER

	while (1)
	{
		tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, ent->s.origin, NULL, MASK_PLAYERSOLID);
		if (!tr.ent)
			break;

		//WF & K2:Begin
		// ++TeT
		if (!strcmp(tr.ent->classname, "SentryStand"))
		{
			tr.ent = tr.ent->creator;
			tr.ent->noteamdamage = false;
		}
		if (!strcmp(tr.ent->classname, "biosentry"))
		{
			tr.ent->noteamdamage = false;
		}

		// --TeT
		// ++TeT Removed Reverse Telefrag - If they weren't smart enough to get out of the way, kill em
		//Need to check for reverse telefrag condition for respawn protection
//		if(K2_IsProtected(tr.ent) && !K2_IsProtected(ent) )
//		{
//			//Reverse Telefrag
//			T_Damage (ent,tr.ent,tr.ent,vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_REVERSE_TELEFRAG);
//		}
		else
		//WF & K2:End
		// nail it
		// --TeT remove reverse telefrag
		T_Damage (tr.ent, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);

			// if we didn't kill it, fail
		if (tr.ent->solid)
		//ERASER START
		{
//			if (tr.ent == world)
				break;//ERASER

//			gi.dprintf("Couldn't KillBox(), something is wrong.\n");
//			return false;//ERASER ,WF USES
		}
	}

	// fixes wierd teleporter/spawning inside others
	i = -1;
	while (++i < num_players)
	{
		if ((trav = players[i]) == ent)
			continue;

		if (entdist(trav, ent) > 42)
			continue;

		// nail it
		T_Damage (trav, ent, ent, vec3_origin, ent->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
//ERASER END
		//??FIXME	return false;

	}



	return true;		// all clear

}
//ERASER START
// Ridah
/*
==================
stuffcmd

  QC equivalent, sends a command to the client's consol
==================
*//* ACRID
void stuffcmd(edict_t *ent, char *text)
{
	gi.WriteByte(11);				// 11 = svc_stufftext
	gi.WriteString(text);
	gi.unicast(ent, 1);
}*///ACRID

float	entdist(edict_t *ent1, edict_t *ent2)
{
	vec3_t	vec;

	VectorSubtract(ent1->s.origin, ent2->s.origin, vec);
	return VectorLength(vec);
}

/*
=================
AddModelSkin

  Adds a skin reference to an .md2 file, saving as filename.md2new
=================
*/
void AddModelSkin (char *modelfile, char *skinname)
{
	FILE	*f, *out;
	int		count = 0, buffer_int, i;
	char	filename[256], infilename[256];
	char	buffer;
//	botDebugPrint("ADDMODELSKIN (ACRID)\n");
	i = sprintf(infilename , modelfile);

	f = fopen (infilename, "rb");
	if (!f)
	{
		gi.dprintf("Cannot open file %s\n", infilename);
		return;
	}

	i =  sprintf(filename, ".\\");
	i += sprintf(filename + i, GAMEVERSION);
	i += sprintf(filename + i, "\\");
	i += sprintf(filename + i, modelfile);
	i += sprintf(filename + i, "new");

	out = fopen (filename, "wb");
	if (!out)
		return;

	// mirror header stuff before skinnum
	for (i=0; i<5; i++)
	{
		fread(&buffer_int, sizeof(buffer_int), 1, f);
		fwrite(&buffer_int, sizeof(buffer_int), 1, out);
	}

	// increment skinnum
	fread(&buffer_int, sizeof(buffer_int), 1, f);
	++buffer_int;
	fwrite(&buffer_int, sizeof(buffer_int), 1, out);

	// mirror header stuff before skin_ofs
	for (i=0; i<5; i++)
	{
		fread(&buffer_int, sizeof(buffer_int), 1, f);
		fwrite(&buffer_int, sizeof(buffer_int), 1, out);
	}

	// copy the skins offset value, since it doesn't change
	fread(&buffer_int, sizeof(buffer_int), 1, f);
	fwrite(&buffer_int, sizeof(buffer_int), 1, out);

	// increment all offsets by 64 to make way for new skin
	for (i=0; i<5; i++)
	{
		fread(&buffer_int, sizeof(buffer_int), 1, f);
		buffer_int += 64;
		fwrite(&buffer_int, sizeof(buffer_int), 1, out);
	}

	// write the new skin
	for (i=0; i<strlen(skinname); i++)
	{	//	botDebugPrint("SKIN K (ACRID)\n");
		fwrite(&(skinname[i]), 1, 1, out);
	}

	buffer = '\0';
	fwrite(&buffer, 1, 1, out);
	buffer = 'x';
	fwrite(&buffer, 1, 1, out);

	buffer = '\0';
	for (i=(strlen(skinname)+2); i<64; i++)
	{
		fwrite(&buffer, 1, 1, out);
	}

	// copy the rest of the file
	fread(&buffer, sizeof(buffer), 1, f);
	while (!feof(f))
	{
		fwrite(&buffer, sizeof(buffer), 1, out);
		fread(&buffer, sizeof(buffer), 1, f);
	}

	fclose (f);
	fclose (out);

	// copy the new file over the old file
	remove(infilename);
	rename(filename, infilename);

	gi.dprintf("Model skin added.\n", filename);
//			botDebugPrint("SKIN M (ACRID)\n");
}

void my_bprintf (int printlevel, char *fmt, ...)
{
	int i;
	char	bigbuffer[0x10000];
	int		len;
	va_list		argptr;
	edict_t	*cl_ent;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	if (dedicated->value)
		safe_cprintf(NULL, printlevel, bigbuffer);

	for (i=0 ; i<maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->bot_client)
			continue;

		safe_cprintf(cl_ent, printlevel, bigbuffer);
	}
/*
	for (i=0; i<num_players; i++)
	{
		if (!players[i]->bot_client)
			safe_cprintf(players[i], printlevel, bigbuffer);
	}
*/
}

//======================================================================
// New ACE-compatible message routines

// botsafe cprintf
void safe_cprintf (edict_t *ent, int printlevel, char *fmt, ...)
{
	char bigbuffer[0x10000];
	va_list  argptr;
	int len;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	if (!ent)
	{
		gi.cprintf(ent, printlevel, bigbuffer);
		return;
	}

	if (!ent->inuse || ent->isbot || ent->bot_client)
		return;

	if (!ent->client) return;


	gi.cprintf(ent, printlevel, bigbuffer);

}

// botsafe centerprintf
void safe_centerprintf (edict_t *ent, char *fmt, ...)
{
	char bigbuffer[0x10000];
	va_list  argptr;
	int len;

	if (!ent->inuse || ent->isbot || ent->bot_client)
		return;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	gi.centerprintf(ent, bigbuffer);

}

// botsafe bprintf
void safe_bprintf (int printlevel, char *fmt, ...)
{
	int i;
	char bigbuffer[0x10000];
	int  len;
	va_list  argptr;
	edict_t *cl_ent;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);


	if (dedicated->value)
		safe_cprintf(NULL, printlevel, bigbuffer);

	// This is to be compatible with Eraser (ACE)
	//for (i=0; i<num_players; i++)
	//{
	// Ridah, changed this so CAM works REMOVE THIS
	for (i=0 ; i<maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;

		if (cl_ent->inuse && !cl_ent->bot_client && !cl_ent->isbot)
			safe_cprintf(cl_ent, printlevel, bigbuffer);
	}

}
//ERASER END