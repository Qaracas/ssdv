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

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <string.h>

#include "cntr_defcom.h"
#include "cntr_ruta.h"
#include "cntr_toma.h"
#include "cntr_stoma.h"
#include "cntr_tope.h"

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

/* cntr_nueva_toma -- Crea nueva toma 'nula' de E/S para una ruta */

int
cntr_nueva_toma(t_cntr_ruta *ruta)
{
    if (ruta == NULL || ruta->toma != NULL)
        return CNTR_ERROR;

    cntr_asigmem(ruta->toma, t_cntr_toma_es *,
                 sizeof(t_cntr_toma_es), "cntr_nueva_toma");
    ruta->toma->servidor = CNTR_DF_NULO;
    ruta->toma->cliente  = CNTR_DF_NULO;

    return CNTR_HECHO;
}

/* cntr_borra_toma -- Borra toma de la memoria */

void
cntr_borra_toma(t_cntr_ruta *ruta)
{
    free(ruta->toma);
    ruta->toma = NULL;
}

/* cntr_envia_a_toma -- Envía datos por la toma de conexión */

int
cntr_envia_toma(t_cntr_ruta *ruta, const void *datos, size_t tramo)
{
    if (send(ruta->toma->cliente, datos, tramo, 0) < 0) {
        perror("send");
        return CNTR_ERROR;
    }
    return CNTR_HECHO;
}

/* cntr_recibe_toma -- Recibe datos por la toma de conexión */

int
cntr_recibe_toma(t_cntr_ruta **ruta, t_datos_conector **dc,
                 char **out, char **rt_start, size_t *rt_len)
{
    int recbt;

    if ((*dc)->tope->ldatos == 0) {
lee_mas:
        recbt = cntr_recb_llena_tope(*ruta, (*dc)->tope);
        switch (recbt) {
            case CNTR_TOPE_RESTO:
                *out = (*dc)->tope->datos;
                return (*dc)->tope->ptrreg;
            case CNTR_TOPE_VACIO:
                return EOF;
            case CNTR_ERROR:
                return CNTR_ERROR;
        }
    } else {
        /* Apunta al siguiente registro del tope */
        (*dc)->tope->ptrreg += (*dc)->lgtreg + (int) (*dc)->tsr;
    }

    /* Apuntar al siguiente registro (variable RT) */
    *rt_start = strstr(  (const char*) (*dc)->tope->datos
                       + (*dc)->tope->ptrreg,
                       (const char*) (*dc)->sdrt);
    *rt_len = (*dc)->tsr;

    if (*rt_start == NULL) {
        *rt_len = 0;
        /* Copia lo que nos queda por leer al inicio del tope */
        memcpy((*dc)->tope->datos,
               (const void *) ((*dc)->tope->datos + (*dc)->tope->ptrreg),
                 ((*dc)->tope->ldatos + (*dc)->tope->ptareg)
               - (*dc)->tope->ptrreg);
        (*dc)->tope->ptrreg =   ((*dc)->tope->ldatos + (*dc)->tope->ptareg)
                           - (*dc)->tope->ptrreg;
        goto lee_mas;
    }

    (*dc)->lgtreg =   long_registro((*dc)->tope->datos
                    + (*dc)->tope->ptrreg, *rt_start);

    *out = (*dc)->tope->datos + (*dc)->tope->ptrreg;

    return CNTR_HECHO;
}

/* cntr_pon_a_escuchar_toma -- Pone a escuchar la toma 'nula' asociada
                               a una ruta local */

int
cntr_pon_a_escuchar_toma(t_cntr_ruta *ruta)
{
    if (   ruta == NULL || ruta->stoma == NULL || ruta->toma == NULL
        || ruta->toma->servidor != CNTR_DF_NULO
        || ruta->local == cntr_falso)
        return CNTR_ERROR;

    struct addrinfo *rp;

    for (rp = ruta->stoma; rp != NULL; rp = rp->ai_next) {
        /* Crear toma de entrada y guardar df asociado a ella */
        ruta->toma->servidor = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (ruta->toma->servidor == CNTR_DF_NULO)
            continue;
        /* Asociar toma de entrada a una dirección IP y un puerto */
        int activo = 1;
        setsockopt(ruta->toma->servidor, SOL_SOCKET, SO_REUSEADDR,
                   &activo, sizeof(activo));
        if (bind(ruta->toma->servidor, rp->ai_addr, rp->ai_addrlen) == 0)
            break; /* Hecho */
        close(ruta->toma->servidor);
    }

    if (rp == NULL) {
        perror("bind");
        return CNTR_ERROR;
    }

    /* Poner toma en modo escucha */
    if (listen(ruta->toma->servidor, CNTR_MAX_PENDIENTES) < 0) {
        perror("listen");
        return CNTR_ERROR;
    }

    cntr_borra_stoma(ruta); /* Ya no se necesita */
    return CNTR_HECHO;
}

/* cntr_trae_primer_cliente_toma -- Extrae la primera conexión de una toma en
                                    modo de escucha */

int
cntr_trae_primer_cliente_toma(t_cntr_ruta *ruta, struct sockaddr *cliente)
{
    if (   ruta == NULL || ruta->toma == NULL 
        || ruta->toma->servidor == CNTR_DF_NULO )
        return CNTR_ERROR;

    socklen_t lnt = (socklen_t) sizeof(*cliente);

    fd_set lst_df_sondear_lect, lst_df_sondear_escr;

    /* Borrar colección de tomas E/S a sondear */
    FD_ZERO(&lst_df_sondear_lect);
    FD_ZERO(&lst_df_sondear_escr);
    /* Sondear toma de escucha */
    FD_SET(ruta->toma->servidor, &lst_df_sondear_lect);

    while (1) {
        /* Parar hasta que llegue evento a una o más tomas activas */
        if (select(FD_SETSIZE, &lst_df_sondear_lect, &lst_df_sondear_escr,
                   NULL, NULL) < 0) {
            perror("select");
            return CNTR_ERROR;
        }
        /* Atender tomas con eventos de entrada pendientes */
        if (FD_ISSET(ruta->toma->servidor, &lst_df_sondear_lect)) {
            /* Extraer primera conexión de la cola de conexiones */
            ruta->toma->cliente = accept(ruta->toma->servidor, cliente, &lnt);
            /* ¿Es cliente? */
            if (ruta->toma->cliente < 0) {
                perror("accept");
                return CNTR_ERROR;
            }
            /* Sí; es cliente */
sondea_salida:
            FD_ZERO(&lst_df_sondear_lect);
            FD_ZERO(&lst_df_sondear_escr);
            FD_SET(ruta->toma->cliente, &lst_df_sondear_lect);
            FD_SET(ruta->toma->cliente, &lst_df_sondear_escr);
        } else {
            if (   FD_ISSET(ruta->toma->cliente, &lst_df_sondear_lect)
                && FD_ISSET(ruta->toma->cliente, &lst_df_sondear_escr))
                break;
            else
                goto sondea_salida;
        }
    }
    return CNTR_HECHO;
}

/* cntr_cierra_toma -- Cierra toma especificada en 'opcion' y de la manera
                       en que, también allí, se especifique */

int 
cntr_cierra_toma(t_cntr_ruta *ruta, t_elector_es opcion)
{
    if (ruta == NULL || ruta->toma == NULL)
        return CNTR_ERROR;

    int *toma_es[2] = {NULL, NULL};

    if (opcion.es_servidor)
        toma_es[0] = &ruta->toma->servidor;

    if (opcion.es_cliente)
        toma_es[1] = &ruta->toma->cliente;

    for (int i = 0; i < 2; i++) {
        if (toma_es[i] == NULL)
            continue;
        if (*toma_es[i] != CNTR_DF_NULO) {
            if (opcion.forzar) {
                /* Forzar cierre y evitar (TIME_WAIT) */
                struct linger so_linger;
                so_linger.l_onoff  = 1;
                so_linger.l_linger = 0;
                setsockopt(*toma_es[i], SOL_SOCKET, SO_LINGER,
                           &so_linger, sizeof(so_linger));
                goto a_trochemoche;
            }
            if (shutdown(*toma_es[i], SHUT_RDWR) < 0) {
                perror("shutdown");
                return CNTR_ERROR;
            }
a_trochemoche:
            if (close(*toma_es[i]) < 0) {
                perror("cierra");
                return CNTR_ERROR;
            }
            *toma_es[i] = CNTR_DF_NULO;
        } else
            return CNTR_ERROR;
    }

    return CNTR_HECHO;
}