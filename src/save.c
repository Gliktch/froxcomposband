/* File: save.c */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies. Other copyrights may also apply.
 */

/* Purpose: interact with savefiles */

#include "angband.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WINDOWS
#include <process.h>
#endif

#ifdef SIGTERM
extern int kill(int, int);
#endif

/*Exo's character information patch*/
void updatecharinfoS(void)
{
	//File Output + Lookup Tables
	char tmp_Path[1024];
	FILE *oFile;

	race_t         *race_ = get_true_race();
	class_t        *class_ = get_class();
	dragon_realm_ptr drealm = dragon_get_realm(p_ptr->dragon_realm);
	bool           race_sub_class_hack = FALSE;

	path_build(tmp_Path, sizeof(tmp_Path), ANGBAND_DIR_USER, "CharOutput.txt");
	oFile = fopen(tmp_Path, "w");
	fprintf(oFile, "{\n");
	fprintf(oFile, "race: \"%s\",\n", race_->name);
	if (race_->subname)
        {
            if ((prace_is_(RACE_MON_POSSESSOR)) || (prace_is_(RACE_MON_MIMIC)) || (prace_is_(RACE_MON_RING)))
            {
                race_sub_class_hack = TRUE;
            }
            else
            {
                 fprintf(oFile, "subRace: \"%s\",\n", race_->subname);
            }
        }
	fprintf(oFile, "class: \"%s\",\n", class_->name);
	if (race_sub_class_hack)
        {
            char nimi[17];
            int paikka;
            bool ok_name = FALSE;
            if ((prace_is_(RACE_MON_MIMIC)) && (p_ptr->current_r_idx == MON_MIMIC)) strncpy(nimi, "nothing", sizeof(nimi));
            else if (strpos("Mouth of Sauron", race_->subname)) strncpy(nimi, "Mouth of Sauron", sizeof(nimi));
            else strncpy(nimi, race_->subname, sizeof(nimi));
            if (strlen(nimi) < 16) ok_name = TRUE;
            while (!ok_name)
            {
                paikka = strpos(",", nimi);
                if (paikka) 
                {
                    nimi[paikka - 1] = '\0';
                    break;
                }
                paikka = strpos(" the", nimi);
                if (paikka) 
                {
                    nimi[paikka - 1] = '\0';
                    break;
                }
                paikka = strpos(" of ", nimi);
                if (paikka) 
                {
                    nimi[paikka - 1] = '\0';
                    break;
                }
                nimi[16] = '\0';
                break;
            }
            fprintf(oFile, "subClass: \"%s\",\n", nimi);
        }
	else if (class_->subname)fprintf(oFile, "subClass: \"%s\",\n", class_->subname);
	fprintf(oFile, "mapName: \"%s\",\n", map_name());
	fprintf(oFile, "dLvl: \"%i\",\n", dun_level);
	if (p_ptr->realm1 > 0)fprintf(oFile, "mRealm1: \"%s\",\n", realm_names[p_ptr->realm1]);
	if (p_ptr->realm2 > 0)fprintf(oFile, "mRealm2: \"%s\",\n", realm_names[p_ptr->realm2]);
	if (!strcmp("Chaos-Warrior", class_->name))fprintf(oFile, "mRealm2: \"%s\",\n", chaos_patrons[p_ptr->chaos_patron]);
	if (p_ptr->dragon_realm > 0)fprintf(oFile, "mRealm1: \"%s\",\n", drealm->name);
	fprintf(oFile, "cLvl: \"%i\",\n", p_ptr->lev);
	fprintf(oFile, "isDead: \"%i\",\n", p_ptr->is_dead);
	fprintf(oFile, "isThrall: \"%i\",\n", thrall_mode ? 1 : 0);
	fprintf(oFile, "killedBy: \"%s\"\n", p_ptr->died_from);
	fprintf(oFile, "}");
	fclose(oFile);
}


void wr_item(savefile_ptr file, object_type *o_ptr)
{
    obj_save(o_ptr, file);
}

static void wr_monster(savefile_ptr file, monster_type *m_ptr)
{
    int i;

    savefile_write_s16b(file, m_ptr->r_idx);
    savefile_write_byte(file, m_ptr->fy);
    savefile_write_byte(file, m_ptr->fx);
    savefile_write_s16b(file, m_ptr->hp);
    savefile_write_s16b(file, m_ptr->maxhp);
    savefile_write_s16b(file, m_ptr->max_maxhp);
    savefile_write_byte(file, m_ptr->mspeed);
    savefile_write_s16b(file, m_ptr->energy_need);
    savefile_write_byte(file, m_ptr->ml);

    if (!is_original_ap(m_ptr))
    {
        savefile_write_byte(file, SAVE_MON_AP_R_IDX);
        savefile_write_s16b(file, m_ptr->ap_r_idx);
    }
    if (m_ptr->sub_align)
    {
        savefile_write_byte(file, SAVE_MON_SUB_ALIGN);
        savefile_write_byte(file, m_ptr->sub_align);
    }
    for (i = 0; i < MTIMED_COUNT; i++)
    {
        if (m_ptr->mtimed[i])
        {
            savefile_write_byte(file, SAVE_MON_TIMER);
            savefile_write_byte(file, i);
            savefile_write_s16b(file, m_ptr->mtimed[i]);
        }
    }
    if (m_ptr->target_y)
    {
        savefile_write_byte(file, SAVE_MON_TARGET_Y);
        savefile_write_s16b(file, m_ptr->target_y);
    }
    if (m_ptr->target_x)
    {
        savefile_write_byte(file, SAVE_MON_TARGET_X);
        savefile_write_s16b(file, m_ptr->target_x);
    }
    if (m_ptr->smart)
    {
        savefile_write_byte(file, SAVE_MON_SMART);
        savefile_write_u32b(file, m_ptr->smart);
    }
    if (m_ptr->exp)
    {
        savefile_write_byte(file, SAVE_MON_EXP);
        savefile_write_u32b(file, m_ptr->exp);
    }
    if (m_ptr->mflag2)
    {
        savefile_write_byte(file, SAVE_MON_MFLAG2);
        savefile_write_u32b(file, m_ptr->mflag2);
    }
    if (m_ptr->nickname)
    {
        savefile_write_byte(file, SAVE_MON_NICKNAME);
        savefile_write_cptr(file, quark_str(m_ptr->nickname));
    }
    if (m_ptr->parent_m_idx)
    {
        savefile_write_byte(file, SAVE_MON_PARENT);
        savefile_write_s16b(file, m_ptr->parent_m_idx);
    }
    if (m_ptr->parent_r_idx)
    {
        savefile_write_byte(file, SAVE_MON_PARENT_RACE);
        savefile_write_s16b(file, m_ptr->parent_r_idx);
    }
    if (m_ptr->pack_idx)
    {
        savefile_write_byte(file, SAVE_MON_PACK_IDX);
        savefile_write_s16b(file, m_ptr->pack_idx);
    }
    if (m_ptr->ac_adj)
    {
        savefile_write_byte(file, SAVE_MON_AC);
        savefile_write_s16b(file, m_ptr->ac_adj);
    }
    if (m_ptr->mpower != 1000)
    {
        savefile_write_byte(file, SAVE_MON_POWER);
        savefile_write_s16b(file, m_ptr->mpower);
    }
    if (m_ptr->drop_ct)
    {
        savefile_write_byte(file, SAVE_MON_DROP_CT);
        savefile_write_byte(file, m_ptr->drop_ct);
    }
    if (m_ptr->stolen_ct)
    {
        savefile_write_byte(file, SAVE_MON_STOLEN_CT);
        savefile_write_byte(file, m_ptr->stolen_ct);
    }
    if (m_ptr->ego_whip_ct)
    {
        savefile_write_byte(file, SAVE_MON_EGO_WHIP);
        savefile_write_byte(file, m_ptr->ego_whip_ct);
        savefile_write_byte(file, m_ptr->ego_whip_pow);
    }
    if (m_ptr->anti_magic_ct)
    {
        savefile_write_byte(file, SAVE_MON_ANTI_MAGIC);
        savefile_write_byte(file, m_ptr->anti_magic_ct);
    }
    if (m_ptr->pexp)
    {
        savefile_write_byte(file, SAVE_MON_PEXP);
        savefile_write_s32b(file, m_ptr->pexp);
    }
    if (m_ptr->anger)
    {
        savefile_write_byte(file, SAVE_MON_ANGER);
        savefile_write_byte(file, m_ptr->anger);
    }
    if (m_ptr->mana)
    {
        savefile_write_byte(file, SAVE_MON_MANA);
        savefile_write_s16b(file, m_ptr->mana);
    }
    if (m_ptr->minislow)
    {
        savefile_write_byte(file, SAVE_MON_MINISLOW);
        savefile_write_byte(file, m_ptr->minislow);
    }
    if (m_ptr->hold_o_idx)
    {
        savefile_write_byte(file, SAVE_MON_HOLD_O_IDX);
        savefile_write_s16b(file, m_ptr->hold_o_idx);
    }

    savefile_write_byte(file, SAVE_MON_DONE);
}

static void wr_race_lore(savefile_ptr file, mon_race_ptr race)
{
    int i, j, ct_blows = 0, ct_auras = 0;
    savefile_write_s16b(file, race->r_sights);
    savefile_write_s16b(file, race->r_deaths);
    savefile_write_s16b(file, race->r_pkills);
    savefile_write_s16b(file, race->r_akills);
    savefile_write_s16b(file, race->r_skills);
    savefile_write_s16b(file, race->r_tkills);
    savefile_write_byte(file, race->r_wake);
    savefile_write_byte(file, race->r_ignore);
    savefile_write_byte(file, race->r_xtra1);
    savefile_write_byte(file, race->r_xtra2);
    savefile_write_byte(file, race->r_drop_gold);
    savefile_write_byte(file, race->r_drop_item);
    savefile_write_u32b(file, race->r_spell_turns);
    savefile_write_u32b(file, race->r_move_turns);
    savefile_write_u32b(file, race->r_flags1);
    savefile_write_u32b(file, race->r_flags2);
    savefile_write_u32b(file, race->r_flags3);
    savefile_write_u32b(file, race->r_flagsr);
    mon_spells_save(race->spells, file); /* 2 + 5S' bytes where S' is a seen spell */
    for (i = 0; i < MAX_MON_BLOWS; i++) /* was 40 bytes ... slightly optimized (cost 2 bytes) */
    {
        mon_blow_ptr blow = &race->blows[i];
        if (!blow->method) break;
        ct_blows++;
    }
    savefile_write_byte(file, ct_blows);
    for (i = 0; i < ct_blows; i++)
    {
        mon_blow_ptr blow = &race->blows[i];
        int          ct_effects = 0;
        savefile_write_s16b(file, blow->lore);
        for (j = 0; j < MAX_MON_BLOW_EFFECTS; j++)
        {
            mon_effect_ptr effect = &blow->effects[j];
            if (!effect->effect) break;
            ct_effects++;
        }
        savefile_write_byte(file, ct_effects);
        for (j = 0; j < ct_effects; j++)
        {
            mon_effect_ptr effect = &blow->effects[j];
            savefile_write_s16b(file, effect->lore);
        }
    }
    for (i = 0; i < MAX_MON_AURAS; i++) /* was 6 bytes ... very very slight optimization */
    {                                   /* but most monsters have no A:* auras (save 6k perhaps)*/
        mon_effect_ptr aura = &race->auras[i];
        if (!aura->effect) break;
        ct_auras++;
    }
    savefile_write_byte(file, ct_auras);
    for (i = 0; i < ct_auras; i++)
    {
        mon_effect_ptr aura = &race->auras[i];
        savefile_write_s16b(file, aura->lore);
    }
}
static bool _race_has_lore(mon_race_ptr race)
{
    return race->r_sights || race->r_tkills; /* XXX */
}
static void wr_r_info(savefile_ptr file)
{
    int i;
    savefile_write_u16b(file, max_r_idx);
    for (i = 0; i < max_r_idx; i++)
    {
        mon_race_ptr race = &r_info[i];
        byte         header = 0;

        if (race->flagsx) header |= 0x01;
        if (_race_has_lore(race)) header |= 0x02;

        savefile_write_byte(file, header);
        savefile_write_byte(file, race->max_num);
        savefile_write_byte(file, race->ball_num);
        savefile_write_s16b(file, race->floor_id);
        savefile_write_byte(file, race->stolen_ct);

        if (race->flagsx)
            savefile_write_u32b(file, race->flagsx);
        if (_race_has_lore(race))
            wr_race_lore(file, race);
    }
}

static bool _have_counts(counts_ptr counts)
{
    return counts->found || counts->bought || counts->used || counts->destroyed;
}

static void wr_xtra_kind(savefile_ptr file, int k_idx)
{
    byte tmp8u = 0;

    object_kind *k_ptr = &k_info[k_idx];

    if (k_ptr->aware) tmp8u |= 0x01;
    if (k_ptr->tried) tmp8u |= 0x02;
    if (_have_counts(&k_ptr->counts)) tmp8u |= 0x04;

    savefile_write_byte(file, tmp8u);
    if (_have_counts(&k_ptr->counts))
    {
        savefile_write_s16b(file, k_ptr->counts.generated);
        savefile_write_s16b(file, k_ptr->counts.found);
        savefile_write_s16b(file, k_ptr->counts.bought);
        savefile_write_s16b(file, k_ptr->counts.used);
        savefile_write_s16b(file, k_ptr->counts.destroyed);
    }
}

static bool _ego_has_lore(ego_ptr ego)
{
    int i;
    if (_have_counts(&ego->counts)) return TRUE;
    for (i = 0; i < OF_ARRAY_SIZE; i++)
    {
        if (ego->known_flags[i]) return TRUE;
        if (ego->xtra_flags[i]) return TRUE;
    }
    return FALSE;
}
static void wr_xtra_ego_aux(savefile_ptr file, ego_ptr ego)
{
    int i;
    for (i = 0; i < OF_ARRAY_SIZE; i++)
    {
        if (!ego->known_flags[i]) continue;
        savefile_write_byte(file, i);
        savefile_write_u32b(file, ego->known_flags[i]);
    }
    savefile_write_byte(file, 0xFF);
    for (i = 0; i < OF_ARRAY_SIZE; i++)
    {
        if (!ego->xtra_flags[i]) continue;
        savefile_write_byte(file, i);
        savefile_write_u32b(file, ego->xtra_flags[i]);
    }
    savefile_write_byte(file, 0xFF);

    savefile_write_s16b(file, ego->counts.generated);
    savefile_write_s16b(file, ego->counts.found);
    savefile_write_s16b(file, ego->counts.bought);
    savefile_write_s16b(file, ego->counts.destroyed);
}
static void wr_xtra_ego(savefile_ptr file)
{
    int i;
    for (i = 0; i < max_e_idx; i++)
    {
        ego_ptr ego = &e_info[i];
        if (!ego->name) continue;
        if (!_ego_has_lore(ego)) continue;
        savefile_write_s16b(file, i);
        wr_xtra_ego_aux(file, ego);
    }
    savefile_write_s16b(file, -1);
}

static bool _art_has_lore(art_ptr art)
{
    int i;
    for (i = 0; i < OF_ARRAY_SIZE; i++)
    {
        if (art->known_flags[i]) return TRUE;
    }
    return FALSE;
}
static void wr_xtra_art_aux(savefile_ptr file, art_ptr art)
{
    int i;
    for (i = 0; i < OF_ARRAY_SIZE; i++)
    {
        if (!art->known_flags[i]) continue;
        savefile_write_byte(file, i);
        savefile_write_u32b(file, art->known_flags[i]);
    }
    savefile_write_byte(file, 0xFF);
}
static void wr_xtra_art(savefile_ptr file)
{
    int i;
    for (i = 0; i < max_a_idx; i++)
    {
        art_ptr art = &a_info[i];
        if (!art->name) continue;
        if (!_art_has_lore(art)) continue;
        savefile_write_s16b(file, i);
        wr_xtra_art_aux(file, art);
    }
    savefile_write_s16b(file, -1);
}

static void wr_randomizer(savefile_ptr file)
{
    int i;

    savefile_write_u16b(file, Rand_place);

    for (i = 0; i < RAND_DEG; i++)
        savefile_write_u32b(file, Rand_state[i]);
}

static void wr_options(savefile_ptr file)
{
    int i;
    u16b c;

    sync_retry_options();

    savefile_write_byte(file, delay_factor);
    savefile_write_byte(file, hitpoint_warn);
    savefile_write_byte(file, mana_warn);
    savefile_write_byte(file, random_artifact_pct);
    savefile_write_byte(file, reduce_uniques_pct);
    savefile_write_byte(file, object_list_width);
    savefile_write_byte(file, monster_list_width);
    savefile_write_byte(file, generate_empty);
    savefile_write_byte(file, small_level_type);

    /*** Cheating options ***/
    c = 0;
    if (p_ptr->wizard) c |= 0x0002;
    if (cheat_peek) c |= 0x0100;
    if (cheat_hear) c |= 0x0200;
    if (cheat_room) c |= 0x0400;
    if (cheat_xtra) c |= 0x0800;
    if (cheat_live) c |= 0x2000;
    if (cheat_save) c |= 0x4000;
    savefile_write_u16b(file, c);

    /* Autosave info */
    savefile_write_byte(file, autosave_l);
    savefile_write_byte(file, autosave_t);
    savefile_write_s16b(file, autosave_freq);

    /*** Extract options ***/
    for (i = 0; option_info[i].o_desc; i++)
    {
        int os = option_info[i].o_set;
        int ob = option_info[i].o_bit;

        if (option_info[i].o_var)
        {
            if (*option_info[i].o_var)
                option_flag[os] |= (1L << ob);
            else
                option_flag[os] &= ~(1L << ob);
        }
    }

    for (i = 0; i < 8; i++) savefile_write_u32b(file, option_flag[i]);
    for (i = 0; i < 8; i++) savefile_write_u32b(file, option_mask[i]);


    /*** Window options ***/
    for (i = 0; i < 8; i++) savefile_write_u32b(file, window_flag[i]);
    for (i = 0; i < 8; i++) savefile_write_u32b(file, window_mask[i]);
}


static void wr_quick_start(savefile_ptr file)
{
    int i;

    savefile_write_byte(file, previous_char.game_mode);
    savefile_write_byte(file, previous_char.coffee_break);
    savefile_write_byte(file, previous_char.psex);
    savefile_write_byte(file, previous_char.prace);
    savefile_write_byte(file, previous_char.psubrace);
    savefile_write_byte(file, previous_char.pclass);
    savefile_write_byte(file, previous_char.psubclass);
    savefile_write_byte(file, previous_char.personality);
    savefile_write_byte(file, previous_char.realm1);
    savefile_write_byte(file, previous_char.realm2);
    savefile_write_byte(file, previous_char.dragon_realm);
    savefile_write_s32b(file, previous_char.au);

    for (i = 0; i < 6; i++)
        savefile_write_s16b(file, previous_char.stat_max[i]);

    savefile_write_byte(file, previous_char.quick_ok);
}

static void wr_mystery(savefile_ptr file)
{
    if ((no_wilderness) || (!seed_dungeon) || (d_info[DUNGEON_MYSTERY].flags1 & DF1_SUPPRESSED))
    {
        savefile_write_byte(file, 0xFD);
        return;
    }
    else
    {
        dungeon_info_type *d_ptr = &d_info[DUNGEON_MYSTERY];
        savefile_write_byte(file, 0xFF);
        savefile_write_byte(file, d_ptr->dy);
        savefile_write_byte(file, d_ptr->dx);
        savefile_write_s16b(file, d_ptr->mindepth);
        savefile_write_s16b(file, d_ptr->maxdepth);
        savefile_write_s16b(file, (s16b)d_ptr->final_guardian);
        savefile_write_s16b(file, (s16b)d_ptr->initial_guardian);
        savefile_write_byte(file, d_ptr->wild_type);
        savefile_write_byte(file, d_ptr->min_plev);
    }
}

static void wr_extra(savefile_ptr file)
{
    int i,j;
    byte tmp8u;

    savefile_write_s32b(file, p_ptr->id);
    savefile_write_cptr(file, player_name);
    savefile_write_cptr(file, p_ptr->died_from);
    savefile_write_cptr(file, p_ptr->last_message ? p_ptr->last_message : "");
    wr_quick_start(file);

    savefile_write_s32b(file, game_mode);
    savefile_write_byte(file, coffee_break);
    savefile_write_byte(file, pantheon_count);
    savefile_write_byte(file, game_pantheon);
    savefile_write_byte(file, active_pantheon);
    savefile_write_byte(file, p_ptr->prace);
    savefile_write_byte(file, p_ptr->pclass);
    savefile_write_byte(file, p_ptr->personality);
    savefile_write_byte(file, p_ptr->psex);
    savefile_write_byte(file, p_ptr->realm1);
    savefile_write_byte(file, p_ptr->realm2);
    savefile_write_byte(file, p_ptr->dragon_realm);
    savefile_write_byte(file, p_ptr->psubclass);
    savefile_write_byte(file, p_ptr->psubrace);
    savefile_write_s16b(file, p_ptr->current_r_idx);
    savefile_write_u16b(file, p_ptr->expfact);

    for (i = 0; i < 6; ++i) savefile_write_s16b(file, p_ptr->stat_max[i]);
    for (i = 0; i < 6; ++i) savefile_write_s16b(file, p_ptr->stat_max_max[i]);
    for (i = 0; i < 6; ++i) savefile_write_s16b(file, p_ptr->stat_cur[i]);

    savefile_write_u32b(file, p_ptr->au);
    savefile_write_s16b(file, p_ptr->fame);
    savefile_write_u32b(file, p_ptr->max_exp);
    savefile_write_u32b(file, p_ptr->max_max_exp);
    savefile_write_u32b(file, p_ptr->exp);
    savefile_write_u32b(file, p_ptr->exp_frac);
    savefile_write_s16b(file, p_ptr->lev);
    savefile_write_u32b(file, p_ptr->quest_seed);

    for (i = 0; i < 64; i++) savefile_write_s16b(file, p_ptr->spell_exp[i]);
    for (i = 0; i < 5; i++) for (j = 0; j < 64; j++) savefile_write_s16b(file, p_ptr->weapon_exp[i][j]);
    for (i = 0; i < 10; i++) savefile_write_s16b(file, p_ptr->skill_exp[i]);
    for (i = 0; i < MAX_MAGIC_NUM; i++) savefile_write_s32b(file, p_ptr->magic_num1[i]);
    for (i = 0; i < MAX_MAGIC_NUM; i++) savefile_write_byte(file, p_ptr->magic_num2[i]);

    savefile_write_byte(file, p_ptr->start_race);
    savefile_write_byte(file, p_ptr->start_sex);
    savefile_write_s32b(file, p_ptr->old_race1);
    savefile_write_s32b(file, p_ptr->old_race2);
    savefile_write_s32b(file, p_ptr->old_race3);
    savefile_write_s16b(file, p_ptr->old_realm);

    for (i = 0; i < MAX_MANE; i++)
    {
        savefile_write_s16b(file, p_ptr->mane_spell[i]);
        savefile_write_s16b(file, p_ptr->mane_dam[i]);
    }
    savefile_write_s16b(file, p_ptr->mane_num);

    for (i = 0; i < MAX_KUBI; i++)
        savefile_write_s16b(file, kubi_r_idx[i]);

    for (i = 0; i < 4; i++)
    {
        savefile_write_s16b(file, battle_mon[i]);
        savefile_write_u32b(file, mon_odds[i]);
    }

    savefile_write_s16b(file, p_ptr->town_num);
    savefile_write_s16b(file, p_ptr->arena_number);
    savefile_write_s16b(file, p_ptr->inside_arena);
    savefile_write_s16b(file, p_ptr->inside_battle);
    savefile_write_byte(file, p_ptr->exit_bldg);
    savefile_write_s16b(file, p_ptr->oldpx);
    savefile_write_s16b(file, p_ptr->oldpy);
    savefile_write_s32b(file, p_ptr->mmhp);
    savefile_write_s32b(file, p_ptr->mhp);
    savefile_write_s32b(file, p_ptr->chp);
    savefile_write_u32b(file, p_ptr->chp_frac);
    savefile_write_s32b(file, p_ptr->msp);
    savefile_write_s32b(file, p_ptr->csp);
    savefile_write_u32b(file, p_ptr->csp_frac);
    savefile_write_s16b(file, p_ptr->clp);
    savefile_write_s16b(file, p_ptr->max_plv);

    tmp8u = (byte)max_d_idx;
    savefile_write_byte(file, tmp8u);
    for (i = 0; i < tmp8u; i++)
        savefile_write_s16b(file, max_dlv[i]);
    for (i = 0; i < tmp8u; i++)
        savefile_write_u32b(file, dungeon_flags[i]);

    wr_mystery(file);

    savefile_write_s16b(file, p_ptr->concent);
    savefile_write_s16b(file, p_ptr->blind);
    savefile_write_s16b(file, p_ptr->paralyzed);
    savefile_write_s16b(file, p_ptr->confused);
    savefile_write_s16b(file, p_ptr->food);
    savefile_write_s16b(file, p_ptr->energy_need);
    savefile_write_s16b(file, p_ptr->fast);
    savefile_write_s16b(file, p_ptr->slow);
    savefile_write_byte(file, p_ptr->minislow);
    savefile_write_u16b(file, p_ptr->mini_energy);
    savefile_write_byte(file, p_ptr->unwell);
    savefile_write_s16b(file, p_ptr->afraid);
    savefile_write_s16b(file, p_ptr->cut);
    savefile_write_s16b(file, p_ptr->stun);
    savefile_write_s16b(file, p_ptr->poisoned);
    savefile_write_s16b(file, p_ptr->image);

    savefile_write_s16b(file, p_ptr->protevil);
    savefile_write_s16b(file, p_ptr->invuln);
    savefile_write_s16b(file, p_ptr->ult_res);
    savefile_write_s16b(file, p_ptr->hero);
    savefile_write_s16b(file, p_ptr->shero);
    savefile_write_s16b(file, p_ptr->shield);
    savefile_write_s16b(file, p_ptr->blessed);
    savefile_write_s16b(file, p_ptr->tim_invis);
    savefile_write_s16b(file, p_ptr->word_recall);
    savefile_write_s16b(file, p_ptr->recall_dungeon);
    savefile_write_s16b(file, p_ptr->alter_reality);
    savefile_write_s16b(file, p_ptr->see_infra);
    savefile_write_s16b(file, p_ptr->tim_infra);
    savefile_write_s16b(file, p_ptr->tim_poet);
    savefile_write_s16b(file, p_ptr->tim_understanding);
    savefile_write_s16b(file, p_ptr->oppose_fire);
    savefile_write_s16b(file, p_ptr->oppose_cold);
    savefile_write_s16b(file, p_ptr->oppose_acid);
    savefile_write_s16b(file, p_ptr->oppose_elec);
    savefile_write_s16b(file, p_ptr->oppose_pois);
    savefile_write_s16b(file, p_ptr->spin);
    savefile_write_s16b(file, p_ptr->tsuyoshi);
    savefile_write_s16b(file, p_ptr->tim_esp);
    savefile_write_s16b(file, p_ptr->tim_esp_magical);
    savefile_write_s16b(file, p_ptr->wraith_form);
    savefile_write_s16b(file, p_ptr->resist_magic);
    savefile_write_s16b(file, p_ptr->tim_regen);
    savefile_write_s16b(file, p_ptr->kabenuke);
    savefile_write_s16b(file, p_ptr->tim_stealth);
    savefile_write_s16b(file, p_ptr->tim_levitation);
    savefile_write_s16b(file, p_ptr->tim_sh_touki);
    savefile_write_s16b(file, p_ptr->lightspeed);
    savefile_write_s16b(file, p_ptr->tsubureru);
    savefile_write_s16b(file, p_ptr->magicdef);
    savefile_write_s16b(file, p_ptr->tim_res_nether);
    savefile_write_s16b(file, p_ptr->tim_res_time);
    savefile_write_s16b(file, p_ptr->tim_res_disenchantment);
    savefile_write_s16b(file, p_ptr->mimic_form);
    savefile_write_s16b(file, p_ptr->tim_mimic);
    savefile_write_s16b(file, p_ptr->tim_sh_fire);
    savefile_write_s16b(file, p_ptr->tim_sh_elements);
    savefile_write_s16b(file, p_ptr->tim_sh_shards);
    savefile_write_s16b(file, p_ptr->tim_sh_domination);
    savefile_write_s16b(file, p_ptr->tim_weaponmastery);
    savefile_write_s16b(file, p_ptr->tim_sh_holy);
    savefile_write_s16b(file, p_ptr->tim_eyeeye);

    savefile_write_s16b(file, p_ptr->tim_spurt);
    savefile_write_s16b(file, p_ptr->tim_no_spells);
    savefile_write_s16b(file, p_ptr->tim_no_device);
    savefile_write_s16b(file, p_ptr->tim_blood_shield);
    savefile_write_s16b(file, p_ptr->tim_blood_sight);
    savefile_write_s16b(file, p_ptr->tim_blood_feast);
    savefile_write_s16b(file, p_ptr->tim_blood_revenge);
    savefile_write_s16b(file, p_ptr->tim_blood_seek);
    savefile_write_s16b(file, p_ptr->tim_blood_rite);
    savefile_write_s16b(file, p_ptr->tim_force);
    savefile_write_s16b(file, p_ptr->tim_building_up);
    savefile_write_s16b(file, p_ptr->tim_vicious_strike);
    savefile_write_s16b(file, p_ptr->tim_enlarge_weapon);
    savefile_write_s16b(file, p_ptr->tim_field);

    savefile_write_s16b(file, p_ptr->tim_spell_reaction);
    savefile_write_s16b(file, p_ptr->tim_resist_curses);
    savefile_write_s16b(file, p_ptr->tim_armor_of_fury);
    savefile_write_s16b(file, p_ptr->tim_spell_turning);

    savefile_write_s16b(file, p_ptr->tim_dark_stalker);
    savefile_write_s16b(file, p_ptr->tim_nimble_dodge);
    savefile_write_s16b(file, p_ptr->tim_stealthy_snipe);

    savefile_write_s16b(file, p_ptr->tim_killing_spree);
    savefile_write_s16b(file, p_ptr->tim_slay_sentient);

    {
        int i;
        savefile_write_s16b(file, MAX_WILD_COUNTERS);
        for (i = 0; i < MAX_WILD_COUNTERS; i++)
        {
            savefile_write_s16b(file, p_ptr->wild_counters[i].type);
            savefile_write_s16b(file, p_ptr->wild_counters[i].counter);
        }
    }
    /* Remember the Monkey Clone */
    {
        int i;
        monster_race *r_ptr = &r_info[MON_MONKEY_CLONE];
        savefile_write_byte(file, r_ptr->cur_num);
        if (r_ptr->cur_num)
        {
            savefile_write_byte(file, r_ptr->hdice); /* Probably not required ... */
            savefile_write_byte(file, r_ptr->hside); /* Probably not required ... */
            savefile_write_s16b(file, r_ptr->ac);
            savefile_write_byte(file, r_ptr->speed); /* Probably not required ... */
            for (i = 0; i < MAX_MON_BLOWS; i++)
            {
                savefile_write_byte(file, r_ptr->blows[i].method);
                savefile_write_byte(file, r_ptr->blows[i].effects[0].effect);
                savefile_write_byte(file, r_ptr->blows[i].effects[0].dd);
                savefile_write_byte(file, r_ptr->blows[i].effects[0].ds);
            }
            savefile_write_u32b(file, r_ptr->flags3);
            savefile_write_u32b(file, r_ptr->flagsr);
            savefile_write_u32b(file, r_ptr->flags2);
            savefile_write_u32b(file, r_ptr->flags7);
        }
    }
    savefile_write_s16b(file, p_ptr->entrench_x);
    savefile_write_s16b(file, p_ptr->entrench_y);
    savefile_write_s16b(file, p_ptr->entrench_ct);
    savefile_write_byte(file, p_ptr->sense_artifact);
    savefile_write_s16b(file, p_ptr->duelist_target_idx);
    savefile_write_s16b(file, p_ptr->health_who);
    savefile_write_s16b(file, target_who);
    savefile_write_s16b(file, pet_t_m_idx);
    savefile_write_s16b(file, riding_t_m_idx);

    /* by henkma */
    savefile_write_s16b(file, p_ptr->tim_reflect);
    savefile_write_s16b(file, p_ptr->multishadow);
    savefile_write_s16b(file, p_ptr->dustrobe);

    savefile_write_s16b(file, p_ptr->tim_superstealth);

    savefile_write_bool(file, p_ptr->fasting);
    savefile_write_s16b(file, p_ptr->tim_sustain_str);
    savefile_write_s16b(file, p_ptr->tim_sustain_int);
    savefile_write_s16b(file, p_ptr->tim_sustain_wis);
    savefile_write_s16b(file, p_ptr->tim_sustain_dex);
    savefile_write_s16b(file, p_ptr->tim_sustain_con);
    savefile_write_s16b(file, p_ptr->tim_sustain_chr);
    savefile_write_s16b(file, p_ptr->tim_hold_life);
    savefile_write_s16b(file, p_ptr->tim_transcendence);
    savefile_write_s16b(file, p_ptr->tim_quick_walk);
    savefile_write_s16b(file, p_ptr->tim_inven_prot);
    savefile_write_s16b(file, p_ptr->tim_inven_prot2);
    savefile_write_s16b(file, p_ptr->tim_device_power);
    savefile_write_s16b(file, p_ptr->tim_sh_time);
    savefile_write_s16b(file, p_ptr->free_turns);
    savefile_write_s16b(file, p_ptr->tim_foresight);

    savefile_write_s16b(file, p_ptr->chaos_patron);
    for (i = 0; i < MUT_FLAG_SIZE; ++i)
        savefile_write_u32b(file, p_ptr->muta[i]);
    for (i = 0; i < MUT_FLAG_SIZE; ++i)
        savefile_write_u32b(file, p_ptr->muta_lock[i]);
    for (i = 0; i < MAX_DEMIGOD_POWERS; ++i)
        savefile_write_s16b(file, p_ptr->demigod_power[i]);
    savefile_write_s16b(file, p_ptr->draconian_power);

    for (i = 0; i<8; i++)
        savefile_write_s16b(file, p_ptr->virtues[i]);
    for (i = 0; i<8; i++)
        savefile_write_s16b(file, p_ptr->vir_types[i]);

    savefile_write_s16b(file, p_ptr->ele_attack);
    savefile_write_u32b(file, p_ptr->special_attack);
    savefile_write_s16b(file, p_ptr->ele_immune);
    savefile_write_u32b(file, p_ptr->special_defense);
    savefile_write_byte(file, p_ptr->knowledge);
    savefile_write_byte(file, p_ptr->autopick_autoregister);
    savefile_write_byte(file, p_ptr->action);
    savefile_write_byte(file, preserve_mode);
    savefile_write_byte(file, p_ptr->wait_report_score);
    savefile_write_u32b(file, seed_flavor);
    savefile_write_u32b(file, seed_town);
    savefile_write_u32b(file, seed_dungeon);

    /* It probably isn't possible to save during a time stop, so world_monster
     * should always be 0 */
    savefile_write_byte(file, world_monster);
    savefile_write_s16b(file, p_ptr->no_air);
    if (p_ptr->no_air) savefile_write_byte(file, no_air_monster);

    /* Careful - we need to tell the savefile whether personality includes
     * Chaotic BEFORE adding the Chaotic-exclusive content... */
    if (p_ptr->personality == PERS_SPLIT) split_save(file);
    if (personality_includes_(PERS_CHAOTIC)) savefile_write_u32b(file, chaotic_py_seed);

    savefile_write_u16b(file, p_ptr->panic_save);
    savefile_write_u16b(file, p_ptr->total_winner);
    savefile_write_u16b(file, p_ptr->noscore);
    savefile_write_byte(file, p_ptr->is_dead);
    savefile_write_byte(file, p_ptr->feeling);
    savefile_write_s32b(file, old_turn);
    savefile_write_s32b(file, p_ptr->feeling_turn);
    savefile_write_s32b(file, game_turn);
    savefile_write_s32b(file, player_turn);
    savefile_write_s32b(file, dungeon_turn);
    savefile_write_s32b(file, image_turn);
    savefile_write_s32b(file, old_battle);
    savefile_write_s16b(file, today_mon);
    savefile_write_s16b(file, p_ptr->today_mon);
    savefile_write_s16b(file, p_ptr->riding);
    savefile_write_s16b(file, p_ptr->floor_id);
    savefile_write_byte(file, overworld_visit);
    savefile_write_u32b(file, playtime);
    savefile_write_u32b(file, p_ptr->count);
    savefile_write_byte(file, p_ptr->coffee_lv_revisits);
    savefile_write_byte(file, p_ptr->filibuster);
    savefile_write_byte(file, p_ptr->upset_okay);
    savefile_write_byte(file, p_ptr->py_summon_kills);
    savefile_write_s16b(file, p_ptr->lv_kills);
    savefile_write_s16b(file, p_ptr->pet_lv_kills);
    savefile_write_s32b(file, p_ptr->p_stat);
    savefile_write_s32b(file, message_pane_wrap_width > 150 ? 0 : message_pane_wrap_width_normalize(message_pane_wrap_width));
    savefile_write_s32b(file, autorun_max_steps);
    savefile_write_s32b(file, always_repeat_count);
    savefile_write_s32b(file, failed_item_retry_count);
    savefile_write_s32b(file, temp_file_policy);
    savefile_write_s32b(file, map_edge_center_distance_normalize(map_edge_center_distance));
    for (i = 0; i < 9; i++)
        savefile_write_s32b(file, 0); /* Future use */

    for (i = 0; i < MAX_POWER_LABEL; i++)
    {
        if (!power_labels[i][0]) savefile_write_byte(file, 0xFF);
        else
        {
            savefile_write_byte(file, i);
            savefile_write_cptr(file, power_labels[i]);
        }
    }

    {
    race_t  *race_ptr = get_true_race();
    class_t *class_ptr = get_class();

        if (race_ptr->save_player)
            race_ptr->save_player(file);
        if (class_ptr->save_player)
            class_ptr->save_player(file);
    }
}

static bool ang_sort_comp_cave_temp(vptr u, vptr v, int a, int b)
{
    cave_template_type *who = (cave_template_type *)(u);

    u16b o1 = who[a].occurrence;
    u16b o2 = who[b].occurrence;

    /* Unused */
    (void)v;

    return o2 <= o1;
}

static void ang_sort_swap_cave_temp(vptr u, vptr v, int a, int b)
{
    cave_template_type *who = (cave_template_type *)(u);

    cave_template_type holder;

    /* Unused */
    (void)v;

    /* Swap */
    holder = who[a];
    who[a] = who[b];
    who[b] = holder;
}


/*
 * Actually write a saved floor data
 * using effectively compressed format.
 */
static void wr_saved_floor(savefile_ptr file, saved_floor_type *sf_ptr)
{
    cave_template_type *template;
    u16b max_num_temp;
    u16b num_temp = 0;
    int dummy_why;

    int i, y, x;

    u16b tmp16u;

    byte count;
    u16b prev_u16b;


    if (!sf_ptr)
    {
        /*** Not a saved floor ***/
        savefile_write_s16b(file, dun_level);
    }
    else
    {
        /*** The saved floor ***/
        savefile_write_s16b(file, sf_ptr->floor_id);
        savefile_write_byte(file, sf_ptr->savefile_id);
        savefile_write_s16b(file, sf_ptr->dun_level);
        savefile_write_s32b(file, sf_ptr->last_visit);
        savefile_write_u32b(file, sf_ptr->visit_mark);
        savefile_write_s16b(file, sf_ptr->upper_floor_id);
        savefile_write_s16b(file, sf_ptr->lower_floor_id);
    }

    savefile_write_u16b(file, base_level);
    savefile_write_s16b(file, num_repro);
    savefile_write_s16b(file, num_repro_kill);
    savefile_write_u16b(file, (u16b)py);
    savefile_write_u16b(file, (u16b)px);
    savefile_write_u16b(file, cur_hgt);
    savefile_write_u16b(file, cur_wid);
    savefile_write_byte(file, p_ptr->feeling);


    /*********** Make template for cave_type **********/

    /*
     * Usually number of templates are fewer than 255. Even if
     * more than 254 are exist, the occurrence of each template
     * with larger ID is very small when we sort templates by
     * occurrence. So we will use two (or more) bytes for
     * templete ID larger than 254.
     *
     * Ex: 256 will be "0xff" "0x01".
     *     515 will be "0xff" "0xff" "0x03"
     */

    /* Fake max number */
    max_num_temp = 255;

    /* Allocate the "template" array */
    C_MAKE(template, max_num_temp, cave_template_type);

    /* Extract template array */
    for (y = 0; y < cur_hgt; y++)
    {
        for (x = 0; x < cur_wid; x++)
        {
            cave_type *c_ptr = &cave[y][x];

            for (i = 0; i < num_temp; i++)
            {
                if (template[i].info == c_ptr->info &&
                    template[i].feat == c_ptr->feat &&
                    template[i].mimic == c_ptr->mimic &&
                    template[i].special == c_ptr->special)
                {
                    /* Same terrain is exist */
                    template[i].occurrence++;
                    break;
                }
            }

            /* Are there same one? */
            if (i < num_temp) continue;

            /* If the max_num_temp is too small, increase it. */
            if (num_temp >= max_num_temp)
            {
                cave_template_type *old_template = template;

                /* Re-allocate the "template" array */
                C_MAKE(template, max_num_temp + 255, cave_template_type);
                C_COPY(template, old_template, max_num_temp, cave_template_type);
                C_FREE(old_template, max_num_temp, cave_template_type);
                max_num_temp += 255;
            }

            /* Add new template */
            template[num_temp].info = c_ptr->info;
            template[num_temp].feat = c_ptr->feat;
            template[num_temp].mimic = c_ptr->mimic;
            template[num_temp].special = c_ptr->special;
            template[num_temp].occurrence = 1;

            /* Increase number of template */
            num_temp++;
        }
    }

    /* Select the sort method */
    ang_sort_comp = ang_sort_comp_cave_temp;
    ang_sort_swap = ang_sort_swap_cave_temp;

    /* Sort by occurrence */
    ang_sort(template, &dummy_why, num_temp);


    /*** Dump templates ***/

    /* Total templates */
    savefile_write_u16b(file, num_temp);

    /* Dump the templates */
    for (i = 0; i < num_temp; i++)
    {
        cave_template_type *ct_ptr = &template[i];

        /* Dump it */
        savefile_write_u32b(file, ct_ptr->info);
        savefile_write_s16b(file, ct_ptr->feat);
        savefile_write_s16b(file, ct_ptr->mimic);
        savefile_write_s16b(file, ct_ptr->special);
    }



    /*** "Run-Length-Encoding" of cave ***/

    /* Note that this will induce two wasted bytes */
    count = 0;
    prev_u16b = 0;

    /* Dump the cave */
    for (y = 0; y < cur_hgt; y++)
    {
        for (x = 0; x < cur_wid; x++)
        {
            cave_type *c_ptr = &cave[y][x];

            for (i = 0; i < num_temp; i++)
            {
                if (template[i].info == c_ptr->info &&
                    template[i].feat == c_ptr->feat &&
                    template[i].mimic == c_ptr->mimic &&
                    template[i].special == c_ptr->special)
                    break;
            }

            /* Extract an ID */
            tmp16u = i;

            /* If the run is broken, or too full, flush it */
            if ((tmp16u != prev_u16b) || (count == MAX_UCHAR))
            {
                savefile_write_byte(file, (byte)count);

                while (prev_u16b >= MAX_UCHAR)
                {
                    /* Mark as actual data is larger than 254 */
                    savefile_write_byte(file, MAX_UCHAR);
                    prev_u16b -= MAX_UCHAR;
                }

                savefile_write_byte(file, (byte)prev_u16b);
                prev_u16b = tmp16u;
                count = 1;
            }

            /* Continue the run */
            else
            {
                count++;
            }
        }
    }

    /* Flush the data (if any) */
    if (count)
    {
        savefile_write_byte(file, (byte)count);

        while (prev_u16b >= MAX_UCHAR)
        {
            /* Mark as actual data is larger than 254 */
            savefile_write_byte(file, MAX_UCHAR);
            prev_u16b -= MAX_UCHAR;
        }
        savefile_write_byte(file, (byte)prev_u16b);
    }


    /* Free the "template" array */
    C_FREE(template, max_num_temp, cave_template_type);


    /*** Dump objects ***/
    savefile_write_u16b(file, o_max);
    for (i = 1; i < o_max; i++)
    {
        object_type *o_ptr = &o_list[i];
        wr_item(file, o_ptr);
    }


    /*** Dump the monsters ***/
    savefile_write_u16b(file, m_max);
    for (i = 1; i < m_max; i++)
    {
        monster_type *m_ptr = &m_list[i];
        wr_monster(file, m_ptr);
    }

    /* Pack Info */
    savefile_write_s16b(file, pack_info_count);
    for (i = 1; i < max_pack_info_idx; ++i)
    {
        pack_info_t    *pack_ptr = &pack_info_list[i];
        if (pack_ptr->pack_idx)
        {
            savefile_write_s16b(file, pack_ptr->pack_idx);
            savefile_write_s16b(file, pack_ptr->leader_idx);
            savefile_write_s16b(file, pack_ptr->count);
            savefile_write_s16b(file, pack_ptr->ai);
            savefile_write_s16b(file, pack_ptr->guard_idx);
            savefile_write_s16b(file, pack_ptr->guard_x);
            savefile_write_s16b(file, pack_ptr->guard_y);
            savefile_write_s16b(file, pack_ptr->distance);
        }
    }
}

static bool wr_dungeon(savefile_ptr file)
{
    saved_floor_type *cur_sf_ptr;
    int i;

    forget_lite();
    forget_view();
    clear_mon_lite();
    p_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
    p_ptr->update |= (PU_MONSTERS | PU_DISTANCE | PU_FLOW);


    /*** Meta info ***/
    savefile_write_s16b(file, max_floor_id);
    savefile_write_byte(file, dungeon_type);


    /*** No saved floor (On the surface etc.) ***/
    if (!p_ptr->floor_id)
    {
        /* No array elements */
        savefile_write_byte(file, 0);

        /* Write the current floor data */
        wr_saved_floor(file, NULL);
        return TRUE;
    }


    /*** In the dungeon ***/

    /* Number of array elements */
    savefile_write_byte(file, MAX_SAVED_FLOORS);

    /* Write the saved_floors array */
    for (i = 0; i < MAX_SAVED_FLOORS; i++)
    {
        saved_floor_type *sf_ptr = &saved_floors[i];

        savefile_write_s16b(file, sf_ptr->floor_id);
        savefile_write_byte(file, sf_ptr->savefile_id);
        savefile_write_s16b(file, sf_ptr->dun_level);
        savefile_write_s32b(file, sf_ptr->last_visit);
        savefile_write_u32b(file, sf_ptr->visit_mark);
        savefile_write_s16b(file, sf_ptr->upper_floor_id);
        savefile_write_s16b(file, sf_ptr->lower_floor_id);
    }

    /* Extract pointer to current floor */
    cur_sf_ptr = get_sf_ptr(p_ptr->floor_id);

    /* Save current floor to temporal file */
    if (!save_floor(cur_sf_ptr, (SLF_SECOND))) return FALSE;

    /* Move data in temporal files to the savefile */
    for (i = 0; i < MAX_SAVED_FLOORS; i++)
    {
        saved_floor_type *sf_ptr = &saved_floors[i];

        /* Unused element */
        if (!sf_ptr->floor_id) continue;

        /* Load temporal saved floor file */
        if (load_floor(sf_ptr, (SLF_SECOND | SLF_NO_KILL)))
        {
            /* Mark success */
            savefile_write_byte(file, 0);

            /* Write saved floor data to the save file */
            wr_saved_floor(file, sf_ptr);
        }
        else
        {
            /* Mark failure */
            savefile_write_byte(file, 1);
        }
    }

    /* Restore current floor */
    if (!load_floor(cur_sf_ptr, (SLF_SECOND))) return FALSE;

    /* Success */
    return TRUE;
}

static bool wr_savefile_new(savefile_ptr file)
{
    int        i;

    u32b              now;
    u16b            tmp16u;


    compact_objects(0);
    compact_monsters(0);
    now = (u32b)time(0);
    sf_system = 0L;
    sf_when = now;
    sf_saves++;

    /*** Actually write the file ***/
    savefile_write_u32b(file, sf_system);
    savefile_write_u32b(file, sf_when);
    savefile_write_u16b(file, sf_lives);
    savefile_write_u16b(file, sf_saves);

    wr_randomizer(file);
    wr_options(file);

    msg_on_save(file);

    wr_r_info(file);

    tmp16u = max_k_idx;
    savefile_write_u16b(file, tmp16u);
    for (i = 0; i < tmp16u; i++) wr_xtra_kind(file, i);

    wr_xtra_ego(file);
    wr_xtra_art(file);

    quests_save(file);

    savefile_write_s32b(file, p_ptr->wilderness_x);
    savefile_write_s32b(file, p_ptr->wilderness_y);
    savefile_write_s16b(file, p_ptr->wilderness_dx);
    savefile_write_s16b(file, p_ptr->wilderness_dy);
    savefile_write_byte(file, p_ptr->wild_mode);
    savefile_write_byte(file, TRUE);
    savefile_write_s32b(file, max_wild_x);
    savefile_write_s32b(file, max_wild_y);

    savefile_write_u32b(file, wilderness_seed);

    tmp16u = max_a_idx;
    savefile_write_u16b(file, tmp16u);
    for (i = 0; i < tmp16u; i++)
    {
        artifact_type *a_ptr = &a_info[i];
        savefile_write_byte(file, a_ptr->generated);
        savefile_write_byte(file, a_ptr->found);
        savefile_write_s16b(file, a_ptr->floor_id);
    }

    wr_extra(file);

    tmp16u = PY_MAX_LEVEL;
    savefile_write_u16b(file, tmp16u);
    for (i = 0; i < tmp16u; i++)
        savefile_write_s16b(file, p_ptr->player_hp[i]);

    savefile_write_u32b(file, p_ptr->spell_learned1);
    savefile_write_u32b(file, p_ptr->spell_learned2);
    savefile_write_u32b(file, p_ptr->spell_worked1);
    savefile_write_u32b(file, p_ptr->spell_worked2);
    savefile_write_u32b(file, p_ptr->spell_forgotten1);
    savefile_write_u32b(file, p_ptr->spell_forgotten2);
    savefile_write_s16b(file, p_ptr->learned_spells);
    savefile_write_s16b(file, p_ptr->add_spells);

    for (i = 0; i < 64; i++)
        savefile_write_byte(file, p_ptr->spell_order[i]);

    equip_save(file);
    pack_save(file);
    quiver_save(file);
    towns_save(file);
    home_save(file);
    cornucopia_save(file);

    savefile_write_s16b(file, p_ptr->pet_follow_distance);
    savefile_write_s16b(file, p_ptr->pet_extra_flags);

    if (screen_dump && (p_ptr->wait_report_score || !p_ptr->is_dead))
        savefile_write_cptr(file, screen_dump);
    else
        savefile_write_cptr(file, "");

    spell_stats_on_save(file);
    skills_on_save(file);
    stats_on_save(file);

    if (!p_ptr->is_dead)
    {
        if (!wr_dungeon(file)) return FALSE;
    }
    savefile_write_u32b(file, file->v_check);
    savefile_write_u32b(file, file->x_check);

	updatecharinfoS();

    if (ferror(file->file) || (fflush(file->file) == EOF)) return FALSE;
    return TRUE;
}


static bool save_player_aux(char *name)
{
    bool ok = FALSE;
    savefile_ptr file = savefile_open_write(name);

    if (file)
    {
        /* Hack: Wiping the monster list clears the current duel! */
        handle_tmp_indices(TRUE, TRUE);
        ok = wr_savefile_new(file);
        handle_tmp_indices(FALSE, TRUE);

        if (!savefile_close(file)) ok = FALSE;
    }

    /* Remove "broken" files */
    if (!ok)
    {
        safe_setuid_grab();
        fd_kill(name);
        safe_setuid_drop();
    }

    if (!ok) return FALSE;

    counts_write(0, playtime);
    character_saved = TRUE;
    return TRUE;
}



/*
 * Attempt to save the player in a savefile
 */
bool save_player(void)
{
    int             result = FALSE;

    char    safe[1024];


#ifdef SET_UID

# ifdef SECURE

    /* Get "games" permissions */
    beGames();

# endif

#endif


    /* New savefile */
    strcpy(safe, savefile);
    strcat(safe, ".new");

#ifdef VM
    /* Hack -- support "flat directory" usage on VM/ESA */
    strcpy(safe, savefile);
    strcat(safe, "n");
#endif /* VM */

    /* Grab permissions */
    safe_setuid_grab();

    /* Remove it */
    fd_kill(safe);

    /* Drop permissions */
    safe_setuid_drop();

    update_playtime();

    /* Attempt to save the player */
    if (save_player_aux(safe))
    {
        char temp[1024];

        /* Old savefile */
        strcpy(temp, savefile);
        strcat(temp, ".old");

#ifdef VM
        /* Hack -- support "flat directory" usage on VM/ESA */
        strcpy(temp, savefile);
        strcat(temp, "o");
#endif /* VM */

        /* Grab permissions */
        safe_setuid_grab();

        /* Remove it */
        fd_kill(temp);

        /* Preserve old savefile */
        fd_move(savefile, temp);

        /* Activate new savefile */
        fd_move(safe, savefile);

        /* Remove preserved savefile */
        fd_kill(temp);

        /* Drop permissions */
        safe_setuid_drop();

        /* Hack -- Pretend the character was loaded */
        character_loaded = TRUE;

#ifdef VERIFY_SAVEFILE

        if (!arg_protected_session)
        {
            /* Lock on savefile */
            strcpy(temp, savefile);
            strcat(temp, ".lok");

            /* Grab permissions */
            safe_setuid_grab();

            /* Remove lock file */
            fd_kill(temp);

            /* Drop permissions */
            safe_setuid_drop();
        }

#endif

        /* Success */
        result = TRUE;
    }


#ifdef SET_UID

# ifdef SECURE

    /* Drop "games" permissions */
    bePlayer();

# endif

#endif

    /* Return the result */
    return (result);
}

static char *versio_nimi(int tavu, int keski)
{
	switch (keski)
	{
		case 0: {
			switch (tavu)
			{
				case 0: return "toffee";
				case 1: return "chocolate";
				case 2: return "liquorice";
				case 3: return "salmiak";
				case 4: return "strawberry";
				case 5: return "peppermint";
				case 6: return "mango";
				case 7: return "nougat";
				case 8: return "raspberry";
				default: return "cloudberry";
			}
		}
		default: {
			switch (tavu)
			{
				case 0: return "toffee";
				case 1: return "chocolate";
				case 2: return "liquorice";
				case 3: return "salmiak";
				case 4: return "raspberry";
				case 5: return "spearmint";
				case 6: return "peach";
				case 7: return "apricot";
				case 8: return "blueberry";
				case 9: return "munkki";
				default: return "cloudberry";
			}
		}
	}
}

extern byte versio_sovitus(void)
{
	int i;
	for (i = 0; i < 10; i++)
	{
		if (streq(VER_PATCH, versio_nimi(i, VER_MINOR))) return i;
	}
	return 4;
}

static int _savefile_session_lock_fd = -1;
static char _savefile_session_lock_path[1024];
static bool _savefile_session_lock_acquire(void);

typedef struct
{
    bool valid;
    char savefile[1024];
    long pid;
    unsigned long uid;
    unsigned long when;
    bool has_pid;
    bool has_uid;
    bool has_when;
} _savefile_lock_meta_t;

enum
{
    _SAVEFILE_PID_UNKNOWN = 0,
    _SAVEFILE_PID_RUNNING,
    _SAVEFILE_PID_NOT_FOUND,
    _SAVEFILE_PID_INACCESSIBLE
};

bool savefile_session_lock_supported(void)
{
#ifdef SET_UID
# ifdef USG
#  if defined(F_SETLK)
    return TRUE;
#  endif
# else
#  if defined(LOCK_EX) && defined(LOCK_NB)
    return TRUE;
#  endif
# endif
#endif
    return FALSE;
}

bool savefile_session_lock_refresh(void)
{
    return _savefile_session_lock_acquire();
}

static errr _savefile_session_try_lock(int fd)
{
    if (fd < 0) return -1;
    if (!savefile_session_lock_supported()) return -1;

#ifdef SET_UID
# ifdef USG
#  if defined(F_SETLK)
    struct flock lock;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &lock) != 0) return 1;
#  endif
# else
#  if defined(LOCK_EX) && defined(LOCK_NB)
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) return 1;
#  endif
# endif
#endif

    return 0;
}

static void _savefile_lock_meta_init(_savefile_lock_meta_t *meta)
{
    memset(meta, 0, sizeof(*meta));
}

static void _savefile_lock_meta_parse(_savefile_lock_meta_t *meta, char *buf)
{
    char *line = buf;

    _savefile_lock_meta_init(meta);

    while (line && *line)
    {
        char *next = strchr(line, '\n');
        char *sep = strchr(line, '=');

        if (next) *next++ = '\0';

        if (sep)
        {
            *sep++ = '\0';

            if (streq(line, "savefile"))
            {
                my_strcpy(meta->savefile, sep, sizeof(meta->savefile));
                meta->valid = TRUE;
            }
            else if (streq(line, "pid"))
            {
                meta->pid = atol(sep);
                meta->has_pid = meta->pid > 0;
                meta->valid = TRUE;
            }
            else if (streq(line, "uid"))
            {
                meta->uid = strtoul(sep, NULL, 10);
                meta->has_uid = TRUE;
                meta->valid = TRUE;
            }
            else if (streq(line, "time"))
            {
                meta->when = strtoul(sep, NULL, 10);
                meta->has_when = TRUE;
                meta->valid = TRUE;
            }
        }

        line = next;
    }
}

static bool _savefile_lock_meta_read(int fd, _savefile_lock_meta_t *meta)
{
    char buf[512];
    int len;

    _savefile_lock_meta_init(meta);

    if (fd < 0) return FALSE;
    if (fd_seek(fd, 0)) return FALSE;

    len = read(fd, buf, sizeof(buf) - 1);
    if (len <= 0) return FALSE;

    buf[len] = '\0';
    _savefile_lock_meta_parse(meta, buf);
    return meta->valid;
}

static int _savefile_pid_status(long pid)
{
#ifdef SIGTERM
    if (pid > 0)
    {
        if (kill((int)pid, 0) == 0) return _SAVEFILE_PID_RUNNING;
        if (errno == ESRCH) return _SAVEFILE_PID_NOT_FOUND;
        if (errno == EPERM) return _SAVEFILE_PID_INACCESSIBLE;
    }
#endif
    return _SAVEFILE_PID_UNKNOWN;
}

static long _savefile_current_pid(void)
{
#ifdef WINDOWS
    return (long)_getpid();
#elif defined(HAVE_UNISTD_H)
    return (long)getpid();
#else
    return 0;
#endif
}

static unsigned long _savefile_current_uid(void)
{
#ifdef SET_UID
    return (unsigned long)getuid();
#else
    return 0;
#endif
}

static cptr _savefile_pid_status_desc(long pid)
{
    switch (_savefile_pid_status(pid))
    {
    case _SAVEFILE_PID_RUNNING:
        return "running";
    case _SAVEFILE_PID_NOT_FOUND:
        return "not found";
    case _SAVEFILE_PID_INACCESSIBLE:
        return "inaccessible";
    default:
        return "unknown";
    }
}

static void _savefile_lock_owner_desc(char *buf, int size, _savefile_lock_meta_t *meta)
{
    if (meta->has_uid)
    {
        cptr same = meta->uid == _savefile_current_uid() ? "same user" : "different user";

#ifdef SET_UID
        char user[32];

        user_name(user, (int)meta->uid);
        if (user[0])
            strnfmt(buf, size, "%s [%lu] (%s)", user, meta->uid, same);
        else
            strnfmt(buf, size, "[%lu] (%s)", meta->uid, same);
#else
        strnfmt(buf, size, "[%lu] (%s)", meta->uid, same);
#endif
    }
    else
        my_strcpy(buf, "unknown", size);
}

static void _savefile_lock_time_desc(char *buf, int size, _savefile_lock_meta_t *meta)
{
    if (meta->has_when)
    {
        time_t now = (time_t)meta->when;
        struct tm *tm = localtime(&now);

        if (tm && strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm)) return;
    }

    my_strcpy(buf, "unknown", size);
}

static void _savefile_lock_command_desc(char *buf, int size, long pid)
{
#ifdef __linux__
    if (pid > 0)
    {
        FILE *fff;
        char path[64];
        char cmd[256];
        int i;
        int len;

        strnfmt(path, sizeof(path), "/proc/%ld/cmdline", pid);
        fff = fopen(path, "r");
        if (fff)
        {
            len = fread(cmd, 1, sizeof(cmd) - 1, fff);
            fclose(fff);

            if (len > 0)
            {
                cmd[len] = '\0';
                for (i = 0; i < len - 1; i++)
                {
                    if (cmd[i] == '\0') cmd[i] = ' ';
                }
                my_strcpy(buf, cmd, size);
                return;
            }
        }
    }
#endif
    my_strcpy(buf, "unavailable", size);
}

static void _savefile_lock_show_details(cptr path, _savefile_lock_meta_t *meta)
{
    char when[80];
    char owner[80];
    char cmd[256];
    char tmp[1280];
    int row = 2;

    _savefile_lock_time_desc(when, sizeof(when), meta);
    _savefile_lock_owner_desc(owner, sizeof(owner), meta);
    _savefile_lock_command_desc(cmd, sizeof(cmd), meta->pid);

    Term_clear();
    prt("Savefile Lock Details", row++, 0);
    row++;
    strnfmt(tmp, sizeof(tmp), "Savefile: %s", meta->savefile[0] ? meta->savefile : savefile);
    prt(tmp, row++, 0);
    strnfmt(tmp, sizeof(tmp), "Lock file: %s", path);
    prt(tmp, row++, 0);
    strnfmt(tmp, sizeof(tmp), "PID: %s", meta->has_pid ? format("%ld", meta->pid) : "unknown");
    prt(tmp, row++, 0);
    strnfmt(tmp, sizeof(tmp), "Lock time: %s", when);
    prt(tmp, row++, 0);
    strnfmt(tmp, sizeof(tmp), "Process status: %s", meta->has_pid ? _savefile_pid_status_desc(meta->pid) : "unknown");
    prt(tmp, row++, 0);
    strnfmt(tmp, sizeof(tmp), "Owner: %s", owner);
    prt(tmp, row++, 0);
    strnfmt(tmp, sizeof(tmp), "Command: %s", cmd);
    prt(tmp, row++, 0);
    prt("Press any key to return.", row + 2, 0);
    (void)inkey();
}

static bool _savefile_lock_try_terminate(int fd, _savefile_lock_meta_t *meta, cptr *msg1, cptr *msg2)
{
#ifdef SIGTERM
    int i;

    if (!meta->has_pid)
    {
        *msg1 = "No valid PID was found in the session lock.";
        *msg2 = "Press ? for details, or any other key to exit.";
        return FALSE;
    }

    if (!meta->has_uid || meta->uid != _savefile_current_uid())
    {
        *msg1 = "Termination is only available for a lock owned by the same user.";
        *msg2 = "Press ? for details, or any other key to exit.";
        return FALSE;
    }

    if (kill((int)meta->pid, SIGTERM) != 0)
    {
        *msg1 = "The other process could not be terminated.";
        *msg2 = "It may already be exiting, or permission may have been denied.";
        return FALSE;
    }

    for (i = 0; i < 25; i++)
    {
        Term_xtra(TERM_XTRA_DELAY, 100);
        if (_savefile_session_try_lock(fd) == 0) return TRUE;
    }

    *msg1 = "The other process did not release the savefile lock in time.";
    *msg2 = "Wait a moment and try again, or close it manually.";
    return FALSE;
#else
    *msg1 = "Process termination is not available on this build.";
    *msg2 = "Press ? for details, or any other key to exit.";
    return FALSE;
#endif
}

static void _savefile_lock_quit_screen(cptr msg1, cptr msg2)
{
    Term_clear();
    c_prt(TERM_RED, "Savefile is currently in use by another running copy of the game.", 2, 0);
    c_prt(TERM_RED, "It's not possible to continue safely, without a strong likelihood of corruption or data loss.", 3, 0);
    if (msg1) prt(msg1, 6, 0);
    if (msg2) prt(msg2, 7, 0);
    prt("Press t to attempt termination, ? for details, or any other key to exit.", 9, 0);
}

static bool _savefile_session_lock_conflict(int fd, cptr path)
{
    _savefile_lock_meta_t meta;
    cptr msg1 = NULL;
    cptr msg2 = NULL;

    (void)_savefile_lock_meta_read(fd, &meta);

    while (TRUE)
    {
        char cmd;

        _savefile_lock_quit_screen(msg1, msg2);
        cmd = inkey();

        if (cmd == '?')
        {
            _savefile_lock_show_details(path, &meta);
            continue;
        }

        if (cmd == 't' || cmd == 'T')
        {
            if (_savefile_lock_try_terminate(fd, &meta, &msg1, &msg2))
                return TRUE;

            (void)_savefile_lock_meta_read(fd, &meta);
            continue;
        }

        quit(NULL);
    }
}

static void _savefile_session_unlock(int fd)
{
    if (fd < 0) return;

#ifdef SET_UID
# ifdef USG
#  if defined(F_SETLK)
    struct flock lock;

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;

    (void)fcntl(fd, F_SETLK, &lock);
#  endif
# else
#  if defined(LOCK_UN)
    (void)flock(fd, LOCK_UN);
#  endif
# endif
#endif
}

static void _savefile_session_lock_release(void)
{
    if (_savefile_session_lock_fd < 0) return;

    safe_setuid_grab();
    _savefile_session_unlock(_savefile_session_lock_fd);
    (void)fd_close(_savefile_session_lock_fd);
    if (_savefile_session_lock_path[0])
        (void)fd_kill(_savefile_session_lock_path);
    safe_setuid_drop();

    _savefile_session_lock_fd = -1;
    _savefile_session_lock_path[0] = '\0';
}

static void _protected_session_mode_unsupported(void)
{
    Term_clear();
    prt("Protected session mode was requested via the -p launch parameter,", 2, 0);
    prt("but this build/platform does not support session locking.", 3, 0);
    prt("Press any key to exit.", 5, 0);
    (void)inkey();
    quit(NULL);
}

static bool _savefile_session_lock_acquire(void)
{
    char path[1024];
    char meta[256];
    int fd;

    if (!arg_protected_session) return TRUE;
    if (!savefile[0]) return TRUE;
    if (!savefile_session_lock_supported())
    {
        _protected_session_mode_unsupported();
        return FALSE;
    }

    strnfmt(path, sizeof(path), "%s.lok", savefile);

    if (_savefile_session_lock_fd >= 0)
    {
        if (streq(_savefile_session_lock_path, path)) return TRUE;
        _savefile_session_lock_release();
    }

    safe_setuid_grab();
    fd = fd_open(path, O_RDWR);
    if (fd < 0)
        fd = fd_make(path, 0644);
    if (fd < 0)
    {
        safe_setuid_drop();
        quit("Cannot create the savefile lock.");
    }

    if (_savefile_session_try_lock(fd) != 0)
    {
        safe_setuid_drop();
        if (_savefile_session_lock_conflict(fd, path))
            safe_setuid_grab();
        else
            quit(NULL);
    }

    strnfmt(meta, sizeof(meta),
        "FroxComposband save session lock\nsavefile=%s\npid=%ld\nuid=%lu\ntime=%lu\n",
        savefile, _savefile_current_pid(), _savefile_current_uid(), (unsigned long)time(NULL));
    (void)fd_seek(fd, 0);
    (void)fd_chop(fd, 0);
    (void)fd_write(fd, meta, strlen(meta));
    safe_setuid_drop();

    _savefile_session_lock_fd = fd;
    my_strcpy(_savefile_session_lock_path, path, sizeof(_savefile_session_lock_path));
    return TRUE;
}

/*
 * Attempt to Load a "savefile"
 *
 * Version 2.7.0 introduced a slightly different "savefile" format from
 * older versions, requiring a completely different parsing method.
 *
 * Note that savefiles from 2.7.0 - 2.7.2 are completely obsolete.
 *
 * Pre-2.8.0 savefiles lose some data, see "load2.c" for info.
 *
 * Pre-2.7.0 savefiles lose a lot of things, see "load1.c" for info.
 *
 * On multi-user systems, you may only "read" a savefile if you will be
 * allowed to "write" it later, this prevents painful situations in which
 * the player loads a savefile belonging to someone else, and then is not
 * allowed to save his game when he quits.
 *
 * We return "TRUE" if the savefile was usable, and we set the global
 * flag "character_loaded" if a real, living, character was loaded.
 *
 * Note that we always try to load the "current" savefile, even if
 * there is no such file, so we must check for "empty" savefile names.
 */
bool load_player(void)
{
    int             fd = -1;

    errr    err = 0;

    byte    vvv[4];

#ifdef VERIFY_TIMESTAMP
    struct stat     statbuf;
#endif

    cptr    what = "generic";


    /* Paranoia */
    game_turn = 0;
    player_turn = 0;

    /* Paranoia */
    p_ptr->is_dead = FALSE;


    /* Allow empty savefile name */
    if (!savefile[0]) return (TRUE);

    if (!_savefile_session_lock_acquire()) return FALSE;


#if !defined(MACINTOSH) && !defined(WINDOWS) && !defined(VM)

    /* XXX XXX XXX Fix this */

    /* Verify the existance of the savefile */
    if (access(savefile, 0) < 0)
    {
        /* Give a message */
        msg_print("Savefile does not exist.");

        msg_print(NULL);

        /* Allow this */
        return (TRUE);
    }

#endif


#ifdef VERIFY_SAVEFILE

    /* Verify savefile usage */
    if (!err && !arg_protected_session)
    {
        FILE *fkk;

        char temp[1024];

        /* Extract name of lock file */
        strcpy(temp, savefile);
        strcat(temp, ".lok");

        /* Check for lock */
        fkk = my_fopen(temp, "r");

        /* Oops, lock exists */
        if (fkk)
        {
            /* Close the file */
            my_fclose(fkk);

            /* Message */
            msg_print("Savefile is currently in use.");

            msg_print(NULL);

            /* Oops */
            return (FALSE);
        }

        /* Create a lock file */
        fkk = my_fopen(temp, "w");

        /* Dump a line of info */
        fprintf(fkk, "Lock file for savefile '%s'\n", savefile);

        /* Close the lock file */
        my_fclose(fkk);
    }

#endif


    /* Okay */
    if (!err)
    {
        /* Open the savefile */
        fd = fd_open(savefile, O_RDONLY);

        /* No file */
        if (fd < 0) err = -1;

        /* Message (below) */
        if (err) what = "Cannot open savefile";

    }

    /* Process file */
    if (!err)
    {

#ifdef VERIFY_TIMESTAMP
        /* Get the timestamp */
        (void)fstat(fd, &statbuf);
#endif

        /* Read the first four bytes */
        if (fd_read(fd, (char*)(vvv), 4)) err = -1;

        /* What */
        if (err) what = "Cannot read savefile";


        /* Close the file */
        (void)fd_close(fd);
    }

    /* Process file */
    if (!err)
    {

        /* Extract version */
        z_major = vvv[0];
        z_minor = vvv[1];
        strcpy(z_patch, versio_nimi(vvv[2], z_minor));
        sf_extra = vvv[3];


        /* Clear screen */
        Term_clear();

        /* Attempt to load */
        err = rd_savefile_new();

        /* Message (below) */
        if (err) what = "Cannot parse savefile";

    }

    /* Paranoia */
    if (!err)
    {
        /* Invalid turn */
        if (!game_turn) err = -1;

        /* Message (below) */
        if (err) what = "Broken savefile";

    }

#ifdef VERIFY_TIMESTAMP
    /* Verify timestamp */
    if (!err && !arg_wizard)
    {
        /* Hack -- Verify the timestamp */
        if (sf_when > (statbuf.st_ctime + 100) ||
            sf_when < (statbuf.st_ctime - 100))
        {
            /* Message */
            what = "Invalid timestamp";


            /* Oops */
            err = -1;
        }
    }
#endif


    /* Okay */
    if (!err)
    {
        /* Give a conversion warning */
        if ((VER_MAJOR != z_major) ||
            (VER_MINOR != z_minor) ||
            (!streq(VER_PATCH, z_patch)))
        {
            msg_format("Converted a %d.%d.%s savefile.",
                (z_major > 9) ? z_major-10 : z_major , z_minor, z_patch);
            msg_print(NULL);
        }

        /* Player is dead */
        if (p_ptr->is_dead)
        {
            /* Cheat death */
            if (arg_wizard)
            {
                /* A character was loaded */
                character_loaded = TRUE;

                /* Done */
                return (TRUE);
            }

            /* Player is no longer "dead" */
            p_ptr->is_dead = FALSE;

            /* Count lives */
            sf_lives++;

            /* Done */
            return (TRUE);
        }

        /* A character was loaded */
        character_loaded = TRUE;

        {
            u32b tmp = counts_read(2);
            if (tmp > p_ptr->count)
                p_ptr->count = tmp;
            if (counts_read(0) > playtime || counts_read(1) == playtime)
                counts_write(2, ++p_ptr->count);
            counts_write(1, playtime);
        }

        /* Success */
        return (TRUE);
    }


#ifdef VERIFY_SAVEFILE

    /* Verify savefile usage */
    if (!arg_protected_session)
    {
        char temp[1024];

        /* Extract name of lock file */
        strcpy(temp, savefile);
        strcat(temp, ".lok");

        /* Remove lock */
        fd_kill(temp);
    }

#endif

    if (arg_protected_session)
        _savefile_session_lock_release();


    /* Message */
    msg_format("Error (%s) reading %d.%d.%s savefile.",
           what, (z_major>9) ? z_major - 10 : z_major, z_minor, z_patch);
    msg_print(NULL);

    /* Oops */
    return (FALSE);
}


void remove_loc(void)
{
    if (arg_protected_session)
    {
        _savefile_session_lock_release();
        return;
    }

    if (!savefile[0]) return;

#ifdef VERIFY_SAVEFILE
    char temp[1024];
#endif /* VERIFY_SAVEFILE */

#ifdef SET_UID
# ifdef SECURE

    /* Get "games" permissions */
    beGames();

# endif /* SECURE */
#endif /* SET_UID */

#ifdef VERIFY_SAVEFILE

    /* Lock on savefile */
    strcpy(temp, savefile);
    strcat(temp, ".lok");

    /* Remove lock file */
    fd_kill(temp);

#endif /* VERIFY_SAVEFILE */

#ifdef SET_UID
# ifdef SECURE

    /* Drop "games" permissions */
    bePlayer();

# endif /* SECURE */
#endif /* SET_UID */

}


static bool save_floor_aux(savefile_ptr file, saved_floor_type *sf_ptr)
{
    compact_objects(0);
    compact_monsters(0);

    savefile_write_u32b(file, saved_floor_file_sign);
    wr_saved_floor(file, sf_ptr);

    savefile_write_u32b(file, file->v_check);
    savefile_write_u32b(file, file->x_check);

    if (ferror(file->file) || (fflush(file->file) == EOF)) return FALSE;
    return TRUE;
}


/*
 * Attempt to save the temporally saved-floor data
 */
bool save_floor(saved_floor_type *sf_ptr, u32b mode)
{
    char floor_savefile[1024];
    bool ok = FALSE;
    savefile_ptr file = NULL;

    if (!(mode & SLF_SECOND))
    {
#ifdef SET_UID
# ifdef SECURE
        /* Get "games" permissions */
        beGames();
# endif
#endif
    }

    snprintf(floor_savefile, sizeof(floor_savefile), "%.1000s.F%02d", savefile, (int)sf_ptr->savefile_id);
    file = savefile_open_write(floor_savefile);
    if (file)
    {
        ok = save_floor_aux(file, sf_ptr);
        if (!savefile_close(file)) ok = FALSE;
    }

    /* Remove "broken" files */
    if (!ok)
    {
        safe_setuid_grab();
        (void)fd_kill(floor_savefile);
        safe_setuid_drop();
    }

    if (!(mode & SLF_SECOND))
    {
#ifdef SET_UID
# ifdef SECURE
        /* Drop "games" permissions */
        bePlayer();
# endif
#endif
    }

    return ok;
}
