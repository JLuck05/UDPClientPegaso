#include "config_parser.h"
#include "zlog.h"
extern zlog_category_t * c;

/*

Questa funzione accetta il nome del file di config.ini e ne effettua il parsing. 

*/
sensorList * parse_ini_file(char * ini_name, config_params **params)
{
	
    dictionary * ini ;

    /* Some temporary variables to hold query results */
    int index, nsections, channelN=0;
    sensorList *head_s = NULL, *temp = NULL;
    char  *s = (char*)malloc(sizeof(char)*40);
    char *secName = (char*)malloc(sizeof(char)*40);
    char *token;
    

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
	zlog_fatal(c, "Error 2: cannot parse ini file, exiting...");
        return NULL ;
    }
    


    /* Get sections */
    if( (nsections = iniparser_getnsec(ini) ) == -1 ){
	zlog_fatal(c, "Error 2: cannot establish section number, exiting...");
        return NULL ;

	}

/* first section contains and standalone option and network configuration parameters */

	/*temp variable to save sectionName for further research into ini file*/
	if (secName==NULL){
		zlog_fatal(c, "Error 2: cannot allocate enough memory, exiting...");
		return NULL;
		}

	if ( iniparser_getsecname(ini,0) == NULL ){
		zlog_fatal(c, "Error 2: cannot read configuration section, exiting...");
		return NULL;
	}

	strcpy( secName, iniparser_getsecname(ini,0) );
	
		/* create params struct and insert network configuration params */
		*params = (config_params*)malloc(sizeof(config_params));
		if(*params == NULL){//check if memory is enough, exit otherwise
			zlog_fatal(c, "Error 2: cannot allocate enough memory, exiting...");
			return NULL;
		}
		/* get IP address from parsed file ini */
		strcat(secName, ":ip");
		(*params)->address = (char*)malloc(sizeof(char)*17);
		if ((*params)->address ==NULL){
			zlog_fatal(c, "Error 2: cannot allocate enough memory, exiting...");
			return NULL;
		}
		
		strcpy( (*params)->address, iniparser_getstring(ini, secName, NULL) );
		
		/* get port from parsed file ini */
		strcpy( secName, iniparser_getsecname(ini,0) );
		strcat(secName, ":port");
		char *string_port = (char*)malloc(sizeof(char)*17);
		strcpy(string_port, iniparser_getstring(ini, secName, NULL));
		int port = atoi( string_port );
		(*params)->port = port;

		/* get directory in wich files will be saved from file ini */
		strcpy( secName, iniparser_getsecname(ini,0) );
		strcat(secName, ":dir");
		(*params)->directory = (char*)malloc(sizeof(char)*50);
		if ((*params)->directory ==NULL){
			zlog_fatal(c, "Error 2: cannot allocate enough memory, exiting...");
			return NULL;
		}
		
		strcpy( (*params)->directory, iniparser_getstring(ini, secName, NULL) );


	zlog_debug(c, "network parameters inserted successfully");
  
for (index=1;index<nsections;index++){

	channelN=0; //variabile per il conteggio dei canali associati per sensore
	sensorList *sens = (sensorList *)malloc(sizeof(sensorList));
					
	if (sens == NULL){//check if memory is enough
		zlog_fatal(c, "Error 2: cannot allocate enough memory, exiting...");
		return NULL;
		}

	if(head_s == NULL){
		head_s = sens;
	}
	
	sens->next = NULL;
	sens->availableChannels = NULL;
	sens->sensore  = ( sensors* )malloc(sizeof(sensors));
		if (sens->sensore ==NULL){
		zlog_fatal(c, "Error 2: cannot allocate enough memory, exiting...");
		return NULL;
		}

	if ( iniparser_getsecname(ini,index) == NULL ){
	
		zlog_fatal(c, "Error 2: cannot read sensor name, exiting...");
		return NULL;

	}

	strcpy( s, iniparser_getsecname(ini,index) );
	

	sens->sensore->sensorName = (char*)malloc(sizeof(s));
	if (sens->sensore->sensorName ==NULL){
		zlog_fatal(c, "Error 2: cannot allocate enough memory, exiting...");
		return NULL;
	}

	strcpy(sens->sensore->sensorName, s);
	
	/* Inserisco il valore dello status*/
	strcpy(s, sens->sensore->sensorName);
	strcat(s, ":Status");
	/*char *status =  (char*)malloc(sizeof(char)*5);
	strcpy(status, );*/
	if(strcmp(iniparser_getstring(ini, s, NULL) ,"ON")==0 ) 
	sens->sensore->status = 1;
	else /* if status is OFF, set status to 0 */
	sens->sensore->status = 0;

	strcpy(s, sens->sensore->sensorName);
	strcat(s, ":Ch");
	char *channels = iniparser_getstring(ini, s, NULL);
	sens->sensore->chHead = NULL; //inizializzo il puntatore
	//separo i vari canali dalla stringa, 
	//strsep sposta il puntatore ogni volta fino a fine stringa, 
	//quindi mi serve una nuova stringa ad ogni ciclo
	while( (token = strsep(&channels, ",")) !=NULL ){ 
		chanList *canale = (chanList *)malloc(sizeof(chanList));
			if (canale ==NULL){
				zlog_fatal(c, "Error 2: cannot allocate enough memory, exiting...");
				return NULL;
			}

 		canale->val = atoi(token);
		canale->next = NULL;
	/* inserisco il canale nella lista di canali del sensore  */
		//printf("inserisco il canale %d nel sensore %s\n",canale->val,sens->sensore->sensorName);
		insertChannel( canale, &sens->sensore->chHead );
		channelN++;
	}//fine ciclo while
	free(channels);
	sens->sensore->chanNumber = channelN;

	strcpy(s, sens->sensore->sensorName);
	strcat(s, ":NSAD");
	sens->sensore->SADNumber = iniparser_getint(ini, s, 0);
	if (sens->sensore->SADNumber == 0){
		zlog_fatal(c, "Error 2: cannot find SAD Number");
		return NULL;
	}

	/*Se è la sezione relativa all'idrofono vanno riempiti campi addizionali*/
	if( strstr(sens->sensore->sensorName, "hydro") != NULL  ){

	bzero(s, sizeof(s));

	sens->sensore->idrofono = ( hydro* )malloc(sizeof(hydro));

	/* inserisce il valore del rate*/
	strcpy(s, sens->sensore->sensorName);
	strcat(s, ":rate");
	strcpy(s, iniparser_getstring(ini, s, NULL) );
	if(s != NULL ){

		int sampleRate = atoi( s );
		sens->sensore->idrofono->sampleRate = sampleRate;
	}


	strcpy(s, sens->sensore->sensorName);
	strcat(s, ":bps");
	strcpy(s, iniparser_getstring(ini, s, NULL) );

	if(s != NULL ){

		int bps = atoi( s );
		sens->sensore->idrofono->bps = bps;
	}
	

	/* inserisce il valore di endianess*/
	strcpy(s, sens->sensore->sensorName);
	strcat(s, ":endian");
	char *endian =  (char*)malloc(sizeof(char)*10);
	strcpy(endian, iniparser_getstring(ini, s, NULL) );
	
	if(endian != NULL ){
		sens->sensore->idrofono->endian = (char*)malloc(sizeof(s));
		strcpy(sens->sensore->idrofono->endian, endian);
	}

	
	/* inserisce il valore di sign*/
	char *sign =  (char*)malloc(sizeof(char)*10);
	strcpy(s, sens->sensore->sensorName);
	strcat(s, ":sign");
	strcpy(sign, iniparser_getstring(ini, s, NULL) );

	if(sign != NULL ){
		sens->sensore->idrofono->sign = (char*)malloc(sizeof(s));
		strcpy(sens->sensore->idrofono->sign, sign);
	  }
	free(sign);
	free(endian);
	}//fine if hydro


	for(temp=head_s;temp->next != NULL; temp = temp->next);
	if(temp != sens){
		temp->next = sens;
	}
	bzero(s, strlen(s)); //reinizializzo lo spazio allocato
 }
	free(s);
	free(secName);
	iniparser_freedict(ini);
	zlog_debug(c, "Parsing done with success!");

	return head_s;
}

/*
inserisce il canale nella lista di canali associati al sensore
arg1: channel = la strcut canale da inserire
arg2: head_ch = il doppio puntatore alla struct di testa
*/


void insertChannel(chanList *channel, chanList **head_ch){

 chanList *q=*head_ch,*r=*head_ch;

 while( (q != NULL) && (q->val > channel->val ) ){
  	r = q;
  	q = q->next;
       }
 if(q==*head_ch){
  	*head_ch = channel;
   }
 else r->next = channel;
 channel->next = q;

}


int MaxCh(chanList **head_ch){
chanList *q=*head_ch;

while( q->next != NULL ){
  	q = q->next;
       }

return q->val;

}

