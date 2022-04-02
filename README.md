# ArthursTable
Chat and support software for pen&amp;paper roleplay

ArthursTable provides a roleplay-optimized audio chat with sound direction processing so that every player seems to sit on a different 
position on a virtual table. With the use of an optional  head tracker you can turn your head to focus each of the players seperately. 
This allows for far less chat discipline than with other chat programs increasing the illusion of sharing one table.
Although you can use a server to simlify exchanging the network addresses, ArthursTable uses direct connections to each player without 
relaying via a server. 
In addition to the voices, the gamemaster can provide a music stream and add ambient sounds to improve immersion. It also features a 
viewer for pictures and maps with some dungeon crawl enhancements and a sketch pad where all players can draw at the same time.
Below these features works a network engine that optimizes low voice latency and an overall bandwidth limiting, so that a 1MBit/s 
connection is sufficient for up to 5 players.

# Getting started
First of all: You'll need Linux at the moment since I do not know how to use the Windows Sound System for fast full duplex audio. To compile 
ArthursTable it is required to have the Qt-5 libraries installed which will likely be part of your distribution or - if not - can be 
obtained from https://www.qt.io/. In addition to run it you'll need mget (when using the server functionality) and mpeg123 to be able to 
playback music and background sounds.
For voice and music compression the OPUS library is required too.
Icons are not yet included - must get royalty free ones first! Sorry, still in progress...

Before you start the program, be sure that you use a headset and connect it to your PC. ArthursTable only works with headphones to ensure 
directivity effect and avoid the need of echo cancellatian.
When you start the program, you (hopefully) see the table selection window. At "... or create a new one" type the name of your roleplay groups 
choice for a new table.
Then press "New" or simply hit RETURN. After that set up the newly generated table. Press the settings button (lower leftmost) to enter the setup. 
First chose a "Local characters name" - usualy the name of the character you play. When you are the gamemaster, enter "GAMEMASTER" instead of a 
name. Also enter a passphrase also known to the other players. All players must use the same table name and the same passphrase.
Then fill in the other players' character names and their IP-adresses directly in IP4 or IP6 format. Additionally inform the other players of
your IP.
If you have setup a Punching Server, you can check the button and fill in its name instead.
Press done.


# Coding 
First: Don't blame me for my coding style. I know, it changed several times throughout the project with increasing experiance. 
Alltough I appreciate contribution to this project, my time is tightly limited and my experience in managing software projects with 
more than one programmer is nonexistant. Feel free to start your own branch but do not expect answered pull requests.
