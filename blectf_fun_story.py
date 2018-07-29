#!/usr/bin/python

def switch(arg):
	pStates = {
		'p1':'para1',
		'p2':'para2',
		'p3':'para3',
		'p4':'para4',
		'p5':'para5',
       		'done':'congrats'
	}
	print pStates.get(arg, 'Incorrect flag:');

def main():  
	state='p1'; 
	flag='';
	while state != 'done':
		switch(state);
		flag = raw_input('Enter the flag: ');
		if flag == 'flag1': 
			state = 'p2';
		if flag == 'flag2': 
			state = 'p3';
		if flag == 'flag3': 
			state = 'p4';
		if flag == 'flag4': 
			state = 'p5';
		if flag == 'flag5': 
			state = 'done';
	switch(state);

if __name__ == '__main__':
    main()
		
		

