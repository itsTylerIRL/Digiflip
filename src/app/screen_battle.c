#include "digiflip_app.h"

/* Where each fighter stands. Single battles use only the pet and the left
   opponent; tag battles add the Copymon and the right opponent behind them. */
static int16_t digiflip_fighter_x(bool tag, uint8_t fighter) {
    if(!tag)
        return fighter == DigiflipFighterPet ? DIGIFLIP_BATTLE_PLAYER_X : DIGIFLIP_BATTLE_ENEMY_X;
    switch(fighter) {
    case DigiflipFighterPet:
        return 2;
    case DigiflipFighterPartner:
        return 24;
    case DigiflipFighterRight:
        return 68;
    default:
        return 90;
    }
}

static bool digiflip_fighter_is_player(uint8_t fighter) {
    return fighter == DigiflipFighterPet || fighter == DigiflipFighterPartner;
}

/* Each fighter trades blows with the one in its lane. */
static uint8_t digiflip_fighter_target(uint8_t fighter) {
    switch(fighter) {
    case DigiflipFighterPet:
        return DigiflipFighterLeft;
    case DigiflipFighterPartner:
        return DigiflipFighterRight;
    case DigiflipFighterLeft:
        return DigiflipFighterPet;
    default:
        return DigiflipFighterPartner;
    }
}

static const DigiflipColosseumOpponent* digiflip_next_left(const DigiflipApp* app) {
    if(app->battle_tag) {
        const DigiflipTagRound* round = digiflip_game_next_tag_round(app->game);
        return round ? &round->left : NULL;
    }
    return digiflip_game_next_opponent(app->game);
}

static const DigiflipColosseumOpponent* digiflip_next_right(const DigiflipApp* app) {
    if(!app->battle_tag) return NULL;
    const DigiflipTagRound* round = digiflip_game_next_tag_round(app->game);
    return round ? &round->right : NULL;
}

static uint8_t digiflip_next_round_number(const DigiflipApp* app) {
    const uint8_t round = app->battle_tag ? app->game->tag_round : app->game->battle_round;
    return (uint8_t)(round % DIGIFLIP_COLOSSEUM_ROUNDS + 1U);
}

static void digiflip_draw_lineup(Canvas* canvas, const DigiflipApp* app, int16_t y) {
    const bool pump = (app->mash_presses & 1U) != 0;
    const DigiflipPose pose = pump ? DigiflipPoseWalk : DigiflipPoseIdle;
    const bool tag = app->battle_tag;
    if(tag) {
        digiflip_draw_character(
            canvas,
            digiflip_fighter_x(true, DigiflipFighterPartner),
            y,
            digiflip_mate_species(app),
            pose,
            true);
        digiflip_draw_character(
            canvas,
            digiflip_fighter_x(true, DigiflipFighterRight),
            y,
            digiflip_opponent_sprite_id(digiflip_next_right(app)),
            DigiflipPoseIdle,
            false);
    }
    digiflip_draw_character(
        canvas, digiflip_fighter_x(tag, DigiflipFighterPet), y, app->game->species_id, pose, true);
    digiflip_draw_character(
        canvas,
        digiflip_fighter_x(tag, DigiflipFighterLeft),
        y,
        digiflip_opponent_sprite_id(digiflip_next_left(app)),
        DigiflipPoseIdle,
        false);
}

void digiflip_draw_battle_intro(Canvas* canvas, const DigiflipApp* app) {
    char buffer[48];
    const DigiflipColosseumOpponent* left = digiflip_next_left(app);
    const DigiflipColosseumOpponent* right = digiflip_next_right(app);
    canvas_set_font(canvas, FontPrimary);
    snprintf(
        buffer,
        sizeof(buffer),
        "%sROUND %u",
        app->battle_tag ? "TAG " : "",
        digiflip_next_round_number(app));
    canvas_draw_str(canvas, 2, 10, buffer);
    canvas_set_font(canvas, FontSecondary);
    snprintf(
        buffer,
        sizeof(buffer),
        "DP %u/%u",
        digiflip_game_dp_quarters(app->game) / 4U,
        digiflip_game_dp_max(app->game) / 4U);
    canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignBottom, buffer);
    digiflip_draw_lineup(canvas, app, 12);
    if(!app->battle_tag) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "VS");
        canvas_set_font(canvas, FontSecondary);
    }
    if(right) {
        snprintf(buffer, sizeof(buffer), "%s & %s", left ? left->name : "???", right->name);
    } else {
        snprintf(buffer, sizeof(buffer), "%s", left ? left->name : "???");
    }
    canvas_draw_str_aligned(canvas, 64, 55, AlignCenter, AlignBottom, buffer);
    canvas_draw_str_aligned(canvas, 64, 64, AlignCenter, AlignBottom, "OK fight   BACK leave");
}

static const char* digiflip_sync_name(DigiflipSync sync) {
    switch(sync) {
    case DigiflipSyncPerfect:
        return "Perfect!";
    case DigiflipSyncNear:
        return "Good";
    default:
        return "Miss";
    }
}

/* Battle charge: mash to fill the meter; the fill picks the attack rank
   (a tag battle's cursor stop nudges it up or down). */
void digiflip_draw_battle_charge(Canvas* canvas, const DigiflipApp* app) {
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "CHARGE!");
    canvas_set_font(canvas, FontSecondary);
    int rank = (int)digiflip_battle_attack_for_presses(app->mash_presses);
    if(app->battle_tag) {
        rank += app->battle_sync == DigiflipSyncPerfect ?
                    1 :
                    (app->battle_sync == DigiflipSyncMiss ? -1 : 0);
        if(rank < (int)DigiflipAttackWeak) rank = DigiflipAttackWeak;
        if(rank > (int)DigiflipAttackCritical) rank = DigiflipAttackCritical;
        canvas_draw_str_aligned(
            canvas, 64, 10, AlignCenter, AlignBottom, digiflip_sync_name(app->battle_sync));
    }
    canvas_draw_str_aligned(
        canvas,
        126,
        10,
        AlignRight,
        AlignBottom,
        digiflip_battle_attack_name((DigiflipAttack)rank));
    digiflip_draw_lineup(canvas, app, 12);
    if(app->mash_start_tick == 0 && ((furi_get_tick() / 400U) & 1U) == 0) {
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Mash OK!");
    }
    digiflip_draw_meter(canvas, 50, app->mash_presses, DIGIFLIP_BATTLE_CHARGE_PRESSES, 0);
    digiflip_draw_mash_timer(canvas, app, DIGIFLIP_BATTLE_CHARGE_MS);
}

static uint32_t digiflip_battle_length_ms(const DigiflipBattleResult* battle) {
    return DIGIFLIP_BATTLE_INTRO_MS + battle->exchange_count * DIGIFLIP_BATTLE_EXCHANGE_MS +
           DIGIFLIP_BATTLE_KO_MS;
}

static uint8_t digiflip_battle_max_hp(const DigiflipBattleResult* battle) {
    return battle->tag ? DIGIFLIP_TAG_BATTLE_HP : DIGIFLIP_BATTLE_HP;
}

/* Remaining HP for both sides after every impact up to `elapsed`. */
static void digiflip_battle_hp(
    const DigiflipBattleResult* battle,
    uint32_t elapsed,
    uint8_t* player_hp,
    uint8_t* enemy_hp) {
    *player_hp = digiflip_battle_max_hp(battle);
    *enemy_hp = digiflip_battle_max_hp(battle);
    for(uint8_t i = 0; i < battle->exchange_count; i++) {
        const uint32_t impact =
            DIGIFLIP_BATTLE_INTRO_MS + i * DIGIFLIP_BATTLE_EXCHANGE_MS + DIGIFLIP_BATTLE_IMPACT_MS;
        if(elapsed < impact) break;
        const DigiflipBattleExchange* exchange = &battle->exchanges[i];
        if(!exchange->hit) continue;
        uint8_t* hp = exchange->by_player ? enemy_hp : player_hp;
        *hp = *hp > exchange->damage ? (uint8_t)(*hp - exchange->damage) : 0U;
    }
}

static void digiflip_draw_hp_bar(Canvas* canvas, uint8_t x, uint8_t hp, uint8_t max) {
    canvas_draw_frame(canvas, x, 2, 40, 6);
    const uint8_t fill = (uint8_t)((uint32_t)hp * 38U / max);
    if(fill) canvas_draw_box(canvas, x + 1, 3, fill, 4);
}

static DigiflipSpriteId
    digiflip_fighter_sprite(const DigiflipBattleResult* battle, uint8_t fighter) {
    switch(fighter) {
    case DigiflipFighterPet:
        return battle->player_species;
    case DigiflipFighterPartner:
        return battle->partner_species;
    case DigiflipFighterLeft:
        return digiflip_opponent_sprite_id(battle->opponent);
    default:
        return digiflip_opponent_sprite_id(battle->opponent2);
    }
}

void digiflip_draw_battle(Canvas* canvas, const DigiflipApp* app) {
    const DigiflipBattleResult* battle = &app->battle;
    const bool tag = battle->tag;
    const uint32_t elapsed = furi_get_tick() - app->battle_start_tick;
    uint8_t player_hp;
    uint8_t enemy_hp;
    digiflip_battle_hp(battle, elapsed, &player_hp, &enemy_hp);
    digiflip_draw_hp_bar(canvas, 2, player_hp, digiflip_battle_max_hp(battle));
    digiflip_draw_hp_bar(canvas, 86, enemy_hp, digiflip_battle_max_hp(battle));
    canvas_set_font(canvas, FontSecondary);
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%s%u", tag ? "T" : "R", battle->round + 1U);
    canvas_draw_str_aligned(canvas, 64, 8, AlignCenter, AlignBottom, buffer);

    int16_t y[4] = {DIGIFLIP_BATTLE_Y, DIGIFLIP_BATTLE_Y, DIGIFLIP_BATTLE_Y, DIGIFLIP_BATTLE_Y};
    bool show[4] = {true, tag, true, tag};
    DigiflipPose pose[4] = {
        DigiflipPoseIdle, DigiflipPoseIdle, DigiflipPoseIdle, DigiflipPoseIdle};
    int8_t front = -1;
    const uint32_t fight_end =
        DIGIFLIP_BATTLE_INTRO_MS + battle->exchange_count * DIGIFLIP_BATTLE_EXCHANGE_MS;

    if(elapsed >= DIGIFLIP_BATTLE_INTRO_MS && elapsed < fight_end) {
        const uint32_t t = elapsed - DIGIFLIP_BATTLE_INTRO_MS;
        const DigiflipBattleExchange* exchange =
            &battle->exchanges[t / DIGIFLIP_BATTLE_EXCHANGE_MS];
        const uint32_t phase = t % DIGIFLIP_BATTLE_EXCHANGE_MS;
        const uint8_t attacker = exchange->attacker;
        const uint8_t target = digiflip_fighter_target(attacker);
        front = (int8_t)attacker;
        const int16_t ax = digiflip_fighter_x(tag, attacker);
        const int16_t tx = digiflip_fighter_x(tag, target);
        const int16_t from = exchange->by_player ? ax + 34 : ax - 4;
        const int16_t to = exchange->by_player ? tx + 8 : tx + 26;
        uint8_t radius = 2;
        bool twin = false;
        if(exchange->by_player) {
            const DigiflipAttack attack = battle->attack;
            const bool strong = attack == DigiflipAttackStrong ||
                                attack == DigiflipAttackDoubleStrong;
            radius = attack == DigiflipAttackCritical ? 4 : strong ? 3 : 2;
            twin = attack == DigiflipAttackDoubleWeak || attack == DigiflipAttackDoubleStrong;
        }
        if(phase < DIGIFLIP_BATTLE_IMPACT_MS) pose[attacker] = DigiflipPoseAttack;
        if(phase < DIGIFLIP_BATTLE_IMPACT_MS || !exchange->hit) {
            const int32_t span =
                (int32_t)(to - from) * (int32_t)phase / (int32_t)DIGIFLIP_BATTLE_IMPACT_MS;
            const int16_t sx = (int16_t)(from + span);
            if(sx > -8 && sx < 136) {
                canvas_draw_disc(canvas, sx, 30, radius);
                if(twin) canvas_draw_disc(canvas, sx - (to > from ? 9 : -9), 30, radius);
            }
            if(!exchange->hit && phase >= DIGIFLIP_BATTLE_IMPACT_MS / 2U) y[target] -= 6;
        } else {
            show[target] = ((phase / 50U) & 1U) == 0;
            pose[target] = DigiflipPoseHurt;
            canvas_draw_icon(
                canvas,
                tx + (digiflip_fighter_is_player(target) ? 12 : 4),
                22,
                digiflip_effect_sprite(DigiflipEffectSparkle));
        }
    } else if(elapsed >= fight_end) {
        const uint32_t t = elapsed - fight_end;
        const bool blink_off = ((t / 200U) & 1U) != 0;
        const int16_t hop = ((t / 250U) & 1U) == 0 ? -5 : 0;
        for(uint8_t f = 0; f < 4; f++) {
            const bool winner = digiflip_fighter_is_player(f) == battle->victory;
            if(winner) {
                pose[f] = DigiflipPoseHappy;
                y[f] += hop;
            } else {
                pose[f] = DigiflipPoseHurt;
                show[f] = show[f] && !blink_off && t < DIGIFLIP_BATTLE_KO_MS / 2U;
            }
        }
    }

    static const uint8_t order[4] = {
        DigiflipFighterPartner, DigiflipFighterRight, DigiflipFighterPet, DigiflipFighterLeft};
    for(uint8_t i = 0; i < 4; i++) {
        const uint8_t f = order[i];
        if(!show[f] || (int8_t)f == front) continue;
        digiflip_draw_character(
            canvas,
            digiflip_fighter_x(tag, f),
            y[f],
            digiflip_fighter_sprite(battle, f),
            pose[f],
            digiflip_fighter_is_player(f));
    }
    if(front >= 0 && show[front]) {
        digiflip_draw_character(
            canvas,
            digiflip_fighter_x(tag, (uint8_t)front),
            y[front],
            digiflip_fighter_sprite(battle, (uint8_t)front),
            pose[front],
            digiflip_fighter_is_player((uint8_t)front));
    }
}

void digiflip_start_battle(DigiflipApp* app) {
    const bool fought = app->battle_tag ?
                            digiflip_game_tag_battle_team(
                                app->game,
                                digiflip_mate_game(app),
                                digiflip_mate_species(app),
                                app->mash_presses,
                                app->battle_sync,
                                &app->battle) :
                            digiflip_game_battle(app->game, app->mash_presses, &app->battle);
    if(!fought) {
        digiflip_refuse(app, "Can't battle now");
        return;
    }
    if(app->battle.victory) digiflip_record_win(app);
    app->battle_start_tick = furi_get_tick();
    app->battle_sound_step = 0;
    app->screen = DigiflipScreenBattle;
}

void digiflip_finish_battle(DigiflipApp* app) {
    const DigiflipBattleResult* battle = &app->battle;
    const uint8_t next = battle->tag ? app->game->tag_round : app->game->battle_round;
    app->battle_start_tick = 0;
    digiflip_sound_play(
        app->notification, battle->victory ? DigiflipSoundVictory : DigiflipSoundFailure);
    digiflip_show_notice(app, battle->victory ? "Victory!" : "Defeat...");
    if((battle->evolved || battle->mate_evolved) && battle->tag) {
        snprintf(app->notice_detail_buffer, sizeof(app->notice_detail_buffer), "Jogress!");
    } else if(battle->injured && battle->mate_injured) {
        snprintf(app->notice_detail_buffer, sizeof(app->notice_detail_buffer), "Both injured!");
    } else if(battle->injured || battle->mate_injured) {
        snprintf(
            app->notice_detail_buffer,
            sizeof(app->notice_detail_buffer),
            "%s injured!",
            battle->injured ? "Lead" : "Partner");
    } else if(battle->victory && next == 0) {
        snprintf(
            app->notice_detail_buffer,
            sizeof(app->notice_detail_buffer),
            battle->tag ? "Tag Colosseum cleared!" : "Colosseum cleared!");
    } else {
        snprintf(
            app->notice_detail_buffer,
            sizeof(app->notice_detail_buffer),
            "Next: %sround %u   %u%% wins",
            battle->tag ? "tag " : "",
            next + 1U,
            digiflip_game_win_percent(app->game));
    }
    app->notice_detail = app->notice_detail_buffer;
}

void digiflip_update_battle(DigiflipApp* app, uint32_t tick) {
    const uint32_t elapsed = tick - app->battle_start_tick;
    while(app->battle_sound_step < app->battle.exchange_count) {
        const uint32_t impact = DIGIFLIP_BATTLE_INTRO_MS +
                                app->battle_sound_step * DIGIFLIP_BATTLE_EXCHANGE_MS +
                                DIGIFLIP_BATTLE_IMPACT_MS;
        if(elapsed < impact) break;
        if(app->battle.exchanges[app->battle_sound_step].hit) {
            digiflip_sound_play(app->notification, DigiflipSoundHit);
        }
        app->battle_sound_step++;
    }
    if(elapsed >= digiflip_battle_length_ms(&app->battle)) digiflip_finish_battle(app);
}

void digiflip_battle_intro_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyBack) {
        app->screen = DigiflipScreenHome;
    } else if(input->key == InputKeyOk) {
        digiflip_sound_play(app->notification, DigiflipSoundClick);
        if(app->battle_tag) {
            /* Tag battles aim first, then charge. */
            app->sync_cursor = 0;
            app->sync_direction = 1;
            app->screen = DigiflipScreenTagSync;
        } else {
            digiflip_start_mash(app, DigiflipScreenBattleCharge);
        }
    }
}

void digiflip_battle_input(DigiflipApp* app, const InputEvent* input) {
    if(input->key == InputKeyOk || input->key == InputKeyBack) digiflip_finish_battle(app);
}
