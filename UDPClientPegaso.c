/*
* Authors: Gianluca Lazzaro, Luca Pellegrino
* This code was developed to achieve the seafloor sensors automated sampling. 
* First of all the program set up an UDP Client, getting IP adress, port and directory in wich save files through configuration file. 
* Secondly builds up a chained list of sensors struct and performs the command passed as argument by the command line
* sending the right commands to the remote UDP Server. 
*
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "config_parser.h"
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "zlog.h"

#define canaliTot 8 
#define MAXROWDIM 40
#define UDPPKTSIZE 35640
#define directoryTest "./"

/*-------------------------------------------------------------------------*/
				/*Codici errori*/
/*
0 = Success
1 = socket errors
2 = file errors
3 = parameter errors (argv, IP, Porta)
*/
/*-------------------------------------------------------------------------*/
		/* STRUCT */
struct arrCh{
	sensorData **head;
	
};

/*-------------------------------------------------------------------------*/
		/* Prototipi funzioni */
void insertByteColonnaIntesta(sensorData **h, char *r, int bScritti);
void powerOff(sensorList *sL, int s);
void liberaMemoria(sensorList **s, char *nome);
sensors * cercaSensore(int canale,sensorList *sList);
void insert(duinoData **h, duinoData **t, char *r, int bScritti);
void stampaVideo(duinoData **h, duinoData **t, int s);
void salvaMisure(sensorList *sL, char* directory);
int rx(duinoData **h, duinoData **t, int s, int numPack, int test);
void getBlockAllChannels(duinoData **h, duinoData **t, int s, int numPack, sensorList * sL);
void getBlockSingleChannel(duinoData **h, duinoData **t, int s, int numPack, sensorList * sL);
void insertByteColonna(sensorData **h, char *r, int bScritti);
void setchannel(char *token, sensorList * sList, int s);
void getvalues(duinoData **h, duinoData **t, int s, int numPack, sensorList * sL, char* directory);
void cercaSensoreGetValues(int canale, char *misura,sensorList *sList);
void salvaAudio(duinoData **h, sensorList *sL, char* directory);
void getData(duinoData **h, duinoData **t, int s, int numPack, sensorList *sL, char* directory);
void powerOn(char *token, sensorList *sList, int s);
int checkAnswer(char* command,int sock);
/*-------------------------------------------------------------------------*/
		/* VARIABILI GLOBALI */
//sensorList * head_s = NULL;
int setch=0;// se 0 setch=all, se 1 setch=hydro, se 2 setch=singleChannel
zlog_category_t *c;
struct sockaddr_in soac;


/*-------------------------------------------------------------------------*/
//extern zlog_category_t *c;//per il logging


int main(int argc, char *argv[]){
	
	sensorList * sensorLst=NULL;
	config_params *params = NULL;
	duinoData *headPack=NULL, *tailPack=NULL;
	duinoData *duinoAnswersHead = NULL, *duinoAnswersTail = NULL;
	int s, numPack=1, arguments, answerStatus;
	char *hostname;
	short port;
	char *buffer=(char *)calloc(32,sizeof(char));
	char *token;
	struct sockaddr_storage t_serv;//mi serve per verificare la correttezza dell'ip

	int rc = zlog_init("zlog.conf");
	if (rc) {
		printf("init failed\n");
		return -1;
	}

	c = zlog_get_category("ACME");
	if (!c) {
		printf("failed to get category\n");
		zlog_fini();
		return -2;
	}

	
	if(argc < 1)
	{
		zlog_error(c, "Client executed with less than one parameter");
		exit(3);
	}

	zlog_info(c, "executing Client");

	memset(buffer, 0, 32);

	sensorLst=parse_ini_file("config V2.0.ini", &params);
	if(sensorLst == NULL){
		zlog_error(c, "can't get sensor list, exiting");
		exit(2);	
	}
	if(params == NULL){
		zlog_error(c, "can't get network parameters, exiting");
		exit(2);	
	}
	zlog_debug(c, "ini file parsed successfully");

	//verifica sulla correttezza dell'indirizzo ip
	if( (inet_pton(AF_INET, params->address, (struct sockaddr *) &t_serv) != 1)){
		zlog_error(c, "Bad ip address, exiting");
		exit(3);	
	}
	
		//verifica sul range delle porte

	if(params->port < 1024 || params->port > 65535){
		zlog_error(c, "Bad port number, exiting");
		exit(3);
	}
	soac.sin_family=AF_INET;
	soac.sin_port=htons(params->port);
	inet_pton(AF_INET,params->address, &(soac.sin_addr.s_addr));
	zlog_debug(c, "Creo socket");
	s=socket(PF_INET, SOCK_DGRAM,IPPROTO_UDP);
	if(s < 0){
		zlog_fatal(c,"socket error, -1 returned");
		exit(1);
		}
	else{// send command to UDP server

		for(arguments=1; arguments<argc;arguments++ ){
			/* read command from keyboard
			fgets(buffer,sizeof(buffer),stdin);
			scanf("%s", buffer);
			*/
			if(strcmp(argv[arguments],"?")==0){
				buffer="?";
				zlog_debug(c, "command to send: ? ");
				sendto(s,buffer,32, 0, (struct sockaddr *) &soac, sizeof(soac));
				//answerStatus = checkAnswer( &duinoAnswersHead, &duinoAnswersTail, buffer,s, &soac);
				stampaVideo(&duinoAnswersHead, &duinoAnswersTail, s);
		
			}else if(strncmp(argv[arguments],"setch=",6) == 0){
				buffer = "setmode=hs";
				char *buffer2=(char *)calloc(32,sizeof(char));
				memcpy(buffer2,argv[arguments], strlen(argv[arguments]));
				sendto(s,buffer,32, 0, (struct sockaddr *) &soac, sizeof(soac));

				token = strstr(argv[arguments],"setch=");
				stampaVideo(&duinoAnswersHead, &duinoAnswersTail, s);
				free(duinoAnswersHead->prec);
				free(duinoAnswersHead->next);
				duinoAnswersHead->byteScritti=0;
				duinoAnswersHead->prec=NULL;
				duinoAnswersHead->next=NULL;
				memset(duinoAnswersHead->pack,0,UDPPKTSIZE);

			
				printf("Richiesta setchannel\n");
				zlog_debug(c, "Richiesta setchannel");
				setchannel(token, sensorLst, s);			

				sendto(s,buffer2,32, 0, (struct sockaddr *) &soac, sizeof(soac));
				printf("setchannel invia: %s %s\n",buffer2, argv[arguments]);
				stampaVideo(&duinoAnswersHead, &duinoAnswersTail, s);
				free(duinoAnswersHead->prec);
				free(duinoAnswersHead->next);
				duinoAnswersHead->byteScritti=0;
				duinoAnswersHead->prec=NULL;
				duinoAnswersHead->next=NULL;
				memset(duinoAnswersHead->pack,0,UDPPKTSIZE);

			
//				if(strstr( argv[arguments], "hydro" ) != NULL)//se è hydro fa la fill se no start=block
					buffer = "start=fill";	
//				else	buffer = "start=block";
	
				printf("INVIO: %s\n", buffer);
				sendto(s,buffer,32, 0, (struct sockaddr *) &soac, sizeof(soac));
				stampaVideo(&duinoAnswersHead, &duinoAnswersTail, s);
				free(duinoAnswersHead->prec);
				free(duinoAnswersHead->next);
				duinoAnswersHead->byteScritti=0;
				duinoAnswersHead->prec=NULL;
				duinoAnswersHead->next=NULL;
				memset(duinoAnswersHead->pack,0,1392);

						 }
				else if(strcmp(argv[arguments],"getvalues")==0){
					//printf("Richiesta getvalues\n");
					zlog_debug(c, "Richiesta getvalues");
					powerOn("all", sensorLst, s);
					sendto(s,"setch=all",32, 0, (struct sockaddr *) &soac, sizeof(soac));
					sendto(s,"getvalues",32, 0, (struct sockaddr *) &soac, sizeof(soac));
					//stampaVideo(&headPack, &tailPack, s);
					getvalues(&headPack, &tailPack, s, 1, sensorLst, params->directory);
						 }
				else if(strcmp(argv[arguments],"getblock")==0){
					
					zlog_debug(c, "Richiesta getblock");
					sendto(s,"getblock",32, 0, (struct sockaddr *) &soac, sizeof(soac));
					if(setch==0){
						printf("Richiesta getblockAllChannels\n");
						getBlockAllChannels(&headPack, &tailPack, s, numPack, sensorLst);
						}
					if(setch==2){
						printf("Richiesta getblockSingleChannel\n");
						getBlockSingleChannel(&headPack, &tailPack, s, numPack, sensorLst);
						}
					}

					else if(strcmp(argv[arguments],"getdata")==0){
						/*codice aggiunto da Gian*/
						//powerOn("all", sensorLst, s);
						/*---------------------*/
						printf("Richiesta getdata\n");
						zlog_debug(c, "Richiesta getdata");
						//sendto(s,"getdata",32, 0, (struct sockaddr *) &soac, sizeof(soac));
						//stampaVideo(&headPack, &tailPack, s);
						getData(&headPack, &tailPack, s, 3014, sensorLst, params->directory);
					 }
					 else if(argv[arguments] != NULL ){
							sendto(s,argv[arguments],32, 0, (struct sockaddr *) &soac, sizeof(soac));
							stampaVideo(&headPack, &tailPack, s);
					 }
					

	 			   }//for
			}//else


zlog_fini();
return 0;
} //



void getData(duinoData **h, duinoData **t, int s, int numPack, sensorList *sL, char* directory){
	sendto(s,"getdata",8, 0, (struct sockaddr *) &soac, sizeof(soac));

	int index = 0, retry=3;
	
	while(index < numPack){
		if( (rx( h, t, s, 1, 0) > 0)  && (retry>0) ) { /* se è andato in timeout rx ritorna un errore, quindi ripeto la misura per 3 volte al massimo*/
		sendto(s,"getdata",8, 0, (struct sockaddr *) &soac, sizeof(soac));
		retry--;
		continue;
			}
		/*se è uscito perchè non raggiunge netDuino, termino*/
		if(retry=0) {
			zlog_fatal(c, "nedDuino irraggiungibile, termino");
			exit(1);
		}
		retry=3;
		sendto(s,"getdata",8, 0, (struct sockaddr *) &soac, sizeof(soac));
		index++;
		//printf("Ricevuti %d pacchetti\n",index);
	}
	//if(setch==1){//caso hydro chiamo salvaAudio che poi a sua volta esegue il codec flac
		salvaAudio(h, sL, directory);				
	//		}


}

void salvaAudio(duinoData **h, sensorList *sL, char* directory){
	FILE *file;//file su cui scrivero'
	char *str = (char*)malloc(sizeof(char)*300);
	//char *date = (char*)malloc(sizeof(char)*25); // questa stringa contiene la data
	char ufdate[80];
	if(str==NULL){//se ritorna null 
		printf("Errore malloc, exit\n");
		zlog_fatal(c, "Malloc null");
		exit(1);	
			}
	strcpy(str,directory);
	strcat(str,"Audio/");

	time_t rawtime;
   	time (&rawtime);
   	struct tm* data;
	
	data = localtime(&rawtime);
	strftime(ufdate, sizeof(ufdate), "%Y%m%d_%H:%M:%S", data);
	char *date = (char*)malloc(strlen(ufdate)); // questa stringa contiene la data
	strcpy(date,ufdate);
	sensorList *tempSL = sL;
	char headerSensore[200];

	strcat(str, date);
	//strcat(str, ".txt");
	printf("Scrivo nel file: %s\n", str);
	file = fopen(str, "a");
	zlog_debug(c, "salvaAudio: salvo nel file");
	duinoData *tempData = *h;
	int byteScrit=0;
	int idx=3014;
//	while(strcmp(tempData->pack, "OK RAM EMPTY") != 0 ){
	while(idx>0){
		idx--;
		byteScrit+=tempData->byteScritti;
//		printf("SalvaAudio: byte scritti %d",byteScrit);
		fwrite(tempData->pack, sizeof(char), tempData->byteScritti, file);
		tempData=tempData->next;	
	}
	fflush(file);
	fclose(file);
	zlog_debug(c, "salvaAudio: file scritto");
//	zlog_debug(c, "salvaAudio: inizio conversione FLAC");
	
	/*char command[50];
	int asd=0;
   	strcpy( command, "ls -l" );
	asd=system(command);
	if(asd != -1)	
		zlog_debug(c, "salvaAudio: conversione FLAC terminata con successo");
	else if(asd == -1)   zlog_fatal(c, "salvaAudio: conversione FLAC fallita");*/
	
	
}


void getvalues(duinoData **h, duinoData **t, int s, int numPack, sensorList * sL, char* directory){
	char * stringCh;
	int i=3, rxreturn=0;
	/* prendo 3 misurazioni poichè le prime sono nulle */
	while(i != 0){
		rx(h, t, s, numPack, 1);
		sendto(s,"getvalues",strlen("getvalues"), 0, (struct sockaddr *) &soac, sizeof(soac));
		i--;
	}
	if(rxreturn = rx(h, t, s, numPack, 0) >0){
		zlog_fatal(c, "network error during data receiving");
		exit(rxreturn);
	}
	duinoData *temp = (*h);
	char c[1392], *ch,*uguale,canale[2],misura[10];
	memset(c,0,sizeof(c));
	memcpy(c,temp->pack,strlen(temp->pack));
	//printf("getvalues: %s\n",c);	
	char *token = strtok(c, ",");
	while(token != NULL){
		//printf("stringCh:%s\n",token);
				
		ch = strchr(token, 'h');
		if (ch == NULL)
	    	    ;//printf ("'=' was not found in the src\n");
		else
		    {	uguale=strchr(token, '=');
			//printf ("found '=' in the src: %s\n", uguale);
			strncpy(canale, ch+1, 1);	
			strcpy(misura, uguale+1);				
			/* printf PORTANTE!! */
			//printf ("Ch:%s misura:%s sizeof:%d\n", canale,misura,strlen(misura));
			//adesso cerco il canale
			cercaSensoreGetValues(atoi(canale),misura,sL);
									
			}
		
		ch=NULL;
		token=strtok(NULL, ",");
		
		}
	//printf("sonon uscito\n");
	powerOff(sL, s);	
	salvaMisure(sL, directory);
	//exit(0);
}


void cercaSensoreGetValues(int canale, char *misura,sensorList *sList){
	sensorList * sl=sList;
	//printf("Cerca sensore che usa questo canale:%d...",canale);
	while(sl!=NULL){		
		chanList *chL=sl->sensore->chHead;
		while(chL != NULL){
//			printf("Sensore: %s Controllo se %d=%d \n",sl->sensore->sensorName,chL->val,canale);
			if(chL->val==canale){
//				printf("canale %d trovato\n",chL->val);
				sl->sensore->sData=(sensorData **)calloc(1, sizeof(sensorData *));
				int i=0, bTmpScritti=0;	
				int aa=(strlen(misura)/3);
				if(aa==0 || strlen(misura)%3>0){aa++;}
				int caz=0;				
				for(;i<strlen(misura);){
					caz++;
					bTmpScritti=3;
					if(caz==aa){
						if(strlen(misura)%3>0)
							bTmpScritti=strlen(misura)%3;
						   }	
//					printf("Sizeof:%d inserisco %d bytes",strlen(misura),bTmpScritti);
					insertByteColonna(sl->sensore->sData, &(misura[i]), bTmpScritti);
					i+=bTmpScritti;				
				}
				return ;
					}
			chL=chL->next;
			
				}
		sl=sl->next;

	} 
	
}




int calcolaNumCanaliAttivi(chanList *listCh){
	chanList *lCh=listCh;
	int n=0;
	while( lCh!=NULL ){
		n++;
		lCh=lCh->next;
		}
	return n;
}

/*funzione che restituisce i dati raw. Restituisce eattamente 1 blocco (1392 byte). Nel caso di una lettura di start=fill l'ultimo pkt udp è una string contenente "memempty" o qlcosa del genere*/
void getBlockAllChannels(duinoData **h, duinoData **t, int s, int numPack, sensorList * sL){
	rx(h, t, s, numPack, 0);
/* Prendo i canali di ogni sensore e metto i 3byte di ogni canale in una sensorData(lista 2concatenata). Presuppongo che i dati mi arrivano dal MSB al LSB. 
Se ho più canali per sensore presuppongo che continui ad essere valida MSB-->LSB(dal canale + piccolo al più grande!!).

*/	
	int numChActive=0, ncanali=0;
	
	sensorList *tempS = sL;
	printf("GetblockAllChannels, %d\n",tempS->NchanActive);
	struct arrCh *array=(struct arrCh*)calloc(tempS->NchanActive+1, sizeof(struct arrCh));//è inizializzato al numero di canali usati
	
	if(array==NULL){
		printf("calloc array=null\n");	
		zlog_fatal(c, "Calloc null");
		exit(2);
	}
	chanList *chLst = sL->availableChannels;
	//n=calcolaNumCanaliAttivi(sL->availableChannels);
	zlog_debug(c, "Getblock: riempio l'array con i puntatori");
	while(tempS != NULL){//per quanti sensori ho ciclo
		//printf("Getblock\n");	
		if(tempS->sensore->status==0){//se il sensore è spento non riceverò misure quindi salto
			tempS = tempS->next;			
			continue;		
		}
		tempS->sensore->sData=(sensorData **)calloc(58, sizeof(sensorData *));//righe vettore
		if(tempS->sensore->sData==NULL){
			zlog_fatal(c, "Calloc null");
			exit(2);
	}		

		
		chanList *chLtmp=tempS->sensore->chHead;
		while(chLtmp != NULL){
			//printf("GetblockCH ch: %d\n",chLtmp->val);
			array[chLtmp->val].head=tempS->sensore->sData;/*metto il puntatore all'elemento array[0][0] d i ogni sensore*/
			//printf("%s array[%d].head=        %d\n", tempS->sensore->sensorName, chLtmp->val, array[chLtmp->val].head);
			//printf("tempS->sensore->sData=%d\n", tempS->sensore->sData);	
			numChActive++;

			chLtmp=chLtmp->next;
				
		}
		tempS = tempS->next;
	}
	//printf("DuinoData:%s",(*h)->pack);
	//printf("Numero canali attivi %d\n",numChActive);
	//chanList *tempChList = sL->availableChannels;
	duinoData *temp = (*h);
	zlog_debug(c, "Getblock: array riempito adesso metto tutti nei byte ai relativi sensori");
	//int numChInseriti=0;
	//printf("DuinoData:%s",temp->pack);
	while(temp != NULL){	// scorro tutti i pack ricevuti da netduino
				//ed inserisco nella lista di ciascun sensore i dati ricevuti da netduino
		int numChInseriti=canaliTot;
		chanList *tempChList = sL->availableChannels;
		int dimPack = temp->byteScritti;	
		int nMis=0;//numero misurazione[1...58]
		int i=0, bTmpScritti=0;	
		int aa=(temp->byteScritti/3);//numero "pacchetti" da inviare, num di cicli for sarebbe
			
		if(aa==0 || temp->byteScritti%3>0){aa++;}
		int caz=0;
		//printf("DimPack= %d \t aa=%d\n", dimPack, aa);
		
		while(i < dimPack){	
						

			//printf("Passo questo indirizzo %d\n",&(array[tempChList->val].head[nMis]));
			//insertByteColonna(array[indiceCH].head[nMis], &(temp->pack[i]), 3);


			caz++;
			bTmpScritti=3;
			if(caz==aa){
				if(dimPack%3>0)
					{
		
					bTmpScritti=dimPack%3;
					//printf("ultimo pack, scriverò %d",bTmpScritti);						

						}
				
				}
			
			
			if(numChInseriti==tempChList->val){
			//	printf("Procedo a scrivere i %d byte del canale %d...", bTmpScritti,tempChList->val);
				insertByteColonna(&(array[tempChList->val].head[nMis]), &(temp->pack[i]), bTmpScritti);
				tempChList=tempChList->next;
				i+=bTmpScritti;
    			}			

			//insertByteColonna(&(array[tempChList->val].head[nMis]), &(temp->pack[i]), bTmpScritti);
			
			//printf("Byte scritti fin'ora: %d\n",i);			
			numChInseriti--;

			if(numChInseriti <= 0){

				tempChList = sL->availableChannels;
				numChInseriti=canaliTot;
				continue;
				}
			//printf("gianluca %d, bytetot: %d\n", tempChList->next->val, temp->byteScritti);
			
			//tempChList=tempChList->next;
							}
		temp= temp->next;

			}

		//procedo a salvare nel file
		powerOff(sL,s);
		salvaMisure(sL, directoryTest);
		
	
}


void getBlockSingleChannel(duinoData **h, duinoData **t, int s, int numPack, sensorList * sL){
	
	int i=3;
	while(i != 0){
		rx(h, t, s, numPack, 1);
		sendto(s,"getblock",32, 0, (struct sockaddr *) &soac, sizeof(soac));
		i--;
	}


	rx(h, t, s, numPack, 0);
/* Prendo i canali di ogni sensore e metto i 3byte di ogni canale in una sensorData(lista 2concatenata). Presuppongo che i dati mi arrivano dal MSB al LSB. 
Se ho più canali per sensore presuppongo che continui ad essere valida MSB-->LSB(dal canale + piccolo al più grande!!).

*/	
	int numChActive=0;
	sensorList *tempS = sL;
	//printf("Getblock, %d\n",tempS->NchanActive);
	struct arrCh *array=(struct arrCh*)calloc(tempS->NchanActive+1, sizeof(struct arrCh));//è inizializzato al numero di canali usati
	//struct arrCh *array=(struct arrCh*)calloc(11, sizeof(struct arrCh));//è inizializzato al numero di canali usati
	if(array==NULL){
		printf("calloc array=null\n");	
		zlog_fatal(c, "Calloc null");
		exit(2);
	}
	chanList *chLst = sL->availableChannels;
	
		
	//printf("array[0]: %d", **(array[0].head));	
	zlog_debug(c, "Getblock: riempio l'array con i puntatori");
	
		//printf("Getblock\n");	
		if(tempS->sensore->status==0){//se il sensore è spento non riceverò misure quindi salto
			tempS = tempS->next;			
			//continue;	
			//QUI NON SI DEVE FARE CONTINUE MA ANDARE A PUTTANE	
		}	
		
			tempS->sensore->sData=(sensorData **)calloc(58*8, sizeof(sensorData *));//righe vettore 464
			if(tempS->sensore->sData==NULL){
				zlog_fatal(c, "Calloc null");
				exit(2);
					}		

		
			chanList *chLtmp=tempS->sensore->chHead;
			while(chLtmp != NULL){
				//printf("GetblockCH ch: %d\n",chLtmp->val);
				array[chLtmp->val].head=tempS->sensore->sData;/*metto il puntatore all'elemento array[0][0] d i ogni sensore*/
				//printf("%s array[%d].head=        %d\n", tempS->sensore->sensorName, chLtmp->val, array[chLtmp->val].head);
				//printf("tempS->sensore->sData=%d\n", tempS->sensore->sData);	
				numChActive++;

				chLtmp=chLtmp->next;
				//printf("Getblock CH dopo di next\n");			
					}
			
		
	
	//printf("DuinoData:%s",(*h)->pack);
	printf("Numero canali attivi %d\n",numChActive);
	//chanList *tempChList = sL->availableChannels;
	duinoData *temp = (*h);
	zlog_debug(c, "Getblock: array riempito adesso metto tutti nei byte ai relativi sensori");
	//int numChInseriti=0;
	//printf("DuinoData:%s",temp->pack);
	while(temp != NULL){	// scorro tutti i pack ricevuti da netduino
				//ed inserisco nella lista di ciascun sensore i dati ricevuti da netduino
		int numChInseriti=0;/* BISOGNA CAPIRE SE I DATI SONO SEMPRE MESSI NELLO STESSO MODO NEL CASO DI PIÙ PACCHETTI*/
		chanList *tempChList = sL->availableChannels;
		int dimPack = temp->byteScritti;	
		int nMis=0;//numero misurazione[1...58]
		int i=0, bTmpScritti=0;	
		int aa=(temp->byteScritti/3);//numero "pacchetti" da inviare, num di cicli for sarebbe
		//printf("DimPack= %d \t aa=%d\n", dimPack, aa);		
		if(aa==0 || temp->byteScritti%3>0){aa++;}
		int caz=0;
		//printf("DimPack= %d \t aa=%d\n", dimPack, aa);
		
		while(i < dimPack){	
						

			//printf("Passo questo indirizzo %d\n",&(array[tempChList->val].head[nMis]));
			//insertByteColonna(array[indiceCH].head[nMis], &(temp->pack[i]), 3);

//			i=i+3
			caz++;
			bTmpScritti=3;
			if(caz==aa){
				if(dimPack%3>0)
					{
		
					bTmpScritti=dimPack%3;
					//printf("ultimo pack, scriverò %d",bTmpScritti);						

						}
				
				}
			
			//printf("Procedo a scrivere i %d byte del canale %d...", bTmpScritti,tempChList->val);
			//insertByteColonna(&(array[tempChList->val].head[nMis]), &(temp->pack[i]), bTmpScritti);
			insertByteColonnaIntesta(&(array[tempChList->val].head[nMis]), &(temp->pack[i]), bTmpScritti);			
			nMis++;
			i+=bTmpScritti;
			//printf("Byte scritti fin'ora: %d\n",i);			
			numChInseriti++;
			
			
			if(numChInseriti >= numChActive){
			//	printf("prima passata di canali fatta, riparto dalla testa della lista di chan\n");
				tempChList = sL->availableChannels;
				numChInseriti=0;
				continue;
				}
			//printf("gianluca %d, bytetot: %d\n", tempChList->next->val, temp->byteScritti);
			
			tempChList=tempChList->next;
							}
		//printf("Uscito dal for: %d ",temp->byteScritti);

		temp= temp->next;

			}
		
		powerOff(sL,s);
		//procedo a salvare nel file
		salvaMisure(sL, directoryTest);
		
	
}

void insertByteColonnaIntesta(sensorData **h, char *r, int bScritti){
	//printf("InsertByteColonna\n");
	char* buf_str = (char*) malloc (2*bScritti + 1);
	char* buf_ptr = buf_str;

	sensorData *nodo;
	nodo = (sensorData *)malloc(sizeof(sensorData));
	
	if(nodo==NULL){//se ritorna null 
		printf("Errore malloc, exit\n");
		zlog_fatal(c, "Malloc null");		
		exit(1);	
			}
	//printf("Sto per inserire %s\n", r);
	nodo->byteScritti = bScritti;
	memset(nodo->pack,0,3);
	memcpy(nodo->pack, r, bScritti);
	int pirasize=0,pa=0;
	printf("Ho inserito %s\n", nodo->pack);
//	printf("Insert: ho messo questo: %s mentre ho ricevuto: ",nodo->pack);
/*	for(;pirasize<bScritti;pirasize++){
//		printf("pirasize: %d ho scritto questo char: %c\n",pirasize,r[pirasize]);
		printf("%c=%c " ,nodo->pack[pirasize],r[pirasize]);
		}*/

	
	int i;
	for (i = 0; i < bScritti; i++)
	{
	    buf_ptr += sprintf(buf_ptr, "%02X", r[i]);
	}
	sprintf(buf_ptr,"\n");
	*(buf_ptr + 1) = '\0';
	//printf("Convertito:%d\n", atoi(buf_str));

	//free(buf_str);
	buf_str = NULL;
	//printf("Finito di convertire\n");
	nodo->next = *h;
	*h = nodo;
				
	
}

void insertByteColonna(sensorData **h, char *r, int bScritti){
	//printf("InsertByteColonna\n");
	sensorData *nodo, *temp;
	nodo = (sensorData *)malloc(sizeof(sensorData));
	
	if(nodo==NULL){//se ritorna null 
		printf("Errore malloc, exit\n");
		zlog_fatal(c, "Malloc null");		
		exit(1);	
			}
		
		
	nodo->next = NULL;
	//nodo->prec = (*t);
	nodo->byteScritti = bScritti;
	memset(nodo->pack,0,3);
	memcpy(nodo->pack, r, bScritti);
	int pirasize=0,pa=0;

//	printf("Insert: ho messo questo: %s mentre ho ricevuto: ",nodo->pack);
/*	for(;pirasize<bScritti;pirasize++){
//		printf("pirasize: %d ho scritto questo char: %c\n",pirasize,r[pirasize]);
		printf("%c=%c " ,nodo->pack[pirasize],r[pirasize]);
		}
	printf("\n");*/
	if(*h == NULL){//creazione lista
		*h = nodo;
		//printf("Sensordata h==null, return\n");
		return;	
	}
		
	temp = *h;
	while(temp->next != NULL){
		//printf("InsertByteColonna: cerco il punto di inserimento\n");
		temp=temp->next;}
	temp->next=nodo;
				
	
}



void stampaVideo(duinoData **h, duinoData **t, int s){//stampa a video le informazioni ricevuto dal socket
	if( rx(h, t, s, 1, 0)>0 ){
			zlog_fatal(c, "tentativi timeout rx terminati, netDuino non raggiungibile, termino");
			exit(1);
		}
	printf("Risposta di netDuino:  %s\n", (*t)->pack);
		

}


void powerOff(sensorList *sL, int s){
	sensorList *tempSL = sL;
	char * buffer = (char*)calloc(32,sizeof(char));
	duinoData *RispostaHead = NULL, * RispostaTail = NULL;
	while(tempSL != NULL){

		sensors *tempSensor=tempSL->sensore;
			
		if(tempSensor->status==0){//se il sensore è spento salto
			//printf("Sensore:%s OFF\n",tempSensor->sensorName);
			tempSL= tempSL->next;
			continue;			
		}
		
		chanList *chL=tempSensor->chHead;
		while(chL != NULL){//inserisco i canali
			duinoData *RispostaHead = NULL, * RispostaTail = NULL;
			char canale[2];
			memset(canale,0,2);
			strcpy(buffer, "pwr_off=");
			sprintf(canale, "%d", (chL->val) );
			strcat(buffer, canale);
			//printf("invio powerOff: %s\n",buffer);
			sendto(s,buffer,32, 0, (struct sockaddr *) &soac, sizeof(soac));
			//stampaVideo(&RispostaHead, &RispostaTail,s);
			
			/*free(RispostaHead->prec);
			free(RispostaHead->next);
			RispostaHead->byteScritti=0;
			RispostaHead->prec=NULL;
			RispostaHead->next=NULL;
			memset(RispostaHead->pack,0,1392);
			
			free(RispostaTail->prec);
			free(RispostaTail->next);
			RispostaTail->byteScritti=0;
			RispostaTail->prec=NULL;
			RispostaTail->next=NULL;
			memset(RispostaTail->pack,0,1392);*/

			memset(buffer,0,sizeof(buffer));
			chL=chL->next;
			}


		tempSL = tempSL->next;	
		}
}

void salvaMisure(sensorList *sL, char* directory){
//salvo la misura di 1 sensore in
//un nuovo file. Il nome con cui le salvo ancora non l'ho deciso

	FILE *file;//file su cui scrivero'
	char *str = (char*)malloc(sizeof(char)*MAXROWDIM);
	char *separator = ";"; //separatore fra un valore e l'altro
	char ufdate[30];
	char Sensordata[10];
	short int newFile =0;
	if(str==NULL){//se ritorna null
		zlog_fatal(c, "not enough memory");
		exit(1);	
	/* directory used for testing */		}
	strcpy(str, directory);
	//strcpy(str,directory);
	//strcat(str, "Sample");
	time_t rawtime;
   	time (&rawtime);
   	struct tm* data;
	
	data = localtime(&rawtime);
	//AnnoMeseGiornoOra.Minuto.Secondo
	strftime(ufdate, sizeof(ufdate), "%Y%m%d", data);	//"%Y%m%d %H:%M:%S-%Z"
	char *date = (char*)malloc(strlen(ufdate)); // questa stringa contiene la data
	strcpy(date, ufdate);
	strcat(str, date);
	//azzero il contenuto della stringa ufdate
	bzero(ufdate, strlen(ufdate));

	strftime(ufdate, sizeof(ufdate), "%H:%M:%S", data);
	
	printf("Scrivo nel file: %s\n", str);
	if( access( str, F_OK ) == -1){ //if file not exists try to create a new file

		if(  ( (file = fopen(str, "w") ) == NULL ) || ( access(str, W_OK) == -1 ) ){
			zlog_fatal(c, "wrong file path or write permission restriction, data lost!");;
			exit(2);
		}
	newFile=1;
	}else{ //if file exists write it in append mode
		if(  ( ( access(str, W_OK) == -1 ) || (file = fopen(str, "a") ) == NULL ) ){
			zlog_fatal(c, "write permission restriction, data lost!");
			exit(2);
		}
	}

	sensorList *tempSL = sL;
	memset(str, 0, MAXROWDIM);
	char *headerSensore;
	zlog_debug(c, "saving samples to file");
	int bscritti=0;
	//comincio la riga con il timestamp
	//fwrite(ufdate, sizeof(char), strlen(ufdate), file);//header file		
	//memcpy(headerSensore, ufdate, strlen(ufdate));
	memcpy(str, ufdate, strlen(ufdate));
	if(newFile == 1){ //if samples has to be written in a new file, insert header too
		headerSensore = (char*)calloc(MAXROWDIM, sizeof(char));
		memcpy(headerSensore, "Timestamp", strlen("Timestamp"));
	}

	while(tempSL != NULL){
//	printf("Entro nel while\n");
		sensors *tempSensor=tempSL->sensore;
		
		if(tempSensor->status==0){//se il sensore è spento salto
			//printf("Sensore:%s OFF\n",tempSensor->sensorName);
			tempSL= tempSL->next;
			continue;			
		}
		//printf("Sensore:%s \n",tempSensor->sensorName);
		strcat( str,separator);
		//metto il nome del sensore
		if(newFile == 1){
			strcat(headerSensore, separator);
			strcat( headerSensore, tempSensor->sensorName);
		}
		//strcat(str, separator);
		//fwrite(str, sizeof(char), strlen(str), file);//write sensor name to file		

		
		int k=0, i=0;
		sensorData **tempSdata=tempSensor->sData;
		
		while(tempSdata[k] != NULL){

			sensorData *tmpData=tempSdata[k];
			while(tmpData != NULL){ //write data bytes to file
				bscritti+=tmpData->byteScritti;
				snprintf(Sensordata, (sizeof(char)*tmpData->byteScritti)+1, "%s", tmpData->pack);
				strcat( str, Sensordata );	
				//fwrite(tmpData->pack, sizeof(char), tmpData->byteScritti, file);
				tmpData=tmpData->next;
			}//chiude il while sulla lista di dati

			k++;				
		} //chiude il while sulla matrice di liste
		tempSL= tempSL->next;
		//memset(str,0,MAXROWDIM);
		
			}//chiude il while sulla lista di sensori
	if(newFile == 1){
		fwrite(headerSensore, sizeof(char), strlen(headerSensore), file);
		fwrite("\n", sizeof(char), strlen("\n"), file);		
	}
	fwrite(str, sizeof(char), strlen(str), file);
	fwrite("\n", sizeof(char), strlen("\n"), file);

	if( fflush(file) !=0 ) 
		zlog_fatal(c, "fflush error");
	fclose(file);
	zlog_debug(c, "sample saved successfully");
}

/*
Questa funzione setta i canali richiesti per l'acquisizione
@param token: rappresenta la stringa del comando, da cui sarà effettuata la split
@param sList: è la struttura che include i sensori disponibili nel file di config

*/
void powerOn(char *token, sensorList *sList, int s) {

	zlog_debug(c, "powering on sensors needed");

		sensorList *listaSensori = sList;
		char *buffer = (char*)calloc(14,sizeof(char));
		chanList *channels;
		char * stringCh = (char*)calloc(4,sizeof(char));
		//char *splitString = strtok(token, "=");
		char *splitString;	

		chanList *temp;	
		duinoData *duinoRispostaHead = NULL, * duinoRispostaTail = NULL;

		if ( strcmp(token, "all") == 0 ){
					strcpy(buffer, "pwr_on=all");
					sendto(s,buffer,32, 0, (struct sockaddr *) &soac, sizeof(soac));
					if( checkAnswer(buffer,s) > 0 ){
						printf("can't power on seafloor sensors, exiting\n");
						zlog_fatal(c, "can't power on seafloor sensors, exiting" );
						exit(4);
					}
		}
		else{
		while(listaSensori != NULL){
			if(listaSensori->sensore->status == 1){
				temp = listaSensori->sensore->chHead;
				while(temp != NULL){
					channels = (chanList* )malloc(sizeof(chanList));
					memcpy(channels, temp, sizeof(temp));
					insertChannel( channels, &(sList->availableChannels) );
					strcpy(buffer, "pwr_on=");
					sprintf(stringCh, "%d", (channels->val) );
					strcat(buffer, stringCh);
					sendto(s,buffer,32, 0, (struct sockaddr *) &soac, sizeof(soac));
					stampaVideo(&duinoRispostaHead, &duinoRispostaTail,s);
									
					zlog_debug(c, "Inserito canale nella lista" );
					//printf("Inserito canale %d nella lista\n", channels->val);
					temp = temp->next;
						}
							}// chiusura if status
		
			listaSensori = listaSensori->next;
		}
	}//chiusura else

}



/*
Questa funzione setta i canali richiesti per l'acquisizione
@param token: rappresenta la stringa del comando, da cui sarà effettuata la split
@param sList: è la struttura che include i sensori disponibili nel file di config

*/
void setchannel(char *token, sensorList *sList, int s){

	zlog_info(c, "setchannel attivata");
	char *string;
	char * buffer = (char*)calloc(14,sizeof(char));
	char * stringCh = (char*)calloc(4,sizeof(char));
	char *splitString = strtok(token, "=");
	chanList *temp;	
	duinoData *duinoRispostaHead = NULL, * duinoRispostaTail = NULL;

        string = strtok(NULL, "=");
	
	/* se il parametro passato è "all" scorro la lista sensori 
	   ed inserisco ciascun canale di ogni sensore attivo nella lista
	   di canali Attivi.
	*/

	if ( strcmp(string, "all") == 0 ){
		zlog_debug(c, "richiesta acquisizione di tutti i canali attivi");

		sensorList *listaSensori = sList;
		chanList *channels;
		while(listaSensori != NULL){
			if(listaSensori->sensore->status == 1){
				temp = listaSensori->sensore->chHead;
				while(temp != NULL){
					channels = (chanList* )malloc(sizeof(chanList));
					memcpy(channels, temp, sizeof(temp));
					insertChannel( channels, &(sList->availableChannels) );
					strcpy(buffer, "pwr_on=");
					sprintf(stringCh, "%d", (channels->val) );
					strcat(buffer, stringCh);
					sendto(s,buffer,32, 0, (struct sockaddr *) &soac, sizeof(soac));
					stampaVideo(&duinoRispostaHead, &duinoRispostaTail,s);
									
					zlog_debug(c, "Inserito canale nella lista" );
					//printf("Inserito canale %d nella lista\n", channels->val);
					temp = temp->next;
						}
							}// chiusura if status
		
			listaSensori = listaSensori->next;
	}//chiusura while lista sensori
	}//chiusura if

	else if(strstr( string, "hydro" ) != NULL){
		
		printf("HYDRO\n");
		setch=1;
		//sendto(s,string,strlen(string), 0, (struct sockaddr *) &soac, sizeof(soac));
		//stampaVideo(&duinoRispostaHead, &duinoRispostaTail,s);

		}	
		else {//caso di setch=x
				setch=2;
				printf("Setch=%s\n", string);
				sensors * sltmp=cercaSensore(atoi(string),sList);
				
				sList->sensore=NULL;
				sList->sensore=sltmp;
				sList->NchanActive=0;
				
				
				sList->next=NULL;
				chanList *canale2 = (chanList *)malloc(sizeof(chanList));
 				canale2->val = atoi(string);
				canale2->next = NULL;
				insertChannel( canale2, &(sList->availableChannels) );
				zlog_debug(c, "Inserito canale nella lista");
				printf("Inserito canale %d nella lista", canale2->val);
				strcpy(buffer, "pwr_on=");
				sprintf(stringCh, "%d", canale2->val);
				strcat(buffer, stringCh);
				sendto(s,buffer,32, 0, (struct sockaddr *) &soac, sizeof(soac));
				stampaVideo(&duinoRispostaHead, &duinoRispostaTail,s);

				
		}
	
	
//	sList->NchanActive = sList->availableChannels->val;
	//printf("maxChannel: %d\n", sList->NchanActive);
	sensorList *fanculo=sList;
	/*while(fanculo!= NULL){
		printf("Sensore: %s canali: ",fanculo->sensore->sensorName);
		chanList *chL=fanculo->sensore->chHead;
		while(chL != NULL){//inserisco i canali
			printf("%d ",chL->val);
			chL=chL->next;
			}
		printf("\n");
		fanculo=fanculo->next;
	}*/
	return;

}


//dato un canale ritorna il sensore e toglie i canali
sensors * cercaSensore(int canale,sensorList *sList){
	sensorList * sl=sList;
	int trovato=0;
	chanList *canaleTrovato=NULL;//è il puntatore al canale trovato, così ritorno un sensore con solo quel canale
	printf("Cerca sensore che usa questo canale:%d\n",canale);
	while(sl!=NULL){
		
		chanList *chL=sl->sensore->chHead;
		while(chL != NULL){//inserisco i canali
			if(chL->val==canale){
				trovato=1;
				canaleTrovato=chL;
				canaleTrovato->next=NULL;
				chL=chL->next;			
				//break;			
				}
			else {	chanList *prec=chL;
				chL=chL->next;
				free(prec);}
			
			}
		if(trovato==1){
			sl->sensore->chHead=canaleTrovato;
			return (sl->sensore);			
		}
		sl=sl->next;

} 
	return NULL;
}


/*-------------------------------------------------------------------------*/
/**
Data una sensorList, effettua la free su tutte le strutture di tutti i sensori ad eccezione di quello chiamato "nome"
 @param		**s	doppio puntatore a sensorList da modificare
 @param 	nome	nome del sensore da non eliminare
 @return		
**/
void liberaMemoria(sensorList **s, char *nome){
	sensorList *stmp=*s;//stmp è la lista temporanea
	sensorList *trovato=NULL;
	while(stmp!=NULL){
		if(strcmp(stmp->sensore->sensorName,nome)==0){
			trovato=stmp;			
			continue;			
		}
		
		}




}



/*-------------------------------------------------------------------------*/
/**
 @param		s	e' il socket
 @param 	numPack	n° pacchetti da ricevere e quindi ritornare
 @return	
**/
int rx(duinoData **h, duinoData **t, int s, int numPack, int test){
//attende di ricevere numPack pack da 1392 B
//inoltre attende per "attesa" secondi nella select e lo fa per 3 tentativi.
//praticamente attende la ricezione di un pacchetto per 3*attesa secondi.
	
	int attesa=10, tentativi=3, stop=1, n=0, k=1, dimBRx=0;
	char rxBff[1392];
	struct timeval time;
//	time.tv_sec=attesa;
//	time.tv_usec=0;
		

//	printf("Attendo %d pacchetti dal server\n", numPack);
		while(stop){
			
			fd_set fd_read;
			FD_ZERO(&fd_read);
			FD_SET(s,&fd_read);
			time.tv_sec=attesa;
			time.tv_usec=0;
			
			if((n=select(s+1,&fd_read,NULL,NULL,&time))==-1){
				printf("Errore select, esco\n");
				zlog_fatal(c, "Select can't get I/O file descriptors!");
				exit(2);			     }
			if(n==0){
				tentativi--;
				zlog_debug(c, "timeout, retrying...");			
				if(tentativi>2)
					continue;
				else if(tentativi==0){
					printf("Tentativi select terminati\n");
					zlog_error(c, "UDP Server unreacheable");
					break;
					}
				
				}
	
			else {
		
				int l=sizeof(soac);
				memset(rxBff,0,1392);
				int o=recvfrom(s,rxBff,1392,0,(struct sockaddr *)&soac, &l);
				dimBRx+=o;
//				printf("%d Ho ricevuto %d bytes\n", k, o);
					
			/*	int asd=0;
				for(;asd<o;asd++){
					printf("%c", rxBff[asd]);}*/
				if(test == 0){
//				printf("inserisco dato\n");
				insert(h, t, rxBff, o);
				}
				//printf("insertDuinoData:%s",(*h)->pack);
				numPack--;				
				if(numPack>0){//finchè non ho ricevuto n pacchetti itero
					continue;						
					}
				else{//esco dal ciclo	
					//zlog_debug(c, "Esco dalla rx");
					stop=0;}

					     }//chiude l'else
			}//chiude il while
	if(tentativi==0) return 3;
	return 0;

}

/**
	Esegue l'inserimento in coda dei pacchetti da 1392B ricevuti da
netDuino.
**/
void insert(duinoData **h, duinoData **t, char *r, int bScritti){
	
	duinoData *nodo;
	nodo = (duinoData *)malloc(sizeof(duinoData));
	
	if(nodo==NULL){//se ritorna null 
		printf("Errore malloc, exit\n");
		zlog_fatal(c, "Malloc null");
		exit(1);	
			}
		
		
	nodo->next = NULL;
	nodo->prec = (*t);
	nodo->byteScritti = bScritti;
	memset(nodo->pack,0,1392);
	//strcpy(nodo->pack,r);//copio il pacchetto ricevuto nel nodo
//	printf("ByteScritti %d strlen %d\n", bScritti, strlen(r));
	memcpy(nodo->pack, r, bScritti);
	if((*h)==NULL){//creazione lista
		*h = nodo;}
		
	else{//inserimento normale, lo faccio in coda
		(*t)->next = nodo;
	}
	(*t) = nodo;

}

/*
*check confirm message returned from server
* @param command: command sent before calling this function
* @duino data h, t: struct list head and tail
* return: 0-> OK msg, 1-> Error msg, 2-> server unreachable
*/

int checkAnswer(char* command,int sock){
	duinoData *temph = NULL, *tempt = NULL;

	int retry=2;
	if (rx(&temph, &tempt, sock, 1, 0) == 0 ){ 
		printf("Server answer:  %s\n", tempt->pack);
		return ( strstr( tempt->pack, "ERR") != NULL ) ? 1 : 0;		
	}
	/*rx(h, t, sock, 1, 0);
	//printf("Risposta di netDuino:  %s\n", (*t)->pack);
	if (strstr( (*t)->pack, "ERR:") != NULL ){
	printf("Error!\n\n");
		duinoData **temph, **tempt;
		unsigned int flag=0;
		while(retry !=0){
			sendto(sock,command,32, 0, (struct sockaddr *) &soac, sizeof(soac) );
			//sleep(1);
			rx(temph, tempt, sock, 1, 0);
			retry--;
		if ( strstr( (*tempt)->pack, "ERR:") == NULL ){//Se è ok break!
			flag=1;
			break;
			}
		}
	
	return (flag==1) ? 0 : -1;
	*/
	
	else return 2;

}




