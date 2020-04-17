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

#include "cntr_defcom.h"
#include "cntr_ruta.h"
#include "cntr_toma.h"
#include "cntr_stoma.h"

int
cntr_nueva_toma(t_cntr_ruta *ruta)
{
    if (ruta == NULL)
        return CNTR_ERROR;

    cntr_asigmem(ruta->toma, t_cntr_toma_es *,
                 sizeof(t_cntr_toma_es), "cntr_nueva_toma");
    ruta->toma->servidor = CNTR_DF_NULO;
    ruta->toma->cliente  = CNTR_DF_NULO;

    return CNTR_HECHO;
}

int
cntr_nueva_toma_servidor(t_cntr_ruta *ruta)
{
    if (ruta == NULL)
        return CNTR_ERROR;

    struct addrinfo *rp;

    for (rp = ruta->stoma; rp != NULL; rp = rp->ai_next) {
        /* Crear toma de entrada y guardar df asociado a ella */
        ruta->toma->servidor = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (ruta->toma->servidor == -1)
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

int 
cntr_cierra_toma(t_cntr_ruta *ruta, t_elector_es opcion)
{
    if (ruta == NULL || ruta->toma == NULL)
        return CNTR_ERROR;

    int *toma_es = NULL;

    if (opcion.es_servidor)
        toma_es = &ruta->toma->servidor;

    if (opcion.es_cliente)
        toma_es = &ruta->toma->cliente;

    if (toma_es != NULL) {
//    /* Leemos lo que quede antes de cerrar la toma */
//    while (1) {
//        flujo->lgtope = recv(rt.toma->cliente,
//                             flujo->tope + flujo->ptrreg,
//                             flujo->max - flujo->ptrreg, MSG_DONTWAIT);
//        if ((   flujo->lgtope < 0
//             && (EAGAIN == errno || EWOULDBLOCK == errno))
//            || flujo->lgtope == 0)
//            break;
//    }
        if (opcion.forzar) {
            /* Forzar cierre y evitar (TIME_WAIT) */
            struct linger so_linger;
            so_linger.l_onoff  = 1;
            so_linger.l_linger = 0;
            setsockopt(*toma_es, SOL_SOCKET, SO_LINGER,
                       &so_linger, sizeof(so_linger));
        }
        if (shutdown(*toma_es, SHUT_RDWR) < 0) {
            perror("shutdown");
            return CNTR_ERROR;
        }
        if (close(*toma_es) < 0) {
            perror("cierra");
            return CNTR_ERROR;
        }
        *toma_es = CNTR_DF_NULO;
    }

    return CNTR_HECHO;
}

void
cntr_borra_toma(t_cntr_ruta *ruta)
{
    if (ruta != NULL && ruta->toma != NULL)
        free(ruta->toma);
}