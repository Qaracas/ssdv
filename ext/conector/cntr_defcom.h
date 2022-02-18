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

#include <stdio.h>

#ifndef DEFCOM_H
#define DEFCOM_H

#define CNTR_HECHO      (0)
#define CNTR_ERROR      (-1)
#define CNTR_DF_NULO    (-1)
#define CNTR_REINTENTAR (-4)

#define cntr_ltd(x) (sizeof(x) / sizeof((x)[0]))

#define cntr_asigmem(puntero, tipo, cabida, mensaje) \
    do { \
        if ((puntero = (tipo) malloc(cabida)) == 0) \
            printf("%s: fallo reservando %d octetos de memoria", \
                   mensaje, (int)cabida); \
    } while(0)

#ifndef T_CTRN_VERDAD
#define T_CTRN_VERDAD
typedef enum cntr_verdad {
    cntr_falso  = 0,
    cntr_cierto = 1
} t_ctrn_verdad;
#endif

typedef struct cntr_resultado {
    int cntr_errno;      /* Código de error guardado en errno (errno.h)     */
    char *cntr_strerror; /* Texto devuelto por strerror(errno)              */
    int codigo;          /* Código con el resultado de una función          */
    char *texto_error;   /* Descripción del error si resultado = CNTR_ERROR */
} t_cntr_resultado;

/* cntr_nuevo_resultado --
 *
 * Crea nuevo resultado relativo a la ejecución de una función
 */

t_cntr_resultado *
cntr_nuevo_resultado(int cntr_errno, int codigo, char *texto_error);

/* cntr_msj_error --
 *
 * Forma mensaje de error a partir de un resultado y un texto dado
 */

char *
cntr_msj_error(const char *desc, ...);

#endif /* DEFCOM_H */