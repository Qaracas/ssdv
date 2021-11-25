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

#ifndef TOPE_H
#define TOPE_H

#define CNTR_TOPE_VACIO (-2)
#define CNTR_TOPE_RESTO (-3)

struct cntr_ruta;
typedef struct cntr_ruta t_cntr_ruta;

typedef struct cntr_tope {
    size_t  bulto;  /* Volumen o capacidad del tope */
    ssize_t ldatos; /* Cantidad datos almacenados   */
    char    *datos; /* Datos almacenados            */
    int     ptrreg; /* Inicio registro actual       */
    int     ptareg; /* Inicio registro anterior     */
} t_cntr_tope;

/* cntr_nuevo_tope --
 *
 * Crea nuevo tope de tamaño 'bulto'
 */

int cntr_nuevo_tope(size_t bulto, t_cntr_tope **tope);

/* cntr_borra_tope --
 *
 * Libera memoria y destruye tope
 */

void cntr_borra_tope(t_cntr_tope *tope);

/* cntr_recb_llena_tope --
 *
 * Llenar tope con los datos recibidos de la toma asociada a una ruta
 */

int cntr_recb_llena_tope(t_cntr_toma_es *toma, t_cntr_tope *tope);

#endif /* TOPE_H */