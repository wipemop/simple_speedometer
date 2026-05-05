#include "arcdps.h"
#include "Shared.h"
#include <cstring>
#include <sstream>

void LogCombatEvent(cbtevent* ev, ag* src, ag* dst, const char* skillname)
{
    std::stringstream ss;

    /* ev is null. dst will only be valid on tracking add. skillname will also be null */
    if (!ev)
    {
        /* notify tracking change */
        if (!src->elite)
        {

            /* add */
            if (src->prof)
            {
                ss << "==== cbtnotify ====\n";
                ss << "agent added: " << src->name << ":" << dst->name << " (" << std::hex << src->id << std::dec << "), instid: " << dst->id << ", prof: " << dst->prof << ", elite: " << dst->elite << ", self: " << dst->self << ", team: " << src->team << ", subgroup: " << dst->team << "\n";
            }

            /* remove */
            else
            {
                ss << "==== cbtnotify ====\n";
                ss << "agent removed: " << src->name << " (" << std::hex << src->id << std::dec << ")\n";
            }
        }

        /* target change */
        else if (src->elite == 1)
        {
            ss << "==== cbtnotify ====\n";
            ss << "new target: " << std::hex << src->id << std::dec << "\n";
        }
    }

    /* combat event. skillname may be null. non-null skillname will remain static until client exit. refer to evtc notes for complete detail */
    else
    {
        /* default names */
        if (!src->name || !strlen(src->name))
            src->name = "(area)";
        if (!dst->name || !strlen(dst->name))
            dst->name = "(area)";

        /* common */
        ss << "combatdemo: ==== cbtevent at " << ev->time << " ====\n";
        ss << "source agent: " << src->name << " (" << std::hex << ev->src_agent << std::dec << ":" << ev->src_instid << ", " << std::hex << src->prof << ":" << src->elite << std::dec << "), master: " << ev->src_master_instid << "\n";
        if (ev->dst_agent)
            ss << "target agent: " << dst->name << " (" << std::hex << ev->dst_agent << std::dec << ":" << ev->dst_instid << ", " << std::hex << dst->prof << ":" << dst->elite << std::dec << ")\n";
        else
            ss << "target agent: n/a\n";

        /* statechange */
        if (ev->is_statechange)
        {
            ss << "is_statechange: " << (int)ev->is_statechange << "\n";
        }

        /* activation */
        else if (ev->is_activation)
        {
            ss << "is_activation: " << (int)ev->is_activation << "\n";
            ss << "skill: " << (skillname ? skillname : "n/a") << ":" << ev->skillid << "\n";
            ss << "ms_expected: " << ev->value << "\n";
        }

        /* buff remove */
        else if (ev->is_buffremove)
        {
            ss << "is_buffremove: " << (int)ev->is_buffremove << "\n";
            ss << "skill: " << (skillname ? skillname : "n/a") << ":" << ev->skillid << "\n";
            ss << "ms_duration: " << ev->value << "\n";
            ss << "ms_intensity: " << ev->buff_dmg << "\n";
        }

        /* buff */
        else if (ev->buff)
        {

            /* damage */
            if (ev->buff_dmg)
            {
                ss << "is_buff: " << (int)ev->buff << "\n";
                ss << "skill: " << (skillname ? skillname : "n/a") << ":" << ev->skillid << "\n";
                ss << "dmg: " << ev->buff_dmg << "\n";
                ss << "is_shields: " << (int)ev->is_shields << "\n";
            }

            /* application */
            else
            {
                ss << "is_buff: " << (int)ev->buff << "\n";
                ss << "skill: " << (skillname ? skillname : "n/a") << ":" << ev->skillid << "\n";
                ss << "raw ms: " << ev->value << "\n";
                ss << "overstack ms: " << ev->overstack_value << "\n";
            }
        }

        /* strike */
        else
        {
            ss << "is_buff: " << (int)ev->buff << "\n";
            ss << "skill: " << (skillname ? skillname : "n/a") << ":" << ev->skillid << "\n";
            ss << "dmg: " << ev->value << "\n";
            ss << "is_moving: " << (int)ev->is_moving << "\n";
            ss << "is_ninety: " << (int)ev->is_ninety << "\n";
            ss << "is_flanking: " << (int)ev->is_flanking << "\n";
            ss << "is_shields: " << (int)ev->is_shields << "\n";
        }

        /* common */
        ss << "iff: " << (int)ev->iff << "\n";
        ss << "result: " << (int)ev->result << "\n";
    }

    APIDefs->Log(ELogLevel_DEBUG, addonName, ss.str().c_str());
}
