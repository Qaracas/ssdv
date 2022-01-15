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

#define _GNU_SOURCE

#include <errno.h>
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

/* cntr_nueva_toma */

t_cntr_toma_es *
cntr_nueva_toma(t_cntr_ruta *ruta)
{
    if (ruta == NULL)
        return NULL;

    cntr_asigmem(ruta->toma, t_cntr_toma_es *,
                 sizeof(t_cntr_toma_es), "cntr_nueva_toma");
    ruta->toma->servidor = CNTR_DF_NULO;
    ruta->toma->cliente  = CNTR_DF_NULO;

    return ruta->toma;
}

/* cntr_borra_toma */

void
cntr_borra_toma(t_cntr_toma_es *toma)
{
    free(toma);
    toma = NULL;
}

/* cntr_nueva_estructura_datos_toma */

t_datos_toma *
cntr_nueva_estructura_datos_toma(t_cntr_toma_es *toma, char *sr, size_t tpm)
{
    cntr_asigmem(toma->pila, t_datos_toma *,
                 sizeof(t_datos_toma), "cntr_nueva_estructura_datos_toma");
    toma->pila->lgtreg = 0;
    toma->pila->en_uso = 1;

    toma->pila->tsr = strlen((const char *) sr);
    cntr_asigmem(toma->pila->sdrt, char *,
                 toma->pila->tsr + 1, "cntr_nueva_estructura_datos_toma");
    strcpy(toma->pila->sdrt, (const char *) sr);

    cntr_nuevo_tope(tpm, &toma->pila->tope);

    return toma->pila;
}

/* cntr_envia_a_toma */

int
cntr_envia_toma(t_cntr_toma_es *toma, const void *datos, size_t bulto)
{
    if (send(toma->cliente, datos, bulto, 0) < 0) {
        perror("send");
        return CNTR_ERROR;
    }
    return CNTR_HECHO;
}

/* cntr_recibe_toma */

char *
cntr_recibe_linea_toma(t_cntr_toma_es *toma, char **sdrt, size_t *tsr,
                       t_cntr_resultado **resul)
{
    int recbt;
    t_cntr_tope *tope = toma->pila->tope;

    if (tope->ldatos == 0) {
        errno = 0;
        recbt = cntr_rcbl_llena_tope(toma);
        switch (recbt) {
            case CNTR_TOPE_RESTO:
                return tope->datos;
            case CNTR_TOPE_VACIO:
                return NULL;
            case CNTR_ERROR:
                *resul = cntr_nuevo_resultado(errno, CNTR_ERROR,
                                              "error leyendo toma");
                break;
        }
    } else {
        /* Apunta al siguiente registro del tope */
        tope->ptrreg += toma->pila->lgtreg + toma->pila->tsr;
    }

    /* Apuntar al siguiente separador de registro */
    *sdrt = strstr((const char*) tope->datos + tope->ptrreg,
                       (const char*) toma->pila->sdrt);
    *tsr = toma->pila->tsr;

    if (*sdrt == NULL) {
        if (tope->ptrreg == 0) {
            *resul = cntr_nuevo_resultado(0, CNTR_ERROR,
                                          "desbordamiento de pila");
            return NULL;
        }
        *tsr = 0;
        /* Copia lo que nos queda por leer al inicio del tope */
        memcpy(tope->datos,
               (const void *) (tope->datos + tope->ptrreg),
               (tope->ldatos + tope->ptareg) - tope->ptrreg);
        tope->ptrreg = (tope->ldatos + tope->ptareg) - tope->ptrreg;
        tope->ldatos = 0;
        return cntr_recibe_linea_toma(toma, sdrt, tsr, resul);
    }

    /* Tamaño del registro */
    toma->pila->lgtreg = *sdrt - (tope->datos + tope->ptrreg);

    return tope->datos + tope->ptrreg;
}

/* cntr_recibe_flujo_toma */

char *
cntr_recibe_flujo_toma(t_cntr_toma_es *toma, char **sdrt, size_t *tsr,
                       t_cntr_resultado **resul)
{
    int recbt;
    t_cntr_tope *tope = toma->pila->tope;

    errno = 0;
    recbt = cntr_rcbf_llena_tope(toma);

    if (recbt == CNTR_ERROR) {
        *resul = cntr_nuevo_resultado(errno, CNTR_ERROR,
                                      "error leyendo toma");
        return NULL;
    } else if (recbt == CNTR_TOPE_VACIO) {
        return NULL;
    }

    /* Tamaño del registro */
    toma->pila->lgtreg = tope->ldatos;

    /* Variable RT no tiene sentido leyendo flujos */
    *sdrt = NULL;
    *tsr = 0;

    return tope->datos;
}

/* cntr_pon_a_escuchar_toma */

int
cntr_pon_a_escuchar_toma(t_cntr_toma_es *toma)
{
    if (   toma == NULL || toma->infred == NULL
        || toma->servidor != CNTR_DF_NULO
        || toma->local == cntr_falso)
        return CNTR_ERROR;

    struct addrinfo *rp;

    for (rp = toma->infred; rp != NULL; rp = rp->ai_next) {
        /* Crear toma de entrada y guardar df asociado a ella */
        toma->servidor = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (toma->servidor == CNTR_DF_NULO)
            continue;
        /* Asociar toma de entrada a una dirección IP y un puerto */
        int activo = 1;
        setsockopt(toma->servidor, SOL_SOCKET, SO_REUSEADDR,
                   &activo, sizeof(activo));
        if (bind(toma->servidor, rp->ai_addr, rp->ai_addrlen) == 0)
            break; /* Hecho */
        close(toma->servidor);
    }

    if (rp == NULL) {
        perror("bind");
        return CNTR_ERROR;
    }

    /* Poner toma en modo escucha */
    if (listen(toma->servidor, CNTR_MAX_PENDIENTES) < 0) {
        perror("listen");
        return CNTR_ERROR;
    }

    cntr_borra_infred(toma); /* Ya no se necesita */
    return CNTR_HECHO;
}

/* cntr_trae_primer_cliente_toma */

int
cntr_trae_primer_cliente_toma(t_cntr_toma_es *toma, struct sockaddr *cliente)
{
    if (   toma == NULL
        || toma->servidor == CNTR_DF_NULO )
        return CNTR_ERROR;

    socklen_t lnt = (socklen_t) sizeof(*cliente);

    fd_set lst_df_sondear_lect, lst_df_sondear_escr;

    /* Borrar colección de tomas E/S a sondear */
    FD_ZERO(&lst_df_sondear_lect);
    FD_ZERO(&lst_df_sondear_escr);
    /* Sondear toma de escucha */
    FD_SET(toma->servidor, &lst_df_sondear_lect);

    while (1) {
        /* Esperar a que los df estén listos para realizar operaciones de E/S */
        if (select(FD_SETSIZE, &lst_df_sondear_lect, &lst_df_sondear_escr,
                   NULL, NULL) < 0) {
            perror("select");
            return CNTR_ERROR;
        }
        /* Atender tomas con eventos de entrada pendientes */
        if (FD_ISSET(toma->servidor, &lst_df_sondear_lect)) {
            /* Extraer primera conexión de la cola de conexiones */
            toma->cliente = accept(toma->servidor, cliente, &lnt);
            /* ¿Es cliente? */
            if (toma->cliente < 0) {
                perror("accept");
                return CNTR_ERROR;
            }
            /* Sí; es cliente */
sondea_salida:
            FD_ZERO(&lst_df_sondear_lect);
            FD_ZERO(&lst_df_sondear_escr);
            FD_SET(toma->cliente, &lst_df_sondear_lect);
            FD_SET(toma->cliente, &lst_df_sondear_escr);
        } else {
            if (   FD_ISSET(toma->cliente, &lst_df_sondear_lect)
                && FD_ISSET(toma->cliente, &lst_df_sondear_escr))
                break;
            else
                goto sondea_salida;
        }
    }
    return CNTR_HECHO;
}

/* cntr_cierra_toma */

int
cntr_cierra_toma(t_cntr_toma_es *toma, t_elector_es opcion)
{
    if (toma == NULL)
        return CNTR_ERROR;

    int *toma_es[2] = {NULL, NULL};

    if (opcion.es_servidor)
        toma_es[0] = &toma->servidor;

    if (opcion.es_cliente)
        toma_es[1] = &toma->cliente;

    for (long unsigned int i = 0; i < cntr_ltd(toma_es); i++) {
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
