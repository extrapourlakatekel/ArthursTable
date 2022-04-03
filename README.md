# ArthursTable
Chat and support software for pen&amp;paper roleplay

ArthursTable provides a roleplay-optimized audio chat with sound direction processing so that every player seems to sit on a different 
position on a virtual table. With the use of an optional  head tracker you can turn your head to focus each of the players seperately. 
This allows for far less chat discipline than with other chat programs increasing the illusion of sharing one table. As a special roleplay-feature
the gamemaster can split up the group into teams, so that they can only chat within. It is also possible to add some voice effects when 
communicating between the teams to simulate a telephone line or a radio link. The gamemaster and every player can add effects to their voices.
Simulation of room acoustics is planned, but not implemented yet.

In addition to the voices, the gamemaster can provide a music stream and add ambient sounds to improve immersion. It also features a 
viewer for pictures and maps with some dungeon crawl enhancements and a sketch pad where all players can draw at the same time.
Below these features works a network engine that optimizes low voice latency and an overall bandwidth limiting, so that a 1MBit/s 
connection is sufficient for up to 5 players.

Although you can use a server to simlify exchanging of the network addresses, ArthursTable uses direct connections to each player without 
relaying via a server. 

# Getting started
First of all, the status of this software is very alpha. I started developing with the beginning of the pandemic and came surprisingly far, but since 
my group is now able to play in real life again, many of the features are not detailed at the moment or are not as convenient as they should be. ArtursTable 
might be a good roleplay chat software compared to others but cannot beat the real thing.

In addition, you'll need Linux at the moment since I do not know how to use the Windows Sound System for fast full duplex audio. To compile 
ArthursTable it is required to have the Qt-5 libraries installed which will likely be part of your distribution or - if not - can be 
obtained from https://www.qt.io/. In addition to run it you'll need mget (when using the server functionality) and mpeg123 to be able to 
playback music and background sounds. For voice and music compression the OPUS library is required too (by the way, congrats to this library - fast, easy to use, perfect quality - loved it). And you have to have the PulseAudio library installed on your machine. Dependent on the initial settings you might have to tweak your 
local settings to get low latency.

Before you start the program, be sure that you use a headset and connect it to your PC. ArthursTable only works with headphones to ensure 
directivity effect and avoid the need of echo cancellatian.
When you start the program, you (hopefully) see the table selection window. At "... or create a new one" type the name of your roleplay groups 
choice for a new table.
Then press "New" or simply hit RETURN. After that set up the newly generated table. Press the settings button (lower leftmost) to enter the setup. 
First chose a "Local characters name" - usualy the name of the character you play. When you are the gamemaster, enter "GAMEMASTER" instead of a 
name. Also enter a passphrase also known to the other players. All players must use the same table name and the same passphrase.
Then fill in the other players' character names and their IP-adresses directly in IP4 or IP6 format. Additionally inform the other players of
your IP.
If you have setup a Punching Server, you can check the button and fill in its name instead of providing the seperate IPs.
Press done.

# Code
First: Don't blame me for my coding style. I know, it changed several times throughout the project with increasing experiance. 
Alltough I appreciate contribution to this project, my time is tightly limited and my experiance in managing software projects with 
more than one programmer is nonexistant. Feel free to start your own branch but do not expect answered pull requests. Sorry - got some more 
important things to do.

# Attributions for the icons
ArtursTable: own art 
back: <a href="https://www.flaticon.com/free-icons/next" title="next icons">Next icons created by Google - Flaticon</a>
bird: https://icon-library.com/icon/bird-icon-29.html.html>Bird Icon # 153433 (modified)
broken: <a href="https://www.flaticon.com/free-icons/broken-image" title="broken image icons">Broken image icons created by verry purnomo - Flaticon</a>
debug: <a href="https://www.flaticon.com/free-icons/debug" title="debug icons">Debug icons created by Freepik - Flaticon</a>
dice: <a href="https://www.flaticon.com/free-icons/dice" title="dice icons">Dice icons created by Dimi Kazak - Flaticon</a>
focus: <a href="https://www.flaticon.com/free-icons/focus" title="focus icons">Focus icons created by Ilham Fitrotul Hayat - Flaticon</a>
fullscreen: <a href="https://www.flaticon.com/free-icons/fullscreen" title="fullscreen icons">Fullscreen icons created by Those Icons - Flaticon</a>
gamemaster: own art
gear: <a href="https://www.flaticon.com/free-icons/system" title="system icons">System icons created by Freepik - Flaticon</a>
hidden: https://icon-library.com/icon/visible-icon-9.html.html>Visible Icon # 64047
loop: <div>Icons made by <a href="https://www.flaticon.com/authors/gregor-cresnar" title="Gregor Cresnar">Gregor Cresnar</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>
microphone: <a href="https://www.flaticon.com/free-icons/mic" title="mic icons">Mic icons created by Dave Gandy - Flaticon</a>
mute: <a href="https://www.flaticon.com/free-icons/mute" title="mute icons">Mute icons created by Freepik - Flaticon</a>
nofullscreen:<a href="https://www.flaticon.com/free-icons/fullscreen" title="fullscreen icons">Fullscreen icons created by Those Icons - Flaticon</a>
noloop: <div>Icons made by <a href="https://www.flaticon.com/authors/gregor-cresnar" title="Gregor Cresnar">Gregor Cresnar</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div> (modified)
note: https://icon-library.com/icon/bird-icon-29.html.html>Bird Icon # 153433 (modified)
notepad: https://icon-library.com/icon/notepad-icon-png-7.html.html>Notepad Icon Png # 366455
play: https://icon-library.com/icon/play-icon-png-5.html.html>Play Icon Png # 312936
reload: https://icon-library.com/icon/reset-icon-png-2.html.html>Reset Icon Png # 76591
skip: <a href="https://www.flaticon.com/free-icons/next" title="next icons">Next icons created by Google - Flaticon</a>
stop: own art
unknown: https://icon-library.com/icon/anonymous-female-icon-5.html.html>Anonymous Female Icon # 406678
visible: https://icon-library.com/icon/visible-icon-25.html.html>Visible Icon # 64036
zero: own art


