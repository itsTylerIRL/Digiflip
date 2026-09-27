# DigiFlip

A virtual pet for the Flipper Zero, modelled closely on the Digimon Original Ver.20th handheld. Raise a Digimon from an egg in real time: feed it, train it, clean up after it, put it to bed, battle through the 100-round Colosseum, and see what it evolves into.

## Features

- **The full Ver.20th chart:** 15 eggs, 134 Digimon and 180 evolution routes, with the device's stage timers (1 minute, 10 minutes, 6, 24, 36 and 48 hours).
- **Care rules from the manual:** hunger and strength calls with a 10-minute window, nightly bedtimes, 3-hour naps, DP, waste that injures at four piles, medicine that can take several doses, and the manual's five causes of death.
- **Colosseum battles:** all 100 single rounds in order, from Kunemon to Lucemon: Falldown Mode, plus the 100-round tag Colosseum, where your pet and its slot-2 partner take on pairs. You mash OK to charge your attack, and the hit rate comes from power and attribute.
- **Two slots, Copymon and Jogress:** raise two pets at once, or put any Digimon you've raised in slot 2 as a Copymon. View pet 1, both, or pet 2, just like the DM20. Tag battles with the right partner unlock the Jogress evolutions.
- **Training:** single training is the Ver.20th mash game (13+ presses gives two strength hearts). Tag training is the high/low guessing game (3 of 5 shots past the guard).
- **Hand-drawn poses for all 134 Digimon:** they walk, chomp their food, cheer, shake their heads, doze, sleep, lunge and flinch.
- **Reactions to your care:** food eaten a bite at a time, a cheer after a flush, a bandage drop for medicine, and a name reveal on evolution.
- **Album:** every Digimon you've raised, kept across eggs, with Owned and All tabs.
- **Device-style screen:** icons along the top and bottom edges, a call icon that lights up, a dark screen with lights off, and the red LED blinking while the pet calls.
- **Real time, even while closed:** everything that happened while the app was shut is played through when you reopen it.
- **Crash-safe saves:** a dead battery mid-save can't corrupt your pet.
- **Logs and cheats:** a timestamped history of everything that happened, exportable for bug reports, plus switches for heart loss and poop and a force-evolution button.

## Install

1. Download `digiflip.fap` from the [Releases](../../releases) page. Pick the build that matches your firmware; see [Firmware compatibility](#firmware-compatibility).
2. Copy it to your Flipper's SD card under `apps/Games/`, using any of these:
   - the **Flipper mobile app**: File Manager → SD Card → apps → Games → upload,
   - **qFlipper** on a computer: File manager tab, then drag the file into `SD Card/apps/Games`,
   - **[lab.flipper.net](https://lab.flipper.net)** in a Chromium browser: Archive → apps → Games → upload.
3. On the Flipper, open **Apps → Games → Digimon - D20**.

It's a single file. The character art travels inside the `.fap` and unpacks to `apps_assets/digiflip/` on the SD card the first time you launch it.

### Firmware compatibility

Flipper apps are built against a specific firmware API version. Release builds target the current **official** firmware (release channel), with a second build for the dev channel. If the Flipper says the app is outdated or incompatible, update your firmware, or build from source against your firmware's SDK (below). Custom firmwares such as Momentum or Unleashed need a build against their own SDK.

## How to play

### Controls

The home screen follows the device: four icons along the top (status, food, train, battle) and four along the bottom (clean, light, medicine, and the call icon). The call icon stays dim until your pet needs you, then lights up and blinks.

| Button | Action |
| --- | --- |
| Left / Right | Step through the icons |
| Up / Down | Jump between the top and bottom rows |
| OK | Use the selected icon (mash it during training and battle charges) |
| Hold OK | On the home screen, switch the view: pet 1, both, or pet 2 |
| Back | Go back; on the home screen, open the menu |

The **menu** (Back from home) has Resume, View, Slot 1, Slot 2, Settings (sound, call light, Logs and Cheats), Album, and Save & Exit. Slot 1 and Slot 2 show who's in each slot. Opening one gives that slot's controls: Stats for a raised pet, Show only this pet, and Raise a new egg. Slot 2 also offers Use a Copymon and Empty the slot. Slot 1 always keeps a raised pet. A stray Back never quits. Your pet's clock keeps running while the menu is open.

### Looking after your pet

- **Calls:** when the call icon lights up, your pet is hungry, weak, or sleepy. Answer within 10 minutes (30 for sleepy), or it counts as a care mistake.
- **Food:** meat fills hunger hearts. Protein fills strength hearts and restores DP. Feeding past full is an overfeed, and it refuses more.
- **Training:** pick who trains: either pet solo, or a tag team led by either. Solo: mash OK before the timer runs out. Tag (needs something in slot 2): press Up to shoot high or Down to shoot low, and get 3 of 5 shots past the guard. Every session counts toward effort, pass or fail.
- **Battles:** pick who fights the same way: either pet solo, or a tag team led by either (the lead's Colosseum progress counts). Choices a pet can't make right now show why, such as "young" or "no DP". Each battle costs 1 DP. Wins move you up that Colosseum; losses let you retry the round. Tag battles pair your pet with slot 2 against two opponents: stop the sweeping cursor on a mark for a stronger attack, then mash OK. Battles and waste can injure your pet, and an injured pet can't battle until treated.
- **Two slots:** as on the DM20, slot 2 holds a second pet or a Copymon. Under Menu → Slot 2, raise a second egg, pick any Digimon from your album as a Copymon, or empty the slot. Menu → Slot 1 replaces the first pet the same way. Hold OK on the home screen (or use Menu → View) to show pet 1, both side by side, or pet 2. Care in the both view feeds, cleans, doses and turns the lights off for both pets at once. When a food would fill only one pet's hearts, only that pet eats. The Stats cards run through pet 1 and then pet 2, and Up/Down jumps between them. Both pets age, call and evolve independently. A second raised pet fights tag battles with its own hearts, pays its own DP, can be injured and can reach Jogress too. A Copymon fights at full strength, needs no care, and stays when you start a new egg. Stage VI+ Digimon evolve through Jogress: tag battles with a specific partner, for example Blitz Greymon with Cres Garurumon for Omegamon Alter S.
- **Lights:** at bedtime your pet gets sleepy and calls. Put it to bed within 30 minutes, or it counts as a care mistake and it falls asleep on its own with the lights on; press the light button to turn them off. Turning the lights off at other times starts a 3-hour nap. Sleep refills DP.
- **Growing up:** babies get hungry and poop the most. The pace slows as they grow, then picks up again once they're old (6+ days).
- **Cleaning:** four piles of waste injure your pet. Waste piles up on the far right, or with both pets on screen, on the far left for pet 1 and the far right for pet 2.
- **Evolution:** the care mistakes, training, overfeeds and battles in each stage decide which form comes next. Stage IV to V needs 15 battles: winning 12 of the last 15 makes it certain, and 6–11 wins still gives a chance (10% per win over 5).
- **Traited eggs:** if a Digimon stays awake for 48 hours after its latest evolution, it leaves a traited egg when it dies. The next egg you pick for that slot is traited, which adds 10% to its Stage V chance. The death screen and egg picker say when you've earned one, and Stats marks a traited Digimon.

**Digi-Eggs unlock** as you play, just like the DM20. The egg picker shows locked eggs dimmed, with what's needed and your progress:

| Egg | Unlocks when |
| --- | --- |
| Ver.1 | Open from the start |
| Ver.2–5 | Any Digimon reaches Child |
| Zuba / Hack | 50 / 100 battles won, counting every pet |
| Slayerdra / Breakdra | 5 / 25 different Digimon in the album |
| Corona, Luna, Taichi, Yamato, DORU, Meicoo | Open (the DM20 needs a device link for these) |

A new unlock is announced on the home screen and recorded in the log.

The **Album** opens on the Owned tab, which lists only the Digimon you've raised. Press OK to switch to All and see the whole roster, with unraised entries shown as "?". Left/Right step one entry; Up/Down jump five owned entries, or ten on All.

**Logs** (Menu → Settings) lists everything that has happened to your pet, newest first: hatching and evolutions (including when no route matched), calls, care mistakes, empty meters, poops, injuries, feeding, training, battles, sleep, medicine, deaths and cheat use. Events from while the app was closed are included, with the time they actually happened. Up/Down scroll, Left/Right page, and the bottom bar shows the selected event's full date and time. Press OK to save the whole log to `apps_data/digiflip/log.txt`, which is useful for bug reports.

**Cheats** (Menu → Settings) can switch off heart loss (hunger and strength stay put) and poop, refill both hearts, force an evolution, or open every Digi-Egg. Forcing picks the route your current stats qualify for, or the first route if none do, and hatches an egg straight away. The toggles are saved and apply to time replayed while the app was closed. Every cheat is recorded in the log.

On the Stats screen, Left/Right or OK moves between the Care, Evolution, Battle and Condition cards, continuing into the second pet's cards when both are on screen. On the egg picker, Left/Right browse and Up/Down jump five entries. Choosing an egg replaces your current pet, so if it's still alive the picker names it and asks for a second OK; Back leaves it alone.

## Build from source

Install [uFBT](https://github.com/flipperdevices/flipperzero-ufbt) (`pip install ufbt`), connect your Flipper, and run:

```sh
make fap      # build dist/digiflip.fap
make launch   # build, install and start it on a connected Flipper
```

`make fap` first writes the app's icons from their text sources. If you run `ufbt` directly, run `python3 tools/make_icons.py` once beforehand. `ufbt` downloads the official release SDK by default. To build for another firmware, point it at that firmware's SDK first, for example `ufbt update --channel=dev`, or `ufbt update --index-url=<your firmware's SDK index>` for a custom firmware.

## Development

```
src/core/       game rules, roster, Colosseum and album logic (no Flipper SDK; host-tested)
src/services/   SD-card storage, sound, and the sprite cache
src/app/        one file per screen, plus the main loop in digiflip.c
assets/poses/   character art as 18×18 text grids (the base sprite plus poses)
assets/icons/   icon art as text grids (eggs, effects, menu, Apps-list icon)
assets_sd/      character sprites shipped to the SD card (built from assets/poses)
images/         icon PNGs written from assets/icons by make (git-ignored)
tools/          asset builders (sprite pack, icons)
tests/          host tests for src/core
```

| Command | What it does |
| --- | --- |
| `make test` | Build and run the host tests for `src/core` |
| `make icons` | Write the icon PNGs from `assets/icons/` (done automatically by `make fap`) |
| `make assets` | Write the icons and rebuild the SD sprite pack (and its review sheet) |
| `make format` / `make lint` | Format or check the C code with the Flipper SDK's clang-format rules |
| `make fap` / `make launch` | Build the app / install and run it on a connected Flipper |

CI (`.github/workflows/build.yml`) runs the tests and lint and builds the `.fap` for the release and dev SDKs on every push. Pushing a `vX.Y.Z` tag that matches `fap_version` in `application.fam` publishes a GitHub release with both builds attached.

### Character art and poses

Character art lives on the SD card rather than in the app, so the whole roster's poses fit in the Flipper's memory. Each Digimon is one small file in `assets_sd/sprites/`, holding its 18×18 base sprite and up to seven poses (walk, happy, eat, refuse, sleep, attack, hurt). The app keeps only the few on screen in memory and doubles them to 36×36 as it draws.

All character art is 18×18 text grids (`#` is ink, `.` is paper): `assets/poses/<species>.txt` for the roster and `assets/poses/colosseum/<key>.txt` for Colosseum-only bosses. Each file has a required `[idle]` section (the Ver.20th base sprite) and optional pose sections. After editing, run `make assets` and check `assets/poses/preview.png`.

### Roster data

The roster, evolution routes and Colosseum ladders in `src/core/*_data.inc` are factual data compiled from the [Humulos Ver.20th guide](https://humulos.com/digimon/dm20/) and its [Colosseum page](https://humulos.com/digimon/dm20/battles/). The 16 Colosseum-only bosses have no published power, so each takes the power of the nearest earlier opponent. Edit the tables directly, then run `make test`.

## Saves

DigiFlip keeps these files in `apps_data/digiflip/` on the SD card:

- `save.bin`: your first pet,
- `save2.bin` and `slot2.bin`: slot 2 (a second pet or a Copymon),
- `album.bin`: every Digimon you've raised,
- `settings.bin`: sound, call-light and cheat settings,
- `records.bin`: battles won across every pet, and which egg unlocks have been announced,
- `log.bin`: the last 128 log events (`log.txt` appears when you save the log from the Logs screen).

The app saves every minute and when you exit. Each file is written to a temporary copy and then renamed into place, so an interrupted save can't damage it. Saves from version 0.5 onward are converted automatically.

To back up your pets, copy `save.bin`, `save2.bin`, `slot2.bin` (and `album.bin`) off the SD card with any of the tools above. Copy them back to restore.

## Mechanics reference

- **Battles:** hit chance is `P×100/(P+E)`, the formula used across the DM family of virtual pets. Power is base power plus 4 per strength heart (up to +16, lost entirely at 99G), and an attribute advantage (Vaccine > Virus > Data > Vaccine) adds 32. The charge meter picks the attack, from weakest to strongest: Weak, Strong, Double, Double Strong or Critical. Max strength adds +1 damage in single battles.
- **Training:** 13+ presses succeeds, gives two strength hearts, and sheds 1–4G. Four sessions fill one effort heart.
- **Tag battles:** the pet fights the left opponent and its slot-2 partner the right, each with its own hit chance. Each team shares an HP pool, and max strength adds +2 damage per Digimon.
- **Care:** an unanswered call is a care mistake after 10 minutes. Naps wake on their own after 3 hours, and 3+ hours of sleep restores DP. Healing an injury can take more than one dose.
- **Bedtime:** a tired Digimon must be put to bed within 30 minutes.
- **Tuned estimates:** the manual doesn't give heart and waste rates by stage and age, the battle DP cost, injury odds, the training, charge and aim windows, the battle HP/damage model, the Stage V odds for 6–11 wins, or the size of the trait bonus. DigiFlip picks its own values for these, set as constants in `src/core/digiflip_game.h`.

### Sources

- Evolution routes, the roster and care rules: the [Humulos Ver.20th evolution guide](https://humulos.com/digimon/dm20/) and its transcription of the [Ver.20th manual](https://humulos.com/digimon/dm20/manual/). The manual is the source for the call timeout, nap length, sleep and DP, the power formula, training, attack ranks, strength damage bonuses and multi-dose healing.
- Single and tag Colosseum ladders: the [Humulos Colosseum page](https://humulos.com/digimon/dm20/battles/).
- Egg unlock conditions, the 6-win floor for a Stage V chance, and traited eggs: the Humulos guide's egg list and FAQ.
- The 30-minute bedtime window: community testing reported on the [With the Will](https://withthewill.net/) Digimon forums. The manual doesn't give it.
- Calendar dates in the log use Howard Hinnant's [`civil_from_days`](https://howardhinnant.github.io/date_algorithms.html) algorithm.

## Roadmap

- Connecting with another Flipper (or a real DM20) for battles, Copymon trades and the link-based egg unlocks.
- Original replacement artwork for the character sprites.

## Credits and legal

DigiFlip is an unofficial fan project and isn't affiliated with or endorsed by Bandai or Toei Animation. Digimon and its characters are owned by Bandai Co., Ltd., Akiyoshi Hongo and Toei Animation Co., Ltd.

**Character art.** The Digi-Egg icons (`assets/icons/eggs.txt`) and the character base sprites (the `[idle]` sections in `assets/poses/`, with Colosseum-only opponents in `assets/poses/colosseum/`) were converted from the dot sprites referenced by the [Humulos Ver.20th guide](https://humulos.com/digimon/dm20/). They are copyrighted character artwork, not original DigiFlip assets. The hand-drawn poses (walk, happy, eat, refuse, sleep, attack, hurt) are edited derivatives of those base sprites, drawn at the same native 18×18 resolution, so they aren't original DigiFlip artwork either.

**Original art.** The reaction effects (`assets/icons/effects.txt`) were generated with OpenAI's image-generation tool, then cropped by hand and converted to 1-bit pixels. The menu icons and the Apps-list icon (`assets/icons/menu.txt`) are original DigiFlip pixel art, with the menu icons modelled on the Ver.20th's icon set.

**Data.** Evolution, care and Colosseum data were compiled from [Humulos](https://humulos.com/digimon/dm20/). See [Sources](#sources).

**License:** not yet chosen. Until one is added, all rights to the DigiFlip code are reserved.
