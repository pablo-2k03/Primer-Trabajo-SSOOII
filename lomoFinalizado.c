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
    int idBuzonMsj;
}recursosIPCS;

typedef struct{
    long tipo; //A que casilla va a ir.
    int boolean;
    
}tipoMensaje;



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
    if(argc == 2 && !strcmp(argv[1],"--mapa")){
        if(LOMO_generar_mapa("i0959394","i0919297") == -1){
            fprintf(stderr,"Error al generar el mapa.\n");
            return 1;
        }
        else{
            fprintf(stderr,"Mapa generado correctamente.\n");
            return 1;
        }
    }
    if(comprobarPrimerArgumento(argv[1])!=-1 && comprobarSegundoArgumento(argv[2])!=-1){
        int retardo=0;
        recursosIPCS.nTrenes=0;
        tipoMensaje tipoMensajes;
        //Creación recursos IPCS.
        if((recursosIPCS.idBuzon = msgget(IPC_PRIVATE,IPC_CREAT | 0600)) == -1){
            fprintf(stderr,"No se ha podido crear el buzon.\n");
        }
        if((recursosIPCS.idSemaforo = semget(IPC_PRIVATE,1,IPC_CREAT | 0600)) == -1){
            fprintf(stderr,"No se ha podido crear el semaforo.\n ");
        }
        if((recursosIPCS.idBuzonMsj = msgget(IPC_PRIVATE,IPC_CREAT | 0600)) == -1){
            fprintf(stderr,"No se ha podido crear el buzon de mensajes.\n");
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

       
       
        if(LOMO_inicio(retardo,recursosIPCS.idSemaforo,recursosIPCS.idBuzon,"i0959394","i0919297") == -1){
            fprintf(stderr,"Error al iniciar LOCOMOTION.\n");
            return 1;
        }
        
        int x;
        //Mandar mensajes de tipo x a todas las casillas.
        for(x = 1; x <= 75*17; x++){ 
            tipoMensajes.tipo = x; //Casilla a la q va.
            tipoMensajes.boolean = 0; //No hay tren.
            msgsnd(recursosIPCS.idBuzonMsj,&tipoMensajes,sizeof(tipoMensajes) - sizeof(long),0);
        }

        #ifdef DEBUG
            fprintf(stderr,"Trenes: %d",nTrenes);
        #endif
        
        //Creacion del tipo de mensaje.
        int i;
        struct mensaje msg;
        msg.tipo = TIPO_TRENNUEVO;
        msg.tren = 0;
        msg.x = 0;
        msg.y = 0;
        
        
        for(i = 0; i< recursosIPCS.nTrenes; i++){
            if(fork()==0){
                //Reestablece comportamiento sigaction.
                if(sigaction(SIGINT,&aux,NULL) == -1 ){
                    fprintf(stderr,"sigaction ERROR.\n");
                }
                //Crear un tren.
                if(msgsnd(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje )- sizeof(long),0) == -1){
                    fprintf(stderr,"Error al enviar el mensaje de creación del tren.\n");
                }
                if(msgrcv(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje)- sizeof(long),TIPO_RESPTRENNUEVO,0) == -1){
                    fprintf(stderr,"Error al recibir la respuesta de la biblioteca en la creación del tren.\n");
                }
                
                while(1){             
                    msg.tipo =  TIPO_PETAVANCE;
                    int posAnterior = msg.y;           
                    if(msgsnd(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje) - sizeof(long),0) == -1){
                        fprintf(stderr,"Error al enviar el mensaje de petición de avance del tren.\n");
                    }
                    if(msgrcv(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje)- sizeof(long),TIPO_RESPPETAVANCETREN0+msg.tren,0) == -1){
                        fprintf(stderr,"Error al recibir la respuesta de la biblioteca en la petición de avance del tren.\n");
                    }
                    
                    //Si hay un mensaje disponible en el buzon de mensajes, se lee y continua y sino, se queda esperando uno.
                    // +1 en caso de coord x = 0 y coord y = 0
                    if(msgrcv(recursosIPCS.idBuzonMsj,&tipoMensajes,sizeof(tipoMensajes) - sizeof(long),msg.y* 75 + msg.x+1,0) == -1){
                        fprintf(stderr,"Error al recibir el mensaje de la casilla.\n");
                    } 

                    //Mensajes para realizar el avance del tren.
                    msg.tipo = TIPO_AVANCE;
                    if(msgsnd(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje) - sizeof(long),0) == -1){
                        fprintf(stderr,"Error al enviar el mensaje de avance del tren.\n");
                    }
                    if(msgrcv(recursosIPCS.idBuzon,&msg,sizeof(struct mensaje)- sizeof(long), TIPO_RESPAVANCETREN0+msg.tren,0) == -1){
                        fprintf(stderr,"Error al recibir la respuesta de la biblioteca en el avance del tren.\n");
                    }
                    //Sin if porque es de tipo void.
                    LOMO_espera(posAnterior,msg.y);
                    //Libera el culo del tren, porque en msg.x y msg.y esta la posicion anterior.
                    if(msg.x >= 0 && msg.y  >=0){
                        tipoMensajes.tipo = msg.y* 75 + msg.x+1; // liberar la casilla que calculas.
                        if(msgsnd(recursosIPCS.idBuzonMsj,&tipoMensajes,sizeof(tipoMensajes)- sizeof(long),0) == -1){
                            fprintf(stderr,"Error al enviar el mensaje de liberación de la casilla.\n");
                        }                         
                    }     
                }
            }
        }
        pause();
    }
    else{
        fprintf(stderr,"Introduzca unos argumentos validos.\n");
    } 
    return 0;
}


void signalHandler(int n){
    //fprintf(stderr,"Entro a la manejadora.\n");    
    if(LOMO_fin() == -1){
        fprintf(stderr,"Error al finalizar LOCOMOTION.\n");
    }
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
    if(msgctl(recursosIPCS.idBuzonMsj,IPC_RMID,NULL) < 0){
        fprintf(stderr,"No se pudo liberar el buzon de mensajes.\n");
    }
    kill(0,SIGKILL);
   
}


int comprobarPrimerArgumento(char *argv){
    if(atoi(argv) < 0 || comprobarSegundoArgumento(argv) == -1)
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
