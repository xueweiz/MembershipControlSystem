2. Machine addresses are stored in Address.add
3. call it by: 
		make && ./logger 3071
   while asked for command, type:
   		grep apple *.log
4. to make server fail during greping, in log.cpp, just
	//ret = robustWrite(connFd, running, 3 );
	OR by using Control +C
