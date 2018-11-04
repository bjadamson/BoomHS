# BoomHS
A sandbox online roleplaying game focusing on emergent gameplay as a core gameplay mechanic. This
project is in active development.

Players can either play by themselves (in a fully offline mode), or log onto the public servers for
an online adventure.

## Table of contents
  + [Project Information](#project-information)
    + [Gameplay](#gameplay)
    + [World Design](#world-design)
    + [World Story](#world-story)
    + [Design Decisions](#design-decisions)
    + [Screenshots](https://github.com/bjadamson/BoomHS/tree/release/screenshots)

  + [Getting Started](#getting-started)
    * [Install Dependencies](#install-dependencies)
    * [Bootstrapping](#bootstrap-the-project)
      + [Command-Line Arguments](#command-line-arguments)
    * [Compiling](#compiling-the-project)
    * [Run the Project](#run-the-project)
    * [Other Information](#other-information)
    * [Hints](#hints)

## Project Information

Players will join an online sandbox environment, in either one of two game modes (chosen by the
player). The first mode will be a hard-core mode containng perma death.

Player's are put into an online world with a vague goal of stopping the evil wizard from destroying
the star in your galaxy. The star's destruction will ensure the entire galaxy you live in will be
destroyed. A player will spawn into the world with nothing, and will need to figure out how to
survive.

Player's will collect items and resources to get stronger. Items can both be taken from enemy
monsters, found in the world, taken from other players, or puchased from NPC characters.
Resources can be used to build the player a home. These homes will exist for the player to take
refuge in, while hiding from monsters and the environment, or other players. These homes can be
invaded/destroyed/stolen by other players. Monster's can also invade/destroy/steal player homes for
their own.

Player's can group together to accomplish their goals. A player wins by making it to the
end-scenario and escaping the world before it is destroyed. The first player/group who makes it to
the end can make the decision to help blow up the sun, sacraficing themself. If a group/player
makes the decision to help the wizard destroy the sun, they themselves cannot move forward. Other
player's may still escape, they will just have to do it more quickly or something. TBD

![Alt text](/screenshots/33.png?raw=true "06/24/2018")

## World Design
  + Online sandbox environment.
    * Open World -- no loading zones (but zones as areas are OK).
    * Player-driven emergent behavior is an explicit goal. Players should playing the game the way
    they want too.
    * Gameplay is group (small and large) centeric. Solo possible but with disadvantages over
      players who choose to group with others.
  + World PvP is core to gameplay.
    * There are N factions/races/??? that have differing alliances at any time (player decided).
    * Pre-arranged battles that players can "queue-up for" shall not exist in the game.
  + Territory can be conquered.
    * NPC groups (factions) can take control of territory.
    * Players can take control of territory.

  + Players can build t
      - Raidable by other players.
      - Buildable using resources. Primarily used for storage and shelter. Safe place to logout,
        assuming you home is not raided.
      - NOTE: Bases are not required for players, typically a player will signifigant resources
        before attempting to build a home/base.
  + Minimal NPC towns, most kj
    * Players have risk/reward when deciding where to put a base.
      - Territory control will be in flux, player aggression (from any side) will cause the
        territory lines to change.
  + Hardcore mode available (accompanying online ladder should accompany this).
  + World should feel large.
    * Travel should take time. Travel can be automatic (ie: riding an npc-driven boat between
    continents, riding a flying something ...), but never instant. Distant places should take
    longer to travel between and should cost more / require higher level teleportation spells,
    etc...
    * Instant travel should not be the default travel mode for most players.
  + World should be large, but not fragmented.
    * Some games make the mistake of making players too spread out, resulting in the world feeling
      small. This causes some zones and cities to be completely abandoned. Every design decision
      made should be done with preventing this in mind. The successful outcome of this effor should
      leave the world feeling massive but alive, even with a smaller player-base.
  + Unique look and feel across the world.
    * Both generated and static content mixed together.
    * When using procedural generation care should be taken to ensure the content doesn't feel
    "samey" (forgive the spelling, I do not know how else to spell this concept).
    * Zones that have ambient music, should have unique ambient music. Not all zones are required
      to have unique music, but it should be an exception when a zone does not have music.
  + Immersive environments.
    * Use the sun, all the other celestial bodies, weather, fog, and lighting to make each
    environment unique so the player remembers the different zones in their brain.
    * It is occasionally a good decision to make zones nearby similar in look/feel, but this should
    be done with good attention to making them distinct within their similarities.
  + Simplistic user interface.
    * This includes simple user controls. The goal here is to make the game extremely intuitive to
      new players, to begin. Once the player has gained mastery, futher complexity can be exposed to
      the player (and the pro).

##### Progression
Player's who **win** a world, will gain access to the next-tier world. Those who do not succeed,
get to play their current level again. Think of it like a ladder, everyone starts on world 0 (this
will be where all the relatively new players should be), and if you can win world 0, you move on to
world 1. I'm not sure if backwards-progression would be a fun.

#### World Story
Initially the player is told they are on the planet somewhere in the galaxy, and that a few million
years ago during a hanging, an evil necromancer's last dying breath was to curse those about to
kill him. In doing so, he cursed the entire planet. Since then, the planet has been falling apart
in strange ways, like the cursed blue portals from dimensions that don't exist.

It was later discoved when those who slayed the necromancer were reading his journal that deep
beneath the subterranean layer the necromancer had a lair he was experimenting with bringing
powerful demons from other worlds to our world. Within this hidden lair is a spellbook with
tremendous power. The spells contained within it's pages are so powerful that it is said when the
journal was read, a powerful spell was erasing the words as they read them. They only had one
chance to read the journal. When the journal was read, they noted the mention of what they call a
"super-powerful" spell. A spell that's entire purpose is to blow up our sun.

### Design Decisions
"I am creating the game that I would like to play some day. This means:
  * Hard. The game is Hard. You are punished for playing poorly.
  * Death is Severe. You want to avoid it.
  * Procedural content. The world should be generated from a random seed.
  * Hand-crafted content. the world has hand-created content injected into the procedural.
  * Weird. The world should be weird. I want weird things that capture my imagination as I play.
  * Emergent behavior. The world is full of Emergent behavior from complex interactions of simple
  systems.
  * Anti-scummable. the world should not be scummable, grinding the same monsters is not fun.
  * Scary. Alive. The world should be scary, and feel alive. It should feel like the game is always
    tring to kill you.
    + example: traveling should be dangerous.
  * Varied Content. Dungeons, open areas, narrow corridors, all with differing atmospheres.
  * Item based progression. There are no levels, instead character advancement is through acquiring
    more poweful items. Consumables.

### Gameplay Ideas
Curse brings out a second or third moon to impact world environment (ie tides rising dramatically, volcanoes, tides being messed with given the gravitational pull)
this plunges world into turmoil (environmental instability, return to farming culture, cultural regression). Ben had idea that cursed moon/moons only occur on certain nights, randomly, impact environment for a day and a night and then people have to regroup the next day. Monsters could be mutated humans/creatures that have adapted to the new world (an option). Would add plenty of gameplay options with exploration, above ground and below, as well  as fights with enemies. 
Example, fighting an enemy and suddenly tides start to rise because the cursed moon/moons have appeared. Additionally, very cool visual that can add a sense of random/impending doom. 
Caving/exploring underground, stakes raised given the pull on the planet's gravity/magma. Ben thinks a moon shaped like a skull would be a good plan (can be simplified, the moons' marias --canyons and patches on the surface--could be shaped to resemble a skull face). Realistically, looking at player's suspension of disbelief is important, can a player believe that a moon/moons curse or otherwise can be created in this world. Possibly. Could also be interesting exploration into the "curse", did this phenemenon just conveniently occur as the curser was to be executed, could allow for interesting dialogue with characters (those who believe in the curse and those who don't and are realists), further storyplot to flesh out. 
could allow for the "cursed moons" to circle in a skeejawed kind of orbit that allows them to appear randomly (one at a time, both at same time) and wreak havoc on the planet involved. 
thought, large asteroid hits single moon, moon breaks into multiple moons, asteroid also caught by planets gravitational pull, becomes an additional moon. Check science of this realistically happening, could the chunks actually be knocked into seperate orbits. 
Ben idea, curse comes, moon chunks decaying 

#### links
* https://github.com/cbaggers/cepl

## Getting Started
General Procedure
1. Install dependencies.
2. Bootstrap the project (setup cmake database).
3. Build and run.

```bash
git clone https://github.com/bjadamson/BoomHS.git
cd BoomHS
scripts/install-dependencies.bash
scripts/bootstrap.bash
scripts/build.bash
scripts/build-and-run.bash
```

### Install dependencies.
To install external dependencies, a script is used.
```bash
scripts/install-dependencies.bash
```

### Bootstrap the project
Bootstrapping the project is handled through a script. This script takes care of enumerating all
the source files in the project and passing them to cmake. 
```bash
scripts/bootstrap.bash
```

##### Command-Line Arguments
The bootstrapping script supports some command line arguments.

* Build System: The [-n] flag switches build systems from the default (Unix Makfiles) to the
<a href="https://ninja-build.org/">Ninja</a> build system (assumes installed locally).
* Static Analysis: The [-a] flag enables static analysis information to be linked with the binary
  during compilation.
* Debug/Release: The [-r] flag will instruct CMake to build a release binary (no debugging
  symbols). The default build type is Debug.
* List of supported commands: The [-h] flag will display a list of all command line arguments
  supported by the script, and then immediately exit.

### Compiling the Project
(Must bootstrap before this step).

```bash
scripts/build.bash
```

#### Important Note
When adding a new source file to the project, you must run the bootstrap script again.

### Run the project
```bash
scripts/build-and-run.bash
```

## Other Information
  + Source-code formatting is done by invoking this script. The script looks for the
    clang-formatter tool installed locally on your machine.
```bash
scripts/code-format.bash
```

## HINTS
### The following symlinks in the project's root directory exist to make running the scripts easier
on the fingers (from the command line). You can see the symlink for yourself in the root directory.
  + bb  => build the project
  + bbc => clean the project (requires bootstrapping again)
  + bbr => build and RUN the project
  + bbf => run the source code formatter.
  + bbk => kill the running game process (parses PID table, unstable).



