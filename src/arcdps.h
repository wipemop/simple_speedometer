// Declarations and comments copied from
// - https://www.deltaconnected.com/arcdps/evtc/README.txt
// - https://www.deltaconnected.com/arcdps/api/arcdps_combatdemo.cpp

#pragma once

#include <cstdint>

/* agent short */
typedef struct ag {
    const char* name; /* agent name. may be null. valid only at time of event. utf8 */
    uintptr_t id; /* agent unique identifier */
    uint32_t prof; /* profession at time of event. refer to evtc notes for identification */
    uint32_t elite; /* elite spec at time of event. refer to evtc notes for identification */
    uint32_t self; /* 1 if self, 0 if not */
    uint16_t team; /* sep21+ */
} ag;

// writeencounter.cpp has code used to write evtc files.
// check file header - evtc bytes for filetype, arcdps build yyyymmdd for compatibility, target species id for boss.
// an npcid of 1 indicates log is wvw.
// an npcid of 2 indicates log is map
// nameless/combatless agents may not be written to table while appearing in events.
// skill and agent names use utf8 encoding.
// evtc_agent.name is a combo string on players - character name <null> account name <null> subgroup str literal <null>.
// add u16 field to the agent table, agents[x].instance_id, initialized to 0.
// add u64 fields agents[x].first_aware initialized to 0, and agents[x].last_aware initialized to u64max.
// add u64 field agents[x].master_addr, initialized to 0.
// if evtc_agent.is_elite == 0xffffffff && upper half of evtc_agent.prof == 0xffff, agent is a gadget with pseudo id as lower half of evtc_agent.prof (volatile id).
// if evtc_agent.is_elite == 0xffffffff && upper half of evtc_agent.prof != 0xffff, agent is a npc with species id as lower half of evtc_agent.prof (reliable id).
// if evtc_agent.is_elite != 0xffffffff, agent is a player with profession as evtc_agent.prof and elite spec as evtc_agent.is_elite.
// gadgets do not have true ids and are generated through a combination of gadget parameters - they will collide with npcs and should be treated separately.
// iterate through all events, assigning instance ids and first/last aware ticks.
// set agents[x].instance_id = src_instid where agents[x].addr == src_agent && !is_statechange.
// set agents[x].first_aware = time on first event, then all consecutive event times to agents[x].last_aware.
// iterate through all events again, this time assigning master agent.
// set agents[z].master_agent on encountering src_master_instid != 0.
// agents[z].master_addr = agents[x].addr where agents[x].instance_id == src_master_instid && agent[x].first_aware < time < last_aware.
// iterate through all events one last time, this time parsing for the data you want.
// src_agent and dst_agent should be used to associate event data with local data.
// parse event type order should check is_statechange > is_activation > is_buffremove > the rest.
// is_statechange will do the heavy lifting of non-internal and parser-requested events - make sure to ignore unknown statechange types.
//
// ---
//
// common to most events:
// time - timeGetTime() at time of registering the event (note: some statechanges use this for other data and not time).
// src_agent - agent the caused the event.
// dst_agent - agent the event happened to.
// skillid - skill id of relevant skill (can be zero).
// src_instid - id of agent as appears in game at time of event. out-of-range agents may have this set even when src_agent is zero
// dst_instid - id of agent as appears in game at time of event. out-of-range agents may have this set even when dst_agent is zero
// src_master_instid - if src_agent has a master (eg. is minion), will be equal to instid of master, zero otherwise.
// dst_master_instid - if dst_agent has a master (eg. is minion), will be equal to instid of master, zero otherwise.
// iff - current affinity of src_agent and dst_agent, is of enum iff.
// is_ninety - src_agent is above 90% health.
// is_fifty - dst_agent is below 50% health.
// is_moving - src_agent is moving at time of event if bit0 is set, dst_agent is moving if bit1 is set.
// is_flanking - src_agent is flanking dst_agent at time of event.
//
// statechange events:
// is_statechange will be non-zero of enum cbtstatechange.
// refer to enum comments below for handling individual statechange types.
//
// is_activation events:
// is_statechange will be zero.
// is_activation will be non-zero of enum cbtactivation.
// cancel_fire or cancel_cancel, value will be the ms duration of the time spent in animation.
// cancel_fire or cancel_cancel, buff_dmg will be the ms duration of the scaled (as if not affected) time spent.
// start, value will be the ms duration at which first significant effect has happened.
// start, buff_dmg will be the ms duration at which control is expected to be returned to character.
// dst_agent will be x/y of target of skill effect.
// overstack_value will be z of target of skill effect.
// result will be of type n_animationstart if start, or n_animationstop if cancel (debug use only).
// pad61-64 will be emote id if skillid == CSK_EMOTE, gadget inst id if skillid == CSK_GADGETINTERACT.
//
// is_buffremove events:
// is_statechange and is_activation will be zero.
// is_buffremove will be non-zero of enum cbtbuffremove.
// buff will be non-zero.
// src_agent is agent that had buff removed, dst_agent is the agent that removed it.
// value will be the remaining time removed calculated as duration.
// buff_dmg will be the remaining time removed calculated as intensity (warning: can overflow on cbtb_all, use as sum check only).
// result will be the number of stacks removed (cbtb_all only).
// pad61-64 will be buff instance id of buff removed (cbtb_single only).
//
// for all events below, statechange, activation, and buffremove will be zero.
//
// buff apply events:
// buff will be non-zero.
// buff_dmg will be zero.
// value will be non-zero duration applied.
// pad61-64 will be buff instance id of the buff.
// is_shields will be stack active status.
// if is_offcycle is zero, overstack_value will be duration of the stack that is expected to be removed.
// if is_offcycle is non-zero, overstack_value will be the new duration of the stack, value will be duration change (no new buff added).
// NOTE: for data eg. category, use CBTS_BUFFINFO.
// logs will always contain skill ID data for 717 718 719 723 725 726 736 738 742 861 873 9162 10233 13061 14458 14459 15788 15790 19426 14459 29025 29466 30328 42883 43499 44871 51683
// NOTE: 4/29/2021+ delays buffapply til end of combat unless both agents are in squad
//
// buff damage events:
// buff will be non-zero.
// value will be zero.
// buff_dmg will be the simulated damage dealt.
// is_offcycle will be zero if damage accumulated by tick, non-zero if reactively (eg. confusion skill use)
// result will be zero if damage expected to hit, 1 for invuln by buff, 2/3/4 for invuln by player skill
// pad61 will be 1 if dst is currently downed.
//
// direct damage events:
// buff will be zero.
// value will be the combined shield+health damage dealt.
// overstack_value will be the shield damage dealt.
// is_offcycle will be 1 if dst is currently downed.
// result will be of enum cbtresult.

// statechange events:
enum cbtstatechange {
    CBTS_NONE, // not used - not this kind of event
    // not used - not this kind of event

    CBTS_ENTERCOMBAT, // agent entered combat
    // src_agent: relates to agent
    // dst_agent: subgroup
    // value: prof id
    // buff_dmg: elite spec id
    // evtc: limited to squad outside instances
    // realtime: limited to squad

    CBTS_EXITCOMBAT, // agent left combat
    // src_agent: relates to agent
    // dst_agent: subgroup
    // value: prof id
    // buff_dmg: elite spec id
    // evtc: limited to squad outside instances
    // realtime: limited to squad

    CBTS_CHANGEUP, // agent is alive at time of event
    // src_agent: relates to agent
    // evtc: limited to agent table outside instances
    // realtime: limited to squad

    CBTS_CHANGEDEAD, // agent is dead at time of event
    // src_agent: relates to agent
    // evtc: limited to agent table outside instances
    // realtime: limited to squad

    CBTS_CHANGEDOWN, // agent is down at time of event
    // src_agent: relates to agent
    // evtc: limited to agent table outside instances
    // realtime: limited to squad

    CBTS_SPAWN, // agent entered tracking
    // src_agent: relates to agent
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_DESPAWN, // agent left tracking
    // src_agent: relates to agent
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_HEALTHPCTUPDATE, // agent health percentage changed
    // src_agent: relates to agent
    // dst_agent: percent * 10000 eg. 99.5% will be 9950
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_SQCOMBATSTART, // squad combat start, first player enter combat. previously named log start
    // dst_agent: 2 if map log, 3 if boss log
    // value: as uint32_t, server unix timestamp
    // buff_dmg: local unix timestamp
    // evtc: yes
    // realtime: yes

    CBTS_SQCOMBATEND, // squad combat stop, last player left combat. previously named log end
    // dst_agent: bit 0 = log ended by pov map exit
    // value: as uint32_t, server unix timestamp
    // buff_dmg: local unix timestamp
    // evtc: yes
    // realtime: yes

    CBTS_WEAPSWAP, // agent weapon set changed
    // src_agent: relates to agent
    // dst_agent: new weapon set id
    // value: old weapon seet id
    // evtc: yes
    // realtime: yes

    CBTS_MAXHEALTHUPDATE, // agent maximum health changed
    // src_agent: relates to agent
    // dst_agent: new max health
    // evtc: limited to non-players
    // realtime: no

    CBTS_POINTOFVIEW, // "recording" player
    // src_agent: relates to agent
    // evtc: yes
    // realtime: no

    CBTS_LANGUAGE, // text language id
    // src_agent: text language id
    // evtc: yes
    // realtime: no

    CBTS_GWBUILD, // game build
    // src_agent: game build number
    // evtc: yes
    // realtime: no

    CBTS_SHARDID, // server shard id
    // src_agent: shard id
    // evtc: yes
    // realtime: no

    CBTS_REWARD, // wiggly box reward
    // dst_agent: reward id
    // value: reward type
    // evtc: yes
    // realtime: yes

    CBTS_BUFFINITIAL, // buff application for buffs already existing at time of event
    // refer to cbtevent struct, identical to buff application. statechange is set to this
    // evtc: limited to squad outside instances
    // realtime: limited to squad

    CBTS_POSITION, // agent position changed
    // src_agent: relates to agent
    // dst_agent: (float*)&dst_agent is float[3], x/y/z
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_VELOCITY, // agent velocity changed
    // src_agent: relates to agent
    // dst_agent: (float*)&dst_agent is float[3], x/y/z
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_FACING, // agent facing direction changed
    // src_agent: relates to agent
    // dst_agent: (float*)&dst_agent is float[2], x/y
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_TEAMCHANGE, // agent team id changed
    // src_agent: relates to agent
    // dst_agent: new team id
    // value: old team id
    // evtc: limited to agent table outside instances
    // realtime: limited to squad

    CBTS_ATTACKTARGET, // attacktarget to gadget association
    // src_agent: relates to agent, the attacktarget
    // dst_agent: the gadget
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_TARGETABLE, // agent targetable state
    // src_agent: relates to agent
    // dst_agent: new targetable state
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_MAPID, // map info
    // src_agent: map id
    // dst_agent: map type
    // evtc: yes
    // realtime: no

    CBTS_REPLINFO, // internal use
    // internal use

    CBTS_STACKACTIVE, // buff instance is now active
    // src_agent: relates to agent
    // dst_agent: buff instance id
    // value: current buff duration
    // evtc: limited to squad outside instances
    // realtime: limited to squad

    CBTS_STACKRESET, // buff instance duration changed, value is the duration to reset to (also marks inactive), pad61-pad64 buff instance id
    // src_agent: relates to agent
    // value: new duration
    // evtc: limited to squad outside instances
    // realtime: limited to squad

    CBTS_GUILD, // agent is a member of guild
    // src_agent: relates to agent
    // dst_agent: (uint8_t*)&dst_agent is uint8_t[16], guid of guild
    // value: new duration
    // evtc: limited to squad outside instances
    // realtime: no

    CBTS_BUFFINFO, // buff information
    // skillid: skilldef id of buff
    // overstack_value: max combined duration
    // src_master_instid: 
    // is_src_flanking: likely an invuln
    // is_shields: likely an invert
    // is_offcycle: category
    // pad61: buff stacking type
    // pad62: likely a resistance
    // pad63: non-zero if used in buff damage simulation (rough pov-only check)
    // evtc: yes
    // realtime: no

    CBTS_BUFFFORMULA, // buff formula, one per event of this type. see bottom of this file for list of always-included skills
    // skillid: skilldef id of buff
    // time: (float*)&time is float[9], type attribute1 attribute2 parameter1 parameter2 parameter3 trait_condition_source trait_condition_self content_reference
    // src_instid: (float*)&src_instid is float[2], buff_condition_source buff_condition_self
    // evtc: yes
    // realtime: no

    CBTS_SKILLINFO, // skill information
    // skillid: skilldef id of skill
    // time: (float*)&time is float[4], cost range0 range1 tooltiptime
    // evtc: yes
    // realtime: no

    CBTS_SKILLTIMING, // skill timing, one per event of this type
    // skillid: skilldef id of skill
    // src_agent: timing type
    // dst_agent: at time since activation in milliseconds
    // evtc: yes
    // realtime: no

    CBTS_BREAKBARSTATE, // agent breakbar state changed
    // src_agent: relates to agent
    // dst_agent: new breakbar state
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_BREAKBARPERCENT, // agent breakbar percentage changed
    // src_agent: relates to agent
    // value: (float*)&value is float[1], new percentage
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_INTEGRITY, // one event per message. previously named error
    // time: (char*)&time is char[32], a short null-terminated message with reason
    // evtc: yes
    // realtime: no

    CBTS_MARKER, // one event per marker on an agent
    // src_agent: relates to agent
    // value: markerdef id. if value is 0, remove all markers presently on agent
    // buff: marker is a commander tag
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_BARRIERPCTUPDATE, // agent barrier percentage changed
    // src_agent: relates to agent
    // dst_agent: percent * 10000 eg. 99.5% will be 9950
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_STATRESET_DEFUNC, // retired, not used since 260402+
    // src_agent: species id of agent that triggered the reset, eg boss species id
    // evtc: yes
    // realtime: yes

    CBTS_EXTENSION, // for extension use. not managed by arcdps
    // evtc: yes
    // realtime: yes

    CBTS_APIDELAYED, // one per cbtevent that got deemed unsafe for realtime but safe for posting after squadcombat
    // evtc: no
    // realtime: yes

    CBTS_INSTANCESTART, // map instance start
    // src_agent: milliseconds ago instance was started
    // value: *(uint32_t*)&value server socket
    // evtc: yes
    // realtime: no

    CBTS_RATEHEALTH, // tick health. previously named tickrate
    // src_agent: 25 - tickrate, when tickrate <= 20 
    // evtc: yes
    // realtime: no

    CBTS_LAST90BEFOREDOWN, // retired, not used since 240529+
    // retired

    CBTS_EFFECT, // retired, not used since 230716+
    // retired

    CBTS_IDTOGUID, // content id to guid association for volatile types. types may contain additional information, see n_contentlocal
    // src_agent: (uint8_t*)&src_agent is uint8_t[16] guid of content
    // overstack_value: is of enum contentlocal
    // evtc: yes
    // realtime: no

    CBTS_LOGNPCUPDATE, // log boss agent changed
    // src_agent: species id of agent
    // dst_agent: related to agent
    // value: as uint32_t, server unix timestamp
    // evtc: yes
    // realtime: yes

    CBTS_IDLEEVENT, // internal use
    // internal use

    CBTS_EXTENSIONCOMBAT, // for extension use. not managed by arcdps
    // assumed to be cbtevent struct, skillid will be processed as such for purpose of buffinfo/skillinfo
    // evtc: yes
    // realtime: yes

    CBTS_FRACTALSCALE, // fractal scale for fractals
    // src_agent: scale
    // evtc: yes
    // realtime: no

    CBTS_EFFECT2_DEFUNC, // retired, not used since 250526+
    // src_agent: related to agent
    // dst_agent: effect at location of agent (if applicable)
    // value: (float*)&value is float[3], location x/y/z (if not at agent location)
    // iff: (uint32_t*)&iff is uint32_t[1], effect duration. if duration is zero, it may be a fixed length duration (see n_contentlocal)
    // buffremove: (uint32_t*)&buffremove is uint32_t[1], trackable id of effect. if dst_agent and location is 0(/0/0), effect was stopped
    // is_shields: (int16_t*)&is_shields is int16_t[3], orientation x/y/z, values are original*1000
    // is_flanking: effect is on a non-static platform
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_RULESET, // ruleset for self
    // src_agent: bit0: pve, bit1: wvw, bit2: pvp
    // evtc: yes
    // realtime: no

    CBTS_SQUADMARKER, // squad ground markers
    // src_agent: (float*)&src_agent is float[3], x/y/z of marker location. if values are all zero or infinity, this marker is removed
    // skillid: index of marker eg. 0 is arrow
    // evtc: yes
    // realtime: no

    CBTS_ARCBUILD, // arc build info
    // src_agent: (char*)&src_agent is a null-terminated string matching the full build string in arcdps.log
    // evtc: yes
    // realtime: no

    CBTS_GLIDER, // glider status change
    // src_agent: related to agent
    // value: 1 deployed, 0 stowed
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_STUNBREAK, // disable stopped early
    // src_agent: related to agent
    // value: duration remaining
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_MISSILECREATE, // create a missile
    // src_agent: related to agent
    // value: (int16*)&value is int16[3], location x/y/z, divided by 10
    // overstack_value: skin id (player only)
    // skillid: missile skill id
    // pad61: (uint32_t*)&pad61 is uint32[1], trackable id
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_MISSILELAUNCH, // launch missile
    // src_agent: related to agent
    // dst_agent: at agent, if set and in range
    // value: (int16*)&value is int16[6], target x/y/z, current x/y/z, divided by 10
    // skillid: missile skill id
    // iff: (uint8_t*)&iff is uint8_t[1], launch motion. unknown, from client
    // result: (int16_t*)&result is int16[1], motion radius
    // is_buffremove: (uint32_t*)&is_buffremove is uint32_t[1], launch flags. unknown, from client
    // is_src_flanking: non-zero if first launch
    // is_shields: (int16_t*)&is_shields is int16[1], missile speed
    // pad61: (uint32_t*)&pad61 is uint32[1], trackable id
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_MISSILEREMOVE, // remove missile
    // src_agent: related to agent
    // value: friendly fire damage total
    // skillid: missile skill id
    // is_src_flanking: hit at least one enemy along the way
    // pad61: (uint32_t*)&pad61 is uint32[1], trackable id
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_EFFECTGROUNDCREATE, // play effect on ground
    // src_agent: related to agent
    // dst_agent: (int16*)&dst_agent is int16[6], origin x/y/z divided by 10, orient x/y/z multiplied by 1000
    // skillid: effect id (prefer using an id to guid map via n_contentlocal)
    // iff: (uint32_t*)&iff is uint32_t[1], effect duration. if duration is zero, it may be a fixed length duration (see n_contentlocal)
    // is_buffremove: flags
    // is_flanking: effect is on a non-static platform
    // is_shields: (int16_t*)&is_shields is int16[1], scale (if zero, assume 1) multiplied by 1000
    // pad61: (uint32_t*)&pad61 is uint32[1], trackable id
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_EFFECTGROUNDREMOVE, // stop effect on ground
    // pad61: (uint32_t*)&pad61 is uint32[1], trackable id
    // evtc: yes
    // realtime: no

    CBTS_EFFECTAGENTCREATE, // play effect around agent
    // src_agent: related to agent
    // skillid: effect id (prefer using an id to guid map via n_contentlocal)
    // iff: (uint32_t*)&iff is uint32_t[1], effect duration. if duration is zero, it may be a fixed length duration (see n_contentlocal)
    // pad61: (uint32_t*)&pad61 is uint32[1], trackable id
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_EFFECTAGENTREMOVE, // stop effect around agent
    // src_agent: related to agent
    // pad61: (uint32_t*)&pad61 is uint32[1], trackable id
    // evtc: limited to agent table outside instances
    // realtime: no

    CBTS_IIDCHANGE, // iid (previously evtc_agent->addr) changed. players only, happens after spawn when player historical data is loaded. does not happen if player has no historical data
    // src_agent: old iid
    // dst_agent: new iid
    // evtc: yes
    // realtime: no

    CBTS_MAPCHANGE, // map changed
    // src_agent: new map id
    // dst_agent: old map id
    // evtc: yes
    // realtime: yes

    CBTS_UNKNOWN // unknown/unsupported type newer than this list perhaps
};

/* is friend/foe */
enum iff {
    IFF_FRIEND,
    IFF_FOE,
    IFF_UNKNOWN
};

/* combat result (direct) */
enum cbtresult {
    CBTR_NORMAL, // strike was neither crit or glance
    CBTR_CRIT, // strike was crit
    CBTR_GLANCE, // strike was glance
    CBTR_BLOCK, // strike was blocked eg. mesmer shield 4
    CBTR_EVADE, // strike was evaded, eg. dodge or mesmer sword 2
    CBTR_INTERRUPT, // strike interrupted something
    CBTR_ABSORB, // strike was "invulned" or absorbed eg. guardian elite
    CBTR_BLIND, // strike missed
    CBTR_KILLINGBLOW, // not a damage strike, target was killed by skill
    CBTR_DOWNED, // not a damage strike, target was downed by skill
    CBTR_BREAKBAR, // not a damage strike, target had value of breakbar damage dealt
    CBTR_ACTIVATION, // not a damage strike, skill activation event
    CBTR_CROWDCONTROL, // not a damage strike, target was crowdcontrolled
    CBTR_UNKNOWN
};

/* combat activation */
enum cbtactivation {
    ACTV_NONE, // not used - not this kind of event
    ACTV_START, // started skill/animation activation
    ACTV_QUICKNESS_UNUSED, // unused as of nov 5 2019
    ACTV_CANCEL_FIRE, // stopped skill activation with reaching tooltip time
    ACTV_CANCEL_CANCEL, // stopped skill activation without reaching tooltip time
    ACTV_RESET, // animation completed fully
    ACTV_CANCEL_NODATA, // same as ACTV_CANCEL_FIRE but on 0 expected duration, where fire is uncertain
    ACTV_UNKNOWN
};

/* combat buff remove type */
enum cbtbuffremove {
    CBTB_NONE, // not used - not this kind of event
    CBTB_ALL, // last/all stacks removed (sent by server)
    CBTB_SINGLE, // single stack removed (sent by server). will happen for each stack on cleanse
    CBTB_MANUAL, // single stack removed (auto by arc on ooc or all stack, ignore for strip/cleanse calc, use for in/out volume)
    CBTB_UNKNOWN
};

/* combat buff cycle type */
enum cbtbuffcycle {
    CBTC_CYCLE, // damage happened on tick timer
    CBTC_NOTCYCLE, // damage happened outside tick timer (resistable)
    CBTC_NOTCYCLENORESIST, // BEFORE MAY 2021: the others were lumped here, now retired
    CBTC_NOTCYCLEDMGTOTARGETONHIT, // damage happened to target on hitting target
    CBTC_NOTCYCLEDMGTOSOURCEONHIT, // damage happened to source on htiting target
    CBTC_NOTCYCLEDMGTOTARGETONSTACKREMOVE, // damage happened to target on source losing a stack
    CBTC_UNKNOWN
};

/* custom skill ids */
enum n_customskill {
    CSK_DODGE = 23275,
    CSK_DEFIANCEDAMAGE,
    CSK_SELFCAST1,
    CSK_ENEMYCAST1,
    CSK_SELFCAST2,
    CSK_ENEMYCAST2,
    CSK_SELFCAST3,
    CSK_ENEMYCAST3,
    CSK_BREAKBAR_DEFUNC,
    CSK_WEAPONDRAW,
    CSK_WEAPONSTOW,
    CSK_GENERICBLOCK,
    CSK_GENERICDAMAGE,
    CSK_GENERICKILL,
    CSK_GENERICDOWN,
    CSK_GENERICEVADE,
    CSK_GENERICINTERRUPT,
    CSK_GENERICABSORB,
    CSK_GENERICMISS,
    CSK_GENERICKNOCKDOWN,
    CSK_GENERICKNOCKBACKPULL,
    CSK_GENERICFLOAT,
    CSK_GENERICLAUNCH,
    CSK_GENERICWATERFLOATSINK,
    CSK_GENERICCCBUFF,
    CSK_GENERICSTAGGER,
    CSK_GENERICINVALID,
    CSK_GADGETINTERACT,
    CSK_EMOTE
};

/* animation start trigger */
enum n_animationstart {
    ANIMSTART_NONE,
    ANIMSTART_COMMAND,
    ANIMSTART_DODGE,
    ANIMSTART_STOWDRAW,
    ANIMSTART_MOVESKILL,
    ANIMSTART_MOTIONSKILL,
    ANIMSTART_GADGETINTERACT,
    ANIMSTART_EMOTE
};

/* animation stop trigger */
enum n_animationstop {
    ANIMSTOP_NONE,
    ANIMSTOP_INSTANT,
    ANIMSTOP_MULTI_DEFUNC,
    ANIMSTOP_TRANSITION,
    ANIMSTOP_PARTIAL,
    ANIMSTOP_ENDED,
    ANIMSTOP_CANCEL,
    ANIMSTOP_STOWDRAW,
    ANIMSTOP_INTERRUPT,
    ANIMSTOP_DEATH,
    ANIMSTOP_DOWNED,
    ANIMSTOP_CROWDCONTROL,
    ANIMSTOP_COMMAND,
    ANIMSTOP_MOTIONSKILL,
    ANIMSTOP_MOVEDODGE,
    ANIMSTOP_MOTIONSKILL_VIA_RESET,
    ANIMSTOP_MOVESKILL,
    ANIMSTOP_MOVEPOS_OR_STOWDRAW,
    ANIMSTOP_ANY,
    ANIMSTOP_ANY_VIA_RESET,
    ANIMSTOP_MANUAL_EXPIRY,
    ANIMSTOP_DESPAWN
};

/* language */
enum gwlanguage {
    GWL_ENG = 0,
    GWL_FRE = 2,
    GWL_GEM = 3,
    GWL_SPA = 4,
    GWL_CN = 5,
};

/* content local enum */
enum n_contentlocal {
    CONTENTLOCAL_EFFECT,
    // src_instid: content type
    // buff_dmg: (float*)&buff_dmg is default duration (if available, when effect2 has no duration or agent set)

    CONTENTLOCAL_MARKER,
    // src_instid: is in commandertag defs

    CONTENTLOCAL_SKILL,
    // no extra data, see SKILL/BUFFINFO events

    CONTENTLOCAL_SPECIES_NOT_GADGET,
    // no extra data

    CONTENTLOCAL_EMOTE
    // no extra data
};

/* evtc agent */
typedef struct evtc_agent {
    uint64_t iid;
    uint32_t prof;
    uint32_t is_elite;
    int16_t toughness;
    int16_t concentration;
    int16_t healing;
    uint16_t hitbox_width;
    int16_t condition;
    uint16_t hitbox_height;
    char name[64];
} evtc_agent;

/* combat event logging (revision 1, when header[12] == 1)
   all fields except time are event-specific, refer to descriptions of events above */
typedef struct cbtevent {
    uint64_t time; /* timegettime() at time of event */
    uint64_t src_agent;
    uint64_t dst_agent;
    int32_t value;
    int32_t buff_dmg;
    uint32_t overstack_value;
    uint32_t skillid;
    uint16_t src_instid;
    uint16_t dst_instid;
    uint16_t src_master_instid;
    uint16_t dst_master_instid;
    uint8_t iff;
    uint8_t buff;
    uint8_t result;
    uint8_t is_activation;
    uint8_t is_buffremove;
    uint8_t is_ninety;
    uint8_t is_fifty;
    uint8_t is_moving;
    uint8_t is_statechange;
    uint8_t is_flanking;
    uint8_t is_shields;
    uint8_t is_offcycle;
    uint8_t pad61;
    uint8_t pad62;
    uint8_t pad63;
    uint8_t pad64;
} cbtevent;

// /* combat event (old, when header[12] == 0) */
// typedef struct cbtevent {
//     uint64_t time;
//     uint64_t src_agent;
//     uint64_t dst_agent;
//     int32_t value;
//     int32_t buff_dmg;
//     uint16_t overstack_value;
//     uint16_t skillid;
//     uint16_t src_instid;
//     uint16_t dst_instid;
//     uint16_t src_master_instid;
//     uint8_t iss_offset;
//     uint8_t iss_offset_target;
//     uint8_t iss_bd_offset;
//     uint8_t iss_bd_offset_target;
//     uint8_t iss_alt_offset;
//     uint8_t iss_alt_offset_target;
//     uint8_t skar;
//     uint8_t skar_alt;
//     uint8_t skar_use_alt;
//     uint8_t iff;
//     uint8_t buff;
//     uint8_t result;
//     uint8_t is_activation;
//     uint8_t is_buffremove;
//     uint8_t is_ninety;
//     uint8_t is_fifty;
//     uint8_t is_moving;
//     uint8_t is_statechange;
//     uint8_t is_flanking;
//     uint8_t is_shields;
//     uint8_t is_offcycle;
//     uint8_t pad64;
// } cbtevent;

struct EvCombatData
{
    cbtevent* ev;
    ag*       src;
    ag*       dst;
    char*     skillname;
    uint64_t  id;
    uint64_t  revision;
};

void LogCombatEvent(cbtevent* ev, ag* src, ag* dst, const char* skillname);
