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

#ifndef SERIE_H
#define SERIE_H

struct cntr_ruta;
typedef struct cntr_ruta t_cntr_ruta;

typedef struct cntr_pieza {
    t_cntr_ruta       *ruta;       /* Ruta de conexión */
    struct cntr_pieza *siguiente;  /* Siguiente pieza  */
} t_cntr_pieza;

/* cntr_pon_ruta_en_serie --
 *
 * Añade una nueva ruta a la cadena
 */

t_cntr_ruta *
cntr_pon_ruta_en_serie(t_cntr_ruta *ruta);

/* cntr_borra_ruta_de_serie --
 *
 * Borra una ruta de la cadena
 */

void
cntr_borra_ruta_de_serie(const char *nombre_ruta);

/* cntr_busca_ruta_en_serie --
 *
 * Busca una ruta en la cadena
 */

t_cntr_ruta *
cntr_busca_ruta_en_serie(const char *nombre_ruta);

#endif /* SERIE_H */