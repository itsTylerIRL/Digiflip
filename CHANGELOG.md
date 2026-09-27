# Changelog

## 0.12.0

- **Egg unlocks:** Ver.1 is open from the start. Ver.2–5 open once any Digimon reaches Child. Zuba and Hack need 50 and 100 battle wins, counted across every pet. Slayerdra and Breakdra need 5 and 25 album entries. The six link-only eggs stay open. Locked eggs show dimmed in the picker with their progress ("Win 50 battles, Progress 23/50"). New unlocks are announced on the home screen and logged. Existing players start from their pets' current wins.
- **Stage V chances:** winning 12 of the last 15 battles still guarantees Stage V. Now 6–11 wins give a 10–60% chance, and the log shows the roll ("Evolved to MetalGreymon (30%)", "Missed Stage V (30%)").
- **Traited eggs:** a Digimon awake 48 hours after its latest evolution leaves a traited egg when it dies. It's the next egg for that slot and adds 10% to the Stage V chance. It's shown on the death screen, the egg picker and Stats.
- **Menu:** Resume, View, Slot 1, Slot 2, Settings, Album, Save & Exit. Slot 1 and Slot 2 show each slot's Digimon and open the same slot screen: Stats, Show only this pet and Raise a new egg, plus Use a Copymon and Empty the slot for slot 2. Change Digi-Egg now lives under Slot 1.
- **Other changes:** new "All eggs" cheat. Saves move to version 7, settings to version 3, and the new `records.bin` holds the device-wide win count; older files convert automatically.

## 0.11.0

- **New name in the Apps menu:** "Digimon - D20". Saves stay in `apps_data/digiflip`.
- **Second slot:** as on the DM20, slot 2 holds a second raised pet or a Copymon (Menu → Slot 2: raise a new egg, use a Copymon, or empty it). Both pets live, call, evolve and die on their own clocks.
- **Views 1 / 1&2 / 2:** hold OK on the home screen (or Menu → View) to show one pet or both side by side, each walking its own half of the screen without overlapping. Pet 1's waste piles up on the far left and pet 2's on the far right, so neither gets boxed in. Care in the both view applies to both pets, and the Feed screen shows both pets' hearts. If a food would fill only one pet's hearts, only that pet eats, so the full one isn't overfed.
- **Pick who plays:** Train and Battle list every option in any view: solo with pet 1, solo with pet 2, or a tag team led by either. Options a pet can't do right now (too young, asleep, injured, no DP) stay listed with the reason, so a baby in one slot never benches the other.
- **See-through sprites:** the blank space around a Digimon is now transparent and only its body is solid. Tag fighters, effects and waste overlap cleanly instead of cutting white boxes into each other.
- **Team tag battles:** a raised second pet fights with its own hearts, pays DP, can be injured, and counts the battle toward its own Jogress routes. Tag training trains both pets.
- **Other changes:** Code comments cut to game logic only. Every source and attribution now lives in the README (Mechanics reference → Sources, and Credits). All art is now text grids with no PNGs in the repo. Each character's base sprite moved into its pose file as an `[idle]` section. The egg, effect, menu and app icons live in `assets/icons/`, and `make` writes their PNGs (git ignores them). The built app is byte-identical. Logs and Cheats moved into Settings to shorten the menu. The menu names who's in slot 2, and "Change Digi-Egg" reads "New egg for pet 1" when there are two pets. Replacing a living pet with a new egg now takes a second OK. With nobody in slot 2, Train and Battle skip the picker. The Stats cards run through pet 1 and then pet 2 (Up/Down jumps between them). Logs tag second-pet events with "2:" ("[pet 2]" in `log.txt`). A second pet's death offers a new egg or an empty slot. The Copymon now lives in slot 2 (`slot2.bin`) instead of `copymon.bin`.

## 0.10.0

- **Copymon:** pick any Digimon you've raised as your pet's tag partner (menu → Copymon). It's kept across eggs.
- **Tag Colosseum:** all 100 rounds, with your pet and Copymon against pairs of opponents. It adds the DM20's aim step: stop the cursor on a mark (Perfect or Good), then mash. Four new Colosseum-only bosses.
- **Tag training:** the high/low game; 3 of 5 shots past the guard succeeds.
- **Jogress:** tag battles with the right Copymon now trigger the Stage VI+ fusion evolutions, such as Blitz Greymon with Cres Garurumon for Omegamon Alter S.
- **Bedtime:** if you don't put your pet to bed within 30 minutes, that's a care mistake, and it falls asleep on its own with the lights on. The light button turns them off without waking it.
- **Needs change with age:** babies get hungry and poop the most, adults less, and old Digimon (6+ days) more often again.
- **Other changes:** Train and Battle now offer solo or tag. The Status battle card shows the tag round. Saves move to version 6; older saves convert automatically.

## 0.9.0

- **Logs:** a timestamped history of everything that happens to the pet. It covers hatching and evolutions (including when no route matched), calls, care mistakes, empty meters, poops, injuries, feeding, training, battles, sleep, medicine, deaths and cheats. Events from while the app was closed carry their real time. It keeps the last 128 events, and OK exports them to `log.txt`.
- **Cheats:** switch off heart loss and poop, refill hearts, or force an evolution. Cheats are saved with your settings and recorded in the log.
- The menu now scrolls, with a scrollbar, to fit its seven entries.

## 0.8.0

- **Every Digimon has hand-drawn poses:** all 134 across every egg, 918 frames in all.
- **Character art moved to the SD card.** Each Digimon is one small file, and only the ones on screen are held in memory, which cut the app's RAM use from 76 KB to 37 KB. The files travel inside the `.fap` and unpack on first launch.
- **Album:** every Digimon you've raised, browsable from the menu and kept across eggs. The Owned tab (the default) lists only what you've raised; press OK to switch to All. "NEW!" appears when you evolve into something for the first time.
- **More reactions:** the pet cheers after a flush. Medicine drops a bandage, then the pet perks up or winces depending on the dose. It shakes its head at medicine it doesn't need.
- **Evolution name reveal:** the new form's name appears when it evolves.
- **New Apps-list icon:** an original, bold 10×10 creature that reads at a glance. It replaces the old shrunken sprite.
- **Crash-safe saves:** the pet save, settings and album are written to a temporary file and renamed into place, with automatic recovery.
- **Project restructured** into `src/core`, `src/services` and `src/app` (one file per screen). Added host tests for the album, a CI workflow, and a rewritten README.

## 0.7.0

- **Hand-drawn poses for the Version 1 line:** walk, happy, eat, refuse, sleep, attack and hurt.
- **Device-style home screen:** 12px Ver.20th icons (scale, meat, barbell, trophy, poop, bulb, bandage) along the top and bottom edges, with a call icon that lights up. Up/Down switches rows.
- **Back opens a menu** (Resume, Settings, Change Digi-Egg, Save & Exit) instead of quitting. Settings covers sound and the call light.
- **Heart icons** replace the square meters.
- **Sprites mirror as they're drawn** instead of being stored twice, which fixed a "Not enough memory" launch failure. The stack grew to 4 KB.

## 0.6.0

- **The 100-round Colosseum**, with the real opponents, powers and attributes. Hit chance is `P×100/(P+E)`, an attribute advantage adds 32, and the strength bonus is lost at 99G. You mash OK to charge Weak through Critical attacks.
- **Single training** became the Ver.20th mash game: 13+ presses gives two strength hearts.
- **New touches:** a dark screen with lights off, the flush wave, a head-shake at overfeeding, the call LED and triple beep, lifetime win %, and a gravestone.
- **Fixes:**
  - Battle state was changed from the draw thread; state is now guarded by a mutex.
  - The battle opponent was cosmetic and didn't affect the outcome.
  - Battle sprites faced away from each other.
- **Saves:** version 0.5 saves are converted automatically.

## 0.5.0

- Data-driven mirror of the Ver.20th chart: 15 eggs, 134 Digimon and 180 evolution routes, with the device's stage timers.
- Care rules, sleep, injury and death from the manual. Offline progress is replayed on launch.
