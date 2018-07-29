#!/usr/bin/python
import sys
import time
pStates = {
		1:'para1',
		2:'para2',
		3:'para3',
		4:'para4',
		5:'para5',
       	6:'congrats'
}

flags = {
	'flag1',
	'flag2',
	'flag3',
	'flag4',
	'flag5'
}

def readFlags():
	try:
		f=open('flagBank.txt','r')
	except IOError:
		f=open('flagBank.txt','w')
		f.close()
		f=open('flagBank.txt','r')
	
	lines = f.readlines()
	curState=1
	for l in lines:
		l = l.rstrip('\n')
		if l in flags:
			curState+=1
	f.close()
	return curState	



def storeFlags(flag):
	f=open('flagBank.txt','a')
	f.write(flag+'\n')
	f.close()

def play(state=1):
	flag=''
	while state != 6:
		
		print pStates.get(state)
		flag = raw_input('Enter the flag: ')
		#sys.stdout.write("\033[F")
		#sys.stdout.write("\033[F")

		if flag == 'flag1' and state == 1: 
			state = 2
			storeFlags('flag1')
		elif flag == 'flag2' and state == 2: 
			state = 3
			storeFlags('flag2')
		elif flag == 'flag3' and state == 3: 
			state = 4
			storeFlags('flag3')
		elif flag == 'flag4' and state == 4: 
			state = 5
			storeFlags('flag4')
		elif flag == 'flag5' and state == 5: 
			state = 6
			storeFlags('flag5')
		else:
			print 'Flag is incorrect'
		
		

	print pStates.get(state)

def main():  
	state = readFlags()
	play(state)

if __name__ == '__main__':
    main()
		
		

