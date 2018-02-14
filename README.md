# BoomHS
This repository contains the source code for the <b>BoomHS</b> project. BoomHS is a project in
active development.

![Alt text](/screenshots/22.png?raw=true "02/05/2018")

## Table of contents

  + [Getting Started](#getting-started)
  + [Project Information](#project-information)
    * [Install Dependencies](#install-dependencies)
    * [Bootstrapping](#bootstrap-the-project)
    * [Compiling](#compiling-the-project)
    * [Run the Project](#run-the-project)
    * [Other Information](#other-information)
    * [Hints](#hints)
  + [Game Information](#game-information)
  + [World](#world)
    * [World Story](#world-story)
    * [Behind the Scenes](#behind-the-scenes)
    * [Player Story](#player-story)
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
To install external dependencies, we rely on a script (for reproducability) so we can reliably
setup builds that work on new machines. The intent is to make it easy to startup development on a
new machine quickly. <i>Today</i> this is a goal, not a reality.
```bash
scripts/install-dependencies.bash
```

### Bootstrap the project
Bootstrapping the project is handled through a script. This script takes care of enumerating all
the source files in the project and telling cmake about them. It also takes care of finding the
compiler on your system. It then invokes cmake with the list of source file (and external libraries
defined in the script as well).
```bash
scripts/bootstrap.bash
```

### Compiling the Project
Compiling is easy, all the hard work was done during the bootstrapping process. Compiling the
project is as easy as running the build script.

```bash
scripts/build.bash
```

#### Important Note
When adding a new source file to the project, you must run the bootstrap script again.

### Run the project
Running the project is easy too (yep you guessed it) if your using the command line, you just need
to run the build-and-run script.
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
The game's name (atleast for now) is **"Fundamentally Broken"** but everything is under the alias
BoomHS for historical reasons. The game name isn't final, so keeping this alias until a permanent
name is chosen.

### World
**Fundamentally Broken** is set in a hand-guided procedurally generated world. The game operates
one turn at a time, initiated by the player taking some kind of action.

While adventuring, there is a (TODO) causing you to make forward progress down through dungeons
and maybe not stick around exploring everything you'd like too because it's right on your heels.

#### World Story
Initially the player is told they are on the planet (??), and that a few million years ago during a
hanging, an evil necromancer's last dying breath was to curse the planet. It was discoved by
reading his book **BOOK TILE 1** that deep beneath the subterranean layer there is a chance to undo
the curse and save the planet. Theres book mentions a companion book, **BOOK TITLE 2**, that when
read, contains further information about the origins of the curse. No adventurer that has gone
looking for the companion book has ever returned. This story is known by all little children in the
present time.

Since this curse was put on the world, life has been changed by reason ABC and distaster XYZ.
Currently most of civiliation lives in small villages. Most of the old cities have been abandoned to
reason XYZ. Daily life is an extreme challenge for the citizens of this world.

##### Behind the Scenes
> Behind the scenes, the player eventually learns the necromancer from the stories of old is
> actually "the creater" hiding among the planet. The player learns the world he lives in, is just
> biological simulation experiment being run by the creator. Well the creator only intended the
> world to live for some amount of time, but when the necromancer died he never turned off the
> simulation. Instead the simulation was corrupted by his death and has been running in a state of
> degrading continuity since then. The player learns that the only way to save his world is to
> escape his dimension, and enter the dimension where the creator lives. From there he can either
> choose to restart the simulation, or end it forever. Either way he will be alleiving the
> sufferring.

#### Player Story
You wake up millions of years after the world has been cursed, with your hands and feet restrained
by rope. You are being guided through the streets of a small town by a restless mob, about to be
hung for stealing food. You are with a couple other individuals, also restrained for similar
crimes, facing imminent punishment.

On your way to the noose, you gather enough courage and plead for your life, offering to venture
through "The Dungeons" and earn your forgiveness, as tradition allows. You know this is a death
sentence, but you figure atleast it is a chance. The others join you in begging for your a chance
to live, even it means for a short amount of time.

Upon your request, the village elders convene to discuss your request. It is made know that this
request is not often made. The elders of this village are maniacal, and decide that ulitmately it
is in their best interest to afford you the opportunity, after all what if they could save the
planet? However, there is a price for this chance. One of the adventurerers must be slain, for
their amusement. You are then informed you are the one who must decide which person lives, and
which person dies.

Once you make the decision, and the person is slain, your (new) party is brought to a famouos fork
in the road known as "The Fork" (TODO: name). At the crossroads sit a large statue of (???), the god
of (???). It is at this statue your party members must each decide whether or not they want to
worship the god of (???). After this process has been completed, the group must decide which path
to take. There are three separate paths, leading to three separate dungeons.

At this point, the player may start interacting with the group members, with their first choice
being to select an initial dungeon. Your are given a few initial supplies to begin your adventure,
but once you enter the dungeon you have a strange feeling you'll never be coming back to the
surface.

### Design Decisions
"Fundamentally Broken" is a game I'm creating that I want to play. This means:
  * Hard. The game is Hard. You are punished for playing poorly.
  * Death is Permanent.
  * Procedural content. The world should be generated from a random seed.
  * Hand-crafted content. the world has hand-created content injected into the procedural.
  * Weird. The world should be weird. I want weird things that capture my imagination as I play.
  * Emergent behavior. The world is full of Emergent behavior from complex interactions of simple
  systems.
  * Anti-scummable. the world should not be scummable, grinding the same monsters is not fun.
  * Scary. Alive. The world should be scary, and feel alive. It should feel like the game is always
    tring to kill you.
    + example: traveling should be dangerous.
  * No instant travel. It's fine if it makes sense, like a Wizard's teleport spell, but no modern
    day instant travelling mechanics.
  * Linear progression. Something should be continually pushing you to move forwards. Side
    progression is fine, but there should be a "food clock" if you will causing you to keep moving
    forwards through the game or loose.
  * Varied Content. Dungeons, open areas, narrow corridors, all with differing atmospheres.
  * Item based progression. There are no levels, instead character advancement is through acquiring
    more poweful items.
  * Tactical. The game is turn based.
  * Persistent. Monsters are randomly generated once during world gen.
  * No resting, except between levels. This causes you to carefully manage your group's HP and MP.
    (When a group advances to the next area, their MP and HP are restored.
    + This could be either advancing down the next set of stairs in a dungeon, or finally leaving an
    open area entering an ancient "Troll City" or something. Both of these would be scenarious where
    HP/MP are  restored.)
  * Consumables.

## Screenshots
![Alt text](/screenshots/24.png?raw=true "02/24/2018")
![Alt text](/screenshots/21.png?raw=true "01/25/2018")
![Alt text](/screenshots/20.png?raw=true "01/25/2018")
![Alt text](/screenshots/18.png?raw=true "01/25/2018")
![Alt text](/screenshots/17.png?raw=true "01/24/2018")
![Alt text](/screenshots/16.png?raw=true "01/24/2018")
![Alt text](/screenshots/15.png?raw=true "01/10/2018")
![Alt text](/screenshots/14.png?raw=true "01/02/2018")
![Alt text](/screenshots/13.png?raw=true "01/02/2018")
![Alt text](/screenshots/12.png?raw=true "12/29/2017")

#### links
* https://github.com/cbaggers/cepl
