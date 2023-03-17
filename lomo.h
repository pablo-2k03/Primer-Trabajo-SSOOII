#define NEGRO    0
#define ROJO     1
#define VERDE    2
#define AMARILLO 3
#define AZUL     4
#define MAGENTA  5
#define CYAN     6
#define BLANCO   7

#define TIPO_TRENNUEVO            1
#define TIPO_PETAVANCE            2
#define TIPO_AVANCE               3

#define TIPO_RESPPETAVANCETREN0 100
#define TIPO_RESPAVANCETREN0    200
#define TIPO_RESPTRENNUEVO      300

struct mensaje
 {long tipo;
  int  tren;
  int  x,y;
  };

void pon_error(char *mensaje);

int LOMO_generar_mapa(char const *login1, char const *login2);
int LOMO_inicio(int ret, int semAforos, int buzOn,
                char const *login1, char const *login2);
void LOMO_espera(int y, int yn);
int LOMO_fin(void);