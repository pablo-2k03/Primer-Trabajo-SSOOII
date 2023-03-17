#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lomo.h"
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
//#define DEBUG

struct{
    int idBuzon;
    struct semid_ds ssd;
    int idSemaforo;
    int nTrenes;
}recursosIPCS;




void signalHandler(int n);
int comprobarPrimerArgumento(char *argv);
int comprobarSegundoArgumento(char *argv);

// semaforo 0 del array -> uso de la biblioteca, creacion de 21 semaforos. ( 1-21 )

int main(int argc,char * argv[]){
    #ifdef DEBUG
        printf("%s %s\n",argv[0],argv[1]);
    #endif
    if(argc < 2){
        fprintf(stderr,"Usage: lomo --mapa\n");
        fprintf(stderr,"Other Usage: lomo retardo nTrenes\n");
        return 1;
    }
    if(!strcmp(argv[1],"--mapa")){
        LOMO_generar_mapa("i0959394","i0919297");
    }
    if(argc == 3){
        int retardo=0;
        recursosIPCS.nTrenes=0;
        //Creación recursos IPCS.
        if((recursosIPCS.idBuzon = msgget(IPC_PRIVATE,IPC_CREAT | 0600)) == -1){
            fprintf(stderr,"No se ha podido crear el buzon.\n");
            }
        if((recursosIPCS.idSemaforo = semget(IPC_PRIVATE,1,IPC_CREAT | 0600)) == -1){
            fprintf(stderr,"No se ha podido crear el semaforo.\n ");
        }
        //Creacion manejadora y auxiliar (hijos).


        struct sigaction aux;



        struct sigaction ss;
        ss.sa_handler = &signalHandler;
        ss.sa_flags = 0;

        if (sigemptyset(&ss.sa_mask) == -1){
            fprintf(stderr,"sigemptyset ERROR.\n");
        }
        if(sigaddset(&ss.sa_mask,SIGINT) == -1){
            fprintf(stderr,"sigaddset ERROR.\n");
        }
        if(sigaction(SIGINT,&ss,&aux) == -1 ){
            fprintf(stderr,"sigaction ERROR.\n");
        }
        //comprobar que argv1 es numerico
        if((retardo = comprobarPrimerArgumento(argv[1]))==-1){
            fprintf(stderr,"El primer argumento debe ser o 0 o 1.\n");
            return 1;
        }
        //comprobar que argv2 es numerico
        if((recursosIPCS.nTrenes = comprobarSegundoArgumento(argv[2]))==-1){
            fprintf(stderr,"El segundo argumento debe ser un numero.\n");
            return 1;
        }

       
       
        LOMO_inicio(retardo,recursosIPCS.idSemaforo,recursosIPCS.idBuzon,"i0959394","i0919297");
        //Creacion del tipo de mensaje.
        int i;
        struct mensaje msg;
        msg.tipo = TIPO_TRENNUEVO;
        msg.tren = 0;
        msg.x = 0;
        msg.y = 0;
        #ifdef DEBUG
            fprintf(stderr,"Trenes: %d",nTrenes);
        #endif
         
        
        for(i = 0; i< recursosIPCS.nTrenes; i++){
            if(fork()==0){
                //Reestablece antigua acción (auxiliar).
                if(sigaction(SIGINT,&aux,NULL) == -1 ){
                    fprintf(stderr,"sigaction ERROR.\n");
                }
                msgsnd(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje )- sizeof(long),0);
                msgrcv(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje)- sizeof(long),TIPO_RESPTRENNUEVO,0);
                
                while(1){             
                    msg.tipo =  TIPO_PETAVANCE;
                    int posAnterior = msg.y;
                    //el tipo de tren va a ser y* 74 + x  (TODO)
                    msgsnd(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje) - sizeof(long),0);
                    if((msgrcv(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje)- sizeof(long),TIPO_RESPPETAVANCETREN0+msg.tren,0)) == -1){
                        // Comprobar que leer mensaje sea == -1 , en ese caso, acabar. IPC_NOWAIT  
                        msgsnd(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje) - sizeof(long),IPC_NOWAIT);
                        break;
                    }

                    msg.tipo = TIPO_AVANCE;
                    msgsnd(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje) - sizeof(long),0);
                    msgrcv(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje)- sizeof(long), TIPO_RESPAVANCETREN0+msg.tren,0);
                    LOMO_espera(posAnterior,msg.y);
                }
            }
            else{
                sleep(2);
            }
        }
        pause();

        /*struct mensaje msg;
        msg.tipo = TIPO_TRENNUEVO;
        msg.tren = 0;
        msg.x = 0;
        msg.y = 0;
        int respuesta = msgsnd(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje) - sizeof(long),0);
        printf("%d",respuesta);


        while(1){
            struct mensaje msg;
            msg.tipo = TIPO_PETAVANCE;
            msg.tren = 0;
            msg.x = 36;
            msg.y = 0;
            msgsnd(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje) - sizeof(long),0);
            sleep(1);
        }
        */
    }
   
    return 0;
}


void signalHandler(int n){
    //fprintf(stderr,"Entro a la manejadora.\n");    
    LOMO_fin();
    int i;
    for(i=0; i< recursosIPCS.nTrenes;i++){
        wait(0);
    }
    if(msgctl(recursosIPCS.idBuzon,IPC_RMID,NULL) < 0){
        fprintf(stderr,"No se pudo liberar el buzon.\n");
    }
    if(semctl(recursosIPCS.idSemaforo,0,IPC_RMID,0) < 0){
        fprintf(stderr,"No se pudo liberar el semaforo.\n");
    }
    kill(0,SIGKILL);
   
}


int comprobarPrimerArgumento(char *argv){
    if(atoi(argv) < 0)
        return -1;
   
    return atoi(argv);

}

int comprobarSegundoArgumento(char *argv){
    int lenArgv = strlen(argv);
    int i;
    for(i = 0; i < lenArgv; i++){
        if(!isdigit(argv[i]))
            return -1;
    }
    if(atoi(argv) < 0 || atoi(argv) > 100)
        return -1;

    return atoi(argv);
}
