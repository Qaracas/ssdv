/*
 * Autor: Ulpiano Tur de Vargas <ulpiano.tur.devargas@gmail.com>
 *
 * Este programa es software libre; puedes distribuirlo y/o
 * modificarlo bajo los términos de la Licencia Pública General de GNU
 * según la publicó la Fundación del Software Libre; ya sea la versión 3, o
 * (a su elección) una versión superior.
 *
 * Este programa se distribuye con la esperanza de que sea útil,
 * pero SIN NINGUNA GARANTIA; ni siquiera la garantía implícita de
 * COMERCIABILIDAD o APTITUD PARA UN PROPÓSITO DETERMINADO. Vea la
 * Licencia Pública General de GNU para más detalles.
 *
 * Deberás haber recibido una copia de la Licencia Pública General
 * de GNU junto con este software; mira el fichero LICENSE. Si
 * no, mira <https://www.gnu.org/licenses/>.
 *
 * Author: Ulpiano Tur de Vargas <ulpiano.tur.devargas@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this software; see the file LICENSE. If
 * not, see <https://www.gnu.org/licenses/>.
 */

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include "gawkapi.h"

#define API_AWK_V2

#define _GNU_SOURCE

#define MAX_CNX_PENDIENTES 100 /* Máximo de conexiones pendientes permitidas */
#define MAX_EVENTOS        10
#define LTD(x) (sizeof(x) / sizeof((x)[0]))

static const gawk_api_t *api; /* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensión conector: versión 1.0";

static awk_bool_t inicia_conector(void);
static awk_bool_t (*init_func)(void) = inicia_conector;

int plugin_is_GPL_compatible;

typedef struct descriptores_es {
    int toma_entrada; /* Descriptor servidor en modo escucha    */
    int toma_salida;  /* Descriptor cliente (conexión entrante) */
} t_des;

typedef struct ruta_de_red {
    char            *nombre; /* Nombre fichero de red */
    awk_bool_t      local;   /* ¿Toma local o remota? */
    struct addrinfo *stoma;  /* Estructura de la toma */
    t_des           des;     /* Descriptores E/S      */
} t_ruta;

/* Variables globales que mantienen su valor */

static t_ruta rt;  /* Ruta de conexión */

static int libera; /* Indica memoria liberada */

/* es_dirip -- Modifica criterios evitando a getaddrinfo() resolver nombre */

//static int
//es_dirip(char *ip, struct addrinfo *criterios)
//{
//    struct sockaddr_in t4;
//    struct in6_addr    t6;
//
//    int rc = inet_pton(AF_INET, ip, &t4.sin_addr);
//    if (rc == 1) {     /* ¿Dirección IPv4 válida? */
//        criterios->ai_family = AF_INET;
//        criterios->ai_flags |= AI_NUMERICHOST;
//    } else {
//        rc = inet_pton(AF_INET6, ip, &t6);
//        if (rc == 1) { /* ¿Dirección IPv6 válida? */
//            criterios->ai_family = AF_INET6;
//            criterios->ai_flags |= AI_NUMERICHOST;
//        }
//    }
//    return rc;
//}

static awk_bool_t
es_nodoip (char *nodo, char *puerto, 
           struct addrinfo **resultados, awk_bool_t *es_local)
{
    int r;
    struct addrinfo criterios;

    /* Criterios para seleccionar las estructuras de tomas IP
     * que se volcarán a la lista 'resultados' */
    memset(&criterios, 0, sizeof(struct addrinfo));
    criterios.ai_family = AF_UNSPEC;     /* Vale IP v4 ó v6 */
    criterios.ai_socktype = SOCK_STREAM; /* Toma de datos sobre TCP */

    if ((r = getaddrinfo(nodo, puerto, &criterios, resultados)) != 0) {
        fatal(ext_id, "getaddrinfo: %s: %s\n", nodo, gai_strerror(r));
        return awk_false;
    }

    /* Averiguamos si el nodo es local */
    struct ifaddrs *tomas_locales, *i;
    struct addrinfo *j;

    *es_local = awk_false;

    if (getifaddrs(&tomas_locales) == -1) {
        fatal(ext_id, "conector: error obteniendo tomas de red locales");
        return awk_false;
    }

    void *resul, *local;
    j = *resultados;
    while (j){
        switch (j->ai_family) {
        case AF_INET:
            resul = &((struct sockaddr_in*)j->ai_addr)->sin_addr;
            break;
        case AF_INET6:
            resul = &((struct sockaddr_in6*)j->ai_addr)->sin6_addr;
            break;
        }
        i = tomas_locales;
        while (i){
            switch (i->ifa_addr->sa_family) {
            case AF_INET:
                local = &((struct sockaddr_in*)i->ifa_addr)->sin_addr;
                break;
            case AF_INET6:
                local = &((struct sockaddr_in6*)i->ifa_addr)->sin6_addr;
                break;
            }
            if (i->ifa_addr->sa_family == AF_INET) {
                if (   ntohl(((struct in_addr*)local)->s_addr)
                    == ntohl(((struct in_addr*)resul)->s_addr))
                {
                    *es_local = awk_true;
                    goto fin;
                }
            } else {
                if ((memcmp
                        (((struct in6_addr*)local)->s6_addr,
                         ((struct in6_addr*)resul)->s6_addr,
                         sizeof(((struct in6_addr*)resul)->s6_addr))) == 0)
                {
                    *es_local = awk_true;
                    goto fin;
                }
            }
            i = i->ifa_next;
        }
        j = j->ai_next;
    }

fin:
    freeifaddrs(tomas_locales);
    return awk_true;
}

static awk_bool_t
es_numero(char *texto)
{
    if (texto != NULL) {
        int j = strlen(texto);
        while(j--) {
            if(isdigit((int)texto[j]))
                continue;
            return awk_false;
        }
        return awk_true;
    }
    return awk_false;
}

static int
caracter_fin(const char *txt)
{
    if(*txt)
        return txt[strlen(txt + 1)];
    else
        return -1;
}

static int
caracter_ini(const char *txt)
{
    if(*txt)
        return txt[0];
    else
        return -1;
}

static int
cuenta_crtrs(const char *txt, char c)
{
    int i, cnt = 0;
    for(i = 0; txt[i]; i++)
        if(txt[i] == c)
            cnt++;
    return cnt;
}

/* Nombres especiales para los ficheros de red
 * 
 * /tipo-red/protocolo/ip-local/puerto-local/nombre-ip-remoto/puerto-remoto
 *
 * Ejemplos: 
 *   - Servidor: /ired/tcp/192.168.1.32/7080/0/0
 *   - Cliente : /ired/tcp/0/0/www.ejemplo.es/8080
 */

/* es_fichero_especial -- Verifica/crea ruta de un nombre de fichero de red */

static awk_bool_t
es_fichero_especial(const char *nombre, t_ruta *ruta)
{
    unsigned int c;
    char *campo[6];
    char *fichero_red;

    ruta->local = awk_false;

    emalloc(fichero_red, char *,
            strlen((const char *) nombre) + 1, 
            "es_fichero_especial");
    strcpy(fichero_red, (const char *) nombre);

    if (   cuenta_crtrs(fichero_red, '/') != 6
        || caracter_ini(fichero_red) == -1
        || caracter_fin(fichero_red) == -1
        || caracter_ini(fichero_red) != '/'
        || caracter_fin(fichero_red) == '/'
       )
        goto no_es;

    campo[0] = strtok(fichero_red, "/");
    for (c = 0; (c < LTD(campo) - 1) && campo[c] != NULL;)
        campo[++c] = strtok(NULL, "/");

    if (   c != (LTD(campo) - 1)
        || strcmp(campo[0], "ired") != 0
        || strcmp(campo[1], "tcp") != 0
        || !es_numero(campo[3])
        || !es_numero(campo[5])
       )
        goto no_es;

    if (   strcmp(campo[2], "0") == 0
        && strcmp(campo[3], "0") == 0
        && strcmp(campo[4], "0") != 0
        && atoi(campo[5]) > 0
        && es_nodoip(campo[4], campo[5], &ruta->stoma, &ruta->local)
        && !ruta->local)
    { /* Cliente */
        // return awk_true;
        goto no_es; /* De momento no */
    } else if 
       (   strcmp(campo[4], "0") == 0
        && strcmp(campo[5], "0") == 0
        && strcmp(campo[2], "0") != 0
        && atoi(campo[3]) > 0
        && es_nodoip(campo[2], campo[3], &ruta->stoma, &ruta->local)
        && ruta->local)
    { /* Servidor */
        goto si_es;
    }
no_es:
    gawk_free(fichero_red);
    return awk_false;
si_es:
    gawk_free(fichero_red);
    return awk_true;
}


/**
 * Funciones que proporciona 'conector.c' a GAWK
 */

/* haz_crea_toma -- Crea toma de escucha */

static awk_value_t *
#ifdef API_AWK_V2
haz_crea_toma(int nargs, awk_value_t *resultado, struct awk_ext_func *desusado)
#else
haz_crea_toma(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t nombre;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1)
        fatal(ext_id, "creatoma: nº de argumentos incorrecto");

    if (! get_argument(0, AWK_STRING, &nombre))
        fatal(ext_id, "creatoma: tipo de argumento incorrecto");

    if (   nombre.str_value.str != NULL
        && es_fichero_especial((const char *) nombre.str_value.str, &rt)
        && rt.local)
    {
        emalloc(rt.nombre, char *,
                strlen((const char *) nombre.str_value.str) + 1,
                "haz_crea_toma");
        strcpy(rt.nombre, (const char *) nombre.str_value.str);
    } else {
        fatal(ext_id, "creatoma: fichero de red remoto o no válido");
    }

    struct addrinfo *rp;
    for (rp = rt.stoma; rp != NULL; rp = rp->ai_next) {
        /* Crear toma de entrada y guardar df asociado a ella */
        rt.des.toma_entrada = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (rt.des.toma_entrada == -1)
            continue;
        /* Asociar toma de entrada a una dirección IP y un puerto */
        int activo = 1;
        setsockopt(rt.des.toma_entrada, SOL_SOCKET, SO_REUSEADDR,
                   &activo, sizeof(activo));
        if (bind(rt.des.toma_entrada, rp->ai_addr, rp->ai_addrlen) == 0)
            break; /* Hecho */
        close(rt.des.toma_entrada);
    }

    if (rp == NULL) {
        perror("bind");
        fatal(ext_id, "creatoma: error nombrando toma de entrada");
    }
    
    freeaddrinfo(rt.stoma); /* Ya no se necesita */

    /* Poner toma en modo escucha */
    if (listen(rt.des.toma_entrada, MAX_CNX_PENDIENTES) < 0) {
        perror("listen");
        fatal(ext_id, "creatoma: error creando toma de escucha");
    }

    return make_number(rt.des.toma_entrada, resultado);
}

/* haz_cierra_toma -- Cierra toma de escucha */

static awk_value_t *
#ifdef API_AWK_V2
haz_cierra_toma(int nargs, awk_value_t *resultado, struct awk_ext_func *desusado)
#else
haz_cierra_toma(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t nombre;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1) {
        lintwarn(ext_id, "cierratoma: nº de argumentos incorrecto");
        return make_number(-1, resultado);
    }

    if (! get_argument(0, AWK_STRING, &nombre)) {   
        lintwarn(ext_id, "cierratoma: tipo de argumento incorrecto");
        return make_number(-1, resultado);
    }

    if (strcmp((const char *) nombre.str_value.str, rt.nombre) != 0) {
        lintwarn(ext_id, "cierratoma: toma no válida");
        return make_number(-1, resultado);
    }

    if (shutdown(rt.des.toma_entrada, SHUT_RDWR) < 0) {
        perror("shutdown");
        lintwarn(ext_id, "cierratoma: error apagando toma");
    }

    if (close(rt.des.toma_entrada) < 0) {
        perror("cierra");
        lintwarn(ext_id, "cierratoma: error cerrando toma");
    }

    rt.des.toma_entrada = INVALID_HANDLE;
    return make_number(0, resultado);
}

/* haz_extrae_primera -- Extrae primera conexión de la cola de conexiones */

static awk_value_t *
#ifdef API_AWK_V2
haz_extrae_primera(int nargs, awk_value_t *resultado,
                   struct awk_ext_func *desusado)
#else
haz_extrae_primera(int nargs, awk_value_t *resultado)
#endif
{
    awk_value_t nombre;
    struct sockaddr_in cliente;
    socklen_t lnt = (socklen_t) sizeof(cliente);

#ifdef API_AWK_V2
    (void) desusado;
#endif

    /* Sólo acepta 1 argumento */
    if (nargs != 1)
        fatal(ext_id, "traepctoma: nº de argumentos incorrecto");

    if (! get_argument(0, AWK_STRING, &nombre))
        fatal(ext_id, "traepctoma: tipo de argumento incorrecto");

    if (strcmp((const char *) nombre.str_value.str, rt.nombre) != 0)
        fatal(ext_id, "traepctoma: toma escucha incorrecta");

    fd_set lst_df_sondear_lect, lst_df_sondear_escr;

    /* Borrar colección de tomas E/S a sondear */
    FD_ZERO(&lst_df_sondear_lect);
    FD_ZERO(&lst_df_sondear_escr);
    /* Sondear toma de escucha */
    FD_SET(rt.des.toma_entrada, &lst_df_sondear_lect);

    while (1) {
        /* Parar hasta que llegue evento a una o más tomas activas */
        if (select(FD_SETSIZE, &lst_df_sondear_lect, &lst_df_sondear_escr,
                   NULL, NULL) < 0) {
            perror("select");
            fatal(ext_id, "traepctoma: error esperando eventos E/S");
        }
        /* Atender tomas con eventos de entrada pendientes */
        if (FD_ISSET(rt.des.toma_entrada, &lst_df_sondear_lect)) {
            /* Extraer primera conexión de la cola de conexiones */
            rt.des.toma_salida = accept(rt.des.toma_entrada,
                                        (struct sockaddr*) &cliente,
                                        &lnt);
            /* ¿Es cliente? */
            if (rt.des.toma_salida < 0) {
                perror("accept");
                fatal(ext_id, "traepctoma: error al extraer conexión");
            }
            
            /* Sí; es cliente */

            /* POR HACER: Función para anunciar cliente */
            //printf ("Conexión desde %s, puerto %d.\n",
            //        inet_ntoa (cliente.sin_addr),
            //        ntohs (cliente.sin_port));

sondea_salida:
            FD_ZERO(&lst_df_sondear_lect);
            FD_ZERO(&lst_df_sondear_escr);
            FD_SET(rt.des.toma_salida, &lst_df_sondear_lect);
            FD_SET(rt.des.toma_salida, &lst_df_sondear_escr);
        } else {
            if (   FD_ISSET(rt.des.toma_salida, &lst_df_sondear_lect)
                && FD_ISSET(rt.des.toma_salida, &lst_df_sondear_escr))
                break;
            else
                goto sondea_salida;
        }
    }

    /* Devuelve accept(), es decir, el descriptor de fichero de la 
       toma de conexión recién creada con el cliente. */
    return make_number(rt.des.toma_salida, resultado);
}

/**
 * Procesador bidireccional que proporciona 'conector.c' a GAWK
 */

/* Los datos del puntero oculto */

typedef struct datos_proc_bidireccional {
    char    *tope;  /* Tope de datos                              */
    char    *sdrt;  /* Separador de registro. Variable RS de gawk */
    size_t  tsr;    /* Tamaño cadena separador de registro        */
    size_t  max;    /* Tamaño máximo asignado al tope             */
    ssize_t lgtope; /* Tamaño actual del tope                     */
    size_t  lgtreg; /* Tamaño actual del registro                 */
    int     ptrreg; /* Puntero inicio registro en tope (actual)   */
    int     ptareg; /* Puntero inicio registro en tope (anterior) */
} t_datos_conector;

/* libera_conector -- Libera datos */

static void
libera_conector(t_datos_conector *flujo)
{
    /* Evita liberar memoria dos veces */
    if (libera > 0)
        libera--;
    else
        return;

    gawk_free(flujo->sdrt);
    gawk_free(flujo->tope);
    gawk_free(flujo);
}

/* long_registro -- Calcula distancia entre dos posiciones de memoria */

static int
long_registro(char *ini, char *fin)
{
    int c = 0;
    char *i = ini;

    while (i++ != fin)
        c++;

    return c;
}

/* cierra_toma_entrada -- Cerrar toma conexión entrada */

static void
cierra_toma_entrada(awk_input_buf_t *iobuf)
{
    t_datos_conector *flujo;

    if (iobuf == NULL || iobuf->opaque == NULL)
        return;

    flujo = (t_datos_conector *) iobuf->opaque;
    libera_conector(flujo);

    /* haz_cierra_toma() hace esto (conexión servidor) */

    iobuf->fd = INVALID_HANDLE;
}

/* cierra_toma_salida --- Cerrar toma conexión salida */

static int
cierra_toma_salida(FILE *fp, void *opaque)
{
    t_datos_conector *flujo;

    if (opaque == NULL)
        return EOF;

    /* Ignorar flujo en favor del descriptor de fichero */
    (void) fp;

    flujo = (t_datos_conector *) opaque;

    /* Leemos lo que quede antes de cerrar la toma */
//    while (1) {
//        flujo->lgtope = recv(rt.des.toma_salida,
//                             flujo->tope + flujo->ptrreg,
//                             flujo->max - flujo->ptrreg, MSG_DONTWAIT);
//        if ((   flujo->lgtope < 0
//             && (EAGAIN == errno || EWOULDBLOCK == errno))
//            || flujo->lgtope == 0)
//            break;
//    }

    libera_conector(flujo);
    
    if (shutdown(rt.des.toma_salida, SHUT_RDWR) < 0) {
        perror("shutdown");
        lintwarn(ext_id, "cierra_toma_salida: error apagando salida");
    }

    /* Evita que la toma quede en espera (TIME_WAIT) al cerrarse */
    struct linger so_linger;
    so_linger.l_onoff  = 1;
    so_linger.l_linger = 0;
    setsockopt(rt.des.toma_salida, SOL_SOCKET, SO_LINGER,
               &so_linger, sizeof(so_linger));

    /* La toma no queda en espera al cerrar. Ver SO_LINGER */
    if(close(rt.des.toma_salida) < 0) {
        perror("close");
        lintwarn(ext_id, "cierra_toma_salida: error cerrando salida");
    }

    rt.des.toma_salida = INVALID_HANDLE;
    return 0;
}

/* apaga_toma_salida --- Apagar toma de salida */

static int
apaga_toma_salida(FILE *fp, void *opaque)
{
    (void) fp;
    (void) opaque;

    return 0;
}

/* maneja_error --- De momento no hacer nada */

static int
maneja_error(FILE *fp, void *opaque)
{
    (void) fp;
    (void) opaque;

    return 0;
}

/* conector_trae_registro -- Lee un registro cada vez */

static int
#ifdef API_AWK_V2
conector_trae_registro(char **out, awk_input_buf_t *iobuf, int *errcode,
                       char **rt_start, size_t *rt_len,
                       const awk_fieldwidth_info_t **desusado)
#else
conector_trae_registro(char **out, awk_input_buf_t *iobuf, int *errcode,
                       char **rt_start, size_t *rt_len)
#endif
{
    t_datos_conector *flujo;

    (void) errcode;

#ifdef API_AWK_V2
    (void) desusado;
#endif

    if (out == NULL || iobuf == NULL || iobuf->opaque == NULL)
        return EOF;

    flujo = (t_datos_conector *) iobuf->opaque;

    if (flujo->lgtope == 0) {
lee_mas:
        flujo->lgtope = recv(rt.des.toma_salida,
                             flujo->tope + flujo->ptrreg,
                             flujo->max - flujo->ptrreg, 0);

        if (flujo->lgtope <= 0) {
            if (flujo->ptrreg > 0) {
                bzero(flujo->tope + flujo->ptrreg,
                      flujo->max - flujo->ptrreg);

                *out = flujo->tope;
                return flujo->ptrreg;
            }else
                return EOF;
        } 

        /* Limpiar tope sobrante */
        if (((size_t)flujo->lgtope + flujo->ptrreg) < flujo->max)
            bzero(flujo->tope + ((size_t)flujo->lgtope + flujo->ptrreg),
                  flujo->max - ((size_t)flujo->lgtope + flujo->ptrreg));

        flujo->ptareg = flujo->ptrreg;
        flujo->ptrreg = 0;
    } else {
        /* Apunta al siguiente registro del tope */
        flujo->ptrreg += flujo->lgtreg + (int) flujo->tsr;
    }

    /* Apuntar al siguiente registro (variable RT) */
    *rt_start = strstr((const char*) flujo->tope + flujo->ptrreg,
                       (const char*) flujo->sdrt);
    *rt_len = flujo->tsr;

    if (*rt_start == NULL) {
        *rt_len = 0;
        
        /* Copia lo que nos queda por leer al inicio */
        memcpy(flujo->tope, (const void *) (flujo->tope + flujo->ptrreg),
               (flujo->lgtope + flujo->ptareg) - flujo->ptrreg);
        flujo->ptrreg = (flujo->lgtope + flujo->ptareg) - flujo->ptrreg;

        goto lee_mas;
    }

    flujo->lgtreg = long_registro(flujo->tope + flujo->ptrreg, *rt_start);

    *out = flujo->tope + flujo->ptrreg;
    return flujo->lgtreg;
}

/* conector_escribe -- Envía respuesta a solicitud del cliente */

static size_t
conector_escribe(const void *buf, size_t size, size_t count, FILE *fp,
                 void *opaque)
{
    (void) fp;
    (void) opaque;

    if (send(rt.des.toma_salida, buf, (size * count), 0) < 0) {
        perror("send");
        lintwarn(ext_id, "conector_escribe: registro demasiado extenso");
        return EOF;
    }

    return (size * count);
}

/* conector_puede_aceptar_fichero -- retorna "true" si aceptamos fichero */

static awk_bool_t
conector_puede_aceptar_fichero(const char *name)
{
    return (   name != NULL
            && strcmp(name, rt.nombre) == 0 /* Toma registrada 'creatoma' */
            && rt.des.toma_salida > 0       /* De momento */
            && rt.local);                   /* De momento sólo local */
}

/* conector_tomar_control_de -- Prepara procesador bidireccional */

static awk_bool_t
conector_tomar_control_de(const char *name, awk_input_buf_t *inbuf,
                          awk_output_buf_t *outbuf)
{
    awk_value_t valor_tpm, valor_rs;
    t_datos_conector *flujo;

    (void) name;

    if (   inbuf == NULL || outbuf == NULL
        || !(sym_lookup("TPM", AWK_NUMBER, &valor_tpm)) /* ToPe Máximo */
        || valor_tpm.num_value <= 0
        || !(sym_lookup("RS",  AWK_STRING, &valor_rs))  /* Valor de RS */
       )
        return awk_false;

    /* Memoriza estructura opaca */
    emalloc(flujo, t_datos_conector *,
        sizeof(t_datos_conector), "conector_tomar_control_de");

    flujo->lgtope = 0;
    flujo->lgtreg = 0;
    flujo->ptrreg = 0;

    flujo->tsr = strlen((const char *) valor_rs.str_value.str);
    emalloc(flujo->sdrt, char *,
            flujo->tsr + 1, "conector_tomar_control_de");
    strcpy(flujo->sdrt, (const char *) valor_rs.str_value.str);

    flujo->max = (size_t) valor_tpm.num_value;
    emalloc(flujo->tope, char *,
            flujo->max, "conector_tomar_control_de");
    bzero(flujo->tope, flujo->max);
    libera = 1;

    /* Entrada */
    inbuf->fd = rt.des.toma_salida + 1;
    inbuf->opaque = flujo;
    inbuf->get_record = conector_trae_registro;
    inbuf->close_func = cierra_toma_entrada;

    /* Salida */
    outbuf->fp = fdopen(rt.des.toma_salida, "w");
    outbuf->opaque = flujo;
    outbuf->gawk_fwrite = conector_escribe;
    outbuf->gawk_fflush = apaga_toma_salida;
    outbuf->gawk_ferror = maneja_error;
    outbuf->gawk_fclose = cierra_toma_salida;
    outbuf->redirected = awk_true;

    return awk_true;
 }

/* Registra nuevo procesador bidireccional llamado 'conector' */

static awk_two_way_processor_t conector_es = {
    "conector",
    conector_puede_aceptar_fichero,
    conector_tomar_control_de,
    NULL
};

/* inicia_conector -- Incicia conector bidireccional */

static awk_bool_t
inicia_conector()
{
    register_two_way_processor(& conector_es);
    return awk_true;
}

#ifdef API_AWK_V2
static awk_ext_func_t lista_de_funciones[] = {
    { "creatoma",   haz_crea_toma,      1, 1, awk_false, NULL },
    { "cierratoma", haz_cierra_toma,    1, 1, awk_false, NULL },
    { "traepctoma", haz_extrae_primera, 1, 1, awk_false, NULL },
};
#else
static awk_ext_func_t lista_de_funciones[] = {
    { "creatoma",   haz_crea_toma,      1 },
    { "cierratoma", haz_cierra_toma,    1 },
    { "traepctoma", haz_extrae_primera, 1 },
};
#endif

/* Cargar funciones */

dl_load_func(lista_de_funciones, conector, "")
