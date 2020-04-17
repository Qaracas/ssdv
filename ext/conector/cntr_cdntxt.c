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
#include <ctype.h>

#include "cntr_defcom.h"
#include "cntr_cdntxt.h"

int
es_numero(const char *txt)
{
    if (txt != NULL) {
        int j = strlen(txt);
        while(j--) {
            if(isdigit((int)txt[j]))
                continue;
            return cntr_falso;
        }
        return cntr_cierto;
    }
    return cntr_falso;
}

int
caracter_fin(const char *txt)
{
    if(*txt)
        return txt[strlen(txt + 1)];
    else
        return CNTR_ERROR;
}

int
caracter_ini(const char *txt)
{
    if(*txt)
        return txt[0];
    else
        return CNTR_ERROR;
}

int
cuenta_crtrs(const char *txt, char c)
{
    if(*txt) {
        int i, cnt = 0;
        for(i = 0; txt[i]; i++)
            if(txt[i] == c)
                cnt++;
        return cnt;
    } else
        return CNTR_ERROR;
}