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

#ifndef TOMA_H
#define TOMA_H

#define GNU_LINUX 0

/* Tome máximo para cola de conexiones pendientes */
#define CNTR_MAX_PENDIENTES 100

typedef enum cntr_verdad t_ctrn_verdad;

struct sockaddr;

struct addrinfo;

struct cntr_ruta;
typedef struct cntr_ruta t_cntr_ruta;

struct cntr_tope;
typedef struct cntr_tope t_cntr_tope;

#if GNU_LINUX
/* Número máximo de descriptores de fichero listos para la operación de E/S
   solicitada, que devuelve epoll_wait */
#define CNTR_MAX_EVENTOS 10

struct epoll_event;
typedef struct epoll_event t_evento;

typedef struct cntr_sonda {
    t_evento        *evt;     /* Estructura de eventos (Linux epoll API)   */
    t_evento eva[CNTR_MAX_EVENTOS]; /* Df. preparados y que tienen eventos */
    int             ndsf;     /* Nº dscs. de fichero listos (epoll_wait)   */
    int             ctdr      /* Orden en la lista de df listos            */
    int             dfsd;     /* Descriptor sonda de eventos E/S (epoll)   */
} t_cntr_sonda;
#endif

struct capa_gnutls;
typedef struct capa_gnutls t_capa_gnutls;

typedef ssize_t (*rcb_datos)(t_capa_gnutls *capatls, int df_cliente,
                             void *tope, size_t len);

typedef ssize_t (*env_datos)(t_capa_gnutls *capatls, int df_cliente,
                             const void *tope, size_t len);

/* Para cargar los datos que se envían o reciben de la toma */

typedef struct datos_toma {
    t_cntr_tope *tope;        /* Tope de datos entre la E/S                */
    char        *sdrt;        /* Separador de registro. Variable RS gawk   */
    size_t      tsr;          /* Tamaño cadena separador de registro       */
    size_t      lgtreg;       /* Tamaño actual del registro                */
    int         en_uso;
} t_cntr_dts_toma;

typedef struct cntr_toma_es {
    t_capa_gnutls   *gtls;
#if GNU_LINUX
    t_cntr_sonda    *sonda;   /* Sonda de eventos E/S (epoll API)          */
#endif
    int             servidor; /* Descriptor servidor en modo escucha       */
    int             cliente;  /* Descriptor cliente (lectura/escritura)    */
    t_cntr_dts_toma *pila;    /* Pila de datos entre el programa y la toma */
    struct addrinfo *infred;  /* Información de red TCP/IP (API Linux)     */
    t_ctrn_verdad   local;    /* ¿Toma local?                              */
    env_datos       envia;    /* Puntero al envío de datos                 */
    rcb_datos       recibe;   /* Puntero a la recepción de datos           */
} t_cntr_toma_es;

/* cntr_nueva_toma --
 *
 * Crea nueva toma 'nula' de E/S para una ruta
 */

t_cntr_toma_es *
cntr_nueva_toma(t_cntr_ruta *ruta);

/* cntr_borra_toma --
 *
 * Borra toma de la memoria
 */

void
cntr_borra_toma(t_cntr_toma_es *toma);

/* cntr_nueva_estructura_datos_toma --
 *
 * Crea estructura de la pila de datos
 */

t_cntr_dts_toma *
cntr_nueva_estructura_datos_toma(t_cntr_toma_es *toma, char *sr, size_t tpm);

/* cntr_envia_datos --
 *
 * Recubrimiento para enviar datos por la toma
 */

ssize_t
cntr_envia_datos(t_capa_gnutls *capatls, int df_cliente,
                 const void *tope, size_t len);

/* cntr_envia_toma --
 *
 * Envía datos por la toma de conexión
 */

int
cntr_envia_toma(t_cntr_toma_es *toma, const void *datos, size_t bulto);

/* cntr_recibe_linea_toma --
 *
 * Recibe línea terminada en RS por la toma de conexión
 */

char *
cntr_recibe_linea_toma(t_cntr_toma_es *toma, char **sdrt, size_t *tsr,
                       t_cntr_resultado **resul);

/* cntr_recibe_flujo_toma --
 *
 * Recibe un flujo contínuo de datos por la toma de conexión
 */

char *
cntr_recibe_flujo_toma(t_cntr_toma_es *toma, char **sdrt, size_t *tsr,
                       t_cntr_resultado **resul);

/* cntr_pon_a_escuchar_toma --
 *
 * Pone a escuchar la toma asociada a una ruta local
 */

int
cntr_pon_a_escuchar_toma(t_cntr_toma_es *toma);

/* cntr_trae_primer_cliente_toma --
 *
 * Extrae la primera conexión de una toma en modo de escucha
 */

int
cntr_trae_primer_cliente_toma(t_cntr_toma_es *toma,
                              struct sockaddr *cliente);

/* cntr_cierra_toma_cliente --
 *
 * Cierra toma de datos del cliente (punto de conexión al cliente)
 */

int
cntr_cierra_toma_cliente(t_cntr_toma_es *toma, int forzar);

/* cntr_cierra_toma_servidor --
 *
 * Cierra toma de datos del servidor (punto de conexión al servidor)
 */

int
cntr_cierra_toma_servidor(t_cntr_toma_es *toma, int forzar);

/* cntr_cierra_toma --
 *
 * Cierra toma de una manera específica ya sea cliente o servidor
 */

int
cntr_cierra_toma(t_cntr_toma_es *toma, int df_toma, int cliente, int forzar);

#endif /* TOMA_H */