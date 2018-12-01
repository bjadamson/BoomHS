# BoomHS

A sandbox online roleplaying game focusing on emergent gameplay through player interaction. Playing with others is a core design goal. All development focuses on creating an immersive environment for players to play a game in without all the modern day game features that ruin immersion *(cash-shops, loot boxes.., pay-to-win, etc...)*

![cone](./assets/cone.png?raw=true)`Under Construction! This project and README file are in active development.`

[TOC]

## Project Information
The project at it's core is a simulation aimed at letting emergent behavior rise organically from interactions between the environment and the players. The universe the game takes place in can be loosely described as a mix of science fiction fantasy and traditional fantasy, mixing static content (known more commonly as prefabs) with generated content. The players will join this sandbox universe of worlds with no real direction, deciding amongst themselves how the game should be played. Underneath this sandbox a story about the universe's imminent destruction or potential salvation is waiting to be discovered. The fate of the world could potentially rest in the hands of a small group of players.

This project was started in response to the current trend of online games coming out. These modern games tend to target a very casual audience, which tend to guide players in a theme-park experience from place-to-place. Some existing games have transitioned to this theme-park gameplay model in order to increase their subscription count, supporting their underlying business model (generating as much money for their stockholders as possible) instead of listening to what their most dedicated players are asking for. 

The project will intentionally be developed in the direction the original creator and it's hardcore audience are asking for. No time will be wasted developing features players do not want (yes, I have a phone, but NO) instead improving the overall experience will be the focus of the project's development.

The aim is to build a sandbox role-playing game that can be played both in a casual or competitive mindset/mode. The game will reward players whom learn how to play the game well, those who learn the systems and how to use them to their advantages better than others. Players who play poorly will be punished using these same systems (ie: death penalty can be substantial).

## Gameplay
Initially the game will present you with a character to control, some initial clothing for your character, and a short dialogue from an NPC greeting you to the world. From there, the player can do whatever they like. The game world will eventually reach a conclusion, at which point all players will be evicted from the server as it will shutdown and begin preparation to launch a new world. This cycle of universe birth, gameplay, conclusion, is core to the game's design. Players should expect somethings to carry forward between gameplay worlds, but not the majority of things. 

The game will play like a traditional sandbox game, offering little direction to the player. Instead as the player engages with the world, she will have to discover the main story through normal gameplay. The player should not expect the plot/quest lines to not be the same between generated worlds.

The game will have a strong focus on emergent behavior and player-to-player interactions driving the player experience. The successful player will have to choose between creating allies or enemies with other players and NPCs.

When the player launches the world, they will be able to select how they wish to play.

- Offline by themselves. Useful for players who wish to play without the interactions with other players.
- LAN mode. Play locally with others, but without the distraction of players not on the local network.
- Online public or private (require a password, or hide the server's IP all-together so players need to know the IP to join) servers. 

The default mode of play is through joining a public online server. After the player has selected a specific server (which can have different rulesets such PvP, PvE, hardcore PvP, hardcore PvE) the player will select the class they wish to play as. After the player selects her starting class, the game will generate a character, selecting a race/gender on their behalf. The player will then be put into the game directly, or placed into a waiting room (chat room) with other players until the world is scheduled to launch.

- This design choice is made to foster a specific gameplay experience that is different from other games. A game-mode where the player can choose their characters race/gender can exist, it will just not be the default mode of play.

The gameplay itself will play like either a first-or-third (toggleable by the player anytime) RPG/shooter, with many traditional mechanics borrowed from both genres. For example the player will be able to target other entities (players, monsters, NPCs) with tab targeting, but combat will also sometimes utilize manual aiming of attacks or abilities (think aiming a bow at a specific location on the terrain, maybe to cause ice to melt, or an area-of-effect spell).

As players successfully defeat other players or monsters they will gain experience. If enough experience is gained, a player will increase their level up to a maximum level of 50. Players can collect items and resources to get stronger. These items can both be taken from enemy monsters, found in the world, taken from other players, or purchased from NPCs, etc...

The player can expect to engage in a wide variety of gameplay styles, each leading the player to a reasonable expectation of being successful within the game world. Some examples of systems the player can pursue are: exploring, trading, crafting, building, diplomacy, leveling, PvE raids/dungeons, PvP, PvP raids (taking over opposing clan or faction territory), resource farming, item farming, joining a group/clan/faction, and more! Players will be able to get engage in any number of these activities, the game will place no artificial restrictions on gameplay. It is a goal that none of these systems will be strictly superior to the others, meaning a diplomat or warrior can be equally successful.

Some of the less traditional gameplay mechanics can lead players into a position where they become responsible for more than just their own character. A successful diplomat may find themselves in a position of power over others, granting them a different perspective. Another example would be a player who becomes mayor of a village, or kind of an entire kingdom. The player in this position will be able to switch from the first/third-person perspective to an orthographic top-down projection with a UI that allows the player to operate in a more civilization management gameplay style. Consider the player who becomes king can order armies subject to her rule around the kingdom, tell peasants what jobs they should be doing, view civilization-wide stats, trade with other civilizations, etc... Players who choose this gameplay style should expect a balanced experience with those players who choose to play in a more traditional RPG style controlling a single character.

At some point in the game world's existence an unpredictable event in the world will occur. This event may or may not make itself known to the players, but it's passing will unblock the beginning of a *quest* leading a player or group of players to confront the evil wizard before he has destroyed the universe. The quest will be epic, sometimes requiring the player to travel to other worlds/planes. The quest will not be the same between game worlds. It it important to note that the game will never tell the player this explicitly, instead experienced players will begin to recognize patterns from previous play throughs that hint an event such as this has occurred. Players who are less experienced will have to either discover this organically from playing the game (ie: conversing with other players, or genuine exploration).

## Story

Players are initially spawned into a world with the vague introduction to the idea that an evil wizard at some point in the near future going to destroy the star within their galaxy. The star's destruction will ensure the entire galaxy you live in will be destroyed. After this short introduction (cutscene or dialogue) the player will spawn into the world barely clothed and alone. They will then be given a small introduction dialogue box welcoming them to the world and telling them a little about the city they just spawned in. At this point the game will play like a sandbox, giving no explicit instructions to the player, instead it will be up to the player to discover the underlying story and ultimately either kill the wizard, or help him execute all life in the known solar system.

## World Design
![Alt text](./screenshots/33.png?raw=true "06/24/2018")

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

## Roadmap

![cone](./assets/cone.png?raw=true) Work in progress! Future milestones may/should be expected to be added.

#### Milestone 1 (Avocado Release)

This milestone for development is world building and world generation. The goal of this release is to develop the infrastructure necessary to allow the developers to create a basic world, and explore it using only the game itself. Basic textures and basic sounds will be used to test the features developed at this stage. This release will support *offline mode only*.

1. Generate a world starting from a random seed.

   - Terrain, water.
   - Time of day, night-day cycle.

2. Basic AI for the NPCs.

   - Pathfinding.
   - Player interaction. (talking and super basic combat)

3. A basic *level editor* (to be used on a generated world, or hand-craft an entire world using the editor.

   - Orthographic top-down view of the world with scrolling and zoom supporting.
   - Uniform translation/rotation/zoom.

4. Move around in the world with <u>**BASIC**</u> systems in place.

   - First-person camera.
   - Third-person orbital camera.

   - Ambient and non-ambient sounds.
   - 3D graphics.
   - Keyboard/mouse and controller support.

5. Offline chat support (ie: allow player to type in commands into an in-game console).

6. Get the game running on supported platforms. (Linux, Windows, and OSX)

7. Performant enough that players can maintain 60+ FPS.

   - Since only basic gameplay is to be implemented, optimizations should be made at this stage so future development can easily profiled and a 60+ framerate can be maintained moving forward.

#### Milestone 2 (Brussel Sprout Release)

This step being complete should give confidence in the project architecture supporting the features planned for future milestones. Specifically the networking code interacting with the features from the first milestone. The bulk of this milestone will require re-architecting some of the project to support an server/client model. The client will support basic prediction, but the server will be the authority.

1. Basic online support. 

   - Online pre-game lobby support. This allows players to gather together and chat in preparation to joining a world.

   - In-game chat support among players (text).
   - Basic microphone support (two players can communicate in-game using their microphones).
   - Players should be able to interact in limited ways. Players should be able to:
     - See each other. See in real-time players and NPCs moving through the world.
     - Player movement shouldn't be smooth in the face of lag, gameplay shouldn't be choppy in an online environment.
2. Performance maintained at 60+ FPS in an online (also offline) environment.
3. Load assets on a background thread while the player sits in the main menu.
   - If the player starts a game before loading is complete, show a loading screen for the remaining duration with a progress indicator.
   - Load assets from multiple background threads (parallelize loading a level).
   - When a player reaches a new zone/level, load the entire zone/level (will switch to a streaming *chunk* system that means players will never see a loading screen later).

#### Milestone 3 (Drop-the-Beets Release)

The focus of this milestone is to start to introduce basic gameplay features.

1. Item system.
   - Pickup/drop items on the ground.
   - Trade items with other players.
   - Basic resource gathering (gathering herbs/ore, various other items) that doesn't come from killing and looting another entity (player or AI).

2. Combat system.
   - Player to AI and player-to-player combat.
   - Death and respawning systems.
   - AI to AI.

3. Implement the chunk system, so the player never has to pause and load when running around the world.

   - As the player moves around, dynamically load/unload terrain/entity data for everything nearby. This will probably use a quadtree data structure. If a player's hardware cannot load the assets fast enough, then the game will have to show a temporary load screen.
     - QUESTION: What is the experience for potato computers that can't load assets fast enough? When should their loading screen occur? We don't want to have a user have random loading screens while trying to run around the world, or in the middle of combat, etc..)
     - ANSWER: The runtime maintains a minimum number of terrain vertices that it keeps in memory Vmin and a maximum number of vertices Vmax. When a player passes a zone line, if the number of nearby vertices a player has loaded currently is less than Vmin, show a loading screen and wait until at least Vmin vertices have been loaded. This will give a slightly worse experience to players with potatoes, but it's intuitive to most players and allows for faster PCs to never show the user a loading screen outside of initial loading / teleportation mechanics.
   - If a player is teleported (ie: gm commands, some teleportation spell, etc...) then the game will have to show a loading screen.

4. Game-over scenario.

   - In some game scenarios the game world will come to a conclusion. Implement a basic system that a player in-game can cause a "win/loss" scenario, ending the current world. In this game mode, the world will support the ability to regenerate a new world for players to join.

   ## Design Goals

   1. Emergent behavior driven from the sandbox nature of the game.
   2. Encourage, but do not require group gameplay.
   3. Recognize player (or NPC) organizations such that territory can be understood as owned by a specific group of players, giving them advantages/disadvantages for *taking/loosing* territory.
   4. Allow players to build in the world (mostly) wherever they choose.
   5. Each game world (hosted either locally or remotely) will eventually come to a conclusion. Players should carry-through some of their advancements to their character to the new session. (This does not include some of the more hardcore competitive modes).
   6. Easy to learn, difficult to master. The game should allow a large range of skill, meaning players will want to keep playing to get better. Players who play poorly should be punished. This is explicitly in response to modern games where mechanics are more often not very punishing when players perform poorly, such as dying).
      - If a player is killed, they shall gain an experience debt proportional to their level. Higher level players will take more time to repay this debt, before they are allowed to continue gaining experience (and thus leveling up). This debt will have a maximum value dependent on the player's current level. This penalty should be severe enough that players will try hard to avoid death, but not be too afraid in late game scenarios such as raiding to die as many times as they want. The experience debt mechanic was chosen over an experience loss system, so the player can choose to engage in late-game activities resulting in multiple deaths. The thought is that players will engage in experience-gaining activities when they are not engaging in risky activities such as raiding.
   7. Weird. The world should be weird, and have weird things occur because of this weirdness.
   8. The world should always feel scary and alive. Traveling through zones that are dangerous should kill the underprepared player.
   9. Varied Content. Dungeons, open areas, narrow corridors, each with differing atmospheres.
   10. Solo players should be at a disadvantage to others who play in a group, but no design decision shall be made that prevents a player from "winning" by themselves. A player with sufficient skill should be able to win solo, but this is not what the game is targeted at. Most players are expected to win with other players aiding them.
   11. Allow NPC or Player clans/factions to acquire/loose territory within the world.
       - A faction with no territory no longer exists.
       - Clans can own territory, but are not required to in order to exist (unlike Factions).
   12. Players can build structures within the world.

   - Requires using resources.
   - They can be taken over through a *raiding* system by other players or NPCs given the proper motivation and resources.
   - Primarily used for storage and shelter.
   - Risk/reward when deciding where to put a base.
   - Safe place to hide from monsters, other players, weather, or just to logout in (assuming you don't get raided).

### Why did I create this game?

In today's modern gaming era, the trend of *hardcore* games are becoming less common then when I was growing up. I grew up when games were designed not to streamline content, but to immerse players into the virtual environment. In a sense, I feel the hardcore games I grew up playing are being replaced by *casual* games.

Many of the games entering today's marked target a general audience, which means the hardcore mechanics that were in games when I grew up are often not included in the product. Instead, casual friendly features such as instant transportation, dying without meaningful consequences (ie: no experience loss, ability to resurrect yourself, often around where you died), pay-to-win cash shops or purchasing in-game buffs with real money). The effect of these *casual-friendly* features is that I find it hard to get immersed in the world, and thus I cannot enjoy the gameplay. This game is my attempt at designing and implementing a game I would personally like to play with my friends in the future.

### Gameplay FAQ
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

Below is an unordered list of ideas currently considered for implementation.

- The wizard periodically places various curses upon the world.
  - A spell that causes a second or third moon to appear. This will impact the climate within the world such as tides rising dramatically, volcanoes, tides being messed with given the gravitational pull). Perhaps this only occurs on certain days of the in-game week/month/year. This can add a  cool visual that can add a sense of immersion and impending doom.

## Developer Information
The project is written in modern C++, and currently uses the OpenGL API directly for rendering. The code is structured so that another backend can be swapped in someday without rewriting everything.

+ The entities within the game are managed using an [ECS system](https://github.com/skypjack/entt).
+ A full list of external dependencies can be seen from the [external](./external/)  directory.

### Getting Started

The general procedure for a developer getting started for the first time is straightforward.

1. Fetch the repository locally.

```
git clone https://github.com/bjadamson/BoomHS.git
```

2. Install dependencies. (A bash script is provided in the `scripts/` directory that will interactively install the dependencies locally necessary to compile the project.)

```
cd BoomHS
scripts/install-dependencies.bash
```

3. Bootstrap the project (setup cmake database).

```
scripts/bootstrap.bash
```

4. Build and run.

```
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

## Developer Guidelines

- Limit the code exposed through header files, try and put as much code as possible inside translation units. <u>If a function is never called outside of a translation unit, do not expose it in a header file</u>. This makes it easier to re-organize the code or change the project architecture as new development proceeds.
  - For testing purposes, consider separating code into smaller functions/objects so the absolute minimum interface is exposed via header.
  - Templates make this difficult. Use a nested namespace (ie: detail) namespaces to limit exposed interfaces. Detail namespaces are assumed to not be called outside of the header file where the interface is exposed.
  - Use the anonymous namespace inside a translation unit when possible, this separates the header implementation from the algorithm implementation.
- Use composition over inheritance, except when necessary to promote DRY. This will keep objects decoupled.
  - So far neither virtual inheritance or multiple inheritance have not been necessary in any manner (not including code in external libraries/dependencies). Keep it this way, it prevents a large class of possible bugs/confusion and makes debugging easier.
  - Sometimes a macro inside a translation unit is a better choice than inheriting from a base class.
- Limit heap usage.
  - Use the stack for your objects.
  - So far invoking raw operator "new" has not been necessary, this should be maintained moving forward unless a discussion results in it being considered necessary.

  + Source-code formatting is done by invoking clang-format, use the format script inside the scripts/ directory.
```bash
scripts/code-format.bash
```

#### links
* https://github.com/cbaggers/cepl
