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

#ifndef RUTA_H
#define RUTA_H

struct addrinfo;

struct cntr_toma_es;
typedef struct cntr_toma_es t_cntr_toma_es;

#ifndef T_CTRN_VERDAD
#define T_CTRN_VERDAD
typedef enum cntr_verdad {
    cntr_falso  = 0,
    cntr_cierto = 1
} t_ctrn_verdad;
#endif

typedef struct cntr_ruta {
    char            *nombre;        /* Identificador de ruta    */
    char            *tipo;          /* Normalmente ired         */
    char            *protocolo;     /* O capa (TCP/TLS)         */
    char            *nodo_local;    /* Nombre o dir IP del nodo */
    char            *puerto_local;  /* Puerto de E/S            */
    char            *nodo_remoto;   /* Nombre o dir IP del nodo */
    char            *puerto_remoto; /* Puerto remoto de E/S     */
    t_cntr_toma_es  *toma;          /* Toma de datos de E/S     */
    t_ctrn_verdad   local  : 1;     /* ¿Ruta local?             */
    t_ctrn_verdad   segura : 1;     /* ¿Es segura?              */
} t_cntr_ruta;

/* cntr_nueva_ruta --
 *
 * Crea nueva ruta a partir de un fichero especial
 */

int
cntr_nueva_ruta(const char *nombre, t_cntr_ruta **ruta);

/* cntr_borra_ruta --
 *
 * Libera memoria y destruye toma
 */

void
cntr_borra_ruta(t_cntr_ruta *ruta);

#endif /* RUTA_H */