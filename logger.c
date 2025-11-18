#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <unistd.h>
#include <sys/epoll.h>

#define BUFFER_SIZE 1024
#define DEVICE_TYPE "event"
#define DIRECTORY "/dev/input"
#define DEVICE_MAX_NAME_LEN (256+11)
#define MAX_NAME_LEN 256
#define BIN_CHAR_ENOUGH 25
#define MAX_KEYBOARDS 10
#define MAXEVENTS 2

#define NBITS(x) (((x)+(sizeof(long)*8)-1)/(sizeof(long)*8))

static int set_fd_nonblocking(int fd){
	int flags=fcntl(fd,F_GETFL,0);
	if(flags==-1){
		perror("fnctl F_GETFL failed");
		return -1;
	}

	if(fcntl(fd,F_SETFL,flags|O_NONBLOCK)==-1){
		perror("fcntl F_SETFL failed");
		return -1;
	}

	return 0;
}

static void exit_close(int eFD,DIR* dir,int* fds){
	if(fds!=NULL){
		for(int i=0;i<MAX_KEYBOARDS;i++)
			if(fds[i]!=-1)
				close(fds[i]);
	}
	if(eFD!=-1)
		close(eFD);
	if(dir!=NULL)
		closedir(dir);
}

static void ev_code_to_ascii(__u16 key_code,__s32 value,char* key){
    if(value==0)
		key[0]='\0';

    switch (key_code){
		case KEY_ESC:              snprintf(key,6,"<ESC>");break;
		case KEY_KP1:
		case KEY_1:                snprintf(key,2,"1");break;
		case KEY_KP2:
		case KEY_2:                snprintf(key,2,"2");break;
		case KEY_KP3:
		case KEY_3:                snprintf(key,2,"3");break;
		case KEY_KP4:
		case KEY_4:                snprintf(key,2,"4");break;
		case KEY_KP5:
		case KEY_5:                snprintf(key,2,"5");break;
		case KEY_KP6:
		case KEY_6:                snprintf(key,2,"6");break;
		case KEY_KP7:
		case KEY_7:                snprintf(key,2,"7");break;
		case KEY_KP8:
		case KEY_8:                snprintf(key,2,"8");break;
		case KEY_KP9:
		case KEY_9:                snprintf(key,2,"9");break;
		case KEY_KP0:
		case KEY_0:                snprintf(key,2,"0");break;
		case KEY_MINUS:            snprintf(key,2,"-");break;
		case KEY_KPEQUAL:
		case KEY_EQUAL:            snprintf(key,2,"=");break;
		case KEY_BACKSPACE:        snprintf(key,12,"<BACKSPACE>");break;
		case KEY_TAB:              snprintf(key,6,"<TAB>");break;
		case KEY_Q:                snprintf(key,2,"q");break;
		case KEY_W:                snprintf(key,2,"w");break;
		case KEY_E:                snprintf(key,2,"e");break;
		case KEY_R:                snprintf(key,2,"r");break;
		case KEY_T:                snprintf(key,2,"t");break;
		case KEY_Y:                snprintf(key,2,"y");break;
		case KEY_U:                snprintf(key,2,"u");break;
		case KEY_I:                snprintf(key,2,"i");break;
		case KEY_O:                snprintf(key,2,"o");break;
		case KEY_P:                snprintf(key,2,"p");break;
		case KEY_LEFTBRACE:        snprintf(key,2,"[");break;
		case KEY_RIGHTBRACE:       snprintf(key,2,"]");break;
		case KEY_KPENTER:
		case KEY_ENTER:            snprintf(key,8,"<ENTER>");break;
		case KEY_RIGHTCTRL:
		case KEY_LEFTCTRL:         snprintf(key,7,"<CTRL>");break;
		case KEY_A:                snprintf(key,2,"a"); break;
		case KEY_S:                snprintf(key,2,"s"); break;
		case KEY_D:                snprintf(key,2,"d"); break;
		case KEY_F:                snprintf(key,2,"f"); break;
		case KEY_G:                snprintf(key,2,"g"); break;
		case KEY_H:                snprintf(key,2,"h"); break;
		case KEY_J:                snprintf(key,2,"j"); break;
		case KEY_K:                snprintf(key,2,"k"); break;
		case KEY_L:                snprintf(key,2,"l"); break;
		case KEY_SEMICOLON:        snprintf(key,2,";"); break;
		case KEY_APOSTROPHE:       snprintf(key,2,"'"); break;
		case KEY_GRAVE:            snprintf(key,2,"`"); break;
		case KEY_RIGHTSHIFT:
		case KEY_LEFTSHIFT:        snprintf(key,8,"<SHIFT>");break;
		case KEY_BACKSLASH:        snprintf(key,2,"\\");break;
		case KEY_Z:                snprintf(key,2,"z");break;
		case KEY_X:                snprintf(key,2,"x");break;
		case KEY_C:                snprintf(key,2,"c");break;
		case KEY_V:                snprintf(key,2,"v");break;
		case KEY_B:                snprintf(key,2,"b");break;
		case KEY_N:                snprintf(key,2,"n");break;
		case KEY_M:                snprintf(key,2,"m");break;
		case KEY_KPCOMMA:
		case KEY_COMMA:            snprintf(key,2,",");break;
		case KEY_DOT:              snprintf(key,2,".");break;
		case KEY_SLASH:            snprintf(key,2,"/");break;
		case KEY_KPASTERISK:       snprintf(key,2,"*");break;
		case KEY_RIGHTALT:
		case KEY_LEFTALT:          snprintf(key,6,"<ALT>");break;
		case KEY_SPACE:            snprintf(key,2," ");break;
		case KEY_CAPSLOCK:         snprintf(key,7,"<CAPS>");break;
		case KEY_F1:               snprintf(key,5,"<F1>");break;
		case KEY_F2:               snprintf(key,5,"<F2>");break;
		case KEY_F3:               snprintf(key,5,"<F3>");break;
		case KEY_F4:               snprintf(key,5,"<F4>");break;
		case KEY_F5:               snprintf(key,5,"<F5>");break;
		case KEY_F6:               snprintf(key,5,"<F6>");break;
		case KEY_F7:               snprintf(key,5,"<F7>");break;
		case KEY_F8:               snprintf(key,5,"<F8>");break;
		case KEY_F9:               snprintf(key,5,"<F9>");break;
		case KEY_F10:              snprintf(key,6,"<F10>");break;
		case KEY_NUMLOCK:          snprintf(key,5,"<NL>");break;
		case KEY_SCROLLLOCK:       snprintf(key,5,"<SL>");break;
		case KEY_KPMINUS:          snprintf(key,2,"-");break;
		case KEY_KPPLUS:           snprintf(key,2,"+");break;
		case KEY_KPDOT:            snprintf(key,2,".");break;
		case KEY_F11:              snprintf(key,6,"<F11>");break;
		case KEY_F12:              snprintf(key,6,"<F12>");break;
		case KEY_KPSLASH:          snprintf(key,2,"/");break;
		case KEY_UP:               snprintf(key,5,"<UP>");break;
		case KEY_PAGEUP:           snprintf(key,10,"<PAGE_UP>");break;
		case KEY_LEFT:             snprintf(key,7,"<LEFT>");break;
		case KEY_RIGHT:            snprintf(key,8,"<RIGHT>");break;
		case KEY_END:              snprintf(key,6,"<END>");break;
		case KEY_DOWN:             snprintf(key,7,"<DOWN>");break;
		case KEY_PAGEDOWN:         snprintf(key,12,"<PAGE_DOWN>");break;
		case KEY_INSERT:           snprintf(key,9,"<INSERT>");break;
		case KEY_DELETE:           snprintf(key,9,"<DELETE>");break;
		case KEY_KPPLUSMINUS:      snprintf(key,6,"<+/->");break;
		case KEY_F13:              snprintf(key,6,"<F13>");break;
		case KEY_F14:              snprintf(key,6,"<F14>");break;
		case KEY_F15:              snprintf(key,6,"<F15>");break;
		case KEY_F16:              snprintf(key,6,"<F16>");break;
		case KEY_F17:              snprintf(key,6,"<F17>");break;
		case KEY_F18:              snprintf(key,6,"<F18>");break;
		case KEY_F19:              snprintf(key,6,"<F19>");break;
		case KEY_F20:              snprintf(key,6,"<F20>");break;
		case KEY_F21:              snprintf(key,6,"<F21>");break;
		case KEY_F22:              snprintf(key,6,"<F22>");break;
		case KEY_F23:              snprintf(key,6,"<F23>");break;
		case KEY_F24:              snprintf(key,6,"<F24>");break;
		case KEY_FN:               snprintf(key,5,"<FN>");break;
		case KEY_DOLLAR:           snprintf(key,2,"$");break;
		case KEY_KPLEFTPAREN:      snprintf(key,2,"(");break;
		case KEY_KPRIGHTPAREN:     snprintf(key,2,")");break;
		default:                   snprintf(key,12,"<non-ascii>");break;
   }
}

//void print_input_event(struct input_event *ev) {
//    // Print the entire input_event struct
//    printf("Event Type: %u\n", ev->type);
//    printf("Event Code: %u\n", ev->code);
//    printf("Event Value: %d\n", ev->value);
//
//    // Print timestamp (time of the event)
//    printf("Time: %ld.%06ld (seconds.microseconds)\n", ev->time.tv_sec, ev->time.tv_usec);
//}

//static const void hex_char_to_bin_char(char* hex_char,char* bin_char){
//	for(int i=0;i<BIN_CHAR_ENOUGH;i++){
//		const unsigned char hex_digit=hex_char[i];
//		switch(hex_digit){
//			case '0':strcat(bin,"0000");break;
//			case '1':strcat(bin,"0001");break;
//			case '2':strcat(bin,"0010");break;
//			case '3':strcat(bin,"0011");break;
//			case '4':strcat(bin,"0100");break;
//			case '5':strcat(bin,"0101");break;
//			case '6':strcat(bin,"0110");break;
//			case '7':strcat(bin,"0111");break;
//			case '8':strcat(bin,"1000");break;
//			case '9':strcat(bin,"1001");break;
//			case 'a':
//			case 'A':strcat(bin,"1010");break;
//			case 'b':
//			case 'B':strcat(bin,"1011");break;
//			case 'c':
//			case 'C':strcat(bin,"1100");break;
//			case 'd':
//			case 'D':strcat(bin,"1101");break;
//			case 'e':
//			case 'E':strcat(bin,"1110");break;
//			case 'f':
//			case 'F':strcat(bin,"1111");break;
//		}
//	}
//}
//
//static const void ev_check(char* ev_str){
//	char last_char;
//	for(int i=0;ev_str[i];i++)
//		last_char=ev_str[i];
//
//	//hex_char to int conversion
//	if(last_char>='0'&&last_char<='9')
//		last_int=last_char-'0';
//	else if(last_char>='A'&&last_char<='F')
//		last_int=last_char-'A'+10;
//	else if(last_char>='a'&&last_char<='f')
//		last_int=last_char-'a'+10;
//
//	if(last_int&0x02) //2nd bit = EV_KEY
//		puts("device supports key input");
//	else 
//		puts("device does NOT support key input");
//}
//
//static const void key_check(const char* key_char){
//	char bin_char[BIN_CHAR_ENOUGH];
//	hex_char_to_bin_char(key_char,bin_char);
//
//	int count=0;
//	for(int i=16;i<BIN_CHAR_ENOUGH;i++){
//		if(bin_char[i]=='1')
//			count++;
//	}
//
//	if(count==5)
//		puts("pretty sure this devices supports letters 'A'->'Z'");
//	else
//		puts("pretty sure this device does NOT support the full keyboard");
//}

int main(int argc,char** argv){
	//const FILE* devices=fopen("proc/bus/input/devices","r");
	//if(devices==NULL){
	//	puts("fopen failed");
	//	return -1;
	//}

	//char buffer[BUFFER_SIZE];
	//while(1){
	//	const size_t bytes_read=fread(buffer,BUFFER_SIZE,1,devices);
	//	if(strstr


	//	if(butes_read<BUFFER_SIZE)
	//		break;

	DIR* dir=opendir(DIRECTORY);
	if(dir==NULL){
		perror("opendir failed");
		return -1;
	}

	struct dirent* entry={NULL};
	int keyboard_fds[MAX_KEYBOARDS]={-1};
	int keyboard_fds_index=0;

	while((entry=readdir(dir))!=NULL){
		char device[DEVICE_MAX_NAME_LEN]={0};
		if(strstr(entry->d_name,DEVICE_TYPE)!=NULL){
			snprintf(device,sizeof(device),"%s/%s",DIRECTORY,entry->d_name);

			int fd=open(device,O_RDONLY,O_NONBLOCK);
			if(fd==-1){
				perror("open failed");
				exit_close(-1,dir,keyboard_fds);
				return -1;
			}

			if(set_fd_nonblocking(fd)==-1){
				close(fd);
				continue;
			}

			char device_name[MAX_NAME_LEN];
			if(ioctl(fd,EVIOCGNAME(MAX_NAME_LEN),device_name)<0){
				perror("ioctl failed");
				close(fd);
				exit_close(-1,dir,keyboard_fds);
				return -1;
			}

			unsigned long ev_bits[NBITS(KEY_MAX)];
			if(ioctl(fd,EVIOCGBIT(EV_SYN,sizeof(ev_bits)),ev_bits)<0){
				perror("ioctl failed");
				close(fd);
				exit_close(-1,dir,keyboard_fds);
				return -1;
			}

			unsigned long key_bits[NBITS(KEY_MAX)];
			if(ioctl(fd,EVIOCGBIT(EV_KEY,sizeof(key_bits)),key_bits)<0){
				perror("ioctl failed");
				close(fd);
				exit_close(-1,dir,keyboard_fds);
				return -1;
			}

			//check evbits
			if(ev_bits[0]&0x02){
				//puts("KEY");

				//check keybits
				if(key_bits[0]&0xff0000){
					//printf("%s - keyboard\n",device_name);
					if(keyboard_fds_index<MAX_KEYBOARDS)
						keyboard_fds[keyboard_fds_index++]=fd;
				}else{
					//puts("NO");
					close(fd);
				}
			}
			//else 
			//	puts("NO");

			//ev_check(ev_bit);
			//key_check(key_bit);

			//if(ioctl(fd,EVIOCGRAB,1)==-1){
			//	perror("ioctl failed");
			//	return -1;
			//}

			//struct input_event ev;
		        //while (1) {
		        //    ssize_t bytes = read(fd, &ev, sizeof(struct input_event));
		        //    if (bytes == -1) {
		        //        perror("Error reading event");
		        //        break;
		        //    }

		        //    // Print the entire event struct
		        //    print_input_event(&ev);
		        //    printf("\n");
		        //}	
		}
	}

	//manage input from multiple keyboards
	struct epoll_event ev;
	const int eFD=epoll_create1(0);
	if(eFD==-1){
		perror("epoll_create failed");
		exit_close(eFD,dir,keyboard_fds);
		return -1;
	}
	for(int i=0;i<keyboard_fds_index;i++){
		ev.events=EPOLLIN;
		ev.data.fd=keyboard_fds[i];
		if(epoll_ctl(eFD,EPOLL_CTL_ADD,keyboard_fds[i],&ev)==-1){
			perror("epoll_ctl failed");
			exit_close(eFD,dir,keyboard_fds);
			return -1;
		}
	}
	struct epoll_event events[MAXEVENTS];

	while(1){
		int numFDs=epoll_wait(eFD,events,MAXEVENTS,-1);
		if(numFDs==-1){
			perror("epoll_wait failed");
			exit_close(eFD,dir,keyboard_fds);
			return -1;
		}

		//handle the events
		for(int i=0;i<numFDs;i++){
			if(events[i].events&EPOLLIN){
			    struct input_event ev;
			    ssize_t bytes=read(events[i].data.fd,&ev,sizeof(struct input_event));
			    if(bytes==-1){
					perror("read failed");
					continue;
			    }else if(bytes==0)
					continue;

				//printf("events: type=%d code=%d value=%d\n",ev.type,ev.code,ev.value);
				if(ev.type==EV_KEY&&ev.value==0x01){ //key event + key pressed (not released)
					char ascii_char[12];
					ev_code_to_ascii(ev.code,ev.value,ascii_char);
					printf("%s",ascii_char);
					fflush(stdout);
				}
			}	
		}
	}
	
	exit_close(eFD,dir,keyboard_fds);
	return 0;
}
