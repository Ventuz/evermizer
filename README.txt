HOWTO:
0. ROM has to be NTSC/US and can be a 3MB .SFC file with or without 512B header
1. Visit the in-browser version, unpack the exe or compile main.c
2. Drag and drop your rom into the in-browser field or onto the .exe
3a In-browser version should present you checkboxes in your browser and offer
   the patched ROM as "download" or show messages in an alert box.
3b Exe should ask you for settings and spit out success or error in a "dos"
   window, create a file starting with "Evermizer" in source ROM's directory
   and print out the complete filename.
4. Checksum is wrong, but neither sd2snes nor (most) emulators seem to care
5. You can skip the intro by pressing start (even without a prior save)
6. You have to loot gourds/chests or kill Thraxx to get a weapon that can cut
   grass to get access to the Windwalker.
7. If you have questions or need help, continue reading below, check the guide
   at https://github.com/black-sliver/evermizer/blob/master/guide.md
   or join the discord.

CHANGES SO FAR:
  This is in an early but working state.
  Crude user interface to select seed and settings
    Default output filename includes settings and seed number
    Option to save a spoiler log
  Command line interface:
    Allow batch processing
    Allow to specify complete output filename with -o
    Allow to specify output directory with -d
    Run the program without any file/arguments to see possible arguments
    Run the program in UI mode to see possible settings
  Alchemzer: randomize where you get which alchemy and
    Make sure Atlas is obtainable and usable in Easy/+Chaos and Normal/+Chaos
    Make sure Levitate and Revealer is obtainable and usable when required
    Easy will give you one formula with buyable ingredients pre-thraxx
    Newly learned alchemy is now preselected/highlighted for alchemy selection
    (Texts still unchanged)
  Ingredienizer:
    Randomize ingredient requirements
    Scales with difficulty
    Goes crazy with chaos
  Boss dropamizer: randomize what each boss drops and
    Horace will not steal your DEs
    Thraxx will always drop a weapon for now
    Bridges in pyramid won't collapse
    Horace spawn should be fixed (tied to DEs in inventory or DEs lost)
    Horace dialog should correcly indicate how many DEs you have
    Tinker taking away your diamond eyes sets flag to not lock-out Aegis
    If gourdomizer is on as well, you can go back through Big Bug after Thraxx
  Gourdomizer: shuffle gourd/pot/chest drops, no gourds are missable
  Sniffamizer: non-chaos shuffles, chaos randomizes dog-sniff ingredients
  Doggomizer: swap (non-chaos) or randomize (chaos) dogs
    Act1-3 only because toaster evasion sound hardlocks for some music tracks
  Pupdunk: Act0 doggo everywhere!
  Musicmizer: randomize music, in testing stage
  Fix sequence:
    Desolarify: Act1 first rock skip is disabled (Levitation required)
    Desaturnate: require the use of teleporter before final boss hatch is open
    Disable double gauge: don't take away rocket parts until you have them all
  Fix cheats:
    Remove infinite call bead glitch
  Glitchless beatable:
    Never require glitches to beat the game, even if glitches are not disabled
  All accessible:
    Make sure all checks/areas/bosses can be reached
  Fix infinite ammo:
    Fixes the bug that Particle Bomb and Cryo-Blast do not drain
  Fix atlas glitch:
    Fixes stat underflow when dying with active stat boosts
  Double Money:
    Enemies give twice as much money
  Double Exp:
    Enemies give twice as much experience points
    Weapons and formulas level twice as fast
  Open world: Unlock windwalker in every fire pit (slashing weapon required)
    Grant access to inner volcano
    Grant access to east swamp
    Fix palace cutscenes
    Lock Barrier (Horace inside palace) + gates behind Aegis (instead of WW)
    Make both Act3 towns filled with people
    Moved ticket lady bazooka shells behind dog race (in both towns)
    Fixed lance not appearing. He now appears in ivory as well as ebon keep
      Still not 100%, look into this again, weird camera glitch in Act3 houses
    Locked east draw bridge behind Mungola
    Disabled exit from Naris to chessboard if Mungola is alive
    Fixed reverse Timberdrake
    Disabled exit from castle to Verminator if Verminator is not dead
    Fixed prison sound crash
    Killing Verminator is optional, prison door unlocks when prison is cleared
    Keep dog after Magmar (room change required to make him visible)
    Play Tinker WW cutscene once, regardless of progress
    Landing skip converted to a regular landing
    Act2 dog statue and dialogues moved behind aegis
    Vigor fight won't start if you did not jump down the cliff at blimp
    Act1 Fire Eyes cutscenes play in order (dog naming skip as in vanilla)
    Act1 top-of-volcano guy fixed
    Act3 Mungola HP / fight fixed
    Act1 move Coleoptera behind talking to FE after killing Thraxx
    Aquagoth is accessible once. Oglin cave gourd can't be randomized
    Madrinus will (still) spawn after Aegis is defeated
    Move call bead glitch behind rocket
    Don't despawn North Market rock when Aegis is dead
    Replaced FE Village Windwalker checks with Magmar-dead checks
    Progress Act3 NPCs based on Castle crumbled instead of WW (ongoing effort)
  Vanilla bugs fixed:
    Fix softlock when entering west prison with dead dog
    Fix softlock when opening mosquito cell last (exit will be bottom left cell)
    Fix softlock when you have more than 3 rocket parts
    Fix texts disappearing when you fall down in Ruins NW room
    Removed option to save at Madronius' Brother
    Fix softlock when leaving tiny's lair
    Saving with Atlas, Defend or Speed does not ruin your stats
  Fix missables:
    Directly reward Queen's Key to inventory
    Make Revealer and Escape still obtainable when Horace is gone
    All rooms with gourds/chests/pots can be re-entered with gourdomizer on
  Some quality of life improvements are always enabled, most notably:
    Preselect the correct spell in alchemy selection screen
    Bazooka charge/energize and stat display fix
    Multiple level-ups from one enemy
  Uses tinymt64 for platform-independend RNG. See tinymt64.h for lincense
  Allow for ROM to grow to 4MB if required
  Allow skipping of intro with no prior save file


STILL TO BE FIXED:
  (with feature set outlined above)
  Show info to the player if market timer is over but vigor won't spawn yet
  Act3 can have camera location bugs in houses until blindly talked to Lance
  Put rocket behind mungola / change tinker progression?
  Remove some cutscenes?
  Desaturnate 2.0: make sure *NO* hatches can be skipped
  Remove unnecessary parts of first WW/open world patch


LINUX AND MAC BUILD INSTRUCTIONS:
  run make

WINDOWS BUILD INSTRUCTIONS:
  Run make for mingw, or create a VS project with main.c as (only) source file.
  The github source tree requires you to have python3 in PATH to build gen.h.
  The zip distribution includes a pre-built gen.h.

EMSCRIPTEN BUILD INSTRUCTIONS:
  Make sure emscripten/emcc is configured and in PATH. run make wasm
  You may need to set some env variables, like EM_CONFIG=~/.emscripten

CREDITS:
  Written by black_sliver
  Some patches by assassin17 - http://assassin17.brinkster.net
  Thanks to all people pushing or commenting on the project, providing ideas or
    artwork, testing it, pointing out bugs and looking into SNES/SoE stuff
  ayame li, colin, cyb3r, darkmoon2321, dot, esmo88, flarezenyu, fronk,
  greenambler, metasigma, mirapoix, neagix, nyrambler, queenanne, scarrfish,
  skarsnikus, solarcell007, turbo, zheal
  Thanks to vr-interactive for providing the website's new background image
  Thanks to sferath for providing the mascot - https://deviantart.com/sferath
  Special thanks to elwismw for existing. #hereforelwis

LICENSE:
  The source tree and generated binaries are distributed under
  the terms of GPL v3. See LICENSE file.
  The patches found in patches/ directory are in public domain unless
  noted otherwise. See patches/LICENSE file.
  For the patches found in ips/, see the corresponding .txt files.
