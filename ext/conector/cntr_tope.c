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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "cntr_defcom.h"
#include "cntr_ruta.h"
#include "cntr_toma.h"
#include "cntr_tope.h"
#include "cntr_capa_tls.h"

/* cntr_nuevo_tope */

int
cntr_nuevo_tope(t_cntr_tope **tope, size_t bulto)
{
    cntr_limpia_error_simple();

    if (tope == NULL) {
        cntr_error(CNTR_ERROR, cntr_msj_error("%s %s",
                             "cntr_nuevo_tope()",
                             "tope nulo"));
        return CNTR_ERROR;
    }

    size_t v_bulto;

    if (bulto == 0)
        v_bulto = CNTR_TOPE_MAX_X_DEF;
    else
        v_bulto = bulto;

    cntr_asigmem(*tope, t_cntr_tope *,
                 sizeof(t_cntr_tope), "cntr_nuevo_tope");
    cntr_asigmem((*tope)->datos, char *,
                 v_bulto, "cntr_nuevo_tope");
    (*tope)->bulto = v_bulto;
    (*tope)->ldatos = 0;
    (*tope)->ptrreg = 0;
    (*tope)->ptareg = 0;
    bzero((*tope)->datos, v_bulto);

    return CNTR_HECHO;
}

/* cntr_borra_tope */

void
cntr_borra_tope(t_cntr_tope *tope)
{
    free(tope->datos);
    free(tope);
    tope->datos = NULL;
    tope = NULL;
}

/* cntr_recibe_datos */

ssize_t
cntr_recibe_datos(t_capa_gnutls *capatls, int df_cliente, void *tope,
                  size_t bulto)
{
    extern int errno;
    (void) capatls;
    ssize_t resul;

    cntr_limpia_error(errno);
    if (   (resul = recv(df_cliente, tope, bulto, 0)) < 0) {
        if (errno != EAGAIN && errno!=  EWOULDBLOCK) {
            cntr_error(errno, cntr_msj_error("%s %s",
                                 "cntr_recibe_datos()",
                                 strerror(errno)));
            return CNTR_ERROR;
        } else {
            return 0;
        }
    }
    return resul;
}

/* cntr_rcbl_llena_tope */

int
cntr_rcbl_llena_tope(t_cntr_toma_es *toma)
{
    t_cntr_tope *tope = toma->pila->tope;

    if (   toma == NULL || tope == NULL
        || toma->cliente == CNTR_DF_NULO)
    return CNTR_ERROR;

    tope->ldatos = (*toma->recibe)(toma->gtls, toma->cliente,
                                   tope->datos + tope->ptrreg,
                                   tope->bulto - tope->ptrreg);

    switch (tope->ldatos) {
        case CNTR_REINTENTAR:
            return CNTR_REINTENTAR;
        case 0:
            if (tope->ptrreg > 0) {
                bzero(tope->datos + tope->ptrreg,
                      tope->bulto - tope->ptrreg);
                /* No hay datos en la toma, y se envía el remanente */
                return CNTR_TOPE_RESTO;
            } else
                return CNTR_TOPE_VACIO;
        case CNTR_ERROR:
            return CNTR_ERROR;
    }

    /* Limpiar el sobrante */
    if (((size_t)tope->ldatos + tope->ptrreg) < tope->bulto)
        bzero(tope->datos + (tope->ldatos + tope->ptrreg),
              tope->bulto - (tope->ldatos + tope->ptrreg));

    tope->ptareg = tope->ptrreg;
    tope->ptrreg = 0;

    return CNTR_HECHO;
}

/* cntr_rcbf_llena_tope */

int
cntr_rcbf_llena_tope(t_cntr_toma_es *toma)
{
    t_cntr_tope *tope = toma->pila->tope;

    if (   toma == NULL || tope == NULL
        || toma->cliente == CNTR_DF_NULO)
    return CNTR_ERROR;

    bzero(tope->datos, tope->bulto);
    tope->ldatos = (*toma->recibe)(toma->gtls, toma->cliente, tope->datos,
                                   tope->bulto);

    switch (tope->ldatos) {
        case CNTR_REINTENTAR:
            return CNTR_REINTENTAR;
        case 0:
            return CNTR_TOPE_VACIO;
        case CNTR_ERROR:
            return CNTR_ERROR;
    }

    return CNTR_HECHO;
}