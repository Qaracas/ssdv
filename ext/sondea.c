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
 * Deberías haber recibido una copia de la Licencia Pública General
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

#define TIEMPOMUERTO 5

#define _GNU_SOURCE         /* Ver feature_test_macros(7) */
#include <poll.h>
#include <assert.h>
#include <errno.h>

#include "gawkapi.h"

static const gawk_api_t *api;	/* Conveniencia para usar macros */
static awk_ext_id_t ext_id;
static const char *ext_version = "extensión sondea E/S: versión 1.0";

static awk_bool_t (*init_func)(void) = NULL;

int plugin_is_GPL_compatible;

/* haz_espera --- proporciona función poll cargada dinámicamente */

static awk_value_t *
haz_espera(int nargs, awk_value_t *result, struct awk_ext_func *unused)
{
    int ret;
    struct pollfd dsf[2];
    awk_value_t fichero, tipo;
    const awk_input_buf_t *entrada;
    const awk_output_buf_t *salida;

    assert(result != NULL);

    /* Sólo acepta 2 argumentos */
    if (nargs != 2) {
        lintwarn(ext_id, "espera: número de parámetros incorrecto, requiere 2");
        return make_number(-2, result);
    }

    /* */
    if (   ! get_argument(0, AWK_STRING, & fichero)
        || ! get_argument(1, AWK_STRING, & tipo))
    {
        lintwarn(ext_id, "espera: parámetros incorrectos");
        return make_number(-2, result);
    }
    

    if ( ! get_file(fichero.str_value.str, fichero.str_value.len,
                    tipo.str_value.str, -1, &entrada, &salida))
    {
        update_ERRNO_int(EIO);
        update_ERRNO_string("Error de E/S");
        lintwarn(ext_id, "espera: error obteniendo información de E/S");
        return make_number(-2, result);
    }

	/* averigua si es posible leer */
	dsf[0].fd = entrada->fd;
	dsf[0].events = POLLIN;

	/* averigua si es posible escribir */
	dsf[1].fd = salida->fd;
	dsf[1].events = POLLOUT;

    ret = poll(&dsf, 2, TIEMPOMUERTO * 1000);

    if (ret == 0) {
        update_ERRNO_int(ENOENT);
        update_ERRNO_string("Tiempo muerto");
        lintwarn(ext_id, "espera: error, tiempo muerto superado");
    }

    if (ret < 0) {
        lintwarn(ext_id, "espera: error inesperado");
    }

    /* Pon resultado */
    return make_number(ret, result);
}

static awk_ext_func_t lista_de_funciones[] = {
	{ "espera", haz_espera, 0, 0, awk_false, NULL },
};

/* Define función de carga */

dl_load_func(lista_de_funciones, sondea, "")