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

#include <string.h>
#include <stdlib.h>

#include "cntr_defcom.h"
#include "cntr_ruta.h"
#include "cntr_serie.h"

t_cntr_pieza *cntr_serie;

int
cntr_pon_ruta_en_serie(const char *nombre_ruta, t_cntr_pieza **serie)
{
    if (nombre_ruta == NULL )
        return CNTR_ERROR;

    if (*serie == NULL) {
        cntr_asigmem(*serie, t_cntr_pieza *,
                     sizeof(t_cntr_pieza), "cntr_pon_ruta_en_serie");
        (*serie)->ruta = NULL;
        (*serie)->siguiente = NULL;
        if (cntr_nueva_ruta(nombre_ruta, &(*serie)->ruta) == CNTR_ERROR) {
            cntr_borra_ruta((*serie)->ruta);
            free(*serie);
            return CNTR_ERROR;
        }
    } else if (strcmp(nombre_ruta, (*serie)->ruta->nombre) == 0)
        return CNTR_ERROR;
    else
        return cntr_pon_ruta_en_serie(nombre_ruta, &(*serie)->siguiente);
    return CNTR_HECHO;
}

t_cntr_pieza
*cntr_borra_ruta_en_serie(const char *nombre_ruta, t_cntr_pieza *serie)
{
    if (nombre_ruta == NULL || serie == NULL)
        return NULL;

    if (strcmp(nombre_ruta, serie->ruta->nombre) == 0) {
        cntr_borra_ruta(serie->ruta);
        serie = serie->siguiente;
        free(serie->siguiente);
    } else {
        t_cntr_pieza *soldar = cntr_borra_ruta_en_serie(nombre_ruta,
                                                        serie->siguiente);
        free(serie->siguiente);
        serie->siguiente = soldar;
    }
    return serie->siguiente;
}

t_cntr_ruta
*cntr_existe_ruta_en_serie(const char *nombre_ruta, t_cntr_pieza *serie)
{
    if (nombre_ruta == NULL || serie == NULL)
        return NULL;
    else if (strcmp(nombre_ruta, serie->ruta->nombre) == 0)
        return serie->ruta;
    else
        return cntr_existe_ruta_en_serie(nombre_ruta, serie->siguiente);
    return NULL;
}