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

struct sockaddr;

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

#endif

/* Para cargar los datos que se envían o reciben de la toma */

typedef struct datos_toma {
    t_cntr_tope *tope;        /* Tope de datos entre la E/S                */
    char        *sdrt;        /* Separador de registro. Variable RS gawk   */
    size_t      tsr;          /* Tamaño cadena separador de registro       */
    size_t      lgtreg;       /* Tamaño actual del registro                */
    int         en_uso;
} t_datos_toma;

typedef struct cntr_toma_es {
#if GNU_LINUX
    t_evento        *evt;     /* Estructura de eventos (Linux epoll API)   */
    t_evento eva[CNTR_MAX_EVENTOS]; /* Df. listos que tienen eventos       */
    int             ndsf;     /* Nº dscs. de fichero listos (epoll_wait)   */
    int             ctdr      /* Orden en la lista de df listos            */
    int             sonda;    /* Descriptor sonda de eventos E/S (epoll)   */
#endif
    int             servidor; /* Descriptor servidor en modo escucha       */
    int             cliente;  /* Descriptor cliente (lectura/escritura)    */
    t_datos_toma    *pila;    /* Pila de datos entre el programa y la toma */
    struct addrinfo *infred;  /* Información de red TCP/IP (API Linux)     */
    t_ctrn_verdad   local;    /* ¿Toma local?                              */
} t_cntr_toma_es;

typedef struct elector_es {
    unsigned int es_servidor : 1;
    unsigned int es_cliente  : 1;
    unsigned int forzar      : 1;
} t_elector_es;

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

/* cntr_nueva_estructura_datos_toma */

t_datos_toma *
cntr_nueva_estructura_datos_toma(t_cntr_toma_es *toma, char *sr, size_t tpm);

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

/* cntr_cierra_toma --
 *
 * Cierra toma de una manera específica ya sea cliente o servidor (opcion)
 */

int
cntr_cierra_toma(t_cntr_toma_es *toma, t_elector_es opcion);

#endif /* TOMA_H */