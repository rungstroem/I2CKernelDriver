#ifndef CMDINTERPRETER_H_INCLUDED
#define CMDINTERPRETER_H_INCLUDED

unsigned char commandIntMPU(char *buf){
	if(!strcmp(buf,"getID\n")){
		return(0x75);
	}else if(!strcmp(buf, "TEMPH\n")){
		return(0x41);
	}else if(!strcmp(buf, "TEMPL\n")){
		return(0x42);
	}else if(!strcmp(buf, "ACCXH\n")){
		return(0x3B);
	}else if(!strcmp(buf, "ACCXL\n")){
		return(0x3C);
	}else if(!strcmp(buf, "ACCYH\n")){
		return(0x3D);
	}else if(!strcmp(buf, "ACCYL\n")){
		return(0x3E);
	}else if(!strcmp(buf, "ACCZH\n")){
		return(0x3F);
	}else if(!strcmp(buf, "ACCZL\n")){
		return(0x40);
	}else if(!strcmp(buf, "GYRXH\n")){
		return(0x43);
	}else if(!strcmp(buf, "GYRXL\n")){
		return(0x44);
	}else if(!strcmp(buf, "GYRYH\n")){
		return(0x45);
	}else if(!strcmp(buf, "GYRYL\n")){
		return(0x46);
	}else if(!strcmp(buf, "GYRZH\n")){
		return(0x47);
	}else if(!strcmp(buf, "GYRZL\n")){
		return(0x48);
	}else{
		return(0x00);	//Return NULL if no command matches
	}
}

unsigned char registerConverterMPU(char *buf){
	if(!strcmp(buf,"getID")){
		return(0x75);
	}else if(!strcmp(buf, "TEMPH")){
		return(0x41);
	}else if(!strcmp(buf, "TEMPL")){
		return(0x42);
	}else if(!strcmp(buf, "ACCXH")){
		return(0x3B);
	}else if(!strcmp(buf, "ACCXL")){
		return(0x3C);
	}else if(!strcmp(buf, "ACCYH")){
		return(0x3D);
	}else if(!strcmp(buf, "ACCYL")){
		return(0x3E);
	}else if(!strcmp(buf, "ACCZH")){
		return(0x3F);
	}else if(!strcmp(buf, "ACCZL")){
		return(0x40);
	}else if(!strcmp(buf, "GYRXH")){
		return(0x43);
	}else if(!strcmp(buf, "GYRXL")){
		return(0x44);
	}else if(!strcmp(buf, "GYRYH")){
		return(0x45);
	}else if(!strcmp(buf, "GYRYL")){
		return(0x46);
	}else if(!strcmp(buf, "GYRZH")){
		return(0x47);
	}else if(!strcmp(buf, "GYRZL")){
		return(0x48);
	}else{
		return(0x00);	//Return NULL if no command matches
	}
}

#endif
