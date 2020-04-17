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

#ifndef TOMA_H
#define TOMA_H

/* Tome máximo para cola de conexiones pendientes */
#define CNTR_MAX_PENDIENTES 100

struct cntr_ruta;
typedef struct cntr_ruta t_cntr_ruta;

typedef struct cntr_toma_es {
    int servidor; /* Descriptor servidor en modo escucha   */
    int cliente;  /* Descriptor cliente (lectura/escritura)*/
} t_cntr_toma_es;

typedef struct elector_es {
    unsigned int es_servidor : 1;
    unsigned int es_cliente  : 1;
    unsigned int forzar      : 1;
} t_elector_es;

int cntr_nueva_toma(t_cntr_ruta *ruta);

int cntr_nueva_toma_servidor(t_cntr_ruta *ruta);

int cntr_cierra_toma(t_cntr_ruta *ruta, t_elector_es opcion);

void cntr_borra_toma(t_cntr_ruta *ruta);

#endif /* TOMA_H */