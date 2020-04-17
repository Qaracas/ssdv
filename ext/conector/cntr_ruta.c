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
#include <string.h>

#include "cntr_defcom.h"
#include "cntr_cdntxt.h"
#include "cntr_ruta.h"
#include "cntr_toma.h"
#include "cntr_stoma.h"

/* cntr_nueva_ruta -- Crea nueva ruta a partir de un fichero especial */

int
cntr_nueva_ruta(const char *nombre, t_cntr_ruta *ruta)
{
    if (   nombre == NULL
        || cuenta_crtrs(nombre, '/') == CNTR_ERROR
        || cuenta_crtrs(nombre, '/') != 6
        || strlen(nombre) < 12
        || caracter_ini(nombre) == CNTR_ERROR
        || caracter_fin(nombre) == CNTR_ERROR
        || caracter_ini(nombre) != '/'
        || caracter_fin(nombre) == '/'
       )
        return CNTR_ERROR;

    char *v_nombre;
    cntr_asigmem(v_nombre, char *,
                 strlen((const char *) nombre) + 1,
                 "cntr_nueva_ruta");
    strcpy(v_nombre, (const char *) nombre);

    unsigned int c;
    char *campo[6];
    campo[0] = strtok(v_nombre, "/");
    for (c = 0; (c < cntr_ltd(campo) - 1) && campo[c] != NULL;)
        campo[++c] = strtok(NULL, "/");

    if (   c != (cntr_ltd(campo) - 1)
        || strcmp(campo[0], "ired") != 0
        || strcmp(campo[1], "tcp") != 0
        || !es_numero(campo[3])
        || !es_numero(campo[5])
       )
        goto error;

    if (ruta == NULL)
        cntr_asigmem(ruta, t_cntr_ruta *,
                     sizeof(t_cntr_ruta), "cntr_nueva_ruta");
    ruta->stoma = NULL;
    ruta->toma  = NULL;
    ruta->local = cntr_falso;

    if (   strcmp(campo[2], "0") == 0
        && strcmp(campo[3], "0") == 0
        && strcmp(campo[4], "0") != 0
        && atoi(campo[5]) > 0
        && cntr_nueva_stoma(campo[4], campo[5], ruta) == CNTR_HECHO)
    { /* Cliente */
        goto error; /* De momento no */
    } else if 
       (   strcmp(campo[4], "0") == 0
        && strcmp(campo[5], "0") == 0
        && atoi(campo[3]) > 0
        && cntr_nueva_stoma(campo[2], campo[3], ruta) == CNTR_HECHO
        && ruta->local)
    { /* Servidor */
        goto hecho;
    } else {
        goto error;
    }

hecho:
    free(v_nombre);

    cntr_asigmem(ruta->nombre, char *,
                 strlen((const char *) nombre) + 1,
                 "cntr_nueva_ruta");
    strcpy(ruta->nombre, (const char *) nombre);

    cntr_nueva_toma(ruta);

    return CNTR_HECHO;
error:
    free(v_nombre);
    cntr_borra_stoma(ruta);
    return CNTR_ERROR;
}

/* cntr_borra_ruta -- Libera memoria y destruye toma */

void
cntr_borra_ruta(t_cntr_ruta *ruta)
{
    if (ruta != NULL) {
        if (ruta->nombre != NULL) free(ruta->nombre);
        if (ruta->toma   != NULL) free(ruta->toma);
        if (ruta->stoma  != NULL) cntr_borra_stoma(ruta);
    }
}