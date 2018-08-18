# BoomHS
This repository contains the source code for the <b>BoomHS</b> project. BoomHS is a project in
active development.

![Alt text](/screenshots/33.png?raw=true "06/24/2018")
![Alt text](/screenshots/32.png?raw=true "05/18/2018.")

## Table of contents

  + [Getting Started](#getting-started)
  + [Project Information](#project-information)
    * [Install Dependencies](#install-dependencies)
    * [Bootstrapping](#bootstrap-the-project)
      + [Command-Line Arguments](#command-line-arguments)
    * [Compiling](#compiling-the-project)
    * [Run the Project](#run-the-project)
    * [Other Information](#other-information)
    * [Hints](#hints)
  + [Game Information](#game-information)
    * [World Story](#world-story)
  + [Design Decisions](#design-decisions)
  + [Screenshots](#screenshots)

## Getting Started
The easiest way to get started is to use the installation script to install required libraries. Run
the following commands to install dependencies,

```bash
git clone https://github.com/bjadamson/BoomHS.git
cd BoomHS
scripts/install-dependencies.bash
scripts/bootstrap.bash
scripts/build.bash
scripts/build-and-run.bash
```

## Project Information
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
  + Source-code formatting. The clang-formatter tool is used for this purpose.
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

## Game Information
The concept is an online multi-player mmorpg/sandbox with multiple modes. One mode, the hard-core
mode (or rogue-like mode) will contain perma death.

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

## Screenshots
![Alt text](/screenshots/34.png?raw=true "06/30/2018 - NPCs can be killed")
![Alt text](/screenshots/31.png?raw=true "05/14/2018 - direction change")
![Alt text](/screenshots/28.png?raw=true "02/25/2018")
![Alt text](/screenshots/27.png?raw=true "02/24/2018")
![Alt text](/screenshots/26.png?raw=true "02/23/2018")
![Alt text](/screenshots/25.png?raw=true "02/22/2018")
![Alt text](/screenshots/24.png?raw=true "02/24/2018")
![Alt text](/screenshots/22.png?raw=true "02/05/2018")
![Alt text](/screenshots/21.png?raw=true "01/25/2018")
![Alt text](/screenshots/20.png?raw=true "01/25/2018")
![Alt text](/screenshots/18.png?raw=true "01/25/2018")
![Alt text](/screenshots/29.png?raw=true "02/28/2018 - First loaded prefab")
![Alt text](/screenshots/17.png?raw=true "01/24/2018")
![Alt text](/screenshots/16.png?raw=true "01/24/2018")
![Alt text](/screenshots/15.png?raw=true "01/10/2018")
![Alt text](/screenshots/14.png?raw=true "01/02/2018")
![Alt text](/screenshots/13.png?raw=true "01/02/2018")

#### links
* https://github.com/cbaggers/cepl

