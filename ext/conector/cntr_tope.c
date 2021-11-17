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
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "cntr_defcom.h"
#include "cntr_ruta.h"
#include "cntr_toma.h"
#include "cntr_tope.h"

/* cntr_nuevo_tope */

int
cntr_nuevo_tope(size_t max, t_cntr_tope **tope)
{
    if (tope == NULL)
        return CNTR_ERROR;
    cntr_asigmem(*tope, t_cntr_tope *,
                 sizeof(t_cntr_tope), "cntr_nuevo_tope");
    cntr_asigmem((*tope)->datos, char *,
                 max, "cntr_nuevo_tope");
    (*tope)->max = max;
    (*tope)->ldatos = 0;
    (*tope)->ptrreg = 0;
    bzero((*tope)->datos, max);
    if (*tope == NULL)
        return CNTR_ERROR;
    else
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

/* cntr_recb_llena_tope */

int
cntr_recb_llena_tope(t_cntr_toma_es *toma, t_cntr_tope *tope)
{
    if (   toma == NULL || tope == NULL
        || toma->cliente == CNTR_DF_NULO)
    return CNTR_ERROR;

    tope->ldatos = recv(toma->cliente,
                        tope->datos + tope->ptrreg,
                        tope->max - tope->ptrreg, 0);

    if (tope->ldatos <= 0) {
        if (tope->ptrreg > 0) {
            bzero(tope->datos + tope->ptrreg,
                  tope->max - tope->ptrreg);
            /* Se envía el remanente, al no recibirse ya nada por la toma */
            return CNTR_TOPE_RESTO;
        }else
            return CNTR_TOPE_VACIO;
    }

    /* Limpiar el sobrante */
    if (((size_t)tope->ldatos + tope->ptrreg) < tope->max)
        bzero(tope->datos + ((size_t)tope->ldatos + tope->ptrreg),
              tope->max - ((size_t)tope->ldatos + tope->ptrreg));

    tope->ptareg = tope->ptrreg;
    tope->ptrreg = 0;

    return CNTR_HECHO;
}