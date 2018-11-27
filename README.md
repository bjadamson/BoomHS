# BoomHS

A sandbox online roleplaying game focusing on emergent gameplay through player interaction. Playing with others is a core design goal. All development focuses on creating an immersive environment for players to play a game in without all the modern day game features that ruin immersion *(cash-shops, loot boxes.., pay-to-win, etc...)*

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
The project at it's core is a simulation aimed at letting emergent behavior rise organically from interactions between the environment and the players. The universe the game takes place in can be loosely described as a mix of science fiction fantasy and traditional fantasy, mixing static content (known more commonly as prefabs) with generated content. The players will join this sandbox universe of worlds with no real direction, deciding amongst themselves how the game should be played. Underneath this sandbox a story about the universe's imminent destruction or potential salvation is waiting to be discovered. The fate of the world could potentially rest in the hands of a small group of players.

This project was started in response to the current trend of online games coming out. These modern games tend to target a very casual audience, which tend to guide players in a theme-park experience from place-to-place. Some existing games have transitioned to this theme-park gameplay model in order to increase their subscription count, supporting their underlying business model (generating as much money for their stockholders as possible) instead of listening to what their most dedicated players are asking for. 

The project will intentionally be developed in the direction the original creator and it's hardcore audience are asking for. No time will be wasted developing features players do not want (yes, I have a phone, but NO) instead improving the overall experience will be the focus of the project's development.

The aim is to build a sandbox role-playing game that can be played both in a casual or competitive mindset/mode. The game will reward players whom learn how to play the game well, those who learn the systems and how to use them to their advantages better than others. Players who play poorly will be punished using these same systems (ie: death penalty can be substantial).

### Design Goals

1. Emergent behavior driven from the sandbox nature of the game.
2. Encourage, but do not require group gameplay.
3. Recognize player (or NPC) organizations such that territory can be understood as owned by a specific group of players, giving them advantages/disadvantages for *taking/loosing* territory.
4. Allow players to build in the world (mostly) wherever they choose.
5. Each game world (hosted either locally or remotely) will eventually come to a conclusion. Players should carry-through some of their advancements to their character to the new session. (This does not include some of the more hardcore competitive modes).
6. Easy to learn, difficult to master. The game should allow a large range of skill, meaning players will want to keep playing to get better. Players who play poorly should

- Weird. The world should be weird, and have weird things occur because of this weirdness. **example? Do players have option to select a "weird" narrative to follow (Fallout series)?
- Emergent behavior. The world is full of Emergent behavior from complex interactions of simple systems.
- Anti-scummable. the world should not be scummable, grinding the same monsters is not fun.
- Scary. Alive. The world should be scary, and feel alive. It should feel like the game is always trying to kill you.
  - example: traveling should be dangerous. **good example. More. How will game punish poor playing? What kind of aspects will appear scary? 
- Varied Content. Dungeons, open areas, narrow corridors, all with differing atmospheres.

MISC:

- Players can collect items and resources to get stronger. These items can both be taken from enemy monsters, found in the world, taken from other players, or purchased from NPC characters, etc...
- Solo players should be at a disadvantage to others who play in a group, but no design decision shall be made that prevents a player from "winning" by themselves. A player with sufficient skill should be able to win solo, but this is not what the game is targeted at. Most players are expected to win with other players aiding them.
- Solo players will still have to interact with other players in order to win, but they will never be required to win as a group. Solo gameplay is possible but has disadvantages over players who choose to group with others.
- NPC or Player clans/factions can take control of territory.
  - A faction with no territory no longer exists.
  - Clans can own territory, but are not required to in order to exist (unlike Factions).
- building
  - Requires using resources.
  - Raidable by other players.
  - Primarily used for storage and shelter.
  - Risk/reward when deciding where to put a base.
  - Safe place to hide from monsters, other players, weather, or just to logout in (assuming you don't get raided).
  - NPC characters can destroy your home given the proper motivation and resources.
- Players should playing the game the way they want to.
- What does it mean when a group of players "wins"?
- Difference between PvP and PvE servers? When does the world on each type of game? Are there other types of game modes. A PvP server that never ends?This worlds within the universe are infused with life and various kinds of magic.

## Gameplay
**good, establishes rules of the world

Initially the game will present you with a character to control, some initial starting gear for your character, and a short dialogue from an NPC greeting you to the world.

From there, the player can do whatever they like. The game's simulation and systems drive the interactions players can have on the world. None of these systems are inheritantly *better* than one another, it's up to the player to pursue systems that they believe will further their own interests within the game. For example some players could derive most of their gamplay through leveling and PvE or PvE interactions, while others could never see an ounce of danger their entire career. It should be the players who determine what is successful and what is not within each world.

Other gamplay systems examples include: exploring, trading, crafting, building, leveling, PvE raids/dungeons, PvP, PvP raids (taking over opposing clan or faction territory), resource farming, item farming, joining a group/clan/faction.

 * 


At certain stages of gameplay the palyers all around the server will simultaneously experience flashforward of the world being destroyed (wizard). What do these flash forwards mean? Are they meant to be hints to advanced players about how the world is going to end specifically? They are meant to create a sense of fear/uncertainty for new/experienced players. It's based on who's found what in terms of prefab locations and items, which will create a sense of collaboration/competition.

![Alt text](./screenshots/33.png?raw=true "06/24/2018")

The player character is controlled directly by the user, either through a first person or third person perspective. As the player advances through the game and will earn autonomy over various NPC characters, other control schemes will present themselves (such as top-down overview if you decide to become a local village leader) to the player.

Unlike most class-fantasy games where the player can choose a race/class/height/weight, in this game the player is assigned/generated a character on their behalf when their account first joins a new server. What this means is that some players will be fat, some short, some ugly, some not... this choice is removed from the player, allowing a different experience then players are used too from traditional games. To balance this mechanic, periodically each server will end up finishing the story (w) or server reset (long periods, think seasons in any other online game) will cause a new race/

The player will then use this character until the end of eternity while playing on that server session. The gameplay will result in the world being periodically re-created 

This process will ultimately end up being opaque to the user, who may wish to choose a

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

      ** Need to list the game will first operate in one world, with multiple zones. Later on, future worlds can be added to the game (idk this portals to another plane of existence) through either community contributions becoming merged into the main game trunk or whatnot.

#### Story
Players are put into an online world with the vague introduction to the idea that an evil wizard is bent on destroying the star within their galaxy. The star's destruction will ensure the entire galaxy you live in will be destroyed. A player will spawn into the world with nothing, and will need to figure out how to survive and thrive.

The player will haave no indication on the wizard's progress towards destroying their universe at the outset. Clues and story lines will be interwoven into the gameplay, through static prefab content that is dynamically placed on the world. 

**is the vague evil wizard ever mentioned again after the start of the game? Larger part of story plot players can access? Does the players progress mean a possible "save the world" route is unlocked for them?

Yeah, when players progress far enough (they have made progress on the appropriate quest line as determined when the world was created/generated).

** Add flashforwards (multiple) indicating to the player different ways the worlds ends. Fire, water air and  earth.

PLOT:

The world takes place in a galaxy on the edge of destruction, a wizard is in the process of preparing to destroy the solar system's sun, destroying all life within the solar system. Players will travel through the world, and visit other worlds in their attempts to reach and confront the wizard. 

The player will then choose whether to fight the wizard (and possibly die) or let the wizard move ahead with their plan and destroy the solar system. Either way when this happens the game is complete and

The gameplay itself is a first/third person view of a character you create (or was created for you) when first joining the world. Once in the world, the sandbox nature of the game will lead players on their adventures. Players will run into other players, and can choose how they want to interact with them. Alternatively they may choose to pursue adventures that do not involve interacting with other players. After all running into other players can be both advantageous and disadvantageous.

**clarify fantasty/sci fi/survival aspect

Loosely, when you start playing your character you end in one of N different starting areas within the game. You'll character will be initially *created* once at a starting spot within the starting areas. Once in the game the player is free to play how they want.

Players will find themselves in one of these starting areas without much direction. *The game will not be telling the player what to do. The player is free to play the game however she likes.*

From this point the player will receive basic control instructions, but that's it. The player will then explore the world around her in order to progress. The simplicity of the control scheme coupled with the open world sandbox world is the primary design goal.

### Why was this game created?
**would suggest changing to "why I created this game" if seeking personalized tone. Otherwise design tenants can be more cut and dry. Either option works. 

I am creating the game that I would like to play some day. This means that certain philosophies behind the design are in play:

### Gameplay FAQ (AWESOME GOOD IDEA)
>Q. Can you play this game online or offline? Both?
>A. Players can either play by themselves (in a fully offline mode), or log onto the public servers for an online adventure. We also would like to see a LAN mode.
>
>Q. Why (did you) make this game?
>A. Most online games today that have come out failed to grasp my interest. Either cash shops, easy game play, no death penalty (or lite), etc.. I wasn't finding any games I wanted to play personally so I started building a game I would like to play someday with friends. This game was specifically designed because the themepark model (coupled with gameplay difficulty reduction over the years) has proven unfun to me. <-- **add unfun comment to QA 2.
>
>Q. Is this game a sandbox or themepark style?
>A. This game is much more sandbox than themepark.
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
>
>Q. Make it explicit how players can contribute lasting changes to the main ark of the game. Do they need coding/3dmodeling/whatever experience to be successful? Clarify that NO experience outside of playing a game is required to play.
>
>***I would add discussion either here or at the top about how much coding and world building experience a player should possess prior to playing. Can play off of the concept of this open world sandbox as being a "game for coders by a coder", i.e. not your average sandbox, needs a certain level of smarts to participate?

### Gameplay Ideas
Curse brings out a second or third moon to impact world environment (ie tides rising dramatically, volcanoes, tides being messed with given the gravitational pull)
this plunges world into turmoil (environmental instability, return to farming culture, cultural regression). Ben had idea that cursed moon/moons only occur on certain nights, randomly, impact environment for a day and a night and then people have to regroup the next day. Monsters could be mutated humans/creatures that have adapted to the new world (an option). Would add plenty of gameplay options with exploration, above ground and below, as well  as fights with enemies. 
Example, fighting an enemy and suddenly tides start to rise because the cursed moon/moons have appeared. Additionally, very cool visual that can add a sense of random/impending doom. 
Caving/exploring underground, stakes raised given the pull on the planet's gravity/magma. Ben thinks a moon shaped like a skull would be a good plan (can be simplified, the moons' marias --canyons and patches on the surface--could be shaped to resemble a skull face). Realistically, looking at player's suspension of disbelief is important, can a player believe that a moon/moons curse or otherwise can be created in this world. Possibly. Could also be interesting exploration into the "curse", did this phenemenon just conveniently occur as the curser was to be executed, could allow for interesting dialogue with characters (those who believe in the curse and those who don't and are realists), further storyplot to flesh out. 
could allow for the "cursed moons" to circle in a skeejawed kind of orbit that allows them to appear randomly (one at a time, both at same time) and wreak havoc on the planet involved. 
thought, large asteroid hits single moon, moon breaks into multiple moons, asteroid also caught by planets gravitational pull, becomes an additional moon. Check science of this realistically happening, could the chunks actually be knocked into seperate orbits. 
Ben idea, curse comes, moon chunks decaying ***go through, edit, select ideas which seem most viable with vague evil wizard storyplot



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