#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys

# dict of state:paragraph pairs to print
pStates = {
    0: 'Find the MAC of the device using hcitool lescan. The flag is MACXX:XX:XX:XX:XX:XX',
    1: 'What’s the Device Name?(handle=0x0016)',
    2: 'Okay you’re all set and targeting a working CTF so lets begin the tale.\n\nYou just got back from a long day of drinking and hacking at Blackhat. Eager to play with your new toys you sit down somewhere in the casino to try your hands at a new BLECTF that was presented earlier today. You chug through most of the flags and start to become drowsy as it’s entering the early hours of the morning. Retreating back to the comfort of you hotel room and bed seems like an excellent idea and so you do so. On arrival you immediately pass out on the bed.\n\nYou awaken in a strange bedroom, its much dingier than what you remember falling asleep in. Your covers are fur pelts and most of the furniture has a rough wooden carved aesthetic. Although you remember that you aren’t where you were before, you have trouble recalling basic facts about yourself. For example what was your name again: ',
    3: 'Ah that’s right that was your name. Your memories come back to you and you decide to leave your room and navigate your way to the lower level. You are presented with what appears to be a standard tavern with three old gentleman and a bartender. In an attempt to gain some information about your whereabouts you resolve to approach the three older and ask them some questions about your predicament. Talk to the first gentleman at UUID 0xff04:',
    4: 'Talk to second gentleman. Single notify.',
    5: 'Talk to third gentleman. Single indicate',
    6: 'You continue to talk to the old gentlemen and come to the conclusion that you have fallen into some kind of fantasy world with a medieval aged theme. Electricity has yet to be discovered so all your skills in hacking are essentially useless in this world. Being an adventurer seems to be the primary occupation in this world, based on all your experience playing video games, watching tv, and that one time you played DND you figured it would be your best option to survive. Your first mission is to find a weapon perhaps asking the bartender where to get one would be a good idea: prompt him with the message "Where 2 get a sword"',
    7: 'The bartender tells you about his blacksmith friend who can help you make a sword for free if you don’t have any money. You just need to gather the 5 necessary components for a sword and give them to the blacksmith. Thankfully there’s a bunch of free parts sprawled throughout the local graveyard. He warns you that blades are much harder to find as they rust and fall apart, finding a working blade might require looking somewhere else than the graveyard. Sometimes they can be found in a random log by the river. One you have all 5 parts give them to the blacksmith in the correct order, he conveniently omitted what the correct order was, to make your sword.',
    8: 'You finally have a weapon now! The blacksmith was impressed by your skill and wants to hire you to help escort his wares to a neighboring town. He offers you some armor, paid upfront, to complete this mission as well as some gold for food and lodging when you reach your destination . You agree not only to complete your setup and get some money, but also as thanks for helping you make your sword. Several hours after leaving the town you encounter a bridge troll. In order to cross the bridge you have to interact(hint pass data in hex form in this mode) with the troll and answer his questions. Before you start the troll recommends that if you have any bitlys now might be a good time to use them.',
    9: 'After answering the Troll’s questions you cross the bridge and continue on your way. Its nearing the end of the day so the blacksmith suggests setting up camp for the night. As dusk sets on our camp you here a rustle surrounding your camp you hear three distinct voices coming from the bushes but you can’t see them. The blacksmith notifies you that they are goblins and although you can’t see them you can use the local terrain to get a jump on them by following their voices and overflow them with your sword. They each have an hp of ‘.’ and setting that to ‘0’ will kill them.',
    10: 'You successfully killed the goblins and eventually arrived at your destination. At this point you said farewell to the blacksmith and collected your gold heading in the direction of the nearest lodging. On your way there you reflect on what you accomplished you talked with the old gentlemen, collected various components for a sword, escorted a villager, completed a trolls puzzle, and beat a group of goblins. If this was a video game you would have just finished the tutorial given the information gathering, collecting, escorting, solving puzzles, and combat missions you underwent. You wonder what your future missions will hold perhaps saving a princess or fighting a demon lord would be appropriate. For now you are content with waiting for someone else to give you a new mission or maybe you want to make a mission for yourself. Either way it’s time for a good drink and a nap.\n\nThe End. ',
}

# list of flags
flags = [
    'MACXX:XX:XX:XX:XX:XX',
    'fun=fantasyuniverse',
    'thisisaflag4yurname',
    'flag:bit.ly/2LOGYtf',
    'flag::bit.ly/2v3Qlhq',
    'flag::bit.ly/2NLcVmC',
    'goget5piecesofasword',
    'yt87a23bg56gt756hd66',
    'qazwsxedcrfvtgbyhnuj',
    'overoveroveroverflow'
]

# read flags from bank to set current position in story
def readFlags():
    # if file doesn't exist create it first
    try:
        f = open('flagBank.txt', 'r')
    except IOError:
        f = open('flagBank.txt', 'w')
        f.close()
        f = open('flagBank.txt', 'r')

    # read in lines and count valid flags
    # TODO: make it confirm order is right
    lines = f.readlines()
    curState = 0
    for l in lines:
        l = l.rstrip('\n')
        if l in flags:
            curState += 1
    f.close()
    return curState


# store flags as they are discovered
def storeFlags(flag):
    f = open('flagBank.txt', 'a')
    f.write(flag+'\n')
    f.close()


# transition state diagram and output ie playing
def play(state=0):
    flag = ''
    while state != 10:

        print pStates.get(state)
        flag = raw_input('Enter the flag: ')
        # sys.stdout.write("\033[F")
        # sys.stdout.write("\033[F")

        if len(flag) == len(flags[0]) and state == 0:#just gonna be lazy on the mac
            state = 1
            storeFlags(flags[0])
        elif flag == flags[1] and state == 1:
            state = 2
            storeFlags(flags[1])
        elif flag == flags[2] and state == 2:
            state = 3
            storeFlags(flags[2])
        elif flag == flags[3] and state == 3:
            state = 4
            storeFlags(flags[3])
        elif flag == flags[4] and state == 4:
            state = 5
            storeFlags(flags[4])
        elif flag == flags[5] and state == 5:
            state = 6
            storeFlags(flags[5])
        elif flag == flags[6] and state == 6:
            state = 7
            storeFlags(flags[6])
        elif flag == flags[7] and state == 7:
            state = 8
            storeFlags(flags[7])
        elif flag == flags[8] and state == 8:
            state = 9
            storeFlags(flags[8])
        elif flag == flags[9] and state == 9:
            state = 10
            storeFlags(flags[9])
        else:
            print 'Flag is incorrect'
            raw_input('Press ENTER to reprint last story entry and then try again: ')
    print pStates.get(state)

def main():
    state = readFlags()
    play(state)


if __name__ == '__main__':
    main()
