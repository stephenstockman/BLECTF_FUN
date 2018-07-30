## BLE Capture the Flag: Fantasy Universe
The purpose of BLECTF_FUN is to grow on the initial design of [ble_ctf](https://github.com/hackgnar/ble_ctf). Although it reiterates some of the information found in the original it also incorporates some new challenges found in the last couple flags. All the flags are introduced and managed in a story format by blectf_fun_story.py. Flags are autosaved on entry and progress resumes on rerunning the python script. The CTF uses the same ESP32 micrococontroller as the original and is meant to be used as a refresher lesson some time after completing the original. 

## Setting Up the CTF
Visit https://github.com/hackgnar/ble_ctf for setup.

## How to Find Flags
All flags after the first two directly correspond to elements in the story. For example talking to the older gentleman in the story could mean you need to char-write-req --listen at the address of one of the gentleman. Note that everything read, notified, indicated, whatever should be converted to ASCII. Lot's of hints and meaning goes into what the esp32 returns so having a hex to ascii converter on hand is necessary.ALL FLAGS ARE 20 CHARACTERS AND SHOULD BE SUBMITTED IN ASCII FORMAT. 

## How to Submit Flags
Run `python blectf_fun_story.py`
Enter flag when prompted. REPEAT:ALL FLAGS ARE 20 CHARACTERS AND SHOULD BE SUBMITTED IN ASCII FORMAT. 

## If You Have Beaten ble_ctf Recently
I highly suggest just skipping most of the initial flags and going to the good ones. All flags can be easily found in blectf_fun_story.py just run it once and throw the flags you want in the flagBanks.txt file it creates. The "good ones" would be the last 3 flags as only they have any new information. You will need the three links found in the previous flags to defeat the bridge troll though.

## Author's Notes
Overall this is more of a fan-made project by an intern who wanted to play around a bit on the developement side after completing ble_ctf. It currently has only been beta tested by myself so there's probably some bugs so sorry about that if you find them post them in issues or submit a pull request with a fix. Most of the code is borrowed form esp-idf demo documentation, similar to ble_ctf, and draws direct inspiration from ble_ctf. I may add some more flags in the future but would love it if someone else added their own or made their own verison of it with their story. I am note a writer so I aplogize for the bad writing but it does, in my opinion, get the hints across for the next flag so some consideration for fluidity/plot needs to be taken into account.
