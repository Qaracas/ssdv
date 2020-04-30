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

#include "cntr_defcom.h"
#include "cntr_ruta.h"
#include "cntr_serie.h"

int
pon_en_serie(t_cntr_ruta *ruta, t_cntr_pieza *serie)
{
    if (ruta == NULL )
        return CNTR_ERROR;

    if (serie == NULL) {
        ntr_asigmem(serie, t_cntr_pieza *,
                    sizeof(t_cntr_pieza), "pon_en_serie");
        serie->ruta = ruta;
        serie->otra = NULL;
    } else if (strcmp(ruta->name, serie->ruta->nombre) == 0)
        return CNTR_ERROR;
    else
        return pon_en_serie(ruta, serie->otra);
    return CNTR_HECHO;
}

t_ctrn_verdad
existe_en_serie(char *ruta, t_cntr_pieza *serie)
{
    if (ruta == NULL || serie == NULL)
        return cntr_falso;
    else if (strcmp(ruta, serie->ruta->nombre) == 0)
        return cntr_cierto;
    else
        return existe_en_serie(ruta, serie->otra);
    return cntr_falso;
}