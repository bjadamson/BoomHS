# BoomHS

A sandbox online roleplaying game focusing on emergent gameplay through player interaction. Playing with others is a core design goal.  BoomHS focuses on creating an immersive environment for players to play a game in without all the modern day game features that ruin immersion *(cash shops, loot boxes.., etc...)*

`This project is in active development.`

## Table of contents
  * [Project Information](#project-information)
      + [Gameplay](#gameplay)
      + [World Design](#world-design)
      + [Player Progression](#player-progression)
      + [Story](#story)
      + [Design Tenants](#design-tenants)
      + [Screenshots](./screenshots/)
      + [FAQ](#gameplay-faq)
      + [Roadmap](#gameplay-roadmap)
        
  * [Developer Information](#developer-information)
      + [Building the Game](#getting-started)
      + [Install Dependencies](#install-dependencies)
      + [Bootstrapping](#bootstrap-the-project)
       + [Command-Line Arguments](#command-line-arguments)
      + [Compiling](#compiling-the-project)
      + [Run the Project](#run-the-project)
      + [Other Information](#other-information)
      + [Helpful Scripts](#helpful-scripts)

## Project Information
The project at it's core is a simulation aimed at letting emergent behavior rise organically from interactions between the environment and the players.

The world can be loosely described as a science fiction fantasy world with roots in traditional fantasy. Some of the inspirations for the game are themselves set either a fantasy or science fiction environment.

The gameplay itself is first/third person following around a character you create (or was created for you) when first joining the world. Once in the world, the sandbox nature of the game will lead players on their adventures. Players will run into other players, and can choose how they want to interact with them. Alternatively they may choose to pursue adventures that do not involve interacting with other players. After all running into other players can be both advantageous and disadvantageous.

Loosely, when you start playing your character you end in one of N different starting areas within the game. You'll character will be initially *created* once at a starting spot within the starting areas. Once in the game the player is free to play how they want.

Players will find themselves in one of these starting areas without much direction. *The game will not be telling the player what to do. The player is free to play the game however she likes.*

From this point the player will receive basic control instructions, but that's it. The player will then explore the world around her in order to progress. The simplicity of the control scheme coupled with the open world sandbox world is the primary design goal.

## Gameplay
Initially the game will present you with a character to control, some initial starting gear for your character, and a short dialogue from an NPC greeting you to the world.

From there, the player can do whatever they like. The game's simulation and systems drive the interactions players can have on the world. None of these systems are inheritantly *better* than one another, it's up to the player to pursue systems that they believe will further their own interests within the game. For example some players could derive most of their gamplay through leveling and PvE or PvE interactions, while others could never see an ounce of danger their entire career. It should be the players who determine what is successful and what is not within each world.

Other gamplay systems examples include: exploring, trading, crafting, building, leveling, PvE raids/dungeons, PvP, PvP raids (taking over opposing clan or faction territory), resource farming, item farming, joining a group/clan/faction.

 * Player emergent behavior is an explicit goal. Players should playing the game the way they want too.
    + Player's can collect items and resources to get stronger.
        - Items can both be taken from enemy monsters, found in the world, taken from other players, or puchased from NPC characters.
 * Gameplay is group (small and large) centeric.
 * Solo gameplay is possible but has disadvantages over players who choose to group with others.
 * Territory can be conquered.
    + NPC or Player clans/factions can take control of territory.
    + A faction with no territory no longer exists.
    + Clans can own territory, but are not required to in order to exist (unlike Factions).
 * Players can build in the world (mostly) wherever they want.
      + Requires using resources.
      + Raidable by other players.
      + Primarily used for storage and shelter.
      + Risk/reward when deciding where to put a base.
      + Safe place to hide from monsters, other players, weather, or just to logout in (assuming you don't get raided).
      + NPC characters can destroy your home given the proper motivation and resources.

![Alt text](./screenshots/33.png?raw=true "06/24/2018")

## World Design
The world is designed with the following:

  * Open sandbox environment.
    + Open World -- no loading zones (but zones as areas are part of the game).
  * The world should feel large, but *not* fragmented and unconnected.
    + Travel should take time. Travel can be automatic (ie: riding an npc-driven boat between continents, riding a flying something ...), but never instant. Distant places should take longer to travel between and should cost more / require higher level teleportation spells, etc...
    + Instant travel should not be the default travel mode for most players.
    + Keep the world small enough so that player interactions occur frequent enough, but the world is as large as possible.
  * Immersive unique environments across the world.
    + Both generated and static content mixed together.
    + When using procedural generation care should be taken to ensure the content doesn't feel "samey".
    + Use the sun, all the other celestial bodies, weather, fog, and lighting to make each environment unique so the player remembers the different zones in their brain.
    + It is occasionally a good decision to make zones nearby similar in look/feel, but this should
    be done with good attention to making them distinct within their similarities.

#### Story
Player's are put into an online world with the vague introduction to the idea that an evil wizard is bent on destroying the star within their galaxy. The star's destruction will ensure the entire galaxy you live in will be destroyed. A player will spawn into the world with nothing, and will need to figure out how to survive and thrive.

### Design Tenants
I am creating the game that I would like to play some day. This means that certrain philosophies behind the design are in play:

  * The game is Hard. The player is punished for playing poorly. This includes dying, you want to avoid dying as much as you can.
  * Weird. The world should be weird, and have weird things occur because of this weirdness.
  * Emergent behavior. The world is full of Emergent behavior from complex interactions of simple systems.
  * Anti-scummable. the world should not be scummable, grinding the same monsters is not fun.
  * Scary. Alive. The world should be scary, and feel alive. It should feel like the game is always tring to kill you.
    + example: traveling should be dangerous.
  * Varied Content. Dungeons, open areas, narrow corridors, all with differing atmospheres.

### Gameplay FAQ
>Q. Can you play this game online or offline? both?
>A. Players can either play by themselves (in a fully offline mode), or log onto the public servers for an online adventure. We also would like to see a LAN mode.
>
>Q. Why make this game?
>A. Most online games today that have come out failed to grasp my interest. Either cash shops, easy game play, no death penalty (or lite), etc.. I wasn't finding any games I wanted to play personally so I started building a game I would like to play someday with friends.
>
>Q. Is this game a sandbox or themepark style?
>A. This game is much more sandbox than themepark. This game was specifically designed because the themepark model (coupled with gameplay difficulty reduction over the years) has proven unfun to me.
>
>Q. Does the game support using a controller? How about keyboard/mouse?
>A. Yes and yes. Further, the game is designed with a simplistic control scheme in mind (PS2 controller and keyboard should be sufficient the do everything within the game).
>
>Q. Is this game PC only? How about consoles?
>A. Yes, for now. Special care has been take through the development of this project to allow porting to other platforms in the future with relatively little code rewriting required.
>
>Q. The project is both a simulation and game. Ok, which one is it really?
>A. The project at it's core is both a simulation and game, however gamplay always wins over simulation accuracy. *That said*, the project goal is to let emergent behavior that is *fun* come from the simulation organically.
>
>Q. What kind of control scheme is the game designed for?
>A. An intuitive control scheme is an *important* design goal, so that the project is *both* accessible and allows hardcore players to sink their teeth into the game as often as they wish. (Some of the game's inspirations come from online PS2 game's where a controller and keyboard was enough to be fully immersed within the world).
>
>Q. Is there PvP gameplay? How about PvE?
>A. The idea is to support PvP enabled servers and PvE servers.
>
>Q. Is there a hardcore mode? How about Hardcore Pvp? Hardcore PvE?
>A. Yes yes and yes!
>
>Q. Is the game more focused on solo gameplay or group gameplay?
>A. The game supports playing by yourself, but is balance around playing with others.
>
>Q. Are there instanced areas within the game? What about sharding?
>A. No. These design decisions go against the core design decisions.

### Gameplay Ideas
Curse brings out a second or third moon to impact world environment (ie tides rising dramatically, volcanoes, tides being messed with given the gravitational pull)
this plunges world into turmoil (environmental instability, return to farming culture, cultural regression). Ben had idea that cursed moon/moons only occur on certain nights, randomly, impact environment for a day and a night and then people have to regroup the next day. Monsters could be mutated humans/creatures that have adapted to the new world (an option). Would add plenty of gameplay options with exploration, above ground and below, as well  as fights with enemies. 
Example, fighting an enemy and suddenly tides start to rise because the cursed moon/moons have appeared. Additionally, very cool visual that can add a sense of random/impending doom. 
Caving/exploring underground, stakes raised given the pull on the planet's gravity/magma. Ben thinks a moon shaped like a skull would be a good plan (can be simplified, the moons' marias --canyons and patches on the surface--could be shaped to resemble a skull face). Realistically, looking at player's suspension of disbelief is important, can a player believe that a moon/moons curse or otherwise can be created in this world. Possibly. Could also be interesting exploration into the "curse", did this phenemenon just conveniently occur as the curser was to be executed, could allow for interesting dialogue with characters (those who believe in the curse and those who don't and are realists), further storyplot to flesh out. 
could allow for the "cursed moons" to circle in a skeejawed kind of orbit that allows them to appear randomly (one at a time, both at same time) and wreak havoc on the planet involved. 
thought, large asteroid hits single moon, moon breaks into multiple moons, asteroid also caught by planets gravitational pull, becomes an additional moon. Check science of this realistically happening, could the chunks actually be knocked into seperate orbits. 
Ben idea, curse comes, moon chunks decaying 

## Developer Information
The project is written in modern C++, and currently uses the OpenGL API directly for rendering. The code is structured so that another backend can be swapped in someday without rewriting everything.

+ The entities within the game are managed using an [ECS system](https://github.com/skypjack/entt).
+ A full list of external dependencies can be seen from the [external](./external/)  directory.


### Getting Started
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

* Build System: The [-n] flag switches build systems from the default (Unix Makfiles) to the [Ninja](https://ninja-build.org/) build system (assumes installed locally).
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

### Helpful Scripts
There are helpful scripts in the [scripts/](./scripts/) directory that do lots of helpful things (from code formatting to executing the debugger after setting up environment variables).

## Other Information
  + Source-code formatting is done by invoking this script. The script looks for the
    clang-formatter tool installed locally on your machine.
```bash
scripts/code-format.bash
```

#### links
* https://github.com/cbaggers/cepl