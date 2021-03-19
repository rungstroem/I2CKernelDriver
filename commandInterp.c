#include <stdio.h>
#include <string.h>

unsigned char commandInt(char *buf){
	if(!strcmp(buf, "getID")){
		return(0x75);
	}else if(!strcmp(buf, "TEMPH")){
		return(0x42);
	}else if(!strcmp(buf, "TEMPL")){
		return(0x41);
	} else{
		return(0x00);
	}
}


int main(){
	unsigned char cmd;
	char message[] = {'T','E','M','P','H'};
	char *message_ptr;
	message_ptr = message;

	printf("%s\n", message);
	printf("%s\n", message_ptr);	// This is apparently how you use a pointer...

	cmd = 0x00;
	printf("command init %x\n", cmd);
	cmd = commandInt(message);
	printf("command convert %x\n", cmd);

	return 0;
}
